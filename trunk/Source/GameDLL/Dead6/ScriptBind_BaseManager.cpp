////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
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
	//m_pSS->SetGlobalValue("Name", Value);
}

////////////////////////////////////////////////////
void CScriptBind_BaseManager::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_BaseManager::

	//SCRIPT_REG_TEMPLFUNC(Function, "arg1, arg2, arg3");
}