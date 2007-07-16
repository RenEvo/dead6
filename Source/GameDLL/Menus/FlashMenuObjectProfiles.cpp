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
#include "Game.h"
#include "OptionsManager.h"

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::UpdateProfiles()
{
	IPlayerProfileManager *pProfileMan = g_pGame->GetOptions()->GetProfileManager();
	if(!pProfileMan)
		return;

	m_pPlayerProfileManager = pProfileMan;

	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.Profile.resetProfiles");

		const char *userName = m_pPlayerProfileManager->GetCurrentUser();

		for(int i = 0; i < m_pPlayerProfileManager->GetProfileCount(userName); ++i )
		{
			IPlayerProfileManager::SProfileDescription profDesc;
			pProfileMan->GetProfileInfo(userName, i, profDesc);
			m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.Profile.addProfileToList", profDesc.name);
		}

		IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(userName);
		if(pProfile)
			m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("setActiveProfile", pProfile->GetName());
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::AddProfile(const char *profileName)
{
	if(m_pPlayerProfileManager)
	{
		const char *userName = m_pPlayerProfileManager->GetCurrentUser();

		bool bDone = m_pPlayerProfileManager->CreateProfile(userName,profileName);
		if(bDone)
		{
			SelectProfile(profileName);

			IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
			if(!pProfile)
				return;

			//pProfile->SetAttribute("PlayerName", (TFlowInputData)profileName);
			UpdateProfiles();
			if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
			{
				m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.Profile.gotoProfileMenu");
				m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("showStatusMessage", SFlashVarValue("@ui_menu_PROFILECREATED"));
			}			
		}
		else
		{
			if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
			{
				//Error hack
				m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.Profile.Profile.Controls_M.Controls.Controls_Sub.Btn_SaveProfile.gotoAndPlay", "error");
			}			
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SelectProfile(const char *profileName, bool silent)
{
	if(m_pPlayerProfileManager)
	{
		const char *userName = m_pPlayerProfileManager->GetCurrentUser();
		IPlayerProfile *oldProfile = m_pPlayerProfileManager->GetCurrentProfile(userName);
		if(oldProfile)
			SwitchProfiles(oldProfile->GetName(), profileName);
		else
			SwitchProfiles(NULL, profileName);
    g_pGame->GetIGameFramework()->GetILevelSystem()->LoadRotation();
		UpdateProfiles();
		g_pGame->GetOptions()->InitProfileOptions();
		g_pGame->GetOptions()->UpdateFlashOptions();
		if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
		{
			if(!silent)
			{
				m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.Profile.gotoProfileMenu");
			}
			m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("showStatusMessage", SFlashVarValue("@ui_menu_PROFILELOADED"));
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SwitchProfiles(const char *oldProfile, const char *newProfile)
{
	const char *userName = m_pPlayerProfileManager->GetCurrentUser();
	if(oldProfile)
	{
		m_pPlayerProfileManager->ActivateProfile(userName,oldProfile);
		g_pGame->GetOptions()->SaveValueToProfile("Activated", 0);
	}
	if(newProfile)
	{
		m_pPlayerProfileManager->ActivateProfile(userName,newProfile);
		g_pGame->GetOptions()->SaveValueToProfile("Activated", 1);
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SelectActiveProfile()
{

	IPlayerProfileManager *pMan = g_pGame->GetOptions()->GetProfileManager();
	if(!pMan) return;

	const char *userName = pMan->GetCurrentUser();

	for(int i = 0; i < pMan->GetProfileCount(userName); ++i )
	{
		IPlayerProfileManager::SProfileDescription profDesc;
		pMan->GetProfileInfo(userName, i, profDesc);
		const IPlayerProfile *preview = pMan->PreviewProfile(userName, profDesc.name);
		int iActive = 0;
		if(preview)
		{
			preview->GetAttribute("Activated",iActive);
		}
		if(iActive>0)
		{
			pMan->ActivateProfile(userName,profDesc.name);
			break;
		}
	}
	pMan->PreviewProfile(userName,NULL);
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::DeleteProfile(const char *profileName)
{
	if(!m_pPlayerProfileManager) return;
	const char *userName = m_pPlayerProfileManager->GetCurrentUser();
	m_pPlayerProfileManager->DeleteProfile(userName, profileName);
	UpdateProfiles();
	
	IPlayerProfile *pCurProfile = m_pPlayerProfileManager->GetCurrentProfile(userName);
	if(!pCurProfile)
	{
		IPlayerProfileManager::SProfileDescription profDesc;
		m_pPlayerProfileManager->GetProfileInfo(userName, 0, profDesc);
		SelectProfile(profDesc.name, true);
	}

	g_pGame->GetOptions()->InitProfileOptions();
	g_pGame->GetOptions()->UpdateFlashOptions();
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Root.MainMenu.Profile.updateProfileList");
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SetProfile()
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return;

	SelectActiveProfile();
	g_pGame->GetOptions()->InitProfileOptions();
	g_pGame->GetOptions()->UpdateFlashOptions();
	UpdateProfiles();
	UpdateMenuColor();
}

//-----------------------------------------------------------------------------------------------------