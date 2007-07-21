////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CBaseManager.h
//
// Purpose: Monitors building logic on the field
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CBASEMANAGER_H_
#define _D6C_CBASEMANAGER_H_

#include "IBaseManager.h"

class CBaseManager : public IBaseManager
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBaseManager(void);
private:
	CBaseManager(CBaseManager const&) {}
	CBaseManager& operator =(CBaseManager const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBaseManager(void);

public:
	////////////////////////////////////////////////////
	// SetTeamManager
	//
	// Purpose: Set the team manager
	//
	// In:	pTM - Team manager to use
	////////////////////////////////////////////////////
	virtual void SetTeamManager(ITeamManager const* pTM);

	////////////////////////////////////////////////////
	// GetTeamManager
	//
	// Purpose; Returns the team manager in use
	////////////////////////////////////////////////////
	virtual ITeamManager const* GetTeamManager(void) const;

	////////////////////////////////////////////////////
	// DefineBuildingClass
	//
	// Purpose: Define a building class for use
	//
	// In:	szName - Building class name
	//		szScript - Building script to use
	//
	// Returns Building CLass ID or BC_INVALID on error
	////////////////////////////////////////////////////
	virtual BuildingClassID CreateTeam(char const* szName, char const* szScript);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Remove all team definitions
	//
	// Note: Should be called when a level is loading
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// GetBuildingClassByName
	//
	// Purpose: Find and returns a building class'
	//	definition by its name
	//
	// In:	szName - Building Class name
	//
	// Returns team's definition or NULL on error
	//
	// Note: Slower, use ID if you have it!
	////////////////////////////////////////////////////
	virtual SBuildingClassDef const* GetBuildingClassByName(char const* szName) const;

	////////////////////////////////////////////////////
	// GetBuildingClassByID
	//
	// Purpose: Find and return a building class'
	//	definition by its ID
	//
	// In:	nID - Building Class ID
	//
	// Returns team's definition or NULL on error
	////////////////////////////////////////////////////
	virtual SBuildingClassDef const* GetBuildingClassByID(BuildingClassID const& nID) const;
};

#endif //_D6C_CBASEMANAGER_H_