/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	Common functionality for all HUDs using Flash player
	Code which is not game-specific should go here
	Shared by G02 and G04

-------------------------------------------------------------------------
History:
- 22:02:2006: Created by Matthew Jack from original HUD class

*************************************************************************/
#include "StdAfx.h"
#include "HUDCommon.h"
#include "HUD.h"

#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "Menus/FlashMenuObject.h"

#include "Player.h"
#include "Weapon.h"
#include "Game.h"
#include "GameCVars.h"

//-----------------------------------------------------------------------------------------------------
//-- IConsoleArgs
//-----------------------------------------------------------------------------------------------------

void CHUDCommon::HUD(ICVar *pVar)
{
	CHUD* pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		pHUD->Show(pVar->GetIVal()!=0);
	}
}

void CHUDCommon::ShowGODMode(IConsoleCmdArgs *pConsoleCmdArgs)
{
	CHUD* pHUD = g_pGame->GetHUD();

	if(pHUD && 2 == pConsoleCmdArgs->GetArgCount())
	{
		if(0 == strcmp(pConsoleCmdArgs->GetArg(1),"0"))
		{
			pHUD->m_bShowGODMode = false;
		}
		else
		{
			pHUD->m_bShowGODMode = true;
		}
	}
}

//-----------------------------------------------------------------------------------------------------
//-- ~ IConsoleArgs
//-----------------------------------------------------------------------------------------------------



//-----------------------------------------------------------------------------------------------------
void CHUDCommon::UpdateRatio()
{
	// try to resize based on any width and height
	for(TGameFlashAnimationsList::iterator i=m_gameFlashAnimationsList.begin(); i!=m_gameFlashAnimationsList.end(); ++i)
	{
		CGameFlashAnimation *pAnim = (*i);
		RepositionFlashAnimation(pAnim);
	}

	m_width		= gEnv->pRenderer->GetWidth();
	m_height	= gEnv->pRenderer->GetHeight();
}

//-----------------------------------------------------------------------------------------------------



CHUDCommon::CHUDCommon() 
{
	m_bShowGODMode = true;
	m_godMode = 0;
	m_iDeaths = 0;

	strcpy(m_strGODMode,"");
	
	m_width					= 0;
	m_height				= 0;

	m_distortionStrength = 0;
	m_displacementStrength = 0;
	m_alphaStrength = 0;
	m_interferenceDecay = 0;

	m_lastInterference = 0;

	m_displacementX = 0;
	m_displacementY = 0;
	m_distortionX = 0;
	m_distortionY = 0;
	m_alpha	 = 100;

	m_iCursorVisibilityCounter = 0;

	m_bShow = true;

	gEnv->pConsole->AddCommand("ShowGODMode",ShowGODMode);

	if(gEnv->pHardwareMouse)
	{
		gEnv->pHardwareMouse->AddListener(this);
	}
}

//-----------------------------------------------------------------------------------------------------

CHUDCommon::~CHUDCommon()
{
	if(m_iCursorVisibilityCounter)
	{
		if(gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->DecrementCounter();
		}
		gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_mouse",false);
	}

	if(gEnv->pHardwareMouse)
	{
		gEnv->pHardwareMouse->RemoveListener(this);
	}

}


//-----------------------------------------------------------------------------------------------------

void CHUDCommon::Show(bool bShow)
{
	m_bShow = bShow;
}

//-----------------------------------------------------------------------------------------------------

void CHUDCommon::SetGODMode(uint8 ucGodMode, bool forceUpdate)
{
	if (forceUpdate || m_godMode != ucGodMode)
	{
		m_godMode = ucGodMode;
		m_fLastGodModeUpdate = gEnv->pTimer->GetAsyncTime().GetSeconds();

		if(0 == ucGodMode)
		{
			strcpy(m_strGODMode,"GOD MODE OFF");
			m_iDeaths = 0;
		}
		else if(1 == ucGodMode)
		{
			strcpy(m_strGODMode,"GOD");
		}
		else if(2 == ucGodMode)
		{
			strcpy(m_strGODMode,"Team GOD");
		}
		else if(3 == ucGodMode)
		{
			strcpy(m_strGODMode,"DEMI GOD");
		}
	}
}


//-----------------------------------------------------------------------------------------------------
//-- Cursor handling
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------

void CHUDCommon::CursorIncrementCounter()
{
	m_iCursorVisibilityCounter++;
	assert(m_iCursorVisibilityCounter >= 0);

	if(1 == m_iCursorVisibilityCounter)
	{
		if(gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->IncrementCounter();
		}
		gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_mouse",true);
		UpdateCrosshairVisibility();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDCommon::CursorDecrementCounter()
{
	m_iCursorVisibilityCounter--;
	assert(m_iCursorVisibilityCounter >= 0);

	if(0 == m_iCursorVisibilityCounter)
	{
		if(gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->DecrementCounter();
		}
		gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_mouse",false);
		UpdateCrosshairVisibility();
	}
}


//-----------------------------------------------------------------------------------------------------
//-- Starting new interference effect 
//-----------------------------------------------------------------------------------------------------

void CHUDCommon::StartInterference(float distortion, float displacement, float alpha, float decay)
{
	m_distortionStrength = distortion;
	m_displacementStrength = displacement;
	m_alphaStrength = alpha;
	m_interferenceDecay = decay;
	m_lastInterference = 0;
}

void CHUDCommon::Register(CGameFlashAnimation* pAnim)
{
	TGameFlashAnimationsList::iterator it = std::find(m_gameFlashAnimationsList.begin(), m_gameFlashAnimationsList.end(), pAnim);

	if (it == m_gameFlashAnimationsList.end())
		m_gameFlashAnimationsList.push_back(pAnim);
}

void CHUDCommon::Remove(CGameFlashAnimation* pAnim)
{
	TGameFlashAnimationsList::iterator it = std::find(m_gameFlashAnimationsList.begin(), m_gameFlashAnimationsList.end(), pAnim);

	if (it != m_gameFlashAnimationsList.end())
		m_gameFlashAnimationsList.erase(it);
}

//-----------------------------------------------------------------------------------------------------
//-- Creating random distortion and displacements
//-----------------------------------------------------------------------------------------------------

void CHUDCommon::CreateInterference()
{
	m_distortionStrength;
	m_displacementStrength;
	m_alphaStrength;

	float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	float timeGone = 0;
	if(m_lastInterference)
	{
		timeGone = now - m_lastInterference;
	}
	m_lastInterference = now;

	m_distortionX = (int)((Random()*m_distortionStrength)*0.5-m_distortionStrength*0.5);
	m_distortionY = (int)((Random()*m_distortionStrength)-m_distortionStrength);
	m_displacementX = (int)((Random()*m_displacementStrength)-m_displacementStrength*0.5f);
	m_displacementY = (int)((Random()*m_displacementStrength)-m_displacementStrength*0.5f);

	m_alpha = 100 - (int)(Random()*m_alphaStrength);
	
	m_distortionStrength		-= m_distortionStrength*m_interferenceDecay*timeGone;
	if(m_distortionStrength<0.5f)
		m_distortionStrength = 0;
	m_displacementStrength	-= m_displacementStrength*m_interferenceDecay*timeGone;
	if(m_displacementStrength<0.5f)
		m_displacementStrength = 0;
	m_alphaStrength					-= m_alphaStrength*m_interferenceDecay*timeGone;
	if(m_alphaStrength<1.0f)
		m_alphaStrength = 0;
}

//-----------------------------------------------------------------------------------------------------
//-- Positioning and scaling animations
//-----------------------------------------------------------------------------------------------------

void CHUDCommon::RepositionFlashAnimation(CGameFlashAnimation *pAnimation) const
{
	if(!pAnimation)
		return;

	IFlashPlayer *player = pAnimation->GetFlashPlayer();
	if(player)
	{
		IRenderer *pRenderer = gEnv->pRenderer;

		float fMovieRatio		=	((float)player->GetWidth()) / ((float)player->GetHeight());
		float fRenderRatio	=	((float)pRenderer->GetWidth()) / ((float)pRenderer->GetHeight());

		float fWidth				=	pRenderer->GetWidth();
		float fHeight				=	pRenderer->GetHeight();
		float fXPos = 0.0f;
		float fYPos = 0.0f;

		float fXOffset			= (fWidth - (fMovieRatio * fHeight));

		int dock = pAnimation->GetDock();

		if(fRenderRatio != fMovieRatio && !(dock & eGFD_Stretch))
		{

			fWidth = fWidth-fXOffset;

			if (dock & eGFD_Left)
				fXPos = 0;
			else if (dock & eGFD_Right)
				fXPos = fXOffset;
			else if (dock & eGFD_Center)
				fXPos = fXOffset * 0.5;
		}

		player->SetViewport((int)(((int)fXPos+m_displacementX)-(m_distortionX*0.5)),(int)(m_displacementY-(m_distortionY*0.5)),(int)fWidth+m_distortionX,(int)fHeight+m_distortionY);
		player->SetVariable("_alpha",SFlashVarValue(m_alpha));
	}
}
