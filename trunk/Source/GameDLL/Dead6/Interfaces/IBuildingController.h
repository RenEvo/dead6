////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// IBuildingController.h
//
// Purpose: Interface object
//	Describes a building controller which contains
//	a logical structure and is interfaced
//
// File History:
//	- 8/11/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_IBUILDINGCONTROLLER_H_
#define _D6C_IBUILDINGCONTROLLER_H_

// Controller update flags - EControllerUpdateFlags
enum EControllerUpdateFlags
{
	CUF_CHECKVISIBILITY		= 0x01,		// Check if player is looking at controller
};

// Building state flags - EControllerStateFlags
enum EControllerStateFlags
{
	CSF_ISVISIBLE			= 0x01,		// Set if building is visible by the player
};

struct IBuildingController
{
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~IBuildingController(void) {}

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
	// Purpose: Initialize the controller
	//
	// In:	nGUID - Building controller's GUID
	//		fHealth - Initial health
	////////////////////////////////////////////////////
	virtual void Initialize(unsigned int nGUID, float fHealth) = 0;

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: Shut the controller down
	////////////////////////////////////////////////////
	virtual void Shutdown(void) = 0;
	
	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update the controller
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	//		nControllerUpdateFlags - Controller-specific
	//			update flags (see EControllerUpdateFlags)
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags, unsigned int nControllerUpdateFlags) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Reset the controller back to its
	//	initial state
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

	////////////////////////////////////////////////////
	// BeforeValidate
	//
	// Purpose: Prepare the controller for validation
	//
	// Returns TRUE if validation should proceed, or
	//	FALSE if it should be skipped
	////////////////////////////////////////////////////
	virtual bool BeforeValidate(void) = 0;

	////////////////////////////////////////////////////
	// Validate
	//
	// Purpose: Validate the controller by checking for
	//	interfaces to it
	////////////////////////////////////////////////////
	virtual void Validate(void) = 0;

	////////////////////////////////////////////////////
	// LoadFromXml
	//
	// Purpose: Define the building controller's attributes
	//	from the given XML file
	//
	// In:	szName - Class name of the building
	//
	// Returns TRUE if building controller is ready
	////////////////////////////////////////////////////
	virtual bool LoadFromXml(char const* szName) = 0;

	////////////////////////////////////////////////////
	// GetGUID
	//
	// Purpose: Return the controller's GUID
	////////////////////////////////////////////////////
	virtual unsigned int GetGUID(void) const = 0;

	////////////////////////////////////////////////////
	// GetScriptTable
	//
	// Purpose: Return the script table attatched to
	//	the controller's script
	////////////////////////////////////////////////////
	virtual struct IScriptTable *GetScriptTable(void) const = 0;

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Returns owning team's ID or TEAMID_NOTEAM
	//	if there is no team
	////////////////////////////////////////////////////
	virtual unsigned short GetTeam(void) const = 0;

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Returns owning team's name
	////////////////////////////////////////////////////
	virtual char const* GetTeamName(void) const = 0;

	////////////////////////////////////////////////////
	// GetClass
	//
	// Purpose: Returns building class or BC_INVALID if
	//	there is no class
	////////////////////////////////////////////////////
	virtual unsigned short GetClass(void) const = 0;

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Returns owning class' name
	////////////////////////////////////////////////////
	virtual char const* GetClassName(void) const = 0;

	////////////////////////////////////////////////////
	// AddInterface
	//
	// Purpose: Mark an entity as an interface to this
	//	controller
	//
	// In:	pEntity - Entity to use
	//
	// Returns TRUE if entity is acting as an interface
	//	to it now or FALSE if not
	//
	// Note: Sets the ENTITY_FLAG_ISINTERFACE on the enemy
	//	so the same entity cannot interface more than one
	//	building at the same time!
	////////////////////////////////////////////////////
	virtual bool AddInterface(struct IEntity *pEntity) = 0;

	////////////////////////////////////////////////////
	// RemoveInterface
	//
	// Purpose: Remove an entity as an interface to this
	//	controller
	//
	// In:	pEntity - Entity to remove
	////////////////////////////////////////////////////
	virtual void RemoveInterface(struct IEntity *pEntity) = 0;

	////////////////////////////////////////////////////
	// IsVisible
	//
	// Purpose: Returns TRUE if the controller is visible
	//	to the player (i.e. player is focused on an
	//	interface to it)
	////////////////////////////////////////////////////
	virtual bool IsVisible(void) const = 0;
};

#endif //_D6C_IBUILDINGCONTROLLER_H_