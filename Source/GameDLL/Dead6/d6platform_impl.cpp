////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// d6platform_impl.cpp
//
// Purpose: Platform implementation for core setup
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include <d6platform_impl.h>

// Core files
#include "CBaseManager.h"
#include "CTeamManager.h"
#include "CPortalManager.h"

////////////////////////////////////////////////////
CD6CoreGlobalEnvironment::CD6CoreGlobalEnvironment(void)
{
	pBaseManager = NULL;
	pTeamManager = NULL;
	pPortalManager = NULL;
	pSystem = NULL;
	pD6Game = NULL;
	pD6GameRules = NULL;
}

////////////////////////////////////////////////////
CD6CoreGlobalEnvironment::~CD6CoreGlobalEnvironment(void)
{
	SAFE_DELETE(pBaseManager);
	SAFE_DELETE(pTeamManager);
	SAFE_DELETE(pPortalManager);
}

////////////////////////////////////////////////////
void CD6CoreGlobalEnvironment::D6CoreModuleInitISystem(ISystem *pSystem)
{
	// Set it up
	this->pSystem = pSystem;
	pBaseManager = new CBaseManager;
	pTeamManager = new CTeamManager;
	pPortalManager = new CPortalManager;
};