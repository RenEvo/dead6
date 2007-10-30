////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_BaseManager.cpp
//
// Purpose: Script binding for the base manager
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptBind_BaseManager.h"
#include "IBaseManager.h"

////////////////////////////////////////////////////
CScriptBind_BaseManager::CScriptBind_BaseManager(ISystem *pSystem)
{
	assert(pSystem);
	m_pSystem = pSystem;
	m_pSS = pSystem->GetIScriptSystem();
	m_pBaseManager = NULL;

	// Initial init
	Init(m_pSS, m_pSystem);
	SetGlobalName("Base");
	RegisterMethods();
	RegisterGlobals();
}

////////////////////////////////////////////////////
CScriptBind_BaseManager::~CScriptBind_BaseManager(void)
{

}

////////////////////////////////////////////////////
void CScriptBind_BaseManager::AttachTo(IBaseManager *pBaseManager)
{
	m_pBaseManager = pBaseManager;
}

////////////////////////////////////////////////////
void CScriptBind_BaseManager::RegisterGlobals(void)
{
	m_pSS->SetGlobalValue("GUID_INVALID", GUID_INVALID);
	m_pSS->SetGlobalValue("BC_INVALID", BC_INVALID);
}

////////////////////////////////////////////////////
void CScriptBind_BaseManager::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_BaseManager::

	SCRIPT_REG_TEMPLFUNC(FindBuilding, "szTeam, szClass");
	SCRIPT_REG_TEMPLFUNC(SetBasePower, "nTeamID, bState");
}

////////////////////////////////////////////////////
int CScriptBind_BaseManager::FindBuilding(IFunctionHandler *pH, char const* szTeam, char const* szClass)
{
	assert(m_pBaseManager);

	// Calculate the GUID
	BuildingGUID GUID = m_pBaseManager->GenerateGUID(szTeam, szClass);
	if (GUID_INVALID == GUID) return pH->EndFunctionNull();

	// Get the controller and return its script table
	IBuildingController *pController = m_pBaseManager->FindBuildingController(GUID);
	if (NULL == pController) return pH->EndFunctionNull();
	return pH->EndFunction(pController->GetScriptTable());
}

////////////////////////////////////////////////////
int CScriptBind_BaseManager::SetBasePower(IFunctionHandler *pH, int nTeamID, bool bState)
{
	assert(m_pBaseManager);
	m_pBaseManager->SetBasePower(nTeamID, bState);
	return pH->EndFunctionNull();
}