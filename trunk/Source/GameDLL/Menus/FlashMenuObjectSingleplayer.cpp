/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Flash menu screen "Profiles"

-------------------------------------------------------------------------
History:
- 09:21:2006: Created by Jan Neugebauer

*************************************************************************/
#include "StdAfx.h"

#include "FlashMenuObject.h"
#include "FlashMenuScreen.h"
#include "IGameFramework.h"
#include "IPlayerProfiles.h"
#include "IUIDraw.h"
#include "IMusicSystem.h"
#include "ISound.h"
#include "IRenderer.h"
#include "Game.h"
#include "Menus/OptionsManager.h"
#include <time.h>

enum EDifficulty
{
	EDifficulty_EASY,
	EDifficulty_NORMAL,
	EDifficulty_REALISTIC,
	EDifficulty_DELTA,
	EDifficulty_END
};


//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::UpdateSingleplayerDifficulties()
{
	if(!m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
		return;

	if(!m_pPlayerProfileManager)
		return;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;

	string sGeneralPath = "Singleplayer.Difficulty";
	int iDifficulties = 8;
/*	for(int i=0; i<EDifficulty_END; ++i)
	{
		string sPath = sGeneralPath;
		char c[5];
		itoa(i, c, 10);
		sPath.append(c);
		sPath.append(".available");

		TFlowInputData data;
		pProfile->GetAttribute(sPath, data, false);
		bool bDone = false;
		data.GetValueWithConversion(bDone);
		if(bDone)
		{
			iDifficulties += i*2;
		}
	}
*/
	int iDifficultiesDone = 0;
	for(int i=0; i<EDifficulty_END; ++i)
	{
		string sPath = sGeneralPath;
		char c[5];
		itoa(i, c, 10);
		sPath.append(c);
		sPath.append(".done");

		TFlowInputData data;
		pProfile->GetAttribute(sPath, data, false);
		bool bDone = false;
		data.GetValueWithConversion(bDone);
		if(bDone)
		{
			iDifficultiesDone += std::max(i*2,1);
		}
	}

	TFlowInputData data;
	pProfile->GetAttribute("Singleplayer.LastSelectedDifficulty", data, false);
	int iDiff = 2;
	data.GetValueWithConversion(iDiff);

	if(iDiff<=0)
	{
		iDiff = 2;
	}

	m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.SinglePlayer.enableDifficulties", iDifficulties);
	m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.SinglePlayer.enableDifficultiesStats", iDifficultiesDone);
	m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.SinglePlayer.selectDifficulty", iDiff);
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::StartSingleplayerGame(const char *strDifficulty)
{
	int iDifficulty = 0;
	if(!strcmp(strDifficulty,"Easy"))
	{
		iDifficulty = 1;
		gEnv->pConsole->ExecuteString("exec diff_easy");
	}
	else if(!strcmp(strDifficulty,"Normal"))
	{
		gEnv->pConsole->ExecuteString("exec diff_normal");
		iDifficulty = 2;
	}
	else if(!strcmp(strDifficulty,"Realistic"))
	{
		gEnv->pConsole->ExecuteString("exec diff_hard");
		iDifficulty = 3;
	}
	else if(!strcmp(strDifficulty,"Delta"))
	{
		gEnv->pConsole->ExecuteString("exec diff_bauer");
		iDifficulty = 4;
	}

	if(m_pPlayerProfileManager)
	{
		IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
		if(pProfile)
		{
			pProfile->SetAttribute("Singleplayer.LastSelectedDifficulty",(TFlowInputData)iDifficulty);
			m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser());
		}
	}
	StopVideo();
	m_bDestroyStartMenuPending = true;
	m_stateEntryMovies = eEMS_GameStart;
}


void CFlashMenuObject::UpdateSaveGames()
{
	CFlashMenuScreen* pScreen = m_pCurrentFlashMenuScreen;
	if(!pScreen)
		return;
	
	pScreen->CheckedInvoke("resetSPGames");

	// TODO: find a better place for this as it needs to be set only once -- CW
	gEnv->pSystem->SetFlashLoadMovieHandler(this);

	if(!m_pPlayerProfileManager)
		return;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;
 
	ISaveGameEnumeratorPtr pSGE = pProfile->CreateSaveGameEnumerator();
	ISaveGameEnumerator::SGameDescription desc;	
	char dateBuf[256];
	char timeBuf[256];
	struct tm timePtr;

	for (int i=0; i<pSGE->GetCount(); ++i)
	{
		pSGE->GetDescription(i, desc);
		timePtr = *localtime(&desc.metaData.saveTime);
		const char* dateString = dateBuf;
		const char* timeString = timeBuf;
		bool bDateOk = (strftime(dateBuf, sizeof(dateBuf), "%#x", &timePtr) != 0);
		bool bTimeOk = (strftime(timeBuf, sizeof(timeBuf), "%X", &timePtr) != 0);
		if (!bDateOk || !bTimeOk)
		{
			timeString = asctime(&timePtr);
			dateString = "";
		}
		SFlashVarValue args[9] = {desc.name, desc.description, desc.humanName, desc.metaData.levelName, desc.metaData.gameRules, desc.metaData.fileVersion, desc.metaData.buildVersion, timeString, dateString};
		pScreen->CheckedInvoke("addGameToList", args, 9);
	}
	pScreen->CheckedInvoke("updateGameList");
}

void CFlashMenuObject::LoadGame(const char *fileName)
{
	gEnv->pGame->GetIGameFramework()->LoadGame(fileName, false);
}

void CFlashMenuObject::DeleteSaveGame(const char *fileName)
{
	CryLogAlways("DeleteSaveGame(%s)",fileName);
	if(!m_pPlayerProfileManager)
		return;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;
	pProfile->DeleteSaveGame(fileName);
	UpdateSaveGames();
}

void CFlashMenuObject::SaveGame(const char *fileName)
{
	string sSaveFileName = fileName;
	string sScreenFileName = fileName;
	sSaveFileName.append(".CRYSISJMSF");
	sScreenFileName.append(".jpg");
	gEnv->pGame->GetIGameFramework()->SaveGame(sSaveFileName,true, false);
	/*string sScreenPath = g_pGame->GetOptions()->GetCurrentProfileDirectory();
	if(sScreenPath)
	{
		sScreenPath.append("savegames\\");
		sScreenPath.append(sScreenFileName);
		m_sScreenshot = sScreenPath;
	}
	else
	{*/
		m_sScreenshot = sScreenFileName;
	//}
	m_bTakeScreenshot = false;
	UpdateSaveGames();
}