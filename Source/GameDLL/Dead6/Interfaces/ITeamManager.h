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

// Harvester flags
#define HARVESTER_ALIVE			(0x01)		// Set if harvester is alive
#define HARVESTER_HASLOAD		(0x02)		// Set if harvester is carrying a load
#define HARVESTER_ISLOADING		(0x04)		// Set if harvester is loading up
#define HARVESTER_ISUNLOADING	(0x08)		// Set if harvester is unloading
#define HARVESTER_USEFACTORY	(0x10)		// Set if harvester should be created through the factory

// Harvester definition
typedef unsigned short HarvesterID;
typedef std::list<int> TSignalQueue;
#define HARVESTERID_INVALID 0
struct STeamHarvesterDef
{
	// Properties
	float fCapacity;
	float fBuildTime;
	float fLoadRate;
	float fUnloadRate;

	// When loading/unloading began
	float fPayloadTimer;

	// How much tiberium is in the tank
	float fPayload;

	// Flags
	unsigned char ucFlags;

	// Creation point
	Vec3 vCreateAt;
	float fCreateDir;

	// Entity ID
	TeamID nTeamID;
	HarvesterID nID;
	EntityId nEntityID;

	// Entity name
	string szEntityName;

	// Signal queue
	TSignalQueue SignalQueue;

	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	STeamHarvesterDef(void);

	////////////////////////////////////////////////////
	// CheckFlag
	//
	// Purpose: Returns TRUE if the given flag is set
	//
	// In:	nFlag - Flag to check
	//
	// Returns TRUE if flag is set, FALSE if not
	////////////////////////////////////////////////////
	virtual bool CheckFlag(int nFlag);

	////////////////////////////////////////////////////
	// SetFlag
	//
	// Purpose: Set the flag specified
	//
	// In:	nFlag - Flag to set
	//		bOn - TRUE to turn it on, FALSE to clear it
	//
	// Returns TRUE on success, FALSE on failure
	////////////////////////////////////////////////////
	virtual bool SetFlag(int nFlag, bool bOn = true);
};
typedef std::map<HarvesterID, STeamHarvesterDef*> HarvesterList;

// Team flags
enum ETeamFlags
{
	TEAM_FLAG_ISPLAYERTEAM		= 0x01,		// Set if player can be on this team, or strictly AI
};

// Team definition structure
struct STeamDef
{
	// Team ID
	TeamID nID;

	// Spawn group ID
	EntityId nSpawnGroupID;

	// Team flags (see ETeamFlags)
	unsigned int nFlags;

	// Team name
	string szName;
	string szLongName;

	// Script file
	string szScript;

	// Player list
	TeamPlayerList PlayerList;

	// Harvester list
	STeamHarvesterDef DefHarvester;
	HarvesterList Harvesters;
};
typedef std::map<TeamID, STeamDef>	TeamMap;
typedef std::map<TeamID, TeamPlayerList> TeamPlayerMap;
typedef std::map<int, TeamID> ChannelMap;

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
	// ResetGame
	//
	// Purpose: Called when the game is reset, such as
	//	when the editor game starts up
	//
	// In:	bGameStart - TRUE if game is starting,
	//	FALSE if game is stopping
	////////////////////////////////////////////////////
	virtual void ResetGame(bool bGameStart) = 0;
	
	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update the teams
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags) = 0;

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
	// LoadTeams
	//
	// Purpose: Load team definitions contained in an
	//	XML node
	//
	// In:	pNode - XML node to extract from
	//
	// Returns TRUE on success or FALSE on error
	//
	// Note: Used in conjunction with CNCRules parsing
	////////////////////////////////////////////////////
	virtual bool LoadTeams(XmlNodeRef pNode) = 0;

	////////////////////////////////////////////////////
	// CreateTeam
	//
	// Purpose: Create a team definition
	//
	// In:	pTeamNode - XML Node containing team data
	//
	// Returns ID of the team created or TEAMID_INVALID
	//	on error
	////////////////////////////////////////////////////
	virtual TeamID CreateTeam(XmlNodeRef pTeamNode) = 0;

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
	virtual bool IsPlayerTeam(TeamID nID) const = 0;
	virtual bool IsPlayerTeam(char const* szName) const = 0;

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
		Vec3 const& vPos, float fDir, STeamHarvesterDef **ppDef = NULL) = 0;

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
	virtual bool GetTeamHarvesterInfo(TeamID nID, HarvesterList& list) = 0;

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
	virtual STeamHarvesterDef *GetTeamHarvester(TeamID nID, HarvesterID nHarvesterID) = 0;

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
	virtual bool RemakeTeamHarvester(STeamHarvesterDef *pDef, bool bLeaveEntity = false) = 0;

	////////////////////////////////////////////////////
	// GetChannelTeam
	//
	// Purpose: Get the team that owns the given channel
	//
	// In:	nChannelID - Channel ID
	//
	// Returns id of Team
	////////////////////////////////////////////////////
	virtual TeamID GetChannelTeam(int nChannelID) const = 0;

	////////////////////////////////////////////////////
	// GetTeamChannelCount
	//
	// Purpose: Returns how many channels a team has
	//
	// In:	nTeamID - team ID
	//		bInGame - TRUE to only count those in the game
	////////////////////////////////////////////////////
	virtual int GetTeamChannelCount(TeamID nTeamID, bool bInGame = false) const = 0;

	////////////////////////////////////////////////////
	// SetEditorTeam
	//
	// Purpose: Set which team you are on in the editor
	//
	// In:	nTeamID - team ID
	//		bResetNow - Reset local player's team now
	////////////////////////////////////////////////////
	virtual void SetEditorTeam(TeamID nTeamID, bool bResetNow = true) = 0;

	////////////////////////////////////////////////////
	// SetTeamCredits
	//
	// Purpose: Set everyone on the team's credits
	//
	// In:	nTeamID - team ID
	//		nAmount - Amount to set
	////////////////////////////////////////////////////
	virtual void SetTeamCredits(TeamID nTeamID, unsigned int nAmount) const = 0;

	////////////////////////////////////////////////////
	// GiveTeamCredits
	//
	// Purpose: Give everyone on the team credits
	//
	// In:	nTeamID - team ID
	//		nAmount - Amount to give
	////////////////////////////////////////////////////
	virtual void GiveTeamCredits(TeamID nTeamID, unsigned int nAmount) const = 0;

	////////////////////////////////////////////////////
	// TakeTeamCredits
	//
	// Purpose: Take from everyone on the team's credits
	//
	// In:	nTeamID - team ID
	//		nAmount - Amount to take
	////////////////////////////////////////////////////
	virtual void TakeTeamCredits(TeamID nTeamID, unsigned int nAmount) const = 0;
};

#endif //_D6C_ITEAMMANAGER_H_