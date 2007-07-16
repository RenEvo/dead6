//-----------------------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "HUD.h"
#include "GameFlashAnimation.h"
#include "../Actor.h"
#include "IVehicleSystem.h"
#include "IWorldQuery.h"
#include "Weapon.h"
#include "HUDRadar.h"
#include "HUDScopes.h"

#define HUD_CALL_LISTENERS(func) \
{ \
	if (m_hudListeners.empty() == false) \
	{ \
		m_hudTempListeners = m_hudListeners; \
		for (std::vector<IHUDListener*>::iterator tmpIter = m_hudTempListeners.begin(); tmpIter != m_hudTempListeners.end(); ++tmpIter) \
			(*tmpIter)->func; \
	} \
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::IsAirStrikeAvailable()
{
	return m_bAirStrikeAvailable;
}

void CHUD::SetAirStrikeEnabled(bool p_bEnabled)
{
	m_animAirStrike.Reload();
	SetFlashColor(&m_animAirStrike);

	m_bAirStrikeAvailable = p_bEnabled;
	m_animAirStrike.Invoke("enableAirStrike",p_bEnabled);
	if(!p_bEnabled)
		ClearAirstrikeEntities();
	SetAirStrikeBinoculars(m_pHUDScopes->IsBinocularsShown());
}

void CHUD::AddAirstrikeEntity(EntityId p_iID)
{
	if(!stl::find(m_possibleAirStrikeTargets, p_iID))
		m_possibleAirStrikeTargets.push_back(p_iID);
}

void CHUD::ClearAirstrikeEntities()
{
	std::vector<EntityId>::const_iterator it = m_possibleAirStrikeTargets.begin();
	for(; it != m_possibleAirStrikeTargets.end(); ++it)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
		if(pEntity)
			UnlockTarget(*it);
	}
	m_possibleAirStrikeTargets.clear();
}

void CHUD::NotifyAirstrikeSucceeded(bool p_bSucceeded)
{
	EntityId id = 0;
	UpdateAirStrikeTarget(id);
	SetAirStrikeEnabled(false);
	m_animTargetAutoAim.SetVisible(false);
}

void CHUD::SetAirStrikeBinoculars(bool p_bEnabled)
{
	if(!m_animAirStrike.IsLoaded())
		return;
	m_animAirStrike.Invoke("setBinoculars",p_bEnabled);

	if(p_bEnabled)
	{
		std::vector<EntityId>::const_iterator it = m_possibleAirStrikeTargets.begin();
		for(; it != m_possibleAirStrikeTargets.end(); ++it)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
			if(pEntity)
				LockTarget(*it, eLT_Locked, false, true);
		}
	}
	else
	{
		std::vector<EntityId>::const_iterator it = m_possibleAirStrikeTargets.begin();
		for(; it != m_possibleAirStrikeTargets.end(); ++it)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
			if(pEntity)
				UnlockTarget(*it);
		}
		EntityId id = 0;
		UpdateAirStrikeTarget(id);
	}
}

bool CHUD::StartAirStrike()
{
	if(!m_animAirStrike.IsLoaded())
		return false;

	CCamera camera=GetISystem()->GetViewCamera();

	IActor *pActor=gEnv->pGame->GetIGameFramework()->GetClientActor();
	IPhysicalEntity *pSkipEnt=pActor?pActor->GetEntity()->GetPhysics():0;

	Vec3 dir=(camera.GetViewdir())*500.0f;

	std::vector<EntityId>::const_iterator it = m_possibleAirStrikeTargets.begin();
	for(; it != m_possibleAirStrikeTargets.end(); ++it)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
		if(pEntity)
		{
			IPhysicalEntity *pPE=pEntity->GetPhysics();
			if(pPE)
			{
				ray_hit hit;

				if (gEnv->pPhysicalWorld->RayWorldIntersection(camera.GetPosition(), dir, ent_all, (13&rwi_pierceability_mask), &hit, 1, &pSkipEnt, pSkipEnt?1:0))
				{
					if (!hit.bTerrain && hit.pCollider==pPE)
					{
						IEntity *pHitEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
						if (pHitEntity == pEntity)
						{
							UpdateAirStrikeTarget(pHitEntity->GetId());
							LockTarget(pHitEntity->GetId(),eLT_Locking, false);
							m_animAirStrike.Invoke("startCountdown");
							HUD_CALL_LISTENERS(OnAirstrike(1,pHitEntity->GetId()));
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void CHUD::UpdateAirStrikeTarget(EntityId p_iTarget)
{
	if(!m_animAirStrike.IsLoaded())
		return;
	if(stl::find(m_possibleAirStrikeTargets, p_iTarget))
	{
		if(m_iAirStrikeTarget)
		{
			LockTarget(m_iAirStrikeTarget,eLT_Locked, false);
			LockTarget(p_iTarget,eLT_Locking, false);
		}
		m_iAirStrikeTarget = p_iTarget;
		m_animAirStrike.Invoke("setTarget", (p_iTarget!=0));
	}
	else
	{
		m_iAirStrikeTarget = 0;
	}
}

void CHUD::LockTarget(EntityId p_iTarget, ELockingType p_iType, bool p_bShowText, bool p_bMultiple)
{
	if(!p_bMultiple)
	{
		if(m_entityTargetAutoaimId && p_iTarget!=m_entityTargetAutoaimId)
		{
			UnlockTarget(m_entityTargetAutoaimId);
		}

		m_entityTargetAutoaimId = p_iTarget;
	}

	if(!p_bShowText)
		m_animTargetAutoAim.Invoke("setTargetAutoaimHideText", (int)p_iTarget);

	m_animTargetAutoAim.SetVisible(true);

	if(p_iType == eLT_Locked)
	{
		m_animTargetAutoAim.Invoke("setTargetAutoaimLocked", (int)p_iTarget);
		m_pHUDScopes->m_animSniperScope.Invoke("setLocking",2);
	}
	else if(p_iType == eLT_Locking)
	{
		m_animTargetAutoAim.Invoke("setTargetAutoaimLocking", (int)p_iTarget);
		m_pHUDScopes->m_animSniperScope.Invoke("setLocking",1);
	}
}

void CHUD::UnlockTarget(EntityId p_iTarget)
{
	m_animTargetAutoAim.SetVisible(false);

	if(p_iTarget)
		m_animTargetAutoAim.Invoke("setTargetAutoaimUnlock", (int)p_iTarget);
	else
		m_animTargetAutoAim.Invoke("setTargetAutoaimUnlock", (int)m_entityTargetAutoaimId);

	m_pHUDScopes->m_animSniperScope.Invoke("setLocking",3);
	
	m_entityTargetAutoaimId = 0;
}

