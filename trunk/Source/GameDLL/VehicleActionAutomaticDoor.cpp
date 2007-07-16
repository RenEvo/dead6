/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a vehicle action for automatic door

-------------------------------------------------------------------------
History:
- 02:06:2006: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "IVehicleSystem.h"
#include "VehicleActionAutomaticDoor.h"
#include "Game.h"

#include "IScriptSystem.h"
#include "ScriptHelpers.h"
#include "GameUtils.h"

const float DOOR_OPENED = 0.0f;
const float DOOR_CLOSED = 1.0f;

//------------------------------------------------------------------------
CVehicleActionAutomaticDoor::CVehicleActionAutomaticDoor()
: m_pVehicle(NULL),
	m_pDoorAnim(NULL),
	m_doorOpenedStateId(InvalidVehicleAnimStateId),
	m_doorClosedStateId(InvalidVehicleAnimStateId),
	m_timeInTheAir(0.0f),
	m_timeOnTheGround(0.0f),
	m_isTouchingGround(false),
	m_isOpenRequested(true),
	m_isBlocked(false),
	m_animTime(0.0f),
	m_animGoal(0.0f)
{

}

//------------------------------------------------------------------------
CVehicleActionAutomaticDoor::~CVehicleActionAutomaticDoor()
{
	m_pVehicle->UnregisterVehicleEventListener(this);
}

//------------------------------------------------------------------------
bool CVehicleActionAutomaticDoor::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
	m_pVehicle = pVehicle;

	SmartScriptTable autoDoorTable;
	if (!table->GetValue("AutomaticDoor", autoDoorTable))
		return false;

	char* pAnimName = 0;
	if (autoDoorTable->GetValue("animation", pAnimName))
		m_pDoorAnim = m_pVehicle->GetAnimation(pAnimName);

	autoDoorTable->GetValue("timeMax", m_timeMax);

	if (!m_pDoorAnim)
		return false;

	m_doorOpenedStateId = m_pDoorAnim->GetStateId("opened");
	m_doorClosedStateId = m_pDoorAnim->GetStateId("closed");

	m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);

	m_pVehicle->RegisterVehicleEventListener(this);

	m_pDoorAnim->StartAnimation();
	m_pDoorAnim->ToggleManualUpdate(true);
	m_pDoorAnim->SetTime(DOOR_OPENED);

	return true;
}

//------------------------------------------------------------------------
void CVehicleActionAutomaticDoor::Reset()
{
	m_isTouchingGround = false;
	m_timeInTheAir = 0.0f;
	m_timeOnTheGround = 0.0f;
	m_isOpenRequested = false;
	m_isBlocked = false;

	m_animGoal = 0.0f;
	m_animTime = 0.0f;

	m_pDoorAnim->StopAnimation();
	m_pDoorAnim->StartAnimation();
	m_pDoorAnim->ToggleManualUpdate(true);
	m_pDoorAnim->SetTime(DOOR_OPENED);
}

//------------------------------------------------------------------------
int CVehicleActionAutomaticDoor::OnEvent(int eventType, SVehicleEventParams& eventParams)
{
	if (eventType == eVAE_OnGroundCollision || eventType == eVAE_OnEntityCollision)
	{
		m_isTouchingGround = true;
		return 1;
	}
	else if (eventType == eVAE_IsUsable)
	{
		return 1;
	}
	else if (eventType == eVAE_OnUsed)
	{
		m_isOpenRequested = !m_isOpenRequested;
	}

	return 0;
}

//------------------------------------------------------------------------
void CVehicleActionAutomaticDoor::Serialize(TSerialize ser, unsigned aspects)
{
	ser.Value("timeInTheAir", m_timeInTheAir);
	ser.Value("timeOnTheGround", m_timeOnTheGround);
}

//------------------------------------------------------------------------
void CVehicleActionAutomaticDoor::Update(const float deltaTime)
{
	bool isSlowEnough = true;
	bool isEnginePowered = false;

	if (IPhysicalEntity* pPhysEntity = m_pVehicle->GetEntity()->GetPhysics())
	{
		pe_status_dynamics dyn;
		pPhysEntity->GetStatus(&dyn);
		isSlowEnough = (dyn.v.GetLength() <= 2.0f);
	}

	if (IVehicleMovement* pMovement = m_pVehicle->GetMovement())
		isEnginePowered = pMovement->IsPowered();
	
	if (m_isTouchingGround && isSlowEnough)
	{
		m_timeInTheAir = 0.0f;
		m_timeOnTheGround += deltaTime;
		m_isTouchingGround = false;
	}
	else
	{
		m_timeInTheAir += deltaTime;
		m_timeOnTheGround = 0.0f;
	}

	/*
	if (m_pDoorAnim)
	{
		if ((m_timeOnTheGround >= m_timeMax || m_isOpenRequested) && !m_isBlocked)
		{
			if (m_pDoorAnim->GetState() != m_doorOpenedStateId)
 				m_pDoorAnim->ChangeState(m_doorOpenedStateId);
		}
		else if ((m_timeInTheAir > 0.5 && isEnginePowered) && !m_isBlocked)
		{
			if (m_pDoorAnim->GetState() != m_doorClosedStateId)
				m_pDoorAnim->ChangeState(m_doorClosedStateId);
		}
	}*/

	if ((m_timeOnTheGround >= m_timeMax || m_isOpenRequested) && !m_isBlocked)
	{
		m_animGoal = DOOR_OPENED;
	}
	else if ((m_timeInTheAir > 0.5 && isEnginePowered) && !m_isBlocked)
	{
		m_animGoal = DOOR_CLOSED;
	}

	//if (m_animGoal != m_animTime)
	{
		float speed = 0.5f;

		if (IPhysicalEntity* pPhysEntity = m_pVehicle->GetEntity()->GetPhysics())
		{
			pe_status_dynamics dyn;
			pPhysEntity->GetStatus(&dyn);
			speed += min(abs(dyn.v.GetLength()) * 0.1f, 1.0f);
		}

		Interpolate(m_animTime, m_animGoal, speed, deltaTime);
		m_pDoorAnim->SetTime(m_animTime);
	}
}

//------------------------------------------------------------------------
bool CVehicleActionAutomaticDoor::IsOpened()
{
	assert(m_pDoorAnim);
	return m_pDoorAnim->GetState() == m_doorOpenedStateId;
}

//------------------------------------------------------------------------
void CVehicleActionAutomaticDoor::OpenDoor(bool value)
{
	assert(m_pDoorAnim);
	m_isOpenRequested = value;
}

//------------------------------------------------------------------------
void CVehicleActionAutomaticDoor::BlockDoor(bool value)
{
	m_isBlocked = value;
}

//------------------------------------------------------------------------
void CVehicleActionAutomaticDoor::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
	if (event == eVE_OpenDoors)
	{
		m_animGoal = DOOR_OPENED;
	}
	else if (event == eVE_CloseDoors)
	{
		m_animGoal = DOOR_CLOSED;
	}
	else if (event == eVE_BlockDoors)
	{
		BlockDoor(params.bParam);
	}
}

DEFINE_VEHICLEOBJECT(CVehicleActionAutomaticDoor);
