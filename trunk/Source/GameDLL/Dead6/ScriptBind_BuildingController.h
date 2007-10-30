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
	virtual void AttachTo(IBuildingController *pController);

public:
	////////////////////////////////////////////////////
	// GetHealth
	//
	// Purpose: Get the health of the building
	////////////////////////////////////////////////////
	virtual int GetHealth(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// IsAlive
	//
	// Purpose: Returns TRUE if building is alive
	////////////////////////////////////////////////////
	virtual int IsAlive(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// HasPower
	//
	// Purpose: Returns TRUE if building has power
	////////////////////////////////////////////////////
	virtual int HasPower(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// SetPower
	//
	// Purpose: Set the power of the building
	//
	// In:	bPower - TRUE to give it power, FALSE to
	//			take it away
	////////////////////////////////////////////////////
	virtual int SetPower(IFunctionHandler *pH, bool bPower);

	////////////////////////////////////////////////////
	// AddEventListener
	//
	// Purpose: Add the given entity to the entity event
	//	listener on the controller
	//
	// In:	nEntityID - Entity Id to add
	//
	// Returns '1' if succeeded, nil if not
	////////////////////////////////////////////////////
	virtual int AddEventListener(IFunctionHandler *pH, ScriptHandle nEntityID);

	////////////////////////////////////////////////////
	// RemoveEventListener
	//
	// Purpose: Remove the given entity from the entity event
	//	listener on the controller
	//
	// In:	nEntityID - Entity Id to add
	////////////////////////////////////////////////////
	virtual int RemoveEventListener(IFunctionHandler *pH, ScriptHandle nEntityID);

	////////////////////////////////////////////////////
	// GetGUID
	//
	// Purpose: Return building's GUID
	////////////////////////////////////////////////////
	virtual int GetGUID(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// GetClass
	//
	// Purpose: Return building's class ID
	////////////////////////////////////////////////////
	virtual int GetClass(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Return building's class name
	////////////////////////////////////////////////////
	virtual int GetClassName(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Return building's team ID
	////////////////////////////////////////////////////
	virtual int GetTeam(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Return building's team name
	////////////////////////////////////////////////////
	virtual int GetTeamName(IFunctionHandler *pH);
};

#endif //_D6C_SCRIPTBIND_BUILDINGCONTROLLER_H_