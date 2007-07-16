/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 20:04:2006   13:02 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "VehicleWeapon.h"
#include "Actor.h"
#include <IViewSystem.h>
#include "CryAction.h"
#include <IVehicleSystem.h>
#include "Single.h"
#include "HUD/HUD.h"
#include "HUD/HUDVehicleInterface.h"

//------------------------------------------------------------------------
CVehicleWeapon::CVehicleWeapon()
: m_pVehicle(0)
, m_pPart(0)
, m_timeToUpdate(0.0f)
, m_dtWaterLevelCheck(0.f)
, m_pOwnerSeat(NULL)
, m_pSeatUser(NULL)
{  
}

//------------------------------------------------------------------------
bool CVehicleWeapon::Init( IGameObject * pGameObject )
{
  if (!CWeapon::Init(pGameObject))
    return false;

	m_properties.mounted=true;

  return true;
}

//------------------------------------------------------------------------
void CVehicleWeapon::PostInit( IGameObject * pGameObject )
{
  CWeapon::PostInit(pGameObject); 
}

//------------------------------------------------------------------------
void CVehicleWeapon::Reset()
{
  CWeapon::Reset();
}

//------------------------------------------------------------------------
void CVehicleWeapon::MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles)
{
  CWeapon::MountAtEntity(entityId, pos, angles);  
}

//------------------------------------------------------------------------
void CVehicleWeapon::StartUse(EntityId userId)
{
	if (m_ownerId && userId != m_ownerId)
		return; 

  if (GetEntity()->GetParent())
  { 
    m_pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(GetEntity()->GetParent()->GetId());
    assert(m_pVehicle && "Using VehicleWeapons on non-vehicles may lead to unexpected behavior.");

    if (m_pVehicle)
    {   
      m_pPart = m_pVehicle->GetWeaponParentPart(GetEntityId()); 
      m_pOwnerSeat = m_pVehicle->GetWeaponParentSeat(GetEntityId());
      m_pSeatUser = m_pVehicle->GetSeatForPassenger(userId);
    }
  }
  
	ScriptHandle scriptHandle(userId);
	CallScriptEvent(eISET_All, "OnStartUse", scriptHandle, 0, 0, 0);
	
	SetOwnerId(userId);
  Select(true);	
	m_stats.used = true;

	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

  if (OutOfAmmo(false))
    Reload(false);
  
  UseManualBlending(true);

	LowerWeapon(false);

}

//------------------------------------------------------------------------
void CVehicleWeapon::StopUse(EntityId userId)
{
	if (m_ownerId && userId != m_ownerId)
		return;

	ScriptHandle scriptHandle(userId);
	CallScriptEvent(eISET_All, "OnStopUse", scriptHandle, 0, 0, 0);
  
  Select(false);  	
  m_stats.used = false;	

  UseManualBlending(false);

  SetOwnerId(0);

  EnableUpdate(false);

	if(IsZoomed() || IsZooming())
	{
		ExitZoom();
		ExitViewmodes();
	}

	LowerWeapon(false);

}

//------------------------------------------------------------------------
void CVehicleWeapon::StartFire(EntityId actorId)
{
  if (!CheckWaterLevel())
    return;

  CWeapon::StartFire(actorId);
}

//------------------------------------------------------------------------
void CVehicleWeapon::Update( SEntityUpdateContext& ctx, int update)
{
  CWeapon::Update(ctx, update);

	if(update==eIUS_General)
  { 
    if (m_fm && m_fm->IsFiring())
    {
      m_dtWaterLevelCheck -= ctx.fFrameTime;      
      
      if (m_dtWaterLevelCheck <= 0.f)
      { 
        if (!CheckWaterLevel())        
          StopFire(GetOwnerId());          
        
        m_dtWaterLevelCheck = 2.0f;
      }
    }
    
		CheckForFriendlyAI(ctx.fFrameTime);
  }
}

//------------------------------------------------------------------------
bool CVehicleWeapon::CheckWaterLevel() const
{
  // if not submerged at all, skip water level check
  if (m_pVehicle && m_pVehicle->GetStatus().submergedRatio < 0.01f)
    return true;
  
  if (gEnv->p3DEngine->IsUnderWater(GetEntity()->GetWorldPos()))
    return false;

  return true;
}

//------------------------------------------------------------------------
void CVehicleWeapon::SetAmmoCount(IEntityClass* pAmmoType, int count)
{ 
  IActor* pOwner = GetOwnerActor();
  
  if (pOwner && !pOwner->IsPlayer() && count < m_ammo[pAmmoType])
    return;
  
  CWeapon::SetAmmoCount(pAmmoType, count);    
}

//------------------------------------------------------------------------
void CVehicleWeapon::SetInventoryAmmoCount(IEntityClass* pAmmoType, int count)
{
  IActor* pOwner = GetOwnerActor();

  if (pOwner && !pOwner->IsPlayer() && m_pVehicle)
  {
    if (count < m_pVehicle->GetAmmoCount(pAmmoType))
      return;
  }

  CWeapon::SetInventoryAmmoCount(pAmmoType, count);
}

//------------------------------------------------------------------------
bool CVehicleWeapon::FilterView(SViewParams& viewParams)
{ 
  if (m_pOwnerSeat != m_pSeatUser)
    return false;

  if (m_camerastats.animating && !m_camerastats.helper.empty())
  {
    viewParams.position = GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper, true);
    viewParams.rotation = Quat(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper, true));    
    viewParams.blend = false;
        
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
bool CVehicleWeapon::GetAimBlending(OldBlendSpace& params)
{   
  float anglemin=0.f, anglemax=0.f;
  if (m_pPart && m_pPart->GetRotationLimits(0, anglemin, anglemax))
  { 
    if (!(anglemin == 0.f && anglemax == 0.f)) // no limits
    {
      Ang3 angles( m_pPart->GetLocalTM(true) );

      float limit = isneg(angles.x) ? anglemin : anglemax;
      float ratio = (limit != 0.f) ? min(1.f, angles.x/limit) : 0.f;

      params.m_turn = sgn(angles.x) * ratio;
      
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
void CVehicleWeapon::UpdateIKMounted(IActor* pActor)
{
  // only apply IK when the weapons user is in the weapons owner seat
  if (m_pSeatUser == m_pOwnerSeat)
    CWeapon::UpdateIKMounted(pActor);
}

//------------------------------------------------------------------------
void CVehicleWeapon::AttachArms(bool attach, bool shadow)
{
  if (attach && m_pSeatUser != m_pOwnerSeat)
    return;

  CWeapon::AttachArms(attach, shadow);
}

//---------------------------------------------------------------------------
void CVehicleWeapon::CheckForFriendlyAI(float frameTime)
{
	CActor* pOwner = GetOwnerActor();

	if(pOwner && pOwner->IsPlayer() && !gEnv->bMultiplayer)
	{
		m_timeToUpdate-=frameTime;
		if(m_timeToUpdate>0.0f)
			return;

		m_timeToUpdate = 0.15f;

		if(IMovementController* pMC = pOwner->GetMovementController())
		{
			SMovementState info;
			pMC->GetMovementState(info);

			LowerWeapon(false);

			ray_hit rayhit;
			IPhysicalEntity* pSkipEnts[10];
			int nSkip = CSingle::GetSkipEntities(this, pSkipEnts, 10);	

			int intersect = gEnv->pPhysicalWorld->RayWorldIntersection(info.weaponPosition, info.aimDirection * WEAPON_HIT_RANGE, ent_all,
				rwi_stop_at_pierceable|rwi_colltype_any, &rayhit, 1, pSkipEnts, nSkip);

			IEntity* pLookAtEntity = NULL;

			if(intersect && rayhit.pCollider)
				pLookAtEntity = m_pEntitySystem->GetEntityFromPhysics(rayhit.pCollider);

      CHUD* pHUD = g_pGame->GetHUD();
      if (pHUD)
        pHUD->GetVehicleInterface()->DisplayFriendlyFire(false); 

			if(pLookAtEntity && pLookAtEntity->GetAI() && pOwner->GetEntity() && pLookAtEntity->GetId()!=GetEntityId())
			{
				if(!pLookAtEntity->GetAI()->IsHostile(pOwner->GetEntity()->GetAI(),false))
				{
					LowerWeapon(true);
					StopFire(pOwner->GetEntityId());//Just in case
					
          if (pHUD)
            pHUD->GetVehicleInterface()->DisplayFriendlyFire(true);
				}	
			}
		}
	}		
}
