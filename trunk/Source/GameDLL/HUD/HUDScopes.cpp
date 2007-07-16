/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Binocular/Scopes HUD object (refactored from old HUD code)

-------------------------------------------------------------------------
History:
- 17:04:2007  17:30 : Created by Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "HUDScopes.h"
#include "HUD.h"
#include "GameFlashAnimation.h"
#include "../Actor.h"
#include "IWorldQuery.h"
#include "GameRules.h"
#include "GameCVars.h"

#define HUD_CALL_LISTENERS(func) \
{ \
	if (g_pHUD->m_hudListeners.empty() == false) \
	{ \
	g_pHUD->m_hudTempListeners = g_pHUD->m_hudListeners; \
	for (std::vector<CHUD::IHUDListener*>::iterator tmpIter = g_pHUD->m_hudTempListeners.begin(); tmpIter != g_pHUD->m_hudTempListeners.end(); ++tmpIter) \
	(*tmpIter)->func; \
	} \
}

CHUDScopes::CHUDScopes(CHUD *pHUD) : g_pHUD(pHUD)
{
	m_eShowScope = ESCOPE_NONE;
	m_fBinocularDistance = 0.0f;
	m_bShowBinoculars = false;
	m_bShowBinocularsNoHUD = false;
	m_oldScopeZoomLevel = 0;
	m_bThirdPerson = false;
	LoadFlashFiles();
}

CHUDScopes::~CHUDScopes()
{
	m_animBinoculars.Unload();
	m_animSniperScope.Unload();
	m_animBinocularsEnemyIndicator.Unload();
}

void CHUDScopes::LoadFlashFiles(bool force)
{
//	m_animAssaultScope.Load("Libs/UI/HUD_ScopeAssault.gfx", eGFD_Center, eFAF_ManualRender);
	m_animSniperScope.Load("Libs/UI/HUD_ScopeSniper.gfx", eGFD_Center, eFAF_ManualRender);
	// Everything which is overlapped by the binoculars / scopes MUST be created before them
	if(force)
	{
		m_animBinoculars.Load("Libs/UI/HUD_Binoculars.gfx",eGFD_Center,eFAF_ManualRender);
		m_animBinocularsEnemyIndicator.Load("Libs/UI/HUD_Binoculars_EnemyIndicator.gfx",eGFD_Center,eFAF_ManualRender);
	}
	else
	{
		m_animBinoculars.Init("Libs/UI/HUD_Binoculars.gfx",eGFD_Center,eFAF_ManualRender);
		m_animBinocularsEnemyIndicator.Init("Libs/UI/HUD_Binoculars_EnemyIndicator.gfx",eGFD_Center,eFAF_ManualRender);
	}
}

void CHUDScopes::OnUpdate(float fDeltaTime,float fFadeValue)
{
/*	if(m_animAssaultScope.GetVisible())
	{
		m_animAssaultScope.GetFlashPlayer()->Advance(fDeltaTime);
		m_animAssaultScope.GetFlashPlayer()->Render();
	}*/
	if(m_animSniperScope.GetVisible())
	{
		m_animSniperScope.GetFlashPlayer()->Advance(fDeltaTime);
		m_animSniperScope.GetFlashPlayer()->Render();
	}
	if(m_animBinoculars.GetVisible())
	{
		m_animBinoculars.GetFlashPlayer()->Advance(fDeltaTime);
		m_animBinoculars.GetFlashPlayer()->Render();
	}
	if(m_animBinocularsEnemyIndicator.GetVisible())
	{
		m_animBinocularsEnemyIndicator.GetFlashPlayer()->Advance(fDeltaTime);
		m_animBinocularsEnemyIndicator.GetFlashPlayer()->Render();
	}
}

void CHUDScopes::DisplayBinoculars(CPlayer* pPlayerActor)
{
	SMovementState sMovementState;
	pPlayerActor->GetMovementController()->GetMovementState(sMovementState);

	char strY[32];
	sprintf(strY,"%f",336.0f+sMovementState.eyeDirection.z*360.0f);

	m_animBinoculars.CheckedSetVariable("Root.Binoculars.Attitude._y",strY);

	char strN[32];
	char strW[32];
	g_pHUD->GetGPSPosition(&sMovementState,strN,strW);

	SFlashVarValue args[2] = {strN, strW};
	m_animBinoculars.Invoke("setPosition", args, 2);

	IAIObject *pAIPlayer = pPlayerActor->GetEntity()->GetAI();
	if(!pAIPlayer && !gEnv->bMultiplayer)
		return;

	const std::vector<EntityId> *entitiesInProximity = g_pHUD->m_pHUDRadar->GetNearbyEntities();

	const std::deque<CHUDRadar::RadarEntity> *pEntitiesOnRadar = g_pHUD->GetRadar()->GetEntitiesList();

	if(!entitiesInProximity->empty() || !pEntitiesOnRadar->empty())
	{
		m_animBinocularsEnemyIndicator.SetVisible(true);
		m_animBinocularsEnemyIndicator.SetVariable("Root._visible",SFlashVarValue(true));

		// TODO: amazing copy/paste here !!!

		float fMovieWidth		= (float) m_animBinocularsEnemyIndicator.GetFlashPlayer()->GetWidth();
		float fMovieHeight	= (float) m_animBinocularsEnemyIndicator.GetFlashPlayer()->GetHeight();

		float fRendererWidth	= (float) gEnv->pRenderer->GetWidth();
		float fRendererHeight	= (float) gEnv->pRenderer->GetHeight();

		float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
		float fScaleY = fMovieHeight / 100.0f;
		float fScale = fMovieHeight / fRendererHeight;
		float fUselessSize = fMovieWidth - fRendererWidth * fScale;
		float fHalfUselessSize = fUselessSize * 0.5f;

		std::vector<double> entityValues;
		std::map<EntityId, bool> drawnEntities;

		for(std::deque<CHUDRadar::RadarEntity>::const_iterator iter=pEntitiesOnRadar->begin(); iter!=pEntitiesOnRadar->end(); ++iter)
		{

			EntityId uiEntityId = (*iter).m_id;

			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(uiEntityId);
			if(!pEntity)
				continue;

			// Do not display vehicles
			IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
			if(pVehicle)
				continue;

			IAIObject *pAIObject = pEntity->GetAI();
			if(!pAIObject)
				continue;

			// Display only enemies
			if(!pAIObject->IsHostile(pAIPlayer,false))
				continue;

			if(!pAIObject->IsEnabled())
				continue;

			if(g_pHUD->ShowLockingBrackets(uiEntityId, &entityValues))
			{
				entityValues.push_back(true);
			}
			drawnEntities[uiEntityId] = true;
		}

		if(g_pGameCVars->g_difficultyLevel < 3)
		{
			for(std::vector<EntityId>::const_iterator iter=entitiesInProximity->begin(); iter!=entitiesInProximity->end(); ++iter)
			{
				EntityId uiEntityId = (*iter);

				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(uiEntityId);
				if(!pEntity)
					continue;

				// Do not display vehicles
				IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
				if(pVehicle)
					continue;

				if(stl::find_in_map(drawnEntities, uiEntityId, false))
					continue;

				if(!gEnv->bMultiplayer)
				{
					IAIObject *pAIObject = pEntity->GetAI();
					if(!pAIObject)
						continue;

					// Display only enemies
					if(!pAIObject->IsHostile(pAIPlayer,false))
						continue;

					if(!pAIObject->IsEnabled())
						continue;
				}
				else
				{
					if(g_pGame->GetGameRules()->GetTeam(pPlayerActor->GetEntityId()) == g_pGame->GetGameRules()->GetTeam(pEntity->GetId()))
						continue;
				}

				if(g_pHUD->ShowLockingBrackets(*iter, &entityValues))
				{
					entityValues.push_back(false);
				}
			}
		}


		if(!entityValues.empty())
		{
			m_animBinocularsEnemyIndicator.GetFlashPlayer()->SetVariableArray(FVAT_Double, "m_allValues", 0, &entityValues[0], entityValues.size());
			m_animBinocularsEnemyIndicator.Invoke("updateLockBrackets");
		}

	}
	else
	{
		m_animBinocularsEnemyIndicator.SetVisible(false);
	}
}

void CHUDScopes::DisplayScope(CPlayer* pPlayerActor)
{
	CGameFlashAnimation *pScope = NULL;
	if(m_eShowScope==ESCOPE_SNIPER)
		pScope = &m_animSniperScope;
//	else if(m_eShowScope==ESCOPE_ASSAULT)
//		pScope = &m_animAssaultScope;

	if(pScope)
	{
		SMovementState sMovementState;
		pPlayerActor->GetMovementController()->GetMovementState(sMovementState);

		char strY[32];
		sprintf(strY,"%f",384.0f+sMovementState.eyeDirection.z*360.0f);

		//pScope->CheckedSetVariable("Root.Scope.Attitude._y",strY);

		const ray_hit *pRay = pPlayerActor->GetGameObject()->GetWorldQuery()->GetLookAtPoint(500.0f);

		if(pRay)
		{
			char strDistance[32];
			sprintf(strDistance,"%.1f",pRay->dist);

			pScope->Invoke("setDistance", strDistance);
		}
		else
		{
			pScope->Invoke("setDistance", "- - - - - - ");
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDScopes::ShowBinoculars(bool bVisible, bool bShowIfNoHUD)
{
	if(bVisible)
	{
		m_animBinoculars.Reload();
		m_animBinocularsEnemyIndicator.Reload();

		SFlashVarValue args[3] = {bVisible, 1, m_bThirdPerson};
		m_animBinoculars.Invoke("setVisible", args, 3);
		m_bShowBinoculars = bVisible;
		m_bShowBinocularsNoHUD = bShowIfNoHUD;
		g_pHUD->SetAirStrikeBinoculars(bVisible);
		g_pHUD->PlaySound(ESound_BinocularsSelect, true);
		g_pHUD->PlaySound(ESound_BinocularsAmbience, true);
	}
	else /* if(!bVisible) */
	{
		g_pHUD->PlaySound(ESound_BinocularsDeselect, true);
		g_pHUD->PlaySound(ESound_BinocularsAmbience, false);
		m_bShowBinoculars = bVisible;
		m_bShowBinocularsNoHUD = false;
		g_pHUD->SetAirStrikeBinoculars(bVisible);
		m_animBinoculars.Unload();
		m_animBinocularsEnemyIndicator.Unload();
	}
	HUD_CALL_LISTENERS(OnBinoculars(bVisible));
}

//-----------------------------------------------------------------------------------------------------

void CHUDScopes::SetBinocularsDistance(float fDistance)
{
	SFlashVarValue args[2] = {(int) fDistance, (int) (((fDistance-(int) fDistance))*100.0f)};
	m_animBinoculars.Invoke("setDistance", args, 2);
	m_fBinocularDistance = fDistance;
}

//-----------------------------------------------------------------------------------------------------

void CHUDScopes::SetBinocularsZoomMode(int iZoomMode)
{
	m_animBinoculars.Invoke("setZoomMode", iZoomMode);
}

//-----------------------------------------------------------------------------------------------------

void CHUDScopes::ShowScope(int iVisible)
{
	if(iVisible==ESCOPE_NONE)
	{
//		m_animAssaultScope.Invoke("setVisible", false);
		m_animSniperScope.Invoke("setVisible", false);
		m_eShowScope = ESCOPE_NONE;
		return;
	}

//	m_animAssaultScope.Invoke("setVisible", false);
	SFlashVarValue args[2] = {true, iVisible};
	m_animSniperScope.Invoke("setVisible", args, 2);
	m_eShowScope = (EScopeMode)iVisible;
	m_oldScopeZoomLevel = 0;
}

//-----------------------------------------------------------------------------------------------------

void CHUDScopes::SetScopeZoomMode(int iZoomMode, string &scopeType)
{
	int type = 1;
	if(!stricmp(scopeType.c_str(), "scope_assault"))	//assault scope has no sound atm.
		type = 0;
	else if(!stricmp(scopeType.c_str(), "scope_sniper"))
		type = 2;

	if(type)
	{
		if(iZoomMode > m_oldScopeZoomLevel)
			g_pHUD->PlaySound((type==2)?ESound_SniperZoomIn:ESound_BinocularsZoomIn);
		else
			g_pHUD->PlaySound((type==2)?ESound_SniperZoomOut:ESound_BinocularsZoomOut);
	}

	m_oldScopeZoomLevel = iZoomMode;

//	m_animAssaultScope.Invoke("setZoomMode", iZoomMode);
	m_animSniperScope.Invoke("setZoomMode", iZoomMode);
}

void CHUDScopes::OnToggleThirdPerson(bool thirdPerson)
{
	SFlashVarValue args[3] = {IsBinocularsShown(), 0, thirdPerson};
	m_animBinoculars.Invoke("setVisible", args, 3);

	m_bThirdPerson = thirdPerson;

	if(m_eShowScope==ESCOPE_NONE)
	{
		SFlashVarValue args[3] = {false, 0, thirdPerson};
//		m_animAssaultScope.Invoke("setVisible", args, 3);
		m_animSniperScope.Invoke("setVisible", args, 3);
	}
	else if(m_eShowScope==ESCOPE_ASSAULT)
	{
		SFlashVarValue args[3] = {true, 0, thirdPerson};
//		m_animAssaultScope.Invoke("setVisible", args, 3);
		args[0] = false;
		m_animSniperScope.Invoke("setVisible", args, 3);
	}
	else if(m_eShowScope==ESCOPE_SNIPER)
	{
		SFlashVarValue args[3] = {false, 0, thirdPerson};
//		m_animAssaultScope.Invoke("setVisible", args, 3);
		args[0] = true;
		m_animSniperScope.Invoke("setVisible",args, 3);
	}
}
