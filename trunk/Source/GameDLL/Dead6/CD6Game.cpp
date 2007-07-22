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

////////////////////////////////////////////////////
CD6Game::CD6Game(void)
{

}

////////////////////////////////////////////////////
CD6Game::~CD6Game(void)
{

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
		// Init base manager
		g_D6Core->GetBaseManager()->Initialize();

		// Init team manager
		g_D6Core->GetTeamManager()->Initialize();
	}

	return bBaseInit;
}

////////////////////////////////////////////////////
bool CD6Game::CompleteInit()
{
	CryLogAlways("CD6Game::CompleteInit()");
	return CGame::CompleteInit();
}

////////////////////////////////////////////////////
void CD6Game::Shutdown()
{
	// Shutdown D6 Core
	if (NULL != g_D6Core->GetBaseManager())
		g_D6Core->GetBaseManager()->Shutdown();
	if (NULL != g_D6Core->GetTeamManager())
		g_D6Core->GetTeamManager()->Shutdown();

	// Base shutdown
	CGame::Shutdown();
}

////////////////////////////////////////////////////
int CD6Game::Update(bool haveFocus, unsigned int updateFlags)
{
	return CGame::Update(haveFocus, updateFlags);
}