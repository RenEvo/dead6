////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
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
#include "..\HUD\HUDMissionObjectiveSystem.h"

////////////////////////////////////////////////////
CTeamManager::CTeamManager(void)
{
	m_pGame = NULL;
	m_nTeamIDGen = TEAMID_NOTEAM;
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
	//assert(m_pGame);
	//m_pGame->GetD6GameRules()->ClearAllTeams();
}

////////////////////////////////////////////////////
void CTeamManager::GetMemoryStatistics(ICrySizer *s)
{
	s->AddContainer(m_TeamMap);
	for (TeamMap::iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end(); itTeam++)
	{
		// String objects
		s->Add(itTeam->second.szName);
		s->Add(itTeam->second.szScript);

		// Player map
		s->AddContainer(itTeam->second.PlayerList);
	}
}

////////////////////////////////////////////////////
void CTeamManager::PostInitClient(int nChannelID) const
{
	assert(g_D6Core->pD6GameRules);

	// Invoke RMIs for all entries in each team's player list
	for (TeamMap::const_iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end(); itTeam++)
	{
		for (TeamPlayerList::const_iterator itPlayer = itTeam->second.PlayerList.begin();
			itPlayer != itTeam->second.PlayerList.end(); itPlayer++)
		{
			g_D6Core->pD6GameRules->GetGameObject()->InvokeRMIWithDependentObject(
				CGameRules::ClSetTeam(), CGameRules::SetTeamParams(*itPlayer, itTeam->first), 
				eRMI_ToClientChannel, *itPlayer, nChannelID);
		}
	}
}

////////////////////////////////////////////////////
void CTeamManager::CmdDebugTeams(IConsoleCmdArgs *pArgs)
{
	CryLogAlways("// Teams //");
	for (TeamMap::const_iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end(); itTeam++)
	{
		CryLogAlways("Team: %s  (id: %d)", itTeam->second.szName.c_str(), itTeam->first);
		for (TeamPlayerList::const_iterator itPlayer = itTeam->second.PlayerList.begin();
			itPlayer != itTeam->second.PlayerList.end(); itPlayer++)
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(*itPlayer);
			CryLogAlways("    -> Entity: %s  class: %s  (eid: %d %08x)", 
				pEntity->GetName(), pEntity->GetClass()->GetName(), pEntity->GetId(), 
				pEntity->GetId());
		}
	}
}

////////////////////////////////////////////////////
void CTeamManager::CmdDebugObjectives(IConsoleCmdArgs *pArgs, const char **status)
{
	CryLogAlways("// Teams //");
	for (TeamMap::const_iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end(); itTeam++)
	{
		if (CGameRules::TObjectiveMap *pObjectives = g_D6Core->pD6GameRules->GetTeamObjectives(itTeam->first))
		{
			for (CGameRules::TObjectiveMap::const_iterator it=pObjectives->begin(); it!=pObjectives->end(); ++it)
				CryLogAlways("  -> Objective: %s  teamId: %d  status: %s  (eid: %d %08x)", it->first.c_str(), itTeam->first,
					status[CLAMP(it->second.status, 0, CHUDMissionObjective::LAST)], it->second.entityId, it->second.entityId);
		}
	}
}

////////////////////////////////////////////////////
TeamID CTeamManager::CreateTeam(char const* szName)
{
	// Check if team already exists
	for (TeamMap::iterator itI = m_TeamMap.begin(); itI != m_TeamMap.end(); itI++)
	{
		if (0 == stricmp(szName, itI->second.szName.c_str()))
			return itI->first;
	}

	// Create entry for team
	STeamDef TeamDef;
	TeamDef.nID = ++m_nTeamIDGen;
	TeamDef.nSpawnGroupID = 0;
	TeamDef.szName = szName;

	// TODO Load script from CNCRules.xml

	// Create entry and return ID
	m_TeamMap[TeamDef.nID] = TeamDef;
	return TeamDef.nID;
}

////////////////////////////////////////////////////
void CTeamManager::RemoveTeam(TeamID nTeamID)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	m_TeamMap.erase(itEntry);
}

////////////////////////////////////////////////////
const char *CTeamManager::GetTeamName(TeamID nTeamID) const
{
	// Find it
	TeamMap::const_iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return NULL;

	// Return the name
	return itEntry->second.szName.c_str();
}

////////////////////////////////////////////////////
TeamID CTeamManager::GetTeamId(char const* szName) const
{
	// Look through all teams for the name
	for (TeamMap::const_iterator itEntry = m_TeamMap.begin(); itEntry != m_TeamMap.end();
		itEntry++)
	{
		if (0 == stricmp(itEntry->second.szName.c_str(), szName))
		{
			return itEntry->first;
		}
	}
	return TEAMID_INVALID;
}

////////////////////////////////////////////////////
int CTeamManager::GetTeamCount(void) const
{
	return m_TeamMap.size();
}

////////////////////////////////////////////////////
int CTeamManager::GetTeamPlayerCount(TeamID nTeamID, bool bInGame) const
{
	// Find team entry
	TeamMap::const_iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return 0;

	// Count player map
	if (false == bInGame)
	{
		// Return size of player map
		return itEntry->second.PlayerList.size();
	}
	else
	{
		// Check each player to see if they are in the game
		int nCount = 0;
		for (TeamPlayerList::const_iterator itPlayer = itEntry->second.PlayerList.begin();
			itPlayer != itEntry->second.PlayerList.end(); itPlayer++)
		{
			if (g_D6Core->pD6GameRules->IsPlayerInGame(*itPlayer))
				++nCount;
		}
		return nCount;
	}
	return 0;
}

////////////////////////////////////////////////////
EntityId CTeamManager::GetTeamPlayer(TeamID nTeamID, int nidx)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return 0;

	// Check bounds
	if (nidx < 0 || nidx >= itEntry->second.PlayerList.size())
		return 0;

	// Return player in index
	return itEntry->second.PlayerList[nidx];
}

////////////////////////////////////////////////////
void CTeamManager::GetTeamPlayers(TeamID nTeamID, TeamPlayerList &players)
{
	// Clear player map
	players.resize(0);

	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	// Set players to player map for team
	players = itEntry->second.PlayerList;
}

////////////////////////////////////////////////////
void CTeamManager::SetTeam(TeamID nTeamID, EntityId nEntityID)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	// TODO Add entity to player map
}

////////////////////////////////////////////////////
TeamID CTeamManager::GetTeam(EntityId nEntityID) const
{
	// Look through all teams
	for (TeamMap::const_iterator itEntry = m_TeamMap.begin(); itEntry != m_TeamMap.end();
		itEntry++)
	{
		for (TeamPlayerList::const_iterator itPlayer = itEntry->second.PlayerList.begin();
			itPlayer != itEntry->second.PlayerList.end(); itPlayer++)
		{
			if (*itPlayer == nEntityID)
				return itEntry->first;
		}
	}

	// Not found
	return 0;
}

////////////////////////////////////////////////////
bool CTeamManager::ChangeTeam(EntityId nEntityID, TeamID nTeamID)
{
	// Get the old team this entity belongs on
	int nOldTeam = GetTeam(nEntityID);
	if (nOldTeam == nTeamID)
		return true;

	// If it is a player, remove it from the map
	bool bIsPlayer = (0 != g_D6Core->pD6Game->GetIGameFramework()->GetIActorSystem()->GetActor(nEntityID));
	if (false == bIsPlayer) return false;
	if (true == bIsPlayer && nOldTeam)
	{
		TeamMap::iterator itTeam = m_TeamMap.find(nOldTeam);
		if (itTeam != m_TeamMap.end())
		{
			stl::find_and_erase(itTeam->second.PlayerList, nEntityID);
		}
	}

	// If we have a new valid team, set the player into it
	if (nTeamID)
	{
		TeamMap::iterator itTeam = m_TeamMap.find(nOldTeam);
		if (itTeam != m_TeamMap.end())
		{
			itTeam->second.PlayerList.push_back(nEntityID);
		}
	}
	return true;
}

////////////////////////////////////////////////////
void CTeamManager::SetTeamDefaultSpawnGroup(int nTeamID, EntityId nSpawnGroupId)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	// Set it
	itEntry->second.nSpawnGroupID = nSpawnGroupId;
}

////////////////////////////////////////////////////
EntityId CTeamManager::GetTeamDefaultSpawnGroup(int nTeamID) const
{
	// Find team entry
	TeamMap::const_iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return 0;
	return itEntry->second.nSpawnGroupID;
}

////////////////////////////////////////////////////
void CTeamManager::RemoveTeamDefaultSpawnGroup(int nTeamID)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	// Reset to 0
	itEntry->second.nSpawnGroupID = 0;
}

////////////////////////////////////////////////////
void CTeamManager::RemoveDefaultSpawnGroupFromTeams(EntityId nSpawnGroupId)
{
	// Look through all teams
	for (TeamMap::iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end();
		itTeam++)
	{
		if (itTeam->second.nSpawnGroupID == nSpawnGroupId)
		{
			itTeam->second.nSpawnGroupID = 0;
		}
	}
}