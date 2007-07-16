/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements movement type for tracked vehicles

-------------------------------------------------------------------------
History:
- 13:06:2005: Created by MichaelR

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include <GameUtils.h>

#include "IVehicleSystem.h"
#include "VehicleMovementTank.h"

#define THREAD_SAFE 1

//------------------------------------------------------------------------
CVehicleMovementTank::CVehicleMovementTank()
{
  m_pedalSpeed = 1.5f;
  m_pedalThreshold = 0.2f;
  
  m_steerSpeed = 0.f;
  m_steerSpeedRelax = 0.f;
  m_steerLimit = 1.f;
  
  m_latFricMin = m_latFricMinSteer = m_latFricMax = m_currentFricMin = 1.f;
  m_latSlipMin = m_currentSlipMin = 0.f;
  m_latSlipMax = 5.f;

  m_currPedal = 0;
  m_currSteer = 0;

  m_boostEndurance = 5.f;
  m_boostRegen = m_boostEndurance;
  m_boostStrength = 4.f;

  m_drivingWheels[0] = m_drivingWheels[1] = 0;
  m_steeringImpulseMin = 0.f;
  m_steeringImpulseMax = 0.f;
  m_steeringImpulseRelaxMin = 0.f;
  m_steeringImpulseRelaxMax = 0.f;

  // AI specific
  m_bWideSteerThreshold = true;
}

//------------------------------------------------------------------------
CVehicleMovementTank::~CVehicleMovementTank()
{
}

//------------------------------------------------------------------------
bool CVehicleMovementTank::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
  if (!CVehicleMovementStdWheeled::Init(pVehicle, table))
    return false;
  
  MOVEMENT_VALUE("pedalSpeed", m_pedalSpeed);
  MOVEMENT_VALUE_OPT("pedalThreshold", m_pedalThreshold, table);
  MOVEMENT_VALUE("steerSpeed", m_steerSpeed);
  MOVEMENT_VALUE_OPT("steerSpeedRelax", m_steerSpeedRelax, table);
  MOVEMENT_VALUE_OPT("steerLimit", m_steerLimit, table);  
  MOVEMENT_VALUE("latFricMin", m_latFricMin);
  MOVEMENT_VALUE("latFricMinSteer", m_latFricMinSteer);
  MOVEMENT_VALUE("latFricMax", m_latFricMax);  
  MOVEMENT_VALUE("latSlipMin", m_latSlipMin);
  MOVEMENT_VALUE("latSlipMax", m_latSlipMax);

  MOVEMENT_VALUE_OPT("steeringImpulseMin", m_steeringImpulseMin, table);
  MOVEMENT_VALUE_OPT("steeringImpulseMax", m_steeringImpulseMax, table);
  MOVEMENT_VALUE_OPT("steeringImpulseRelaxMin", m_steeringImpulseRelaxMin, table);
  MOVEMENT_VALUE_OPT("steeringImpulseRelaxMax", m_steeringImpulseRelaxMax, table);
	
  m_movementTweaks.Init(table);
  m_maxSoundSlipSpeed = 10.f;  

	return true;
}

//------------------------------------------------------------------------
void CVehicleMovementTank::PostInit()
{
  CVehicleMovementStdWheeled::PostInit();
  
  for (int i=0; i<m_wheelParts.size(); ++i)
  { 
    if (m_wheelParts[i]->GetIWheel()->GetCarGeomParams()->bDriving)
      m_drivingWheels[m_wheelParts[i]->GetLocalTM(false).GetTranslation().x > 0.f] = m_wheelParts[i];
  }

  assert(m_drivingWheels[0] && m_drivingWheels[1]);
}

//------------------------------------------------------------------------
void CVehicleMovementTank::SetLatFriction(float latFric)
{
  // todo: do calculation per-wheel?
  IPhysicalEntity* pPhysics = GetPhysics();  
  int numWheels = m_pVehicle->GetWheelCount();    
  
  pe_params_wheel params;
  params.kLatFriction = latFric;
  
  for (int i=0; i<numWheels; ++i)
  {    
    params.iWheel = i;    
    pPhysics->SetParams(&params, THREAD_SAFE);
  }

  m_latFriction = latFric;
}


//------------------------------------------------------------------------
void CVehicleMovementTank::ProcessMovement(const float deltaTime)
{ 
  FUNCTION_PROFILER( gEnv->pSystem, PROFILE_GAME );
  
	m_netActionSync.UpdateObject(this);
  
  CVehicleMovementBase::ProcessMovement(deltaTime);

  IPhysicalEntity* pPhysics = GetPhysics();
  MARK_UNUSED m_action.clutch;
  
  if (!(m_actorId && m_isEnginePowered))
  {
    if (m_latFriction != 1.f)
      SetLatFriction(1.f);

    if (m_axleFriction != m_axleFrictionMax)
      UpdateAxleFriction(0.f, false, deltaTime);

    m_action.bHandBrake = (m_damage < 1.f || m_pVehicle->IsDestroyed()) ? 1 : 0;
    m_action.pedal = 0;
    m_action.steer = 0;
    pPhysics->Action(&m_action, 1);
    return;
  }

  const Matrix34& worldTM = m_pVehicle->GetEntity()->GetWorldTM();   
  Vec3 localVel = worldTM.GetInvertedFast().TransformVector(m_statusDyn.v);
  Vec3 localW = worldTM.GetInvertedFast().TransformVector(m_statusDyn.w);
  float speed = m_statusDyn.v.len();
  float speedRatio = min(1.f, speed/m_maxSpeed);

  float actionPedal = abs(m_movementAction.power) > 0.001f ? m_movementAction.power : 0.f;        

  // tank specific:
  // avoid steering input around 0.5 (ask Anton)
  float actionSteer = m_movementAction.rotateYaw;
  float absSteer = abs(actionSteer);
  float steerSpeed = (absSteer < 0.01f && abs(m_currSteer) > 0.01f) ? m_steerSpeedRelax : m_steerSpeed;
  
  if (steerSpeed == 0.f)
  {
    m_currSteer =	sgn(actionSteer);
  }
  else
  { 
    if (m_movementAction.isAI)
    {
      if (fabsf(actionSteer) < 0.2f)
        m_currSteer = actionSteer;
      else
        m_currSteer =	sgn(actionSteer);
    }
    else
    {
      m_currSteer += min(abs(actionSteer-m_currSteer), deltaTime*steerSpeed) * sgn(actionSteer-m_currSteer);        
    }
  }
  Limit(m_currSteer, -m_steerLimit, m_steerLimit);  
  
  if (abs(m_currSteer) > 0.0001f) 
  {
    // if steering, apply full throttle to have enough turn power    
    actionPedal = sgn(actionPedal);
    
    if (actionPedal == 0.f) 
    {
      // allow steering-on-teh-spot only above maxReverseSpeed (to avoid sudden reverse of controls)
      const float maxReverseSpeed = -1.5f;
      actionPedal = max(0.f, min(1.f, 1.f-(localVel.y/maxReverseSpeed)));
      
      // todo
      float steerLim = 0.75f;
      Limit(m_currSteer, -steerLim*m_steerLimit, steerLim*m_steerLimit);
    }
  }

  pPhysics->GetStatus(&m_vehicleStatus);
  int currGear = m_vehicleStatus.iCurGear - 1; // indexing for convenience: -1,0,1,2,..

  UpdateAxleFriction(actionPedal, false, deltaTime);
  UpdateSuspension(deltaTime);  

  float absPedal = abs(actionPedal);  
  
  // pedal ramping   
  if (m_pedalSpeed == 0.f)
    m_currPedal = actionPedal;
  else
  {
    m_currPedal += deltaTime * m_pedalSpeed * sgn(actionPedal - m_currPedal);  
    m_currPedal = clamp_tpl(m_currPedal, -absPedal, absPedal);
  }

  // only apply pedal after threshold is exceeded
  if (currGear == 0 && fabs_tpl(m_currPedal) < m_pedalThreshold) 
    m_action.pedal = 0;
  else
    m_action.pedal = m_currPedal;

  // reverse steering value for backward driving
  float effSteer = m_currSteer * sgn(actionPedal);   
 
  // update lateral friction  
  float latSlipMinGoal = 0.f;
  float latFricMinGoal = m_latFricMin;
  
  if (abs(effSteer) > 0.01f && !m_movementAction.brake)
  {
    latSlipMinGoal = m_latSlipMin;
    
    // use steering friction, but not when countersteering
    if (sgn(effSteer) != sgn(localW.z))
      latFricMinGoal = m_latFricMinSteer;
  }

  Interpolate(m_currentSlipMin, latSlipMinGoal, 3.f, deltaTime);   
  
  if (latFricMinGoal < m_currentFricMin)
    m_currentFricMin = latFricMinGoal;
  else
    Interpolate(m_currentFricMin, latFricMinGoal, 3.f, deltaTime);

  float fractionSpeed = min(1.f, max(0.f, m_avgLateralSlip-m_currentSlipMin) / (m_latSlipMax-m_currentSlipMin));
  float latFric = fractionSpeed * (m_latFricMax-m_currentFricMin) + m_currentFricMin;
  
  if (latFric != m_latFriction)
  { 
    SetLatFriction(latFric);    
  }      
 
  const static float maxSteer = gf_PI/4.f; // fix maxsteer, shouldn't change  
  m_action.steer = m_currSteer * maxSteer;  
   
  if ((!m_movementAction.isAI || g_pGameCVars->v_help_tank_steering) && m_steeringImpulseMin > 0.f && m_wheelContactsLeft != 0 && m_wheelContactsRight != 0)
  {  
    const float maxW = 0.3f*gf_PI;
    float steer = abs(m_currSteer)>0.001f ? m_currSteer : 0.f;    
    float desired = steer * maxW; 
    float curr = -localW.z;
    float err = desired - curr; // err>0 means correction to right 
    Limit(err, -maxW, maxW);

    if (abs(err) > 0.01f)
    { 
      float amount = m_steeringImpulseMin + speedRatio*(m_steeringImpulseMax-m_steeringImpulseMin);
      
      // bigger correction for relaxing
      if (desired == 0.f || (desired*curr>0 && abs(desired)<abs(curr))) 
        amount = m_steeringImpulseRelaxMin + speedRatio*(m_steeringImpulseRelaxMax-m_steeringImpulseRelaxMin);

      float corr = -err * amount * m_statusDyn.mass * deltaTime;

      pe_action_impulse imp;
      imp.iApplyTime = 1;      
      imp.angImpulse = worldTM.GetColumn2() * corr;
      pPhysics->Action(&imp, THREAD_SAFE);

      if (IsProfilingMovement())
      {
        float color[] = {1,1,1,1};
        gEnv->pRenderer->Draw2dLabel(300,300,1.5f,color,false,"err: %.2f ", err);
        gEnv->pRenderer->Draw2dLabel(300,320,1.5f,color,false,"corr: %.3f", corr/m_statusDyn.mass);

        IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
        float len = 4.f * imp.angImpulse.len() / deltaTime / m_statusDyn.mass;
        Vec3 dir = -sgn(corr) * worldTM.GetColumn0(); //imp.angImpulse.GetNormalized();
        pGeom->DrawCone(worldTM.GetTranslation()+Vec3(0,0,5)-(dir*len), dir, 0.5f, len, ColorB(128,0,0,255));        
      }    
    }    
  }
  
  if (m_movementAction.isAI && m_movementAction.brake)
  {
    // by tetsuji
	  m_action.bHandBrake = 0;	
	  m_action.pedal = 0;
  }
  else
  {
	  m_action.bHandBrake = (m_movementAction.brake) ? 1 : 0;	
  }

  if (currGear > 0 && m_vehicleStatus.iCurGear < m_currentGear) 
  {
    // when shifted down, disengage clutch immediately to avoid power/speed dropdown
    m_action.clutch = 1.f;
  }
  
  pPhysics->Action(&m_action, 1);

  if (Boosting())  
    ApplyBoost(speed, 1.2f*m_maxSpeed, m_boostStrength, deltaTime);  

  DebugDrawMovement(deltaTime);

	if (m_netActionSync.PublishActions( CNetworkMovementStdWheeled(this) ))
		m_pVehicle->GetGameObject()->ChangedNetworkState( eEA_GameClientDynamic );
}

//----------------------------------------------------------------------------------
void CVehicleMovementTank::DebugDrawMovement(const float deltaTime)
{
  if (!IsProfilingMovement())
    return;

  CVehicleMovementStdWheeled::DebugDrawMovement(deltaTime);

  IPhysicalEntity* pPhysics = GetPhysics();
  IRenderer* pRenderer = gEnv->pRenderer;
  float color[4] = {1,1,1,1};
  float green[4] = {0,1,0,1};
  ColorB colRed(255,0,0,255);
  float y = 50.f, step1 = 15.f, step2 = 20.f, size=1.3f, sizeL=1.5f;

  if (g_pGameCVars->v_dumpFriction)
  {
    if (m_avgLateralSlip > 0.01f)
    {
      CryLog("%4.2f, %4.2f, %4.2f", m_currSteer, m_avgLateralSlip, m_latFriction);
    }
  }
}


//------------------------------------------------------------------------
bool CVehicleMovementTank::RequestMovement(CMovementRequest& movementRequest)
{
	if (!CVehicleMovementStdWheeled::RequestMovement(movementRequest))	
		return false;
	
	// [mikko]: use hysterisis to control the steering.
	// Decrease this threshold too much and the tank is always correcting. Increase
	// it too much and hit deviates from its path and hits trees etc.
	const static float wideThreshold = 0.12f;
	const static float smallThreshold = 0.08f;

	float threshold = (m_bWideSteerThreshold) ? wideThreshold : smallThreshold;

	if (fabs_tpl(m_movementAction.rotateYaw) < threshold)
	{
		// inside the threshold
		m_movementAction.rotateYaw = 0.0f;
		m_bWideSteerThreshold = true;	    
	}
	else
	{
		// outside the threshold. if the wide steering threshold is exceed switch to smaller one.	    
		m_bWideSteerThreshold = false;	    
	}

	if (m_movementAction.brake)
	{
		m_movementAction.power = 0.f;
		m_movementAction.rotateYaw = 0.f;
	}

	return true;
}

//------------------------------------------------------------------------
void CVehicleMovementTank::Update(const float deltaTime)
{
  CVehicleMovementStdWheeled::Update(deltaTime); 
}

//------------------------------------------------------------------------
void CVehicleMovementTank::UpdateSounds(const float deltaTime)
{ 
  CVehicleMovementStdWheeled::UpdateSounds(deltaTime);
}

//------------------------------------------------------------------------
void CVehicleMovementTank::UpdateSpeedRatio(const float deltaTime)
{  
  float speedSqr = max(sqr(m_avgWheelRot), m_statusDyn.v.len2());
  
  Interpolate(m_speedRatio, min(1.f, speedSqr/sqr(m_maxSpeed)), 5.f, deltaTime);
}

//------------------------------------------------------------------------
void CVehicleMovementTank::StopEngine()
{
  CVehicleMovementStdWheeled::StopEngine();
}

void CVehicleMovementTank::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	CVehicleMovementStdWheeled::GetMemoryStatistics(s);
}