////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_BuildingController.h
//
// Purpose: Binding script object for the building
//	controllers
//
// File History:
//	- 8/12/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_SCRIPTBIND_BUILDINGCONTROLLER_H_
#define _D6C_SCRIPTBIND_BUILDINGCONTROLLER_H_

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IBuildingController;
class CScriptBind_BuildingController : public CScriptableBase
{
	ISystem *m_pSystem;
	IScriptSystem *m_pSS;
	IGameFramework *m_pGameFW;

	////////////////////////////////////////////////////
	// RegisterGlobals
	//
	// Purpose: Called to register global variables
	////////////////////////////////////////////////////
	void RegisterGlobals(void);

	////////////////////////////////////////////////////
	// RegisterMethods
	//
	// Purpose: Called to register function methods
	////////////////////////////////////////////////////
	void RegisterMethods(void);

	////////////////////////////////////////////////////
	// GetController
	//
	// Purpose: Return the controller
	//
	// In:	pH - Function handler to get controller from
	////////////////////////////////////////////////////
	IBuildingController *GetController(IFunctionHandler *pH);

public:
	////////////////////////////////////////////////////
	// Constructor
	//
	// In:	pSystem - System object
	//		pGameFW - Game frame work object
	////////////////////////////////////////////////////
	CScriptBind_BuildingController(ISystem *pSystem, IGameFramework *pGameFW);
private:
	CScriptBind_BuildingController(CScriptBind_BuildingController const&) {}
	CScriptBind_BuildingController& operator =(CScriptBind_BuildingController const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CScriptBind_BuildingController(void);

	////////////////////////////////////////////////////
	// AttachTo
	//
	// Purpose: Bind to a controller
	//
	// In:	pController - Controller to be bound
	////////////////////////////////////////////////////
	void AttachTo(IBuildingController *pController);

public:
	// TODO Functions
	int Test(IFunctionHandler *pH, int nTest);
};

#endif //_D6C_SCRIPTBIND_BUILDINGCONTROLLER_H_