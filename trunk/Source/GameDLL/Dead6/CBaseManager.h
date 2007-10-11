////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
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
#include "IBuildingController.h"

class CBaseManager : public IBaseManager
{
	// Class name repository
	typedef std::map<string, BuildingClassID> ClassRepository;
	BuildingClassID m_nClassNameIDGen;
	ClassRepository m_ClassNameList;

	// Building controllers
	typedef std::map<BuildingGUID, IBuildingController*> ControllerList;
	ControllerList m_ControllerList;

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
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(class ICrySizer *s);

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
		XmlNodeRef pAttr, struct IBuildingController **ppController = NULL);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Clears all loaded BCs and prepares
	//	for new controller definitions
	//
	// Note: Should be called at the start of a level
	//	load
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// ResetControllers
	//
	// Purpose: Reset all loaded building controllers
	//
	// Note: Should be used whenever the "game" is reset,
	//	like in the Editor
	////////////////////////////////////////////////////
	virtual void ResetControllers(void);
};

#endif //_D6C_CBASEMANAGER_H_