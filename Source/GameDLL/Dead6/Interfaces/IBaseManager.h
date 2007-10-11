////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
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
// Make a building GUID quickly
////////////////////////////////////////////////////
#define MAKE_BUILDING_GUID(TEAMID, CLASSID) ((((TeamID)TEAMID)<<16)|(BuildingClassID)CLASSID)


////////////////////////////////////////////////////
//	Building class definition structure
//struct SBuildingClassDef
//{
//	// Building class ID
//	BuildingClassID nID;
//
//	// Building class name
//	string szName;
//
//	// Building class script file
//	string szScript;
//
//	// XML file
//	string szXML;
//};


////////////////////////////////////////////////////
struct IBaseManager
{
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~IBaseManager(void) {}

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(class ICrySizer *s) = 0;

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
	// CreateBuildingController
	//
	// Purpose: Create a building controller
	//
	// In:	szTeam - Team that owns the controller
	//		szName - Class name to use for controller
	//		pAttr - XML node containing attributes for it
	//
	// Out:	ppController - Optional controller catcher
	//
	// Returns the building's GUID or GUID_INVALID on error
	////////////////////////////////////////////////////
	virtual BuildingGUID CreateBuildingController(char const* szTeam, char const* szName,
		XmlNodeRef pAttr, struct IBuildingController **ppController = NULL) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Clears all loaded BCs and prepares
	//	for new controller definitions
	//
	// Note: Should be called at the start of a level
	//	load
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

	////////////////////////////////////////////////////
	// ResetControllers
	//
	// Purpose: Reset all loaded building controllers
	//
	// Note: Should be used whenever the "game" is reset,
	//	like in the Editor
	////////////////////////////////////////////////////
	virtual void ResetControllers(void) = 0;
};

#endif //_D6C_IBASEMANAGER_H_