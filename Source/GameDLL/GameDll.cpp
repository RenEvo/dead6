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

// [D6] Include our platform too
#include <d6platform_impl.h>
#include "CD6Game.h"
// [/D6]

// [D6] Set up extern
CD6CoreGlobalEnvironment CD6CoreGlobalEnvironment::m_Instance;
CD6CoreGlobalEnvironment* g_D6Core = &CD6CoreGlobalEnvironment::GetInstance();
// [/D6]

#define GAME_FRAMEWORK_FILENAME	"cryaction.dll"

/*
 * this section makes sure that the framework dll is loaded and cleaned up
 * at the appropriate time
 */

#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)

extern "C"
{
	GAME_API IGame *CreateGame(IGameFramework* pGameFramework)
	{
		ModuleInitISystem(pGameFramework->GetISystem());

		// [D6] Init our platform
		g_D6Core->D6CoreModuleInitISystem(pGameFramework->GetISystem());
		// [/D6]

		static char pGameBuffer[sizeof(CD6Game)];

		// [D6] Run our game, not theirs!
		CD6Game *pGame = new ((void*)pGameBuffer) CD6Game();
		g_D6Core->pD6Game = pGame;
		return pGame;
		// [/D6]
	}
}

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
#if defined(SP_DEMO) || defined(CRYSIS_BETA)
///////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// we need pass the full command line, including the filename
	// lpCmdLine does not contain the filename.

	HANDLE mutex = CreateMutex(NULL, TRUE, "CrytekApplication");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if(MessageBox(GetDesktopWindow(), "There is already a Crytek application running\nDo you want to start another one?", "Too many apps", MB_YESNO)!=IDYES)
			return 1;
	}

	// create the startup interface
	//IGameStartup *pGameStartup = CreateGameStartup();

	static char gameStartup_buffer[sizeof(CGameStartup)];
	IGameStartup *pGameStartup = new ((void*)gameStartup_buffer) CGameStartup();	

	if (!pGameStartup)
	{
		// failed to create the startup interface
		//CryFreeLibrary(gameDll);

		MessageBox(0, "Failed to create the GameStartup Interface!", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		CloseHandle(mutex);
		return 0;
	}

	static const char logFileName[] = "Game.log";

	SSystemInitParams startupParams;

	memset(&startupParams, 0, sizeof(SSystemInitParams));

	startupParams.hInstance = GetModuleHandle(0);
	startupParams.sLogFileName = logFileName;	
	sprintf(startupParams.szSystemCmdLine,"Crysis.exe %s",lpCmdLine);
	//strcpy(startupParams.szSystemCmdLine, lpCmdLine);

	// run the game
	if (pGameStartup->Init(startupParams))
	{
		pGameStartup->Run(NULL);

		pGameStartup->Shutdown();
		pGameStartup = 0;
	}

	CloseHandle(mutex);
	return 0;
}
#endif
