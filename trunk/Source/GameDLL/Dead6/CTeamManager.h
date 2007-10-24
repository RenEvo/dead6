////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CTeamManager.h
//
// Purpose: Monitors team fluctuations and
//	callback scripts
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CTEAMMANAGER_H_
#define _D6C_CTEAMMANAGER_H_

#include "ITeamManager.h"

class CD6Game;
class CTeamManager : public ITeamManager
{
	// Game class
	CD6Game *m_pGame;

	// Team ID generator
	TeamID m_nTeamIDGen;

	// Harvester ID generator
	HarvesterID m_nHarvIDGen;

	// Team map
	TeamMap m_TeamMap;
	ChannelMap m_ChannelMap;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CTeamManager(void);
private:
	CTeamManager(CTeamManager const&) {}
	CTeamManager& operator =(CTeamManager const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CTeamManager(void);

public:
	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: One-time initialization at the start
	////////////////////////////////////////////////////
	virtual void Initialize(void);

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: One-time clean up at the end
	////////////////////////////////////////////////////
	virtual void Shutdown(void);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Clears all loaded teams and prepares
	//	for new team definitions
	//
	// Note: Should be called at the start of a level
	//	load
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// ResetGame
	//
	// Purpose: Called when the game is reset, such as
	//	when the editor game starts up
	//
	// In:	bGameStart - TRUE if game is starting,
	//	FALSE if game is stopping
	////////////////////////////////////////////////////
	virtual void ResetGame(bool bGameStart);
	
	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update the teams
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags);

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s);

	////////////////////////////////////////////////////
	// PostInitClient
	//
	// Purpose: Called when the local client has finished
	//	loading for net synch issues
	//
	// In:	nChannelID - Network channel ID
	////////////////////////////////////////////////////
	virtual void PostInitClient(int nChannelID) const;

	////////////////////////////////////////////////////
	// CmdDebugTeams
	//
	// Purpose: Console command used to debug teams
	//
	// In:	pArgs - Console command arguments
	//
	// Note: Delegated from GameRules
	////////////////////////////////////////////////////
	virtual void CmdDebugTeams(struct IConsoleCmdArgs *pArgs);

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
	virtual void CmdDebugObjectives(struct IConsoleCmdArgs *pArgs, const char **status);

	////////////////////////////////////////////////////
	// CreateTeam
	//
	// Purpose: Create a team definition
	//
	// In:	szTeam - Team to load (looks for its .XML file)
	//		pTeamNode - XML Node containing team data
	//
	// Returns ID of the team created or TEAMID_INVALID
	//	on error
	////////////////////////////////////////////////////
	virtual TeamID CreateTeam(char const* szTeam);
	virtual TeamID CreateTeam(XmlNodeRef pTeamNode);

	////////////////////////////////////////////////////
	// RemoveTeam
	//
	// Purpose: Remove a team and force anyone on that
	//	team onto another.
	//
	// In:	nTeamID - ID of the team to destroy
	////////////////////////////////////////////////////
	virtual void RemoveTeam(TeamID nTeamID);

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Returns the name of the given team
	//
	// In:	nTeamID - ID of the team
	////////////////////////////////////////////////////
	virtual const char *GetTeamName(TeamID nTeamID) const;

	////////////////////////////////////////////////////
	// GetTeamId
	//
	// Purpose: Returns the ID of the team with the
	//	given ID
	//
	// In:	szName - Name to search for
	////////////////////////////////////////////////////
	virtual TeamID GetTeamId(char const* szName) const;

	////////////////////////////////////////////////////
	// GetTeamCount
	//
	// Purpose: Returns how many teams are defined
	////////////////////////////////////////////////////
	virtual int GetTeamCount(void) const;

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
	virtual int GetTeamPlayerCount(TeamID nTeamID, bool bInGame = false) const;

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
	virtual EntityId GetTeamPlayer(TeamID nTeamID, int nidx);

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
	virtual void GetTeamPlayers(TeamID nTeamID, TeamPlayerList &players);

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
	virtual bool SetTeam(TeamID nTeamID, EntityId nEntityID);

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Get the team an entity belongs to
	//
	// In:	nEntityID - ID of the entity
	//
	// Returns the ID of the team the entity belongs to
	////////////////////////////////////////////////////
	virtual TeamID GetTeam(EntityId nEntityID) const;

	////////////////////////////////////////////////////
	// SetTeamDefaultSpawnGroup
	//
	// Purpose: Sets the team's default spawn group
	//
	// In:	nTeamID - ID of team
	//		nSpawnGroupId - ID of spawn group to use
	////////////////////////////////////////////////////
	virtual void SetTeamDefaultSpawnGroup(TeamID nTeamID, EntityId nSpawnGroupId);

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
	virtual EntityId GetTeamDefaultSpawnGroup(TeamID nTeamID) const;

	////////////////////////////////////////////////////
	// RemoveTeamDefaultSpawnGroup
	//
	// Purpose: Removes the spawn group being used by
	//	the given ID
	//
	// In:	nTeamID - ID of team
	////////////////////////////////////////////////////
	virtual void RemoveTeamDefaultSpawnGroup(TeamID nTeamID);

	////////////////////////////////////////////////////
	// RemoveDefaultSpawnGroupFromTeams
	//
	// Purpose: Remove the spawn group from any team
	//	that is using it
	//
	// In:	nSpawnGroupId - ID of spawn group to remove
	////////////////////////////////////////////////////
	virtual void RemoveDefaultSpawnGroupFromTeams(EntityId nSpawnGroupId);

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
	virtual bool IsValidTeam(TeamID nID) const;
	virtual bool IsValidTeam(char const* szName) const;

	////////////////////////////////////////////////////
	// IsPlayerTeam
	//
	// Purpose: Returns TRUE if the specified team can
	//	be used by a player (for joining rights)
	//
	// In:	nID - Team ID
	//		szName - Name of the team
	//
	// Note: Using the name is slower than the ID!
	////////////////////////////////////////////////////
	virtual bool IsPlayerTeam(TeamID nID) const;
	virtual bool IsPlayerTeam(char const* szName) const;

	////////////////////////////////////////////////////
	// CreateTeamHarvester
	//
	// Purpose: Create a harvester for the team
	//
	// In:	nID - Team ID
	//		bUseFactory - TRUE to create the harvester
	//			through the vehicle factory (if present)
	//		vPos - Where to create the harvester at if not
	//			though the vehicle factory
	//		fDir - Direction to face if not created through
	//			the vehicle factory
	//
	// Out:	ppDef - Harvester defintion (optional)
	//
	// Returns ID of harvester or HARVESTERID_INVALID
	//	on error
	////////////////////////////////////////////////////
	virtual HarvesterID CreateTeamHarvester(TeamID nID, bool bUseFactory,
		Vec3 const& vPos, float fDir, STeamHarvesterDef **ppDef = NULL);

	////////////////////////////////////////////////////
	// GetTeamHarvesterInfo
	//
	// Purpose: Return map of harvesters owned by the
	//	given team
	//
	// In:	nID - Team ID
	//
	// Out:	list - List of harvesters
	//
	// Returns TRUE on success
	////////////////////////////////////////////////////
	virtual bool GetTeamHarvesterInfo(TeamID nID, HarvesterList& list);

	////////////////////////////////////////////////////
	// GetTeamHarvester
	//
	// Purpose: Return the harvestere definition
	//
	// In:	nID - Owning team ID
	//		nHarvesterID - ID of the harvester
	//
	// Returns STeamHarvesterDef object or NULL on error
	////////////////////////////////////////////////////
	virtual STeamHarvesterDef *GetTeamHarvester(TeamID nID, HarvesterID nHarvesterID);

	////////////////////////////////////////////////////
	// RemakeTeamHarvester
	//
	// Purpose: Remakes the entity for the given harvester
	//
	// In:	pDef - Harvester definition
	//		bLeaveEntity - TRUE if old entity should be
	//			left in the world
	//
	// Returns TRUE on success, FALSE on error
	////////////////////////////////////////////////////
	virtual bool RemakeTeamHarvester(STeamHarvesterDef *pDef, bool bLeaveEntity = false);

	////////////////////////////////////////////////////
	// GetChannelTeam
	//
	// Purpose: Get the team that owns the given channel
	//
	// In:	nChannelID - Channel ID
	//
	// Returns id of Team
	////////////////////////////////////////////////////
	virtual TeamID GetChannelTeam(int nChannelID) const;

	////////////////////////////////////////////////////
	// GetTeamChannelCount
	//
	// Purpose: Returns how many channels a team has
	//
	// In:	nTeamID - team ID
	//		bInGame - TRUE to only count those in the game
	////////////////////////////////////////////////////
	virtual int GetTeamChannelCount(TeamID nTeamID, bool bInGame = false) const;

protected:
	////////////////////////////////////////////////////
	// _CreateHarvesterEntity
	//
	// Purpose: Create the harvester entity and set up
	//	its initial properties
	//
	// In:	def - Harvester definition to use
	//
	// Out:	def - Updated definition with entity id
	//
	// Return TRUE on success, FALSE otherwise
	////////////////////////////////////////////////////
	virtual bool _CreateHarvesterEntity(STeamHarvesterDef *def);

	////////////////////////////////////////////////////
	// _RemoveTeamsHarvesters
	//
	// Purpose: Remove all harvesters from the world that
	//	belong to the given team
	//
	// In:	nID - Team ID
	////////////////////////////////////////////////////
	virtual void _RemoveTeamsHarvesters(TeamID nID);
};

#endif //_D6C_CTEAMMANAGER_H_