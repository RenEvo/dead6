////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CVehicleMovementHarvester.cpp
//
// Purpose: Vehicle Movement class for the Harvester
//	Uses a "ghost" driver for dummy-AI control
//
// Note: Keep up-to-date with VehicleMovementStdWheeled!
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "Stdafx.h"
#include "CVehicleMovementHarvester.h"

////////////////////////////////////////////////////
CVehicleMovementHarvester::CVehicleMovementHarvester(void)
{

}

////////////////////////////////////////////////////
CVehicleMovementHarvester::~CVehicleMovementHarvester(void)
{

}

////////////////////////////////////////////////////
bool CVehicleMovementHarvester::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
	if (false == CVehicleMovementStdWheeled::Init(pVehicle, table))
		return false;
	m_movementTweaks.Init(table);
	return true;
}

////////////////////////////////////////////////////
void CVehicleMovementHarvester::PostInit(void)
{
	CVehicleMovementStdWheeled::PostInit();
}

////////////////////////////////////////////////////
void CVehicleMovementHarvester::ProcessMovement(const float deltaTime)
{
	m_netActionSync.UpdateObject(this);
	CVehicleMovementBase::ProcessMovement(deltaTime);
}

////////////////////////////////////////////////////
bool CVehicleMovementHarvester::RequestMovement(CMovementRequest& movementRequest)
{
	if (false == CVehicleMovementStdWheeled::RequestMovement(movementRequest))	
		return false;

	return true;
}

////////////////////////////////////////////////////
void CVehicleMovementHarvester::Update(const float deltaTime)
{
	CVehicleMovementStdWheeled::Update(deltaTime);
}

////////////////////////////////////////////////////
void CVehicleMovementHarvester::UpdateSounds(const float deltaTime)
{
	CVehicleMovementStdWheeled::UpdateSounds(deltaTime);
}

////////////////////////////////////////////////////
void CVehicleMovementHarvester::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	CVehicleMovementStdWheeled::GetMemoryStatistics(s);
}

////////////////////////////////////////////////////
bool CVehicleMovementHarvester::StartEngine(EntityId driverId)
{
	// Keep below up to date with CVehicleMovementBase!
	if (true == m_isEngineDisabled || true == m_isEngineStarting)
		return false;
	m_actorId = 0; // No actor!

	m_movementAction.Clear();
	m_movementAction.brake = false;

	// Always use AI tweeks
	if (m_aiTweaksId > -1)
	{
		if (m_movementTweaks.UseGroup(m_aiTweaksId))
			OnValuesTweaked();
	}
	if (gEnv->bMultiplayer && m_multiplayerTweaksId > -1)
	{
		if (m_movementTweaks.UseGroup(m_multiplayerTweaksId))
			OnValuesTweaked();
	}

	if (m_isEnginePowered)
	{ 
		if (!m_isEngineGoingOff)
			return false;
	}

	if (m_damage >= 1.0f)
		return false;

	m_isEngineGoingOff = false;
	m_isEngineStarting = true;
	m_engineStartup = 0.0f;
	m_rpmScale = 0.f;
	m_engineIgnitionTime = m_runSoundDelay + 0.5f; //RUNSOUND_FADEIN_TIME

	StopSound(eSID_Run);
	StopSound(eSID_Ambience);
	StopSound(eSID_Damage);
	StopSound(eSID_Stop);

	PlaySound(eSID_Start, 0.f, m_enginePos);

	StartExhaust();
	StartAnimation(eVMA_Engine);
	InitWind();

	m_pVehicle->GetGameObject()->EnableUpdateSlot(m_pVehicle, IVehicle::eVUS_EnginePowered);

	// Keep below up to date with CVehicleMovementStdWheeled!
	m_brakeTimer = 0.0f;
	m_action.pedal = 0.0f;
	m_action.steer = 0.0f;

	return true;
}