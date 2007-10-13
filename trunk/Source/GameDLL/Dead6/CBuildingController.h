////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CBuildingController.h
//
// Purpose: Building controller which contains
//	a logical structure and is interfaced
//
// File History:
//	- 8/11/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CBUILDINGCONTROLLER_H_
#define _D6C_CBUILDINGCONTROLLER_H_

#include "IBuildingController.h"
#include "IBaseManager.h"

class CBuildingController : public IBuildingController
{
	// Building's GUID
	BuildingGUID m_nGUID;

	// Building's health
	float m_fHealth, m_fInitHealth, m_fMaxHealth;

	// Script object
	IScriptSystem *m_pSS;
	SmartScriptTable m_pScriptTable;

	// Name of the building
	string m_szName;

	// Building's script
	string m_szScript;

	// Interface map
	typedef std::vector<IEntity*> InterfaceMap;
	InterfaceMap m_Interfaces;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBuildingController(void);
private:
	CBuildingController(CBuildingController const&) {}
	CBuildingController& operator =(CBuildingController const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBuildingController(void);

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
	// Purpose: Initialize the controller
	//
	// In:	nGUID - Building controller's GUID
	//		fHealth - Initial health
	////////////////////////////////////////////////////
	virtual void Initialize(BuildingGUID nGUID, float fHealth);

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: Shut the controller down
	////////////////////////////////////////////////////
	virtual void Shutdown(void);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Reset the controller back to its
	//	initial state
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// BeforeValidate
	//
	// Purpose: Prepare the controller for validation
	//
	// Returns TRUE if validation should proceed, or
	//	FALSE if it should be skipped
	////////////////////////////////////////////////////
	virtual bool BeforeValidate(void);

	////////////////////////////////////////////////////
	// Validate
	//
	// Purpose: Validate the controller by checking for
	//	interfaces to it
	////////////////////////////////////////////////////
	virtual void Validate(void);

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
	virtual bool LoadFromXml(char const* szName);

	////////////////////////////////////////////////////
	// GetGUID
	//
	// Purpose: Return the controller's GUID
	////////////////////////////////////////////////////
	virtual BuildingGUID GetGUID(void) const;

	////////////////////////////////////////////////////
	// GetScriptTable
	//
	// Purpose: Return the script table attatched to
	//	the controller's script
	////////////////////////////////////////////////////
	virtual struct IScriptTable *GetScriptTable(void) const;

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Returns owning team's ID or TEAMID_NOTEAM
	//	if there is no team
	////////////////////////////////////////////////////
	virtual TeamID GetTeam(void) const;

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Returns owning team's name
	////////////////////////////////////////////////////
	virtual char const* GetTeamName(void) const;

	////////////////////////////////////////////////////
	// GetClass
	//
	// Purpose: Returns building class or BC_INVALID if
	//	there is no class
	////////////////////////////////////////////////////
	virtual BuildingClassID GetClass(void) const;

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Returns owning class' name
	////////////////////////////////////////////////////
	virtual char const* GetClassName(void) const;

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
	virtual bool AddInterface(struct IEntity *pEntity);

	////////////////////////////////////////////////////
	// RemoveInterface
	//
	// Purpose: Remove an entity as an interface to this
	//	controller
	//
	// In:	pEntity - Entity to remove
	////////////////////////////////////////////////////
	virtual void RemoveInterface(struct IEntity *pEntity);
};

#endif //_D6C_CBUILDINGCONTROLLER_H_