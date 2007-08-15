////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_TeamManager.cpp
//
// Purpose: Script binding for the team manager
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptBind_TeamManager.h"
#include "ITeamManager.h"

////////////////////////////////////////////////////
CScriptBind_TeamManager::CScriptBind_TeamManager(ISystem *pSystem)
{
	assert(pSystem);
	m_pSystem = pSystem;
	m_pSS = pSystem->GetIScriptSystem();
	m_pTeamManager = NULL;

	// Initial init
	Init(m_pSS, m_pSystem);
	SetGlobalName("Team");
	RegisterMethods();
	RegisterGlobals();
}

////////////////////////////////////////////////////
CScriptBind_TeamManager::~CScriptBind_TeamManager(void)
{

}

////////////////////////////////////////////////////
void CScriptBind_TeamManager::AttachTo(ITeamManager *pTeamManager)
{
	m_pTeamManager = pTeamManager;
}

////////////////////////////////////////////////////
void CScriptBind_TeamManager::RegisterGlobals(void)
{
	//m_pSS->SetGlobalValue("Name", Value);
}

////////////////////////////////////////////////////
void CScriptBind_TeamManager::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_TeamManager::

	//SCRIPT_REG_TEMPLFUNC(Function, "arg1, arg2, arg3");
}