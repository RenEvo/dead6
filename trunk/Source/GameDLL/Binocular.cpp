/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:12:2005   14:01 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Binocular.h"
#include "GameActions.h"

#include <IActorSystem.h>
#include <IMovementController.h>

#include "Game.h"
#include "HUD/HUD.h"
#include "HUD/HUDScopes.h"

//------------------------------------------------------------------------
void CBinocular::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	bool ok=true;
	if (CHUD *pHUD=g_pGame->GetHUD())
		ok=!pHUD->IsPDAActive();

	const SGameActions& actions = g_pGame->Actions();

	if (ok && (actionId == actions.zoom_in || actionId == actions.v_zoom_in))
	{
		if (m_zm && (m_zm->GetCurrentStep()<m_zm->GetMaxZoomSteps()) && m_zm->StartZoom(false, false))
			if(GetOwnerActor() == g_pGame->GetIGameFramework()->GetClientActor() && g_pGame->GetHUD())
				g_pGame->GetHUD()->PlaySound(ESound_BinocularsZoomIn);
	}
	else if (ok && (actionId == actions.zoom_out || actionId == actions.v_zoom_out))
	{
		if (m_zm && m_zm->ZoomOut())
			if(GetOwnerActor() == g_pGame->GetIGameFramework()->GetClientActor() && g_pGame->GetHUD())
				g_pGame->GetHUD()->PlaySound(ESound_BinocularsZoomOut);
	}
  else if (actionId == actions.attack1)
  { 
    if (activationMode == eAAM_OnPress)
    {      
      // trigger OnShoot in here.. Binocs don't have any firemode
      Vec3 pos(ZERO);
      Vec3 dir(FORWARD_DIRECTION);

      IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
      if (pActor)
      {
        IMovementController* pMC = pActor->GetMovementController();
        if (pMC)
        {
          SMovementState state;
          pMC->GetMovementState(state);          
          pos = state.pos;
          dir = state.eyeDirection;
        }
      }
      OnShoot(actorId, 0, 0, pos, dir, Vec3(ZERO));
    }
  }
	else
		CWeapon::OnAction(actorId, actionId, activationMode, value);
}

//------------------------------------------------------------------------
void CBinocular::Select(bool select)
{
	CWeapon::Select(select);

	if (!GetOwnerActor() || !GetOwnerActor()->IsClient())
		return;

	//if(gEnv->pSoundSystem)		//turn sound-zooming on / off
	//	gEnv->pSoundSystem->CalcDirectionalAttenuation(GetOwnerActor()->GetEntity()->GetWorldPos(), GetOwnerActor()->GetViewRotation().GetColumn1(), select?0.75f:0.0f);

	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->GetScopes()->ShowBinoculars(select);

	if (select && m_zm)
	{
		SetBusy(false);

		m_zm->StartZoom();
	}
}