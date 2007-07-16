/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:10:2005   14:14 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "Projectile.h"
#include "Bullet.h"
#include "WeaponSystem.h"
#include "ISerialize.h"
#include "IGameObject.h"

#include <IEntitySystem.h>
#include <ISound.h>
#include <IItemSystem.h>
#include <IAgent.h>
#include "ItemParamReader.h"
#include "GameRules.h"

#include "HUD/HUD.h"

EntityId CProjectile::m_currentlyTracked = 0;

//------------------------------------------------------------------------
CProjectile::CProjectile()
: m_whizSoundId(INVALID_SOUNDID),
	m_trailSoundId(INVALID_SOUNDID),
	m_trailEffectId(-1),
	m_trailUnderWaterId(-1),
	m_stuck(0),
	m_stickyCollisions(0),
	m_pPhysicalEntity(0),
	m_pStickyBuddy(0),
	m_pAmmoParams(0),
	m_destroying(false),
	m_remote(false),
	m_totalLifetime(0.0f),
	m_scaledEffectval(0.0f),
	m_obstructObject(0),
	m_damageDropPerMeter(0.0f),
  m_hitTypeId(0),
	m_scaledEffectSignaled(false),
	m_stickToActor(false),
	m_hitListener(false),
	m_hitPoints(-1)
{
}

//------------------------------------------------------------------------
CProjectile::~CProjectile()
{
	GetGameObject()->ReleasePhysics(this);
	GetGameObject()->EnablePhysicsEvent(false, eEPE_OnCollisionLogged);

	if (m_obstructObject)
		gEnv->pPhysicalWorld->DestroyPhysicalEntity(m_obstructObject);

	if(m_hitListener)
		if (CGameRules * pGameRules = g_pGame->GetGameRules())
			pGameRules->RemoveHitListener(this);
	
	if (g_pGame)
		g_pGame->GetWeaponSystem()->RemoveProjectile(this);
}

//------------------------------------------------------------------------
bool CProjectile::SetProfile( uint8 profile )
{
	if (m_pAmmoParams->physicalizationType == ePT_None)
		return true;

	Vec3 spin(m_pAmmoParams->spin);
	Vec3 spinRandom(BiRandom(m_pAmmoParams->spinRandom.x), BiRandom(m_pAmmoParams->spinRandom.y), BiRandom(m_pAmmoParams->spinRandom.z));
	spin += spinRandom;
	spin = DEG2RAD(spin);

	switch (profile)
	{
	case ePT_Particle:
		{
			if (m_pAmmoParams->pParticleParams)
			{
				m_pAmmoParams->pParticleParams->wspin = spin;
				if (!m_initialDir.IsZero() && !gEnv->bServer)
					m_pAmmoParams->pParticleParams->heading=m_initialDir;
			}

			SEntityPhysicalizeParams params;
			params.type = PE_PARTICLE;
			params.mass = m_pAmmoParams->mass;
			if (m_pAmmoParams->pParticleParams)
			{
				params.pParticle = m_pAmmoParams->pParticleParams;
			}


			GetEntity()->Physicalize(params);
		}
		break;
	case ePT_Rigid:
		{
			SEntityPhysicalizeParams params;
			params.type = PE_RIGID;
			params.mass = m_pAmmoParams->mass;
			params.nSlot = 0;

			GetEntity()->Physicalize(params);

			pe_action_set_velocity velocity;
			m_pPhysicalEntity = GetEntity()->GetPhysics();
			velocity.w = spin;
			m_pPhysicalEntity->Action(&velocity);

			if (m_pAmmoParams->pSurfaceType)
			{
				int sfid = m_pAmmoParams->pSurfaceType->GetId();

				pe_params_part part;
				part.ipart = 0;

				GetEntity()->GetPhysics()->GetParams(&part);
				for (int i=0; i<part.nMats; i++)
					part.pMatMapping[i] = sfid;
			}
		}
		break;
	}

	m_pPhysicalEntity = GetEntity()->GetPhysics();

	if (m_pPhysicalEntity)
	{
		pe_simulation_params simulation;
		simulation.maxLoggedCollisions = m_pAmmoParams->maxLoggedCollisions;

		pe_params_flags flags;
		flags.flagsOR = pef_log_collisions|(m_pAmmoParams->traceable?pef_traceable:0);

		m_pPhysicalEntity->SetParams(&simulation);
		m_pPhysicalEntity->SetParams(&flags);
	}

	return true;
}

//------------------------------------------------------------------------
bool CProjectile::SerializeProfile( TSerialize ser, uint8 profile, int pflags )
{
	pe_type type = PE_NONE;
	switch (profile)
	{
	case ePT_Rigid:
		type = PE_RIGID;
		break;
	case ePT_Particle:
		type = PE_PARTICLE;
		break;
	default:
		return false;
	}

	IEntityPhysicalProxy * pEPP = (IEntityPhysicalProxy *) GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS);
	if (ser.IsWriting())
	{
		if (!pEPP || !pEPP->GetPhysicalEntity() || pEPP->GetPhysicalEntity()->GetType() != type)
		{
			gEnv->pPhysicalWorld->SerializeGarbageTypedSnapshot( ser, type, 0 );
			return true;
		}
	}
	else if (!pEPP)
	{
		return false;
	}

	pEPP->SerializeTyped( ser, type, pflags );
	return true;
}

//------------------------------------------------------------------------
bool CProjectile::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);

	g_pGame->GetWeaponSystem()->AddProjectile(GetEntity(), this);

	if (!GetGameObject()->CapturePhysics(this))
		return false;

	m_pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(GetEntity()->GetClass());

	if (0 == (GetEntity()->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY)))
		if (!m_pAmmoParams->predictSpawn)
			if (!GetGameObject()->BindToNetwork())
				return false;

	GetGameObject()->EnablePhysicsEvent(true, eEPE_OnCollisionLogged);

	LoadGeometry();
	Physicalize();

	IEntityRenderProxy *pProxy = static_cast<IEntityRenderProxy *>(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	if (pProxy && pProxy->GetRenderNode())
	{
		pProxy->GetRenderNode()->SetViewDistRatio(255);
		pProxy->GetRenderNode()->SetLodRatio(255);
	}

	float lifetime = m_pAmmoParams->lifetime;
	if (lifetime > 0.0f)
		GetEntity()->SetTimer(ePTIMER_LIFETIME, (int)(lifetime*1000.0f));

	float showtime = m_pAmmoParams->showtime;
	if (showtime > 0.0f)
	{
		GetEntity()->SetSlotFlags(0, GetEntity()->GetSlotFlags(0)&(~ENTITY_SLOT_RENDER));
		GetEntity()->SetTimer(ePTIMER_SHOWTIME, (int)(showtime*1000.0f));
	}
	else
		GetEntity()->SetSlotFlags(0, GetEntity()->GetSlotFlags(0)|ENTITY_SLOT_RENDER);

	// Only for bullets
	m_hitPoints = m_pAmmoParams->hitPoints;
	m_hitListener = false;
	if(m_hitPoints>0)
	{
		//Only projectiles with hit points are hit listeners
		g_pGame->GetGameRules()->AddHitListener(this);
		m_hitListener = true;
	}

	// register with ai if needed
	//FIXME
	//make AI ignore grenades thrown by AI; needs proper/readable grenade reaction
	if (m_pAmmoParams->aiType!=AIOBJECT_NONE)
	{
		bool	isFriendlyGrenade(false);
		IEntity *pOwnerEntity = gEnv->pEntitySystem->GetEntity(m_ownerId);
		if (pOwnerEntity && pOwnerEntity->GetAI())
			isFriendlyGrenade = (pOwnerEntity->GetAI()->GetAIType()==AIOBJECT_PUPPET);

		if (!isFriendlyGrenade)
		{
			AIObjectParameters params;
			GetEntity()->RegisterInAISystem(m_pAmmoParams->aiType, params);
		}
	}

	GetGameObject()->SetAIActivation(eGOAIAM_Always);

	return true;
}

//------------------------------------------------------------------------
void CProjectile::PostInit(IGameObject *pGameObject)
{
	GetGameObject()->EnableUpdateSlot(this, 0);
}

//------------------------------------------------------------------------
void CProjectile::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CProjectile::Serialize(TSerialize ser, unsigned aspects)
{
	if(ser.GetSerializationTarget() != eST_Network)
	{
		ser.Value("Remote", m_remote);
		// m_tracerpath should be serialized but the template-template stuff doesn't work under VS2005
		ser.Value("Owner", m_ownerId, 'eid');
		ser.Value("Weapon", m_weaponId, 'eid');
		ser.Value("TrailEffect", m_trailEffectId);
		ser.Value("TrailSound", m_trailSoundId);
		ser.Value("WhizSound", m_whizSoundId);
		ser.Value("Damage", m_damage);
		ser.Value("Destroying", m_destroying);
		ser.Value("LastPos", m_last);
		ser.Value("InitialPos", m_initialPos);
		ser.Value("DamageDrop", m_damageDropPerMeter);
		ser.Value("ScaledEffectSignaled", m_scaledEffectSignaled);
		ser.Value("HitListener", m_hitListener);
		ser.Value("HitPoints", m_hitPoints);

		bool wasVisible = false;
		if(ser.IsWriting())
			wasVisible = (GetEntity()->GetSlotFlags(0)&(ENTITY_SLOT_RENDER))?true:false;
		ser.Value("Visibility", wasVisible);
		if(ser.IsReading())
		{
			if(wasVisible)
				GetEntity()->SetSlotFlags(0, GetEntity()->GetSlotFlags(0)|ENTITY_SLOT_RENDER);
			else
				GetEntity()->SetSlotFlags(0, GetEntity()->GetSlotFlags(0)&(~ENTITY_SLOT_RENDER));
		}

	}
}

//------------------------------------------------------------------------
void CProjectile::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (updateSlot!=0)
		return;

	float color[4] = {1,1,1,1};
	bool bDebug = g_pGameCVars->i_debug_projectiles > 0;

	if(bDebug)
		gEnv->pRenderer->Draw2dLabel(50,15,2.0f,color,false,"Projectile: %s",GetEntity()->GetClass()->GetName());

	Vec3 pos = GetEntity()->GetWorldPos();

	ScaledEffect(m_pAmmoParams->pScaledEffect);

	// update whiz
	if (m_whizSoundId == INVALID_SOUNDID)
	{
		IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor();
		if (pActor && (m_ownerId != pActor->GetEntityId()))
		{
			float probability = 0.85f;

			if (Random()<=probability)
			{
				Lineseg line(m_last, pos);
				Vec3 player = pActor->GetEntity()->GetWorldPos();

				float t;
				float distanceSq=Distance::Point_LinesegSq(player, line, t);

				if (distanceSq < 5*5 && (t>=0.0f && t<=1.0f))
				{
					if (distanceSq >= 0.65*0.65)
					{
						Sphere s;
						s.center = player;
						s.radius = 6.0f;

						Vec3 entry,exit;
						int intersect=Intersect::Lineseg_Sphere(line, s, entry,exit);
						if (intersect==0x1 || intersect==0x3) // one entry or one entry and one exit
							WhizSound(true, entry, (pos-m_last).GetNormalized());
					}
				}
			}
		}
	}

	if (m_trailSoundId==INVALID_SOUNDID)
		TrailSound(true);

	// hack - fixes smoke grenade sometimes not showing up.
	if (m_trailEffectId<0 /*&& m_totalLifetime > 0.5f*/)
		TrailEffect(true);

	m_totalLifetime += ctx.fFrameTime;
	m_last = pos;
}

//------------------------------------------------------------------------
void CProjectile::HandleEvent(const SGameObjectEvent &event)
{
	if (m_destroying)
		return;

	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (event.event == eGFE_OnCollision)
	{
    EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;

		IEntity *pCollidee = pCollision->iForeignData[0]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[0]:0;
		
		// DO NOT USE THIS (makes explosives explode just placing another one on top)
		// For bullets hits see CProjectile::OnHit
		/*if (pCollidee && pCollidee->GetId() != GetEntity()->GetId())
		{
			if (CProjectile* pProjectile = g_pGame->GetWeaponSystem()->GetProjectile(pCollidee->GetId()))
			{
				Explode(true);
				return;
			}
		}*/

		const SCollisionParams* pCollisionParams = m_pAmmoParams->pCollision;
    if (pCollisionParams)
    {
			if (pCollisionParams->pParticleEffect)
          pCollisionParams->pParticleEffect->Spawn(true, IParticleEffect::ParticleLoc(pCollision->pt, pCollision->n, pCollisionParams->scale));

      if (pCollisionParams->sound)
      {
        _smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(pCollisionParams->sound, FLAG_SOUND_DEFAULT_3D);
        pSound->SetPosition(pCollision->pt);
        pSound->Play();
      }

      IStatObj *statObj = 0;
      if (pCollision->iForeignData[1] == PHYS_FOREIGN_ID_STATIC)
      {
        IRenderNode * pRN = (IRenderNode*)pCollision->pForeignData[1];
        if (pRN && pRN->GetEntityStatObj(0))
          statObj = pRN->GetEntityStatObj(0);
      }
    }

    if (m_pAmmoParams->sticky && !m_stickToActor && m_stuck < MAX_STICKY_POINTS)
      Stick(pCollision);

    Ricochet(pCollision);

		//Update damage
		if(m_damageDropPerMeter>0.0001f)
		{
			Vec3 vDiff = pCollision->pt - m_initialPos;
			float dis  = vDiff.len();

					m_damage -= (int)(floor_tpl(m_damageDropPerMeter * dis));
					
					//Check m_damage is positive
					if(m_damage<MIN_DAMAGE)
						m_damage=MIN_DAMAGE;

			//Also modify initial position (the projectile could not be destroyed, cause of pirceability)
			m_initialPos = pCollision->pt;
		}
  }
}

//------------------------------------------------------------------------
void CProjectile::ProcessEvent(SEntityEvent &event)
{
	switch(event.event)
	{
	case ENTITY_EVENT_TIMER:
		{
			switch(event.nParam[0])
			{
			case ePTIMER_SHOWTIME:
				GetEntity()->SetSlotFlags(0, GetEntity()->GetSlotFlags(0)|ENTITY_SLOT_RENDER);
				break;
			case ePTIMER_LIFETIME:
				Explode(true);
				break;
			case ePTIMER_STICKY:
				{
					if (m_stuck<MIN_STICKY_POINTS)
						Unstick();
				}
				break;
			}
		}
		break;
	}
}

//------------------------------------------------------------------------
void CProjectile::SetAuthority(bool auth)
{
}

//------------------------------------------------------------------------
void CProjectile::LoadGeometry()
{
	if (m_pAmmoParams && m_pAmmoParams->fpGeometry)
	{
		GetEntity()->SetStatObj(m_pAmmoParams->fpGeometry, 0, false);
		GetEntity()->SetSlotLocalTM(0, m_pAmmoParams->fpLocalTM);
	}
}

//------------------------------------------------------------------------
void CProjectile::Physicalize()
{
	if (!m_pAmmoParams || m_pAmmoParams->physicalizationType == ePT_None)
		return;

	GetGameObject()->SetPhysicalizationProfile(m_pAmmoParams->physicalizationType);
}

//------------------------------------------------------------------------
void CProjectile::SetVelocity(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	if (!m_pPhysicalEntity)
		return;

	Vec3 totalVelocity = (dir * m_pAmmoParams->speed * speedScale) + velocity;

	if (m_pPhysicalEntity->GetType()==PE_PARTICLE)
	{
		pe_params_particle particle;
		particle.heading = totalVelocity.GetNormalized();
		particle.velocity = totalVelocity.GetLength();

		m_pPhysicalEntity->SetParams(&particle);
	}
	else if (m_pPhysicalEntity->GetType()==PE_RIGID)
	{
		pe_action_set_velocity vel;
		vel.v = totalVelocity;

		m_pPhysicalEntity->Action(&vel);
	}
}

//------------------------------------------------------------------------
void CProjectile::SetParams(EntityId ownerId, EntityId hostId, EntityId weaponId, int damage, int hitTypeId, float damageDrop /*= 0.0f*/)
{
	m_ownerId = ownerId;
	m_weaponId = weaponId;
	m_hostId = hostId;
	m_damage = damage;
  m_hitTypeId = hitTypeId;
	m_damageDropPerMeter = damageDrop;

	if (m_hostId || m_ownerId)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_hostId?m_hostId:m_ownerId);
		if (pEntity)
		{
			if (GetEntity())
			{
				//need to set AI species to the shooter - not to be scared of it's own rockets 
				IAIActor* pAIActor = CastToIAIActorSafe(GetEntity()->GetAI());
				IAIActor* pShooterAIActor = CastToIAIActorSafe(pEntity->GetAI());
				if (pAIActor && pShooterAIActor)
				{
					AgentParameters ap = pAIActor->GetParameters();
					ap.m_nSpecies = pShooterAIActor->GetParameters().m_nSpecies;
					pAIActor->SetParameters(ap);
				}
			}
			if (m_pPhysicalEntity && m_pPhysicalEntity->GetType()==PE_PARTICLE)
			{
				pe_params_particle pparams;
				pparams.pColliderToIgnore = pEntity->GetPhysics();

				m_pPhysicalEntity->SetParams(&pparams);
			}
		}
	}
}

//------------------------------------------------------------------------
void CProjectile::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	Matrix34 worldTM=Matrix34(Matrix33::CreateRotationVDir(dir.GetNormalizedSafe()));
	worldTM.SetTranslation(pos);
	GetEntity()->SetWorldTM(worldTM);

	//Must set velocity after position, if not velocity could be reseted for PE_RIGID
	SetVelocity(pos, dir, velocity, speedScale);

	m_initialPos = pos;
	m_initialDir = dir;
	m_last = pos;

	IAIObject* pAI = 0;
	if ((pAI = GetEntity()->GetAI()) != NULL && pAI->GetAIType() == AIOBJECT_GRENADE)
	{
		IEntity *pOwnerEntity = gEnv->pEntitySystem->GetEntity(m_ownerId);
		if (pOwnerEntity && pOwnerEntity->GetAI())
		{
			if (pOwnerEntity->GetAI()->GetProxy() && pOwnerEntity->GetPhysics())
			{
				pe_status_dynamics dyn;
				pOwnerEntity->GetPhysics()->GetStatus(&dyn);
				Vec3 ownerVel = dyn.v;
				GetEntity()->GetPhysics()->GetStatus(&dyn);
				Vec3 grenadeDir = dyn.v.GetNormalizedSafe();

				// Trigger the signal at the predicted landing position.
				Vec3 predictedPos = pos;
				float dummySpeed;
				if (GetWeapon())
					GetWeapon()->PredictProjectileHit(pOwnerEntity->GetPhysics(), pos, dir, velocity, speedScale * m_pAmmoParams->speed, predictedPos, dummySpeed);
/*				bool res = pOwnerEntity->GetAI()->GetProxy()->GetSecWeapon()->PredictProjectileHit(
					pOwnerEntity->GetPhysics(), GetEntity()->GetPos(), grenadeDir, ownerVel, 1, predictedPos, speed);*/

				// Inform the AI that sees the throw
				IAIObject* pOwnerAI = pOwnerEntity->GetAI();
				AutoAIObjectIter it(gEnv->pAISystem->GetFirstAIObjectInRange(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET, predictedPos, 20.0f, false));
				for(; it->GetObject(); it->Next())
				{
					IAIObject* pAI = it->GetObject();
					if (!pAI->IsEnabled()) continue;
					if (pOwnerAI && !pOwnerAI->IsHostile(pAI,false))
						continue;

					// Only sense grenades that are on front of the AI and visible when thrown.
					// Another signal is sent when the grenade hits the ground.
					Vec3 delta = GetEntity()->GetPos() - pAI->GetPos();	// grenade to AI
					float dist = delta.NormalizeSafe();
					const float thr = cosf(DEG2RAD(160.0f));
					if (delta.Dot(pAI->GetViewDir()) > thr)
					{
						ray_hit hit;
						static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
						static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
						int res = gEnv->pPhysicalWorld->RayWorldIntersection(pAI->GetPos(), delta*dist, objTypes, flags, &hit, 1);
						if (!res || hit.dist > dist*0.9f)
						{
							IAISignalExtraData* pEData = gEnv->pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SendAnonymousSignal
							pEData->point = predictedPos;
							pEData->nID = pOwnerEntity->GetId();
							pEData->iValue = 1;
							gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER, 1, "OnGrenadeDanger", pAI, pEData);
						}
					}
				}
			}
		}
	}

}

//------------------------------------------------------------------------
void CProjectile::Destroy()
{
	if (GetEntityId() == CProjectile::m_currentlyTracked)
	{
		g_pGame->GetHUD()->SetGrenade(0);
	}
	m_destroying=true;
	if ((GetEntity()->GetFlags()&ENTITY_FLAG_CLIENT_ONLY) || gEnv->bServer)
		gEnv->pEntitySystem->RemoveEntity(GetEntity()->GetId());
}

//------------------------------------------------------------------------
bool CProjectile::IsRemote() const
{
	return m_remote;
}

//------------------------------------------------------------------------
void CProjectile::SetRemote(bool remote)
{
	m_remote = remote;
}

//------------------------------------------------------------------------
void CProjectile::Explode(bool destroy, bool impact, const Vec3 &pos, const Vec3 &normal, const Vec3 &vel, EntityId targetId)
{
	const SExplosionParams* pExplosionParams = m_pAmmoParams->pExplosion;
	if (pExplosionParams)
	{
		Vec3 dir(0,0,1);
		if (impact && vel.len2()>0)
			dir = vel.normalized();
		else if (normal.len2()>0)
			dir = -normal;

		m_hitPoints = 0;

		Vec3 epos = pos.len2()>0 ? pos : GetEntity()->GetWorldPos();

		CGameRules *pGameRules = g_pGame->GetGameRules();

		ExplosionInfo explosionInfo(m_ownerId, m_weaponId, m_damage, epos, dir, pExplosionParams->maxRadius, 0.0f, pExplosionParams->pressure, pExplosionParams->holeSize, pGameRules->GetHitTypeId(pExplosionParams->type.c_str()));
		explosionInfo.SetEffect(pExplosionParams->pParticleEffect, pExplosionParams->effectScale, pExplosionParams->maxblurdist);
		explosionInfo.SetEffectClass(m_pAmmoParams->pEntityClass->GetName());

		if (impact)
			explosionInfo.SetImpact(normal, vel, targetId);

		if (gEnv->bServer)
		{
			pGameRules->ServerExplosion(explosionInfo);
		}
	}

	if(!gEnv->bMultiplayer)
	{
		//Single player (AI related code)is processed here, CGameRules::ClientExplosion process the effect
		if (m_pAmmoParams->pFlashbang)
			FlashbangEffect(m_pAmmoParams->pFlashbang);
	}

	EndScaledEffect(m_pAmmoParams->pScaledEffect);

	if (destroy)
		Destroy();
}

//------------------------------------------------------------------------
void CProjectile::TrailSound(bool enable, const Vec3 &dir)
{
	if (enable)
	{
		if (!m_pAmmoParams->pTrail || !m_pAmmoParams->pTrail->sound)
			return;

		m_trailSoundId = GetSoundProxy()->PlaySound(m_pAmmoParams->pTrail->sound, Vec3(0,0,0), FORWARD_DIRECTION, FLAG_SOUND_DEFAULT_3D, 0, 0);
		if (m_trailSoundId != INVALID_SOUNDID)
		{
			ISound *pSound=GetSoundProxy()->GetSound(m_trailSoundId);
			if (pSound)
				pSound->SetLoopMode(true);
		}
	}
	else if (m_trailSoundId!=INVALID_SOUNDID)
	{
		GetSoundProxy()->StopSound(m_trailSoundId);
		m_trailSoundId=INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
void CProjectile::WhizSound(bool enable, const Vec3 &pos, const Vec3 &dir)
{
	if (enable)
	{
		if (!m_pAmmoParams->pWhiz || !gEnv->pSoundSystem)
			return;

		ISound *pSound=gEnv->pSoundSystem->CreateSound(m_pAmmoParams->pWhiz->sound, FLAG_SOUND_DEFAULT_3D|FLAG_SOUND_SELFMOVING);
		if (pSound)
		{
			m_whizSoundId = pSound->GetId();

			pSound->SetPosition(pos);
			pSound->SetDirection(dir*m_pAmmoParams->pWhiz->speed);
			pSound->Play();
		}
	}
	else if (m_whizSoundId!=INVALID_SOUNDID)
	{
		ISound *pSound=gEnv->pSoundSystem->GetSound(m_whizSoundId);
		if (pSound)
			pSound->Stop();
		m_whizSoundId=INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
void CProjectile::RicochetSound(const Vec3 &pos, const Vec3 &dir)
{
	if (!m_pAmmoParams->pRicochet || !gEnv->pSoundSystem)
		return;

	ISound *pSound=gEnv->pSoundSystem->CreateSound(m_pAmmoParams->pRicochet->sound, FLAG_SOUND_DEFAULT_3D|FLAG_SOUND_SELFMOVING);
	if (pSound)
	{
		pSound->GetId();

		pSound->SetPosition(pos);
		pSound->SetDirection(dir*m_pAmmoParams->pRicochet->speed);
		pSound->Play();
	}
}

//------------------------------------------------------------------------
void CProjectile::TrailEffect(bool enable, bool underWater /*=false*/)
{
	if (enable)
	{
		const STrailParams* pTrail = NULL;
		if(!underWater)
			pTrail = m_pAmmoParams->pTrail;
		else
			pTrail = m_pAmmoParams->pTrailUnderWater;

		if (!pTrail)
			return;

		if (pTrail->effect)
		{
			if(!underWater)
				m_trailEffectId = AttachEffect(true, 0, pTrail->effect, Vec3(0,0,0), Vec3(0,1,0), pTrail->scale, pTrail->prime);
			else
				m_trailUnderWaterId = AttachEffect(true, 0, pTrail->effect, Vec3(0,0,0), Vec3(0,1,0), pTrail->scale, pTrail->prime);
		}
	}
	else if (m_trailEffectId>=0 && !underWater)
	{
		AttachEffect(false, m_trailEffectId);
		m_trailEffectId=-1;
	}
	else if(m_trailUnderWaterId && underWater)
	{
		AttachEffect(false, m_trailUnderWaterId);
		m_trailUnderWaterId=-1;
	}
}

//------------------------------------------------------------------------
int CProjectile::AttachEffect(bool attach, int id, const char *name, const Vec3 &offset, const Vec3 &dir, float scale, bool bParticlePrime)
{
	// m_trailEffectId is -1 for invalid, otherwise it's the slot number where the particle effect was loaded
	if (!attach)
	{
		if (id>=0)
			GetEntity()->FreeSlot(id);
	}
	else
	{
		IParticleEffect *pParticleEffect = gEnv->p3DEngine->FindParticleEffect(name);
		if (!pParticleEffect)
			return -1;

		// find a free slot
		SEntitySlotInfo dummy;
		int i=0;
		while (GetEntity()->GetSlotInfo(i, dummy))
			i++;

		GetEntity()->LoadParticleEmitter(i, pParticleEffect, 0, bParticlePrime);
		Matrix34 tm = IParticleEffect::ParticleLoc(offset, dir, scale);
		GetEntity()->SetSlotLocalTM(i, tm);

		return i;
	}

	return -1;
}

//------------------------------------------------------------------------
void CProjectile::Stick(EventPhysCollision *pCollision)
{
	assert(pCollision);
	int trgId = 1;
	int srcId = 0;
	IPhysicalEntity *pTarget = pCollision->pEntity[trgId];
	
	if (pTarget == GetEntity()->GetPhysics())
	{
		trgId = 0;
		srcId = 1;
		pTarget = pCollision->pEntity[trgId];
	}

	IEntity *pTargetEntity = pTarget ? gEnv->pEntitySystem->GetEntityFromPhysics(pTarget) : 0;

	if (pTarget && (!pTargetEntity || (pTargetEntity->GetId() != m_ownerId)))
	{

		//Special cases
		if(pTargetEntity)
		{
			CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId()));
			if(pActor && pActor->GetHealth()>0 && pActor->GetActorSpecies()!=eGCT_UNKNOWN)
			{
				  StickToCharacter(true,pTargetEntity);
					return;
			}
			
			//Do not stick to small objects...
			if(!pActor)
			{
				pe_params_part pPart;
				pPart.ipart = 0;
				if(pTarget->GetParams(&pPart) && pPart.pPhysGeom && pPart.pPhysGeom->V<0.15f)
					return;
			}

		}
		else if(pTarget->GetType()==PE_LIVING)
		{
			//If for some reason we get here (which should not happen), return
			//Never try to stick to living entities using constraints
			return;
		}

		Vec3 an(GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION));
		an.Normalize();
		float dot = an.Dot(pCollision->n);
		dot = fabs_tpl(dot);

		bool stick=true;
		if (m_pStickyBuddy)
		{
			for (int i=0;i<m_stuck;i++)
			{
				if (m_constraintspt[i].GetDistance(pCollision->pt)<0.175f)
				{
					stick=false;
					break;
				}
			}
		}

		if (stick && (!m_pStickyBuddy || m_pStickyBuddy==pTarget) && pCollision->penetration<0.05f && (!m_pStickyBuddy || dot>=0.3f))
		{
			if (!m_pStickyBuddy)
			{
				GetEntity()->SetTimer(ePTIMER_STICKY, STICKY_TIMER);

				pe_action_impulse ai;
				ai.iApplyTime=0;
				ai.impulse=-pCollision->n* m_pAmmoParams->mass;
				m_pPhysicalEntity->Action(&ai);
			}

			pe_action_add_constraint ac;
			ac.pBuddy = pTarget;
			ac.pt[0] = pCollision->pt;
			ac.pt[1] = pCollision->pt;
			ac.partid[0] = pCollision->partid[srcId];
			ac.partid[1] = pCollision->partid[trgId];

			m_pStickyBuddy = pTarget;
			m_constraints[m_stuck]=m_pPhysicalEntity->Action(&ac);
			m_constraintspt[m_stuck++]=pCollision->pt;
		}
		else
		{
			// if we reached maximum number of collisions without a stuck 
			if (++m_stickyCollisions>=MAX_STICKY_BADCOLLISIONS)
				Unstick();
		}
	}
}

//------------------------------------------------------------------------
void CProjectile::Unstick()
{
	for (int i=0;i<m_stuck;i++)
	{
		pe_action_update_constraint uc;
		uc.bRemove = 1;
		uc.idConstraint = m_constraints[i];
		m_pPhysicalEntity->Action(&uc);
	}

	m_stuck=0;
	m_stickyCollisions=0;
	m_pStickyBuddy=0;

	GetEntity()->KillTimer(ePTIMER_STICKY);
};
//------------------------------------------------------------------------
bool CProjectile::StickToCharacter(bool stick,IEntity* pActor)
{
	if(!pActor)
		return false;

	ICharacterInstance* pCharacter = pActor->GetCharacter(0);
	if(!pCharacter)
		return false;

	//Actors doesn't support constraints, try to stick as character attachment
	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = NULL;
	
	//Select one of the attachment points
	Vec3 charOrientation = pActor->GetRotation().GetColumn1();
	Vec3 c4ToChar = pActor->GetWorldPos() - GetEntity()->GetWorldPos();
	c4ToChar.Normalize();

	if(c4ToChar.Dot(charOrientation)>0.0f)
		pAttachment = pAttachmentManager->GetInterfaceByName("c4_back");
	else
		pAttachment = pAttachmentManager->GetInterfaceByName("c4_front");

	if (!pAttachment)
	{
			GameWarning("No c4 face attachment found in actor");
			if(!pAttachment)
				return false;
	}

	if(stick)
	{
		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());

		pAttachment->AddBinding(pEntityAttachment);
		pAttachment->HideAttachment(0);
		IgnoreCollisions(true,pActor);
		//Dephysicalize object, instead of ignore collisions
		//GetGameObject()->SetPhysicalizationProfile(eAP_NotPhysicalized);

		m_pStickyBuddy = pActor->GetPhysics();
		m_stickToActor = true;
	}
	else
	{
		pAttachment->ClearBinding();
		IgnoreCollisions(false, pActor);
		m_pStickyBuddy = NULL;
		m_stickToActor = false;
	}
	return true;

}
//------------------------------------------------------------------------
void CProjectile::IgnoreCollisions(bool ignore, IEntity *pEntity)
{
	if (!pEntity)
		return;

	if (!m_pPhysicalEntity)
		return;

	if (ignore)
	{
		pe_action_add_constraint ic;
		ic.flags=constraint_inactive|constraint_ignore_buddy;
		ic.pBuddy=pEntity->GetPhysics();
		ic.pt[0].Set(0,0,0);
		m_constraints[0]=m_pPhysicalEntity->Action(&ic);
	}
	else
	{
		pe_action_update_constraint up;
		up.bRemove=true;
		up.idConstraint = m_constraints[0];
		m_constraints[0]=0;
		m_pPhysicalEntity->Action(&up);
	}
}

//------------------------------------------------------------------------
IEntitySoundProxy *CProjectile::GetSoundProxy()
{
	IEntitySoundProxy *pSoundProxy=static_cast<IEntitySoundProxy *>(GetEntity()->GetProxy(ENTITY_PROXY_SOUND));
	if (!pSoundProxy)
		pSoundProxy=static_cast<IEntitySoundProxy *>(GetEntity()->CreateProxy(ENTITY_PROXY_SOUND));

	assert(pSoundProxy);

	return pSoundProxy;
}

void CProjectile::FlashbangEffect(const SFlashbangParams* flashbang)
{
	if (!flashbang)
		return;

	float		radius	= flashbang->maxRadius;

	// collect nearby players and enemies
	SEntityProximityQuery query;
	Vec3 center = GetEntity()->GetWorldPos();
	AABB queryBox(Vec3(center.x - radius, center.y - radius, center.z - radius), Vec3(center.x + radius, center.y + radius, center.z + radius));
	query.box = queryBox;
	gEnv->pEntitySystem->QueryProximity(query);
	
	for (int i = 0; i < query.nCount; i++)
	{
		IEntity *ent = query.pEntities[i];
		if (ent)
		{
			//CryLogAlways("found: %s in box", ent->GetName());
			// cull based on geom
			IActor *trgActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(ent->GetId());
			if (trgActor)
			{
				IActor *clientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();

				//We don't need it for clientActor
				if(clientActor && clientActor->GetEntityId() == trgActor->GetEntityId())
					continue;

				AABB bbox;
				ent->GetWorldBounds(bbox);
				Vec3 eyePos = bbox.GetCenter();
				Vec3 dir = (Vec3)ent->GetWorldAngles();

				SMovementState state;
				if (IMovementController *pMV = trgActor->GetMovementController())
				{
					pMV->GetMovementState(state);
					eyePos = state.eyePosition;
					dir = state.aimDirection;
				}

				Vec3 dirToTrg = eyePos - GetEntity()->GetWorldPos();
				ray_hit hit;
				static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
				static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
				int col = gEnv->pPhysicalWorld->RayWorldIntersection(GetEntity()->GetWorldPos(), dirToTrg, objTypes, flags, &hit, 1, GetEntity()->GetPhysics());

				if (col)
				{
					continue; // hit geom between ent and flashbang
				}

				// Signal the AI to react to the flash bang.
				// The view angle is not being checked as it feels like a bug if
				// the AI does not react to the flash.
				if (ent->GetAI())
				{
					IAISignalExtraData* pExtraData = gEnv->pAISystem->CreateSignalExtraData();
					pExtraData->iValue = 0;
					gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER, 1, "OnExposedToFlashBang", ent->GetAI(), pExtraData);
				}
			}

		}
	}
}
//------------------------------------------------------------------------
void CProjectile::ScaledEffect(const SScaledEffectParams* pScaledEffect)
{
	if (!pScaledEffect)
		return;

	float lifetime = m_pAmmoParams->lifetime;
	
	IActor *local = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (local)
	{
		float dist = (GetEntity()->GetWorldPos() - local->GetEntity()->GetWorldPos()).len();
		if (m_totalLifetime < pScaledEffect->delay || pScaledEffect->radius == 0.0f)
			return;

		float fadeInAmt = 1.0f;
		float fadeOutAmt = 1.0f;
		if (pScaledEffect->fadeInTime > 0.0f)
		{
			fadeInAmt = (m_totalLifetime - pScaledEffect->delay) / pScaledEffect->fadeInTime;
			fadeInAmt = min(fadeInAmt, 1.0f);
			fadeOutAmt = 1.0f - (m_totalLifetime - (lifetime - pScaledEffect->fadeOutTime)) / pScaledEffect->fadeOutTime;
			fadeOutAmt = max(fadeOutAmt, 0.0f);
		}

		if (!m_obstructObject && pScaledEffect->aiObstructionRadius != 0.0f)
		{
			pe_params_pos pos;
			pos.scale = 0.1f;
			pos.pos = GetEntity()->GetWorldPos() + Vec3(0,0,pScaledEffect->aiObstructionRadius/4 * pos.scale);
			m_obstructObject = gEnv->pPhysicalWorld->CreatePhysicalEntity(PE_STATIC, &pos);
			if (m_obstructObject)
			{
				primitives::sphere sphere;
				sphere.center = Vec3(0,0,0);
				sphere.r = pScaledEffect->aiObstructionRadius;
				int obstructID = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeIdByName("mat_obstruct");
				IGeometry *pGeom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::sphere::type, &sphere);
				phys_geometry *geometry = gEnv->pPhysicalWorld->GetGeomManager()->RegisterGeometry(pGeom, obstructID);
				pe_geomparams params;
				params.flags = geom_colltype14;
				geometry->nRefCount = 0; // automatically delete geometry
				m_obstructObject->AddGeometry(geometry, &params);
			}
		}
		else
		{
			pe_params_pos pos;
			pos.scale = 0.1f + min(fadeInAmt, fadeOutAmt) * 0.9f;
			pos.pos = GetEntity()->GetWorldPos() + Vec3(0,0, pScaledEffect->aiObstructionRadius/4.0f * pos.scale);
			m_obstructObject->SetParams(&pos);

			// Signal the AI
			if (!m_scaledEffectSignaled &&  m_totalLifetime > (pScaledEffect->delay + pScaledEffect->fadeInTime))
			{
				m_scaledEffectSignaled = true;
				AutoAIObjectIter it(gEnv->pAISystem->GetFirstAIObjectInRange(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET, pos.pos, pScaledEffect->aiObstructionRadius*1.5f, false));
				for(; it->GetObject(); it->Next())
				{
					IAIObject* pAI = it->GetObject();
					if (!pAI->IsEnabled()) continue;
					gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER, 1, "OnExposedToSmoke", pAI);
				}
			}
		}

		if (dist > pScaledEffect->radius)
		{
			gEnv->p3DEngine->SetPostEffectParam(pScaledEffect->ppname, 0.0f);
			return;
		}

		float effectAmt = 1.0f - (dist / pScaledEffect->radius);
		effectAmt = max(effectAmt, 0.0f);
		float effectVal = effectAmt * pScaledEffect->maxValue;
		effectVal *= fadeInAmt;
		m_scaledEffectval = effectVal;

		gEnv->p3DEngine->SetPostEffectParam(pScaledEffect->ppname, effectVal);

		
	}
}
//------------------------------------------------------------------------
void CProjectile::EndScaledEffect(const SScaledEffectParams* pScaledEffect)
{
	if (!pScaledEffect || m_scaledEffectval == 0.0f)
		return;

	IActor *local = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (local)
	{
		if (pScaledEffect->fadeOutTime > 0.0f)
		{
			CActor *act = (CActor *)local;
			if (act->GetScreenEffects() != 0)
			{
				CPostProcessEffect *blur = new CPostProcessEffect(local->GetEntityId(), pScaledEffect->ppname, 0.0f);
				CLinearBlend *linear = new CLinearBlend(1.0f);
				act->GetScreenEffects()->StartBlend(blur, linear, 1.0f/pScaledEffect->fadeOutTime, 70);
			}
		}
		else
		{
			gEnv->p3DEngine->SetPostEffectParam(pScaledEffect->ppname, 0.0f);
		}
	}
}

//------------------------------------------------------------------------
void CProjectile::SetTrackedByHUD()
{
	CProjectile::m_currentlyTracked = GetEntityId();
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->SetGrenade(m_currentlyTracked);
}

//------------------------------------------------------------------------
void CProjectile::Ricochet(EventPhysCollision *pCollision)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (!pActor)
		return;

	Vec3 dir=pCollision->vloc[0];
	dir.NormalizeSafe();

	float dot=pCollision->n.Dot(dir);

	if (dot>=0.0f) // backface
		return;

	float b=0,f=0;
	uint matPierceability=0;
	if (!gEnv->pPhysicalWorld->GetSurfaceParameters(pCollision->idmat[1], b, f, matPierceability))
		return;

	matPierceability&=sf_pierceable_mask;
	float probability=0.25+0.25*(MAX(0,7-matPierceability)/7.0f);
	if ((matPierceability && matPierceability>=8) || Random()>probability)
		return;

	float angle=RAD2DEG(cry_fabsf(cry_acosf(dir.Dot(-pCollision->n))));
	if (angle<10.0f)
		return;

	Vec3 ricochetDir = -2.0f*dot*pCollision->n+dir;
	ricochetDir.NormalizeSafe();

	Ang3 angles=Ang3::GetAnglesXYZ(Matrix33::CreateRotationVDir(ricochetDir));

	float rx=Random()-0.5f;
	float rz=Random()-0.5f;

	angles.x+=rx*DEG2RAD(10.0f);
	angles.z+=rz*DEG2RAD(10.0f);

	ricochetDir=Matrix33::CreateRotationXYZ(angles).GetColumn(1).normalized();

	Lineseg line(pCollision->pt, pCollision->pt+ricochetDir*20.0f);
	Vec3 player = pActor->GetEntity()->GetWorldPos();

	float t;
	float distanceSq=Distance::Point_LinesegSq(player, line, t);

	if (distanceSq < 7.5*7.5 && (t>=0.0f && t<=1.0f))
	{
		if (distanceSq >= 0.25*0.25)
		{
			Sphere s;
			s.center = player;
			s.radius = 6.0f;

			Vec3 entry,exit;
			int intersect=Intersect::Lineseg_Sphere(line, s, entry,exit);
			if (intersect) // one entry or one entry and one exit
			{
				if (intersect==0x2)
					entry=pCollision->pt;
				RicochetSound(entry, ricochetDir);

				//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entry, ColorB(255, 255, 255, 255), entry+ricochetDir, ColorB(255, 255, 255, 255), 2);
			}
		}
	}
}


CWeapon *CProjectile::GetWeapon()
{
	if (m_weaponId)
	{
		IItem *pItem=g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_weaponId);
		if (pItem)
			return static_cast<CWeapon *>(pItem->GetIWeapon());
	}
	return 0;
}

EntityId CProjectile::GetOwnerId()const
{
    return m_ownerId;
}

float CProjectile::GetSpeed() const
{ 
	return m_pAmmoParams->speed;
}

//==================================================================
void CProjectile::OnHit(const HitInfo& hit)
{
	//Not explode if frozen
	if (CGameRules * pGameRules = g_pGame->GetGameRules())
		if(pGameRules->IsFrozen(GetEntityId()))
			return;

	//Reduce hit points if hit, and explode (only for C4, AVMine and ClayMore)
	if(hit.targetId==GetEntityId() && m_hitPoints>0 && !m_destroying)
	{
		m_hitPoints -= (int)hit.damage;

		if(m_hitPoints<=0)
			Explode(true);
	}
}
//==================================================================
void CProjectile::OnExplosion(const ExplosionInfo& explosion)
{	

}
//==================================================================
void CProjectile::OnServerExplosion(const ExplosionInfo& explosion)
{
	//In case this was the same projectile that created the explosion, hitPoints should be already 0
	if(m_hitPoints<=0 || m_destroying)
		return;

	//Not explode if frozen
	if (CGameRules * pGameRules = g_pGame->GetGameRules())
		if(pGameRules->IsFrozen(GetEntityId()))
			return;

	//One check more, just in case...
	//if(CWeapon* pWep = GetWeapon())
		//if(pWep->GetEntityId()==explosion.weaponId)
			//return;

	//Stolen from SinglePlayer.lua ;p
	IPhysicalEntity *pPE = GetEntity()->GetPhysics();
	if(pPE)
	{
		float obstruction = 1.0f-gEnv->pSystem->GetIPhysicalWorld()->IsAffectedByExplosion(pPE);

	  float distance	= (GetEntity()->GetWorldPos()-explosion.pos).len();
    distance = max(0.0f, min(distance,explosion.radius));
		
		float		 effect = (explosion.radius-distance)/explosion.radius;
		effect =  max(min(1.0f, effect*effect), 0.0f);
		effect =  effect*(1.0f-obstruction*0.7f); 
		
		m_hitPoints -= (int)(effect*explosion.damage);

		if(m_hitPoints<=0)
			Explode(true);
	}

}

void CProjectile::GetMemoryStatistics(ICrySizer *s)
{
	s->Add(*this);
}

//------------------------------------------------------------------------
void CProjectile::SerializeSpawnInfo( TSerialize ser )
{
	ser.Value("hostId", m_hostId, 'eid');
	ser.Value("ownerId", m_ownerId, 'eid');
	ser.Value("weaponId", m_weaponId, 'eid');
	ser.Value("dir", m_initialDir, 'dir0');

	if (ser.IsReading())
		SetParams(m_ownerId, m_hostId, m_weaponId, m_damage, m_hitTypeId, m_damageDropPerMeter);
}

//------------------------------------------------------------------------
ISerializableInfoPtr CProjectile::GetSpawnInfo()
{
	struct SInfo : public ISerializableInfo
	{
		EntityId hostId;
		EntityId ownerId;
		EntityId weaponId;
		Vec3 dir;
		void SerializeWith( TSerialize ser )
		{
			ser.Value("hostId", hostId, 'eid');
			ser.Value("ownerId", ownerId, 'eid');
			ser.Value("weaponId", weaponId, 'eid');
			ser.Value("dir", dir, 'dir0');
		}
	};

	SInfo *p = new SInfo();
	p->hostId=m_hostId;
	p->ownerId=m_ownerId;
	p->weaponId=m_weaponId;
	p->dir=m_initialDir;
	
	return p;
}
