////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CTeamManager.cpp
//
// Purpose: Monitors team fluctuations and
//	callback scripts
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CTeamManager.h"

////////////////////////////////////////////////////
CTeamManager::CTeamManager(void)
{
	m_pGame = NULL;
}

////////////////////////////////////////////////////
CTeamManager::~CTeamManager(void)
{

}

////////////////////////////////////////////////////
void CTeamManager::Initialize(CD6Game *pGame)
{
	CryLogAlways("Initializing Dead6 Core: CTeamManager...");
	m_pGame = pGame;

	// Initial reset
	Reset();
}

////////////////////////////////////////////////////
void CTeamManager::Shutdown(void)
{
	// Clear the map
	m_TeamMap.clear();
}

////////////////////////////////////////////////////
void CTeamManager::Reset(void)
{
	// Clear the map
	m_TeamMap.clear();

	// Clear all teams from gamerules
	assert(m_pGame);
	m_pGame->GetD6GameRules()->ClearAllTeams();
}

////////////////////////////////////////////////////
TeamID CTeamManager::CreateTeam(char const* szName, char const* szScript)
{
	// Check if team already exists
	for (TeamMap::iterator itI = m_TeamMap.begin(); itI != m_TeamMap.end(); itI++)
	{
		if (0 == stricmp(szName, itI->second.szName.c_str()))
			return itI->first;
	}

	// Create entry for team
	STeamDef TeamDef;
	TeamDef.szName = szName;
	TeamDef.szScript = szScript;

	// Create team in gamerules
	assert(m_pGame);
	CD6GameRules *pRules = m_pGame->GetD6GameRules();
	assert(pRules);
	TeamDef.nID = pRules->CreateTeam(szName);

	// TODO Load team script

	return TEAMID_INVALID;
}

////////////////////////////////////////////////////
void CTeamManager::RemoveTeam(char const* szName)
{

}

////////////////////////////////////////////////////
void CTeamManager::RemoveTeam(TeamID const& nID)
{

}

////////////////////////////////////////////////////
STeamDef const* CTeamManager::GetTeamByName(char const* szName) const
{
	return NULL;
}

////////////////////////////////////////////////////
STeamDef const* CTeamManager::GetTeamByID(TeamID const& nID) const
{
	return NULL;
}