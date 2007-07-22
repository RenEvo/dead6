////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// IBaseManager.h
//
// Purpose: Interface object
//	Describes a base manager for monitoring building
//	logic on the field
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_IBASEMANAGER_H_
#define _D6C_IBASEMANAGER_H_

struct ITeamManager;

// Building Class ID
//	Placement in GUID: (0x0000FFFF)
//	Total possible buildings: 65535
typedef unsigned short BuildingClassID;
#define BC_INVALID (0x0000)

// Building GUID
//	Unique ID given to each building to describe its team and class
typedef unsigned int BuildingGUID;
#define GUID_INVALID (0x00000000)


////////////////////////////////////////////////////
//	Building class definition structure
struct SBuildingClassDef
{
	// Building class ID
	BuildingClassID nID;

	// Building class name
	string szName;

	// Building class script file
	string szScript;
};


////////////////////////////////////////////////////
struct IBaseManager
{
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~IBaseManager(void) {}

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
	// SetTeamManager
	//
	// Purpose: Set the team manager
	//
	// In:	pTM - Team manager to use
	////////////////////////////////////////////////////
	virtual void SetTeamManager(ITeamManager const* pTM) = 0;

	////////////////////////////////////////////////////
	// GetTeamManager
	//
	// Purpose; Returns the team manager in use
	////////////////////////////////////////////////////
	virtual ITeamManager const* GetTeamManager(void) const = 0;

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
	virtual BuildingClassID CreateTeam(char const* szName, char const* szScript) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Remove all team definitions
	//
	// Note: Should be called when a level is loading
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

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
	virtual SBuildingClassDef const* GetBuildingClassByName(char const* szName) const = 0;

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
	virtual SBuildingClassDef const* GetBuildingClassByID(BuildingClassID const& nID) const = 0;
};

#endif //_D6C_IBASEMANAGER_H_