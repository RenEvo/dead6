/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 28:10:2005   16:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Scope.h"
#include "Player.h"

#include "Game.h"
#include "HUD/HUD.h"
#include "HUD/HUDScopes.h"

//------------------------------------------------------------------------
void CScope::Update(float frameTime, uint frameId)
{
	CIronSight::Update(frameTime, frameId);

	if (m_showTimer>0.0f)
	{
		m_showTimer-=frameTime;
		if (m_showTimer<=0.0f)
		{
			m_showTimer=0.0f;
			m_pWeapon->Hide(false);
		}

		m_pWeapon->RequireUpdate(eIUS_Zooming);
	}
}

//------------------------------------------------------------------------
void CScope::ResetParams(const struct IItemParamsNode *params)
{
	CIronSight::ResetParams(params);

	const IItemParamsNode *scope = params?params->GetChild("scope"):0;
	m_scopeparams.Reset(scope);
}

//------------------------------------------------------------------------
void CScope::PatchParams(const struct IItemParamsNode *patch)
{
	CIronSight::PatchParams(patch);

	const IItemParamsNode *scope = patch->GetChild("scope");
	m_scopeparams.Reset(scope, false);
}

//------------------------------------------------------------------------
void CScope::Activate(bool activate)
{
	if (!activate)
	{
		if (m_zoomed || m_zoomTimer>0.0f)
		{
			if(	!strcmp(m_scopeparams.scope.c_str(),"scope_default") ||
					!strcmp(m_scopeparams.scope.c_str(),"scope_assault") ||
					!strcmp(m_scopeparams.scope.c_str(),"scope_sniper"))
			{
				if(g_pGame->GetHUD())
				{
					g_pGame->GetHUD()->GetScopes()->ShowScope(0);
				}
			}

/*			IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
			HSCRIPTFUNCTION func = 0;
			if (pScriptSystem->GetGlobalValue("WeaponResetScope", func))
			{
				Script::Call(pScriptSystem, func, m_scopeparams.scope.c_str());
				pScriptSystem->ReleaseFunc(func);
			}*/
		}
	}

	CIronSight::Activate(activate);
}

//------------------------------------------------------------------------
void CScope::OnZoomedIn()
{
	CIronSight::OnZoomedIn();

	m_pWeapon->Hide(true);
}

//------------------------------------------------------------------------
void CScope::OnEnterZoom()
{
	int iZoom = 0;
	if(!strcmp(m_scopeparams.scope.c_str(),"scope_default") || !strcmp(m_scopeparams.scope.c_str(),"scope_assault"))
	{
		iZoom = 1;
	}
	else if(!strcmp(m_scopeparams.scope.c_str(),"scope_sniper"))
	{
		iZoom = 2;
	}

	if(iZoom != 0)
	{
		if(g_pGame->GetHUD())
		{
			g_pGame->GetHUD()->GetScopes()->ShowScope(iZoom);
		}
	}


/*	IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
	HSCRIPTFUNCTION func = 0;
	if (pScriptSystem->GetGlobalValue("WeaponActivateScope", func))
	{
		Script::Call(pScriptSystem, func, m_scopeparams.scope.c_str(), ScriptHandle(m_pWeapon->GetEntityId()), true,
			m_scopeparams.dark_in_time, m_zoomparams.zoom_in_time);
		pScriptSystem->ReleaseFunc(func);
	}*/
}

//------------------------------------------------------------------------
void CScope::OnLeaveZoom()
{
	m_showTimer = 0.15f;

	if(	!strcmp(m_scopeparams.scope.c_str(),"scope_default") ||
			!strcmp(m_scopeparams.scope.c_str(),"scope_assault") ||	
			!strcmp(m_scopeparams.scope.c_str(),"scope_sniper"))
	{
		if(g_pGame->GetHUD())
		{
			g_pGame->GetHUD()->GetScopes()->ShowScope(0);
		}
	}

/*	IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
	HSCRIPTFUNCTION func = 0;
	if (pScriptSystem->GetGlobalValue("WeaponActivateScope", func))
	{
		Script::Call(pScriptSystem, func, m_scopeparams.scope.c_str(), ScriptHandle(m_pWeapon->GetEntityId()), false,
			m_scopeparams.dark_out_time, m_zoomparams.zoom_out_time);
		pScriptSystem->ReleaseFunc(func);
	}*/

	ClearDoF();
}

void CScope::OnZoomedOut()
{
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (pActor && pActor->GetScreenEffects() != 0)
	{
		pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomOutID);
		pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomInID);
		pActor->GetScreenEffects()->EnableBlends(true, pActor->m_autoZoomInID);
		pActor->GetScreenEffects()->EnableBlends(true, pActor->m_autoZoomOutID);
		pActor->GetScreenEffects()->EnableBlends(true, pActor->m_hitReactionID);
	}

	//Reset spread and recoil modifications
	IFireMode* pFireMode = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode());

	if(pFireMode)
	{
		pFireMode->ResetSpreadMod();
		pFireMode->ResetRecoilMod();
	}

}

void CScope::Serialize(TSerialize ser)
{
	CIronSight::Serialize(ser);

	if(ser.GetSerializationTarget()!=eST_Network)
	{
		ser.Value("m_showTimer", m_showTimer);
		if(m_zoomed && m_enabled && g_pGame->GetHUD())
			g_pGame->GetHUD()->GetScopes()->SetScopeZoomMode(m_currentStep, m_scopeparams.scope);
	}
}

void CScope::OnZoomStep(bool zoomingIn, float t)
{
	CIronSight::OnZoomStep(zoomingIn, t);
	if(	!strcmp(m_scopeparams.scope.c_str(),"scope_default") ||
			!strcmp(m_scopeparams.scope.c_str(),"scope_assault") ||
			!strcmp(m_scopeparams.scope.c_str(),"scope_sniper"))
	{
		if(g_pGame->GetHUD())
		{
			g_pGame->GetHUD()->GetScopes()->SetScopeZoomMode(m_currentStep, m_scopeparams.scope);
		}
	}
	else if(!strcmp(m_scopeparams.scope.c_str(),"scope_binoculars"))
	{
		if(g_pGame->GetHUD())
		{
			g_pGame->GetHUD()->GetScopes()->SetBinocularsZoomMode(m_currentStep);
		}
	}
}

void CScope::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	m_scopeparams.GetMemoryStatistics(s);
	CIronSight::GetMemoryStatistics(s);
}