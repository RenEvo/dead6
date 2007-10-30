////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_BuildingController.cpp
//
// Purpose: Binding script object for the building
//	controllers
//
// File History:
//	- 8/12/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptBind_BuildingController.h"
#include "IBuildingController.h"

////////////////////////////////////////////////////
CScriptBind_BuildingController::CScriptBind_BuildingController(ISystem *pSystem, IGameFramework *pGameFW)
{
	m_pSystem = pSystem;
	m_pSS = pSystem->GetIScriptSystem();
	m_pGameFW = pGameFW;

	// Initialize it
	Init(m_pSS, m_pSystem, 1);
	RegisterMethods();
	RegisterGlobals();
}

////////////////////////////////////////////////////
CScriptBind_BuildingController::~CScriptBind_BuildingController(void)
{

}

////////////////////////////////////////////////////
void CScriptBind_BuildingController::AttachTo(IBuildingController *pController)
{
	// Delegate methods to it
	IScriptTable *pScriptTable = pController->GetScriptTable();
	if (NULL != pScriptTable)
	{
		SmartScriptTable thisTable(m_pSS);
		thisTable->SetValue("__this", ScriptHandle(pController->GetGUID())); // so pH->GetThis() returns its GUID
		thisTable->Delegate(GetMethodsTable());
		pScriptTable->SetValue("controller", thisTable);
	}
}

////////////////////////////////////////////////////
IBuildingController *CScriptBind_BuildingController::GetController(IFunctionHandler *pH)
{
	// Need base manager to do this
	if (NULL == g_D6Core->pBaseManager) return NULL;

	// Convert this and find it
	void *pThis = pH->GetThis();
	if (NULL == pThis) return NULL;
	return g_D6Core->pBaseManager->FindBuildingController((BuildingGUID)(UINT_PTR)pThis);
}

////////////////////////////////////////////////////
void CScriptBind_BuildingController::RegisterGlobals(void)
{
	// Events
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_VALIDATED",		CONTROLLER_EVENT_VALIDATED);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_RESET",			CONTROLLER_EVENT_RESET);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_ONHIT",			CONTROLLER_EVENT_ONHIT);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_ONEXPLOSION",	CONTROLLER_EVENT_ONEXPLOSION);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_INVIEW",		CONTROLLER_EVENT_INVIEW);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_OUTOFVIEW",		CONTROLLER_EVENT_OUTOFVIEW);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_POWER",			CONTROLLER_EVENT_POWER);
	m_pSS->SetGlobalValue("CONTROLLER_EVENT_DESTROYED",		CONTROLLER_EVENT_DESTROYED);
}

////////////////////////////////////////////////////
void CScriptBind_BuildingController::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_BuildingController::

	// Register functions
	SCRIPT_REG_FUNC(GetHealth);
	SCRIPT_REG_FUNC(IsAlive);
	SCRIPT_REG_FUNC(HasPower);
	SCRIPT_REG_TEMPLFUNC(SetPower, "bPower");
	SCRIPT_REG_TEMPLFUNC(AddEventListener, "nEntityID");
	SCRIPT_REG_TEMPLFUNC(RemoveEventListener, "nEntityID");
	SCRIPT_REG_FUNC(GetGUID);
	SCRIPT_REG_FUNC(GetClass);
	SCRIPT_REG_FUNC(GetClassName);
	SCRIPT_REG_FUNC(GetTeam);
	SCRIPT_REG_FUNC(GetTeamName);
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::GetHealth(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? 0.0f : pController->GetHealth());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::IsAlive(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? false : pController->IsAlive());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::HasPower(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? false : pController->HasPower());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::SetPower(IFunctionHandler *pH, bool bPower)
{
	IBuildingController *pController = GetController(pH);
	if (NULL != pController) pController->SetPower(bPower);
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::AddEventListener(IFunctionHandler *pH, ScriptHandle nEntityID)
{
	IBuildingController *pController = GetController(pH);
	if (NULL != pController && true == pController->AddScriptEventListener(nEntityID.n))
		return pH->EndFunction(1);
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::RemoveEventListener(IFunctionHandler *pH, ScriptHandle nEntityID)
{
	IBuildingController *pController = GetController(pH);
	if (NULL != pController) pController->RemoveScriptEventListener(nEntityID.n);
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::GetGUID(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? GUID_INVALID : pController->GetGUID());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::GetClass(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? BC_INVALID : pController->GetClass());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::GetClassName(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? "" : pController->GetClassName());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::GetTeam(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? TEAMID_NOTEAM : pController->GetTeam());
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::GetTeamName(IFunctionHandler *pH)
{
	IBuildingController *pController = GetController(pH);
	return pH->EndFunction(NULL == pController ? "" : pController->GetTeamName());
}