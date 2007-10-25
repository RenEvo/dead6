////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_D6GameRules.cpp
//
// Purpose: Dead6 Core GameRules script binding, replaces
//	ScriptBind_GameRules for D6-specific GameRules controls
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#include "Stdafx.h"
#include "ScriptBind_D6GameRules.h"

////////////////////////////////////////////////////
CScriptBind_D6GameRules::CScriptBind_D6GameRules(ISystem *pSystem, IGameFramework *pGameFramework) :
	CScriptBind_GameRules(pSystem, pGameFramework)
{
	m_pGameFW = pGameFramework;

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_D6GameRules::
	
	SCRIPT_REG_FUNC(IsEditor);
	SCRIPT_REG_FUNC(IsInEditorGame);
}

////////////////////////////////////////////////////
CScriptBind_D6GameRules::~CScriptBind_D6GameRules()
{

}

////////////////////////////////////////////////////
CD6GameRules *CScriptBind_D6GameRules::GetD6GameRules(IFunctionHandler *pH)
{
	return static_cast<CD6GameRules *>(m_pGameFW->GetIGameRulesSystem()->GetCurrentGameRules());
}

////////////////////////////////////////////////////
int CScriptBind_D6GameRules::IsEditor(IFunctionHandler *pH)
{
	CD6GameRules *pGameRules = GetD6GameRules(pH);
	if (NULL == pGameRules)
		return pH->EndFunctionNull();
	return pH->EndFunction(gEnv->bEditor);
}

////////////////////////////////////////////////////
int CScriptBind_D6GameRules::IsInEditorGame(IFunctionHandler *pH)
{
	CD6GameRules *pGameRules = GetD6GameRules(pH);
	if (NULL == pGameRules)
		return pH->EndFunctionNull();
	return pH->EndFunction(true == gEnv->bEditor && true == g_D6Core->pD6Game->IsEditorGameStarted());
}