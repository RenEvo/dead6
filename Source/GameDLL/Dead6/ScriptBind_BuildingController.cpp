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
	// TODO
}

////////////////////////////////////////////////////
void CScriptBind_BuildingController::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_BuildingController::
	// TODO
	SCRIPT_REG_TEMPLFUNC(Test, "nTestVal");
}

////////////////////////////////////////////////////
int CScriptBind_BuildingController::Test(IFunctionHandler *pH, int nTest)
{
	IBuildingController *pController = GetController(pH);
	CryLogAlways("[CScriptBind_BuildingController] Test called for GUID: %u (nTest = %d)", pController->GetGUID(), nTest);
	return 0;
}