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
		s->Add(itI->first);
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

	// Initial reset
	Reset();
}

////////////////////////////////////////////////////
void CBaseManager::Shutdown(void)
{
	// Final reset
	Reset();
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
void CBaseManager::ResetControllers(void)
{
	CryLog("[BaseManager] Resetting controllers...");
	for (ControllerList::iterator itBuilding = m_ControllerList.begin();
		itBuilding != m_ControllerList.end(); itBuilding++)
	{
		itBuilding->second->Reset();
	}
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
	ClassRepository::iterator itI = m_ClassNameList.find(szName);
	if (itI == m_ClassNameList.end())
	{
		// Add it
		m_ClassNameList[szName] = ++m_nClassNameIDGen;
		CryLog("[BaseManager] Added building class \'%s\' to the list (%u)", szName, m_ClassNameList[szName]);
	}
	BuildingClassID nClassID = m_ClassNameList[szName];

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
	m_ControllerList[GUID] = pController;
	return GUID;
}