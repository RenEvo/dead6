/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre

*************************************************************************/
#include "StdAfx.h"
#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "IFlashPlayer.h"
#include "HUD.h"

//-----------------------------------------------------------------------------------------------------

CGameFlashAnimation::CGameFlashAnimation()
{
	m_dock = eGFD_Stretch;
	m_flags = 0;
}

//-----------------------------------------------------------------------------------------------------

CGameFlashAnimation::~CGameFlashAnimation()
{
	CHUD* pHUD = g_pGame->GetHUD();

	// deregister us from the HUD Updates and Rendering
	if(pHUD)
	{
		pHUD->Remove(this);
	}

	for(TGameFlashLogicsList::iterator iter=m_gameFlashLogicsList.begin(); iter!=m_gameFlashLogicsList.end(); ++iter)
	{
		SAFE_DELETE(*iter);
	}
}

//-----------------------------------------------------------------------------------------------------
void CGameFlashAnimation::SetDock(uint32 eGFDock)
{
	m_dock = eGFDock;
}
//-----------------------------------------------------------------------------------------------------
uint32 CGameFlashAnimation::GetDock() const
{
	return m_dock;
}
//-----------------------------------------------------------------------------------------------------
uint32 CGameFlashAnimation::GetFlags() const
{
	return m_flags;
}
//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::Init(const char *strFileName, EGameFlashDock docking, uint32 flags)
{
	Unload();
	m_fileName = strFileName;
	m_dock = docking;
	m_flags = flags;
}

//-----------------------------------------------------------------------------------------------------
bool CGameFlashAnimation::Reload(bool forceUnload)
{
	if (forceUnload)
		Unload();

	if (!m_fileName.empty() && !IsLoaded())
	{
		CHUD* pHUD = g_pGame->GetHUD();

		if (pHUD && LoadAnimation(m_fileName.c_str()))
		{
			SetDock(m_dock);

			IRenderer *pRenderer = gEnv->pRenderer;
			GetFlashPlayer()->SetViewport(0,0,pRenderer->GetWidth(),pRenderer->GetHeight());
			GetFlashPlayer()->SetBackgroundAlpha(0.0f);

			pHUD->Register(this);

			if (m_flags & eFAF_ThisHandler)
				GetFlashPlayer()->SetFSCommandHandler(pHUD);

			pHUD->RepositionFlashAnimation(this);
			pHUD->SetFlashColor(this);

			if (!(m_flags & eFAF_Visible))
				SetVisible(false);

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CGameFlashAnimation::Load(const char *strFileName, EGameFlashDock docking, uint32 flags)
{
	Init(strFileName, docking, flags);
	return Reload();
}

void CGameFlashAnimation::Unload()
{
	// early out
	if (!IsLoaded())
		return;

	CFlashAnimation::Unload();

	CHUD* pHUD = g_pGame->GetHUD();

	// remove us from the HUD updates and rendering
	if(pHUD)
	{
		pHUD->Remove(this);
	}
}

//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::AddVariable(const char *strControl,const char *strVariable,const char *strToken,float fScale,float fOffset)
{
	string token(strToken);
	// make sure we don't already have this variable
	TGameFlashLogicsList::const_iterator endIt = m_gameFlashLogicsList.end();
	for (TGameFlashLogicsList::const_iterator i = m_gameFlashLogicsList.begin(); i != endIt; ++i)
	{
		if ((*i)->GetToken() == token)
			return;
	}

	// it's unique ... so we create and add it
	CGameFlashLogic *pGFVariable = new CGameFlashLogic(this);
	pGFVariable->Init(strControl, strVariable, strToken, fScale, fOffset);
	m_gameFlashLogicsList.push_back(pGFVariable);
}

//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::ReInitVariables()
{
	TGameFlashLogicsList::const_iterator endIt = m_gameFlashLogicsList.end();
	for (TGameFlashLogicsList::const_iterator i = m_gameFlashLogicsList.begin(); i != endIt; ++i)
	{
		(*i)->ReInit();
	}
}

//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::GetMemoryStatistics(ICrySizer * s)
{
	s->AddContainer(m_gameFlashLogicsList);
	for (TGameFlashLogicsList::iterator iter = m_gameFlashLogicsList.begin(); iter != m_gameFlashLogicsList.end(); ++iter)
	{
		(*iter)->GetMemoryStatistics(s);
	}
}
