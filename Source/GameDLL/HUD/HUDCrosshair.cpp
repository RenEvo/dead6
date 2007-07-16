//-----------------------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "HUDCrosshair.h"
#include "IWorldQuery.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "HUD.h"
#include "HUDVehicleInterface.h"

CHUDCrosshair::CHUDCrosshair(CHUD* pHUD) : g_pHUD(pHUD), m_bUsable(false)
{
	m_animCrossHair.Load("Libs/UI/HUD_Crosshair.gfx", eGFD_Center, eFAF_ManualRender);
	m_iFriendlyTarget = 0;
}

//-----------------------------------------------------------------------------------------------------

CHUDCrosshair::~CHUDCrosshair()
{
	m_animCrossHair.Unload();
}

//-----------------------------------------------------------------------------------------------------

void CHUDCrosshair::OnUpdate(float fDeltaTime,float fFadeValue)
{
	if(m_animCrossHair.GetVisible() && g_pGameCVars->hud_enablecrosshair)
	{
		m_animCrossHair.GetFlashPlayer()->Advance(fDeltaTime);
		m_animCrossHair.GetFlashPlayer()->Render();
	}

	UpdateCrosshair();
}

//-----------------------------------------------------------------------------------------------------

void CHUDCrosshair::SetUsability(int usable, const char* actionLabel, const char* param)
{
	m_bUsable = (usable>0)?true:false;
	m_animCrossHair.Invoke("setUsable", usable);
	if(actionLabel)
	{
		if(param)
			g_pHUD->DisplayFlashMessage(actionLabel, 4, Col_White, true, param);
		else
			g_pHUD->DisplayFlashMessage(actionLabel, 4);
	}
}

//-----------------------------------------------------------------------------------------------------

bool CHUDCrosshair::GetUsability() const
{
	return m_bUsable;
}

//-----------------------------------------------------------------------------------------------------

void CHUDCrosshair::CrosshairHit()
{
	m_animCrossHair.Invoke("setHit");
}

//-----------------------------------------------------------------------------------------------------

void CHUDCrosshair::SetCrosshair(int iCrosshair)
{
	if(!g_pGameCVars->hud_enablecrosshair || g_pGameCVars->g_difficultyLevel > 3)
		iCrosshair = 0;

	iCrosshair = MAX(0,iCrosshair);
	iCrosshair = MIN(10,iCrosshair);
	m_animCrossHair.Invoke("setCrossHair", iCrosshair);
	m_animCrossHair.Invoke("setUsable", m_bUsable);
}

//-----------------------------------------------------------------------------------------------------

void CHUDCrosshair::UpdateCrosshair()
{
  IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();  
  int iNewFriendly = 0;

  if (!gEnv->bMultiplayer && pClientActor && pClientActor->GetLinkedVehicle())
  { 
    // JanM/MichaelR: 
    // Get status from the VehicleWeapon, which raycasts considering the necessary SkipEntities (in contrast to WorldQuery)
    // this is only done in SP (so far)
    iNewFriendly = g_pHUD->GetVehicleInterface()->GetFriendlyFire();    
  }
  else
  {
    EntityId uiCenterId = pClientActor->GetGameObject()->GetWorldQuery()->GetLookAtEntityId();
    IVehicle *pCenterVehicle = uiCenterId ? g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(uiCenterId) : NULL;    
    IUIDraw *pUIDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();

    if(!gEnv->bMultiplayer)
    {
      if(uiCenterId)
      {
        if(pCenterVehicle)
        {
          IVehicleSeat *pVehicleSeat = pCenterVehicle->GetSeatById(1);
          if(pVehicleSeat)
          {
            iNewFriendly = g_pHUD->IsFriendlyToClient(pVehicleSeat->GetPassenger());
          }
        }
        else
        {
          iNewFriendly = g_pHUD->IsFriendlyToClient(uiCenterId);
        }
      }
    }
    else
    {
      CGameRules *pGameRules = g_pGame->GetGameRules();

      if(!pGameRules)
      {
        GameWarning("[HUD]: No game rules!");
        return;
      }

      IActor *pCenterActor = uiCenterId ? g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(uiCenterId) : NULL;
      int iClientTeam = pGameRules->GetTeam(pClientActor->GetEntityId());

      if(pCenterActor && pCenterActor->IsPlayer())
      {
        if(iClientTeam && (pGameRules->GetTeam(uiCenterId) == iClientTeam))
        {
          iNewFriendly = 1;
        }
      }
      else if(pCenterVehicle)
      {
        IVehicleSeat *pVehicleSeat = pCenterVehicle->GetSeatById(1);
        if(pVehicleSeat)
        {
          EntityId uiDriverId = pVehicleSeat->GetPassenger();
          if(uiDriverId)
          {
            if(iClientTeam && (pGameRules->GetTeam(uiDriverId) == iClientTeam))
            {
              iNewFriendly = 1;
            }
          }
        }
      }
    }
  }	

	if(iNewFriendly != m_iFriendlyTarget)
	{
		m_iFriendlyTarget = iNewFriendly;
		m_animCrossHair.Invoke("setFriendly", m_iFriendlyTarget);
	}
}

//-----------------------------------------------------------------------------------------------------
