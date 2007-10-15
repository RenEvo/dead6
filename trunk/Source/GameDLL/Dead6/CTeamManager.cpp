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
#include "IVehicleSystem.h"

////////////////////////////////////////////////////
CTeamManager::CTeamManager(void)
{
	m_nTeamIDGen = TEAMID_NOTEAM;
	m_nHarvIDGen = HARVESTERID_INVALID;
}

////////////////////////////////////////////////////
CTeamManager::~CTeamManager(void)
{
	Shutdown();
}

////////////////////////////////////////////////////
void CTeamManager::Initialize(void)
{
	CryLogAlways("Initializing Dead6 Core: CTeamManager...");

	// Initial reset
	Reset();
}

////////////////////////////////////////////////////
void CTeamManager::Shutdown(void)
{
	// Final reset
	Reset();
}

////////////////////////////////////////////////////
void CTeamManager::Reset(void)
{
	CryLog("[TeamManager] Reset");

	// Do a game reset
	ResetGame();

	// Remove all teams
	while (false == m_TeamMap.empty())
	{
		RemoveTeam(m_TeamMap.begin()->first);
	}

	// Reset ID gens
	m_nTeamIDGen = TEAMID_NOTEAM;
}

////////////////////////////////////////////////////
void CTeamManager::ResetGame(void)
{
	CryLog("[TeamManager] Removing team's harvesters...");

	// Remove all harvesters for all the teams
	for (TeamMap::iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end(); itTeam++)
	{
		_RemoveTeamsHarvesters(itTeam->first);
	}

	// Reset ID generator
	m_nHarvIDGen = HARVESTERID_INVALID;
}

////////////////////////////////////////////////////
void CTeamManager::_RemoveTeamsHarvesters(TeamID nID)
{
	// Find team entry
	TeamMap::iterator itTeam = m_TeamMap.find(nID);
	if (itTeam == m_TeamMap.end()) return;

	// Remove them
	std::string szTeamName = GetTeamName(itTeam->first);
	for (HarvesterList::iterator itHarv = itTeam->second.Harvesters.begin();
		itHarv != itTeam->second.Harvesters.end(); itHarv++)
	{
		CryLog("[TeamManager] Removing harvester %d (Team \'%s\' : %u)", itHarv->first,
			szTeamName.c_str(), itTeam->first);
		gEnv->pEntitySystem->RemoveEntity(itHarv->second.nEntityID);
	}
	itTeam->second.Harvesters.clear();
}

////////////////////////////////////////////////////
void CTeamManager::Update(bool bHaveFocus, unsigned int nUpdateFlags)
{
	// TODO Update harvester logic here


	// TODO Check for harvester respawn

}

////////////////////////////////////////////////////
void CTeamManager::GetMemoryStatistics(ICrySizer *s)
{
	s->AddContainer(m_TeamMap);
	for (TeamMap::iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end(); itTeam++)
	{
		// String objects
		s->Add(itTeam->second.szName);
		s->Add(itTeam->second.szLongName);
		s->Add(itTeam->second.szScript);
		s->Add(itTeam->second.szXML);

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
TeamID CTeamManager::CreateTeam(char const* szTeam)
{
	assert(g_D6Core->pSystem);

	// Create path to XML file for this team
	string szTeamXML = D6C_PATH_TEAMSXML, szMapTeamXML;
	szTeamXML += szTeam;
	szTeamXML += ".xml";
	szMapTeamXML = g_D6Core->pSystem->GetI3DEngine()->GetLevelFilePath("Teams\\");
	szMapTeamXML += szTeam;
	szMapTeamXML += ".xml";

	// Check if team already exists
	for (TeamMap::iterator itI = m_TeamMap.begin(); itI != m_TeamMap.end(); itI++)
	{
		if (0 == stricmp(szTeamXML.c_str(), itI->second.szXML.c_str()))
			return itI->first;
	}

	// Find and open team's XML file
	XmlNodeRef pRootNode = NULL;
	if (NULL == (pRootNode = g_D6Core->pSystem->LoadXmlFile(szMapTeamXML.c_str())))
	{
		// Try default dir
		if (NULL == (pRootNode = g_D6Core->pSystem->LoadXmlFile(szTeamXML.c_str())))
		{
			g_D6Core->pSystem->Warning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING,
				VALIDATOR_FLAG_FILE, szTeamXML.c_str(), "Failed to load Team Definition for \'%s\'", szTeam);
			return TEAMID_INVALID;
		}
	}

	// Create entry for team
	STeamDef TeamDef;
	TeamDef.nID = ++m_nTeamIDGen;
	TeamDef.nSpawnGroupID = 0;
	TeamDef.szXML = szTeamXML;

	// Extract attributes
	{
		// Name
		XmlString szAttrName = "";
		pRootNode->getAttr("Name", szAttrName);
		TeamDef.szName = szAttrName;
	}
	{
		// Long Name
		XmlString szAttrLongName = "";
		pRootNode->getAttr("LongName", szAttrLongName);
		TeamDef.szLongName = szAttrLongName;
	}
	{
		// Script
		XmlString szScript = "";
		pRootNode->getAttr("Script", szScript);
		TeamDef.szScript = szScript;
	}

	// Get harvester info
	XmlNodeRef pHarvesterNode = pRootNode->findChild("Harvester");
	XmlString szHarvName = "";
	float fCapacity = 0.0f;
	float fBuildTime = 0.0f;
	if (NULL != pHarvesterNode)
	{
		pHarvesterNode->getAttr("Entity", szHarvName);
		pHarvesterNode->getAttr("Capacity", fCapacity);
		pHarvesterNode->getAttr("BuildTime", fBuildTime);
	}
	TeamDef.DefHarvester.fCapacity = fCapacity;
	TeamDef.DefHarvester.fBuildTime = fBuildTime;
	TeamDef.DefHarvester.szEntityName = szHarvName;

	// TODO Load script

	// Create entry and return ID
	CryLog("[TeamManager] Created team \'%s\' (%u)", TeamDef.szName.c_str(), TeamDef.nID);
	m_TeamMap[TeamDef.nID] = TeamDef;
	return TeamDef.nID;
}

////////////////////////////////////////////////////
void CTeamManager::RemoveTeam(TeamID nTeamID)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	// Remove all harvesters
	_RemoveTeamsHarvesters(itEntry->first);

	CryLog("[TeamManager] Removed team \'%s\' (%d)", itEntry->second.szName.c_str(), itEntry->first);

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
bool CTeamManager::SetTeam(TeamID nTeamID, EntityId nEntityID)
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
void CTeamManager::SetTeamDefaultSpawnGroup(TeamID nTeamID, EntityId nSpawnGroupId)
{
	// Find team entry
	TeamMap::iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return;

	// Set it
	itEntry->second.nSpawnGroupID = nSpawnGroupId;
}

////////////////////////////////////////////////////
EntityId CTeamManager::GetTeamDefaultSpawnGroup(TeamID nTeamID) const
{
	// Find team entry
	TeamMap::const_iterator itEntry = m_TeamMap.find(nTeamID);
	if (itEntry == m_TeamMap.end()) return 0;
	return itEntry->second.nSpawnGroupID;
}

////////////////////////////////////////////////////
void CTeamManager::RemoveTeamDefaultSpawnGroup(TeamID nTeamID)
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

////////////////////////////////////////////////////
bool CTeamManager::IsValidTeam(TeamID nID) const
{
	TeamMap::const_iterator itTeam = m_TeamMap.find(nID);
	return (itTeam != m_TeamMap.end());
}
bool CTeamManager::IsValidTeam(char const* szName) const
{
	for (TeamMap::const_iterator itTeam = m_TeamMap.begin(); itTeam != m_TeamMap.end();
		itTeam++)
	{
		if (itTeam->second.szName == szName)
			return true;
	}
	return false;
}

////////////////////////////////////////////////////
HarvesterID CTeamManager::CreateTeamHarvester(TeamID nID, bool bUseFactory, Vec3 const& vPos)
{
	// Find team
	TeamMap::iterator itTeam = m_TeamMap.find(nID);
	if (itTeam == m_TeamMap.end()) return HARVESTERID_INVALID;

	// Create new harvester profile for it
	STeamHarvesterDef harvester = itTeam->second.DefHarvester;
	harvester.ucFlags = (true == bUseFactory ? HARVESTER_USEFACTORY : 0);
	harvester.vCreateAt = vPos;
	harvester.nID = ++m_nHarvIDGen;

	// Create the entity
	if (true == _CreateHarvesterEntity(harvester))
	{
		CryLog("[TeamManager] Created harvester %d for Team \'%s\' (%u)", harvester.nID,
			GetTeamName(nID), nID);
	}
	else
	{
		GameWarning("[TeamManager] Failed to create harvester for team \'%s\' (%u) [Entity = \'%s\']",
			GetTeamName(nID), nID, harvester.szEntityName);
	}

	// Add to map
	itTeam->second.Harvesters[harvester.nID] = harvester;
	return harvester.nID;
}

////////////////////////////////////////////////////
bool CTeamManager::_CreateHarvesterEntity(STeamHarvesterDef &def)
{
	// Create entity
	SEntitySpawnParams spawnParams;
	IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
	pClassRegistry->IteratorMoveFirst();
	if (NULL != (spawnParams.pClass = pClassRegistry->FindClass(def.szEntityName)))
		spawnParams.sName = spawnParams.pClass->GetName();

	// TODO Go through factory
	if (HARVESTER_USEFACTORY != (def.ucFlags&HARVESTER_USEFACTORY))
		spawnParams.vPosition = def.vCreateAt;

	IEntity *pNewEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);
	if (NULL == pNewEntity) return false;

	// Set ID
	def.nEntityID = pNewEntity->GetId();
	def.ucFlags |= HARVESTER_ALIVE;

	// Get the vehicle and turn its engine on
	IVehicle *pVehicle = g_D6Core->pD6Game->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(def.nEntityID);
	IVehicleMovement *pVehicleMovement = (NULL == pVehicle ? NULL : pVehicle->GetMovement());
	if (NULL != pVehicleMovement)
	{
		pVehicleMovement->DisableEngine(false);
		pVehicleMovement->EnableMovementProcessing(true);
		pVehicleMovement->StartEngine(0);
	}

	return true;
}

////////////////////////////////////////////////////
bool CTeamManager::GetTeamHarvesterInfo(TeamID nID, HarvesterList& list)
{
	// Find team
	TeamMap::const_iterator itTeam = m_TeamMap.find(nID);
	if (itTeam == m_TeamMap.end()) return false;

	// Set it
	list = itTeam->second.Harvesters;
	return true;
}

////////////////////////////////////////////////////
bool CTeamManager::CheckHarvesterFlag(TeamID nID, HarvesterID nHarvesterID, int nFlag)
{
	// Must be valid flag (power of 2)
	if (0 != (nFlag & (nFlag - 1)))
		return false;

	// Get harvester info
	HarvesterList list;
	if (false == GetTeamHarvesterInfo(nID, list))
		return false;

	// Find the harvester
	HarvesterList::const_iterator itHarv = list.find(nHarvesterID);
	if (itHarv == list.end())
		return false;

	// Check the flag
	return (nFlag == (itHarv->second.ucFlags & nFlag));
}

////////////////////////////////////////////////////
bool CTeamManager::SetHarvesterFlag(TeamID nID, HarvesterID nHarvesterID, int nFlag, bool bOn)
{
	// Must be valid flag (power of 2)
	if (0 != (nFlag & (nFlag - 1)))
		return false;

	// Get harvester info
	HarvesterList list;
	if (false == GetTeamHarvesterInfo(nID, list))
		return false;

	// Find the harvester
	HarvesterList::iterator itHarv = list.find(nHarvesterID);
	if (itHarv == list.end())
		return false;

	// Set flag
	if (true == bOn)
		itHarv->second.ucFlags |= nFlag;
	else
		itHarv->second.ucFlags &= (~nFlag);
	return true;
}