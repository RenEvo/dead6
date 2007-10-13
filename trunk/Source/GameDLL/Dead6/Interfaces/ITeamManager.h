////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ITeamManager.h
//
// Purpose: Interface object
//	Describes a team manager for monitoring team
//	fluctuations and callback scripts
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_ITEAMMANAGER_H_
#define _D6C_ITEAMMANAGER_H_

#include "Gamerules.h"

// Team ID
//	Placement in GUID: (0xFFFF0000)
//	Total possible teams: 65535
typedef unsigned short TeamID;
#define TEAMID_INVALID	(0xFFFF)
#define TEAMID_NOTEAM	(0x0000)

typedef CGameRules::TPlayers TeamPlayerList;

////////////////////////////////////////////////////
//	Team definition structure
struct STeamDef
{
	// Team ID
	TeamID nID;

	// Spawn group ID
	EntityId nSpawnGroupID;

	// Team name
	string szName;
	string szLongName;

	// Script file
	string szScript;

	// XML file
	string szXML;

	// Player list
	TeamPlayerList PlayerList;
};
typedef std::map<TeamID, STeamDef>	TeamMap;
typedef std::map<TeamID, TeamPlayerList> TeamPlayerMap;

////////////////////////////////////////////////////
struct ITeamManager
{
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~ITeamManager(void) {}

	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: One-time initialization at the start
	////////////////////////////////////////////////////
	virtual void Initialize(void) = 0;

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: One-time clean up at the end
	////////////////////////////////////////////////////
	virtual void Shutdown(void) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Clears all loaded teams and prepares
	//	for new team definitions
	//
	// Note: Should be called at the start of a level
	//	load
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s) = 0;

	////////////////////////////////////////////////////
	// PostInitClient
	//
	// Purpose: Called when the local client has finished
	//	loading for net synch issues
	//
	// In:	nChannelID - Network channel ID
	////////////////////////////////////////////////////
	virtual void PostInitClient(int nChannelID) const = 0;

	////////////////////////////////////////////////////
	// CmdDebugTeams
	//
	// Purpose: Console command used to debug teams
	//
	// In:	pArgs - Console command arguments
	//
	// Note: Delegated from GameRules
	////////////////////////////////////////////////////
	virtual void CmdDebugTeams(struct IConsoleCmdArgs *pArgs) = 0;

	////////////////////////////////////////////////////
	// CmdDebugObjectives
	//
	// Purpose: Console command used to debug team
	//	objectives
	//
	// In:	pArgs - Console command arguments
	//		status - HUD Objectives status
	//
	// Note: Delegated from GameRules
	////////////////////////////////////////////////////
	virtual void CmdDebugObjectives(struct IConsoleCmdArgs *pArgs, const char **status) = 0;

	////////////////////////////////////////////////////
	// CreateTeam
	//
	// Purpose: Create a team definition
	//
	// In:	szTeam - Team to load (looks for its .XML file)
	//
	// Returns ID of the team created or TEAMID_INVALID
	//	on error
	////////////////////////////////////////////////////
	virtual TeamID CreateTeam(char const* szTeam) = 0;

	////////////////////////////////////////////////////
	// RemoveTeam
	//
	// Purpose: Remove a team and force anyone on that
	//	team onto another.
	//
	// In:	nTeamID - ID of the team to destroy
	////////////////////////////////////////////////////
	virtual void RemoveTeam(TeamID nTeamID) = 0;

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Returns the name of the given team
	//
	// In:	nTeamID - ID of the team
	////////////////////////////////////////////////////
	virtual const char *GetTeamName(TeamID nTeamID) const = 0;

	////////////////////////////////////////////////////
	// GetTeamId
	//
	// Purpose: Returns the ID of the team with the
	//	given ID
	//
	// In:	szName - Name to search for
	////////////////////////////////////////////////////
	virtual TeamID GetTeamId(char const* szName) const = 0;

	////////////////////////////////////////////////////
	// GetTeamCount
	//
	// Purpose: Returns how many teams are defined
	////////////////////////////////////////////////////
	virtual int GetTeamCount(void) const = 0;

	////////////////////////////////////////////////////
	// GetTeamPlayerCount
	//
	// Purpose: Returns how many players belong to the
	//	given team
	//
	// In:	nTeamID - ID of the team
	//		bInGame - TRUE to count only players who
	//			are currently in the game
	////////////////////////////////////////////////////
	virtual int GetTeamPlayerCount(TeamID nTeamID, bool bInGame = false) const = 0;

	////////////////////////////////////////////////////
	// GetTeamPlayer
	//
	// Purpose: Returns the player in the given slot on
	//	the specified team.
	//
	// In:	nTeamID - ID of the team
	//		nidx - Index into the player list
	//
	// Returns the ID of the entity in the slot
	////////////////////////////////////////////////////
	virtual EntityId GetTeamPlayer(TeamID nTeamID, int nidx) = 0;

	////////////////////////////////////////////////////
	// GetTeamPlayers
	//
	// Purpose: Returns the list of players on the given
	//	team
	//
	// In:	nTeamID - ID of the team
	//
	// Out:	players - List of players
	////////////////////////////////////////////////////
	virtual void GetTeamPlayers(TeamID nTeamID, TeamPlayerList &players) = 0;

	////////////////////////////////////////////////////
	// SetTeam
	//
	// Purpose: Put a player on the given team
	//
	// In:	nTeamID - ID of the team to use
	//		nEntityID - ID of the entity to put on the
	//			team
	//
	// Returns TRUE if the team was changed
	////////////////////////////////////////////////////
	virtual bool SetTeam(TeamID nTeamID, EntityId nEntityID) = 0;

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Get the team an entity belongs to
	//
	// In:	nEntityID - ID of the entity
	//
	// Returns the ID of the team the entity belongs to
	////////////////////////////////////////////////////
	virtual TeamID GetTeam(EntityId nEntityID) const = 0;

	////////////////////////////////////////////////////
	// SetTeamDefaultSpawnGroup
	//
	// Purpose: Sets the team's default spawn group
	//
	// In:	nTeamID - ID of team
	//		nSpawnGroupId - ID of spawn group to use
	////////////////////////////////////////////////////
	virtual void SetTeamDefaultSpawnGroup(TeamID nTeamID, EntityId nSpawnGroupId) = 0;

	////////////////////////////////////////////////////
	// GetTeamDefaultSpawnGroup
	//
	// Purpose: Get the spawn group ID being used by
	//	the given team
	//
	// In:	nTeamID - ID of team to get
	//
	// Returns ID of the spawn group
	////////////////////////////////////////////////////
	virtual EntityId GetTeamDefaultSpawnGroup(TeamID nTeamID) const = 0;

	////////////////////////////////////////////////////
	// RemoveTeamDefaultSpawnGroup
	//
	// Purpose: Removes the spawn group being used by
	//	the given ID
	//
	// In:	nTeamID - ID of team
	////////////////////////////////////////////////////
	virtual void RemoveTeamDefaultSpawnGroup(TeamID nTeamID) = 0;

	////////////////////////////////////////////////////
	// RemoveDefaultSpawnGroupFromTeams
	//
	// Purpose: Remove the spawn group from any team
	//	that is using it
	//
	// In:	nSpawnGroupId - ID of spawn group to remove
	////////////////////////////////////////////////////
	virtual void RemoveDefaultSpawnGroupFromTeams(EntityId nSpawnGroupId) = 0;

	////////////////////////////////////////////////////
	// IsValidTeam
	//
	// Purpose: Returns TRUE if the specified team ID or
	//	name is valid
	//
	// In:	nID - Team ID
	//		szName - Name of the team
	//
	// Note: Using the name is slower than the ID!
	////////////////////////////////////////////////////
	virtual bool IsValidTeam(TeamID nID) const = 0;
	virtual bool IsValidTeam(char const* szName) const = 0;
};

#endif //_D6C_ITEAMMANAGER_H_