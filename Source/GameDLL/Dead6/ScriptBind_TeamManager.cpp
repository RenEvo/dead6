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
	SetGlobalName("Teams");
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
	m_pSS->SetGlobalValue("TEAMID_NOTEAM", TEAMID_NOTEAM);
}

////////////////////////////////////////////////////
void CScriptBind_TeamManager::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_TeamManager::

	SCRIPT_REG_TEMPLFUNC(SetEditorTeam, "nTeamID");
	SCRIPT_REG_TEMPLFUNC(SetEditorTeamByName, "szTeam");
	SCRIPT_REG_TEMPLFUNC(SetTeamCredits, "nTeamID, nAmount");
	SCRIPT_REG_TEMPLFUNC(GiveTeamCredits, "nTeamID, nAmount");
	SCRIPT_REG_TEMPLFUNC(TakeTeamCredits, "nTeamID, nAmount");
}

////////////////////////////////////////////////////
int CScriptBind_TeamManager::SetEditorTeam(IFunctionHandler *pH, int nTeamID)
{
	assert(m_pTeamManager);
	m_pTeamManager->SetEditorTeam(nTeamID);
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_TeamManager::SetEditorTeamByName(IFunctionHandler *pH, char const* szTeam)
{
	assert(m_pTeamManager);
	m_pTeamManager->SetEditorTeam(m_pTeamManager->GetTeamId(szTeam));
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_TeamManager::SetTeamCredits(IFunctionHandler *pH, int nTeamID, unsigned int nAmount)
{
	assert(m_pTeamManager);
	m_pTeamManager->SetTeamCredits(nTeamID, nAmount);
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_TeamManager::GiveTeamCredits(IFunctionHandler *pH, int nTeamID, unsigned int nAmount)
{
	assert(m_pTeamManager);
	m_pTeamManager->GiveTeamCredits(nTeamID, nAmount);
	return pH->EndFunctionNull();
}

////////////////////////////////////////////////////
int CScriptBind_TeamManager::TakeTeamCredits(IFunctionHandler *pH, int nTeamID, unsigned int nAmount)
{
	assert(m_pTeamManager);
	m_pTeamManager->TakeTeamCredits(nTeamID, nAmount);
	return pH->EndFunctionNull();
}