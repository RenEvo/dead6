/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Handles options setting, getting, saving and loading
between profile, system and flash.

-------------------------------------------------------------------------
History:
- 03/2007: Created by Jan Müller

*************************************************************************/
#ifndef __OPTIONS_MANAGER_H__
#define __OPTIONS_MANAGER_H__

#pragma once

#include "IGameFramework.h"
#include "GameCVars.h"

class CGameFlashAnimation;
class CFlashMenuScreen;

class COptionsManager : public ICVarDumpSink
{

	typedef void (COptionsManager::*OpFuncPtr) (const char*);

public :

	~COptionsManager() 
	{
		sp_optionsManager = 0;
	};

	static COptionsManager* CreateOptionsManager()
	{
		if(!sp_optionsManager)
			sp_optionsManager = new COptionsManager();
		return sp_optionsManager;
	}

	ILINE IPlayerProfileManager *GetProfileManager() { return m_pPlayerProfileManager; }

	//ICVarDumpSink
	virtual void OnElementFound(ICVar *pCVar);
	//~ICVarDumpSink

	//flash system
	bool HandleFSCommand(const char *strCommand,const char *strArgs);
	void UpdateFlashOptions();
	void InitProfileOptions();
	void UpdateToProfile();
	void ResetDefaults();
	//flash system

	//profile system
	void SetProfileManager(IPlayerProfileManager* pProfileManager);
	void SaveCVarToProfile(const char* key, const string& value);
	bool GetProfileValue(const char* key, int &value);
	bool GetProfileValue(const char* key, float &value);
	bool GetProfileValue(const char* key, string &value);
	void SaveValueToProfile(const char* key, int value);
	void SaveValueToProfile(const char* key, float value);
	void SaveValueToProfile(const char* key, const string& value);
	void CVarToProfile();
	void ProfileToCVar();
	bool IgnoreProfile();
	//~profile system

private:
	COptionsManager();
	IPlayerProfileManager* m_pPlayerProfileManager;

	std::map<string, string> m_profileOptions;


	//************ OPTION FUNCTIONS ************
	void AutoDetectHardware(const char* params);
	void SetVideoMode(const char* params);
	//initialize option-functions
	void InitOpFuncMap();
	//option-function mapper
	typedef std::map<string, OpFuncPtr> TOpFuncMap;
	TOpFuncMap	m_opFuncMap;
	typedef TOpFuncMap::iterator TOpFuncMapIt;
	//******************************************

	void SystemConfigChanged();
	CFlashMenuScreen * GetCurrentMenu();

	static COptionsManager* sp_optionsManager;

	const char *m_defaultColorLine;
	const char *m_defaultColorOver;
	const char *m_defaultColorText;
};

#endif