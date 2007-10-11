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
};