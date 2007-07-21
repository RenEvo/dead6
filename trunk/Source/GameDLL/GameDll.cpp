/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Game DLL entry point.

	-------------------------------------------------------------------------
	History:
	- 2:8:2004   10:38 : Created by Márcio Martins
	- 7/21/07 : Edited for D6 core - KAK

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"

#include <CryLibrary.h>
#include <platform_impl.h>

// [D6] Include our platform too
#include <d6platform_impl.h>
#include "CD6Game.h"

// [D6] Set up extern
CD6CoreGlobalEnvironment CD6CoreGlobalEnvironment::m_Instance;
CD6CoreGlobalEnvironment* D6Core = &CD6CoreGlobalEnvironment::GetInstance();

#define GAME_FRAMEWORK_FILENAME	"cryaction.dll"

extern "C"
{
	GAME_API IGame *CreateGame(IGameFramework* pGameFramework)
	{
		ModuleInitISystem(pGameFramework->GetISystem());

		// [D6] Init our platform
		D6Core->D6CoreModuleInitISystem(pGameFramework->GetISystem());

		static char pGameBuffer[sizeof(IGame)];

		// [D6] Run our game, not theirs!
		return new ((void*)pGameBuffer) CD6Game();
		return new CD6Game();
	}
}

/*
 * this section makes sure that the framework dll is loaded and cleaned up
 * at the appropriate time
 */

#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)

static HMODULE s_frameworkDLL;

static void CleanupFrameworkDLL()
{
	assert( s_frameworkDLL );
	FreeLibrary( s_frameworkDLL );
	s_frameworkDLL = 0;
}

HMODULE GetFrameworkDLL()
{
	if (!s_frameworkDLL)
	{
		s_frameworkDLL = CryLoadLibrary(GAME_FRAMEWORK_FILENAME);
		atexit( CleanupFrameworkDLL );
	}
	return s_frameworkDLL;
}

#endif //WIN32
