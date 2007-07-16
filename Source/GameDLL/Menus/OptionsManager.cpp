/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Handles options setting, getting, saving and loading.

-------------------------------------------------------------------------
History:
- 03/2007: Created by Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "OptionsManager.h"
#include "IPlayerProfiles.h"
#include "FlashMenuObject.h"
#include "FlashMenuScreen.h"
#include "Game.h"
#include "HUD/Hud.h"
//VF_WASINCONFIG

COptionsManager* COptionsManager::sp_optionsManager = 0;

COptionsManager::COptionsManager() : m_pPlayerProfileManager(NULL)
{
	m_defaultColorLine = "4481854";
	m_defaultColorOver = "14125840";
	m_defaultColorText = "12386209";

	InitOpFuncMap();
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::SetProfileManager(IPlayerProfileManager* pProfileMgr)
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) 
		return;

	m_pPlayerProfileManager = pProfileMgr;
	InitProfileOptions();
	g_pGame->GetOptions()->UpdateFlashOptions();
}

//-----------------------------------------------------------------------------------------------------

bool COptionsManager::IgnoreProfile()
{
	return GetISystem()->IsDevMode() && (g_pGameCVars->g_useProfile==0);
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::InitProfileOptions()
{
	if(!m_pPlayerProfileManager)
		return;

	const char* user = m_pPlayerProfileManager->GetCurrentUser();
	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(user);
	if(!pProfile)
		return;

	CryLog("InitProfileOptions: g_useProfile = %d",(int)(!IgnoreProfile()));

	IAttributeEnumeratorPtr attribEnum = pProfile->CreateAttributeEnumerator();
	IAttributeEnumerator::SAttributeDescription attrib;
	const char *o = "Option.";
	int oSize = strlen(o);
	m_profileOptions.clear();
	while(attribEnum->Next(attrib))
	{
		string attribName(attrib.name);
		int pos = attribName.find(o);
		if(pos != string::npos)
		{
			string attribCVar = attribName.substr(pos+oSize, attribName.size() - pos+oSize);
			m_profileOptions[attribCVar] = attribName;
			if(!IgnoreProfile())
			{
				string value;
				ICVar *pCVar = gEnv->pConsole->GetCVar(attribCVar);
				if(pCVar && GetProfileValue(attribName.c_str(), value))
				{
					if(stricmp(pCVar->GetString(), value.c_str()))
					{
						pCVar->Set(value.c_str());
						//CryLogAlways("Inited, loaded and changed: %s = %s", attribName, value);
					}
					else
					{
						//CryLogAlways("Inited, loaded, but not changed: %s = %s", attribName, value);
					}
				}
			}
			else
			{
				//CryLogAlways("Inited, but not loaded: %s / %s", attribName, attribCVar);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::ResetDefaults()
{
	if(!m_pPlayerProfileManager)
		return;

	const char* user = m_pPlayerProfileManager->GetCurrentUser();
	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(user);
	if(!pProfile)
		return;
	XmlNodeRef root = GetISystem()->LoadXmlFile("libs/config/profiles/default/attributes.xml");
	const char *o = "Option.";
	int oSize = strlen(o);
	for (int i = 0; i < root->getChildCount(); ++i)
	{
		XmlNodeRef enumNameNode = root->getChild(i);
		const char *name = enumNameNode->getAttr("name");
		const char *value = enumNameNode->getAttr("value");
		if(name && value)
		{
			string attribName(name);
			int pos = attribName.find(o);
			if(pos != string::npos)
			{
				string attribCVar = attribName.substr(pos+oSize, attribName.size() - pos+oSize);
				ICVar *pCVar = gEnv->pConsole->GetCVar(attribCVar);
				if(pCVar)
				{
					pCVar->Set(value);
				}
			}
		}
	}
	if(!IgnoreProfile())
	{
		UpdateToProfile();
		m_pPlayerProfileManager->SaveProfile(user);
	}
	UpdateFlashOptions();
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::UpdateFlashOptions()
{
	std::map<string,string>::const_iterator it = m_profileOptions.begin();
	std::map<string,string>::const_iterator end = m_profileOptions.end();

	CFlashMenuObject *pMenu = g_pGame->GetMenu();
	CFlashMenuScreen *pMainMenu = pMenu->GetMenuScreen(CFlashMenuObject::MENUSCREEN_FRONTENDSTART);
	CFlashMenuScreen *pInGameMenu = pMenu->GetMenuScreen(CFlashMenuObject::MENUSCREEN_FRONTENDINGAME);

	CFlashMenuScreen *pCurrentMenu = NULL;
	if(pMainMenu && pMainMenu->IsLoaded())
		pCurrentMenu = pMainMenu;
	else if(pInGameMenu && pInGameMenu->IsLoaded())
		pCurrentMenu = pInGameMenu;

	if(!pCurrentMenu) return;

	for(;it!=end;++it)
	{
		ICVar *pCVar = gEnv->pConsole->GetCVar(it->first);
		if(pCVar)
		{
			const char *name = pCVar->GetName();
			string value = pCVar->GetString();

			SFlashVarValue option[3] = {name, value, pCVar?pCVar->IsValid():true};
			pCurrentMenu->Invoke("Root.MainMenu.Options.SetOption", option, 3);
		}
	}
	pCurrentMenu->CheckedInvoke("_root.Root.MainMenu.Options.Options.Controls_M.Controls.updateAllValues");  
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::UpdateToProfile()
{
	std::map<string,string>::const_iterator it = m_profileOptions.begin();
	std::map<string,string>::const_iterator end = m_profileOptions.end();

	for(;it!=end;++it)
	{
		ICVar *pCVAR = gEnv->pConsole->GetCVar(it->first);
		if(pCVAR)
		{
			string value = pCVAR->GetString();
			SaveValueToProfile(it->second.c_str(), value);
		}
	}
}

//-----------------------------------------------------------------------------------------------------
bool COptionsManager::HandleFSCommand(const char *strCommand,const char *strArgs)
{
	
	if(!m_pPlayerProfileManager)
		return false;

	const char* user = m_pPlayerProfileManager->GetCurrentUser();
	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(user);
	if(!pProfile)
		return false;

	if(!stricmp(strCommand, "SaveProfile"))
	{
		if(!IgnoreProfile())
		{
			UpdateToProfile();
			m_pPlayerProfileManager->SaveProfile(user);
			//gEnv->pConsole->ExecuteString("sys_SaveCVars 1");
			//gEnv->pSystem->SaveConfiguration();
			//gEnv->pConsole->ExecuteString("sys_SaveCVars 0");
		}
		return true;
	}

	if(!stricmp(strCommand, "RestoreDefaultProfile"))
	{
		ResetDefaults();
		return true;
	}

	if(!stricmp(strCommand, "UpdateCVars"))
	{
		UpdateFlashOptions();
		return true;
	}

	std::map<string,string>::iterator it = m_profileOptions.find(strCommand);
	if(it!=m_profileOptions.end())
	{
		ICVar *pCVAR = gEnv->pConsole->GetCVar(strCommand);
		if(pCVAR)
		{
			if(pCVAR->GetType()==1)	//int
			{
				int value = atoi(strArgs);
				pCVAR->Set(value);
			}
			else if(pCVAR->GetType()==2)	//float
			{
				float value = atof(strArgs);
				pCVAR->Set(value);
			}
			else if(pCVAR->GetType()==3)	//string
				pCVAR->Set(strArgs);
		}
		return true;
	}
	else //does this map to an options function ?
	{
		TOpFuncMapIt it = m_opFuncMap.find(strCommand);
		if(it!=m_opFuncMap.end())
		{
			(this->*(it->second))(strArgs);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::SaveCVarToProfile(const char* key, const string& value)
{
	if(!m_pPlayerProfileManager)
		return;
	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;

	pProfile->SetAttribute(key, value);
	m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser());
}

//-----------------------------------------------------------------------------------------------------

bool COptionsManager::GetProfileValue(const char* key, int &value)
{
	if(!m_pPlayerProfileManager) return false;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile) return false;

	return pProfile->GetAttribute(key, value);
}

//-----------------------------------------------------------------------------------------------------

bool COptionsManager::GetProfileValue(const char* key, float &value)
{
	if(!m_pPlayerProfileManager) return false;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile) return false;

	return pProfile->GetAttribute(key, value);
}

//-----------------------------------------------------------------------------------------------------

bool COptionsManager::GetProfileValue(const char* key, string &value)
{
	if(!m_pPlayerProfileManager)
	{
		if(gEnv->pSystem->IsEditor())
		{
			if(strcmp(key, "ColorLine") == 0)
			{
				value = m_defaultColorLine;
			}
			else if(strcmp(key, "ColorOver") == 0)
			{
				value = m_defaultColorOver;
			}
			else if(strcmp(key, "ColorText") == 0)
			{
				value = m_defaultColorText;
			}
			else
			{
				value.clear();
			}
			return true;
		}
		return false;
	}

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile) return false;

	return pProfile->GetAttribute(key, value);
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::SaveValueToProfile(const char* key, const string& value)
{
	if(!m_pPlayerProfileManager)
		return;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;

	pProfile->SetAttribute(key, value);
	m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser());
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::SaveValueToProfile(const char* key, int value)
{
	if(!m_pPlayerProfileManager)
		return;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;

	pProfile->SetAttribute(key, value);
	m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser());
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::SaveValueToProfile(const char* key, float value)
{
	if(!m_pPlayerProfileManager)
		return;

	IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
	if(!pProfile)
		return;

	pProfile->SetAttribute(key, value);
	m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser());
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::OnElementFound(ICVar *pCVar)
{
	if(pCVar)
	{
		CFlashMenuObject *pMenu = g_pGame->GetMenu();
		CFlashMenuScreen *pMainMenu = pMenu->GetMenuScreen(CFlashMenuObject::MENUSCREEN_FRONTENDSTART);
		CFlashMenuScreen *pInGameMenu = pMenu->GetMenuScreen(CFlashMenuObject::MENUSCREEN_FRONTENDINGAME);

		CFlashMenuScreen *pCurrentMenu = NULL;
		if(pMainMenu && pMainMenu->IsLoaded())
			pCurrentMenu = pMainMenu;
		else if(pInGameMenu && pInGameMenu->IsLoaded())
			pCurrentMenu = pInGameMenu;

		const char *name = pCVar->GetName();
		string value = pCVar->GetString();

		SFlashVarValue option[3] = {name, value, pCVar?pCVar->IsValid():true};
		pCurrentMenu->Invoke("Root.MainMenu.Options.SetOption", option, 3);
	}
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::AutoDetectHardware(const char* params)
{
	GetISystem()->AutoDetectSpec();
	SystemConfigChanged();
}
//-----------------------------------------------------------------------------------------------------

void COptionsManager::SetVideoMode(const char* params)
{
	CryFixedStringT<64> resolution(params);
	int pos = resolution.find('x');
	if(pos != CryFixedStringT<64>::npos)
	{
		CryFixedStringT<64> width = "r_width ";
		width.append(resolution.substr(0, pos));
		CryFixedStringT<64> height = "r_height ";
		height.append(resolution.substr(pos+1, resolution.size()));

		gEnv->pConsole->ExecuteString(width);
		gEnv->pConsole->ExecuteString(height);
	}
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::InitOpFuncMap()
{
	//set-up function pointer for complex options
	m_opFuncMap["SetVideoMode"] = &COptionsManager::SetVideoMode;
	m_opFuncMap["AutoDetectHardware"] = &COptionsManager::AutoDetectHardware;
}

//-----------------------------------------------------------------------------------------------------

void COptionsManager::SystemConfigChanged()
{
	//gEnv->pConsole->ExecuteString("sys_SaveCVars 1");
	//gEnv->pSystem->SaveConfiguration();
	//gEnv->pConsole->ExecuteString("sys_SaveCVars 0");

	if(m_pPlayerProfileManager)
	{
		if(!IgnoreProfile())
		{
			UpdateToProfile();
			m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser());
		}
	}
	
	if(CFlashMenuScreen *pCurrentMenu = GetCurrentMenu())
	{
		pCurrentMenu->Invoke("showErrorMessage", "Box1");
		CryFixedStringT<128> text = "@system_spec_";
		text.append(gEnv->pConsole->GetCVar("sys_spec")->GetString());
		pCurrentMenu->Invoke("setErrorText", text.c_str());
		text = "sys_spec_Full \0";
		text.append(gEnv->pConsole->GetCVar("sys_spec")->GetString());
		gEnv->pConsole->ExecuteString(text.c_str());
	}
	UpdateFlashOptions();

}

//-----------------------------------------------------------------------------------------------------

CFlashMenuScreen *COptionsManager::GetCurrentMenu()
{
	CFlashMenuScreen *pMainMenu = g_pGame->GetMenu()->GetMenuScreen(CFlashMenuObject::MENUSCREEN_FRONTENDSTART);
	CFlashMenuScreen *pInGameMenu = g_pGame->GetMenu()->GetMenuScreen(CFlashMenuObject::MENUSCREEN_FRONTENDINGAME);

	CFlashMenuScreen *pCurrentMenu = NULL;
	if(pMainMenu && pMainMenu->IsLoaded())
		pCurrentMenu = pMainMenu;
	else if(pInGameMenu && pInGameMenu->IsLoaded())
		pCurrentMenu = pInGameMenu;

	return pCurrentMenu;
}
