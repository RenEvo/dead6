////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// d6platform_impl.h
//
// Purpose: Platform implementation for core setup
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_d6platform_impl_H_ 
#define _D6C_d6platform_impl_H_

#include "CBaseManager.h"
#include "CTeamManager.h"

////////////////////////////////////////////////////
// D6 Core global environment
class CD6CoreGlobalEnvironment
{
	struct ISystem						*pSystem;
	struct SSystemGlobalEnvironment		*pSystemGE;
	struct IBaseManager					*pBaseManager;
	struct ITeamManager					*pTeamManager;

	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6CoreGlobalEnvironment(void)
	{
		pSystem = NULL;
		pSystemGE = NULL;
		pBaseManager = NULL;
		pTeamManager = NULL;
	}
	CD6CoreGlobalEnvironment(CD6CoreGlobalEnvironment const&) {}
	CD6CoreGlobalEnvironment& operator =(CD6CoreGlobalEnvironment const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6CoreGlobalEnvironment(void)
	{
		SAFE_DELETE(pBaseManager);
		SAFE_DELETE(pTeamManager);
	}

	////////////////////////////////////////////////////
	// GetInstance
	//
	// Purpose: Returns single instance
	////////////////////////////////////////////////////
	static CD6CoreGlobalEnvironment m_Instance;
	static CD6CoreGlobalEnvironment& GetInstance(void)
	{
		return m_Instance;
	}

	////////////////////////////////////////////////////
	// D6CoreModuleInitISystem
	//
	// Purpose: Create the D6 core global environment
	//
	// In:	pSystem - System object
	////////////////////////////////////////////////////
	void D6CoreModuleInitISystem(ISystem *pSystem)
	{
		// Set it up
		pSystem = pSystem;
		pSystemGE = pSystem->GetGlobalEnvironment();
		pBaseManager = new CBaseManager;
		pTeamManager = new CTeamManager;
	};

public:
	////////////////////////////////////////////////////
	// Accessors
	////////////////////////////////////////////////////
	struct ISystem *GetSystem(void) const { return pSystem; }
	struct SSystemGlobalEnvironment *GetGlobalEnvironment(void) const { return pSystemGE; }
	struct IBaseManager *GetBaseManager(void) const { return pBaseManager; }
	struct ITeamManager *GetTeamManager(void) const { return pTeamManager; }
};
extern CD6CoreGlobalEnvironment* D6Core;

#endif //_D6C_d6platform_impl_H_