////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
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

class CTeamManager : public ITeamManager
{
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
	// Reset
	//
	// Purpose: Remove all team definitions
	//
	// Note: Should be called when a level is loading
	////////////////////////////////////////////////////
	virtual void Reset(void);

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