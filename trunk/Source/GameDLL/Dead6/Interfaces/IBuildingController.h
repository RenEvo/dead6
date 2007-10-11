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
	// Reset
	//
	// Purpose: Reset the controller back to its
	//	initial state
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

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
};

#endif //_D6C_IBUILDINGCONTROLLER_H_