////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
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
#include "CD6GameRules.h"

// Script binds
#include "ScriptBind_BaseManager.h"
#include "ScriptBind_TeamManager.h"
#include "ScriptBind_BuildingController.h"
#include "ScriptBind_PortalManager.h"
#include "ScriptBind_D6Player.h"

////////////////////////////////////////////////////
CD6Game::CD6Game(void)
{
	g_pGame = this;
	m_pScriptBindBaseManager = NULL;
	m_pScriptBindTeamManager = NULL;
	m_pScriptBindBuildingController = NULL;
	m_pScriptBindPortalManager = NULL;
	m_pScriptBindD6Player = NULL;
	m_bEditorGameStarted = false;
}

////////////////////////////////////////////////////
CD6Game::~CD6Game(void)
{
	g_pGame = 0;
}

////////////////////////////////////////////////////
CD6GameRules *CD6Game::GetD6GameRules() const
{
	return static_cast<CD6GameRules*>(m_pFramework->GetIGameRulesSystem()->GetCurrentGameRules());
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
		g_D6Core->pBaseManager->Initialize();
		m_pScriptBindBaseManager->AttachTo(g_D6Core->pBaseManager);

		// Init team manager
		g_D6Core->pTeamManager->Initialize();
		m_pScriptBindTeamManager->AttachTo(g_D6Core->pTeamManager);

		// Init portal manager
		g_D6Core->pPortalManager->Initialize();
		m_pScriptBindPortalManager->AttachTo(g_D6Core->pPortalManager);
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
	if (NULL != g_D6Core->pBaseManager)
		g_D6Core->pBaseManager->Shutdown();

	// Shutdown team manager
	if (NULL != g_D6Core->pTeamManager)
		g_D6Core->pTeamManager->Shutdown();

	// Shutdown portal manager
	if (NULL != g_D6Core->pPortalManager)
		g_D6Core->pPortalManager->Shutdown();

	// Base shutdown
	CGame::Shutdown();
}

////////////////////////////////////////////////////
int CD6Game::Update(bool haveFocus, unsigned int updateFlags)
{
	// Base update
	int nUpdate = CGame::Update(haveFocus, updateFlags);

	// Update team manager
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->Update(haveFocus, updateFlags);

	// Update base manager
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->Update(haveFocus, updateFlags);

	// Update portal manaager
	assert(g_D6Core->pPortalManager);
	g_D6Core->pPortalManager->Update(haveFocus, updateFlags);

	return nUpdate;
}
	
////////////////////////////////////////////////////
void CD6Game::GetMemoryStatistics(ICrySizer *s)
{
	// Base first
	CGame::GetMemoryStatistics(s);

	// Core items
	if (NULL != g_D6Core->pBaseManager)
		g_D6Core->pBaseManager->GetMemoryStatistics(s);
	if (NULL != g_D6Core->pTeamManager)
		g_D6Core->pTeamManager->GetMemoryStatistics(s);
	if (NULL != g_D6Core->pPortalManager)
		g_D6Core->pPortalManager->GetMemoryStatistics(s);
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
bool CD6Game::IsEditorGameStarted(void) const
{
	return m_bEditorGameStarted;
}

////////////////////////////////////////////////////
void CD6Game::EditorResetGame(bool bStart)
{
	m_bEditorGameStarted = bStart;

	// Base reset
	CGame::EditorResetGame(bStart);

	// Perform soft resets
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->ResetGame(bStart);
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->ResetGame(bStart);

	// Reset portal manager
	assert(g_D6Core->pPortalManager);
	g_D6Core->pPortalManager->Reset();
}

////////////////////////////////////////////////////
void CD6Game::InitScriptBinds()
{
	m_pScriptBindBaseManager = new CScriptBind_BaseManager(m_pFramework->GetISystem());
	m_pScriptBindTeamManager = new CScriptBind_TeamManager(m_pFramework->GetISystem());
	m_pScriptBindBuildingController = new CScriptBind_BuildingController(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindPortalManager = new CScriptBind_PortalManager(m_pFramework->GetISystem());
	m_pScriptBindD6Player = new CScriptBind_D6Player(m_pFramework->GetISystem());

	// Base script bind init
	CGame::InitScriptBinds();
}

////////////////////////////////////////////////////
void CD6Game::ReleaseScriptBinds()
{
	SAFE_DELETE(m_pScriptBindBaseManager);
	SAFE_DELETE(m_pScriptBindTeamManager);
	SAFE_DELETE(m_pScriptBindBuildingController);
	SAFE_DELETE(m_pScriptBindPortalManager);
	SAFE_DELETE(m_pScriptBindD6Player);

	// Base script bind release
	CGame::ReleaseScriptBinds();
}

////////////////////////////////////////////////////
void CD6Game::OnLoadingStart(ILevelInfo *pLevel)
{
	// Reset the team manager
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->Reset();

	// Reset base manager
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->Reset();

	// Reset portal manager
	assert(g_D6Core->pPortalManager);
	g_D6Core->pPortalManager->Reset();

	// TODO Hope and pray pLevel becomes valid!
	// We want to parse the XML file from this location, so any entities that get created
	//	during the level load have the required info to initialize themselves.
	// For now, we will send a message to all entities to let them do a post-init after the
	//	XML file has been parsed.

	// TODO Don't do the CNC rules load if no level is loaded. The editor gives a false alarm here
	//	when it is first loaded.

	// Get CNC rules file path and open to root
	char const* szCNCRules = m_pFramework->GetISystem()->GetI3DEngine()->GetLevelFilePath("CNCRules.xml");
	XmlNodeRef pRootNode = m_pFramework->GetISystem()->LoadXmlFile(szCNCRules);
	if (NULL == pRootNode)
	{
		// Try default
		pRootNode = m_pFramework->GetISystem()->LoadXmlFile(D6C_DEFAULT_GAMERULES);
		if (NULL == pRootNode)
		{
			// No good..
			g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR,
				VALIDATOR_FLAG_FILE, "", "Missing/Corrupted CNCRules file for level : %s",
				"TODO"/*TODO: pLevel->GetLevelInfo()->GetName()*/);
			return;
		}
		else
		{
			m_pFramework->GetISystem()->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
				VALIDATOR_FLAG_FILE, szCNCRules, "Using default CNC Rules for level \'%s\'",
				"TODO"/*TODO: pLevel->GetLevelInfo()->GetName()*/);
		}
	}

	// Parse general settings
	XmlNodeRef pGeneralSettingsNode = pRootNode->findChild("General");
	ParseCNCRules_General(pGeneralSettingsNode);

	// Parse team settings
	XmlNodeRef pTeamSettingsNode = pRootNode->findChild("Teams");
	ParseCNCRules_Teams(pTeamSettingsNode);

	// Parse building controller settings
	XmlNodeRef pBuildingSettingsNode = pRootNode->findChild("Buildings");
	ParseCNCRules_Buildings(pBuildingSettingsNode);
}

////////////////////////////////////////////////////
void CD6Game::OnLoadingComplete(ILevel *pLevel)
{
	assert(g_D6Core->pBaseManager);

	// Prepare team manager
	g_D6Core->pTeamManager->ResetGame(true);

	// Validate and prepare controllers
	g_D6Core->pBaseManager->ResetGame(true);
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
		pTOD->SetPaused(!bLoop);
	}
}

////////////////////////////////////////////////////
void CD6Game::ParseCNCRules_Teams(XmlNodeRef &pNode)
{
	if (NULL == pNode || NULL == g_D6Core->pTeamManager) return;

	// Parse each team entry
	XmlNodeRef pTeamNode;
	int nCount = pNode->getChildCount();
	for (int i = 0; i < nCount; i++)
	{
		// Get attribute and create team with it
		pTeamNode = pNode->getChild(i);
		if (NULL != pTeamNode && stricmp(pTeamNode->getTag(), "Team") == 0)
		{
			// If element, have team manager open up team's XML file
			char const* szContent = pTeamNode->getContent();
			if (NULL != szContent && NULL != szContent[0])
			{
				// Have manager open it up and extract the node info
				g_D6Core->pTeamManager->CreateTeam(szContent);
			}
			else
			{
				// Create the team
				g_D6Core->pTeamManager->CreateTeam(pTeamNode);
			}
		}
	}
}

////////////////////////////////////////////////////
void CD6Game::ParseCNCRules_Buildings(XmlNodeRef &pNode)
{
	if (NULL == pNode || NULL == g_D6Core->pBaseManager) return;

	// Parse each building entry and create the controllers for them
	XmlNodeRef pBuildingNode;
	XmlString szTeam, szName;
	int nCount = pNode->getChildCount();
	for (int i = 0; i < nCount; i++)
	{
		// Check if it is a real building
		pBuildingNode = pNode->getChild(i);
		if (NULL != pBuildingNode && stricmp(pBuildingNode->getTag(), "Building") == 0)
		{
			// Get team and name attributes
			if (false == pBuildingNode->getAttr("Team", szTeam) || false == pBuildingNode->getAttr("Name", szName))
				continue;
			g_D6Core->pBaseManager->CreateBuildingController(szTeam, szName, pBuildingNode);
		}
	}
}

////////////////////////////////////////////////////
CScriptBind_Actor *CD6Game::GetActorScriptBind()
{
	return m_pScriptBindD6Player;
}