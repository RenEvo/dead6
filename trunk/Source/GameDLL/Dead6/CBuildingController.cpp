////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CBuildingController.cpp
//
// Purpose: Building controller which contains
//	a logical structure and is interfaced
//
// File History:
//	- 8/11/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CBuildingController.h"

////////////////////////////////////////////////////
CBuildingController::CBuildingController(void)
{
	m_nGUID = GUID_INVALID;
	m_fHealth = m_fInitHealth = m_fMaxHealth = 0.0f;
	m_pSS = NULL;
}

////////////////////////////////////////////////////
CBuildingController::~CBuildingController(void)
{
	Shutdown();
}

////////////////////////////////////////////////////
void CBuildingController::GetMemoryStatistics(ICrySizer *s)
{
	s->Add(m_szName);
	s->Add(m_szScript);
}

////////////////////////////////////////////////////
void CBuildingController::Initialize(BuildingGUID nGUID, float fHealth)
{
	m_nGUID = nGUID;
	m_fHealth = m_fInitHealth = MAX(0.0f,fHealth);
}

////////////////////////////////////////////////////
void CBuildingController::Shutdown(void)
{
	// Final reset
	Reset();

	// Call OnShutdown before releasing
	BEGIN_CALL_SERVER(m_pSS, m_pScriptTable, "OnShutdown")
	END_CALL(m_pSS)
	BEGIN_CALL_CLIENT(m_pSS, m_pScriptTable, "OnShutdown")
	END_CALL(m_pSS)

	if (NULL != m_pSS)
	{
		m_pSS->UnloadScript(m_szScript);
		m_pSS = NULL;
	}
}

////////////////////////////////////////////////////
void CBuildingController::Reset(void)
{
	// Reset to initial state
	m_fHealth = m_fInitHealth;

	// Call OnReset
	BEGIN_CALL_SERVER(m_pSS, m_pScriptTable, "OnReset")
	END_CALL(m_pSS)
	BEGIN_CALL_CLIENT(m_pSS, m_pScriptTable, "OnReset")
	END_CALL(m_pSS)
}

////////////////////////////////////////////////////
bool CBuildingController::LoadFromXml(char const* szName)
{
	assert(g_D6Core->pSystem);

	// Create path to XML file for this controller
	string szControllerXML = D6C_PATH_BUILDINGSXML, szMapControllerXML, szActualXML;
	szControllerXML += szName;
	szControllerXML += ".xml";
	szMapControllerXML = g_D6Core->pSystem->GetI3DEngine()->GetLevelFilePath("Buildings\\");
	szMapControllerXML += szName;
	szMapControllerXML += ".xml";
	szActualXML = szMapControllerXML;

	// Find and open controller's XML file
	XmlNodeRef pRootNode = NULL;
	if (NULL == (pRootNode = g_D6Core->pSystem->LoadXmlFile(szMapControllerXML.c_str())))
	{
		// Try default dir
		if (NULL == (pRootNode = g_D6Core->pSystem->LoadXmlFile(szControllerXML.c_str())))
		{
			g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
				VALIDATOR_FLAG_FILE, szControllerXML.c_str(), "Failed to load Building Definition for \'%s\'", szName);
			return false;
		}
		szActualXML = szControllerXML;
	}

	// Parse XML file
	m_szName = pRootNode->getAttr("Name");
	m_szScript = D6C_PATH_BUILDINGS;
	m_szScript += pRootNode->getAttr("Script");
	if (false == pRootNode->getAttr("MaxHealth", m_fMaxHealth))
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			VALIDATOR_FLAG_FILE, szActualXML.c_str(), "Missing MaxHealth property for building definition \'%s\', health is set to 0!", szName);
	}

	// Clamp init health to max health
	m_fHealth = m_fInitHealth = CLAMP(m_fInitHealth, 0.0f, m_fMaxHealth);

	// Load script and get table
	m_pSS = g_D6Core->pSystem->GetIScriptSystem();
	ScriptAnyValue temp;
	if (false == m_pSS->ExecuteFile(m_szScript.c_str(), true, false))
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			VALIDATOR_FLAG_FILE, m_szScript.c_str(), "Failed to execute building script for: %s", szName);
	}
	else if (false == m_pSS->GetGlobalAny(m_szName, temp) ||
		false == temp.CopyTo(m_pScriptTable))
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			VALIDATOR_FLAG_FILE, m_szScript.c_str(), "Failed to get script table for building: %s (Looking for \'%s\')",
			szName, m_szName);
	}
	else
	{
		// Call OnInit
		BEGIN_CALL_SERVER(m_pSS, m_pScriptTable, "OnInit")
		END_CALL(m_pSS)
		BEGIN_CALL_CLIENT(m_pSS, m_pScriptTable, "OnInit")
		END_CALL(m_pSS)
	}

	return true;
}