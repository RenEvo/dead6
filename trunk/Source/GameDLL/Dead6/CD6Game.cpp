////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CD6Game.cpp
//
// Purpose: Dead6 Core Game class
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CD6Game.h"

// Script binds
#include "ScriptBind_BaseManager.h"
#include "ScriptBind_TeamManager.h"

////////////////////////////////////////////////////
CD6Game::CD6Game(void)
{
	g_pGame = this;
	m_pScriptBindBaseManager = NULL;
	m_pScriptBindTeamManager = NULL;
}

////////////////////////////////////////////////////
CD6Game::~CD6Game(void)
{
	g_pGame = 0;
}

////////////////////////////////////////////////////
bool CD6Game::Init(IGameFramework *pFramework)
{
	CryLogAlways("CD6Game::Init()");
	
	// Base init
	bool bBaseInit = CGame::Init(pFramework);

	// D6 Core Init
	if (true == bBaseInit)
	{
		// Add to level listener
		pFramework->GetILevelSystem()->AddListener(this);

		// Init base manager
		g_D6Core->GetBaseManager()->Initialize();
		m_pScriptBindBaseManager->AttachTo(g_D6Core->GetBaseManager());

		// Init team manager
		g_D6Core->GetTeamManager()->Initialize();
		m_pScriptBindTeamManager->AttachTo(g_D6Core->GetTeamManager());
	}

	return bBaseInit;
}

////////////////////////////////////////////////////
bool CD6Game::CompleteInit()
{
	CryLogAlways("CD6Game::CompleteInit()");

	// Base complete init
	return CGame::CompleteInit();
}

////////////////////////////////////////////////////
void CD6Game::Shutdown()
{
	CryLogAlways("CD6Game::Shutdown()");

	// Remove as listener
	m_pFramework->GetILevelSystem()->RemoveListener(this);

	// Shutdown base manager
	if (NULL != g_D6Core->GetBaseManager())
		g_D6Core->GetBaseManager()->Shutdown();

	// Shutdown team manager
	if (NULL != g_D6Core->GetTeamManager())
		g_D6Core->GetTeamManager()->Shutdown();

	// Base shutdown
	CGame::Shutdown();
}

////////////////////////////////////////////////////
int CD6Game::Update(bool haveFocus, unsigned int updateFlags)
{
	// Base update
	int nUpdate = CGame::Update(haveFocus, updateFlags);

	return nUpdate;
}

////////////////////////////////////////////////////
const char *CD6Game::GetLongName()
{
	return D6GAME_LONGNAME;
}

////////////////////////////////////////////////////
const char *CD6Game::GetName()
{
	return D6GAME_NAME;
}

////////////////////////////////////////////////////
void CD6Game::InitScriptBinds()
{
	m_pScriptBindBaseManager = new CScriptBind_BaseManager(m_pFramework->GetISystem());
	m_pScriptBindTeamManager = new CScriptBind_TeamManager(m_pFramework->GetISystem());

	// Base script bind init
	CGame::InitScriptBinds();
}

////////////////////////////////////////////////////
void CD6Game::ReleaseScriptBinds()
{
	SAFE_DELETE(m_pScriptBindBaseManager);
	SAFE_DELETE(m_pScriptBindTeamManager);

	// Base script bind release
	CGame::ReleaseScriptBinds();
}

////////////////////////////////////////////////////
void CD6Game::OnLoadingStart(ILevelInfo *pLevel)
{
	// TODO Hope and pray pLevel becomes valid!
	// We want to parse the XML file from this location, so any entities that get created
	// during the level load have the required info to initialize themselves
}

////////////////////////////////////////////////////
void CD6Game::OnLoadingComplete(ILevel *pLevel)
{
	// TODO Migrate this to OnLoadingStart
	
	// Get CNC rules file path and open to root
	char const* szCNCRules = m_pFramework->GetISystem()->GetI3DEngine()->GetLevelFilePath("CNCRules.xml");
	XmlNodeRef pRootNode = m_pFramework->GetISystem()->LoadXmlFile(szCNCRules);
	if (NULL == pRootNode)
	{
		m_pFramework->GetISystem()->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
			VALIDATOR_FLAG_FILE, szCNCRules, "Missing/Corrupted CNCRules file for level");
	}
	else
	{
		// Parse general settings
		XmlNodeRef pGeneralSettingsNode = pRootNode->findChild("General");
		ParseCNCRules_General(pGeneralSettingsNode);
	}
}

////////////////////////////////////////////////////
void CD6Game::ParseCNCRules_General(XmlNodeRef &pNode)
{
	if (NULL == pNode) return;

	// Parse time of day
	XmlNodeRef pTODNode = pNode->findChild("TimeOfDay");
	if (NULL != pTODNode)
	{
		// Get info
		float fHour, fMin;
		bool bLoop = false;
		if (false == pTODNode->getAttr("Hour", fHour)) fHour = 0.0f;
		if (true == pTODNode->getAttr("Minute", fMin))
		{
			fMin = CLAMP(fMin, 0.0f, 60.0f);
			fHour += (fMin * (1.0f/60.0f));
		}
		fHour = CLAMP(fHour, 0.0f, 23.99f);
		pTODNode->getAttr("EnableLoop", bLoop);

		// Set it
		CryLog("[CNCRules] Setting time of day to \'%.2f\' %s looping", fHour, bLoop?"with":"without");
		ITimeOfDay *pTOD = m_pFramework->GetISystem()->GetI3DEngine()->GetTimeOfDay();
		assert(pTOD);
		pTOD->SetTime(fHour, true);
		pTOD->SetPaused(bLoop);
	}
}