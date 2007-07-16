/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 28:08:2006 : Created by TomasN

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_MusicLogic.h"
#include "MusicLogic.h"
#include "IAnimationGraph.h"
#include "Game.h"
//#include "Menus/FlashMenuObject.h"


//------------------------------------------------------------------------
CScriptBind_MusicLogic::CScriptBind_MusicLogic(CMusicLogic *pMusicLogic)
{
	m_pSystem			= gEnv->pSystem;
	m_pSS					= gEnv->pScriptSystem;
	m_pMusicLogic = pMusicLogic;

	Init(m_pSS, m_pSystem);
	SetGlobalName("MusicLogic");

	RegisterMethods();
	RegisterGlobals();
}

//------------------------------------------------------------------------
CScriptBind_MusicLogic::~CScriptBind_MusicLogic()
{
}

//------------------------------------------------------------------------
void CScriptBind_MusicLogic::RegisterGlobals()
{
	m_pSS->SetGlobalValue("MUSICEVENT_ENTER_VEHICLE",MUSICEVENT_ENTER_VEHICLE);
	m_pSS->SetGlobalValue("MUSICEVENT_LEAVE_VEHICLE",MUSICEVENT_LEAVE_VEHICLE);
	m_pSS->SetGlobalValue("MUSICEVENT_MOUNT_WEAPON",MUSICEVENT_MOUNT_WEAPON);
	m_pSS->SetGlobalValue("MUSICEVENT_UNMOUNT_WEAPON",MUSICEVENT_UNMOUNT_WEAPON);
	m_pSS->SetGlobalValue("MUSICEVENT_ENEMY_SPOTTED",MUSICEVENT_ENEMY_SPOTTED);
	m_pSS->SetGlobalValue("MUSICEVENT_ENEMY_KILLED",MUSICEVENT_ENEMY_KILLED);
	m_pSS->SetGlobalValue("MUSICEVENT_ENEMY_HEADSHOT",MUSICEVENT_ENEMY_HEADSHOT);
	m_pSS->SetGlobalValue("MUSICEVENT_ENEMY_OVERRUN",MUSICEVENT_ENEMY_OVERRUN);
	m_pSS->SetGlobalValue("MUSICEVENT_PLAYER_WOUNDED",MUSICEVENT_PLAYER_WOUNDED);
	m_pSS->SetGlobalValue("MUSICEVENT_EXPLOSION",MUSICEVENT_EXPLOSION);

}

//------------------------------------------------------------------------
void CScriptBind_MusicLogic::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_MusicLogic::

	SCRIPT_REG_TEMPLFUNC(SetMusicState, "fIntensity,fBoredom");
	SCRIPT_REG_TEMPLFUNC(SetMusicEvent, "eMusicEvent, fValue");
	SCRIPT_REG_TEMPLFUNC(StartLogic, "");
	SCRIPT_REG_TEMPLFUNC(StopLogic, "");

	//SCRIPT_REG_TEMPLFUNC(PauseGame, "pause");

#undef SCRIPT_REG_CLASSNAME
}

//------------------------------------------------------------------------
int CScriptBind_MusicLogic::SetMusicState(IFunctionHandler *pH, float fIntensity, float fBoredom)
{
	if (m_pMusicLogic)
	{
		CMusicLogic::SMusicStateInfo MusicInfo;
		MusicInfo.fIntensity = fIntensity;
		MusicInfo.fBoredom = fBoredom;
		m_pMusicLogic->SetMusicStateInfo(&MusicInfo);
	}

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_MusicLogic::SetMusicEvent(IFunctionHandler *pH, int eMusicEvent)
{
//	SCRIPT_CHECK_PARAMETERS(1);
//	int nCookie = 0;
//	float fRatio;
//	_smart_ptr<ISound> pSound = GetSoundPtr(pH,1);

	if (m_pMusicLogic)
	{
		switch(eMusicEvent)
		{
		case MUSICEVENT_ENTER_VEHICLE:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_ENTER_VEHICLE,0.0f);
			break;
		case MUSICEVENT_LEAVE_VEHICLE:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_LEAVE_VEHICLE,0.0f);
			break;
		case MUSICEVENT_MOUNT_WEAPON:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_MOUNT_WEAPON,0.0f);			
			break;
		case MUSICEVENT_UNMOUNT_WEAPON:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_UNMOUNT_WEAPON,0.0f);
			break;
		case MUSICEVENT_ENEMY_SPOTTED:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_ENEMY_SPOTTED,0.0f);
			break;
		case MUSICEVENT_ENEMY_KILLED:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_ENEMY_KILLED,0.0f);
			break;
		case MUSICEVENT_ENEMY_HEADSHOT:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_ENEMY_HEADSHOT,0.0f);
			break;
		case MUSICEVENT_ENEMY_OVERRUN:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_ENEMY_OVERRUN,0.0f);
			break;
		case MUSICEVENT_PLAYER_WOUNDED:
			m_pMusicLogic->SetMusicEvent(MUSICEVENT_PLAYER_WOUNDED,0.0f);
			break;
		case MUSICEVENT_MAX:
			break;
		default:
			break;
		}

	}

	//pH->GetParam(2, fRatio);
	//CFlashMenuObject* pFMO = CFlashMenuObject::GetFlashMenuObject();
	//if (pFMO)
	//{
	//	pFMO->ShowInGameMenu();
	//}
	return pH->EndFunction();
}

//------------------------------------------------------------------------
//int CScriptBind_MusicLogic::PauseGame( IFunctionHandler *pH, bool pause )
//{
//	bool forced = false;
//
//	if (pH->GetParamCount() > 1)
//	{
//		pH->GetParam(2, forced);
//	}
//	m_pGameFW->PauseGame(pause, forced);
//
//	return pH->EndFunction();
//}


//------------------------------------------------------------------------
int CScriptBind_MusicLogic::StartLogic(IFunctionHandler *pH)
{
	if (m_pMusicLogic)
		m_pMusicLogic->Start();

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_MusicLogic::StopLogic(IFunctionHandler *pH)
{
	if (m_pMusicLogic)
		m_pMusicLogic->Stop();

	return pH->EndFunction();
}