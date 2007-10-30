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
#include <windows.h>
#include <psapi.h>
#pragma comment (lib, "psapi.lib")
#include "CD6Game.h"
#include "CD6GameRules.h"

// Script binds
#include "ScriptBind_BaseManager.h"
#include "ScriptBind_TeamManager.h"
#include "ScriptBind_BuildingController.h"
#include "ScriptBind_PortalManager.h"
#include "ScriptBind_D6Player.h"
#include "ScriptBind_D6GameRules.h"

////////////////////////////////////////////////////
CD6Game::CD6Game(void)
{
	g_pGame = this;
	m_pScriptBindBaseManager = NULL;
	m_pScriptBindTeamManager = NULL;
	m_pScriptBindBuildingController = NULL;
	m_pScriptBindPortalManager = NULL;
	m_pScriptBindD6Player = NULL;
	m_pScriptBindD6GameRules = NULL;
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
CGameRules *CD6Game::GetGameRules() const
{
	return GetD6GameRules();
}

////////////////////////////////////////////////////
void CD6Game::RegisterConsoleVars()
{
	CGame::RegisterConsoleVars();
}

////////////////////////////////////////////////////
void CD6Game::RegisterConsoleCommands()
{
	CGame::RegisterConsoleCommands();

	// D6 commands
	m_pConsole->AddCommand("level", CmdLoadLevel, 0, "Load specified level");
}

////////////////////////////////////////////////////
void CD6Game::UnregisterConsoleCommands()
{
	CGame::UnregisterConsoleCommands();

	// D6 commands
	m_pConsole->RemoveCommand("level");
}

////////////////////////////////////////////////////
void CD6Game::CmdLoadLevel(IConsoleCmdArgs *pArgs)
{
	if (pArgs->GetArgCount() <= 0) return;

	// Get level to load
	string szLevel = pArgs->GetArg(1);

	// Prepare for loading
	ILevelSystem *pLevelSystem = g_D6Core->pD6Game->GetIGameFramework()->GetILevelSystem();
	ILevel *pLevel = pLevelSystem->LoadLevel(szLevel.c_str());
	ILevelInfo *pLevelInfo = pLevelSystem->GetLevelInfo(szLevel.c_str());
	if (NULL == pLevel || NULL == pLevelInfo) return;
	//gEnv->pMovieSystem->StopAllCutScenes();
	//GetISystem()->SerializingFile(1);

	//// Load level
	//EntityId playerID = GetIGameFramework()->GetClientActorId();
	//pLevelSystem->OnLoadingStart(pLevelInfo);
	//PlayerIdSet(playerID);
	//if(const char* visibleName = GetMappedLevelName(levelstart))
	//	levelstart = visibleName;
	////levelstart.append("_levelstart.crysisjmsf"); //because of the french law we can't do this ...
	//levelstart.append("_crysis.crysisjmsf");
	//GetIGameFramework()->LoadGame(levelstart.c_str(), true, true);
	////**********
	//pLevelSystem->OnLoadingComplete(pLevel);
	//GetMenu()->OnActionEvent(SActionEvent(eAE_inGame));	//reset the menu
	//m_bReload = false;	//if m_bReload is true - load at levelstart

	//// Unpause and continue
	//m_pFramework->PauseGame(false, true);
	//GetISystem()->SerializingFile(0);
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

	m_bEditorGameStarted = false;

	// Base complete init
	return CGame::CompleteInit();
}

////////////////////////////////////////////////////
void CD6Game::Shutdown()
{
	CryLogAlways("CD6Game::Shutdown()");

	m_EditorGameListeners.clear();

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

	// Signal listeners
	CD6Player *pLocalPlayer = static_cast<CD6Player*>(g_pGame->GetIGameFramework()->GetClientActor());
	void (IEditorGameListener::*fCallBack)(CD6Player*) = (true == bStart ? &IEditorGameListener::OnEditorGameStart : &IEditorGameListener::OnEditorGameEnd);
	for (EditorGameListeners::iterator itListener = m_EditorGameListeners.begin(); itListener != m_EditorGameListeners.end(); itListener++)
	{
		((*itListener)->*fCallBack)(pLocalPlayer);
	}
}

////////////////////////////////////////////////////
void CD6Game::InitScriptBinds()
{
	m_pScriptBindBaseManager = new CScriptBind_BaseManager(m_pFramework->GetISystem());
	m_pScriptBindTeamManager = new CScriptBind_TeamManager(m_pFramework->GetISystem());
	m_pScriptBindBuildingController = new CScriptBind_BuildingController(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindPortalManager = new CScriptBind_PortalManager(m_pFramework->GetISystem());
	m_pScriptBindD6Player = new CScriptBind_D6Player(m_pFramework->GetISystem());
	m_pScriptBindD6GameRules = new CScriptBind_D6GameRules(m_pFramework->GetISystem(), m_pFramework);

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
	SAFE_DELETE(m_pScriptBindD6GameRules);

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
	
	// Load default rules in
	if (false == ParseCNCRules(D6C_DEFAULT_GAMERULES))
	{
		g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR,
			VALIDATOR_FLAG_FILE, "", "Missing/Corrupted Default CNCRules file : %s", D6C_DEFAULT_GAMERULES);
	}

	// Load map-specific rules in
	ParseCNCRules(m_pFramework->GetISystem()->GetI3DEngine()->GetLevelFilePath("CNCRules.xml"));
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
bool CD6Game::ParseCNCRules(char const* szXMLFile)
{
	// Attempt to load it
	XmlNodeRef pRootNode = m_pFramework->GetISystem()->LoadXmlFile(szXMLFile);
	if (NULL == pRootNode) return false;

	// Extract general settings
	XmlNodeRef pGeneralSettingsNode = pRootNode->findChild("General");
	if (NULL != pGeneralSettingsNode)
	{
		// Parse time of day
		XmlNodeRef pTODNode = pGeneralSettingsNode->findChild("TimeOfDay");
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
			ITimeOfDay *pTOD = m_pFramework->GetISystem()->GetI3DEngine()->GetTimeOfDay();
			assert(pTOD);
			CryLog("[CNCRules] Setting time of day to \'%.2f\' %s looping", fHour, bLoop?"with":"without");
			pTOD->SetTime(fHour, true);
			pTOD->SetPaused(!bLoop);
		}
	}

	// Extract team settings
	g_D6Core->pTeamManager->LoadTeams(pRootNode->findChild("Teams"));

	// Extract building settings
	g_D6Core->pBaseManager->LoadBuildingControllers(pRootNode->findChild("Buildings"));

	return true;
}

////////////////////////////////////////////////////
CScriptBind_Actor *CD6Game::GetActorScriptBind()
{
	return m_pScriptBindD6Player;
}

////////////////////////////////////////////////////
CScriptBind_GameRules *CD6Game::GetGameRulesScriptBind()
{
	return m_pScriptBindD6GameRules;
}

////////////////////////////////////////////////////
void CD6Game::AddEditorGameListener(IEditorGameListener *pListener)
{
	// If already added, drop it
	for (EditorGameListeners::iterator itListener = m_EditorGameListeners.begin();
		itListener != m_EditorGameListeners.end(); itListener++)
	{
		if (*itListener == pListener) return;
	}
	m_EditorGameListeners.push_back(pListener);
}

////////////////////////////////////////////////////
void CD6Game::RemoveEditorGameListener(IEditorGameListener *pListener)
{
	// Find and remove
	for (EditorGameListeners::iterator itListener = m_EditorGameListeners.begin();
		itListener != m_EditorGameListeners.end(); itListener++)
	{
		if (*itListener == pListener)
		{
			m_EditorGameListeners.erase(itListener);
			return;
		}
	}
}