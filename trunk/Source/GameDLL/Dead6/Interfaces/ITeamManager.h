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

// Team ID
//	Placement in GUID: (0xFFFF0000)
//	Total possible teams: 65535
typedef unsigned short TeamID;
#define TEAMID_INVALID (0x0000)


////////////////////////////////////////////////////
//	Team definition structure
struct STeamDef
{
	// Team ID
	TeamID nID;

	// Team name
	string szName;

	// Script file
	string szScript;
};
typedef std::map<TeamID, STeamDef> TeamMap;

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
	//
	// In:	pGame - Game class
	////////////////////////////////////////////////////
	virtual void Initialize(class CD6Game *pGame) = 0;

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
	// CreateTeam
	//
	// Purpose: Create and initialize a team
	//
	// In:	szName - Team name
	//		szScript - Team script to use
	//
	// Returns Team ID or TEAMID_INVALID on error
	////////////////////////////////////////////////////
	virtual TeamID CreateTeam(char const* szName, char const* szScript) = 0;

	////////////////////////////////////////////////////
	// RemoveTeam
	//
	// Purpose: Remove a team entry
	//
	// In:	szName - Team name
	//		nID - Team ID
	////////////////////////////////////////////////////
	virtual void RemoveTeam(char const* szName) = 0;
	virtual void RemoveTeam(TeamID const& nID) = 0;

	////////////////////////////////////////////////////
	// GetTeamByName
	//
	// Purpose: Find and returns a team's definition by
	//	its name
	//
	// In:	szName - Team name
	//
	// Returns team's definition or NULL on error
	//
	// Note: Slower, use ID if you have it!
	////////////////////////////////////////////////////
	virtual STeamDef const* GetTeamByName(char const* szName) const = 0;

	////////////////////////////////////////////////////
	// GetTeamByID
	//
	// Purpose: Find and return a team's definition by
	//	its team ID
	//
	// In:	nID - Team ID
	//
	// Returns team's definition or NULL on error
	////////////////////////////////////////////////////
	virtual STeamDef const* GetTeamByID(TeamID const& nID) const = 0;
};

#endif //_D6C_ITEAMMANAGER_H_