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

	// Team map
	TeamMap m_TeamMap;

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
	//
	// In:	pGame - Game class
	////////////////////////////////////////////////////
	virtual void Initialize(CD6Game *pGame);

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
	// CreateTeam
	//
	// Purpose: Create and initialize a team
	//
	// In:	szName - Team name
	//		szScript - Team script to use
	//
	// Returns Team ID or TEAMID_INVALID on error
	////////////////////////////////////////////////////
	virtual TeamID CreateTeam(char const* szName, char const* szScript);

	////////////////////////////////////////////////////
	// RemoveTeam
	//
	// Purpose: Remove a team entry
	//
	// In:	szName - Team name
	//		nID - Team ID
	////////////////////////////////////////////////////
	virtual void RemoveTeam(char const* szName);
	virtual void RemoveTeam(TeamID const& nID);

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
	virtual STeamDef const* GetTeamByName(char const* szName) const;

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
	virtual STeamDef const* GetTeamByID(TeamID const& nID) const;
};

#endif //_D6C_CTEAMMANAGER_H_