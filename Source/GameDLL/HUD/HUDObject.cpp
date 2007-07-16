/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	Header for HUD object base class
	Defines base class for HUD elements that are drawn using C++ rather than Flash
	Shared by G02 and G04

-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre
- 22:02:2006: Refactored for G04 by Matthew Jack

*************************************************************************/
#include "StdAfx.h"
#include "HUDObject.h"
#include "IGameTokens.h"
#include "IGame.h"
#include "IGameFramework.h"

//-----------------------------------------------------------------------------------------------------

CHUDObject::CHUDObject(bool bVisible)
{
	m_fX = 0.0f;
	m_fY = 0.0f;

	m_fFadeValue = 1.0f;
}

//hack - Jan
void CHUDObject::SetParent(void* parent)
{
	m_parent = (void*)parent;
}

//-----------------------------------------------------------------------------------------------------

CHUDObject::~CHUDObject()
{
}

//-----------------------------------------------------------------------------------------------------

void CHUDObject::SetFadeValue(float fFadeValue)
{
	m_fFadeValue = fFadeValue;
}

//-----------------------------------------------------------------------------------------------------

void CHUDObject::Update(float fDeltaTime)
{
	if(m_fFadeValue > 0.0f)
	{
		OnUpdate(fDeltaTime,m_fFadeValue);
	}
}

//-----------------------------------------------------------------------------------------------------

// Convenience function for accessing the GameToken system.
IGameTokenSystem *CHUDObject::GetIGameTokenSystem() const
{
	return gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();
}

//-----------------------------------------------------------------------------------------------------

void CHUDObject::GetHUDObjectMemoryStatistics(ICrySizer * s)
{
}