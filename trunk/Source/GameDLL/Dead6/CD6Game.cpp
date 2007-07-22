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
CD6GameLevelListener::CD6GameLevelListener(IGame *pD6Game)
{
	m_pGame = NULL;
}

////////////////////////////////////////////////////
void CD6GameLevelListener::OnLevelNotFound(const char *levelName)
{
	CryLogAlways("CD6GameLevelListener::OnLevelNotFound(%s)", levelName);
}

////////////////////////////////////////////////////
void CD6GameLevelListener::OnLoadingStart(ILevelInfo *pLevel)
{
	CryLogAlways("CD6GameLevelListener::OnLoadingStart(%s)", pLevel->GetName());
}

////////////////////////////////////////////////////
void CD6GameLevelListener::OnLoadingComplete(ILevel *pLevel)
{
	CryLogAlways("CD6GameLevelListener::OnLoadingComplete(%s)", pLevel->GetLevelInfo()->GetName());
}

////////////////////////////////////////////////////
void CD6GameLevelListener::OnLoadingError(ILevelInfo *pLevel, const char *error)
{
	CryLogAlways("CD6GameLevelListener::OnLoadingError(%s, %s)", pLevel->GetName(), error);
}

////////////////////////////////////////////////////
void CD6GameLevelListener::OnLoadingProgress(ILevelInfo *pLevel, int progressAmount)
{

}

////////////////////////////////////////////////////
CD6Game::CD6Game(void)
{
	g_pGame = this;
	m_pLevelListener = NULL;
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
		// Create level listener
		m_pLevelListener = new CD6GameLevelListener(this);
		GetIGameFramework()->GetILevelSystem()->AddListener(m_pLevelListener);

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
	// Destroy level listener
	GetIGameFramework()->GetILevelSystem()->RemoveListener(m_pLevelListener);
	SAFE_DELETE(m_pLevelListener);

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
	return CGame::Update(haveFocus, updateFlags);
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