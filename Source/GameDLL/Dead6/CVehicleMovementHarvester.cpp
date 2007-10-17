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
#include "NetInputChainDebug.h"

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

// Note: Keep below up-to-date with what is in CVehicleMovementBase!
	if (!GetPhysics()->GetStatus(&m_statusDyn))
	{
		GameWarning( "[CVehicleMovementBase]: '%s' is missing physics status", m_pEntity->GetName() );
		return;
	}

	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(e_Def3DPublicRenderflags);

	m_measureSpeedTimer+=deltaTime;
	if (m_measureSpeedTimer > 0.25f)
	{ 
		Vec3 accel = (m_statusDyn.v - m_lastMeasuredVel) * (1.f/m_measureSpeedTimer);
		m_localAccel = m_pVehicle->GetEntity()->GetWorldRotation().GetInverted() * accel;

		m_lastMeasuredVel = m_statusDyn.v;
		m_measureSpeedTimer = 0.f;
	}

	ProcessAI(deltaTime);

// Note: Keep below up-to-date with what is in CVehicleMovementStdWheeled!
	IPhysicalEntity* pPhysics = GetPhysics();
	const SVehicleStatus& status = m_pVehicle->GetStatus();

	NETINPUT_TRACE(m_pVehicle->GetEntityId(), m_action.pedal);

	if (!m_isEnginePowered)
	{
		m_action.bHandBrake = (m_damage < 1.f || m_pVehicle->IsDestroyed()) ? 1 : 0;
		m_action.pedal = 0;
		m_action.steer = 0;
		pPhysics->Action(&m_action, 1); //THREAD_SAFE
		return;
	}

	pPhysics->GetStatus(&m_vehicleStatus);
	float speed = m_vehicleStatus.vel.len();

	UpdateSuspension(deltaTime);   	
	UpdateAxleFriction(m_movementAction.power, true, deltaTime);
	UpdateBrakes(deltaTime);

	// speed ratio    
	float speedRel = min(speed, m_vMaxSteerMax) / m_vMaxSteerMax;  
	float steerMax = GetMaxSteer(speedRel);

	// calc steer error  	
	float steering = m_movementAction.rotateYaw;    
	float steerError = steering * steerMax - m_action.steer;
	steerError = (fabs(steerError)<0.01) ? 0 : steerError;

	if (fabs(m_movementAction.rotateYaw) > 0.005f)
	{ 
		bool reverse = sgn(m_action.steer)*sgn(steerError) < 0;
		float steerSpeed = GetSteerSpeed(reverse ? 0.f : speedRel);

		if (IsProfilingMovement() && reverse)
		{
			float color[] = {1,1,1,1};
			gEnv->pRenderer->Draw2dLabel(100,300,1.5f,color,false,"reverse");  
		}

		// adjust steering based on current error    
		m_action.steer = m_action.steer + sgn(steerError) * DEG2RAD(steerSpeed) * deltaTime;  
		m_action.steer = CLAMP(m_action.steer, -steerMax, steerMax);
	}
	else
	{
		// relax to center
		float d = -m_action.steer;
		float a = min(DEG2RAD(deltaTime * m_steerRelaxation), 1.0f);
		m_action.steer = m_action.steer + d * a;
	}

	// reduce actual pedal with increasing steering and velocity
	float maxPedal = 1 - (speedRel * abs(steering) * m_pedalLimitMax);  
	float damageMul = 1.0f - 0.25f*m_damage;  

	m_action.pedal = CLAMP(m_movementAction.power, -maxPedal, maxPedal ) * damageMul;

	pPhysics->Action(&m_action, 1); //THREAD_SAFE

	if (Boosting())  
		ApplyBoost(speed, 1.25f*m_maxSpeed, m_boostStrength, deltaTime);  

	DebugDrawMovement(deltaTime);

	if (m_netActionSync.PublishActions( CNetworkMovementStdWheeled(this) ))
		m_pVehicle->GetGameObject()->ChangedNetworkState( eEA_GameClientDynamic );
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
// Note: Keep below up-to-date with CVehicleMovementBase!
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

// Note: Keep below up-to-date with CVehicleMovementStdWheeled!
	m_brakeTimer = 0.0f;
	m_action.pedal = 0.0f;
	m_action.steer = 0.0f;

	return true;
}