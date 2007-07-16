/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a standard wheel based vehicle movements

-------------------------------------------------------------------------
History:
- 04:04:2005: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"

#include "VehicleMovementStdWheeled.h"

#include "IVehicleSystem.h"
#include "Network/NetActionSync.h"
#include <IAgent.h>
#include "GameUtils.h"
#include "IGameTokens.h"
#include "Player.h"

#include "NetInputChainDebug.h"

#define THREAD_SAFE 1
#define LOAD_RAMP_TIME 0.2f
#define LOAD_RELAX_TIME 0.25f

//------------------------------------------------------------------------
CVehicleMovementStdWheeled::CVehicleMovementStdWheeled()
{
	m_steerSpeed = 40.0f;
	m_steerMax = 20.0f;
	m_steerSpeedMin = 90.0f;
	m_steerSpeedScale = 0.8f;
	m_kvSteerMax = 10.0f;
	m_steerSpeedScaleMin = 1.0f;
	m_v0SteerMax = 30.0f;
	m_steerRelaxation = 90.0f;
  m_vMaxSteerMax = 20.f;
  m_pedalLimitMax = 0.3f;  
  m_suspDampingMin = 0.f;
  m_suspDampingMax = 0.f;  
  m_speedSuspUpdated = -1.f;
  m_suspDamping = 0.f;
  m_suspDampingMaxSpeed = 0.f;
  m_stabiMin = m_stabiMax = m_stabi = 0.f;
  m_rpmTarget = 0.f;
  m_engineMaxRPM = m_engineIdleRPM = m_engineShiftDownRPM = 0.f;
  m_rpmRelaxSpeed = m_rpmInterpSpeed = m_rpmGearShiftSpeed = 4.f;
  m_pullModUpdate = 0.f;
  m_maxBrakingFriction = 0.f;
  m_lastBump = 0.f;
  m_axleFrictionMax = m_axleFrictionMin = m_axleFriction = 0.f;
  m_load = 0.f;
  m_bumpMinSusp = m_bumpMinSpeed = 0.f;
  m_bumpIntensityMult = 1.f;
  m_rpmScalePrev = 0.f;
  m_gearSoundPending = false;
  m_latFriction = 1.f;    
  m_prevAngle = 0.0f;  
  m_lostContactTimer = 0.f;
  m_airbrakeTime = 0.f;
  m_brakeTimer = 0.f;
  m_tireBlownTimer = 0.f;
  m_boostEndurance = 7.5f;
  m_boostRegen = m_boostEndurance;
  m_brakeImpulse = 0.f;
  m_boostStrength = 6.f;
  m_lastSteerUpdateTime = gEnv->pTimer->GetFrameStartTime();
  m_lastDebugFrame = 0;
  m_wheelContactsLeft = 0;  
  m_wheelContactsRight = 0;
	m_netActionSync.PublishActions( CNetworkMovementStdWheeled(this) );  
}

//------------------------------------------------------------------------
CVehicleMovementStdWheeled::~CVehicleMovementStdWheeled()
{ 
}

//------------------------------------------------------------------------
bool CVehicleMovementStdWheeled::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
	if (!CVehicleMovementBase::Init(pVehicle, table))
	{
		assert(0);
    return false;
	}

  m_carParams.enginePower = 0.f;
  m_carParams.kStabilizer = 0.f;  
  m_carParams.engineIdleRPM = 0.f;
  m_carParams.engineMinRPM = m_carParams.engineMaxRPM = 0.f;     
  m_carParams.engineShiftDownRPM = m_carParams.engineShiftUpRPM = 0.f;
  m_carParams.steerTrackNeutralTurn = 0.f;  

  MOVEMENT_VALUE_OPT("steerSpeed", m_steerSpeed, table);
  MOVEMENT_VALUE_OPT("steerSpeedMin", m_steerSpeedMin, table);
  MOVEMENT_VALUE_OPT("steerSpeedScale", m_steerSpeedScale, table);
  MOVEMENT_VALUE_OPT("steerSpeedScaleMin", m_steerSpeedScaleMin, table);
  MOVEMENT_VALUE_OPT("kvSteerMax", m_kvSteerMax, table);
  MOVEMENT_VALUE_OPT("v0SteerMax", m_v0SteerMax, table);
  MOVEMENT_VALUE_OPT("steerRelaxation", m_steerRelaxation, table);
  MOVEMENT_VALUE_OPT("vMaxSteerMax", m_vMaxSteerMax, table);
  MOVEMENT_VALUE_OPT("pedalLimitMax", m_pedalLimitMax, table);
  
  table->GetValue("rpmRelaxSpeed", m_rpmRelaxSpeed);
  table->GetValue("rpmInterpSpeed", m_rpmInterpSpeed);
  table->GetValue("rpmGearShiftSpeed", m_rpmGearShiftSpeed);

  SmartScriptTable soundParams;
  if (table->GetValue("SoundParams", soundParams))
  {
    soundParams->GetValue("roadBumpMinSusp", m_bumpMinSusp);
    soundParams->GetValue("roadBumpMinSpeed", m_bumpMinSpeed);
    soundParams->GetValue("roadBumpIntensity", m_bumpIntensityMult);
    soundParams->GetValue("airbrake", m_airbrakeTime);
  }
  
	m_action.steer = 0.0f;
	m_action.pedal = 0.0f;
	m_action.dsteer = 0.0f;

	// Initialise the direction PID.
	m_direction = 0.0f;
	m_dirPID.Reset();
	m_dirPID.m_kP = 0.6f;
	m_dirPID.m_kD = 0.1f;
	m_dirPID.m_kI = 0.01f;

	// Initialise the steering history.
	m_steering = 0.0f;
  m_prevAngle = 0.0f;

	m_rpmScale = 0.0f;
	m_currentGear = 0;
  m_avgLateralSlip = 0.f;
  m_compressionMax = 0.f;
  m_avgWheelRot = 0.f;
  m_wheelContacts = 0;
	  
	if (!InitPhysics(table))
		return false;

	m_movementTweaks.Init(table);
	return true;
}

//------------------------------------------------------------------------
bool CVehicleMovementStdWheeled::InitPhysics(const SmartScriptTable &table)
{
	SmartScriptTable wheeledTable;
	if (!table->GetValue("Wheeled", wheeledTable))
    return false;

  m_carParams.maxTimeStep = 0.02f;
    
	MOVEMENT_VALUE_REQ("axleFriction", m_axleFrictionMin, wheeledTable);
	MOVEMENT_VALUE_OPT("axleFrictionMax", m_axleFrictionMax, wheeledTable);
	MOVEMENT_VALUE_REQ("brakeTorque", m_carParams.brakeTorque, wheeledTable);
	MOVEMENT_VALUE_REQ("clutchSpeed", m_carParams.clutchSpeed, wheeledTable);
	MOVEMENT_VALUE_REQ("damping", m_carParams.damping, wheeledTable);
	MOVEMENT_VALUE_REQ("engineIdleRPM", m_carParams.engineIdleRPM, wheeledTable);
	MOVEMENT_VALUE_REQ("engineMaxRPM", m_carParams.engineMaxRPM, wheeledTable);
	MOVEMENT_VALUE_REQ("engineMinRPM", m_carParams.engineMinRPM, wheeledTable);
	MOVEMENT_VALUE_REQ("engineShiftDownRPM", m_carParams.engineShiftDownRPM, wheeledTable);
	MOVEMENT_VALUE_REQ("engineShiftUpRPM", m_carParams.engineShiftUpRPM, wheeledTable);
	MOVEMENT_VALUE_REQ("engineStartRPM", m_carParams.engineStartRPM, wheeledTable);
	wheeledTable->GetValue("integrationType", m_carParams.iIntegrationType);
	MOVEMENT_VALUE_REQ("stabilizer", m_carParams.kStabilizer, wheeledTable);
	MOVEMENT_VALUE_OPT("minBrakingFriction", m_carParams.minBrakingFriction, wheeledTable);		
	MOVEMENT_VALUE_REQ("maxSteer", m_carParams.maxSteer, wheeledTable);    
	wheeledTable->GetValue("maxTimeStep", m_carParams.maxTimeStep);
	wheeledTable->GetValue("minEnergy", m_carParams.minEnergy);
	MOVEMENT_VALUE_REQ("slipThreshold", m_carParams.slipThreshold, wheeledTable);
	MOVEMENT_VALUE_REQ("gearDirSwitchRPM", m_carParams.gearDirSwitchRPM, wheeledTable);
	MOVEMENT_VALUE_REQ("dynFriction", m_carParams.kDynFriction, wheeledTable);
	MOVEMENT_VALUE_OPT("latFriction", m_latFriction, wheeledTable);
	MOVEMENT_VALUE_OPT("steerTrackNeutralTurn", m_carParams.steerTrackNeutralTurn, wheeledTable);  
	MOVEMENT_VALUE_OPT("suspDampingMin", m_suspDampingMin, wheeledTable);
	MOVEMENT_VALUE_OPT("suspDampingMax", m_suspDampingMax, wheeledTable);
  MOVEMENT_VALUE_OPT("suspDampingMaxSpeed", m_suspDampingMaxSpeed, wheeledTable);
  MOVEMENT_VALUE_OPT("stabiMin", m_stabiMin, wheeledTable);
  MOVEMENT_VALUE_OPT("stabiMax", m_stabiMax, wheeledTable);
	MOVEMENT_VALUE_OPT("maxSpeed", m_maxSpeed, wheeledTable);
  MOVEMENT_VALUE_OPT("brakeImpulse", m_brakeImpulse, wheeledTable);
  
  m_carParams.axleFriction = m_axleFrictionMin;
  m_axleFriction = m_axleFrictionMin;
  m_stabi = m_carParams.kStabilizer;
	
	if (wheeledTable->GetValue("enginePower", m_carParams.enginePower))
	{
		m_carParams.enginePower *= 1000.0f;
		m_movementTweaks.AddValue("enginePower", &m_carParams.enginePower, true);
	}
	else
	{
		CryLog("Movement Init (%s) - failed to init due to missing <%s> parameter", m_pVehicle->GetEntity()->GetClass()->GetName(), "enginePower");
		return false;
	}

  if (gEnv->pSystem->GetConfigSpec() == CONFIG_LOW_SPEC)
    m_carParams.maxTimeStep = max(m_carParams.maxTimeStep, 0.04f);

	// don't submit maxBrakingFriction to physics, it's controlled by gamecode
	MOVEMENT_VALUE_OPT("maxBrakingFriction", m_maxBrakingFriction, wheeledTable);
	  
  m_carParams.pullTilt = 0.f;
  wheeledTable->GetValue("pullTilt", m_carParams.pullTilt);
  m_carParams.pullTilt = DEG2RAD(m_carParams.pullTilt);
  m_movementTweaks.AddValue("pullTilt", &m_carParams.pullTilt);

	SmartScriptTable gearRatiosTable;
	if (wheeledTable->GetValue("gearRatios", gearRatiosTable))
	{
		int count = min(gearRatiosTable->Count(), 16);

		for (int i=0; i<count; ++i)
		{	
			m_gearRatios[i] = 0.f;
			gearRatiosTable->GetAt(i+1, m_gearRatios[i]);
		}

		m_carParams.nGears = count;
		m_carParams.gearRatios = m_gearRatios;
	}

	if (!table->GetValue("isBreakingOnIdle", m_isBreakingOnIdle))
		m_isBreakingOnIdle = false;

	return true;
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::PostInit()
{
  CVehicleMovementBase::PostInit();

  m_wheelStats.clear();
  m_wheelParts.clear();
  
  int numWheels = m_pVehicle->GetWheelCount();    
  m_wheelStats.resize(numWheels);

  int nParts = m_pVehicle->GetPartCount();

  for (int i=0; i<nParts; ++i)
  {      
    IVehiclePart* pPart = m_pVehicle->GetPart(i);
    if (pPart->GetIWheel())
    { 
      m_wheelParts.push_back(pPart);
      m_wheelStats[m_wheelParts.size()-1].friction = pPart->GetIWheel()->GetCarGeomParams()->kLatFriction;
    }
  }
  assert(m_wheelParts.size() == numWheels);
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::Physicalize()
{
	CVehicleMovementBase::Physicalize();

	SEntityPhysicalizeParams physicsParams(m_pVehicle->GetPhysicsParams());	
  
  physicsParams.type = PE_WHEELEDVEHICLE;	  
  m_carParams.nWheels = m_pVehicle->GetWheelCount();  
	
  pe_params_car carParams(m_carParams);  
  physicsParams.pCar = &carParams;

  m_pVehicle->GetEntity()->Physicalize(physicsParams);
    
  m_engineMaxRPM = m_carParams.engineMaxRPM;
  m_engineIdleRPM = m_carParams.engineIdleRPM;
  m_engineShiftDownRPM = m_carParams.engineShiftDownRPM;

	IPhysicalEntity *pPhysEnt;
	if (gEnv->pSystem->GetConfigSpec()==CONFIG_LOW_SPEC && m_carParams.steerTrackNeutralTurn && 
			(pPhysEnt=m_pVehicle->GetEntity()->GetPhysics()))
	{
		pe_params_flags pf; pf.flagsOR = wwef_fake_inner_wheels;
		pe_params_foreign_data pfd; pfd.iForeignFlagsOR = PFF_UNIMPORTANT;
		pPhysEnt->SetParams(&pf);
		pPhysEnt->SetParams(&pfd);
	}
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::PostPhysicalize()
{
  CVehicleMovementBase::PostPhysicalize();
  
  if (m_maxSpeed == 0.f)
  {
    pe_status_vehicle_abilities ab;
    GetPhysics()->GetStatus(&ab);
    m_maxSpeed = ab.maxVelocity * 0.5f; // fixme! maxVelocity too high
    CryLog("%s maxSpeed: %f", m_pVehicle->GetEntity()->GetClass()->GetName(), m_maxSpeed);
  }
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::InitSurfaceEffects()
{ 
  IPhysicalEntity* pPhysics = GetPhysics();
  pe_status_nparts tmpStatus;
  int numParts = pPhysics->GetStatus(&tmpStatus);
  int numWheels = m_pVehicle->GetWheelCount();

  m_paStats.envStats.emitters.clear();

  // for each wheelgroup, add 1 particleemitter. the position is the wheels' 
  // center in xy-plane and minimum on z-axis
  // direction is upward
  SEnvironmentParticles* envParams = m_pPaParams->GetEnvironmentParticles();
  
  for (int iLayer=0; iLayer<envParams->GetLayerCount(); ++iLayer)
  {
    const SEnvironmentLayer& layer = envParams->GetLayer(iLayer);
    
    m_paStats.envStats.emitters.reserve(m_paStats.envStats.emitters.size() + layer.GetGroupCount());

    for (int i=0; i<layer.GetGroupCount(); ++i)
    { 
      Matrix34 tm;
      tm.SetIdentity();
      
      if (layer.GetHelperCount() == layer.GetGroupCount() && layer.GetHelper(i))
      {
        // use helper pos if specified
				if (IVehicleHelper* pHelper = layer.GetHelper(i))
					tm = pHelper->GetVehicleTM();
				else
					tm.SetIdentity();
      }
      else
      {
        // else use wheels' center
        Vec3 pos(ZERO);
        
        for (int w=0; w<layer.GetWheelCount(i); ++w)
        {       
          int ipart = numParts - numWheels + layer.GetWheelAt(i,w)-1; // wheels are last

          if (ipart < 0 || ipart >= numParts)
          {
            CryLog("%s invalid wheel index: %i, maybe asset/setup mismatch", m_pEntity->GetName(), ipart);
            continue;
          }

          pe_status_pos spos;
          spos.ipart = ipart;
          if (pPhysics->GetStatus(&spos))
          {
            spos.pos.z += spos.BBox[0].z;
            pos = (pos.IsZero()) ? spos.pos : 0.5f*(pos + spos.pos);        
          }
        }
        tm = Matrix34::CreateRotationX(DEG2RAD(90.f));      
        tm.SetTranslation( m_pEntity->GetWorldTM().GetInverted().TransformPoint(pos) );
      }     

      TEnvEmitter emitter;
      emitter.layer = iLayer;        
      emitter.slot = -1;
      emitter.group = i;
      emitter.active = layer.IsGroupActive(i);
      emitter.quatT = QuatT(tm);
      m_paStats.envStats.emitters.push_back(emitter);
      
      if (DebugParticles())
      {
        const Vec3 loc = tm.GetTranslation();
        CryLog("WheelGroup %i local pos: %.1f %.1f %.1f", i, loc.x, loc.y, loc.z);        
      }      
    }
  }
  
  m_paStats.envStats.initalized = true;  
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::Reset()
{
	CVehicleMovementBase::Reset();

	m_direction = 0;
	m_steering = 0;
	m_dirPID.Reset();
  m_prevAngle = 0.0f;

	m_action.pedal = 0.f;
	m_action.steer = 0.f;

	m_rpmScale = 0.0f;
  m_rpmScalePrev = 0.f;
	m_currentGear = 0;
  m_gearSoundPending = false;
  m_avgLateralSlip = 0.f;  
  m_tireBlownTimer = 0.f; 
  m_wheelContacts = 0;
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::Release()
{
	CVehicleMovementBase::Release();

	delete this;
}

//------------------------------------------------------------------------
bool CVehicleMovementStdWheeled::StartEngine(EntityId driverId)
{
  if (!CVehicleMovementBase::StartEngine(driverId))
    return false;

  m_brakeTimer = 0.f;
  m_action.pedal = 0.f;
  m_action.steer = 0.f;

  return true;
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::StopEngine()
{
	CVehicleMovementBase::StopEngine();
	m_movementAction.Clear(true);

  UpdateBrakes(0.f);
  m_pGameTokenSystem->SetOrCreateToken("vehicle.rpmNorm", TFlowInputData(0.f, true));
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::OnEvent(EVehicleMovementEvent event, const SVehicleMovementEventParams& params)
{
	CVehicleMovementBase::OnEvent(event, params);

	if (event == eVME_Damage || event == eVME_Repair)
	{
		// not needed anymore
	}
  else if (event == eVME_TireBlown)
  {
    int wheelIndex = params.iValue;
    SEnvironmentParticles* envParams = m_pPaParams->GetEnvironmentParticles();
    
    SEnvParticleStatus::TEnvEmitters::iterator emitterIt = m_paStats.envStats.emitters.begin();
    SEnvParticleStatus::TEnvEmitters::iterator emitterItEnd = m_paStats.envStats.emitters.end();
    for (; emitterIt != emitterItEnd; ++emitterIt)
    { 
      // disable this wheel in layer 0, enable in layer 1            
      if (emitterIt->group >= 0)
      {
        const SEnvironmentLayer& layer = envParams->GetLayer(emitterIt->layer);

        for (int i=0; i<layer.GetWheelCount(emitterIt->group); ++i)
        {
          if (layer.GetWheelAt(emitterIt->group, i)-1 == wheelIndex)
          {
            bool enable = !strcmp(layer.GetName(), "rims");
            EnableEnvEmitter(*emitterIt, enable);
            emitterIt->active = enable;
          }
        }
      }
    }
  }  
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::UpdateAxleFriction(float pedal, bool backward, const float deltaTime)
{
  // apply high axleFriction for idle and footbraking, low for normal driving    
  int pedalGearDir = sgn(m_vehicleStatus.iCurGear-1) * sgn(pedal);

  if (m_axleFrictionMax > m_axleFrictionMin)
  {
    float fric;

    if ((!backward || m_vehicleStatus.iCurGear != 0) && pedalGearDir > 0)      
      fric = m_axleFrictionMin; 
    else
      fric = m_axleFrictionMax;

    if (m_axleFriction != fric)
    {
      m_axleFriction = fric;

      pe_params_car carparams;
      carparams.axleFriction = m_axleFriction;
      GetPhysics()->SetParams(&carparams);
    }
  }

  if (m_brakeImpulse != 0.f && m_wheelContacts && pedal != 0.f && m_statusDyn.v.len2()>1.f)
  {
    const Matrix34& worldTM = m_pVehicle->GetEntity()->GetWorldTM();
    Vec3 vel = worldTM.GetInverted().TransformVector(m_statusDyn.v);

    if (sgn(vel.y)*sgn(pedal) < 0)
    {
      // add helper impulse for braking
      pe_action_impulse imp;      
      imp.impulse = -m_statusDyn.v.GetNormalized();
      imp.impulse *= m_brakeImpulse * abs(vel.y) * ((float)m_wheelContacts/m_pVehicle->GetWheelCount()) * deltaTime * m_statusDyn.mass;      
      imp.point = m_statusDyn.centerOfMass;
      imp.iApplyTime = 1;
      GetPhysics()->Action(&imp, THREAD_SAFE);

      if (IsProfilingMovement())
      {
        IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
        float len = 5.f * imp.impulse.len() / deltaTime / m_statusDyn.mass;
        Vec3 dir = imp.impulse.GetNormalized();
        pGeom->DrawCone(imp.point-(dir*len), dir, 1.f, len, ColorB(128,0,0,255));

        pGeom->DrawLine(imp.point+worldTM.TransformVector(Vec3(0,0,1)), ColorB(0,0,255,255), imp.point+worldTM.TransformVector(Vec3(0,0,1))+m_statusDyn.v, ColorB(0,0,255,255));
      }    
    }
  }
}

//------------------------------------------------------------------------
float CVehicleMovementStdWheeled::GetMaxSteer(float speedRel)
{
  // reduce max steering angle with increasing speed
  m_steerMax = m_v0SteerMax - (m_kvSteerMax * speedRel);
  
  return DEG2RAD(m_steerMax);
}


//------------------------------------------------------------------------
float CVehicleMovementStdWheeled::GetSteerSpeed(float speedRel)
{
  // reduce steer speed with increasing speed
  float steerDelta = m_steerSpeed - m_steerSpeedMin;
  float steerSpeed = m_steerSpeedMin + steerDelta * speedRel;

  // additionally adjust sensitivity based on speed
  float steerScaleDelta = m_steerSpeedScale - m_steerSpeedScaleMin;
  float sensivity = m_steerSpeedScaleMin + steerScaleDelta * speedRel;
  
  return steerSpeed * sensivity;
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::ProcessMovement(const float deltaTime)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );
  
  m_netActionSync.UpdateObject(this);

  CVehicleMovementBase::ProcessMovement(deltaTime);

  IPhysicalEntity* pPhysics = GetPhysics();
  const SVehicleStatus& status = m_pVehicle->GetStatus();

	NETINPUT_TRACE(m_pVehicle->GetEntityId(), m_action.pedal);
  
  if (!(m_actorId && m_isEnginePowered))
  {
    m_action.bHandBrake = (m_damage < 1.f || m_pVehicle->IsDestroyed()) ? 1 : 0;
    m_action.pedal = 0;
    m_action.steer = 0;
    pPhysics->Action(&m_action, THREAD_SAFE);
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
	
	pPhysics->Action(&m_action, THREAD_SAFE);
  
  if (Boosting())  
    ApplyBoost(speed, 1.25f*m_maxSpeed, m_boostStrength, deltaTime);  
  
  DebugDrawMovement(deltaTime);
 
	if (m_netActionSync.PublishActions( CNetworkMovementStdWheeled(this) ))
		m_pVehicle->GetGameObject()->ChangedNetworkState( eEA_GameClientDynamic );
 
}

//----------------------------------------------------------------------------------
void CVehicleMovementStdWheeled::Boost(bool enable)
{  
  if (enable)
  {
    if (m_action.bHandBrake)
      return;
  }

  CVehicleMovementBase::Boost(enable);
}


//----------------------------------------------------------------------------------
void CVehicleMovementStdWheeled::ApplyBoost(float speed, float maxSpeed, float strength, float deltaTime)
{
  const static float fullBoostMaxSpeed = 0.75f*m_maxSpeed;
    
  if (m_action.pedal > 0.01f && m_wheelContacts >= 0.5f*m_pVehicle->GetWheelCount() && speed < maxSpeed)
  {       
    float fraction = max(0.f, 1.f - max(0.f, speed-fullBoostMaxSpeed)/(maxSpeed-fullBoostMaxSpeed));
    float amount = fraction * strength * m_action.pedal * m_statusDyn.mass * deltaTime;
  
    float angle = DEG2RAD(m_carParams.steerTrackNeutralTurn == 0.f ? 30.f : 0.f);
    Vec3 dir(0, cos(angle), -sin(angle));

    AABB bounds;
    m_pVehicle->GetEntity()->GetLocalBounds(bounds);
    
    pe_action_impulse imp;            
    imp.impulse = m_pVehicle->GetEntity()->GetWorldRotation() * dir * amount;
    imp.point = m_pVehicle->GetEntity()->GetWorldTM() * Vec3(0, bounds.min.y, 0); // approx. at ground
    
    GetPhysics()->Action(&imp, THREAD_SAFE);

    //const static float color[] = {1,1,1,1};
    //gEnv->pRenderer->Draw2dLabel(400, 400, 1.4f, color, false, "fBoost: %.2f", fraction);
  }
}

//----------------------------------------------------------------------------------
void CVehicleMovementStdWheeled::DebugDrawMovement(const float deltaTime)
{
  if (!IsProfilingMovement())
    return;

  if (g_pGameCVars->v_profileMovement==1 && m_lastDebugFrame == gEnv->pRenderer->GetFrameID())
    return;
  
  m_lastDebugFrame = gEnv->pRenderer->GetFrameID();
  
  IPhysicalEntity* pPhysics = GetPhysics();
  IRenderer* pRenderer = gEnv->pRenderer;
  static float color[4] = {1,1,1,1};
  float green[4] = {0,1,0,1};
  float red[4] = {1,0,0,1};
  static ColorB colRed(255,0,0,255);
  float y = 50.f, step1 = 15.f, step2 = 20.f, size=1.3f, sizeL=1.5f;

  float speed = m_vehicleStatus.vel.len();
  float speedRel = min(speed, m_vMaxSteerMax) / m_vMaxSteerMax;
  float steerMax = GetMaxSteer(speedRel);
  float steerSpeed = GetSteerSpeed(speedRel);
  
  int percent = (int)(speed / m_maxSpeed * 100.f);

  pRenderer->Draw2dLabel(5.0f,   y, sizeL, color, false, "Car movement");
  pRenderer->Draw2dLabel(5.0f,  y+=step2, size, color, false, "Speed: %.1f (%.1f km/h) (%i)", speed, speed*3.6f, percent);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Accel: %.1f", m_localAccel.y);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, m_vehicleStatus.engineRPM>m_engineMaxRPM? red : color, false, "RPM:   %.0f", m_vehicleStatus.engineRPM); 
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "rpm_scale:   %.2f", m_rpmScale); 
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Gear:  %i", m_vehicleStatus.iCurGear-1);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Clutch:  %.2f", m_vehicleStatus.clutch);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Torque:  %.1f", m_vehicleStatus.drivingTorque);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "AxleFric:  %.0f", m_axleFriction);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Dampers:  %.2f", m_suspDamping);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Stabi:  %.2f", m_stabi);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "LatSlip:  %.2f", m_avgLateralSlip);  
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "AvgWheelSpeed:  %.2f", m_avgWheelRot);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "BrakeTime:  %.2f", m_brakeTimer);
    
  if (m_statusDyn.submergedFraction > 0.f)
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "Submerged:  %.2f", m_statusDyn.submergedFraction);

  if (m_damage > 0.f)
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size, color, false, "Damage:  %.2f", m_damage);  
    
  if (Boosting())
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size, green, false, "Boost:  %.2f", m_boostCounter);

  pRenderer->Draw2dLabel(5.0f,  y+=step2, sizeL, color, false, "Driver input");
  pRenderer->Draw2dLabel(5.0f,  y+=step2, size, color, false, "power: %.2f", m_movementAction.power);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "steer: %.2f", m_movementAction.rotateYaw); 
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "brake: %i", m_movementAction.brake);

  pRenderer->Draw2dLabel(5.0f,  y+=step2, sizeL, color, false, "Car action");
  pRenderer->Draw2dLabel(5.0f,  y+=step2, size, color, false, "pedal: %.2f", m_action.pedal);
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "steer: %.2f (max %.2f)", RAD2DEG(m_action.steer), RAD2DEG(steerMax)); 
  pRenderer->Draw2dLabel(5.0f,  y+=step1, size, color, false, "brake: %i", m_action.bHandBrake);

  pRenderer->Draw2dLabel(5.0f,  y+=step2, size, color, false, "steerSpeed: %.2f", steerSpeed); 

  const Matrix34& worldTM = m_pVehicle->GetEntity()->GetWorldTM();
  
  IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
  ColorB colGreen(0,255,0,255);
  pGeom->DrawSphere(m_statusDyn.centerOfMass, 0.1f, colGreen);
  
  pe_status_wheel ws;
  pe_status_pos wp;
  pe_params_wheel wparams;

  pe_status_nparts tmpStatus;
  int numParts = pPhysics->GetStatus(&tmpStatus);
  
  int count = m_pVehicle->GetWheelCount();
  
  // wheel-specific
  for (int i=0; i<count; ++i)
  {
    ws.iWheel = i;
    wp.ipart = numParts - count + i;
    wparams.iWheel = i;

    int ok = pPhysics->GetStatus(&ws);
    ok &= pPhysics->GetStatus(&wp);
    ok &= pPhysics->GetParams(&wparams);

    if (!ok)
      continue;

    // slip
    if (g_pGameCVars->v_draw_slip)
    {
      if (ws.bContact)
      { 
        pGeom->DrawSphere(ws.ptContact, 0.05f, colRed);

        float slip = ws.velSlip.len();        
        if (ws.bSlip>0)
        { 
          IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
          pGeom->DrawLine(wp.pos, colRed, wp.pos+ws.velSlip, colRed);
        }        
      }
            
      //gEnv->pRenderer->DrawLabel(wp.pos, 1.2f, "%.2f", m_wheelStats[i].friction);      

      if (wparams.bDriving || g_pGameCVars->v_draw_slip>1)
      {
        gEnv->pRenderer->DrawLabel(wp.pos, 1.2f, "T: %.0f", ws.torque);
      }      
    }    

    // suspension    
    if (g_pGameCVars->v_draw_suspension)
    {
      IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
      ColorB col(255,0,0,255);

      Vec3 lower = m_wheelParts[i]->GetLocalTM(false).GetTranslation();
      lower.x += sgn(lower.x) * 0.5f;
      
      Vec3 upper(lower);
      upper.z += ws.suspLen;
      
      lower = worldTM.TransformPoint(lower);
      pAuxGeom->DrawSphere(lower, 0.1f, col);              
      
      upper = worldTM.TransformPoint(upper);
      pAuxGeom->DrawSphere(upper, 0.1f, col);

      //pAuxGeom->DrawLine(lower, col, upper, col);
    }    
  }

  if (m_pWind[0])
  {
    pe_params_buoyancy buoy;
    pe_status_pos pos;
    if (m_pWind[0]->GetParams(&buoy) && m_pWind[0]->GetStatus(&pos))
    {
      gEnv->pRenderer->DrawLabel(pos.pos, 1.3f, "rad: %.1f", buoy.waterFlow.len());
    }
    if (m_pWind[1]->GetParams(&buoy) && m_pWind[1]->GetStatus(&pos))
    {
      gEnv->pRenderer->DrawLabel(pos.pos, 1.3f, "lin: %.1f", buoy.waterFlow.len());
    }
  }
}


//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::Update(const float deltaTime)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

  IPhysicalEntity* pPhysics = GetPhysics();
	if(!pPhysics)
	{
    assert(0 && "[CVehicleMovementStdWheeled::Update]: PhysicalEntity NULL!");
		return;
	}
    
  pPhysics->GetStatus(&m_vehicleStatus);

	CVehicleMovementBase::Update(deltaTime);

  bool distant = m_pVehicle->GetGameObject()->IsProbablyDistant();
     
  if (gEnv->bClient && !distant)
    UpdateSounds(deltaTime);    

  if (!distant && m_carParams.steerTrackNeutralTurn == 0.f)
  {
    for (TWheelParts::const_iterator it=m_wheelParts.begin(),end=m_wheelParts.end(); it!=end; ++it)
    {
      if (IVehiclePart::eVGS_Damaged1 == (*it)->GetState())
      {
        m_tireBlownTimer += deltaTime;
        break;
      }
    }    
  }
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::UpdateGameTokens(const float deltaTime)
{
  CVehicleMovementBase::UpdateGameTokens(deltaTime);

  if (m_pVehicle->IsPlayerDriving())
  { 
    m_pGameTokenSystem->SetOrCreateToken("vehicle.rpmNorm", TFlowInputData(m_rpmScale, true));
  }    
}


//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::UpdateSounds(const float deltaTime)
{ 
  FUNCTION_PROFILER( gEnv->pSystem, PROFILE_GAME );

  // engine load  
  float loadTarget = -1.f;
  
  // update engine sound
  if (m_isEnginePowered && !m_isEngineGoingOff)
  {
    float rpmScale = min(m_vehicleStatus.engineRPM / m_engineMaxRPM, 1.f);
    
    if (rpmScale < GetMinRPMSoundRatio())
    {
      // pitch rpm with pedal, if MinSoundRatio is used and rpm is below that
      rpmScale = min(GetMinRPMSoundRatio(), max(m_action.pedal, rpmScale));
    }

    // scale rpm down when in backward gear
    //if (m_currentGear == 0)
      //rpmScale *= 0.8; 
        
    if (m_vehicleStatus.bHandBrake)
    {
      Interpolate(m_rpmScale, rpmScale, 2.5f, deltaTime);
      m_rpmTarget = 0.f;
      loadTarget = 0.f;
    }
    else if (m_rpmTarget)
    {
      if (m_rpmTarget < m_rpmScale && DoGearSound())
      {
        loadTarget = 0.f;
      }

      Interpolate(m_rpmScale, m_rpmTarget, m_rpmGearShiftSpeed, deltaTime);
      
      float diff = abs(m_rpmScale-m_rpmTarget);
      
      if (m_gearSoundPending && m_currentGear >= 3) // only from 1st gear upward
      {          
        if (diff < 0.5f*abs(m_rpmScalePrev-m_rpmTarget))
        {           
          GetOrPlaySound(eSID_Gear, 0.f, m_enginePos);          
          m_gearSoundPending = false;
        }
      }
      
      if (diff < 0.02)
      {
        m_rpmTarget = 0.f;        
      }
    }
    else
    {
      // don't allow rpm to rev up when in 1st forward/backward gear and clutch is disengaged.
      // a bit hacky, but it prevents the sound glitch
      if (m_vehicleStatus.clutch < 0.9f && (m_vehicleStatus.iCurGear == 0 || m_vehicleStatus.iCurGear == 2))
      {
        rpmScale = min(m_rpmScale, rpmScale);
      }

      float interpspeed = (rpmScale < m_rpmScale) ? m_rpmRelaxSpeed : m_rpmInterpSpeed;
      Interpolate(m_rpmScale, rpmScale, interpspeed, deltaTime);
    }
    
    float rpmSound = max(ENGINESOUND_IDLE_RATIO, m_rpmScale);
    SetSoundParam(eSID_Run, "rpm_scale", rpmSound);
    SetSoundParam(eSID_Ambience, "rpm_scale", rpmSound);

    if (m_currentGear != m_vehicleStatus.iCurGear)
    { 
      // when shifting up from 1st upward, set sound target to low rpm to simulate dropdown 
      // during clutch disengagement
      if (m_currentGear >= 2 && m_vehicleStatus.iCurGear>m_currentGear)
      {
        m_rpmTarget = m_engineShiftDownRPM/m_engineMaxRPM;
        m_rpmScalePrev = m_rpmScale;
                
        if (DoGearSound())
        {
          loadTarget = 0.f;
          m_gearSoundPending = true;        
        }
      }

      if (DoGearSound() && !m_rpmTarget && !(m_currentGear<=2 && m_vehicleStatus.iCurGear<=2) && m_vehicleStatus.iCurGear > m_currentGear)
      {
        // do gearshift sound only for gears higher than 1st forward
        // in case rpmTarget has been set, shift is played upon reaching it        
        GetOrPlaySound(eSID_Gear, 0.f, m_enginePos);        
      }

      m_currentGear = m_vehicleStatus.iCurGear;
    }

    if (loadTarget < 0.f)
    {
      // if not yet set, set load according to pedal
      loadTarget = abs(GetEnginePedal());
    }

    float loadSpeed = 1.f / (loadTarget >= m_load ? LOAD_RAMP_TIME : LOAD_RELAX_TIME);    
    Interpolate(m_load, loadTarget, loadSpeed, deltaTime);    
  }
  else
  {
    m_load = 0.f;
  }

  //SetSoundParam(eSID_Run, "load", m_load);
  SetSoundParam(eSID_Run, "surface", m_surfaceSoundStats.surfaceParam);  
  SetSoundParam(eSID_Run, "scratch", m_surfaceSoundStats.scratching);  

  // tire slip sound
  if (m_maxSoundSlipSpeed > 0.f)
  {
    ISound* pSound = GetSound(eSID_Slip);    

    if (m_surfaceSoundStats.slipRatio > 0.08f)
    { 
      float slipTimerPrev = m_surfaceSoundStats.slipTimer;
      m_surfaceSoundStats.slipTimer += deltaTime;
      
      const static float slipSoundMinTime = 0.12f;
      if (!pSound && slipTimerPrev <= slipSoundMinTime && m_surfaceSoundStats.slipTimer > slipSoundMinTime)
      {
        pSound = PlaySound(eSID_Slip);
      }      
    }
    else if (m_surfaceSoundStats.slipRatio < 0.03f && m_surfaceSoundStats.slipTimer > 0.f)
    {
      m_surfaceSoundStats.slipTimer -= deltaTime;

      if (m_surfaceSoundStats.slipTimer <= 0.f)
      {
        StopSound(eSID_Slip);
        pSound = 0;
        m_surfaceSoundStats.slipTimer = 0.f;
      }      
    }
 
    if (pSound)
    {
      SetSoundParam(eSID_Slip, "slip_speed", m_surfaceSoundStats.slipRatio);
      SetSoundParam(eSID_Slip, "surface", m_surfaceSoundStats.surfaceParam);
      SetSoundParam(eSID_Slip, "scratch", m_surfaceSoundStats.scratching);
      SetSoundParam(eSID_Slip, "in_out", m_soundStats.inout);
    }   
  }
}


//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::UpdateSuspension(const float deltaTime)
{
  FUNCTION_PROFILER( gEnv->pSystem, PROFILE_GAME );

  if (m_pVehicle->GetStatus().health <= 0.f)
    return;

  IPhysicalEntity* pPhysics = GetPhysics();
  bool visible = m_pVehicle->GetGameObject()->IsProbablyVisible();
  bool distant = m_pVehicle->GetGameObject()->IsProbablyDistant();
  
  // update suspension and friction, if needed      
  float speed = m_statusDyn.v.len();
  bool bSuspUpdate = false;
        
  pe_status_nparts tmpStatus;
  int numParts = pPhysics->GetStatus(&tmpStatus);

  const Matrix34& worldTM = m_pVehicle->GetEntity()->GetWorldTM();

  assert(m_wheelParts.size() == m_pVehicle->GetWheelCount());

  float diffSusp = m_suspDampingMax - m_suspDampingMin;    
  float diffStabi = m_stabiMax - m_stabiMin;
  
  if (diffSusp || diffStabi)
  {
    if (abs(m_speedSuspUpdated-speed) > 0.25f) // only update when speed changes
    {
      float maxSpeed = (m_suspDampingMaxSpeed > 0.f) ? m_suspDampingMaxSpeed : 0.15f*m_maxSpeed;
      float speedNorm = min(1.f, speed/maxSpeed);
      
      if (diffSusp)
      {
        m_suspDamping = m_suspDampingMin + (speedNorm * diffSusp);
        bSuspUpdate = true;
      }           

      if (diffStabi)
      {
        m_stabi = m_stabiMin + (speedNorm * diffStabi);
        
        pe_params_car params;
        params.kStabilizer = m_stabi;
        pPhysics->SetParams(&params, 1);        
      }

      m_speedSuspUpdated = speed;    
    }
  }
  
  bool bBraking = m_movementAction.brake && m_isEnginePowered;
  
  m_compressionMax = 0.f;
  m_avgLateralSlip = 0.f;
  m_avgWheelRot = 0.f;
  m_lostContactTimer += deltaTime;  
  int ipart = 0;
  int numSlip = 0;
  int numRot = 0;  
  int soundMatId = 0;
  m_wheelContactsLeft = 0; 
  m_wheelContactsRight = 0;
  m_surfaceSoundStats.scratching = 0;
  
  int numWheels = m_pVehicle->GetWheelCount();
  for (int i=0; i<numWheels; ++i)
  { 
    pe_params_wheel wheelParams;
    bool bUpdate = bSuspUpdate;
    IVehicleWheel* pWheel = m_wheelParts[i]->GetIWheel();

    pe_status_wheel ws;
    ws.iWheel = i;
    if (!pPhysics->GetStatus(&ws))
      continue;

    const Matrix34& wheelTM = m_wheelParts[i]->GetLocalTM(false);

    if (ws.bContact)
    { 
      m_avgWheelRot += abs(ws.w)*ws.r;
      ++numRot;

      if (wheelTM.GetTranslation().x < 0.f)
        ++m_wheelContactsLeft;
      else
        ++m_wheelContactsRight;
      
      // sound-related                   
      if (!distant && visible && !m_surfaceSoundStats.scratching && soundMatId==0 && speed > 0.001f)
      {
        if (gEnv->p3DEngine->GetWaterLevel(&ws.ptContact) > ws.ptContact.z+0.02f)
        {        
          soundMatId = gEnv->pPhysicalWorld->GetWaterMat();
          m_lostContactTimer = 0;
        }
        else if (ws.contactSurfaceIdx > 0 /*&& soundMatId != gEnv->pPhysicalWorld->GetWaterMat()*/)
        {   
          if (m_wheelParts[i]->GetState() == IVehiclePart::eVGS_Damaged1)
            m_surfaceSoundStats.scratching = 1;
          
          soundMatId = ws.contactSurfaceIdx;
          m_lostContactTimer = 0;
        }
      }      
    }
    
    // update friction for handbraking
    if ((!gEnv->bClient || !distant) && m_maxBrakingFriction > 0.f && ws.bContact && (bBraking || m_wheelStats[i].handBraking))
    {
      if (bBraking)
      {
        // apply maxBrakingFriction for handbraking
        float friction = m_maxBrakingFriction;

        if (m_carParams.steerTrackNeutralTurn == 0.f)
        {
          // when steering, keep friction high at inner front wheels to achieve better turning
          Vec3 pos = wheelTM.GetTranslation();

          float diff = max(0.f, pWheel->GetCarGeomParams()->maxFriction - m_maxBrakingFriction);
          float steerAmt = abs(m_action.steer)/DEG2RAD(m_steerMax);

          if (pos.y > 0.f && sgn(m_action.steer)*sgn(pos.x)>0)
            friction = m_maxBrakingFriction + diff * steerAmt;
          else
            friction = m_maxBrakingFriction + diff * (1.f-steerAmt); 
        }
                        
        wheelParams.maxFriction = friction;
        wheelParams.minFriction = friction;

        m_wheelStats[i].handBraking = true;        
      }      
      else
      {
        wheelParams.maxFriction = pWheel->GetCarGeomParams()->maxFriction;
        wheelParams.minFriction = pWheel->GetCarGeomParams()->minFriction;
        
        m_wheelStats[i].handBraking = false;        
      }
      bUpdate = true;
    }    

    
    // update slip friction
    if (!m_wheelStats[i].handBraking)
    {
      // update friction with slip vel
      float lateralSlip(0.f);           
      if (ws.bContact && ws.velSlip.GetLengthSquared() > sqr(0.01f))
      {
        if (m_carParams.steerTrackNeutralTurn == 0.f && (ipart = numParts - numWheels + i) >= 0)
        { 
          pe_status_pos wp;
          wp.ipart = ipart;
          if (pPhysics->GetStatus(&wp))
            lateralSlip = abs(wp.q.GetColumn0() * ws.velSlip);         
        }
        else
        {
          lateralSlip = abs(worldTM.GetColumn0() * ws.velSlip);
        }
      }
      
      if (lateralSlip < 0.001f)
        lateralSlip = 0.f;

      if (ws.bContact)
      {
        m_avgLateralSlip += lateralSlip;
        ++numSlip;
      }

      if (lateralSlip != m_wheelStats[i].lateralSlip)
      { 
        if (m_carParams.steerTrackNeutralTurn == 0.f && pWheel->GetSlipFrictionMod(1.f) != 0.f)          
        {
          float slipMod = pWheel->GetSlipFrictionMod(lateralSlip);          
          
          //wheelParams.maxFriction = pWheel->GetCarGeomParams()->maxFriction + slipMod;            
          //wheelParams.minFriction = wheelParams.maxFriction;
          wheelParams.kLatFriction = pWheel->GetCarGeomParams()->kLatFriction + slipMod;
          m_wheelStats[i].friction = wheelParams.kLatFriction;

          //if (i==0 && lateralSlip > 0.05f && g_pGameCVars->v_draw_slip)
            //CryLog("%.3f, %.3f", lateralSlip, slipMod);
                      
          bUpdate = true;
        }
                  
        m_wheelStats[i].lateralSlip = lateralSlip;          
      }    
    }

    if (bUpdate)
    {
      if (bSuspUpdate)
        wheelParams.kDamping = m_suspDamping;
      
      wheelParams.iWheel = i;      
      pPhysics->SetParams(&wheelParams, THREAD_SAFE);
    }

    // check for hard bump
    if (visible && !distant && (m_bumpMinSusp + m_bumpMinSpeed > 0.f) && m_lastBump > 1.f && ws.suspLen0 > 0.01f && ws.suspLen < ws.suspLen0)
    { 
      // compression as fraction of relaxed length over time
      m_wheelStats[i].compression = ((m_wheelStats[i].suspLen-ws.suspLen)/ws.suspLen0) / deltaTime;
      m_compressionMax = max(m_compressionMax, m_wheelStats[i].compression);
    }
    m_wheelStats[i].suspLen = ws.suspLen;
  }  

  m_lastBump += deltaTime;

  if (visible && !distant && m_pVehicle->GetStatus().speed > m_bumpMinSpeed && m_lastBump > 1.f)
  { 
    if (m_compressionMax > m_bumpMinSusp)
    {
      // do bump sound        
      if (ISound* pSound = PlaySound(eSID_Bump))
      {
        pSound->SetParam("speed", ENGINESOUND_IDLE_RATIO + (1.f-ENGINESOUND_IDLE_RATIO)*m_speedRatio, false);
        pSound->SetParam("intensity", min(1.f, m_bumpIntensityMult*m_compressionMax/m_bumpMinSusp), false);
        m_lastBump = 0;
      }      
    }            
  }   
 
  // compute average lateral slip
  m_wheelContacts = numRot;
  m_avgLateralSlip /= max(1, numSlip);
  m_avgWheelRot /= max(1, numRot); 

  // set surface sound type
  if (visible && !distant && soundMatId != m_surfaceSoundStats.matId)
  { 
    if (m_lostContactTimer == 0.f || m_lostContactTimer > 3.f)
    {
      if (soundMatId > 0)
      {
        m_surfaceSoundStats.surfaceParam = GetSurfaceSoundParam(soundMatId);
      }    
      else
      {
        m_surfaceSoundStats.surfaceParam = 0.f;      
      }   
      m_surfaceSoundStats.matId = soundMatId;
    }
  } 
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::UpdateBrakes(const float deltaTime)
{
  if (m_movementAction.brake || m_pVehicle->GetStatus().health <= 0.f)
    m_action.bHandBrake = 1;
  else
    m_action.bHandBrake = 0;

  if (m_isBreakingOnIdle && m_movementAction.power == 0.0f)
  {
    m_action.bHandBrake = 1;
  }

  if (IsPowered() && m_action.bHandBrake == 1)
  {
    if (m_brakeTimer == 0.f)
    {
      SVehicleEventParams params;
      params.bParam = true;
      m_pVehicle->BroadcastVehicleEvent(eVE_Brake, params);
    }

    m_brakeTimer += deltaTime;  
  }
  else
  {
    if (m_brakeTimer > 0.f)
    {
      SVehicleEventParams params;
      params.bParam = false;
      m_pVehicle->BroadcastVehicleEvent(eVE_Brake, params);

      // airbrake sound
      if (m_airbrakeTime > 0.f && IsPowered())
      { 
        if (m_brakeTimer > m_airbrakeTime)
        {
          char name[256];
          _snprintf(name, sizeof(name), "sounds/vehicles:%s:airbrake", m_pVehicle->GetEntity()->GetClass()->GetName());
          name[sizeof(name)-1] = '\0';
          m_pEntitySoundsProxy->PlaySound(name, Vec3(0), FORWARD_DIRECTION, FLAG_SOUND_DEFAULT_3D);                
        }          
      }  
    }

    m_brakeTimer = 0.f;  
  }
}

//------------------------------------------------------------------------
bool CVehicleMovementStdWheeled::RequestMovement(CMovementRequest& movementRequest)
{
  FUNCTION_PROFILER( gEnv->pSystem, PROFILE_GAME );
  
  if (m_pVehicle->IsPlayerDriving(false))
  {
    assert(0 && "AI Movement request on a player-driven vehicle!");
    return false;
  }

  m_movementAction.isAI = true;
  
	Vec3 worldPos = m_pEntity->GetWorldPos();

	float inputSpeed;
	if (movementRequest.HasDesiredSpeed())
		inputSpeed = movementRequest.GetDesiredSpeed();
	else
		inputSpeed = 0.0f;

	Vec3 moveDir;
	if (movementRequest.HasMoveTarget())
		moveDir = (movementRequest.GetMoveTarget() - worldPos).GetNormalizedSafe();
	else
		moveDir.zero();

	// If the movement vector is nonzero there is a target to drive at.
	if (moveDir.GetLengthSquared() > 0.01f)
	{
		SmartScriptTable movementAbilityTable;
		if (!m_pEntity->GetScriptTable()->GetValue("AIMovementAbility", movementAbilityTable))
		{
			GameWarning("Vehicle '%s' is missing 'AIMovementAbility' LUA table. Required by AI", m_pEntity->GetName());
			return false;
		}

		float	maxSpeed = 0.0f;
		if( !movementAbilityTable->GetValue( "sprintSpeed", maxSpeed ) )
		{
			GameWarning("CVehicleMovementStdWheeled::RequestMovement '%s' is missing 'MovementAbility.sprintSpeed' LUA variable. Required by AI", m_pEntity->GetName());
			return false;
		}

		const Matrix34& entRotMat = m_pEntity->GetWorldTM();
		
		Vec3 forwardDir = entRotMat.TransformVector(FORWARD_DIRECTION);
		forwardDir.z = 0.0f;
    forwardDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);
		Vec3 rightDir(forwardDir.y, -forwardDir.x, 0.0f);

		// Normalize the movement dir so the dot product with it can be used to lookup the angle.
		Vec3 targetDir = moveDir;
		targetDir.z = 0.0f;
		targetDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);

    /// component parallel to target dir
		float	cosAngle = forwardDir.Dot(targetDir);
    Limit(cosAngle, -1.0f, 1.0f);
		float angle = RAD2DEG((float)acos(cosAngle));

    if (targetDir.Dot(rightDir) < 0.0f)
			angle = -angle;

    if (sgn(inputSpeed) < 0.0f)
      angle = -angle;

	//CryLog("angle %f",angle);

    if ( !(angle < 181.0f && angle > -181.0f) )
      angle = 0.0f;

		// Danny no need for PID - just need to get the proportional constant right
		// to turn angle into range 0-1. Predict the angle(=error) between now and 
		// next step - in fact over-predict to account for the lag in steering
    CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
    float dt = (curTime - m_lastSteerUpdateTime).GetSeconds();
    if (dt > 0.0f)
    {
      // this time prediction is to take into account the steering lag and prevent
      // oscillations - really it should be vehicle dependant. 
		  static float steerTimescale = 0.1f;
		  static float steerConst = 0.05f;

		  float predAngle = angle + (angle - m_prevAngle) * steerTimescale / dt;
      m_lastSteerUpdateTime = curTime;
		  m_prevAngle = angle;
      if (m_steerMax > 0.0f)
        m_steering = predAngle / m_steerMax; // nearly exact for real wheeled vehicles - approx for tank
      else
    		m_steering = steerConst * predAngle; // shouldn't really get used
    }  		

    Limit(m_steering, -1.0f, 1.0f);
		
    const SVehicleStatus& status = m_pVehicle->GetStatus();		
		float	speed = forwardDir.Dot(status.vel);

    if (!(speed < 100.0f && speed > -100.0f))
    {
      GameWarning("[CVehicleMovementStdWheeled]: Bad speed from physics: %5.2f", speed);
      speed = 0.0f;
    }

		// If the desired speed is negative, it means that the maximum speed of the vehicle
		// should be used.
		float	desiredSpeed;
		if (movementRequest.HasDesiredSpeed())
			desiredSpeed = movementRequest.GetDesiredSpeed();
		else
			desiredSpeed = 0.0f;
		
		// desiredSpeed is in m/s
		Limit(desiredSpeed, -maxSpeed, maxSpeed);
		// Allow breaking if the error is too high.
		float	clampMin;
		float	clampMax;
		float	absolutedDesiredSpeed = fabs( desiredSpeed );

		if ( desiredSpeed >= 0.0f )
		{
			clampMin =0.0;
			clampMax =1.0;
			if ( fabs( absolutedDesiredSpeed - speed) > absolutedDesiredSpeed * 0.5f)
				clampMin = -1.0f;
		}
		else
		{
			clampMin =-1.0;
			clampMax =0.0;
			if ( fabs( absolutedDesiredSpeed - speed) > absolutedDesiredSpeed * 0.5f)
				clampMax = 1.0f;
		}

		static bool usePID = false;

		if (usePID)
	   {
	 		m_direction = m_dirPID.Update(speed/maxSpeed, desiredSpeed/maxSpeed, clampMin, clampMax);
	   }
	   else
	   {
	     static float accScale = 0.5f;
	     m_direction = (desiredSpeed - speed) * accScale;
	     Limit(m_direction, clampMin, clampMax);
	   }

    static bool dumpSpeed = false;
    if (dumpSpeed)
      gEnv->pLog->Log("speed = %5.2f desiredSpeed = %5.2f power = %5.2f", speed, desiredSpeed, m_direction);

    // let's break - we don't want to move anywhere
    if (fabs(desiredSpeed) < 0.001f)
    {		  
		  m_movementAction.brake = true;
    }
    else
    {		  
		  m_movementAction.brake = false;
    }

    // When the slow down is significant (e.g. fwd to reverse, reverse to fwd) brake
    float speedChange = desiredSpeed - speed;
    if (speed < 0.0f)
      speedChange = -speedChange;
    static float speedChangeForBrake = 10.0f;
    if (speedChange < -speedChangeForBrake)
      m_movementAction.brake = true;
		
		m_movementAction.power = m_direction;
		m_movementAction.rotateYaw = m_steering;
		
    if (m_tireBlownTimer > 1.5f)
    {
      m_movementAction.brake = true;
    }
	}
	else
	{
		// let's break - we don't want to move anywhere		
		m_movementAction.brake = true;
		m_movementAction.rotateYaw = m_steering;
	}
//	CryLog("pathdir %f,%f,%f power %f yaw %f ",moveDir.x,moveDir.y,moveDir.z ,m_direction, m_steering);

	return true;
}

void CVehicleMovementStdWheeled::GetMovementState(SMovementState& movementState)
{
	IPhysicalEntity* pPhysics = GetPhysics();
  if (!pPhysics)
    return;

  if (m_maxSpeed == 0.f)
  {
    pe_status_vehicle_abilities ab;
    pPhysics->GetStatus(&ab);
    m_maxSpeed = ab.maxVelocity * 0.5f; // fixme
  }  

	movementState.minSpeed = 0.0f;
	movementState.maxSpeed = m_maxSpeed;
	movementState.normalSpeed = movementState.maxSpeed;
}


//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::Serialize(TSerialize ser, unsigned aspects) 
{
	CVehicleMovementBase::Serialize(ser, aspects);

	if (ser.GetSerializationTarget() == eST_Network)
	{
		if (aspects&CNetworkMovementStdWheeled::CONTROLLED_ASPECT)
			m_netActionSync.Serialize(ser, aspects);
	}
	else 
	{	
		ser.Value("brakeTimer", m_brakeTimer);
    ser.Value("tireBlownTimer", m_tireBlownTimer);

    ser.Value("m_direction", m_direction);
    ser.Value("m_steering", m_steering);
    ser.Value("m_prevAngle", m_prevAngle);
    ser.Value("m_lastSteerUpdateTime", m_lastSteerUpdateTime);
    m_dirPID.Serialize(ser);
	}
};

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::UpdateSurfaceEffects(const float deltaTime)
{ 
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );
  
  if (0 == g_pGameCVars->v_pa_surface)
  {
    ResetParticles();
    return;
  }

  const SVehicleStatus& status = m_pVehicle->GetStatus();
  if (status.speed < 0.01f)
    return;

  float distSq = m_pVehicle->GetEntity()->GetWorldPos().GetSquaredDistance(gEnv->pRenderer->GetCamera().GetPosition());
  if (distSq > sqr(300.f) || (distSq > sqr(50.f) && !m_pVehicle->GetGameObject()->IsProbablyVisible()))
    return;

  IPhysicalEntity* pPhysics = GetPhysics();

  // process wheeled particles   
  float soundSlip = 0;

  if (DebugParticles())
  {
    float color[] = {1,1,1,1};
    gEnv->pRenderer->Draw2dLabel(100, 280, 1.3f, color, false, "%s:", m_pVehicle->GetEntity()->GetName());
  }
    
  SEnvironmentParticles* envParams = m_pPaParams->GetEnvironmentParticles();
  SEnvParticleStatus::TEnvEmitters::iterator emitterIt = m_paStats.envStats.emitters.begin();
  SEnvParticleStatus::TEnvEmitters::iterator emitterItEnd = m_paStats.envStats.emitters.end();

  for (; emitterIt != emitterItEnd; ++emitterIt)
  { 
    if (emitterIt->layer < 0)
    {
      assert(0);
      continue;
    }

    if (!emitterIt->active)
      continue;

    const SEnvironmentLayer& layer = envParams->GetLayer(emitterIt->layer);

    //if (!layer.active || !layer.IsGroupActive(emitterIt->group))
    
    // scaling for each wheelgroup is based on vehicle speed + avg. slipspeed
    float slipAvg = 0; 
    int cnt = 0;
    bool bContact = false;
    int matId = 0;
    
    int wheelCount = layer.GetWheelCount(emitterIt->group);    
    for (int w=0; w<wheelCount; ++w)
    {
      // all wheels in group
      ++cnt;
      pe_status_wheel wheelStats;
      wheelStats.iWheel = layer.GetWheelAt(emitterIt->group, w) - 1;
      
      if (!pPhysics->GetStatus(&wheelStats))
        continue;

      if (wheelStats.bContact)
      {
        bContact = true;
        
        // take care of water
        if (gEnv->p3DEngine->GetWaterLevel(&wheelStats.ptContact) > wheelStats.ptContact.z+0.02f)
        {
          matId = gEnv->pPhysicalWorld->GetWaterMat();
        }
        else if (wheelStats.contactSurfaceIdx > matId)
          matId = wheelStats.contactSurfaceIdx;

        if (wheelStats.bSlip)
          slipAvg += wheelStats.velSlip.len();
      }
    }

    if (!bContact && !emitterIt->bContact)
      continue;
    
    emitterIt->bContact = bContact;    
    slipAvg /= cnt;

    bool isSlip = !strcmp(layer.GetName(), "slip");    
    float vel = isSlip ? 0.f : m_statusDyn.v.len(); 
    vel += 1.f*slipAvg;
     
    soundSlip = max(soundSlip, slipAvg);       
    
    float countScale = 1;
    float sizeScale = 1;

    if (!bContact || matId == 0)    
      countScale = 0;          
    else
      GetParticleScale(layer, vel, 0.f, countScale, sizeScale);
    
    IEntity* pEntity = m_pVehicle->GetEntity();
    SEntitySlotInfo info;
    info.pParticleEmitter = 0;
    pEntity->GetSlotInfo(emitterIt->slot, info);
        
    if (matId != emitterIt->matId)
    {
      // change effect                        
      const char* effect = GetEffectByIndex(matId, layer.GetName());
      IParticleEffect* pEff = 0;   
              
      if (effect && (pEff = gEnv->p3DEngine->FindParticleEffect(effect)))
      {           
        if (DebugParticles())          
          CryLog("<%s> changes sfx to %s (slot %i)", pEntity->GetName(), effect, emitterIt->slot);

        if (info.pParticleEmitter)
        {   
          // free old emitter and load new one, for old effect to die gracefully           
          info.pParticleEmitter->Activate(false);            
          pEntity->FreeSlot(emitterIt->slot);
        }         
        
        emitterIt->slot = pEntity->LoadParticleEmitter(emitterIt->slot, pEff);
        
        if (emitterIt->slot != -1)
          pEntity->SetSlotLocalTM(emitterIt->slot, Matrix34(emitterIt->quatT));

        info.pParticleEmitter = 0;
        pEntity->GetSlotInfo(emitterIt->slot, info);

        emitterIt->matId = matId;
      }
      else 
      {
        if (DebugParticles())
          CryLog("<%s> found no effect for %i", pEntity->GetName(), matId);

        // effect not available, disable
        //info.pParticleEmitter->Activate(false);
        countScale = 0.f; 
        emitterIt->matId = 0;
      }        
    }

    if (emitterIt->matId == 0)      
      countScale = 0.f;

    if (info.pParticleEmitter)
    {
      SpawnParams sp;
      sp.fSizeScale = sizeScale;
      sp.fCountScale = countScale;
      info.pParticleEmitter->SetSpawnParams(sp);
    }
    
    if (DebugParticles())
    {
      float color[] = {1,1,1,1};
      gEnv->pRenderer->Draw2dLabel(100+330*emitterIt->layer, 300+25*emitterIt->group, 1.2f, color, false, "group %i, matId %i: sizeScale %.2f, countScale %.2f (emit: %i)", emitterIt->group, emitterIt->matId, sizeScale, countScale, info.pParticleEmitter?1:0);
      gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_pVehicle->GetEntity()->GetSlotWorldTM(emitterIt->slot).GetTranslation(), 0.2f, ColorB(0,0,255,200));
    }     
  }
  
  if (m_maxSoundSlipSpeed > 0.f)
    m_surfaceSoundStats.slipRatio = min(soundSlip/m_maxSoundSlipSpeed, 1.f);
}

//------------------------------------------------------------------------
void CVehicleMovementStdWheeled::OnValuesTweaked()
{	
	if (IPhysicalEntity* pPhysicsEntity = GetPhysics())
  {
    pe_params_car params(m_carParams);
		pPhysicsEntity->SetParams(&params);
  }
}


//------------------------------------------------------------------------
bool CVehicleMovementStdWheeled::DoGearSound()
{
  return true;
}

void CVehicleMovementStdWheeled::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_wheelParts);
	s->AddContainer(m_wheelStats);
}

//------------------------------------------------------------------------
CNetworkMovementStdWheeled::CNetworkMovementStdWheeled()
: m_steer(0.0f),
m_pedal(0.0f),
m_brake(false),
m_boost(false)
{
}

//------------------------------------------------------------------------
CNetworkMovementStdWheeled::CNetworkMovementStdWheeled(CVehicleMovementStdWheeled *pMovement)
{
	m_steer = pMovement->m_movementAction.rotateYaw;
	m_pedal = pMovement->m_movementAction.power;
	m_brake = pMovement->m_movementAction.brake;
  m_boost = pMovement->m_boost;
}

//------------------------------------------------------------------------
void CNetworkMovementStdWheeled::UpdateObject(CVehicleMovementStdWheeled *pMovement)
{
	pMovement->m_movementAction.rotateYaw = m_steer;
	pMovement->m_movementAction.power = m_pedal;
	pMovement->m_movementAction.brake = m_brake;
  pMovement->m_boost = m_boost;
}

//------------------------------------------------------------------------
void CNetworkMovementStdWheeled::Serialize(TSerialize ser, unsigned aspects)
{
	if (ser.GetSerializationTarget()==eST_Network)
	{
		if (aspects & CONTROLLED_ASPECT)
		{      
			ser.Value("pedal", m_pedal, 'vPed');
			ser.Value("steer", m_steer, 'vStr');      
			ser.Value("brake", m_brake, 'bool');
      ser.Value("boost", m_boost, 'bool');      
		}		
	}	
}
