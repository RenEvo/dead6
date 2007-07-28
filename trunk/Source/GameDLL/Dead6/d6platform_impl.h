////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// d6platform_impl.h
//
// Purpose: Platform implementation for core setup
//
// Note: I'm including in the precompiled header!
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_d6platform_impl_H_ 
#define _D6C_d6platform_impl_H_

#include "ISystem.h"
#include "IBaseManager.h"
#include "ITeamManager.h"
#include "CD6Game.h"
#include "CD6GameRules.h"

////////////////////////////////////////////////////
// D6 Core global environment
class CD6CoreGlobalEnvironment
{
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6CoreGlobalEnvironment(void);
	CD6CoreGlobalEnvironment(CD6CoreGlobalEnvironment const&) {}
	CD6CoreGlobalEnvironment& operator =(CD6CoreGlobalEnvironment const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6CoreGlobalEnvironment(void);

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
	void D6CoreModuleInitISystem(ISystem *pSystem);

public:
	// Created modules
	IBaseManager *pBaseManager;
	ITeamManager *pTeamManager;

	// Referenced modules
	ISystem *pSystem;
	CD6Game *pD6Game;
	CD6GameRules *pD6GameRules;
};
extern CD6CoreGlobalEnvironment* g_D6Core;

#endif //_D6C_d6platform_impl_H_