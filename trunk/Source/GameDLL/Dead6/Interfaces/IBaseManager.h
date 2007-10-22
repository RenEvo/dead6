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
#define GET_TEAM_FROM_GUID(GUID) ((GUID&0xFFFF0000)>>16)
#define GET_CLASS_FROM_GUID(GUID) (GUID&0x0000FFFF)

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
	// Update
	//
	// Purpose: Update the controllers
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
	// ResetGame
	//
	// Purpose: Called when the game is reset, such as
	//	when the editor game starts up
	////////////////////////////////////////////////////
	virtual void ResetGame(void) = 0;

	////////////////////////////////////////////////////
	// Validate
	//
	// Purpose: Validates all controllers by (re)setting
	//	any interfaces to them and checks for errors
	//
	// In:	nGUID - Controller GUID to validate or GUID_INVALID
	//	to validate all controlles
	//
	// Note: Should be called at least once after level
	//	has loaded and when game is reset
	////////////////////////////////////////////////////
	virtual void Validate(BuildingGUID nGUID = GUID_INVALID) = 0;

	////////////////////////////////////////////////////
	// FindBuildingController
	//
	// Purpose: Find the building controller with the
	//	given GUID
	//
	// In:	nGUID - GUID to use
	//
	// Returns building controller's object or NULL on
	//	error
	////////////////////////////////////////////////////
	virtual IBuildingController *FindBuildingController(BuildingGUID nGUID) const = 0;

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Returns the name of the given class
	//
	// In:	nClassID - ID of the class
	////////////////////////////////////////////////////
	virtual const char *GetClassName(BuildingClassID nClassID) const = 0;

	////////////////////////////////////////////////////
	// GetClassId
	//
	// Purpose: Returns the ID of the class with the
	//	given ID
	//
	// In:	szName - Name to search for
	////////////////////////////////////////////////////
	virtual BuildingClassID GetClassId(char const* szName) const = 0;

	////////////////////////////////////////////////////
	// IsValidClass
	//
	// Purpose: Returns TRUE if the specified class ID or
	//	name is valid
	//
	// In:	nID - Class ID
	//		szName - Name of the class
	//
	// Note: Using the name is slower than the ID!
	////////////////////////////////////////////////////
	virtual bool IsValidClass(BuildingClassID nID) const = 0;
	virtual bool IsValidClass(char const* szName) const = 0;

	////////////////////////////////////////////////////
	// GenerateGUID
	//
	// Purpose: Generates a Building GUID given the
	//	team and class names
	//
	// In:	szTeam - Team name
	//		szClass - Class name
	//
	// Returns building GUID or GUID_INVALID on error
	////////////////////////////////////////////////////
	virtual BuildingGUID GenerateGUID(char const* szTeam, char const* szClass) const = 0;
};

#endif //_D6C_IBASEMANAGER_H_