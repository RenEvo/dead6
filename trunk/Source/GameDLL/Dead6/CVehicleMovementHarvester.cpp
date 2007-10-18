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
	IPhysicalEntity* pPhysics = GetPhysics();
	assert(pPhysics);

	m_movementAction.isAI = true;

	if (!pPhysics->GetStatus(&m_PhysDyn))
		return;

	if (!pPhysics->GetStatus(&m_PhysPos))
		return;

	ProcessAI(deltaTime);

// Note: Keep below up-to-date with what is in CVehicleMovementStdWheeled!
	NETINPUT_TRACE(m_pVehicle->GetEntityId(), m_action.pedal);

	float speed = m_PhysDyn.v.len();

	if (!m_isEnginePowered || m_pVehicle->IsDestroyed() )
	{

		const float sleepTime = 3.0f;

		if ( m_passengerCount > 0 || ( m_pVehicle->IsDestroyed() && m_bForceSleep == false ))
		{
			UpdateSuspension(deltaTime);
			UpdateAxleFriction(m_movementAction.power, true, deltaTime);
		}

		m_action.bHandBrake = (m_movementAction.brake || m_pVehicle->IsDestroyed()) ? 1 : 0;
		m_action.pedal = 0;
		m_action.steer = 0;
		pPhysics->Action(&m_action, 1); //THREAD_SAFE

		bool maybeInTheAir = fabsf(m_PhysDyn.v.z) > 1.0f;
		if ( maybeInTheAir )
		{
			UpdateGravity(-9.81f * 1.4f);
			ApplyAirDamp(DEG2RAD(20.f), DEG2RAD(10.f), deltaTime, 1); //THREAD_SAFE
		}

		if ( m_pVehicle->IsDestroyed() && m_bForceSleep == false )
		{
			int numContact= 0;
			int numWheels = m_pVehicle->GetWheelCount();
			for (int i=0; i<numWheels; ++i)
			{ 
				pe_status_wheel ws;
				ws.iWheel = i;
				if (!pPhysics->GetStatus(&ws))
					continue;
				if (ws.bContact)
					numContact ++;
			}
			if ( numContact > m_pVehicle->GetWheelCount()/2 || speed<0.2f )
				m_forceSleepTimer += deltaTime;
			else
				m_forceSleepTimer = max(0.0f,m_forceSleepTimer-deltaTime);

			if ( m_forceSleepTimer > sleepTime )
			{
				IPhysicalEntity* pPhysics = GetPhysics();
				if (pPhysics)
				{
					pe_params_car params;
					params.minEnergy = 0.05f;
					pPhysics->SetParams(&params, 1);
					m_bForceSleep = true;
				}
			}
		}
		return;

	}

	// moved to main thread
	UpdateSuspension(deltaTime);   	
	UpdateAxleFriction(m_movementAction.power, true, deltaTime);
	//UpdateBrakes(deltaTime);

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
	float damageMul = 1.0f - 0.7f*m_damage;  
	float submergeMul = 1.0f;  
	float totalMul = 1.0f;  
	bool bInWater = m_PhysDyn.submergedFraction > 0.01f;
	if ( GetMovementType()!=IVehicleMovement::eVMT_Amphibious && bInWater )
	{
		submergeMul = max( 0.0f, 0.04f - m_PhysDyn.submergedFraction ) * 10.0f;
		submergeMul *=submergeMul;
		submergeMul = max( 0.2f, submergeMul );
	}

	Vec3 vUp(m_PhysPos.q.GetColumn2());
	Vec3 vUnitUp(0.0f,0.0f,1.0f);

	float slopedot = vUp.Dot( vUnitUp );
	bool bSteep =  fabsf(slopedot) < 0.7f;
	{ //fix for 30911
		if ( bSteep && speed > 7.5f )
		{
			Vec3 vVelNorm = m_PhysDyn.v.GetNormalizedSafe();
			if ( vVelNorm.Dot(vUnitUp)> 0.0f )
			{
				pe_action_impulse imp;
				imp.impulse = -m_PhysDyn.v;
				imp.impulse *= deltaTime * m_PhysDyn.mass*5.0f;      
				imp.point = m_PhysDyn.centerOfMass;
				imp.iApplyTime = 0;
				GetPhysics()->Action(&imp, 1); //THREAD_SAFE
			}
		}
	}

	totalMul = max( 0.3f, damageMul *  submergeMul );
	m_action.pedal = CLAMP(m_movementAction.power, -maxPedal, maxPedal ) * totalMul;

	// make sure cars can't drive under water
	if(GetMovementType()!=IVehicleMovement::eVMT_Amphibious && m_PhysDyn.submergedFraction >= m_submergedRatioMax && m_damage >= 0.99f)
	{
		m_action.pedal = 0.0f;
	}

	pPhysics->Action(&m_action, 1); //THREAD_SAFE

	if ( !bSteep && Boosting() )
		ApplyBoost(speed, 1.25f*m_maxSpeed*GetWheelCondition()*damageMul, m_boostStrength, deltaTime);  

	if (m_wheelContacts <= 1 && speed > 5.f)
	{
		ApplyAirDamp(DEG2RAD(20.f), DEG2RAD(10.f), deltaTime, 1); //THREAD_SAFE
		if ( !bInWater )
			UpdateGravity(-9.81f * 1.4f);  
	}

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
	if (m_isEngineDisabled || m_isEngineStarting)
		return false;

	if (/*m_damage >= 1.0f || */ m_pVehicle->IsDestroyed())
		return false;

	m_actorId = driverId;

	m_movementAction.Clear();
	m_movementAction.brake = false;

	if (m_movementTweaks.UseGroup(m_aiTweaksId))
		OnValuesTweaked();
	if (gEnv->bMultiplayer && m_multiplayerTweaksId > -1)
	{
		if (m_movementTweaks.UseGroup(m_multiplayerTweaksId))
			OnValuesTweaked();
	}

	// WarmupEngine relies on this being done here!
	if (m_isEnginePowered && !m_isEngineGoingOff)
	{ 
		StartExhaust(false, false);

		if (m_pVehicle->IsPlayerPassenger())
			GetOrPlaySound(eSID_Ambience);

		if (!m_isEngineGoingOff)
			return true;
	}

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
	m_brakeTimer = 0.f;
	m_action.pedal = 0.f;
	m_action.steer = 0.f;

	return true;
}