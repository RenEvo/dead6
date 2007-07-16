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

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"

#include <CryLibrary.h>
#include <platform_impl.h>

#define GAME_FRAMEWORK_FILENAME	"cryaction.dll"

extern "C"
{
	GAME_API IGame *CreateGame(IGameFramework* pGameFramework)
	{
		ModuleInitISystem(pGameFramework->GetISystem());

		static char pGameBuffer[sizeof(IGame)];
		return new ((void*)pGameBuffer) CGame();

		return new CGame();
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
