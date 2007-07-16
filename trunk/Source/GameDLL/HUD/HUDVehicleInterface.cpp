/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Vehicle HUD object (refactored from old HUD code)

-------------------------------------------------------------------------
History:
- 21:02:2007  16:00 : Created by Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "HUDVehicleInterface.h"
#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "HUD.h"
#include "Weapon.h"
#include "IWorldQuery.h"

CHUDVehicleInterface::CHUDVehicleInterface(CHUD *pHUD, CGameFlashAnimation *pAmmo) : m_pVehicle(NULL)
{
	m_bParachute = false;
	m_bThirdPerson = false;	
	m_eCurVehicleHUD = EHUD_NONE;
	g_pHUD = pHUD;
	g_pAmmo = pAmmo;  
	m_lastSetFriendly = m_friendlyFire = false;
  m_seatId = InvalidVehicleSeatId;
  
	m_animMainWindow.Init("Libs/UI/HUD_VehicleHUD.gfx", eGFD_Center, eFAF_ManualRender|eFAF_Visible);
	m_animStats.Init("Libs/UI/HUD_VehicleStats.gfx", eGFD_Center, eFAF_ManualRender|eFAF_Visible);

	memset(m_hasMainHUD, 0, (int)EHUD_LAST);

	//fill "hasMainHUD" list
	m_hasMainHUD[EHUD_TANKUS] = true;
	m_hasMainHUD[EHUD_AAA] = true;
	m_hasMainHUD[EHUD_HELI] = true;
	m_hasMainHUD[EHUD_VTOL] = true;
	m_hasMainHUD[EHUD_LTV] = false;
	m_hasMainHUD[EHUD_APC] = true;
	m_hasMainHUD[EHUD_APC2] = true;
	m_hasMainHUD[EHUD_SMALLBOAT] = true;
	m_hasMainHUD[EHUD_PATROLBOAT] = true;
	m_hasMainHUD[EHUD_CIVCAR] = false;
	m_hasMainHUD[EHUD_CIVBOAT] = false;
	m_hasMainHUD[EHUD_TRUCK] = false;
	m_hasMainHUD[EHUD_HOVER] = true;
	m_hasMainHUD[EHUD_PARACHUTE ] = true;
	m_hasMainHUD[EHUD_TANKA] = true;

	m_hudTankNames["US_tank"] = "M5A2 Atlas";
	m_hudTankNames["Asian_tank"] = "NK T-108";
	m_hudTankNames["Asian_aaa"] = "NK AAA";
	m_hudTankNames["US_apc"] = "US APC";
}

//-----------------------------------------------------------------------------------------------------

CHUDVehicleInterface::~CHUDVehicleInterface()
{
	if(m_pVehicle)
	{
		m_pVehicle->UnregisterVehicleEventListener(this);
		m_pVehicle = NULL;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::OnUpdate(float fDeltaTime,float fFadeValue)
{
	if(m_animMainWindow.GetVisible())
	{
		if(m_friendlyFire != m_lastSetFriendly)
		{
			m_animMainWindow.Invoke("setFriendly", m_friendlyFire);
			m_lastSetFriendly = m_friendlyFire;
		}

		m_animMainWindow.GetFlashPlayer()->Advance(fDeltaTime);
		m_animMainWindow.GetFlashPlayer()->Render();
	}
	
	if(m_animStats.GetVisible())
	{
		m_animStats.GetFlashPlayer()->Advance(fDeltaTime);
		m_animStats.GetFlashPlayer()->Render();
	}
	g_pHUD->UpdateCrosshairVisibility();
}

//-----------------------------------------------------------------------------------------------------

bool CHUDVehicleInterface::ForceCrosshair()
{
  if (m_pVehicle && m_seatId != InvalidVehicleSeatId)
  {
    if (IVehicleSeat* pSeat = m_pVehicle->GetSeatById(m_seatId))
    {
      return pSeat->IsGunner() && !m_animMainWindow.GetVisible();
    }
  }

  return false;
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::ChooseVehicleHUD()
{
	if(m_bParachute)
	{
		m_eCurVehicleHUD = EHUD_PARACHUTE;
		return;
	}

	if(!m_pVehicle)
	{
		m_eCurVehicleHUD = EHUD_NONE;
		return;
	}

	IEntityClass *cls = m_pVehicle->GetEntity()->GetClass();
	CHUDRadar *radar = g_pHUD->GetRadar();

	if(!cls || !radar)
	{
		m_eCurVehicleHUD = EHUD_NONE;
		return;
	}

	if(cls == radar->m_pTankUS)
	{
		m_eCurVehicleHUD = EHUD_TANKUS;
	}
	else if(cls == radar->m_pTankA)
	{
		m_eCurVehicleHUD = EHUD_TANKA;
	}
	else if(cls == radar->m_pAAA)
	{
		m_eCurVehicleHUD = EHUD_AAA;
	}
	else if(cls == radar->m_pVTOL)
	{
		m_eCurVehicleHUD = EHUD_VTOL;
	}
	else if(cls == radar->m_pHeli)
	{
		m_eCurVehicleHUD = EHUD_HELI;
	}
	else if(cls == radar->m_pLTVA || cls == radar->m_pLTVUS)
	{
		m_eCurVehicleHUD = EHUD_LTV;
	}
	else if(cls == radar->m_pAPCUS)
	{
		m_eCurVehicleHUD = EHUD_APC;
	}
	else if(cls == radar->m_pAPCA)
	{
		m_eCurVehicleHUD = EHUD_APC2;
	}
	else if(cls == radar->m_pTruck)
	{
		m_eCurVehicleHUD = EHUD_TRUCK;
	}
	else if(cls == radar->m_pBoatCiv)
	{
		m_eCurVehicleHUD = EHUD_CIVBOAT;
	}
	else if(cls == radar->m_pCarCiv)
	{
		m_eCurVehicleHUD = EHUD_CIVCAR;
	}
	else if(cls == radar->m_pBoatUS)
	{
		m_eCurVehicleHUD = EHUD_SMALLBOAT;
	}
	else if(cls == radar->m_pBoatA)
	{
		m_eCurVehicleHUD = EHUD_PATROLBOAT;
	}
	else if(cls == radar->m_pHover)
	{
		m_eCurVehicleHUD = EHUD_HOVER;
	}
	else
		m_eCurVehicleHUD = EHUD_NONE;
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::OnEnterVehicle(IActor *pActor,const char *szVehicleClassName,const char *szSeatName,bool bThirdPerson)
{
	m_bParachute = (bool) (!strcmpi(szVehicleClassName,"Parachute"));
	if(m_bParachute)
	{
		bool open = (bool)(!strcmpi(szSeatName,"Open"));
		m_animStats.Reload();
		CRY_ASSERT_MESSAGE(NULL == m_pVehicle,"Attempt to enter in parachute while already in a vehicle!");
		m_animStats.Invoke("setActiveParachute", open);
		if(!open || !m_animMainWindow.GetVisible())
			OnEnterVehicle(static_cast<CPlayer*> (pActor),bThirdPerson);
	}
	else
		OnEnterVehicle(static_cast<CPlayer*> (pActor),bThirdPerson);
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::OnEnterVehicle(CPlayer *pPlayer,bool bThirdPerson)
{
	if (!pPlayer || !pPlayer->IsClient())
		return;

	if(m_pVehicle)
	{
		GameWarning("[HUD]: Attempt to enter a vehicle while already in one!");
		return;
	}

	m_bThirdPerson = bThirdPerson;

	m_pVehicle = pPlayer->GetLinkedVehicle();
  m_seatId = InvalidVehicleSeatId;
  
	if(m_pVehicle)
	{
		m_pVehicle->RegisterVehicleEventListener(this);
    
		if (IVehicleSeat *seat = m_pVehicle->GetSeatForPassenger(pPlayer->GetEntityId()))
		{ 
      m_seatId = seat->GetSeatId();
			g_pHUD->UpdateCrosshairVisibility();
		}
	}

	//choose vehicle hud
	ChooseVehicleHUD();
	LoadVehicleHUDs();

	//setup flash hud

	InitVehicleHuds();

	g_pHUD->UpdateCrosshairVisibility();
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::LoadVehicleHUDs(bool forceEverything)
{
	if(m_hasMainHUD[m_eCurVehicleHUD] || forceEverything)
		m_animMainWindow.Reload();
	m_animStats.Reload();
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::OnExitVehicle(IActor *pActor)
{
	if(m_pVehicle)
	{
		m_pVehicle->UnregisterVehicleEventListener(this);
		m_pVehicle = NULL;
	}
	else
		m_bParachute = false;

  m_seatId = InvalidVehicleSeatId;

	if(m_eCurVehicleHUD!=EHUD_NONE)
	{
		HideVehicleInterface();
		m_eCurVehicleHUD = EHUD_NONE;
	}

	m_animMainWindow.Unload();
	m_animStats.Unload();

	g_pHUD->UpdateCrosshairVisibility();
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::InitVehicleHuds()
{
	if(m_eCurVehicleHUD != EHUD_NONE)
	{
		const char *szVehicleClassName = NULL;

		if(m_pVehicle)
		{
			szVehicleClassName = m_pVehicle->GetEntity()->GetClass()->GetName();
		}

		m_animMainWindow.Invoke("showHUD");
		m_animMainWindow.Invoke("setVehicleHUDMode", (int)m_eCurVehicleHUD);
		m_animMainWindow.SetVisible(true);

		m_animStats.Invoke("showStats");
		m_animStats.Invoke("setVehicleStatsMode", (int)m_eCurVehicleHUD);
		m_animStats.SetVisible(true);

		UpdateVehicleHUDDisplay();
		UpdateDamages();

		if(szVehicleClassName)
		{
			string name = m_hudTankNames[szVehicleClassName];

			if(g_pAmmo)
				g_pAmmo->Invoke("setTankName", name.c_str());
		}
		m_statsSpeed = -999;
		m_statsHeading = -999;
		m_lastSetFriendly = false;
		m_friendlyFire = false;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::UpdateDamages()	//this could be put in an xml file for better mod-ability
{
	if(!m_pVehicle || !m_pVehicle->GetEntity())
		return;
	if(m_eCurVehicleHUD == EHUD_TANKA || m_eCurVehicleHUD == EHUD_TANKUS || m_eCurVehicleHUD == EHUD_APC)
	{
		IEntityClass *cls = m_pVehicle->GetEntity()->GetClass();
		if(cls == g_pHUD->GetRadar()->m_pTankA || cls == g_pHUD->GetRadar()->m_pTankUS || cls == g_pHUD->GetRadar()->m_pAPCUS)
		{
			float fH = m_pVehicle->GetComponent("hull")->GetDamageRatio();
			float fE = m_pVehicle->GetComponent("engine")->GetDamageRatio();
			float fL = m_pVehicle->GetComponent("leftTread")->GetDamageRatio();
			float fR = m_pVehicle->GetComponent("rightTread")->GetDamageRatio();
			float fT = m_pVehicle->GetComponent("turret")->GetDamageRatio();
			SFlashVarValue args[5] = {fH, fE, fL, fR, fT};
			m_animStats.Invoke("setDamage", args, 5);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_APC2)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pAPCA)
		{
			float fH = m_pVehicle->GetComponent("hull")->GetDamageRatio();
			float fE = m_pVehicle->GetComponent("engine")->GetDamageRatio();
			float fT = m_pVehicle->GetComponent("turret")->GetDamageRatio();
			/*float fW1 = m_pVehicle->GetComponent("wheel1")->GetDamageRatio();
			float fW2 = m_pVehicle->GetComponent("wheel2")->GetDamageRatio();
			float fW3 = m_pVehicle->GetComponent("wheel3")->GetDamageRatio();
			float fW4 = m_pVehicle->GetComponent("wheel4")->GetDamageRatio();
			float fW5 = m_pVehicle->GetComponent("wheel5")->GetDamageRatio();
			float fW6 = m_pVehicle->GetComponent("wheel6")->GetDamageRatio();
			float fW7 = m_pVehicle->GetComponent("wheel7")->GetDamageRatio();
			float fW8 = m_pVehicle->GetComponent("wheel8")->GetDamageRatio();*/
			SFlashVarValue args[3/*11*/] = {fH, fE, fT/*, fW1, fW2, fW3, fW4, fW5, fW6, fW7, fW8*/};
			m_animStats.Invoke("setDamage", args, 3/*11*/);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_AAA)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pAAA)
		{
			float hull = m_pVehicle->GetComponent("hull")->GetDamageRatio();
			float turret = m_pVehicle->GetComponent("turret")->GetDamageRatio();
			float radar = m_pVehicle->GetComponent("radar")->GetDamageRatio();
			SFlashVarValue args[3] = {hull, turret, radar};
			m_animStats.Invoke("setDamage", args, 3);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_VTOL)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pVTOL)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			float fL = m_pVehicle->GetComponent("WingLeft")->GetDamageRatio();
			float fR = m_pVehicle->GetComponent("WingRight")->GetDamageRatio();
			SFlashVarValue args[3] = {fH, fL, fR};
			m_animStats.Invoke("setDamage", args, 3);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_HELI)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pHeli)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			float fR = m_pVehicle->GetComponent("Rotor")->GetDamageRatio();
			float fE = m_pVehicle->GetComponent("BackRotor")->GetDamageRatio();
			float fC = m_pVehicle->GetComponent("Cockpit")->GetDamageRatio();
			SFlashVarValue args[4] = {fH, fR, fE, fC};
			m_animStats.Invoke("setDamage", args, 4);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_LTV)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pLTVA || m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pLTVUS)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			float fE = m_pVehicle->GetComponent("Engine")->GetDamageRatio();
			float fT = m_pVehicle->GetComponent("FuelCan")->GetDamageRatio();
			float fW1 = m_pVehicle->GetComponent("wheel1")->GetDamageRatio();
			float fW2 = m_pVehicle->GetComponent("wheel2")->GetDamageRatio();
			float fW3 = m_pVehicle->GetComponent("wheel3")->GetDamageRatio();
			float fW4 = m_pVehicle->GetComponent("wheel4")->GetDamageRatio();
			SFlashVarValue args[7] = {fH, fE, fT, fW1, fW2, fW3, fW4};
			m_animStats.Invoke("setDamage", args, 7);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_TRUCK)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pTruck)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			float fE = m_pVehicle->GetComponent("Engine")->GetDamageRatio();
			float fT1 = m_pVehicle->GetComponent("LeftFuelTank")->GetDamageRatio();
			float fT2 = m_pVehicle->GetComponent("RightFuelTank")->GetDamageRatio();
			float fW1 = m_pVehicle->GetComponent("wheel1")->GetDamageRatio();
			float fW2 = m_pVehicle->GetComponent("wheel2")->GetDamageRatio();
			float fW3 = m_pVehicle->GetComponent("wheel3")->GetDamageRatio();
			float fW4 = m_pVehicle->GetComponent("wheel4")->GetDamageRatio();
			float fW5 = m_pVehicle->GetComponent("wheel5")->GetDamageRatio();
			float fW6 = m_pVehicle->GetComponent("wheel6")->GetDamageRatio();
			SFlashVarValue args[10] = {fH, fE, fT1, fT2, fW1, fW2, fW3, fW4, fW5, fW6};
			m_animStats.Invoke("setDamage", args, 10);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_SMALLBOAT || m_eCurVehicleHUD == EHUD_CIVBOAT)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pBoatUS || m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pBoatCiv)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			float fL = m_pVehicle->GetComponent("leftEngine")->GetDamageRatio();
			float fR = m_pVehicle->GetComponent("rightEngine")->GetDamageRatio();
			//m_animStats.Invoke("engines", 0);

			SFlashVarValue args[3] = {fH, fL, fR};
			m_animStats.Invoke("setDamage", args, 3);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_PATROLBOAT)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pBoatA)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			SFlashVarValue args[3] = {fH, 0, 0};
			m_animStats.Invoke("setDamage", args, 3);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_HOVER)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pHover)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			SFlashVarValue args[3] = {fH, 0, 0};
			m_animStats.Invoke("setDamage", args, 3);
		}
	}
	else if(m_eCurVehicleHUD == EHUD_CIVCAR)
	{
		if(m_pVehicle->GetEntity()->GetClass() == g_pHUD->GetRadar()->m_pCarCiv)
		{
			float fH = m_pVehicle->GetComponent("Hull")->GetDamageRatio();
			float fE = m_pVehicle->GetComponent("Engine")->GetDamageRatio();
			float fW1 = m_pVehicle->GetComponent("wheel1")->GetDamageRatio();
			float fW2 = m_pVehicle->GetComponent("wheel2")->GetDamageRatio();
			float fW3 = m_pVehicle->GetComponent("wheel3")->GetDamageRatio();
			float fW4 = m_pVehicle->GetComponent("wheel4")->GetDamageRatio();
			SFlashVarValue args[6] = {fH,fE, fW1, fW2, fW3, fW4};
			m_animStats.Invoke("setDamage", args, 6);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::UpdateSeats()
{
	if(!m_pVehicle)
		return;
	int seatCount = m_pVehicle->GetSeatCount();
	for(int i = 1; i <= seatCount; ++i)
	{
		IVehicleSeat *pSeat = m_pVehicle->GetSeatById(TVehicleSeatId(i));
		if(pSeat)
		{
			EntityId passenger = pSeat->GetPassenger();

			//set seats in flash
			if(m_eCurVehicleHUD)
			{
				SFlashVarValue args[2] = {i, 0};
				if(passenger)
				{
					args[1] = (passenger == gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId())?2:1;
				}
				m_animStats.Invoke("setSeat", args, 2);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
	if(eVE_VehicleDeleted == event)  
  {
		m_pVehicle = NULL;
    m_seatId = InvalidVehicleSeatId;
  }

	if (!m_pVehicle)
		return;

	const char *strVehicleClassName = m_pVehicle->GetEntity()->GetClass()->GetName();

	CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayerActor)
		return;

	if(eVE_PassengerEnter == event || eVE_PassengerChangeSeat == event)
	{
		if(params.entityId == pPlayerActor->GetEntityId())
		{ 
      m_seatId = params.iParam;
			
      UpdateVehicleHUDDisplay();
		}
    
    if (eVE_PassengerChangeSeat == event)
    { 
      if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.entityId))
      {
        IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)pEntity->GetProxy(ENTITY_PROXY_SOUND);
        if (pSoundProxy)      
          pSoundProxy->PlaySound("sounds/physics:player_foley:switch_seat", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, 0);      
      }
    }

		UpdateSeats();
	}
	else if(eVE_Damaged == event || eVE_Collision == event)
	{
		UpdateDamages();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::UpdateVehicleHUDDisplay()
{
	CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayerActor)
		return;

	IVehicleSeat *seat = NULL;
	if(m_pVehicle)
	{
		seat = m_pVehicle->GetSeatForPassenger(pPlayerActor->GetEntityId());
		if(!seat)
			return;
	}
  
	ChooseVehicleHUD();

	if(!g_pAmmo)
		return;

	if((seat && seat->IsDriver()) || m_bParachute)
	{
		if(m_bThirdPerson)
		{
			m_animMainWindow.Invoke("hideHUD");
			m_animMainWindow.SetVisible(false);
		}
		else
		{
			m_animMainWindow.Invoke("showHUD");
			m_animMainWindow.SetVisible(true);
		}

		if(m_pVehicle && m_pVehicle->GetWeaponCount() > 1)
			g_pAmmo->Invoke("showReloadDuration2");
		else
			g_pAmmo->Invoke("hideReloadDuration2");
		SFlashVarValue args[2] = {m_eCurVehicleHUD, 1};
		g_pAmmo->Invoke("setAmmoMode", args, 2);
	}
	else if(seat && seat->IsGunner())
	{
		m_animMainWindow.Invoke("hideHUD");
		m_animMainWindow.SetVisible(false);
		SFlashVarValue args[2] = {m_eCurVehicleHUD, 1};
		g_pAmmo->Invoke("setAmmoMode", args, 2);
		g_pAmmo->Invoke("hideReloadDuration2");
	}
	else
	{
		m_animMainWindow.Invoke("hideHUD");
		m_animMainWindow.SetVisible(false);
		SFlashVarValue args[2] = {0, 1};
		g_pAmmo->Invoke("setAmmoMode", args, 2);
	}
	
	g_pHUD->UpdateCrosshairVisibility();
}

//-----------------------------------------------------------------------------------------------------

float CHUDVehicleInterface::GetVehicleSpeed()
{
	float fSpeed = 0.0;
	if(m_pVehicle)
	{
		fSpeed = m_pVehicle->GetStatus().speed;
	}
	else
	{
		CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayerActor)
		{
			fSpeed = pPlayerActor->GetActorStats()->velocity.len();
		}
	}
	fSpeed *= 2.24f; // Meter per second TO Miles hour
	return fSpeed;
}

//-----------------------------------------------------------------------------------------------------

float CHUDVehicleInterface::GetVehicleHeading()
{
	float fAngle = 0.0;
	if(m_pVehicle)
	{
		SMovementState sMovementState;
		m_pVehicle->GetMovementController()->GetMovementState(sMovementState);
		Vec3 vEyeDirection = sMovementState.eyeDirection;
		vEyeDirection.z = 0.0f;
		vEyeDirection.normalize();
		fAngle = RAD2DEG(acosf(vEyeDirection.x));
		if(vEyeDirection.y < 0) fAngle = -fAngle;
	}
	return fAngle;
}


//-----------------------------------------------------------------------------------------------------

float CHUDVehicleInterface::GetRelativeHeading()
{
	float fAngle = 0.0;
	if(m_pVehicle)
	{
		CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayerActor)
		{
			if (IVehicleSeat *pSeat = m_pVehicle->GetSeatForPassenger(pPlayerActor->GetEntityId()))
			{
				//this is kinda hacky since it requires the "turning part" of the vehicle to be called "turret" (but everything else would be way more complicated)
				if (IVehiclePart* pPart = m_pVehicle->GetPart("turret"))
				{
					const Matrix34& matLocal = pPart->GetLocalTM(false);

					Vec3 vLocalLook = matLocal.GetColumn(1);
					vLocalLook.z = 0.0f;
					vLocalLook.normalize();

					fAngle = RAD2DEG(acosf(vLocalLook.x));
					if(vLocalLook.y < 0) fAngle = -fAngle;
					fAngle -= 90.0f;
				}
				else
					return 0.0f;
			}
		}
	}
	return fAngle;
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::ShowVehicleInterface(EVehicleHud type)
{
	if(!m_pVehicle && !m_bParachute)
		return;

	CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());

	IVehicleSeat *pSeat = NULL;
	if(m_pVehicle)
	{
		pSeat = m_pVehicle->GetSeatForPassenger(pPlayerActor->GetEntityId());
		if(!pSeat)
			return;
	}

	if(g_pAmmo)
	{
		int iPrimaryAmmoCount = 0;
		int iPrimaryClipSize = 0;
		int iSecondaryAmmoCount = 0;
		int iSecondaryClipSize = 0;

		if(m_pVehicle)
		{
			CWeapon *pWeapon = pPlayerActor->GetWeapon(m_pVehicle->GetCurrentWeaponId(pPlayerActor->GetEntity()->GetId()));
			if(pWeapon)
			{
				IFireMode *pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
				if(pFireMode)
				{
					iPrimaryAmmoCount = pWeapon->GetAmmoCount(pFireMode->GetAmmoType());
					iPrimaryClipSize = m_pVehicle->GetAmmoCount(pFireMode->GetAmmoType());
				}
			}

			pWeapon = pPlayerActor->GetWeapon(m_pVehicle->GetCurrentWeaponId(pPlayerActor->GetEntity()->GetId(),true));
			if(pWeapon)
			{
				IFireMode *pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
				if(pFireMode)
				{
					iSecondaryAmmoCount = pWeapon->GetAmmoCount(pFireMode->GetAmmoType());
					iSecondaryClipSize = m_pVehicle->GetAmmoCount(pFireMode->GetAmmoType());
				}
			}
			// FIXME: move to member to remove static
			static int s_iSecondaryAmmoCount	= -1;
			static int s_iPrimaryAmmoCount		= -1;
			static int s_iSecondaryClipSize		= -1;
			static int s_iPrimaryClipSize			= -1;
			if(	s_iSecondaryAmmoCount	!= iSecondaryAmmoCount	||
				s_iPrimaryAmmoCount		!= iPrimaryAmmoCount		||
				s_iSecondaryClipSize	!= iSecondaryClipSize		||
				s_iPrimaryClipSize		!= iPrimaryClipSize)
			{
				SFlashVarValue args[4] = {iSecondaryAmmoCount, iPrimaryAmmoCount, iSecondaryClipSize, iPrimaryClipSize};
				g_pAmmo->Invoke("setAmmo", args, 4);
				s_iSecondaryAmmoCount	= iSecondaryAmmoCount;
				s_iPrimaryAmmoCount		= iPrimaryAmmoCount;
				s_iSecondaryClipSize	= iSecondaryClipSize;
				s_iPrimaryClipSize		= iPrimaryClipSize;
			}
		}
	}

	SMovementState sMovementState;
	if(m_pVehicle)
	{
		m_pVehicle->GetMovementController()->GetMovementState(sMovementState);
	}
	else
	{
		pPlayerActor->GetMovementController()->GetMovementState(sMovementState);
	}

	float fAngle = GetVehicleHeading();
	float fSpeed = GetVehicleSpeed();
	float fRelAngle = GetRelativeHeading();

	float fPosHeading = (fAngle*8.0f/3.0f);

	char szSpeed[32];

	char szN[32];
	char szW[32];

	char szAltitude[32];
	char szDistance[32];

	sprintf(szSpeed,"%.2f",fSpeed);
	sprintf(szN,"%.0f",0.f);
	sprintf(szW,"%.0f",0.f);
	sprintf(szAltitude,"%.0f",0.f);
	sprintf(szDistance,"%.0f",0.f);

	float fAltitude;

	if(((int)fSpeed) != m_statsSpeed)
	{
		SFlashVarValue args[2] = {szSpeed, (int)fSpeed};
		m_animStats.CheckedInvoke("setSpeed", args, 2);
		m_statsSpeed = (int)fSpeed;
	}

	// Note: this needs to be done even if we are not the driver, as the driver
	// may change the direction of the main turret while we'are at the gunner seat
	if(pSeat && (type == EHUD_TANKA || type == EHUD_TANKUS || type == EHUD_AAA || type == EHUD_APC || type == EHUD_APC2))
	{
		if((int)fRelAngle != m_statsHeading)
		{
			m_animStats.CheckedInvoke("setDirection", (int)fRelAngle);	//vehicle/turret angle
			m_statsHeading = (int)fRelAngle;
		}
	}

	if(type == EHUD_VTOL || type == EHUD_HELI || type == EHUD_PARACHUTE)
	{
		float fHorizon;
		if(m_pVehicle)
		{
			Vec3 vWorldPos = m_pVehicle->GetEntity()->GetWorldPos();
			fAltitude = vWorldPos.z-GetISystem()->GetI3DEngine()->GetTerrainZ((int)vWorldPos.x,(int)vWorldPos.y);
			fHorizon = -RAD2DEG(m_pVehicle->GetEntity()->GetWorldAngles().y);
		}
		else
		{
			Vec3 vWorldPos = pPlayerActor->GetEntity()->GetWorldPos();
			fAltitude = vWorldPos.z-GetISystem()->GetI3DEngine()->GetTerrainZ((int)vWorldPos.x,(int)vWorldPos.y);
			fHorizon = -RAD2DEG(pPlayerActor->GetAngles().y);
		}

		sprintf(szAltitude,"%.2f",fAltitude);

		g_pHUD->GetGPSPosition(&sMovementState,szN,szW);
		m_animMainWindow.Invoke("setHorizon", fHorizon);
	}
	if(type == EHUD_VTOL || type == EHUD_HELI)
	{
		float fVerticalHorizon = 0.0f;
		if(m_pVehicle)
		{
			fVerticalHorizon = RAD2DEG(m_pVehicle->GetEntity()->GetWorldAngles().x);
			m_animMainWindow.Invoke("setVerticalHorizon", fVerticalHorizon);
		}
	}
	if(type == EHUD_VTOL || type == EHUD_HELI || type == EHUD_TANKA || type == EHUD_TANKUS || type == EHUD_AAA || type == EHUD_LTV || type == EHUD_APC || type == EHUD_TRUCK || type == EHUD_SMALLBOAT || type == EHUD_PATROLBOAT)
	{
		if(g_pAmmo)
		{
			IWeapon *pPlayerWeapon = pPlayerActor->GetWeapon(m_pVehicle->GetCurrentWeaponId(pPlayerActor->GetEntity()->GetId()));
			if(pPlayerWeapon)
			{
				IFireMode *pFireMode = pPlayerWeapon->GetFireMode(pPlayerWeapon->GetCurrentFireMode());
				float fFireRate = 60.0f / pFireMode->GetFireRate();
				float fNextShotTime = pFireMode->GetNextShotTime();
				float fDuration = (((fFireRate-fNextShotTime)/fFireRate)*100.0f+1.0f);
				g_pAmmo->Invoke("setReloadDuration", (int)fDuration);
			}

			if(type == EHUD_AAA || type == EHUD_APC || type == EHUD_TANKA || type == EHUD_TANKUS)	//get reload for secondary guns
			{
				IItem *pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_pVehicle->GetWeaponId(1));
				if(pItem)
				{
					IWeapon *pWeapon =  pItem->GetIWeapon();
					IFireMode *pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
					float fFireRate = 60.0f / pFireMode->GetFireRate();
					float fNextShotTime = pFireMode->GetNextShotTime();
					float fDuration = (((fFireRate-fNextShotTime)/fFireRate)*100.0f+1.0f);
					g_pAmmo->Invoke("setReloadDuration2", (int)fDuration);
				}
			}
		}

		// FIXME: This one doesn't work because the nearest object often is ... the cannon of the tank !!!
		const ray_hit *pRay = pPlayerActor->GetGameObject()->GetWorldQuery()->GetLookAtPoint(100.0f);

		if(pRay)
		{
			sprintf(szDistance,"%.1f",pRay->dist);
		}
		else
		{
			sprintf(szDistance,"%.0f",11.0);
			szDistance[0]='-';
		}
	}

	{
		SFlashVarValue args[10] = {(int)fPosHeading, szN, szW, szAltitude, fAltitude, (int)(sMovementState.eyeDirection.z*90.0), szSpeed, (int)fSpeed, (int)fAngle, szDistance};
		m_animMainWindow.Invoke("setVehicleValues", args, 10);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::HideVehicleInterface()
{
	m_animMainWindow.Invoke("hideHUD");
	m_animMainWindow.SetVisible(false);

	m_animStats.Invoke("hideStats");
	m_animStats.SetVisible(false);
}

//-----------------------------------------------------------------------------------------------------

void CHUDVehicleInterface::UnloadVehicleHUD(bool remove)
{
	if(remove)
	{
		m_animStats.Unload();
		m_animMainWindow.Unload();
		m_statsSpeed = -999;
		m_statsHeading = -999;
	}
	else if(m_pVehicle && m_eCurVehicleHUD != EHUD_NONE)
	{
		m_animStats.Reload();
		m_animMainWindow.Reload();
		ShowVehicleInterface(m_eCurVehicleHUD);
		UpdateVehicleHUDDisplay();
		UpdateSeats();
		UpdateDamages();
		InitVehicleHuds();
	}
}

void CHUDVehicleInterface::Serialize(TSerialize &ser)
{
	ser.Value("hudParachute", m_bParachute);	
	EVehicleHud oldVehicleHud = m_eCurVehicleHUD;
	ser.EnumValue("CurVehicleHUD", m_eCurVehicleHUD, EHUD_NONE, EHUD_LAST);

	if(ser.IsReading())
	{
		if(oldVehicleHud != m_eCurVehicleHUD)
			HideVehicleInterface();
	}
}
