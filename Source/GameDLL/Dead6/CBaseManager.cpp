////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CBaseManager.cpp
//
// Purpose: Monitors building logic on the field
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CBaseManager.h"
#include "CBuildingController.h"

////////////////////////////////////////////////////
CBaseManager::CBaseManager(void)
{
	m_nClassNameIDGen = 0;
	m_pSink = NULL;
}

////////////////////////////////////////////////////
CBaseManager::~CBaseManager(void)
{
	Shutdown();
}

////////////////////////////////////////////////////
void CBaseManager::GetMemoryStatistics(ICrySizer *s)
{
	// Class name list
	s->AddContainer(m_ClassNameList);
	for (ClassRepository::iterator itI = m_ClassNameList.begin(); itI != m_ClassNameList.end(); itI++)
	{
		// String objects
		s->Add(itI->second);
	}

	// Controller list
	s->AddContainer(m_ControllerList);
	for (ControllerList::iterator itBuilding = m_ControllerList.begin();
		itBuilding != m_ControllerList.end(); itBuilding++)
	{
		itBuilding->second->GetMemoryStatistics(s);
	}
}

////////////////////////////////////////////////////
void CBaseManager::Initialize(void)
{
	CryLogAlways("Initializing Dead6 Core: CBaseManager...");

	// Create the sink
	m_pSink = new CBaseManagerEntitySink(this);
	gEnv->pEntitySystem->AddSink(m_pSink);

	// Initial reset
	Reset();
}

////////////////////////////////////////////////////
void CBaseManager::Shutdown(void)
{
	// Final reset
	Reset();

	// Destroy the sink
	if (NULL != m_pSink)
	{
		gEnv->pEntitySystem->RemoveSink(m_pSink);
		delete m_pSink;
		m_pSink = NULL;
	}
}

////////////////////////////////////////////////////
void CBaseManager::Reset(void)
{
	CryLog("[BaseManager] Reset");

	// Clear class name repository
	m_ClassNameList.clear();
	m_nClassNameIDGen = BC_INVALID;

	// Destroy controllers
	for (ControllerList::iterator itBuilding = m_ControllerList.begin();
		itBuilding != m_ControllerList.end(); itBuilding++)
	{
		itBuilding->second->Shutdown();
		delete itBuilding->second;
	}
	m_ControllerList.clear();
}

////////////////////////////////////////////////////
BuildingGUID CBaseManager::CreateBuildingController(char const* szTeam, char const* szName,
													XmlNodeRef pAttr, IBuildingController **ppController)
{
	if (NULL != ppController) *ppController = NULL;
	assert(g_D6Core->pTeamManager);

	// Get team ID first
	TeamID nTeamID;
	if (NULL == szTeam || NULL == g_D6Core->pTeamManager || 
		TEAMID_INVALID == (nTeamID = g_D6Core->pTeamManager->GetTeamId(szTeam)))
		return GUID_INVALID;

	// Check building class repository for class name
	ClassRepository::iterator itI = m_ClassNameList.begin();
	BuildingClassID nClassID;
	for (itI; itI != m_ClassNameList.end(); itI++)
	{
		if (itI->second == szName)
		{
			nClassID = itI->first;
			break;
		}
	}
	if (itI == m_ClassNameList.end())
	{
		// Add it
		nClassID = ++m_nClassNameIDGen;
		m_ClassNameList[nClassID] = szName;
		CryLog("[BaseManager] Added building class \'%s\' to the list (%u)", szName, nClassID);
	}

	// Construct GUID and check if it already exists
	BuildingGUID GUID = MAKE_BUILDING_GUID(nTeamID, nClassID);
	ControllerList::iterator itBuilding = m_ControllerList.find(GUID);
	if (itBuilding != m_ControllerList.end())
	{
		// Return its ID
		return itBuilding->first;
	}

	// Get rest of its attributes
	float fInitHealth = 99999.0f; // Note: Clamp will put it down to max health based on building's definition
	if (NULL != pAttr)
	{
		pAttr->getAttr("InitHealth", fInitHealth);
	}

	// Create a new building
	IBuildingController *pController = new CBuildingController;
	m_ControllerList[GUID] = pController;
	pController->Initialize(GUID, fInitHealth);
	if (false == pController->LoadFromXml(szName) && NULL != g_D6Core->pSystem)
	{
		// Error it
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR,
			VALIDATOR_FLAG_FILE, "", "Missing/Corrupted Building Definition file for class : %s", szName);
	}
	if (NULL != ppController) *ppController = pController;

	CryLog("[BaseManager] Created building controller \'%s\' for team \'%s\' (GUID = %u)",
		szName, szTeam, GUID);
	return GUID;
}

////////////////////////////////////////////////////
IBuildingController *CBaseManager::FindBuildingController(BuildingGUID nGUID) const
{
	ControllerList::const_iterator itBuilding = m_ControllerList.find(nGUID);
	if (itBuilding == m_ControllerList.end())
		return NULL;
	return itBuilding->second;
}

////////////////////////////////////////////////////
void CBaseManager::ResetControllers(void)
{
	CryLog("[BaseManager] Resetting controllers...");
	
	// Validate them first
	Validate();

	// Call reset on all of them
	for (ControllerList::iterator itBuilding = m_ControllerList.begin();
		itBuilding != m_ControllerList.end(); itBuilding++)
	{
		itBuilding->second->Reset();
	}
}

////////////////////////////////////////////////////
void CBaseManager::Validate(BuildingGUID nGUID)
{
	ControllerList::iterator itBuilding = m_ControllerList.begin();
	if (GUID_INVALID != nGUID)
	{
		// Find it
		itBuilding = m_ControllerList.find(nGUID);
	}

	// Iterate through list. If we specified a specific, iterator it already pointing
	//	to it
	for (itBuilding; itBuilding != m_ControllerList.end(); itBuilding++)
	{
		// Validate
		itBuilding->second->Validate();

		// If we specified a GUID, break out at end
		if (GUID_INVALID != nGUID) break;
	}
}

////////////////////////////////////////////////////
const char *CBaseManager::GetClassName(BuildingClassID nClassID) const
{
	// Find it
	ClassRepository::const_iterator itEntry = m_ClassNameList.find(nClassID);
	if (itEntry == m_ClassNameList.end()) return NULL;

	// Return the name
	return itEntry->second.c_str();
}

////////////////////////////////////////////////////
BuildingClassID CBaseManager::GetClassId(char const* szName) const
{
	// Look through all teams for the name
	for (ClassRepository::const_iterator itEntry = m_ClassNameList.begin(); itEntry != m_ClassNameList.end();
		itEntry++)
	{
		if (0 == stricmp(itEntry->second.c_str(), szName))
		{
			return itEntry->first;
		}
	}
	return BC_INVALID;
}

////////////////////////////////////////////////////
bool CBaseManager::IsValidClass(BuildingClassID nID) const
{
	ClassRepository::const_iterator itClass = m_ClassNameList.find(nID);
	return (itClass != m_ClassNameList.end());
}
bool CBaseManager::IsValidClass(char const* szName) const
{
	for (ClassRepository::const_iterator itClass = m_ClassNameList.begin(); itClass != m_ClassNameList.end();
		itClass++)
	{
		if (itClass->second == szName)
			return true;
	}
	return false;
}

////////////////////////////////////////////////////
BuildingGUID CBaseManager::GenerateGUID(char const* szTeam, char const* szClass) const
{
	// Get the team and class IDs
	TeamID nTeamID = TEAMID_INVALID;
	BuildingClassID nClassID = GetClassId(szClass);
	if (NULL != g_D6Core->pTeamManager)
		nTeamID = g_D6Core->pTeamManager->GetTeamId(szTeam);

	// Must be valid
	if (TEAMID_INVALID == nTeamID || BC_INVALID == nClassID)
		return GUID_INVALID;
	return MAKE_BUILDING_GUID(nTeamID, nClassID);
}

////////////////////////////////////////////////////
CBaseManagerEntitySink::CBaseManagerEntitySink(CBaseManager *pManager) :
	m_pManager(pManager)
{

}

////////////////////////////////////////////////////
CBaseManagerEntitySink::~CBaseManagerEntitySink(void)
{

}

////////////////////////////////////////////////////
bool CBaseManagerEntitySink::OnBeforeSpawn(SEntitySpawnParams &params)
{
	return true;
}

////////////////////////////////////////////////////
void CBaseManagerEntitySink::OnSpawn(IEntity *pEntity, SEntitySpawnParams &params)
{
	// Check if it has a CNC Table. If it does, manually add it to the controller
	IScriptTable *pTable;
	if (NULL != pEntity && NULL != (pTable = pEntity->GetScriptTable()))
	{
		// Get property table
		SmartScriptTable props, cncbuilding;
		if (true == pTable->GetValue("Properties", props) &&
			true == props->GetValue("CNCBuilding", cncbuilding))
		{
			// Extract and build GUID
			char const* szTeam = 0;
			char const* szClass = 0;
			cncbuilding->GetValue("Team", szTeam);
			cncbuilding->GetValue("Class", szClass);
			BuildingGUID GUID = m_pManager->GenerateGUID(szTeam, szClass);
			if (GUID_INVALID != GUID)
			{
				// Get controller and manually add it
				IBuildingController *pController = m_pManager->FindBuildingController(GUID);
				if (NULL != pController)
					pController->AddInterface(pEntity);
			}
		}
	}
}

////////////////////////////////////////////////////
bool CBaseManagerEntitySink::OnRemove(IEntity *pEntity)
{
	// Remove it from all controllers
	for (CBaseManager::ControllerList::iterator itBuilding = m_pManager->m_ControllerList.begin();
		itBuilding != m_pManager->m_ControllerList.end(); itBuilding++)
	{
		itBuilding->second->RemoveInterface(pEntity);
	}
	return true;
}

////////////////////////////////////////////////////
void CBaseManagerEntitySink::OnEvent(IEntity *pEntity, SEntityEvent &event)
{
	// TODO Report damage to controller if entity is an interface to it
	//	Use "ENTITY_EVENT_ONHIT"

	//CryLogAlways("Entity %d event: %d", pEntity->GetId(), event.event);
}