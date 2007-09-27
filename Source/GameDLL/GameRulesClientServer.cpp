/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 23:5:2006   9:27 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_GameRules.h"
#include "GameRules.h"
#include "Game.h"
#include "Actor.h"
#include "Player.h"
#include "HUD/HUD.h"
#include "HUD/HUDRadar.h"
#include "HUD/HUDCrosshair.h"
#include "HUD/HUDTagNames.h"

#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "IMaterialEffects.h"

#include "WeaponSystem.h"
#include "TacticalAttachment.h"
#include "Radio.h"

#include <StlUtils.h>

//------------------------------------------------------------------------
void CGameRules::ClientSimpleHit(const SimpleHitInfo &simpleHitInfo)
{
	if (!simpleHitInfo.remote)
	{
		if (!gEnv->bServer)
			GetGameObject()->InvokeRMI(SvRequestSimpleHit(), simpleHitInfo, eRMI_ToServer);
		else
			ServerSimpleHit(simpleHitInfo);
	}
}

//------------------------------------------------------------------------
void CGameRules::ClientHit(const HitInfo &hitInfo)
{
	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	IEntity *pTarget = m_pEntitySystem->GetEntity(hitInfo.targetId);
	IEntity *pShooter =	m_pEntitySystem->GetEntity(hitInfo.shooterId);
	IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(hitInfo.targetId);
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);
	bool dead = pActor?(pActor->GetHealth()<=0):false;

	if((pClientActor && pClientActor->GetEntity()==pShooter) && pTarget && (pVehicle || pActor) && !dead)
	{
		SAFE_HUD_FUNC(GetCrosshair()->CrosshairHit());
		SAFE_HUD_FUNC(GetTagNames()->AddEnemyTagName(pActor?pActor->GetEntityId():pVehicle->GetEntityId()));
	}

	if(pActor == pClientActor)
		gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.5f * hitInfo.damage * 0.01f, hitInfo.damage * 0.02f, 0.0f));

	CreateScriptHitInfo(m_scriptHitInfo, hitInfo);
	CallScript(m_clientStateScript, "OnHit", m_scriptHitInfo);

	bool backface = hitInfo.dir.Dot(hitInfo.normal)>0;
	if (!hitInfo.remote && hitInfo.targetId && !backface)
	{
		if (!gEnv->bServer)
			GetGameObject()->InvokeRMI(SvRequestHit(), hitInfo, eRMI_ToServer);
		else
			ServerHit(hitInfo);
	}
}

//------------------------------------------------------------------------
void CGameRules::ServerSimpleHit(const SimpleHitInfo &simpleHitInfo)
{
	switch (simpleHitInfo.type)
	{
	case 0: // tag
		{
			if (!simpleHitInfo.targetId)
				return;

			AddTaggedEntity(simpleHitInfo.shooterId, simpleHitInfo.targetId, false);
		}
		break;
	case 1: // tac
		{
			if (!simpleHitInfo.targetId)
				return;

			if (CItem *pWeapon=static_cast<CItem *>(m_pGameFramework->GetIItemSystem()->GetItem(simpleHitInfo.weaponId)))
			{
				if (CTacticalAttachment *pTactical=static_cast<CTacticalAttachment *>(pWeapon->GetAccessory("TacticalAttachment")))
					pTactical->SleepTarget(simpleHitInfo.shooterId,simpleHitInfo.targetId);
			}
		}
		break;
	case 0xe: // freeze
		{
			if (!simpleHitInfo.targetId)
				return;
			FreezeEntity(simpleHitInfo.targetId, true, true);
		}
		break;
	default:
		assert(!"Unknown Simple Hit type!");
	}
}

//------------------------------------------------------------------------
void CGameRules::ServerHit(const HitInfo &hitInfo)
{
	bool ok=true;
	// check if shooter is alive
	CActor *pShooter=GetActorByEntityId(hitInfo.shooterId);

	if (hitInfo.shooterId)
	{
		if (pShooter && pShooter->GetHealth()<=0)
			ok=false;
	}

	if (hitInfo.targetId)
	{
		CActor *pTarget=GetActorByEntityId(hitInfo.targetId);
		if (pTarget && pTarget->GetSpectatorMode())
			ok=false;
	}

	if (ok)
	{
		CreateScriptHitInfo(m_scriptHitInfo, hitInfo);
		CallScript(m_serverStateScript, "OnHit", m_scriptHitInfo);

		// call hit listeners if any
		if (m_hitListeners.empty() == false)
		{
			THitListenerVec::iterator iter = m_hitListeners.begin();
			while (iter != m_hitListeners.end())
			{
				(*iter)->OnHit(hitInfo);
				++iter;
			}
		}

		if (pShooter && hitInfo.shooterId!=hitInfo.targetId && hitInfo.weaponId!=hitInfo.shooterId && hitInfo.weaponId!=hitInfo.targetId && hitInfo.damage>=0)
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_WeaponHit, 0, 0, (void *)hitInfo.weaponId));

		if (pShooter)
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_Hit, 0, 0, (void *)hitInfo.weaponId));

		if (pShooter)
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_Damage, 0, hitInfo.damage, (void *)hitInfo.weaponId));
	}
}

//------------------------------------------------------------------------
void CGameRules::ServerExplosion(const ExplosionInfo &explosionInfo)
{
  m_queuedExplosions.push(explosionInfo);
}

//------------------------------------------------------------------------
void CGameRules::ProcessServerExplosion(const ExplosionInfo &explosionInfo)
{  
  //CryLog("[ProcessServerExplosion] (frame %i) shooter %i, damage %.0f, radius %.1f", gEnv->pRenderer->GetFrameID(), explosionInfo.shooterId, explosionInfo.damage, explosionInfo.radius);

  GetGameObject()->InvokeRMI(ClExplosion(), explosionInfo, eRMI_ToRemoteClients);
  ClientExplosion(explosionInfo);  
}

//------------------------------------------------------------------------
void CGameRules::ProcessQueuedExplosions()
{
  const static uint8 nMaxExp = 3;
    
  for (uint8 exp=0; !m_queuedExplosions.empty() && exp<nMaxExp; ++exp)
  { 
    ExplosionInfo info(m_queuedExplosions.front());
    ProcessServerExplosion(info);	        
    m_queuedExplosions.pop();
  }
}


//------------------------------------------------------------------------
void CGameRules::ClientExplosion(const ExplosionInfo &explosionInfo)
{
	gEnv->p3DEngine->OnExplosion(explosionInfo.pos, explosionInfo.dir, explosionInfo.hole_size, 0, 0, true);

	pe_explosion explosion;
	explosion.epicenter = explosionInfo.pos;
	explosion.holeSize = explosionInfo.hole_size;
	explosion.r = 1.0f;
	explosion.rmin = 1.0f;
	explosion.rmax = explosionInfo.radius;
	explosion.impulsivePressureAtR = explosionInfo.pressure;
	explosion.epicenterImp = explosionInfo.pos;
	explosion.explDir = explosionInfo.dir;
	explosion.nGrow = 2;
	explosion.rminOcc = 0.15f;
	explosion.nOccRes = 32;

	if (gEnv->bServer)
  {
		gEnv->pPhysicalWorld->SimulateExplosion( &explosion, 0, 0, ent_rigid|ent_sleeping_rigid|ent_living|ent_independent|ent_static );

    CreateScriptExplosionInfo(m_scriptExplosionInfo, explosionInfo, &explosion);
    CallScript(m_serverStateScript, "OnExplosion", m_scriptExplosionInfo);    

		// call hit listeners if any
		if (m_hitListeners.empty() == false)
		{
			THitListenerVec::iterator iter = m_hitListeners.begin();
			while (iter != m_hitListeners.end())
			{
				(*iter)->OnServerExplosion(explosionInfo);
				++iter;
			}
		}

  }

	if (gEnv->bClient)
	{
		if (explosionInfo.pParticleEffect)
			explosionInfo.pParticleEffect->Spawn(true, IParticleEffect::ParticleLoc(explosionInfo.pos, explosionInfo.dir, explosionInfo.effect_scale));

		CreateScriptExplosionInfo(m_scriptExplosionInfo, explosionInfo, 0);
		CallScript(m_clientStateScript, "OnExplosion", m_scriptExplosionInfo);

		// call hit listeners if any
		if (m_hitListeners.empty() == false)
		{
			THitListenerVec::iterator iter = m_hitListeners.begin();
			while (iter != m_hitListeners.end())
			{
				(*iter)->OnExplosion(explosionInfo);
				++iter;
			}
		}
	}

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor)
	{
		//Distance
		float dist = (pClientActor->GetEntity()->GetWorldPos() - explosionInfo.pos).len();

		//Is the explosion in Player's FOV (let's suppose the FOV a bit higher, like 80)
		CActor *pActor = (CActor *)pClientActor;
		SMovementState state;
		if (IMovementController *pMV = pActor->GetMovementController())
		{
			pMV->GetMovementState(state);
		}

		Vec3 eyeToExplosion = explosionInfo.pos - state.eyePosition;
		eyeToExplosion.Normalize();
		bool inFOV = (state.eyeDirection.Dot(eyeToExplosion) > 0.68f);

		if (explosionInfo.maxblurdistance>0.0f && gEnv->pConsole->GetCVar("g_radialBlur")->GetFVal()>0.0f)
		{		
				if (inFOV && dist < explosionInfo.maxblurdistance)
				{
					float blurRadius = (-1.0f/explosionInfo.maxblurdistance)*dist + 1.0f;

					gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", blurRadius);
					gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 1.0f);

					//CActor *pActor = (CActor *)pClientActor;
					if (pActor->GetScreenEffects() != 0)
					{
						CPostProcessEffect *pBlur = new CPostProcessEffect(pClientActor->GetEntityId(),"FilterRadialBlurring_Amount", 0.0f);
						CLinearBlend *pLinear = new CLinearBlend(1.0f);
						pActor->GetScreenEffects()->StartBlend(pBlur, pLinear, 1.0f, 98);
						pActor->GetScreenEffects()->SetUpdateCoords("FilterRadialBlurring_ScreenPosX","FilterRadialBlurring_ScreenPosY", explosionInfo.pos);
					}
					float distAmp = 1.0f - (dist / explosionInfo.maxblurdistance);
					gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.5f, distAmp*3.0f, 0.0f));
				}
							
		}

		//Flashbang effect 
		if(dist<explosionInfo.radius &&
			(!strcmp(explosionInfo.effect_class,"flashbang") || !strcmp(explosionInfo.effect_class,"FlashbangAI")))
		{
			ray_hit hit;
			int col = gEnv->pPhysicalWorld->RayWorldIntersection(explosionInfo.pos , -eyeToExplosion*dist, ent_static | ent_terrain, rwi_stop_at_pierceable|rwi_colltype_any, &hit, 1);

			//If there was no obstacle between flashbang grenade and player
			if(!col)
			{
				float power = 3.0f;
				power *= max(0.0f, 1 - (dist/explosionInfo.radius));
				float lookingAt = (eyeToExplosion.Dot(state.eyeDirection.normalize()) + 1)*0.5f;
				power *= lookingAt;

				gEnv->p3DEngine->SetPostEffectParam("Flashbang_Time", 1.0f + (power * 4));
				gEnv->p3DEngine->SetPostEffectParam("Flashbang_DifractionAmount", (power * 2));
				gEnv->p3DEngine->SetPostEffectParam("Flashbang_Active", 1);
			}
		}

		
		float fDist2=(pClientActor->GetEntity()->GetWorldPos()-explosionInfo.pos).len2();
		if (fDist2<250.0f*250.0f)
		{		
			SAFE_HUD_FUNC(ShowSoundOnRadar(explosionInfo.pos, explosionInfo.hole_size));
			if (fDist2<sqr(SAFE_HUD_FUNC_RET(GetBattleRange())))
				SAFE_HUD_FUNC(TickBattleStatus(1.0f));
		}
	}

	IEntity *pShooter =	m_pEntitySystem->GetEntity(explosionInfo.shooterId);
	if (gEnv->pAISystem)
	{
		IAIObject	*pShooterAI(pShooter!=NULL ? pShooter->GetAI() : NULL);
		gEnv->pAISystem->ExplosionEvent(explosionInfo.pos,explosionInfo.radius, pShooterAI);
	}

	// if an effect was specified, don't use MFX
	if (explosionInfo.pParticleEffect)
		return;

	// impact stuff here
	SMFXRunTimeEffectParams params;
	params.pos = params.decalPos = explosionInfo.pos;
	params.trg = 0;
	params.trgRenderNode = 0;

	Vec3 gravity;
	pe_params_buoyancy buoyancy;
	gEnv->pPhysicalWorld->CheckAreas(params.pos, gravity, &buoyancy);
	
	// 0 for water, 1 for air
	Vec3 pos=params.pos;
	params.inWater = (buoyancy.waterPlane.origin.z > params.pos.z) && (gEnv->p3DEngine->GetWaterLevel(&pos)>=params.pos.z);
	params.inZeroG = (gravity.len2() < 0.0001f);
	params.trgSurfaceId = 0;

	static const int objTypes = ent_all;    
	static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;

	ray_hit ray;

	if (explosionInfo.impact)
	{
		params.dir[0] = explosionInfo.impact_velocity.normalized();
		params.normal = explosionInfo.impact_normal;

		if (gEnv->pPhysicalWorld->RayWorldIntersection(params.pos-params.dir[0]*0.0125f, params.dir[0]*0.25f, objTypes, flags, &ray, 1))
		{
			params.trgSurfaceId = ray.surface_idx;
			if (ray.pCollider->GetiForeignData()==PHYS_FOREIGN_ID_STATIC)
				params.trgRenderNode = (IRenderNode*)ray.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
		}
	}
	else
	{
		params.dir[0] = gravity;
		params.normal = -gravity.normalized();

		if (gEnv->pPhysicalWorld->RayWorldIntersection(params.pos, gravity, objTypes, flags, &ray, 1))
		{
			params.trgSurfaceId = ray.surface_idx;
			if (ray.pCollider->GetiForeignData()==PHYS_FOREIGN_ID_STATIC)
				params.trgRenderNode = (IRenderNode*)ray.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
		}
	}

	string effectClass = explosionInfo.effect_class;
	if (effectClass.empty())
		effectClass = "generic";

	string query = effectClass + "_explode";

	IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
	TMFXEffectId effectId = pMaterialEffects->GetEffectId(query.c_str(), params.trgSurfaceId);

	if (effectId == InvalidEffectId)
		effectId = pMaterialEffects->GetEffectId(query.c_str(), pMaterialEffects->GetDefaultSurfaceIndex());

	if (effectId != InvalidEffectId)
		pMaterialEffects->ExecuteEffect(effectId, params);
}

//------------------------------------------------------------------------
// RMI
//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestRename)
{
	CActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	RenamePlayer(pActor, params.name.c_str());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClRenameEntity)
{
	IEntity *pEntity=gEnv->pEntitySystem->GetEntity(params.entityId);
	if (pEntity)
	{
		string old=pEntity->GetName();
		pEntity->SetName(params.name.c_str());

		CryLogAlways("$8%s$o renamed to $8%s", old.c_str(), params.name.c_str());
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestChatMessage)
{
	SendChatMessage((EChatMessageType)params.type, params.sourceId, params.targetId, params.msg.c_str());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClChatMessage)
{
	OnChatMessage((EChatMessageType)params.type, params.sourceId, params.targetId, params.msg.c_str());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClForbiddenAreaWarning)
{
	SAFE_HUD_FUNC(ShowKillAreaWarning(params.active, params.timer));

	return true;
}


//------------------------------------------------------------------------

IMPLEMENT_RMI(CGameRules, SvRequestRadioMessage)
{
	SendRadioMessage(params.sourceId,params.msg);

	return true;
}

//------------------------------------------------------------------------

IMPLEMENT_RMI(CGameRules, ClRadioMessage)
{
	OnRadioMessage(params.sourceId,params.msg);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestChangeTeam)
{
	CActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	ChangeTeam(pActor, params.teamId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestSpectatorMode)
{
	CActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	ChangeSpectatorMode(pActor, params.mode);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetTeam)
{
	if (!params.entityId) // ignore these for now
		return true;

	// [D6] Code is handled in team manager now
	assert(g_D6Core->pTeamManager);
	bool bResult = g_D6Core->pTeamManager->SetTeam(params.teamId, params.entityId);
	/*int oldTeam = GetTeam(params.entityId);
	if (oldTeam==params.teamId)
		return true;

	TEntityTeamIdMap::iterator it=m_entityteams.find(params.entityId);
	if (it!=m_entityteams.end())
		m_entityteams.erase(it);

	bool isplayer=m_pActorSystem->GetActor(params.entityId)!=0;
	if (isplayer && oldTeam)
	{
		TPlayerTeamIdMap::iterator pit=m_playerteams.find(oldTeam);
		assert(pit!=m_playerteams.end());
		stl::find_and_erase(pit->second, params.entityId);
	}

	if (params.teamId)
	{
		m_entityteams.insert(TEntityTeamIdMap::value_type(params.entityId, params.teamId));
		if (isplayer)
		{
			TPlayerTeamIdMap::iterator pit=m_playerteams.find(params.teamId);
			assert(pit!=m_playerteams.end());
			pit->second.push_back(params.entityId);
		}
	}*/
	// [/D6]

	int oldTeam = GetTeam(params.entityId);
	bool isplayer=m_pActorSystem->GetActor(params.entityId)!=0;
	if(isplayer)
		ReconfigureVoiceGroups(params.entityId,oldTeam,params.teamId);

	m_pRadio->SetTeam(GetTeamName(params.teamId));

	ScriptHandle handle(params.entityId);
	CallScript(m_clientStateScript, "OnSetTeam", handle, params.teamId);

	return bResult;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClTextMessage)
{
	OnTextMessage((ETextMessageType)params.type, params.msg.c_str(), 
		params.params[0].empty()?0:params.params[0].c_str(),
		params.params[1].empty()?0:params.params[1].c_str(),
		params.params[2].empty()?0:params.params[2].c_str(),
		params.params[3].empty()?0:params.params[3].c_str()
		);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestSimpleHit)
{
	ServerSimpleHit(params);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestHit)
{
	HitInfo info(params);
	info.remote=true;

	ServerHit(info);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClExplosion)
{
	ClientExplosion(params);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClFreezeEntity)
{
	FreezeEntity(params.entityId, params.freeze, 0);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClShatterEntity)
{
	ShatterEntity(params.entityId, params.pos, params.impulse);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetGameTime)
{
	m_endTime = params.endTime;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetRoundTime)
{
	m_roundEndTime = params.endTime;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetPreRoundTime)
{
	m_preRoundEndTime = params.endTime;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClTaggedEntity)
{
	if (!params.entityId)
		return true;

	//SAFE_HUD_FUNC(GetRadar()->AddTaggedEntity(params.entityId)); //we have no tagging anymore, just temp and non-temp adding
	SAFE_HUD_FUNC(GetRadar()->AddEntityToRadar(params.entityId));

	SEntityEvent scriptEvent( ENTITY_EVENT_SCRIPT_EVENT );
	scriptEvent.nParam[0] = (INT_PTR)"OnGPSTagged";
	scriptEvent.nParam[1] = IEntityClass::EVT_BOOL;
	bool bValue = true;
	scriptEvent.nParam[2] = (INT_PTR)&bValue;

	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(params.entityId);
	if (pEntity)
		pEntity->SendEvent( scriptEvent );

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClTempRadarEntity)
{
	SAFE_HUD_FUNC(GetRadar()->AddEntityTemporarily(params.entityId));

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClAddSpawnGroup)
{
	AddSpawnGroup(params.entityId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClRemoveSpawnGroup)
{
	RemoveSpawnGroup(params.entityId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClAddMinimapEntity)
{
	AddMinimapEntity(params.entityId, params.type, params.lifetime);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClRemoveMinimapEntity)
{
	RemoveMinimapEntity(params.entityId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClResetMinimap)
{
	ResetMinimap();

	return true;
}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetObjective)
{
	CHUDMissionObjective *pObjective = SAFE_HUD_FUNC_RET(GetMissionObjectiveSystem().GetMissionObjective(params.name.c_str()));
	if(pObjective)
	{
		pObjective->SetStatus((CHUDMissionObjective::HUDMissionStatus)params.status);
		pObjective->SetTrackedEntity(params.entityId);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetObjectiveStatus)
{
	CHUDMissionObjective *pObjective = SAFE_HUD_FUNC_RET(GetMissionObjectiveSystem().GetMissionObjective(params.name.c_str()));
	if(pObjective)
		pObjective->SetStatus((CHUDMissionObjective::HUDMissionStatus)params.status);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetObjectiveEntity)
{
	CHUDMissionObjective *pObjective = SAFE_HUD_FUNC_RET(GetMissionObjectiveSystem().GetMissionObjective(params.name.c_str()));
	if(pObjective)
		pObjective->SetTrackedEntity(params.entityId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClResetObjectives)
{
	SAFE_HUD_FUNC(GetMissionObjectiveSystem().DeactivateObjectives(false));

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClRequestHitIndicator)
{
	SAFE_HUD_FUNC(IndicateHit());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClRequestDamageIndicator)
{
	SAFE_HUD_FUNC(IndicateDamage(0, Vec3(0,0,0), false));

	return true;
}

//------------------------------------------------------------------------

IMPLEMENT_RMI(CGameRules, SvVote)
{
  CActor* pActor = GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel));
  if(pActor)
    Vote(pActor);
  return true;
}

IMPLEMENT_RMI(CGameRules, SvStartVoting)
{
  CActor* pActor = GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel));
  if(pActor)
    StartVoting(pActor,params.vote_type,params.entityId,params.param);
  return true;
}

IMPLEMENT_RMI(CGameRules, ClVotingStatus)
{
	SAFE_HUD_FUNC(SetVotingState(params.state,params.timeout,params.entityId,params.description));
  return true;
}
