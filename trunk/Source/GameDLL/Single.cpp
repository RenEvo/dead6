/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Single.h"
#include "Item.h"
#include "Weapon.h"
#include "Projectile.h"
#include "Actor.h"
#include "Player.h"
#include "Game.h"
#include "GameCVars.h"
#include "HUD/HUD.h"
#include "WeaponSystem.h"
#include <IEntitySystem.h>
#include "ISound.h"
#include <IVehicleSystem.h>
#include <IMaterialEffects.h>
#include "GameRules.h"

#include "IronSight.h"

#include "IRenderer.h"
#include "IRenderAuxGeom.h"	

#define SAFE_HUD_FUNC(func)\
	if(g_pGame && g_pGame->GetHUD()) (g_pGame->GetHUD()->func)

struct DebugShoot{
	Vec3 pos;
	Vec3 hit;
	Vec3 dir;
};

//std::vector<DebugShoot> g_shoots;

//---------------------------------------------------------------------------
// TODO remove when aiming/fire direction is working
// debugging aiming dir
struct DBG_shoot{
	Vec3	src;
	Vec3	dst;
};

const	int	DGB_ShotCounter(3);
int	DGB_curIdx(-1);
int	DGB_curLimit(-1);
DBG_shoot DGB_shots[DGB_ShotCounter];
// remove over
//---------------------------------------------------------------------------



//------------------------------------------------------------------------
CSingle::CSingle()
: m_pWeapon(0),
	m_projectileId(0),
	m_enabled(true),	
	m_mflTimer(0.0f),
	m_suTimer(0.0f),
	m_speed_scale(1.0f),
	m_zoomtimeout(0.0f),
	m_bLocked(false),
	m_fStareTime(0.0f),
	m_suId(0),
	m_sulightId(0),
	m_lockedTarget(0),
	m_recoil(0.0f),
	m_recoilMultiplier(1.0f),
	m_recoil_dir_idx(0),
	m_recoil_dir(0,0),
	m_recoil_offset(0,0),
	m_spread(0),
	m_spinUpTime(0),
	m_firstShot(true),
	m_next_shot(0),
	m_next_shot_dt(0),
	m_emptyclip(false),
	m_reloading(false),
	m_firing(false),
	m_fired(false),
	m_shooterId(0),
  m_heatEffectId(0),
  m_heatSoundId(INVALID_SOUNDID),
  m_barrelId(0),
	m_autoaimTimeOut(AUTOAIM_TIME_OUT),
	m_bLocking(false),
	m_autoFireTimer(-1.0f),
	m_autoAimHelperTimer(-1.0f),
	m_reloadCancelled(false),
	m_reloadStartFrame(0)
{	
  m_mflightId[0] = m_mflightId[1] = 0;
	m_soundVariationParam = floor_tpl(Random(1.1f,3.9f));		//1.0, 2.0f or 3.0f
}

//------------------------------------------------------------------------
CSingle::~CSingle()
{
	ClearTracerCache();
}

//------------------------------------------------------------------------
void CSingle::Init(IWeapon *pWeapon, const IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);
	CacheTracer();
}

//------------------------------------------------------------------------
void CSingle::Update(float frameTime, uint frameId)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool keepUpdating=false;

	if (m_fireparams.autoaim && m_pWeapon->IsSelected() && m_pWeapon->IsServer())
	{
		//For the LAW only use "cruise-mode" while you are using the zoom... 
		if(!m_fireparams.autoaim_zoom || (m_fireparams.autoaim_zoom && m_pWeapon->IsZoomed()))
			UpdateAutoAim(frameTime);
		else if(m_fireparams.autoaim_zoom && !m_pWeapon->IsZoomed() && (m_bLocked || m_bLocking) )
			Unlock();

		keepUpdating=true;
	}

	if (m_zoomtimeout > 0.0f && m_fireparams.autozoom)
	{
		m_zoomtimeout -= frameTime;
		if (m_zoomtimeout < 0.0f)
		{
			m_zoomtimeout = 0.0f;
			CActor *pActor = m_pWeapon->GetOwnerActor();
			if (pActor && pActor->IsClient() && pActor->GetScreenEffects() != 0)
			{
				// Only start a zoom out if we're zooming in and not already zooming out
				if (pActor->GetScreenEffects()->HasJobs(pActor->m_autoZoomInID) &&
					!pActor->GetScreenEffects()->HasJobs(pActor->m_autoZoomOutID))
				{
					pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomInID, false);
					CLinearBlend *blend = new CLinearBlend(1);
					CFOVEffect *zoomOutEffect = new CFOVEffect(pActor->GetEntityId(), 1.0f);
					pActor->GetScreenEffects()->StartBlend(zoomOutEffect, blend, 1.0f/.1f, pActor->m_autoZoomOutID);
				}
			}
		}

		keepUpdating=true;
	}
	if (m_spinUpTime>0.0f)
	{
		m_spinUpTime -= frameTime;
		if (m_spinUpTime<=0.0f)
		{
			m_spinUpTime=0.0f;
			Shoot(true);
		}

		keepUpdating=true;
	}
	else
	{
		if (m_next_shot>0.0f)
		{
			m_next_shot -= frameTime;
			if (m_next_shot<=0.0f)
				m_next_shot=0.0f;

			keepUpdating=true;
		}
	}

	if (IsFiring())
	{
		if(m_fireparams.auto_fire && m_autoFireTimer>0.0f)
		{
			m_autoFireTimer -=frameTime;
			if(m_autoFireTimer<=0.0f)
			{
				SetAutoFireTimer(1.0f);
				AutoFire();
			}
			keepUpdating = true;
		}
	}

	if (IsReadyToFire())
		m_pWeapon->OnReadyToFire();

	// update muzzle flash light
	if (m_mflTimer>0.0f)
	{
		m_mflTimer -= frameTime;
		if (m_mflTimer <= 0.0f)
		{
			m_mflTimer = 0.0f;
			
      if (m_mflightId[0])
        m_pWeapon->EnableLight(false, m_mflightId[0]);
      if (m_mflightId[1])
        m_pWeapon->EnableLight(false, m_mflightId[1]);
		}

		keepUpdating=true;
	}

	// update spinup effect
	if (m_suTimer>0.0f)
	{
		m_suTimer -= frameTime;
		if (m_suTimer <= 0.0f)
		{
			m_suTimer = 0.0f;
			if (m_suId)
				SpinUpEffect(false);
		}

		keepUpdating=true;
	}


	UpdateRecoil(frameTime);
	UpdateHeat(frameTime);

	m_fired = false;

	if (keepUpdating)
		m_pWeapon->RequireUpdate(eIUS_FireMode);

	//---------------------------------------------------------------------------
	// TODO remove when aiming/fire direction is working
	// debugging aiming dir
	static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
	if(pAimDebug->GetIVal()!=0)
	{
		const ColorF	queueFireCol( .4f, 1.0f, 0.4f, 1.0f );
		for(int dbgIdx(0);dbgIdx<DGB_curLimit; ++dbgIdx)
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( DGB_shots[dbgIdx].src, queueFireCol, DGB_shots[dbgIdx].dst, queueFireCol );
	}
	
	if(g_pGameCVars->i_debug_zoom_mods!=0 && m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsPlayer())
	{
		float white[4] = {1,1,1,1};
		gEnv->pRenderer->Draw2dLabel(50.0f,50.0f,1.4f,white,false,"Recoil.angular_impulse : %f", m_recoilparams.angular_impulse);
		gEnv->pRenderer->Draw2dLabel(50.0f,60.0f,1.4f,white,false,"Recoil.attack : %f", m_recoilparams.attack);
		gEnv->pRenderer->Draw2dLabel(50.0f,70.0f,1.4f,white,false,"Recoil.back_impulse : %f", m_recoilparams.back_impulse);
		gEnv->pRenderer->Draw2dLabel(50.0f,80.0f,1.4f,white,false,"Recoil.decay : %f", m_recoilparams.decay);
		gEnv->pRenderer->Draw2dLabel(50.0f,90.0f,1.4f,white,false,"Recoil.impulse : %f", m_recoilparams.impulse);
		gEnv->pRenderer->Draw2dLabel(50.0f,100.0f,1.4f,white,false,"Recoil.max x,y : %f, %f", m_recoilparams.max.x, m_recoilparams.max.y);
		gEnv->pRenderer->Draw2dLabel(50.0f,110.0f,1.4f,white,false,"Recoil.max_recoil : %f", m_recoilparams.max_recoil);
		gEnv->pRenderer->Draw2dLabel(50.0f,120.0f,1.4f,white,false,"Recoil.recoil_crouch_m : %f", m_recoilparams.recoil_crouch_m);
		gEnv->pRenderer->Draw2dLabel(50.0f,130.0f,1.4f,white,false,"Recoil.recoil_jump_m : %f", m_recoilparams.recoil_jump_m);
		gEnv->pRenderer->Draw2dLabel(50.0f,140.0f,1.4f,white,false,"Recoil.recoil_prone_m : %f", m_recoilparams.recoil_prone_m);
		gEnv->pRenderer->Draw2dLabel(50.0f,150.0f,1.4f,white,false,"Recoil.recoil_strMode_m : %f", m_recoilparams.recoil_strMode_m);
		gEnv->pRenderer->Draw2dLabel(50.0f,160.0f,1.4f,white,false,"Recoil.recoil_zeroG_m : %f", m_recoilparams.recoil_zeroG_m);

		gEnv->pRenderer->Draw2dLabel(300.0f, 50.0f, 1.4f, white, false, "Spread.attack : %f", m_spreadparams.attack);
		gEnv->pRenderer->Draw2dLabel(300.0f, 60.0f, 1.4f, white, false, "Spread.decay : %f", m_spreadparams.decay);
		gEnv->pRenderer->Draw2dLabel(300.0f, 70.0f, 1.4f, white, false, "Spread.max : %f", m_spreadparams.max);
		gEnv->pRenderer->Draw2dLabel(300.0f, 80.0f, 1.4f, white, false, "Spread.min : %f", m_spreadparams.min);
		gEnv->pRenderer->Draw2dLabel(300.0f, 90.0f, 1.4f, white, false, "Spread.rotation_m : %f", m_spreadparams.rotation_m);
		gEnv->pRenderer->Draw2dLabel(300.0f, 100.0f, 1.4f, white, false, "Spread.speed_m : %f", m_spreadparams.speed_m);
		gEnv->pRenderer->Draw2dLabel(300.0f, 110.0f, 1.4f, white, false, "Spread.spread_crouch_m : %f", m_spreadparams.spread_crouch_m);
		gEnv->pRenderer->Draw2dLabel(300.0f, 120.0f, 1.4f, white, false, "Spread.spread_jump_m : %f", m_spreadparams.spread_jump_m);
		gEnv->pRenderer->Draw2dLabel(300.0f, 130.0f, 1.4f, white, false, "Spread.spread_prone_m : %f", m_spreadparams.spread_prone_m);
		gEnv->pRenderer->Draw2dLabel(300.0f, 130.0f, 1.4f, white, false, "Spread.spread_zeroG_m : %f", m_spreadparams.spread_zeroG_m);

	}
	
	//---------------------------------------------------------------------------
}

void CSingle::PostUpdate(float frameTime)
{
	bool ok = false;
	bool startTarget = false;

	if (m_targetSpotSelected)
	{
		
		if(m_autoAimHelperTimer>0.0f)
		{
			m_autoAimHelperTimer -= frameTime;
			
			if(m_autoAimHelperTimer<=0.0f)
			{
				Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
				Vec3 pos = GetFiringPos(hit);
				m_targetSpotSelected = true;
				m_targetSpot = hit;
				startTarget = true;
				ok = true;
			}
		}
		else
		{
			ok = true;
		}

		const SAmmoParams *pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(GetAmmoType());
		if (pAmmoParams && ok)
		{
			if (pAmmoParams->physicalizationType != ePT_None)
			{
				float speed = pAmmoParams->speed;
				if (speed == 0.0f)
					speed = 60.0f;
        Vec3 hit = m_targetSpot;
				Vec3 pos = m_lastAimSpot;

				float x, y;
				Vec3 diff = hit - pos;
				y = diff.z;
				diff.z = 0;
				x = diff.GetLength();
				float angle = GetProjectileFiringAngle(speed,9.8f,x,y);
				Matrix33 m = Matrix33::CreateRotationVDir(diff);
				m.TransformVector(pos);
				m.OrthonormalizeFast();
				Ang3 aAng = RAD2DEG(Ang3::GetAnglesXYZ(m));
				aAng.x = angle;
				Ang3 ang2 = DEG2RAD(aAng);
				Matrix33 m2 = Matrix33::CreateRotationXYZ(ang2);
				Vec3 dir3 = m2.GetColumn(1);
				dir3 = dir3.normalize() * 10.0f;
				Vec3 spot = pos + dir3;

				m_pWeapon->SetAimLocation(spot);
				m_pWeapon->SetTargetLocation(m_targetSpot);
			}
			if(startTarget)
			{
				m_pWeapon->ActivateTarget(true);		//Activate Targeting on the weapon
				m_pWeapon->OnStartTargetting(m_pWeapon);
			}

		}

	}
}

void CSingle::UpdateFPView(float frameTime)
{
	if (m_targetSpotSelected)
	{
		Vec3 hit = m_targetSpot;
		m_lastAimSpot = GetFiringPos(hit);
	}
}

//------------------------------------------------------------------------
bool CSingle::IsValidAutoAimTarget(IEntity* pEntity)
{  
  IActor *pActor = 0;				
  IVehicle* pVehicle = 0;
  
  if (pEntity->IsHidden())
    return false;

  AABB box;
  pEntity->GetLocalBounds(box);
  float vol = box.GetVolume();  
  
  if (vol < m_fireparams.autoaim_minvolume || vol > m_fireparams.autoaim_maxvolume)
  {
    //CryLogAlways("volume check failed: %f", vol);
    return false;
  }
  
  pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
  if (pActor && pActor->GetHealth() > 0.f)
    return true;
  
  pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId()); 
  if (pVehicle && pVehicle->GetStatus().health > 0.f)
    return true;
    
  return false;
}

//------------------------------------------------------------------------
bool CSingle::CheckAutoAimTolerance(const Vec3& aimPos, const Vec3& aimDir)
{
  // todo: this check is probably not sufficient
  IEntity *pLocked = gEnv->pEntitySystem->GetEntity(m_lockedTarget);
  AABB bbox;
  pLocked->GetWorldBounds(bbox);
  Vec3 targetPos = bbox.GetCenter();
  Vec3 dirToTarget = (targetPos - aimPos).normalize();
  float dot = aimDir.Dot(dirToTarget);
  Matrix33 mat = Matrix33::CreateRotationVDir(dirToTarget);
  Vec3 right = mat.GetColumn(0).normalize();
  Vec3 maxVec = (targetPos - aimPos) + (right * m_fireparams.autoaim_tolerance);
  float maxDot = dirToTarget.Dot(maxVec.normalize());
  
  return (dot >= maxDot);
}

void CSingle::Lock(EntityId targetId)
{
	m_lockedTarget = targetId;
	m_bLocking = false;
	m_bLocked = true;
	m_autoaimTimeOut = AUTOAIM_TIME_OUT;

	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if (pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimLocked(m_lockedTarget));

			_smart_ptr< ISound > pBeep = gEnv->pSoundSystem->CreateSound("Sounds/interface:hud:target_lock", FLAG_SOUND_2D);
			if (pBeep)
				pBeep->Play();
		}

		if (m_pWeapon->IsServer())
			m_pWeapon->GetGameObject()->InvokeRMI(CWeapon::ClLock(), CWeapon::LockParams(targetId), eRMI_ToClientChannel|eRMI_NoLocalCalls, pActor->GetChannelId());
	}
}

void CSingle::ResetLock()
{
	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if ((m_bLocking || m_bLocked) && pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimUnlock(m_lockedTarget));
		}
	}

	m_bLocked = false;
	m_bLocking = false;
	m_lockedTarget = 0;
	m_fStareTime = 0.0f;
	m_autoaimTimeOut = AUTOAIM_TIME_OUT;
}

void CSingle::Unlock()
{
	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if (pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimUnlock(m_lockedTarget));
		}

		if (m_pWeapon->IsServer())
			m_pWeapon->GetGameObject()->InvokeRMI(CWeapon::ClUnlock(), CWeapon::EmptyParams(), eRMI_ToClientChannel|eRMI_NoLocalCalls, pActor->GetChannelId());
	}

	m_bLocked = false;
	m_bLocking = false;
	m_lockedTarget = 0;
	m_fStareTime = 0.0f;
	m_autoaimTimeOut = AUTOAIM_TIME_OUT;
}

void CSingle::StartLocking(EntityId targetId)
{
	// start locking
	m_lockedTarget = targetId;
	m_bLocking = true;
	m_bLocked = false;
	m_fStareTime = 0.0f;

	if (CActor *pActor=m_pWeapon->GetOwnerActor())
	{
		if (pActor->IsClient())
		{
			SAFE_HUD_FUNC(AutoAimLocking(m_lockedTarget));
		}

		if (m_pWeapon->IsServer())
			m_pWeapon->GetGameObject()->InvokeRMI(CWeapon::ClStartLocking(), CWeapon::LockParams(targetId), eRMI_ToClientChannel|eRMI_NoLocalCalls, pActor->GetChannelId());
	}
}

//------------------------------------------------------------------------
void CSingle::UpdateAutoAim(float frameTime)
{
  static IGameObjectSystem* pGameObjectSystem = gEnv->pGame->GetIGameFramework()->GetIGameObjectSystem();

	CActor *pOwner = m_pWeapon->GetOwnerActor();
  if (!pOwner || !pOwner->IsPlayer())
    return;
	
  // todo: use crosshair/aiming dir
	IMovementController *pMC = pOwner->GetMovementController();
	if (!pMC)
		return;

	SMovementState state;
	pMC->GetMovementState(state);

	Vec3 aimDir = state.eyeDirection;
	Vec3 aimPos = state.eyePosition;
	
	float maxDistance = m_fireparams.autoaim_distance;
  
  ray_hit ray;
  
  IPhysicalEntity* pSkipEnts[10];
  int nSkipEnts = GetSkipEntities(m_pWeapon, pSkipEnts, 10);
  	
	int result = gEnv->pPhysicalWorld->RayWorldIntersection(aimPos, aimDir * 2.f * maxDistance, 
		ent_all, rwi_stop_at_pierceable|rwi_colltype_any, &ray, 1, pSkipEnts, nSkipEnts);		

  bool hitValidTarget = false;
  IEntity* pEntity = 0;

  if (result && ray.pCollider)
	{	
		pEntity = (IEntity *)ray.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);    	        
    if (pEntity && IsValidAutoAimTarget(pEntity))
      hitValidTarget = true;
  }

	if(m_bLocked)
		m_autoaimTimeOut -= frameTime;

  if (hitValidTarget && ray.dist <= maxDistance)
  {	
    if (m_bLocked)
		{
			if ((m_lockedTarget != pEntity->GetId()) && m_autoaimTimeOut<=0.0f)
				StartLocking(pEntity->GetId());
		}
		else
		{	
			if (!m_bLocking || m_lockedTarget!=pEntity->GetId())
				StartLocking(pEntity->GetId());
			else
				m_fStareTime += frameTime;
		}
	}
	else if(!hitValidTarget && m_bLocking)
	{
		Unlock();
	}
	else
	{
		// check if we're looking far away from our locked target
		if ((m_bLocked && !(ray.dist<=maxDistance && CheckAutoAimTolerance(aimPos, aimDir))) || (!m_bLocked && m_lockedTarget && m_fStareTime != 0.0f))
    { 
			if(!m_fireparams.autoaim_timeout)
				Unlock();
    }
	}

  if (m_bLocking && !m_bLocked && m_fStareTime >= m_fireparams.autoaim_locktime && m_lockedTarget)
		Lock(m_lockedTarget);
	else if(m_bLocked && hitValidTarget && m_lockedTarget!=pEntity->GetId())
		Lock(pEntity->GetId());
  else if (m_bLocked)
	{ 
    // check if target still valid (can e.g. be killed)
    IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_lockedTarget);	
		if ((pEntity && !IsValidAutoAimTarget(pEntity)) || (m_fireparams.autoaim_timeout && m_autoaimTimeOut<=0.0f) )
			Unlock();
	}
}
//------------------------------------------------------------------------
void CSingle::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CSingle::ResetParams(const IItemParamsNode *params)
{
	const IItemParamsNode *fire = params?params->GetChild("fire"):0;
	const IItemParamsNode *tracer = params?params->GetChild("tracer"):0;
	const IItemParamsNode *ooatracer = params?params->GetChild("outofammotracer"):0;
	const IItemParamsNode *recoil = params?params->GetChild("recoil"):0;
	const IItemParamsNode *spread = params?params->GetChild("spread"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;
	const IItemParamsNode *muzzleflash = params?params->GetChild("muzzleflash"):0;
	const IItemParamsNode *muzzlesmoke = params?params->GetChild("muzzlesmoke"):0;
	const IItemParamsNode *reject = params?params->GetChild("reject"):0;
	const IItemParamsNode *spinup = params?params->GetChild("spinup"):0;
	const IItemParamsNode *heating = params?params->GetChild("heating"):0;
  const IItemParamsNode *dust = params?params->GetChild("dust"):0;

	m_fireparams.Reset(fire);
	m_tracerparams.Reset(tracer);
	m_ooatracerparams.Reset(ooatracer);
	m_recoilparams.Reset(recoil);
	m_spreadparams.Reset(spread);
	m_actions.Reset(actions);
	m_muzzleflash.Reset(muzzleflash);
	m_muzzlesmoke.Reset(muzzlesmoke);
	m_reject.Reset(reject);
	m_spinup.Reset(spinup);
	m_heatingparams.Reset(heating);
  m_dustparams.Reset(dust);

	BackUpOriginalSpreadRecoil();
}

//------------------------------------------------------------------------
void CSingle::PatchParams(const IItemParamsNode *patch)
{
	const IItemParamsNode *fire = patch->GetChild("fire");
	const IItemParamsNode *tracer = patch->GetChild("tracer");
	const IItemParamsNode *ooatracer = patch->GetChild("outofammotracer");
	const IItemParamsNode *recoil = patch->GetChild("recoil");
	const IItemParamsNode *spread = patch->GetChild("spread");
	const IItemParamsNode *actions = patch->GetChild("actions");
	const IItemParamsNode *muzzleflash = patch->GetChild("muzzleflash");
	const IItemParamsNode *muzzlesmoke = patch->GetChild("muzzlesmoke");
	const IItemParamsNode *reject = patch->GetChild("reject");
	const IItemParamsNode *spinup = patch->GetChild("spinup");
	const IItemParamsNode *heating = patch->GetChild("heating");
  const IItemParamsNode *dust = patch->GetChild("dust");

	m_fireparams.Reset(fire, false);
	m_tracerparams.Reset(tracer, false);
	m_ooatracerparams.Reset(ooatracer, false);
	m_recoilparams.Reset(recoil, false);
	m_spreadparams.Reset(spread, false);
	m_actions.Reset(actions, false);
	m_muzzleflash.Reset(muzzleflash, false);
	m_muzzlesmoke.Reset(muzzlesmoke, false);
	m_reject.Reset(reject, false);
	m_spinup.Reset(spinup, false);
	m_heatingparams.Reset(heating, false);
  m_dustparams.Reset(dust, false);

	BackUpOriginalSpreadRecoil();

	Activate(true);
}

//------------------------------------------------------------------------
void CSingle::Activate(bool activate)
{
	m_fired = m_firing = m_reloading = m_emptyclip = false;
	m_spinUpTime = 0.0f;
	m_next_shot = 0.0f;
	m_next_shot_dt = 60.0f/m_fireparams.rate;
  m_barrelId = 0;
  m_mfIds.resize(m_fireparams.barrel_count);
  
  m_heat = 0.0f;
	m_overheat = 0.0f;
  m_pWeapon->StopSound(m_heatSoundId);  
  m_heatSoundId = INVALID_SOUNDID;  
  
  if (!activate && m_heatEffectId)
    m_heatEffectId = m_pWeapon->AttachEffect(0, m_heatEffectId, false);

	m_targetSpotSelected = false;
	m_reloadCancelled = false;

  if (!activate)
	  MuzzleFlashEffect(false);
  	
  SpinUpEffect(false);

  m_firstShot = activate;
	
	ResetLock();

  if (activate && m_fireparams.autoaim)
    m_pWeapon->RequireUpdate(eIUS_FireMode);  

  m_fStareTime = 0.f;    

  CActor *owner = m_pWeapon->GetOwnerActor();
  if (owner)
  {
	  if (!activate && owner->GetScreenEffects() != 0)
	  {
		  owner->GetScreenEffects()->ClearBlendGroup(owner->m_autoZoomInID, false);
		  owner->GetScreenEffects()->ClearBlendGroup(owner->m_autoZoomOutID, false);
	  }
  }

	ResetRecoil();  
}

//------------------------------------------------------------------------
int CSingle::GetAmmoCount() const
{
	return m_pWeapon->GetAmmoCount(m_fireparams.ammo_type_class);
}

//------------------------------------------------------------------------
int CSingle::GetClipSize() const
{
	return m_fireparams.clip_size;
}

//------------------------------------------------------------------------
bool CSingle::OutOfAmmo() const
{
	if (m_fireparams.clip_size!=0)
		return m_fireparams.ammo_type_class && m_fireparams.clip_size != -1 && m_pWeapon->GetAmmoCount(m_fireparams.ammo_type_class)<1;

	return m_fireparams.ammo_type_class && m_pWeapon->GetInventoryAmmoCount(m_fireparams.ammo_type_class)<1;
}

//------------------------------------------------------------------------
bool CSingle::CanReload() const
{
	int clipSize = GetClipSize();

	bool isAI = m_pWeapon->GetOwner()?!m_pWeapon->GetOwnerActor()->IsPlayer():false;

	if(m_fireparams.bullet_chamber)
		clipSize += 1;

	if (m_fireparams.clip_size!=0)
		return !m_reloading && (GetAmmoCount()<clipSize) && ((m_pWeapon->GetInventoryAmmoCount(m_fireparams.ammo_type_class)>0)||(isAI));
	return false;
}

bool CSingle::IsReloading()
{
	return m_reloading;
}

//------------------------------------------------------------------------
void CSingle::Reload(int zoomed)
{
	StartReload(zoomed);
}

//------------------------------------------------------------------------
bool CSingle::CanFire(bool considerAmmo) const
{
	return !m_reloading && (m_next_shot<=0.0f) && (m_spinUpTime<=0.0f) && (m_overheat<=0.0f) &&
		!m_pWeapon->IsBusy() && (!considerAmmo || !OutOfAmmo() || !m_fireparams.ammo_type_class || m_fireparams.clip_size == -1);
}

//------------------------------------------------------------------------
void CSingle::StartFire(EntityId shooterId)
{
	if (m_fireparams.aim_helper && !m_targetSpotSelected)
	{
		//Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
		//Vec3 pos = GetFiringPos(hit);
		m_targetSpotSelected = true;
		//m_pWeapon->ActivateTarget(true);		//Activate Targeting on the weapon
		//m_pWeapon->OnStartTargetting(m_pWeapon);
		//m_targetSpot = hit;
		SetAutoAimHelperTimer(m_fireparams.aim_helper_delay);
		m_pWeapon->GetGameObject()->EnablePostUpdates(m_pWeapon);
		return;
	}
	if (m_pWeapon->IsBusy())
		return;
	m_shooterId = shooterId;
	if (m_fireparams.spin_up_time>0.0f)
	{
		m_firing = true;
		m_spinUpTime = m_fireparams.spin_up_time;

		m_pWeapon->PlayAction(m_actions.spin_up);
		SpinUpEffect(true);
	}
	else
		m_firing = Shoot(true);

	if(m_firing && m_fireparams.auto_fire && m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsPlayer())
		SetAutoFireTimer(1.0f);

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CSingle::StopFire(EntityId shooterId)
{
	if (m_targetSpotSelected)
	{
		m_shooterId = shooterId;

		if (m_fireparams.spin_up_time>0.0f)
		{
			m_firing = true;
			m_spinUpTime = m_fireparams.spin_up_time;

			m_pWeapon->PlayAction(m_actions.spin_up);
			SpinUpEffect(true);
		}
		else
			m_firing = Shoot(true);

		m_pWeapon->RequireUpdate(eIUS_FireMode);
		m_targetSpotSelected = false;
		m_pWeapon->ActivateTarget(false);
		m_pWeapon->OnStopTargetting(m_pWeapon);
		m_pWeapon->GetGameObject()->DisablePostUpdates(m_pWeapon);
	}
	
	if(m_fireparams.auto_fire)
		SetAutoFireTimer(-1.0f);

	m_firing = false;
}


//------------------------------------------------------------------------
const char *CSingle::GetType() const
{
	return "Single";
}

//------------------------------------------------------------------------
IEntityClass* CSingle::GetAmmoType() const
{
	return m_fireparams.ammo_type_class;
}

//------------------------------------------------------------------------
float CSingle::GetSpinUpTime() const
{
	return m_fireparams.spin_up_time;
}

//------------------------------------------------------------------------
float CSingle::GetSpinDownTime() const
{
	return m_fireparams.spin_down_time;
}

//------------------------------------------------------------------------
float CSingle::GetNextShotTime() const
{
  return m_next_shot;
}

//------------------------------------------------------------------------
float CSingle::GetFireRate() const
{
  return m_fireparams.rate;
}

//------------------------------------------------------------------------
void CSingle::Enable(bool enable)
{
	m_enabled = enable;
}

//------------------------------------------------------------------------
bool CSingle::IsEnabled() const
{
	return m_enabled;
}

//------------------------------------------------------------------------
struct CSingle::EndReloadAction
{
	EndReloadAction(CSingle *_single, int zoomed, int reloadStartFrame):
	single(_single), _zoomed(zoomed), _reloadStartFrame(reloadStartFrame){};
	
	CSingle *single;
	int _zoomed;
	int _reloadStartFrame;

	void execute(CItem *_this)
	{
		if(single->m_reloadStartFrame == _reloadStartFrame)
			single->EndReload(_zoomed);
	}
};

struct CSingle::StartReload_SliderBack
{
	StartReload_SliderBack(CSingle *_single): single(_single) {};
	CSingle *single;

	void execute(CItem *_this)
	{
		_this->StopLayer(single->m_fireparams.slider_layer);
	}
};

void CSingle::CancelReload()
{
	m_reloadCancelled = true;
	EndReload(0);
}

void CSingle::StartReload(int zoomed)
{
	m_reloading = true;
	if (zoomed != 0)
		m_pWeapon->ExitZoom();
	m_pWeapon->SetBusy(true);
	
	const char *action = m_actions.reload.c_str();
	IEntityClass* ammo = m_fireparams.ammo_type_class;

	m_pWeapon->OnStartReload(m_pWeapon->GetOwnerId(), ammo);

	//When interrupting reload to melee, scheduled reload action can get a bit "confused"
	//This way we can verify that the scheduled EndReloadAction matches this StartReload call... 
	m_reloadStartFrame = gEnv->pRenderer->GetFrameID();

	if (m_fireparams.bullet_chamber)
	{
		int ammoCount = m_pWeapon->GetAmmoCount(ammo);
		if ((ammoCount>0) && (ammoCount < m_fireparams.clip_size))
			action = m_actions.reload_chamber_full.c_str();
		else
			action = m_actions.reload_chamber_empty.c_str();
	}
	//float speedOverride = -1.0f;
	float mult = 1.0f;
	/*CActor *owner = m_pWeapon->GetOwnerActor();
	if (owner && owner->GetActorClass() == CPlayer::GetActorClassType())
	{
		CPlayer *plr = (CPlayer *)owner;
		if(plr->GetNanoSuit())
		{
			ENanoMode curMode = plr->GetNanoSuit()->GetMode();
			if (curMode == NANOMODE_SPEED)
			{
				speedOverride = 1.75f;
				mult = 1.0f/1.75f;
			}
		}
	}*/
	//if (speedOverride > 0.0f)
	//	m_pWeapon->PlayAction(action, 0, false, CItem::eIPAF_Default, speedOverride);
	//else
		m_pWeapon->PlayAction(action);

	m_pWeapon->GetScheduler()->TimerAction((uint)(m_fireparams.reload_time*1000*mult), CSchedulerAction<EndReloadAction>::Create(EndReloadAction(this, zoomed, m_reloadStartFrame)), false);

	int time=(int)(MAX(0,((m_fireparams.reload_time*1000)-m_fireparams.slider_layer_time)*mult));
	m_pWeapon->GetScheduler()->TimerAction(time, CSchedulerAction<StartReload_SliderBack>::Create(this), false);
}

//------------------------------------------------------------------------
void CSingle::EndReload(int zoomed)
{
	m_reloading = false;
	m_emptyclip = false;
	m_spinUpTime = m_firing?m_fireparams.spin_up_time:0.0f;

	IEntityClass* ammo = m_fireparams.ammo_type_class;
	m_pWeapon->OnEndReload(m_pWeapon->GetOwnerId(), ammo);

	if (m_pWeapon->IsServer() && !m_reloadCancelled)
	{
		bool ai=m_pWeapon->GetOwnerActor()?!m_pWeapon->GetOwnerActor()->IsPlayer():false;

		int ammoCount = m_pWeapon->GetAmmoCount(ammo);
		int inventoryCount=m_pWeapon->GetInventoryAmmoCount(m_fireparams.ammo_type_class);
		int refill= MIN(inventoryCount, m_fireparams.clip_size-ammoCount);
		if (m_fireparams.bullet_chamber && (ammoCount>0) && (ammoCount<m_fireparams.clip_size+1) && ((inventoryCount-refill)>0))
			ammoCount += ++refill;
		else
			ammoCount += refill;

		if(ai)
			ammoCount = m_fireparams.clip_size;

		m_pWeapon->SetAmmoCount(ammo, ammoCount);

		if (m_pWeapon->IsServer())
		{
			
			if ((g_pGameCVars->i_unlimitedammo == 0 && m_fireparams.max_clips != -1) && !ai)
				m_pWeapon->SetInventoryAmmoCount(ammo, m_pWeapon->GetInventoryAmmoCount(ammo)-refill);
		}
	}

	m_reloadStartFrame = 0;
	m_reloadCancelled = false;
	m_pWeapon->SetBusy(false);

	//Do not zoom after reload
	//if (zoomed && m_pWeapon->IsSelected())
		//m_pWeapon->StartZoom(m_pWeapon->GetOwnerId(),zoomed);
}

//------------------------------------------------------------------------
struct CSingle::RezoomAction
{
	RezoomAction(){};
	void execute(CItem *pItem)
	{
		CWeapon *pWeapon=static_cast<CWeapon *>(pItem);
		IZoomMode *pIZoomMode = pWeapon->GetZoomMode(pWeapon->GetCurrentZoomMode());

		if (pIZoomMode)
		{
			CIronSight *pZoomMode=static_cast<CIronSight *>(pIZoomMode);
			pZoomMode->TurnOff(false);
		}
	}
};

struct CSingle::Shoot_SliderBack
{
	Shoot_SliderBack(CSingle *_single): pSingle(_single) {};
	CSingle *pSingle;

	void execute(CItem *pItem)
	{
		pItem->StopLayer(pSingle->m_fireparams.slider_layer);
	}
};

struct CSingle::CockAction
{
	CSingle *pSingle;
	CockAction(CSingle *_single): pSingle(_single) {};
	void execute(CItem *pItem)
	{
		pItem->PlayAction(pSingle->m_actions.cock);
		pItem->GetScheduler()->TimerAction(pItem->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<RezoomAction>::Create(), false);

		int time=MAX(0,pItem->GetCurrentAnimationTime(CItem::eIGS_FirstPerson)-pSingle->m_fireparams.slider_layer_time);
		pItem->GetScheduler()->TimerAction(time, CSchedulerAction<Shoot_SliderBack>::Create(pSingle), false);
	}
};

class CSingle::ScheduleReload
{
public:
	ScheduleReload(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) 
	{
		_pWeapon->SetBusy(false);
		_pWeapon->Reload();
	}
private:
	CWeapon *_pWeapon;
};

bool CSingle::Shoot(bool resetAnimation, bool autoreload, bool noSound)
{
	IEntityClass* ammo = m_fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);

	CActor *pActor = m_pWeapon->GetOwnerActor();

	if (m_fireparams.clip_size==0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	if (!CanFire(true))
	{
		if ((ammoCount <= 0) && (!m_reloading))
		{
			m_pWeapon->PlayAction(m_actions.empty_clip);
			//Auto reload
			m_pWeapon->Reload();			
		}

		return false;
	}
	else if(m_pWeapon->IsWeaponLowered())
	{
		m_pWeapon->PlayAction(m_actions.empty_clip);
		return false;
	}

	// Aim assistance
	m_pWeapon->AssistAiming();

	Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
	Vec3 pos = GetFiringPos(hit);
	Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(dir);

	// Advanced aiming (VTOL Ascension)
	if(m_fireparams.advanced_AAim)
		m_pWeapon->AdvancedAssistAiming(m_fireparams.advanced_AAim_Range,pos,dir);
	
	const char *action = m_actions.fire_cock.c_str();
	if (ammoCount == 1 || (m_fireparams.no_cock && m_pWeapon->IsZoomed()) || (m_fireparams.unzoomed_cock && m_pWeapon->IsZoomed()))
		action = m_actions.fire.c_str();

	int flags = CItem::eIPAF_Default|CItem::eIPAF_RestartAnimation|CItem::eIPAF_CleanBlending;
	if (m_firstShot)
	{
		m_firstShot = false;
		flags|=CItem::eIPAF_NoBlend;
	}
	
	flags = PlayActionSAFlags(flags);
	if(noSound)
		flags&=~CItem::eIPAF_Sound;
	m_pWeapon->PlayAction(action, 0, false, flags);

	// debug
  static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
  if (pAimDebug->GetIVal()) 
  {
    IPersistantDebug* pDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
    pDebug->Begin("CSingle::Shoot", false);
    pDebug->AddSphere(hit, 0.6f, ColorF(0,0,1,1), 10.f);
    pDebug->AddDirection(pos, 0.25f, dir, ColorF(0,0,1,1), 1.f);
  }
/*
	DebugShoot shoot;
	shoot.pos=pos;
	shoot.dir=dir;
	shoot.hit=hit;
	g_shoots.push_back(shoot);*/
	

	Vec3 tracerhit = ZERO;
	ray_hit rayhit;
  IPhysicalEntity* pSkipEnts[10];
  int nSkip = GetSkipEntities(m_pWeapon, pSkipEnts, 10);	
  int intersect = gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir * WEAPON_HIT_RANGE, ent_all,
		rwi_stop_at_pierceable|rwi_colltype_any, &rayhit, 1, pSkipEnts, nSkip);
	if (intersect)
		tracerhit = rayhit.pt;
	else
		tracerhit = pos + dir * WEAPON_HIT_RANGE;

	if (!m_fireparams.nearmiss_signal.empty())
		CheckNearMisses(hit, pos, dir, WEAPON_HIT_RANGE, 1.0f, m_fireparams.nearmiss_signal.c_str());

	CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, false);
	if (pAmmo)
	{
		if (m_fireparams.track_projectiles)
			pAmmo->SetTrackedByHUD();

    CGameRules* pGameRules = g_pGame->GetGameRules();

		float damage = m_fireparams.damage;
		if(m_fireparams.fake_fire_rate && pActor && !pActor->IsPlayer())
			damage = m_fireparams.ai_vs_player_damage;

		pAmmo->SetParams(m_shooterId, m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), damage, pGameRules->GetHitTypeId(m_fireparams.hit_type.c_str()), m_fireparams.damage_drop_per_meter);
    
    if (m_bLocked)
      pAmmo->SetDestination(m_lockedTarget);
    else
      pAmmo->SetDestination(m_pWeapon->GetDestination());

    pAmmo->Launch(pos, dir, vel, m_speed_scale);
    
		int frequency = m_tracerparams.frequency;

		// marcok: please don't touch
		if (g_pGameCVars->cl_tryme && (g_pGameCVars->cl_tryme_bt_ironsight || g_pGameCVars->cl_tryme_bt_speed))
		{
			frequency = 1;
		}

		bool emit = (!m_tracerparams.geometry.empty() || !m_tracerparams.effect.empty()) && (ammoCount==GetClipSize() || (ammoCount%frequency==0));
		bool ooa = ((m_fireparams.ooatracer_treshold>0) && m_fireparams.ooatracer_treshold>=ammoCount);

		if (emit || ooa)
		{
			CTracerManager::STracerParams params;
			params.position = GetTracerPos(pos, ooa);
			params.destination = tracerhit;

			if (ooa)
			{
				params.geometry = m_ooatracerparams.geometry.c_str();
				params.effect = m_ooatracerparams.effect.c_str();
				params.effectScale = params.geometryScale = m_ooatracerparams.scale;
				params.speed = m_ooatracerparams.speed;
				params.lifetime = m_ooatracerparams.lifetime;
			}
			else
			{
				params.geometry = m_tracerparams.geometry.c_str();
				params.effect = m_tracerparams.effect.c_str();
				params.effectScale = params.geometryScale = m_tracerparams.scale;
				params.speed = m_tracerparams.speed;
				params.lifetime = m_tracerparams.lifetime;
			}

			g_pGame->GetWeaponSystem()->GetTracerManager().EmitTracer(params);
		}
		m_projectileId = pAmmo->GetEntity()->GetId();
	}

  if (pActor && pActor->IsPlayer() && pActor->IsClient())
  {			
    pActor->ExtendCombat();
    // Only start a zoom in if we're not zooming in or out
    if (m_fireparams.autozoom &&
      pActor->GetScreenEffects() != 0 &&
      !pActor->GetScreenEffects()->HasJobs(pActor->m_autoZoomInID) &&
      !pActor->GetScreenEffects()->HasJobs(pActor->m_autoZoomOutID))
    {
      CLinearBlend *blend = new CLinearBlend(1);
      CFOVEffect *fovEffect = new CFOVEffect(pActor->GetEntityId(), .75f);
      pActor->GetScreenEffects()->StartBlend(fovEffect, blend, 1.0f/5.0f, pActor->m_autoZoomInID);
    }    
  }

	if (m_pWeapon->IsServer())
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, GetName(), 1, (void *)m_pWeapon->GetEntityId()));

  m_pWeapon->OnShoot(m_shooterId, pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	MuzzleFlashEffect(true); 
  SmokeEffect();
  DustEffect(pos);
	RejectEffect();
  RecoilImpulse(pos, dir);

	m_fired = true;
	m_next_shot += m_next_shot_dt;
	m_zoomtimeout = m_next_shot + 0.5f;

  if (++m_barrelId == m_fireparams.barrel_count)
    m_barrelId = 0;
	
	ammoCount--;
	if(m_fireparams.fake_fire_rate && pActor && pActor->IsPlayer() )
	{
		//Hurricane fire rate fake
		ammoCount -= Random(m_fireparams.fake_fire_rate);
		if(ammoCount<0)
			ammoCount = 0;
	}
	if (m_fireparams.clip_size != -1)
	{
		if (m_fireparams.clip_size!=0)
			m_pWeapon->SetAmmoCount(ammo, ammoCount);
		else
			m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
	}

	bool dounzoomcock=(m_pWeapon->IsZoomed() && !OutOfAmmo() && m_fireparams.unzoomed_cock);

	if (!m_fireparams.slider_layer.empty() && (dounzoomcock || (ammoCount<1)))
	{
		const char *slider_back_layer = m_fireparams.slider_layer.c_str();
		m_pWeapon->PlayLayer(slider_back_layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}

	if (dounzoomcock)
	{
		IZoomMode *pIZoomMode = m_pWeapon->GetZoomMode(m_pWeapon->GetCurrentZoomMode());
		if (pIZoomMode && pIZoomMode->IsZoomed())
		{
			CIronSight *pZoomMode=static_cast<CIronSight *>(pIZoomMode);
			pZoomMode->TurnOff(true);

			m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<CockAction>::Create(this), false);
		}
	}

	if (OutOfAmmo())
	{
		m_pWeapon->OnOutOfAmmo(ammo);

		CActor *pActor=m_pWeapon->GetOwnerActor(); 
		if (autoreload && (!pActor || pActor->IsPlayer()))
		{
			m_pWeapon->SetBusy(true);
			m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<ScheduleReload>::Create(m_pWeapon), false);
		}
	}
	//---------------------------------------------------------------------------
	// TODO remove when aiming/fire direction is working
	// debugging aiming dir
	if(++DGB_curLimit>DGB_ShotCounter)	DGB_curLimit = DGB_ShotCounter;
	if(++DGB_curIdx>=DGB_ShotCounter)	DGB_curIdx = 0;
	DGB_shots[DGB_curIdx].dst=pos+dir*200.f;
	DGB_shots[DGB_curIdx].src=pos;
	//---------------------------------------------------------------------------

	m_pWeapon->RequestShoot(ammo, pos, dir, vel, hit, pAmmo? pAmmo->GetGameObject()->GetPredictionHandle() : 0, false);

	return true;
}

//------------------------------------------------------------------------
bool CSingle::ShootFromHelper(const Vec3 &eyepos, const Vec3 &probableHit) const
{
	Vec3 dp(eyepos-probableHit);
	return dp.len2()>(WEAPON_HIT_MIN_DISTANCE*WEAPON_HIT_MIN_DISTANCE);
}

//------------------------------------------------------------------------
bool CSingle::HasFireHelper() const
{ 
  return !m_fireparams.helper[m_pWeapon->GetStats().fp?0:1].empty();
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFireHelperPos() const
{
  if (HasFireHelper())
  {
    int id = m_pWeapon->GetStats().fp?0:1;
    int slot = id?CItem::eIGS_ThirdPerson:CItem::eIGS_FirstPerson;

    return m_pWeapon->GetSlotHelperPos(slot, m_fireparams.helper[id].c_str(), true);
  }

  return Vec3(ZERO);
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFireHelperDir() const
{
  if (HasFireHelper())
  {
    int id = m_pWeapon->GetStats().fp?0:1;
    int slot = id?CItem::eIGS_ThirdPerson:CItem::eIGS_FirstPerson;

    return m_pWeapon->GetSlotHelperRotation(slot, m_fireparams.helper[id].c_str(), true).GetColumn(1);
  }  

  return FORWARD_DIRECTION;
}

//------------------------------------------------------------------------
int CSingle::GetSkipEntities(CWeapon* pWeapon, IPhysicalEntity** pSkipEnts, int nMaxSkip)
{
  int nSkip = 0;
  
  if (CActor *pActor = pWeapon->GetOwnerActor())  
  { 
    if (IVehicle* pVehicle = pActor->GetLinkedVehicle())
    {
      // skip vehicle and all child entities
      IEntity* pVehicleEntity = pVehicle->GetEntity();
      
      if (nSkip < nMaxSkip)
        pSkipEnts[nSkip++] = pVehicleEntity->GetPhysics();

      int count = pVehicleEntity->GetChildCount();      
      
      for (int c=0; c<count&&nSkip<nMaxSkip; ++c)
      {
        if (IPhysicalEntity* pPhysics = pVehicleEntity->GetChild(c)->GetPhysics())
        {
          if (pPhysics->GetType() == PE_LIVING)
            if (ICharacterInstance* pCharacter = pVehicleEntity->GetChild(c)->GetCharacter(0)) 
              if (IPhysicalEntity* pCharPhys = pCharacter->GetISkeleton()->GetCharacterPhysics())
                pPhysics = pCharPhys;
          
          pSkipEnts[nSkip++] = pPhysics;
        }
      }
    }
    else
    {
      if (nSkip < nMaxSkip)
        pSkipEnts[nSkip++] = pActor->GetEntity()->GetPhysics();

      if (IPhysicalEntity* pPhysics = pWeapon->GetEntity()->GetPhysics())
      {
        if (nSkip < nMaxSkip)
          pSkipEnts[nSkip++] = pPhysics;
      }
    }
  }  

  return nSkip;
}

//------------------------------------------------------------------------
Vec3 CSingle::GetProbableHit(float range, bool *pbHit, ray_hit *pHit) const
{
  static Vec3 pos,dir; 
  static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");

  CActor *pActor = m_pWeapon->GetOwnerActor();
    
  IPhysicalEntity* pSkipEntities[10];
  int nSkip = GetSkipEntities(m_pWeapon, pSkipEntities, 10);
  
  IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();      
  if (pLocator)
  {
    Vec3 hit;
    if (pLocator->GetProbableHit(m_pWeapon->GetEntityId(), this, hit))
      return hit;
  }
  
  IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
  if (pMC)
  { 
    SMovementState info;
    pMC->GetMovementState(info);
    
    pos = info.weaponPosition;
    
    if (!pActor->IsPlayer())
    {
      if (pAimDebug->GetIVal())
      {
        //gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(e_Def3DPublicRenderflags);
        //gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(info.fireTarget, 0.5f, ColorB(255,0,0,255));
      }
      
      dir = range * (info.fireTarget-pos).normalized();
    }
    else
		{
      dir = range * info.fireDirection;    

			// marcok: leave this alone
			if (g_pGameCVars->cl_tryme && pActor->IsClient())
			{
				CPlayer* pPlayer = (CPlayer*)pActor;
				pos = pPlayer->GetViewMatrix().GetTranslation();
			}
		}
  }
  else
  { 
    // fallback    
    pos = GetFiringPos(Vec3Constants<float>::fVec3_Zero);
    dir = range * GetFiringDir(Vec3Constants<float>::fVec3_Zero, pos);
  }


	static ray_hit hit;	
	if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir, ent_all,
		rwi_stop_at_pierceable|rwi_ignore_back_faces, &hit, 1, pSkipEntities, nSkip))
	{
 		if (pbHit)
			*pbHit=true;
		if (pHit)
			*pHit=hit;
		return hit.pt;
	}

	if (pbHit)
		*pbHit=false;

	return pos+dir;
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFiringPos(const Vec3 &probableHit) const
{
  static Vec3 pos;
	
  IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  { 
		if (pLocator->GetFiringPos(m_pWeapon->GetEntityId(), this, pos))
      return pos;
  }
	
  int id = m_pWeapon->GetStats().fp?0:1;
  int slot = id?CItem::eIGS_ThirdPerson:CItem::eIGS_FirstPerson;
  
  pos = m_pWeapon->GetEntity()->GetWorldPos();
  
	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	
  if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);

		pos = info.weaponPosition;

		// FIXME
		// should be getting it from MovementCotroller (same for AIProxy::QueryBodyInfo)
		// update: now AI always should be using the fire_pos from movement controller
		if (pActor->IsPlayer() && (HasFireHelper() && ShootFromHelper(pos, probableHit)))
		{
			// FIXME
			// making fire pos be at eye when animation is not updated (otherwise shooting from ground)
			bool	isCharacterVisible(false);
			CActor *pActor=m_pWeapon->GetOwnerActor();
			if (pActor)
			{
				IEntity *pEntity(pActor->GetEntity());
				ICharacterInstance * pCharacter(pEntity ? pEntity->GetCharacter(0) : NULL);
				if(pCharacter && pCharacter->IsCharacterVisible()!=0)
					isCharacterVisible = true;
			}
			if(isCharacterVisible)
				pos = m_pWeapon->GetSlotHelperPos(slot, m_fireparams.helper[id].c_str(), true);
		}
	}
  else
  {
    // when no MC, fall back to helper
    if (HasFireHelper())
    {
      pos = m_pWeapon->GetSlotHelperPos(slot, m_fireparams.helper[id].c_str(), true);
    }
  }

	return pos;
}


//------------------------------------------------------------------------
Vec3 CSingle::GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const
{
  static Vec3 dir;

	if (m_fireparams.autoaim && m_fireparams.autoaim_autofiringdir)
	{
		if (m_bLocked)
		{
			IEntity *pEnt = gEnv->pEntitySystem->GetEntity(m_lockedTarget);
			if (pEnt)
			{ 
				AABB bbox;
				pEnt->GetWorldBounds(bbox);
				Vec3 center = bbox.GetCenter();
				IActor *pAct = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_lockedTarget);
				if (pAct)
				{
					if (IMovementController *pMV = pAct->GetMovementController())
					{
						SMovementState ms;
						pMV->GetMovementState(ms);
						center = ms.eyePosition;
					}
				}				
				dir = (center - firingPos).normalize();
				return dir;
			}
		}
	}
	
	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  { 
    if (pLocator->GetFiringDir(m_pWeapon->GetEntityId(), this, dir, probableHit, firingPos))
      return dir;		
  }

  int id = m_pWeapon->GetStats().fp?0:1;
  int slot = id?CItem::eIGS_ThirdPerson:CItem::eIGS_FirstPerson;

  dir = m_pWeapon->GetEntity()->GetWorldRotation().GetColumn1();

	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
	if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);

		dir = info.fireDirection;

    if (HasFireHelper() && ShootFromHelper(info.weaponPosition, probableHit))
    {
      if (!pActor->IsPlayer())      
        dir = (info.fireTarget-firingPos).normalized();
      else
        dir = (probableHit-firingPos).normalized();
    }
	}  
  else
  {
    // if no MC, fall back to helper    
    if (HasFireHelper())
    { 
      dir = m_pWeapon->GetSlotHelperRotation(slot, m_fireparams.helper[id].c_str(), true).GetColumn(1);
    }
  }
  
	return dir;
}

//------------------------------------------------------------------------
Vec3 CSingle::GetFiringVelocity(const Vec3 &dir) const
{
	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  {
    Vec3 vel;
    if (pLocator->GetFiringVelocity(m_pWeapon->GetEntityId(), this, vel, dir))
      return vel;
  }

	CActor *pActor=m_pWeapon->GetOwnerActor();
	if (pActor)
	{
		IPhysicalEntity *pPE=pActor->GetEntity()->GetPhysics();
		if (pPE)
		{
			pe_status_dynamics sv;
			if (pPE->GetStatus(&sv))
			{
				if (sv.v.len2()>0.01f)
				{
					float dot=sv.v.GetNormalized().Dot(dir);
					if (dot<0.0f)
						dot=0.0f;


					//CryLogAlways("velocity dot: %.3f", dot);

					return sv.v*dot;
				}
			}
		}
	}

	return Vec3(0,0,0);
}

//------------------------------------------------------------------------
Vec3 CSingle::NetGetFiringPos(const Vec3 &probableHit) const
{
	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  {
    Vec3 pos;
		if (pLocator->GetFiringPos(m_pWeapon->GetEntityId(), this, pos))
      return pos;
  }

	int id = m_pWeapon->GetStats().fp?0:1;
	int slot = id?CItem::eIGS_ThirdPerson:CItem::eIGS_FirstPerson;

	Vec3 pos = m_pWeapon->GetEntity()->GetWorldPos();

	CActor *pActor = m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;

	if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);

		pos = info.weaponPosition;

		if (!m_fireparams.helper[id].empty() && ShootFromHelper(pos, probableHit))
			pos = m_pWeapon->GetSlotHelperPos(slot, m_fireparams.helper[id].c_str(), true);
	}
	else if (!m_fireparams.helper[id].empty())
		pos = m_pWeapon->GetSlotHelperPos(slot, m_fireparams.helper[id].c_str(), true);

	return pos;
}

//------------------------------------------------------------------------
Vec3 CSingle::NetGetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const
{
	IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();
	if (pLocator)
  {
    Vec3 dir;
		if (pLocator->GetFiringDir(m_pWeapon->GetEntityId(), this, dir, probableHit, firingPos))
      return dir;
  }
	
	Vec3 dir = (probableHit-firingPos).normalized();

	return dir;
}

//------------------------------------------------------------------------
Vec3 CSingle::NetGetFiringVelocity(const Vec3 &dir) const
{
	return GetFiringVelocity(dir);
}

//------------------------------------------------------------------------
Vec3 CSingle::ApplySpread(const Vec3 &dir, float spread)
{
	Ang3 angles=Ang3::GetAnglesXYZ(Matrix33::CreateRotationVDir(dir));
	
	float rx=Random()-0.5f;
	float rz=Random()-0.5f;

	angles.x+=rx*DEG2RAD(spread);
	angles.z+=rz*DEG2RAD(spread);

	return Matrix33::CreateRotationXYZ(angles).GetColumn(1).normalized();
}

//------------------------------------------------------------------------
Vec3 CSingle::GetTracerPos(const Vec3 &firingPos, bool ooa)
{
	int id=m_pWeapon->GetStats().fp?0:1;
	int slot=id?CItem::eIGS_ThirdPerson:CItem::eIGS_FirstPerson;
	const char *helper=0;
	
	if (ooa)
		helper=m_ooatracerparams.helper[id].c_str();
	else
		helper=m_tracerparams.helper[id].c_str();

	if (!helper[0])
		return firingPos;

	return m_pWeapon->GetSlotHelperPos(slot, helper, true);
}
//------------------------------------------------------------------------
void CSingle::SetupEmitters(bool attach)
{
	if (attach)
	{		
		int id = m_pWeapon->GetStats().fp ? 0 : 1;
    Vec3 offset(ZERO);

    if (m_muzzleflash.helper[id].empty())
    { 
      // if no helper specified, try getting pos from firing locator
      IWeaponFiringLocator *pLocator = m_pWeapon->GetFiringLocator();            
      
      if (pLocator && pLocator->GetFiringPos(m_pWeapon->GetEntityId(), this, offset))
        offset = m_pWeapon->GetEntity()->GetWorldTM().GetInvertedFast() * offset;
    }
            
		if (!m_muzzleflash.effect[0].empty())
		{   
			m_mfIds[m_barrelId].mfId[0] = m_pWeapon->AttachEffect(CItem::eIGS_FirstPerson, -1, true, m_muzzleflash.effect[0].c_str(), 
        m_muzzleflash.helper[0].c_str(), offset, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}
		if (!m_muzzleflash.effect[1].empty())
		{
			m_mfIds[m_barrelId].mfId[1] = m_pWeapon->AttachEffect(CItem::eIGS_ThirdPerson, -1, true, m_muzzleflash.effect[1].c_str(), 
				m_muzzleflash.helper[1].c_str(), offset, Vec3Constants<float>::fVec3_OneY, 1.0f, false);
		}
	}
	else
	{
    for (int i=0; i<m_mfIds.size(); ++i)
    {
      m_mfIds[i].mfId[0] = m_pWeapon->AttachEffect(CItem::eIGS_FirstPerson, m_mfIds[i].mfId[0], false);
      m_mfIds[i].mfId[1] = m_pWeapon->AttachEffect(CItem::eIGS_ThirdPerson, m_mfIds[i].mfId[1], false);
    }
	}
}


//------------------------------------------------------------------------
void CSingle::MuzzleFlashEffect(bool attach, bool light, bool effect)
{ 
  // muzzle effects & lights are permanently attached and emitted on attach==true
  // calling with attach==false removes the emitters
  if (attach)
	{    
    int slot = m_pWeapon->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
    int id = m_pWeapon->GetStats().fp ? 0 : 1;

		if (effect)
    {
      if (!m_muzzleflash.effect[id].empty() && !m_pWeapon->GetEntity()->IsHidden())
		  {
			  if (!m_mfIds[m_barrelId].mfId[id])
				  SetupEmitters(true);		

			  IParticleEmitter *pEmitter = m_pWeapon->GetEffectEmitter(m_mfIds[m_barrelId].mfId[id]);
			  if (pEmitter)
				  pEmitter->EmitParticle();
		  }		  
    }
		
    if (light && m_muzzleflash.light_radius[id] != 0.f)
		{
      if (!m_mflightId[id])
      {
			  m_mflightId[id] = m_pWeapon->AttachLight(slot, 0, true, false, m_muzzleflash.light_radius[id],          
          m_muzzleflash.light_color[id], 1.0f, 0, 0, m_muzzleflash.light_helper[id].c_str());
          //m_muzzleflash.light_color[id], Vec3Constants<float>::fVec3_One, 0, 0, m_muzzleflash.light_helper[id].c_str());
      }
      
      m_pWeapon->EnableLight(true, m_mflightId[id]);
			m_mflTimer = m_muzzleflash.light_time[id];

			// Report muzzle flash to AI.
			if (m_pWeapon->GetOwner() && m_pWeapon->GetOwner()->GetAI())
			{
				IAIObject* pShooter = m_pWeapon->GetOwner()->GetAI();
				gEnv->pAISystem->DynOmniLightEvent(m_pWeapon->GetOwner()->GetWorldPos(), m_muzzleflash.light_radius[id], pShooter);
			}
		}
	}
  else
  {
    if (effect)
      SetupEmitters(false);
    
    if (light)
    {
      m_mflightId[0] = m_pWeapon->AttachLight(CItem::eIGS_FirstPerson, m_mflightId[0], false);      
      m_mflightId[1] = m_pWeapon->AttachLight(CItem::eIGS_ThirdPerson, m_mflightId[1], false);
    }
  }
}

//------------------------------------------------------------------------
class CSingle::SmokeEffectAction
{
public:
	SmokeEffectAction(const char *_effect, const char *_helper, bool _fp)
	{
		helper=_helper;
		effect=_effect;
		fp=_fp;
	}

	void execute(CItem *_item)
	{
		if (fp!=_item->GetStats().fp)
			return;

		Vec3 dir(0,1.0f,0);
		CActor *pActor = _item->GetOwnerActor();
		IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
		if (pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);
			dir = info.aimDirection;
		}

		int slot = _item->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
		_item->SpawnEffect(slot, effect.c_str(), helper.c_str(), Vec3(0,0,0), dir);
	}

	string effect;
	string helper;
	bool fp;
};

void CSingle::SmokeEffect(bool effect)
{
	if (effect)
	{
		int id = m_pWeapon->GetStats().fp ? 0 : 1;
		if (!m_muzzlesmoke.effect[id].empty())
			m_pWeapon->GetScheduler()->TimerAction((int)m_muzzlesmoke.time[id],
			CSchedulerAction<SmokeEffectAction>::Create(SmokeEffectAction(m_muzzlesmoke.effect[id].c_str(), m_muzzlesmoke.helper[id].c_str(), m_pWeapon->GetStats().fp)), false);;
	}
}

//------------------------------------------------------------------------
void CSingle::DustEffect(const Vec3& pos)
{
  if (!m_dustparams.mfxtag.empty())
  { 
    IPhysicalEntity* pSkipEnts[10];
    int nSkip = GetSkipEntities(m_pWeapon, pSkipEnts, 10);
    ray_hit hit;    
    
    if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, m_dustparams.maxheight*Vec3(0,0,-1), ent_static|ent_terrain|ent_water, 0, &hit, 1, pSkipEnts, nSkip))
    {
      IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
      
      TMFXEffectId effectId = pMaterialEffects->GetEffectId(m_dustparams.mfxtag.c_str(), hit.surface_idx);
      if (effectId != InvalidEffectId)
      {
        SMFXRunTimeEffectParams params;
        params.pos = hit.pt;           
        params.normal = hit.n;
        
        if (m_dustparams.maxheightscale < 1.f)
          params.scale = 1.f - (hit.dist/m_dustparams.maxheight)*(1.f-m_dustparams.maxheightscale);
        
        pMaterialEffects->ExecuteEffect(effectId, params);
      }
    }
  }
}

//------------------------------------------------------------------------
void CSingle::SpinUpEffect(bool attach)
{ 
  m_pWeapon->AttachEffect(0, m_suId, false);
	m_pWeapon->AttachLight(0, m_sulightId, false);
	m_suId=0;
	m_sulightId=0;

	if (attach)
	{
		int slot = m_pWeapon->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
		int id = m_pWeapon->GetStats().fp ? 0 : 1;

		if (!m_spinup.effect[0].empty() || !m_spinup.effect[1].empty())
		{
      //CryLog("[%s] spinup effect (true)", m_pWeapon->GetEntity()->GetName());

			m_suId = m_pWeapon->AttachEffect(slot, 0, true, m_spinup.effect[id].c_str(), 
        m_spinup.helper[id].c_str(), Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, 1.0f, false);

      //m_sulightId = m_pWeapon->AttachLight(slot, 0, true, m_spinup.light_radius[id], m_spinup.light_color[id], Vec3(1,1,1), 0, 0,
			m_sulightId = m_pWeapon->AttachLight(slot, 0, true, false, m_spinup.light_radius[id], m_spinup.light_color[id], 1, 0, 0,
				m_spinup.light_helper[id].c_str());
		}

		m_suTimer = (uint)(m_spinup.time[id]);
	}
  else
  {
    //CryLog("[%s] spinup effect (false)", m_pWeapon->GetEntity()->GetName());
  }
}

//------------------------------------------------------------------------
void CSingle::RejectEffect()
{
	if (g_pGameCVars->i_rejecteffects==0)
		return;

	int slot = m_pWeapon->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
	int id = m_pWeapon->GetStats().fp ? 0 : 1;
	 
	if (!m_reject.effect[id].empty())
	{
		Vec3 front(m_pWeapon->GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION));
		Vec3 up(m_pWeapon->GetEntity()->GetWorldTM().TransformVector(Vec3(0.0f,0.0f,1.0f)));

		CActor *pActor = m_pWeapon->GetOwnerActor();
		IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
		if (pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);

			front = info.aimDirection;			
			up = info.upDirection;
		}
	
		IActor *pClientActor=gEnv->pGame->GetIGameFramework()->GetClientActor();
		if (pClientActor)	
		{
			Vec3 vPlayerPos=pClientActor->GetEntity()->GetWorldPos();
			Vec3 vEffectPos=m_pWeapon->GetEntity()->GetWorldPos();
			float fDist2=(vPlayerPos-vEffectPos).len2();
			if (fDist2>25.0f*25.0f)			
				return; // too far, do not spawn physicalized empty shells and make sounds 
		}

		Vec3 dir = front.Cross(up);
		float dot = fabs_tpl(front.Dot(up));

		dir+=(up*(1.0f-dot)*0.65f);
		dir.normalize();
		m_pWeapon->SpawnEffect(slot, m_reject.effect[id].c_str(), m_reject.helper[id].c_str(),
			Vec3(0,0,0), dir, m_reject.scale[id]);
	}
}

//------------------------------------------------------------------------
int CSingle::GetDamage() const
{
	return m_fireparams.damage;
}

//------------------------------------------------------------------------
float CSingle::GetRecoil() const
{
	return m_recoil;
}

//------------------------------------------------------------------------
float CSingle::GetRecoilScale() const
{

	float mult = 1.0f;
	CActor *owner = m_pWeapon->GetOwnerActor();
	if (owner && owner->GetActorClass() == CPlayer::GetActorClassType())
	{
		//TODO (Remove) - Probably this code is not needed since we tested this same thing in UpdateRecoil 
		CPlayer *plr = (CPlayer *)owner;
		if(plr->GetNanoSuit())
		{
			ENanoMode curMode = plr->GetNanoSuit()->GetMode();
			if (curMode == NANOMODE_STRENGTH)
				mult = m_recoilparams.recoil_strMode_m;
		}
	}

	//Same as for the spread (apply stance multipliers)
	float stanceScale=1.0f;

	if(owner)
	{
		bool inAir=owner->GetActorStats()->inAir>=0.05f;
		bool inZeroG = owner->GetActorStats()->inZeroG;

		if (owner->GetStance()==STANCE_CROUCH && !inAir)
			stanceScale = m_recoilparams.recoil_crouch_m;
		else if (owner->GetStance()==STANCE_PRONE && !inAir)
			stanceScale = m_recoilparams.recoil_prone_m;
		else if (inAir && !inZeroG)
			stanceScale = m_recoilparams.recoil_jump_m;
		else if(inZeroG)
			stanceScale = m_recoilparams.recoil_zeroG_m;
	}

	IZoomMode *pZoomMode=m_pWeapon->GetZoomMode(m_pWeapon->GetCurrentZoomMode());
	if (!pZoomMode)
		return 1.0f*mult*m_recoilMultiplier*stanceScale;

	return pZoomMode->GetRecoilScale()*mult*m_recoilMultiplier*stanceScale; 
}

//------------------------------------------------------------------------
float CSingle::GetSpread() const
{
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (!pActor)
		return m_spread;

	float stanceScale=1.0f;
	float speedSpread=0.0f;
	float rotationSpread=0.0f;
	float dualWieldScale=1.0f;	//<--Parametrize this one too

	bool inAir=pActor->GetActorStats()->inAir>=0.05f;
	bool inZeroG=pActor->GetActorStats()->inZeroG;

	if (pActor->GetStance()==STANCE_CROUCH && !inAir)
		stanceScale = m_spreadparams.spread_crouch_m;
	else if (pActor->GetStance()==STANCE_PRONE && !inAir)
		stanceScale = m_spreadparams.spread_prone_m;
	else if (inAir && !inZeroG)
		stanceScale = m_spreadparams.spread_jump_m;
	else if (inZeroG)
		stanceScale = m_spreadparams.spread_zeroG_m;

	pe_status_dynamics dyn;

	IPhysicalEntity *pPhysicalEntity=pActor->GetEntity()->GetPhysics();
	if (pPhysicalEntity && pPhysicalEntity->GetStatus(&dyn))
	{
		speedSpread=dyn.v.len()*m_spreadparams.speed_m;
		rotationSpread=dyn.w.len()*m_spreadparams.rotation_m;
		rotationSpread = CLAMP(rotationSpread,0.0f,3.0f);
	}

	if(m_pWeapon->IsDualWield())
		dualWieldScale = 0.5f;
	
	IZoomMode *pZoomMode= m_pWeapon->GetZoomMode(m_pWeapon->GetCurrentZoomMode());
	if (!pZoomMode)
		return (speedSpread+rotationSpread+m_spread)*stanceScale*dualWieldScale;

	return pZoomMode->GetRecoilScale()*(speedSpread+rotationSpread+m_spread)*stanceScale*dualWieldScale;
	
	//return (speedSpread+m_spread)*stanceScale;
}

//------------------------------------------------------------------------
const char *CSingle::GetCrosshair() const
{
	return m_fireparams.crosshair.c_str();
}

//------------------------------------------------------------------------
float CSingle::GetHeat() const
{
	return m_heat;
}

//------------------------------------------------------------------------
void CSingle::UpdateHeat(float frameTime)
{
	float oldheat=m_heat;

	if (m_heatingparams.attack>0.0f)
	{
		if (m_overheat>0.0f)
		{
			m_overheat-=frameTime;
			if (m_overheat<=0.0f)
			{
				m_overheat=0.0f;
				
        m_pWeapon->StopSound(m_heatSoundId);
				m_pWeapon->PlayAction(m_actions.cooldown);				
			}
		}
		else
		{
			float add=0.0f;
			float sub=0.0f;

			if (m_fired)
				add=m_heatingparams.attack;
			else if (m_next_shot<=0.0001f)
				sub=frameTime/m_heatingparams.decay;

			m_heat += add-sub;
			m_heat = CLAMP(m_heat, 0.0f, 1.0f);

      static ICVar* pAimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
      if (pAimDebug && pAimDebug->GetIVal() > 1)
      {
        float color[] = {1,1,1,1};
        gEnv->pRenderer->Draw2dLabel(300, 300, 1.2f, color, false, "    + %.2f", add);
        gEnv->pRenderer->Draw2dLabel(300, 315, 1.2f, color, false, "    - %.2f", sub);
        gEnv->pRenderer->Draw2dLabel(300, 335, 1.3f, color, false, "heat: %.2f", m_heat);
      }

			if (m_heat >= 0.999f && oldheat<0.999f)
			{
				m_overheat=m_heatingparams.duration;

				StopFire(m_shooterId);
				m_heatSoundId = m_pWeapon->PlayAction(m_actions.overheating);

				int slot=m_pWeapon->GetStats().fp?CItem::eIGS_FirstPerson:CItem::eIGS_ThirdPerson;
				if (!m_heatingparams.effect[slot].empty())
        {
          if (m_heatEffectId)
            m_heatEffectId = m_pWeapon->AttachEffect(0, m_heatEffectId, false);

					m_heatEffectId = m_pWeapon->AttachEffect(slot, 0, true, m_heatingparams.effect[slot].c_str(), m_heatingparams.helper[slot].c_str());
        }
			}
		}

		if (m_heat>=0.0001f)
			m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CSingle::UpdateRecoil(float frameTime)
{
	//float white[4]={1,1,1,1};
	// spread
	float spread_add = 0.0f;
	float spread_sub = 0.0f;
	bool dw=m_pWeapon->IsDualWield();

	CActor *pActor=m_pWeapon->GetOwnerActor();
	IAIObject *pOwnerAI=(pActor && pActor->GetEntity()) ? pActor->GetEntity()->GetAI() : 0;
	bool isOwnerAIControlled = pOwnerAI && pOwnerAI->GetAIType() != AIOBJECT_PLAYER;

	if(isOwnerAIControlled)
	{
		// The AI system will offset the shoot target in any case, so do not apply spread.
		m_spread = 0.0f;
	}
	else
	{
		if (m_spreadparams.attack>0.0f)
		{
			// shot
			float attack=m_spreadparams.attack;
			if (dw) // experimental recoil increase for dual wield
				attack *= 1.20f;

			if (m_fired)
				spread_add = m_spreadparams.attack;

			float decay=m_recoilparams.decay;
			if (dw) // experimental recoil increase for dual wield
				decay*=1.25f;

			spread_sub = frameTime*(m_spreadparams.max-m_spreadparams.min)/decay;

			m_spread += (spread_add-spread_sub)*m_recoilMultiplier;
			m_spread = CLAMP(m_spread, m_spreadparams.min, m_spreadparams.max*m_recoilMultiplier);
			//gEnv->pRenderer->Draw2dLabel(50,75,2.0f,white,false,"Current Spread: %f", m_spread);
		}
		else
			m_spread = m_spreadparams.min;
	}

	// recoil
	static bool lastModeStrength = false;
	float nanoSuitScale=1.0f;
	if (pActor && pActor->GetActorClass() == CPlayer::GetActorClassType())
	{
		CPlayer *pPlayer=static_cast<CPlayer *>(pActor);
		CNanoSuit* pSuit = pPlayer->GetNanoSuit();
		float strength = 0.0f;
		if(pSuit)
		{
			strength = pSuit->GetSlotValue(NANOSLOT_STRENGTH);
			if(pSuit->GetMode()==NANOMODE_STRENGTH)
			{
				if(!lastModeStrength)
					m_recoil *= 0.25f;   //Reduce recoil when switching to strenght while firing
				lastModeStrength = true;
			}
			else
				lastModeStrength = false;
		}

		if (strength>0)
			nanoSuitScale=1.0f-(strength/100.0f)*0.25f;
	}

	float scale = GetRecoilScale()*nanoSuitScale;
	float recoil_add = 0.0f;
	float recoil_sub = 0.0f;

	if (m_recoilparams.decay>0.0f)
	{
		if (m_fired)
		{
			float attack=m_recoilparams.attack;
			if (dw) // experimental recoil increase for dual wield
				attack*=1.20f;

			recoil_add = attack*scale;
			//CryLogAlways("recoil scale: %.3f", scale);

			if (pActor)
			{
				SGameObjectEvent e(eCGE_Recoil,eGOEF_ToExtensions);
				e.param = (void*)(&recoil_add);
				pActor->HandleEvent(e);
			}
		}

		float decay=m_recoilparams.decay;
		if (dw) // experimental recoil increase for dual wield
			decay*=1.25f;

		recoil_sub = frameTime*m_recoilparams.max_recoil/decay;
		recoil_sub *= scale;
		m_recoil += recoil_add-recoil_sub;

		m_recoil = CLAMP(m_recoil, 0.0f, m_recoilparams.max_recoil*m_recoilMultiplier);

		//gEnv->pRenderer->Draw2dLabel(50,50,2.0f,white,false,"Current recoil: %f", m_recoil);
	}
	else
		m_recoil = 0.0f;

	if ((m_recoilparams.max.x>0) || (m_recoilparams.max.y>0))
	{
		Vec2 recoil_dir_add(0.0f, 0.0f);

		if (m_fired)
		{
			int n = m_recoilparams.hints.size();

			if (m_recoil_dir_idx >= 0 && m_recoil_dir_idx<n)
			{
				recoil_dir_add = m_recoilparams.hints[m_recoil_dir_idx];

				if (m_recoil_dir_idx+1>=n)
				{
					if (m_recoilparams.hint_loop_start<n)
						m_recoil_dir_idx = m_recoilparams.hint_loop_start;
					else
						m_recoil_dir_idx = 0;
				}
				else
					++m_recoil_dir_idx;
			}
		}

		CActor *pOwner = m_pWeapon->GetOwnerActor();
		CItem  *pMaster = NULL;
		if(m_pWeapon->IsDualWieldSlave())
			pMaster = static_cast<CItem*>(m_pWeapon->GetDualWieldMaster());

		if (pOwner && (m_pWeapon->IsCurrentItem() || (pMaster && pMaster->IsCurrentItem())))
		{
			if (m_fired)
			{
				Vec2 rdir(Random()*m_recoilparams.randomness,BiRandom(m_recoilparams.randomness));
				m_recoil_dir = Vec2(recoil_dir_add.x+rdir.x, recoil_dir_add.y+rdir.y);
				m_recoil_dir.NormalizeSafe();
			}

			if (m_recoil > 0.001f)
			{
				float t = m_recoil/m_recoilparams.max_recoil;
				Vec2 new_offset = Vec2(m_recoil_dir.x*m_recoilparams.max.x, m_recoil_dir.y*m_recoilparams.max.y)*t*3.141592f/180.0f;
				m_recoil_offset = new_offset*0.66f+m_recoil_offset*0.33f;
				pOwner->SetViewAngleOffset(Vec3(m_recoil_offset.x, 0.0f, m_recoil_offset.y));

				m_pWeapon->RequireUpdate(eIUS_FireMode);
			}
			else
				ResetRecoil(false);
		}
	}
	else
		ResetRecoil();

	/*g_shoots.clear();
	for (std::vector<DebugShoot>::iterator it=g_shoots.begin(); g_shoots.end() != it; it++)
	{
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(it->pos, ColorB(200, 200, 0), it->hit, ColorB(200, 200, 0) );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(it->pos, ColorB(0, 200, 0), it->pos+it->dir*((it->hit-it->pos).len()), ColorB(0, 200, 0) );

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(it->pos, 0.125, ColorB(200, 0, 0));
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(it->hit, 0.125, ColorB(0, 0, 200));
	}*/
	
}

//------------------------------------------------------------------------
void CSingle::ResetRecoil(bool spread)
{
	m_recoil = 0.0f;
	m_recoil_dir_idx = 0;
	m_recoil_dir = Vec2(0.0f,0.0f);
	m_recoil_offset = Vec2(0.0f,0.0f);
	if (spread)
		m_spread = m_spreadparams.min;

	CActor *pOwner = m_pWeapon->GetOwnerActor();
	if (pOwner && m_pWeapon->IsCurrentItem())
		pOwner->SetViewAngleOffset(Vec3(0.0f,0.0f,0.0f));
}

//------------------------------------------------------------------------
void CSingle::NetShoot(const Vec3 &hit, int predictionHandle)
{
	Vec3 pos = NetGetFiringPos(hit);
	Vec3 dir = ApplySpread(NetGetFiringDir(hit, pos), GetSpread());
	Vec3 vel = NetGetFiringVelocity(dir);

	NetShootEx(pos, dir, vel, hit, predictionHandle);
}

//------------------------------------------------------------------------
void CSingle::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int predictionHandle)
{
	IEntityClass* ammo = m_fireparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);
	if (m_fireparams.clip_size==0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	const char *action = m_actions.fire_cock.c_str();
	if (ammoCount == 1)
		action = m_actions.fire.c_str();

	m_pWeapon->ResetAnimation();

	int flags = CItem::eIPAF_Default|CItem::eIPAF_NoBlend;
	flags = PlayActionSAFlags(flags);
	m_pWeapon->PlayAction(action, 0, false, flags);

	CProjectile *pAmmo = GetISystem()->IsDemoMode() == 2 ? NULL : m_pWeapon->SpawnAmmo(ammo, true);
	if (pAmmo)
	{
		m_shooterId=m_pWeapon->GetOwnerId(); // hax
    int hitTypeId = g_pGame->GetGameRules()->GetHitTypeId(m_fireparams.hit_type.c_str());			
		pAmmo->SetParams(m_shooterId, m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), m_fireparams.damage, hitTypeId, m_fireparams.damage_drop_per_meter);
		
		if (m_bLocked)
			pAmmo->SetDestination(m_lockedTarget);
		else
			pAmmo->SetDestination(m_pWeapon->GetDestination());

		pAmmo->SetRemote(true);
		pAmmo->Launch(pos, dir, vel);

		bool emit = (!m_tracerparams.geometry.empty() || !m_tracerparams.effect.empty()) && (ammoCount==GetClipSize() || (ammoCount%m_tracerparams.frequency==0));
		bool ooa = ((m_fireparams.ooatracer_treshold>0) && m_fireparams.ooatracer_treshold>=ammoCount);

		if (emit || ooa)
		{	
			CTracerManager::STracerParams params;
			params.position = GetTracerPos(pos, ooa);
			params.destination = hit;

			if (ooa)
			{
				params.geometry = m_ooatracerparams.geometry.c_str();
				params.effect = m_ooatracerparams.effect.c_str();
				params.effectScale = params.geometryScale = m_ooatracerparams.scale;
				params.speed = m_ooatracerparams.speed;
				params.lifetime = m_ooatracerparams.lifetime;
			}
			else
			{
				params.geometry = m_tracerparams.geometry.c_str();
				params.effect = m_tracerparams.effect.c_str();
				params.effectScale = params.geometryScale = m_tracerparams.scale;
				params.speed = m_tracerparams.speed;
				params.lifetime = m_tracerparams.lifetime;
			}

			g_pGame->GetWeaponSystem()->GetTracerManager().EmitTracer(params);
		}

		m_projectileId = pAmmo->GetEntity()->GetId();
	}

	if (m_pWeapon->IsServer())
		g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(m_pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, GetName(), 1, (void *)m_pWeapon->GetEntityId()));

  m_pWeapon->OnShoot(m_shooterId, pAmmo?pAmmo->GetEntity()->GetId():0, ammo, pos, dir, vel);

	MuzzleFlashEffect(true);
  DustEffect(pos);
	RejectEffect();
  RecoilImpulse(pos, dir);

	m_fired = true;
	m_next_shot = 0.0f;

  if (++m_barrelId == m_fireparams.barrel_count)
    m_barrelId = 0;

	CActor* pActor = m_pWeapon->GetOwnerActor();

	ammoCount--;
	if(m_fireparams.fake_fire_rate && pActor && pActor->IsPlayer() )
	{
		//Hurricane fire rate fake
		ammoCount -= Random(m_fireparams.fake_fire_rate);
		if(ammoCount<0)
			ammoCount = 0;
	}
	if (m_pWeapon->IsServer())
	{
		if (m_fireparams.clip_size != -1)
		{
			if (m_fireparams.clip_size!=0)
				m_pWeapon->SetAmmoCount(ammo, ammoCount);
			else
				m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
		}
	}

	if ((ammoCount<1) && !m_fireparams.slider_layer.empty())
	{
		const char *slider_back_layer = m_fireparams.slider_layer.c_str();
		m_pWeapon->PlayLayer(slider_back_layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}

	if (OutOfAmmo())
		m_pWeapon->OnOutOfAmmo(ammo);

	if (pAmmo && predictionHandle)
	{
		CActor * pActor = m_pWeapon->GetOwnerActor();
		if (pActor)
		{
			pAmmo->GetGameObject()->RegisterAsValidated(pActor->GetGameObject(), predictionHandle);
			pAmmo->GetGameObject()->BindToNetwork();
		}
	}

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CSingle::RecoilImpulse(const Vec3& firingPos, const Vec3& firingDir)
{
  // todo: integrate the impulse params when time..
  if (m_recoilparams.impulse > 0.f)
  {
    EntityId id = (m_pWeapon->GetHostId()) ? m_pWeapon->GetHostId() : m_pWeapon->GetOwnerId();
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id);

    if (pEntity && pEntity->GetPhysics())
    {        
      pe_action_impulse impulse;
      impulse.impulse = -firingDir * m_recoilparams.impulse; 
      impulse.point = firingPos;
      pEntity->GetPhysics()->Action(&impulse);
    }
  }
  
  CActor* pActor = m_pWeapon->GetOwnerActor();
  if (pActor && pActor->IsPlayer())
  {
		if(pActor->InZeroG())
		{
			IEntityPhysicalProxy *pPhysicsProxy = (IEntityPhysicalProxy*)pActor->GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS);
			SMovementState ms;
			pActor->GetMovementController()->GetMovementState(ms);
			CPlayer *plr = (CPlayer *)pActor;
			if(m_recoilparams.back_impulse > 0.0f)
			{
				Vec3 impulseDir = ms.aimDirection * -1.0f;
				Vec3 impulsePos = ms.pos;
				float impulse = m_recoilparams.back_impulse;
				if(plr->GetNanoSuit() && plr->GetNanoSuit()->GetMode() == NANOMODE_STRENGTH)
					impulse *= 0.75f;
				pPhysicsProxy->AddImpulse(-1, impulsePos, impulseDir * impulse * 100.0f, true, 1.0f);
			}
			if(m_recoilparams.angular_impulse > 0.0f)
			{
				float impulse = m_recoilparams.angular_impulse;
				if(plr->GetNanoSuit() && plr->GetNanoSuit()->GetMode() == NANOMODE_STRENGTH)
					impulse *= 0.5f;
				pActor->AddAngularImpulse(Ang3(0,impulse,0), 1.0f);
			}
		}

		if(pActor->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 0.0f, fabsf(m_recoilparams.back_impulse)*2.0f) );
  }
}

//------------------------------------------------------------------------
void CSingle::CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius, const char *signalName)
{
	if (!m_pWeapon->IsOwnerFP())	// only works for first person shooters
		return;

	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	IActorSystem *pActorSystem=g_pGame->GetIGameFramework()->GetIActorSystem();
	IPhysicalWorld *pPhysicalWorld=gEnv->pPhysicalWorld;

	CCamera camera(GetISystem()->GetViewCamera());	// create a virtual camera with a very small fov to check for near misses
	camera.SetFrustum(camera.GetViewSurfaceX(),camera.GetViewSurfaceZ(),3.141592f/12.0f,camera.GetNearPlane(), camera.GetFarPlane());

	int count=pActorSystem->GetActorCount();
	if (count > 0)
	{
		IActorIteratorPtr pIter = pActorSystem->CreateActorIterator();
		while (IActor* pActor = pIter->Next())
		{
			if (!pActor || pActor->GetHealth()<=0)
				continue;

			IPhysicalEntity *pPE=pActor->GetEntity()->GetPhysics();
			if (!pPE)
				continue;
	
			if (!camera.IsPointVisible(pActor->GetEntity()->GetWorldPos()))
				continue;

			static ray_hit hit;
			if (pPhysicalWorld->CollideEntityWithBeam(pPE, pos, dir*range, 1, &hit))
			{
				float dp2=(hit.pt-probableHit).len2();
				float dd2=(hit.dist+2.0f)*(hit.dist+2.0f); // 2 meters intolerance
				if (hit.dist>=2.0f && hit.dist+2*hit.dist<=dp2)
				{
					//CryLogAlways("Near Miss!!! %s :: Signal: %s", pActor->GetEntity()->GetName(), signalName);
	
					IAIObject *pObject=m_pWeapon->GetOwnerActor()?m_pWeapon->GetOwnerActor()->GetEntity()->GetAI():0;
					gEnv->pAISystem->SendAnonymousSignal(1,signalName,hit.pt,2.0f,pObject,0);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CSingle::CacheTracer()
{
	if (!m_tracerparams.geometry.empty())
	{
		IStatObj *pStatObj = gEnv->p3DEngine->LoadStatObj(m_tracerparams.geometry.c_str());
		if (pStatObj)
		{
			pStatObj->AddRef();
			m_tracerCache.push_back(pStatObj);
		}
	}

	if (!m_ooatracerparams.geometry.empty())
	{
		IStatObj *pStatObj = gEnv->p3DEngine->LoadStatObj(m_ooatracerparams.geometry.c_str());
		if (pStatObj)
		{
			pStatObj->AddRef();
			m_tracerCache.push_back(pStatObj);
		}
	}
}

//------------------------------------------------------------------------
void CSingle::ClearTracerCache()
{
	for (std::vector<IStatObj *>::iterator it=m_tracerCache.begin(); it!=m_tracerCache.end(); ++it)
		(*it)->Release();

	m_tracerCache.resize(0);
}
//------------------------------------------------------------------------
void CSingle::SetName(const char *name)
{
	m_name = name;
}
//------------------------------------------------------------------------
float CSingle::GetProjectileFiringAngle(float v, float g, float x, float y)
{
	float angle=0.0,t,a;

	// Avoid square root in script
	float d = cry_sqrtf(powf(v,4)-2*g*y*powf(v,2)-powf(g,2)*powf(x,2));
	if(d>=0)
	{
		a=powf(v,2)-g*y;
		if (a-d>0) {
			t=cry_sqrtf(2*(a-d)/powf(g,2));
			angle = (float)acos_tpl(x/(v*t));	
			float y_test;
			y_test=float(-v*sin_tpl(angle)*t-g*powf(t,2)/2);
			if (fabsf(y-y_test)<0.02f)
				return RAD2DEG(-angle);
			y_test=float(v*sin_tpl(angle)*t-g*pow(t,2)/2);
			if (fabsf(y-y_test)<0.02f)
				return RAD2DEG(angle);
		}
		t = cry_sqrtf(2*(a+d)/powf(g,2));
		angle = (float)acos_tpl(x/(v*t));	
		float y_test=float(v*sin_tpl(angle)*t-g*pow(t,2)/2);

		if (fabsf(y-y_test)<0.02f)
			return RAD2DEG(angle);

		return 0;
	}
	return 0;
}
//------------------------------------------------------------------------
void CSingle::Cancel()
{
	m_targetSpotSelected = false;
}
//------------------------------------------------------------------------
bool CSingle::AllowZoom() const
{
	return !m_targetSpotSelected && !m_firing;
}
//------------------------------------------------------------------------

void CSingle::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_tracerCache);
	m_fireparams.GetMemoryStatistics(s);
	m_tracerparams.GetMemoryStatistics(s);
	m_ooatracerparams.GetMemoryStatistics(s);
	m_actions.GetMemoryStatistics(s);
	m_muzzleflash.GetMemoryStatistics(s);
	m_muzzlesmoke.GetMemoryStatistics(s);
	m_reject.GetMemoryStatistics(s);
	m_spinup.GetMemoryStatistics(s);
	m_recoilparams.GetMemoryStatistics(s);
	m_spreadparams.GetMemoryStatistics(s);
	m_heatingparams.GetMemoryStatistics(s);
	m_dustparams.GetMemoryStatistics(s);
	s->Add(m_name);

	m_spreadparamsCopy.GetMemoryStatistics(s);
	m_recoilparamsCopy.GetMemoryStatistics(s);

}

//------------------------------------------------------------------------------
//BackUpOriginalSpreadRecoil()
//Store original spread/recoil parameters (not all are needed). We will use this copy
//to restore the original values after un-zomming the weapon (this way we don't lose
//precision after some mult/div modifications

void CSingle::BackUpOriginalSpreadRecoil()
{
	//Spread (All the values)
	m_spreadparamsCopy = m_spreadparams;

	//Recoil (Don't need all)
	m_recoilparamsCopy.angular_impulse			= m_recoilparams.angular_impulse;
	m_recoilparamsCopy.attack								= m_recoilparams.attack;
	m_recoilparamsCopy.back_impulse					= m_recoilparams.back_impulse;
	m_recoilparamsCopy.decay								= m_recoilparams.decay;
	m_recoilparamsCopy.impulse							= m_recoilparams.impulse;
	m_recoilparamsCopy.max									= m_recoilparams.max;
	m_recoilparamsCopy.max_recoil						= m_recoilparams.max_recoil;
	m_recoilparamsCopy.recoil_crouch_m			= m_recoilparams.recoil_crouch_m;
	m_recoilparamsCopy.recoil_jump_m				= m_recoilparams.recoil_jump_m;
	m_recoilparamsCopy.recoil_prone_m				= m_recoilparams.recoil_prone_m;
	m_recoilparamsCopy.recoil_strMode_m			= m_recoilparams.recoil_strMode_m;
	m_recoilparamsCopy.recoil_zeroG_m				= m_recoilparams.recoil_zeroG_m;
}

//-------------------------------------------------------------------------------
//PatchSpreadMod(SSpreadModParams &sSMP)
//
// - sSMP - Some multipliers to modify the spread per value

void CSingle::PatchSpreadMod(SSpreadModParams &sSMP)
{
	m_spreadparams.attack			*= sSMP.attack_mod;
	m_spreadparams.decay			*= sSMP.decay_mod;
	m_spreadparams.max				*= sSMP.max_mod;
	m_spreadparams.min				*= sSMP.min_mod;
	m_spreadparams.rotation_m *= sSMP.rotation_m_mod;
	m_spreadparams.speed_m    *= sSMP.speed_m_mod;
	
	m_spreadparams.spread_crouch_m *= sSMP.spread_crouch_m_mod;
	m_spreadparams.spread_jump_m   *= sSMP.spread_jump_m_mod;
	m_spreadparams.spread_prone_m  *= sSMP.spread_prone_m_mod;
	m_spreadparams.spread_zeroG_m	 *= sSMP.spread_zeroG_m_mod;

	//Reset spread
	m_spread = m_spreadparams.min;

}

//-------------------------------------------------------------------------------
//ResetSpreadMod()
//
// Restore initial values

void CSingle::ResetSpreadMod()
{
	m_spreadparams = m_spreadparamsCopy;

	//Reset min spread too
	m_spread = m_spreadparams.min;
}

//-------------------------------------------------------------------------------
//PatchRecoilMod(SRecoilModParams &sRMP)
//
// - sRMP - Some multipliers to modify the recoil per value

void CSingle::PatchRecoilMod(SRecoilModParams &sRMP)
{
	m_recoilparams.angular_impulse					*= sRMP.angular_impulse_mod;
	m_recoilparams.attack										*= sRMP.attack_mod;
	m_recoilparams.back_impulse							*= sRMP.back_impulse_mod;
	m_recoilparams.decay										*= sRMP.decay_mod;
	m_recoilparams.impulse									*= sRMP.impulse_mod;
	m_recoilparams.max.x										*= sRMP.max_mod.x;
	m_recoilparams.max.y										*= sRMP.max_mod.y;
	m_recoilparams.max_recoil								*= sRMP.max_recoil_mod;
	m_recoilparams.recoil_crouch_m					*= sRMP.recoil_crouch_m_mod;
	m_recoilparams.recoil_jump_m						*= sRMP.recoil_jump_m_mod;
	m_recoilparams.recoil_prone_m						*= sRMP.recoil_prone_m_mod;
	m_recoilparams.recoil_strMode_m					*= sRMP.recoil_strMode_m_mod;
	m_recoilparams.recoil_zeroG_m						*= sRMP.recoil_zeroG_m_mod;
}

//-------------------------------------------------------------------------------
//ResetRecoilMod()
//
// Restore initial values

void CSingle::ResetRecoilMod()
{
	m_recoilparams.angular_impulse					= m_recoilparamsCopy.angular_impulse;
	m_recoilparams.attack										= m_recoilparamsCopy.attack;
	m_recoilparams.back_impulse							= m_recoilparamsCopy.back_impulse;
	m_recoilparams.decay										= m_recoilparamsCopy.decay;
	m_recoilparams.impulse									= m_recoilparamsCopy.impulse;
	m_recoilparams.max											= m_recoilparamsCopy.max;
	m_recoilparams.max_recoil								= m_recoilparamsCopy.max_recoil;
	m_recoilparams.recoil_crouch_m					= m_recoilparamsCopy.recoil_crouch_m;
	m_recoilparams.recoil_jump_m						= m_recoilparamsCopy.recoil_jump_m;
	m_recoilparams.recoil_prone_m						= m_recoilparamsCopy.recoil_prone_m;
	m_recoilparams.recoil_strMode_m					= m_recoilparamsCopy.recoil_strMode_m;
	m_recoilparams.recoil_zeroG_m						= m_recoilparamsCopy.recoil_zeroG_m;

}

//----------------------------------------------------------------------------------
void CSingle::AutoFire()
{
	if(!m_pWeapon->IsDualWield())
		Shoot(true);
	else
	{
		if(m_pWeapon->IsDualWieldMaster())
		{
			IItem *slave = m_pWeapon->GetDualWieldSlave();
			if(slave && slave->GetIWeapon())
			{
				CWeapon* dualWieldSlave = static_cast<CWeapon*>(slave);
				{
					if(!dualWieldSlave->IsWeaponRaised())
					{
						m_pWeapon->StopFire(m_pWeapon->GetOwnerId());
						m_pWeapon->SetFireAlternation(!m_pWeapon->GetFireAlternation());
						dualWieldSlave->StartFire(m_pWeapon->GetOwnerId());
					}
					else
						Shoot(true);
				}
			}
		}
		else if(m_pWeapon->IsDualWieldSlave())
		{
			IItem *master = m_pWeapon->GetDualWieldMaster();
			if(master && master->GetIWeapon())
			{
				CWeapon* dualWieldMaster = static_cast<CWeapon*>(master);
				{
					if(!dualWieldMaster->IsWeaponRaised())
					{
						m_pWeapon->StopFire(m_pWeapon->GetOwnerId());
						dualWieldMaster->SetFireAlternation(!dualWieldMaster->GetFireAlternation());
						dualWieldMaster->StartFire(dualWieldMaster->GetOwnerId());
					}
					else
						Shoot(true);
				}
			}
		}
	}
}
