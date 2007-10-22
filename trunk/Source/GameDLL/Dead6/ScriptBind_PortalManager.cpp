////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_PortalManager.cpp
//
// Purpose: Script binding for the portal manager
//
// File History:
//	- 8/20/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptBind_PortalManager.h"
#include "IPortalManager.h"

////////////////////////////////////////////////////
CScriptBind_PortalManager::CScriptBind_PortalManager(ISystem *pSystem)
{
	assert(pSystem);
	m_pSystem = pSystem;
	m_pSS = pSystem->GetIScriptSystem();
	m_pPortalManager = NULL;

	// Initial init
	Init(m_pSS, m_pSystem);
	SetGlobalName("Portal");
	RegisterMethods();
	RegisterGlobals();
}

////////////////////////////////////////////////////
CScriptBind_PortalManager::~CScriptBind_PortalManager(void)
{

}

////////////////////////////////////////////////////
void CScriptBind_PortalManager::AttachTo(IPortalManager *pPortalManager)
{
	m_pPortalManager = pPortalManager;
}

////////////////////////////////////////////////////
void CScriptBind_PortalManager::RegisterGlobals(void)
{
	//m_pSS->SetGlobalValue("Name", Value);
}

////////////////////////////////////////////////////
void CScriptBind_PortalManager::RegisterMethods(void)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_PortalManager::

	SCRIPT_REG_TEMPLFUNC(MakeEntityPortal, "nEntityID, szCameraEntity, szTexture, nFrameSkip");
	SCRIPT_REG_TEMPLFUNC(RemoveEntityPortal, "nEntityID");
}

////////////////////////////////////////////////////
int CScriptBind_PortalManager::MakeEntityPortal(IFunctionHandler *pH, ScriptHandle nEntityID, char const* szCameraEntity,
									char const* szTexture, int nFrameSkip)
{
	if (NULL == m_pPortalManager)
		return pH->EndFunctionNull();

	// Delegate
	if (false == m_pPortalManager->MakeEntityPortal(nEntityID.n, szCameraEntity, szTexture, nFrameSkip))
		return pH->EndFunctionNull();
	return pH->EndFunction(1);
}

////////////////////////////////////////////////////
int CScriptBind_PortalManager::RemoveEntityPortal(IFunctionHandler *pH, ScriptHandle nEntityID)
{
	if (NULL != m_pPortalManager)
		m_pPortalManager->RemoveEntityPortal(nEntityID.n);
	return pH->EndFunctionNull();
}