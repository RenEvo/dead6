////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CBuildingController.cpp
//
// Purpose: Building controller which contains
//	a logical structure and is interfaced
//
// File History:
//	- 8/11/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CBuildingController.h"
#include "ScriptBind_BuildingController.h"
#include "Player.h"

// MAX_VIEW_DISTANCE - How far away the player can "see" a building's information
#define MAX_VIEW_DISTANCE	(100.0f)

////////////////////////////////////////////////////
CBuildingController::CBuildingController(void)
{
	m_nGUID = GUID_INVALID;
	m_nFlags = 0;
	m_fHealth = m_fInitHealth = m_fMaxHealth = 0.0f;
	m_pSS = NULL;
}

////////////////////////////////////////////////////
CBuildingController::~CBuildingController(void)
{
	Shutdown();
}

////////////////////////////////////////////////////
void CBuildingController::GetMemoryStatistics(ICrySizer *s)
{
	s->Add(*this);
	s->Add(m_szName);
	s->Add(m_szScript);
}

////////////////////////////////////////////////////
void CBuildingController::Initialize(BuildingGUID nGUID)
{
	m_nGUID = nGUID;

	// Clamp init health to max health
	if (m_fMaxHealth <= 0.0f)
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, VALIDATOR_FLAG_SCRIPT,
			m_szName, "Missing/Bad MaxHealth property for building definition [%s %s], health is set to 0!",
			GetTeamName(), m_szName.c_str());
		m_fMaxHealth = 0.0f;
	}
	m_fInitHealth = CLAMP(m_fInitHealth, 0.0f, m_fMaxHealth);
	m_fHealth = m_fInitHealth;

	// Load script and get table
	m_pSS = g_D6Core->pSystem->GetIScriptSystem();
	ScriptAnyValue temp;
	if (true == m_szScript.empty() || false == m_pSS->ExecuteFile(m_szScript.c_str(), true, false))
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			VALIDATOR_FLAG_FILE, m_szScript.c_str(), "Failed to execute building script for [%s %s]",
			GetTeamName(), m_szName.c_str());
	}
	else if (false == m_pSS->GetGlobalAny(m_szName, temp) ||
		false == temp.CopyTo(m_pScriptTable))
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			VALIDATOR_FLAG_FILE, m_szScript.c_str(), "Failed to get script table for building [%s %s]",
			GetTeamName(), m_szName.c_str());
	}
	else
	{
		// Attach it to the building controller script bind
		if (NULL != g_D6Core->pD6Game)
		{
			g_D6Core->pD6Game->GetBuildingControllerScriptBind()->AttachTo(this);
		}

		// Call OnInit
		BEGIN_CALL_SERVER(m_pSS, m_pScriptTable, "OnInit")
		END_CALL(m_pSS)
		BEGIN_CALL_CLIENT(m_pSS, m_pScriptTable, "OnInit")
		END_CALL(m_pSS)
	}
}

////////////////////////////////////////////////////
void CBuildingController::Shutdown(void)
{
	// Final reset
	Reset();

	// Call OnShutdown before releasing
	BEGIN_CALL_SERVER(m_pSS, m_pScriptTable, "OnShutdown")
	END_CALL(m_pSS)
	BEGIN_CALL_CLIENT(m_pSS, m_pScriptTable, "OnShutdown")
	END_CALL(m_pSS)

	if (NULL != m_pSS)
	{
		m_pSS->UnloadScript(m_szScript);
		m_pSS = NULL;
	}

	// Reset interface list and inform all interfaces they no longer represent me
	for (InterfaceMap::iterator itI = m_Interfaces.begin(); itI != m_Interfaces.end(); itI++)
	{
		(*itI)->ClearFlags(ENTITY_FLAG_ISINTERFACE);
	}
	m_Interfaces.clear();
}

////////////////////////////////////////////////////
void CBuildingController::Update(bool bHaveFocus, unsigned int nUpdateFlags, unsigned int nControllerUpdateFlags)
{
	// Update visibility
	EntityId nViewID = 0;
	bool bPrevInView = (CSF_ISVISIBLE == (m_nFlags&CSF_ISVISIBLE));
	if (CUF_CHECKVISIBILITY == (nControllerUpdateFlags&CUF_CHECKVISIBILITY))
	{
		// Get player's orientation
		CPlayer *pPlayer = (CPlayer*)g_pGame->GetIGameFramework()->GetClientActor();
		CCamera PlayerCam = g_D6Core->pSystem->GetViewCamera();
		Matrix34 PlayerMat = PlayerCam.GetMatrix();
		Vec3 vPlayerPos(PlayerCam.GetPosition());
		Vec3 vPlayerForward(PlayerMat.m01,PlayerMat.m11,PlayerMat.m21);
		vPlayerForward *= MAX_VIEW_DISTANCE;

		// Perform hit
		ray_hit hit;
		IPhysicalEntity *pSkip[1] = {pPlayer->GetEntity()->GetPhysics()};
		if (gEnv->pPhysicalWorld->RayWorldIntersection(vPlayerPos, vPlayerForward, (ent_terrain|ent_static), 
				(rwi_stop_at_pierceable|rwi_colltype_any), &hit, 1, (NULL==pPlayer?NULL:pSkip), (NULL==pPlayer?0:1)) &&
			NULL != hit.pCollider)
		{
			// Check the interfaces to see if it is being looked at by the player
			IEntity *pHitEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
			for (InterfaceMap::iterator itI = m_Interfaces.begin(); pHitEntity && itI != m_Interfaces.end(); itI++)
			{
				if ((*itI)->GetId() == pHitEntity->GetId())
				{
					// Got a hit!
					nViewID = (*itI)->GetId();
					break;
				}
			}
		}
	}
	if (true == bPrevInView && 0 == nViewID)
	{
		m_nFlags &= ~CSF_ISVISIBLE;
		CryLogAlways("No longer looking at building [%s %s]", GetTeamName(), GetClassName());

		SControllerEvent event(CONTROLLER_EVENT_OUTOFVIEW);
		_SendListenerEvent(event);
	}
	if (false == bPrevInView && 0 != nViewID)
	{
		m_nFlags |= CSF_ISVISIBLE;
		CryLogAlways("Looking at building [%s %s]", GetTeamName(), GetClassName());

		SControllerEvent event(CONTROLLER_EVENT_INVIEW);
		event.nParam[0] = nViewID;
		_SendListenerEvent(event);
	}

	// Handle damage queue
	if (CUF_PARSEEXPLOSIONQUEUE == (nControllerUpdateFlags&CUF_PARSEEXPLOSIONQUEUE))
	{
		// Apply what is in it
		for (ExplosionQueue::iterator itDamage = m_ExplosionQueue.begin(); itDamage != m_ExplosionQueue.end(); itDamage++)
		{
			// TODO Adjust health
			CryLogAlways("[%s %s] Damaged by explosion: %.4f", GetTeamName(), GetClassName(), itDamage->second.damage);

			SControllerEvent event(CONTROLLER_EVENT_ONEXPLOSION);
			event.nParam[0] = (INT_PTR)&(itDamage->second);
			_SendListenerEvent(event);
		}
		m_ExplosionQueue.clear();
	}
}

////////////////////////////////////////////////////
void CBuildingController::Reset(void)
{
	// Reset to initial state
	m_fHealth = m_fInitHealth;

	// Reset flags
	m_nFlags &= ~CSF_ISVISIBLE;

	// Call OnReset
	BEGIN_CALL_SERVER(m_pSS, m_pScriptTable, "OnReset")
	END_CALL(m_pSS)
	BEGIN_CALL_CLIENT(m_pSS, m_pScriptTable, "OnReset")
	END_CALL(m_pSS)

	SControllerEvent event(CONTROLLER_EVENT_RESET);
	_SendListenerEvent(event);
}

////////////////////////////////////////////////////
bool CBuildingController::BeforeValidate(void)
{
	// Remove validation flag
	m_nFlags &= ~CSF_ISVALIDATED;

	// Reset interface list and inform all interfaces they no longer represent me
	for (InterfaceMap::iterator itI = m_Interfaces.begin(); itI != m_Interfaces.end(); itI++)
	{
		(*itI)->ClearFlags(ENTITY_FLAG_ISINTERFACE);
	}
	m_Interfaces.clear();

	return true;
}

////////////////////////////////////////////////////
void CBuildingController::Validate(void)
{
	bool bWantsToBeDestroyed = (CSF_WANTSTOBEDESTROYED == (m_nFlags&CSF_WANTSTOBEDESTROYED));
	// If no interfaces are found, warning and turn off bMustBeDestroyed
	if (true == m_Interfaces.empty())
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			0, 0, "Building \'%s\' \'%s\' (GUID = %u) has no interfaces! MustBeDestroyed forced to FALSE.",
			GetTeamName(), GetClassName(), m_nGUID);

		// Set bMustBeDestroyed to FALSE
		bWantsToBeDestroyed = false;
	}

	// Set must be destroyed flag
	if (true == bWantsToBeDestroyed)
		m_nFlags |= CSF_MUSTBEDESTROYED;
	else
		m_nFlags &= ~CSF_MUSTBEDESTROYED;

	// Validated
	m_nFlags |= CSF_ISVALIDATED;
	SControllerEvent event(CONTROLLER_EVENT_VALIDATED);
	_SendListenerEvent(event);
}

////////////////////////////////////////////////////
bool CBuildingController::IsValidated(void) const
{
	return (CSF_ISVALIDATED == (m_nFlags&CSF_ISVALIDATED));
}

////////////////////////////////////////////////////
void CBuildingController::LoadFromXml(XmlNodeRef pNode)
{
	assert(g_D6Core->pSystem);
	if (NULL == pNode) return;

	// Will always be the same
	m_szName = pNode->getAttr("Name");

	// Basic info
	if (true == pNode->haveAttr("Script"))
	{
		m_szScript = D6C_PATH_BUILDINGS;
		m_szScript += pNode->getAttr("Script");
	}

	// Health info
	if (true == pNode->haveAttr("InitHealth"))
		pNode->getAttr("InitHealth", m_fInitHealth);
	if (true == pNode->haveAttr("MaxHealth"))
		pNode->getAttr("MaxHealth", m_fMaxHealth);

	// Flags
	if (true == pNode->haveAttr("MustBeDestroyed"))
	{
		bool bMustBeDestroyed = false;
		pNode->getAttr("MustBeDestroyed", bMustBeDestroyed);
		SetMustBeDestroyed(bMustBeDestroyed);
	}
}

////////////////////////////////////////////////////
BuildingGUID CBuildingController::GetGUID(void) const
{
	return m_nGUID;
}

////////////////////////////////////////////////////
IScriptTable *CBuildingController::GetScriptTable(void) const
{
	return m_pScriptTable;
}

////////////////////////////////////////////////////
TeamID CBuildingController::GetTeam(void) const
{
	TeamID nID = GET_TEAM_FROM_GUID(m_nGUID);
	if (NULL == g_D6Core->pTeamManager || false == g_D6Core->pTeamManager->IsValidTeam(nID))
		return TEAMID_NOTEAM;
	return nID;
}

////////////////////////////////////////////////////
char const* CBuildingController::GetTeamName(void) const
{
	TeamID nID = GET_TEAM_FROM_GUID(m_nGUID);
	if (NULL == g_D6Core->pTeamManager || false == g_D6Core->pTeamManager->IsValidTeam(nID))
		return TEAMID_NOTEAM;
	return g_D6Core->pTeamManager->GetTeamName(nID);
}

////////////////////////////////////////////////////
BuildingClassID CBuildingController::GetClass(void) const
{
	BuildingClassID nID = GET_CLASS_FROM_GUID(m_nGUID);
	if (NULL == g_D6Core->pBaseManager || false == g_D6Core->pBaseManager->IsValidClass(nID))
		return BC_INVALID;
	return nID;
}

////////////////////////////////////////////////////
char const* CBuildingController::GetClassName(void) const
{
	BuildingClassID nID = GET_CLASS_FROM_GUID(m_nGUID);
	if (NULL == g_D6Core->pBaseManager || false == g_D6Core->pBaseManager->IsValidClass(nID))
		return BC_INVALID;
	return g_D6Core->pBaseManager->GetClassName(nID);
}

////////////////////////////////////////////////////
bool CBuildingController::AddInterface(IEntity *pEntity)
{
	if (NULL == pEntity) return false;

	// Check to see if it already is an interface
	if (true == pEntity->CheckFlags(ENTITY_FLAG_ISINTERFACE))
		return false;

	// Check to see if it isn't already in the list
	for (InterfaceMap::iterator itI = m_Interfaces.begin(); itI != m_Interfaces.end(); itI++)
	{
		if ((*itI)->GetId() == pEntity->GetId())
			return true;
	}
	
	// Set the flag and add it
	pEntity->AddFlags(ENTITY_FLAG_ISINTERFACE);
	m_Interfaces.push_back(pEntity);
	CryLog("[%s %s] Added interface %d", GetTeamName(), GetClassName(), pEntity->GetId());
	return true;
}

////////////////////////////////////////////////////
void CBuildingController::RemoveInterface(IEntity *pEntity)
{
	if (NULL == pEntity) return;

	// Find it
	for (InterfaceMap::iterator itI = m_Interfaces.begin(); itI != m_Interfaces.end(); itI++)
	{
		if ((*itI)->GetId() == pEntity->GetId())
		{
			pEntity->ClearFlags(ENTITY_FLAG_ISINTERFACE);
			m_Interfaces.erase(itI);
			return;
		}
	}
}

////////////////////////////////////////////////////
bool CBuildingController::HasInterface(IEntity *pEntity) const
{
	// Find it
	for (InterfaceMap::const_iterator itI = m_Interfaces.begin(); pEntity && itI != m_Interfaces.end(); itI++)
	{
		if ((*itI)->GetId() == pEntity->GetId())
			return true;
	}
	return false;
}

////////////////////////////////////////////////////
bool CBuildingController::HasInterface(EntityId nEntityId) const
{
	// Get entity
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(nEntityId);
	return HasInterface(pEntity);
}

////////////////////////////////////////////////////
bool CBuildingController::IsVisible(void) const
{
	return (CSF_ISVISIBLE == (m_nFlags&CSF_ISVISIBLE));
}

////////////////////////////////////////////////////
void CBuildingController::SetMustBeDestroyed(bool b)
{
	// Set it
	if (true == b)
		m_nFlags |= CSF_WANTSTOBEDESTROYED;
	else
		m_nFlags &= ~CSF_WANTSTOBEDESTROYED;
}

////////////////////////////////////////////////////
bool CBuildingController::MustBeDestroyed(void) const
{
	return (CSF_MUSTBEDESTROYED == (m_nFlags&CSF_MUSTBEDESTROYED));
}

////////////////////////////////////////////////////
void CBuildingController::AddEventListener(IBuildingControllerEventListener *pListener)
{
	// If already added, drop it
	for (EventListeners::iterator itListener = m_EventListeners.begin(); itListener != m_EventListeners.end(); itListener++)
	{
		if (*itListener == pListener) return;
	}
	m_EventListeners.push_back(pListener);
}

////////////////////////////////////////////////////
void CBuildingController::RemoveEventListener(IBuildingControllerEventListener *pListener)
{
	// Find and remove
	for (EventListeners::iterator itListener = m_EventListeners.begin(); itListener != m_EventListeners.end(); itListener++)
	{
		if (*itListener == pListener)
		{
			m_EventListeners.erase(itListener);
			return;
		}
	}
}

////////////////////////////////////////////////////
void CBuildingController::_SendListenerEvent(SControllerEvent &event)
{
	// Send it out
	for (EventListeners::iterator itListener = m_EventListeners.begin(); itListener != m_EventListeners.end(); itListener++)
	{
		(*itListener)->OnBuildingControllerEvent(this, m_nGUID, event);
	}
}

////////////////////////////////////////////////////
void CBuildingController::OnClientHit(HitInfo const& hitInfo)
{
	// TODO Anything local on the client

	if (!gEnv->bServer)
	{
		SControllerEvent event(CONTROLLER_EVENT_ONHIT);
		event.nParam[0] = (INT_PTR)&hitInfo;
		_SendListenerEvent(event);
	}
}

////////////////////////////////////////////////////
void CBuildingController::OnServerHit(HitInfo const& hitInfo)
{
	// TODO Update building health
	CryLogAlways("[%s %s] Damaged by weapon: %.4f", GetTeamName(), GetClassName(), hitInfo.damage);

	SControllerEvent event(CONTROLLER_EVENT_ONHIT);
	event.nParam[0] = (INT_PTR)&hitInfo;
	_SendListenerEvent(event);
}

////////////////////////////////////////////////////
void CBuildingController::OnClientExplosion(ExplosionInfo const& explosionInfo, EntityId nInterfaceId, float fObstruction)
{
	// TODO Anything local on the client
}

////////////////////////////////////////////////////
void CBuildingController::OnServerExplosion(ExplosionInfo const& explosionInfo, EntityId nInterfaceId, float fObstruction)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(nInterfaceId);
	if (NULL == pEntity) return;

	float fDamage = 0.0f;

	// Calculate distance to closest point on AABB from explosion's center
	const Vec3& vExplosionCenter = explosionInfo.pos;
	AABB aabb; pEntity->GetWorldBounds(aabb);
	float fDistanceSq = 0.0f;
#define ADD_COMP(c) \
	if (vExplosionCenter.c < aabb.min.c) fDistanceSq += (aabb.min.c - vExplosionCenter.c) * (aabb.min.c - vExplosionCenter.c); \
	if (vExplosionCenter.c > aabb.max.c) fDistanceSq += (vExplosionCenter.c - aabb.max.c) * (vExplosionCenter.c - aabb.max.c);
	ADD_COMP(x);
	ADD_COMP(y);
	ADD_COMP(z);
#undef ADD_COMP

	// Calculate ratio between min to max radius, and distance
	const float fMinRadiusSq = explosionInfo.minRadius * explosionInfo.minRadius;
	const float fMaxRadiusSq = explosionInfo.radius * explosionInfo.radius;
	if (fDistanceSq <= fMinRadiusSq) fDamage = explosionInfo.damage;
	else if (fDistanceSq > fMaxRadiusSq) fDamage = 0.0f;
	else
	{
		// Calculate ratio
		fDamage = explosionInfo.damage * (1.0f-((fDistanceSq-fMinRadiusSq)/(fMaxRadiusSq-fMinRadiusSq)));
	}
	fDamage *= fObstruction; // Apply obstruction ratio

	// Queue damage if valid
	if (fDamage > 0.0f)
	{
		// Update damage entry
		ExplosionInfo updatedInfo = explosionInfo;
		updatedInfo.damage = fDamage;

		// If there is already an entry for this weapon, see who did more
		ExplosionQueue::iterator itEntry = m_ExplosionQueue.find(updatedInfo.weaponId);
		if (itEntry == m_ExplosionQueue.end() || fDamage > updatedInfo.damage)
		{
			// Add/replace it
			m_ExplosionQueue[updatedInfo.weaponId] = updatedInfo;
		}
	}
}