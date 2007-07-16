/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a standard boat movement

-------------------------------------------------------------------------
History:
- 30:05:2005: Created by Michael Rauh

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"

#include "IVehicleSystem.h"
#include "VehicleMovementStdBoat.h"
#include <IAgent.h>
#include "Network/NetActionSync.h"




//------------------------------------------------------------------------
CVehicleMovementStdBoat::CVehicleMovementStdBoat()
: m_velMax( 15 )
, m_velMaxReverse( 10 )
, m_accel( 5 )
, m_turnRateMax( 1 )
, m_turnAccel( 1 )
, m_cornerForceCoeff( 1 )
, m_turnAccelCoeff( 2 )
, m_accelCoeff( 2 )
, m_pushTilt( 0 )
, m_pushOffset(ZERO)
, m_cornerTilt( 0 )
, m_cornerOffset(ZERO)
, m_turnDamping( 0 )
, m_massOffset(ZERO)
, m_pedalLimitReverse(1.f)
, m_waveIdleStrength(ZERO)
, m_waveSpeedMult(0.f)
, m_turnVelocityMult(1.f)
, m_pSplashPos(NULL)
, m_inWater(false)
, m_waveRandomMult(1.f)
, m_velLift(0.f)
, m_waterDensity(100.f)
, m_lifted(false)
{ 
  m_prevAngle = 0.0f;
  m_netActionSync.PublishActions( CNetworkMovementStdBoat(this) );
}

//------------------------------------------------------------------------
CVehicleMovementStdBoat::~CVehicleMovementStdBoat()
{
}

//------------------------------------------------------------------------
bool CVehicleMovementStdBoat::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
  if (!CVehicleMovementBase::Init(pVehicle, table))
    return false;
  
  MOVEMENT_VALUE("velMax", m_velMax);      
  MOVEMENT_VALUE("velMaxReverse", m_velMaxReverse);
  MOVEMENT_VALUE("acceleration", m_accel);      
  MOVEMENT_VALUE("accelerationVelMax", m_accelVelMax);      
  MOVEMENT_VALUE("accelerationMultiplier", m_accelCoeff);         
  MOVEMENT_VALUE("pushTilt", m_pushTilt);     
  MOVEMENT_VALUE("turnRateMax", m_turnRateMax);
  MOVEMENT_VALUE("turnAccel", m_turnAccel);      
  MOVEMENT_VALUE("cornerForce", m_cornerForceCoeff);      
  MOVEMENT_VALUE("cornerTilt", m_cornerTilt);    
  MOVEMENT_VALUE("turnDamping", m_turnDamping); 
  MOVEMENT_VALUE("turnAccelMultiplier", m_turnAccelCoeff);
  MOVEMENT_VALUE_OPT("pedalLimitReverse", m_pedalLimitReverse, table);
  MOVEMENT_VALUE_OPT("turnVelocityMult", m_turnVelocityMult, table);
  MOVEMENT_VALUE_OPT("velLift", m_velLift, table);
    
  table->GetValue("waveIdleStrength", m_waveIdleStrength);
  table->GetValue("waveSpeedMult", m_waveSpeedMult);
  
  const char* helper = "";  
  
  if (table->GetValue("cornerHelper", helper))
	{
		if (IVehicleHelper* pHelper = m_pVehicle->GetHelper(helper))
			m_cornerOffset = pHelper->GetVehicleTM().GetTranslation();
	}

  if (table->GetValue("pushHelper", helper))
	{
		if (IVehicleHelper* pHelper = m_pVehicle->GetHelper(helper))
			m_pushOffset = pHelper->GetVehicleTM().GetTranslation();
	}

	m_movementTweaks.Init(table);

  // compute inertia [assumes box]
  AABB bbox;	
	if (IVehiclePart* massPart = pVehicle->GetPart("mass"))
	{
		bbox = massPart->GetLocalBounds();
	}
	else
	{
		GameWarning("[CVehicleMovementStdBoat]: initialization: No \"mass\" geometry found!");
		m_pEntity->GetLocalBounds(bbox);
	}

  m_maxSpeed = m_velMax;
	float mass = pVehicle->GetMass();
  
  float width = bbox.max.x - bbox.min.x;
  float length = bbox.max.y - bbox.min.y;
  float height = bbox.max.z - bbox.min.z;
  m_Inertia.x = mass * (sqr(length)+ sqr(height)) / 12;
  m_Inertia.y = mass * (sqr(width) + sqr(height)) / 12;
  m_Inertia.z = mass * (sqr(width) + sqr(length)) / 12;
  
  m_massOffset = bbox.GetCenter();

  //CryLog("[StdBoat movement]: got mass offset (%f, %f, %f)", m_massOffset.x, m_massOffset.y, m_massOffset.z);

	m_pSplashPos = m_pVehicle->GetHelper("splashPos");

	if (m_pSplashPos)
		m_lastWakePos = m_pSplashPos->GetWorldTM().GetTranslation();
	else
		m_lastWakePos = m_pVehicle->GetEntity()->GetWorldTM().GetTranslation();

  m_waveTimer = Random()*gf_PI;
  m_diving = false;
  m_wakeSlot = -1;   
  m_waveSoundPitch = 0.f;
  m_rpmPitchDir = 0;
  m_waveSoundAmount = 0.1f;

  // AI related
  // Initialise the direction PID.
  m_direction = 0.0f;
  m_dirPID.Reset();
  m_dirPID.m_kP = 0.6f;
  m_dirPID.m_kD = 0.1f;
  m_dirPID.m_kI = 0.01f;

  // Initialise the steering.
  m_steering = 0.0f;
  m_prevAngle = 0.0f;

  return true;
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::Reset()
{
  CVehicleMovementBase::Reset();

  Lift(false);    
  m_waveTimer = Random()*gf_PI;

  m_direction = 0;
  m_steering = 0;
  m_dirPID.Reset();
  m_prevAngle = 0.0f;
  m_diving = false;  
  m_wakeSlot = -1;
  m_rpmPitchDir = 0;
  m_waveSoundPitch = 0.f;
  m_inWater = false;  
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::Release()
{
  CVehicleMovementBase::Release();
  delete this;
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::Physicalize()
{
	CVehicleMovementBase::Physicalize();
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::PostPhysicalize()
{
  CVehicleMovementBase::PostPhysicalize();

  pe_status_dynamics status;
  if (GetPhysics()->GetStatus(&status))
  {
    m_massOffset = m_pVehicle->GetEntity()->GetWorldTM().GetInverted() * status.centerOfMass;
  }
}

//------------------------------------------------------------------------
bool CVehicleMovementStdBoat::SetParams(const SmartScriptTable &table)
{
  return true;
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::OnEvent(EVehicleMovementEvent event, const SVehicleMovementEventParams& params)
{
  CVehicleMovementBase::OnEvent(event, params);

  if (eVME_BecomeVisible == event)
  { 
    if (g_pGameCVars->v_rockBoats)
      m_pVehicle->NeedsUpdate(IVehicle::eVUF_AwakePhysics);
  }
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::Update(const float deltaTime)
{
  CVehicleMovementBase::Update(deltaTime);

  SetAnimationSpeed(eVMA_Engine, abs(m_rpmScaleSgn));
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::UpdateRunSound(const float deltaTime)
{
  if (m_pVehicle->GetGameObject()->IsProbablyDistant())
    return;

  float soundSpeedRatio = ENGINESOUND_IDLE_RATIO + (1.f-ENGINESOUND_IDLE_RATIO) * m_speedRatio;      

  SetSoundParam(eSID_Run, "speed", soundSpeedRatio);
  SetSoundParam(eSID_Ambience, "speed", soundSpeedRatio);
  //SetSoundParam(eSID_Run, "boost", Boosting() ? 1.f : 0.f);

  float acceleration = min(1.f, abs(m_localAccel.y) / m_accel*max(1.f, m_accelCoeff));
  if (acceleration > 0.5f) 
  {     
    if (ISound* pSound = GetOrPlaySound(eSID_Acceleration, 2.f))
      SetSoundParam(pSound, "acceleration", acceleration);
  }

  float damage = GetSoundDamage();
  if (damage > 0.1f)
  { 
    if (ISound* pSound = GetOrPlaySound(eSID_Damage, 5.f, m_enginePos))
      SetSoundParam(pSound, "damage", damage);        
  }

  // rpm dropdown for waves
  if (m_rpmPitchDir != 0)    
  { 
    float speed = (m_rpmPitchDir > 0) ? 0.1f : -0.8f; // quick down, slow up
    m_waveSoundPitch += deltaTime * speed;

    if (m_waveSoundPitch < -m_waveSoundAmount) // dropdown amount
    {
      m_waveSoundPitch = -m_waveSoundAmount;
      m_rpmPitchDir = 1;
    }      
    else if (m_waveSoundPitch > 0.f)
    {
      m_waveSoundPitch = 0.f;
      m_rpmPitchDir = 0;
    }
  }

  if (m_rpmPitchSpeed>0.f)
  {    
    const float maxPedal = (!m_inWater) ? 1.f : Boosting() ? 0.8f : 0.7f;

    // pitch rpm with pedal          
    float pedal = GetEnginePedal();
    pedal = sgnnz(pedal)*max(ENGINESOUND_IDLE_RATIO, min(maxPedal, abs(pedal))); // clamp "pedal" to [0.2..0.7] range

    float delta = pedal - m_rpmScaleSgn;
    m_rpmScaleSgn = max(-1.f, min(1.f, m_rpmScaleSgn + sgn(delta)*min(abs(delta), m_rpmPitchSpeed*deltaTime)));

    // skip transition around 0 when on pedal (sounds more realistic)
    if (abs(GetEnginePedal()) > 0.001f && abs(delta) > 0.001f && sgn(m_rpmScaleSgn) != sgn(delta) && abs(m_rpmScaleSgn) <= 0.3f)
      m_rpmScaleSgn = sgn(delta)*0.3f;

    // for normal driving, rpm is clamped at max defined by sound dept
    m_rpmScale = abs(m_rpmScaleSgn);
    m_rpmScale = min(1.f, max(ENGINESOUND_IDLE_RATIO, m_rpmScale + m_waveSoundPitch));
    
    SetSoundParam(eSID_Run, "rpm_scale", m_rpmScale);
    SetSoundParam(eSID_Ambience, "rpm_scale", m_rpmScale);
  }
}


//------------------------------------------------------------------------
void CVehicleMovementStdBoat::UpdateSurfaceEffects(const float deltaTime)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );
  
  const Matrix34& worldTM = m_pVehicle->GetEntity()->GetWorldTM();
  Matrix34 worldTMInv = worldTM.GetInverted();

  const SVehicleStatus& status = m_pVehicle->GetStatus();    
  float velDot = status.vel * worldTM.GetColumn1();  
  float powerNorm = min(abs(m_movementAction.power), 1.f);

  SEnvironmentParticles* envParams = m_pPaParams->GetEnvironmentParticles();

  SEnvParticleStatus::TEnvEmitters::iterator end = m_paStats.envStats.emitters.end();
  for (SEnvParticleStatus::TEnvEmitters::iterator emitterIt = m_paStats.envStats.emitters.begin(); emitterIt!=end; ++emitterIt)  
  { 
    if (emitterIt->layer < 0)
    {
      assert(0);
      continue;
    }

    const SEnvironmentLayer& layer = envParams->GetLayer(emitterIt->layer);
  
    IEntity* pEntity = m_pVehicle->GetEntity();
    SEntitySlotInfo info;        
    info.pParticleEmitter = 0;
    pEntity->GetSlotInfo(emitterIt->slot, info);        

    float countScale = 1.f;
    float sizeScale = 1.f;
    float speed = 0.f;

    // check if helper position is beneath water level      
                
    Vec3 emitterWorldPos = worldTM * emitterIt->quatT.t;
    float waterLevel = gEnv->p3DEngine->GetWaterLevel(&emitterWorldPos);
    int matId = 0;
    
    if (emitterWorldPos.z <= waterLevel+0.1f)
    {
      matId = gEnv->pPhysicalWorld->GetWaterMat();
      speed = status.speed;

      bool spray = !strcmp(layer.GetName(), "spray");        
      
      if (spray)
      {
        // slip based          
        speed -= abs(velDot);
      }

      GetParticleScale(layer, speed, powerNorm, countScale, sizeScale);
    }
    else
    {
      countScale = 0.f;
    }
    
    if (matId && matId != emitterIt->matId)
    {
      // change effect       
      IParticleEffect* pEff = 0;                
      const char* effect = GetEffectByIndex( matId, layer.GetName() );

      if (effect && (pEff = gEnv->p3DEngine->FindParticleEffect(effect)))
      {           
        if (DebugParticles())              
          CryLog("%s changes water sfx to %s (slot %i)", pEntity->GetName(), effect, emitterIt->slot);

        if (info.pParticleEmitter)
        {
          info.pParticleEmitter->Activate(false);
          pEntity->FreeSlot(emitterIt->slot);                  
        }

        emitterIt->slot = pEntity->LoadParticleEmitter(emitterIt->slot, pEff);

        if (emitterIt->slot != -1)
          pEntity->SetSlotLocalTM(emitterIt->slot, Matrix34(emitterIt->quatT));

        info.pParticleEmitter = 0;
        pEntity->GetSlotInfo(emitterIt->slot, info);
      }
      else
        countScale = 0.f;
    }

    if (matId)
      emitterIt->matId = matId;

    if (info.pParticleEmitter)
    {
      SpawnParams sp;
      sp.fSizeScale = sizeScale;
      sp.fCountScale = countScale;          
      info.pParticleEmitter->SetSpawnParams(sp);

      if (layer.alignToWater && countScale > 0.f)
      {          
        Vec3 worldPos(emitterWorldPos.x, emitterWorldPos.y, waterLevel+0.05f);

        Matrix34 localTM(emitterIt->quatT);
        localTM.SetTranslation(worldTMInv * worldPos);
        pEntity->SetSlotLocalTM(emitterIt->slot, localTM);           
      }
    }

    if (DebugParticles() && m_pVehicle->IsPlayerDriving())
    {          
      float color[] = {1,1,1,1};
      ColorB red(255,0,0,255);
      IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
      
      const char* effect = info.pParticleEmitter ? info.pParticleEmitter->GetName() : "";
      const Matrix34& slotTM = m_pEntity->GetSlotWorldTM(emitterIt->slot);
      Vec3 ppos = slotTM.GetTranslation();
      
      pAuxGeom->DrawSphere(ppos, 0.2f, red);
      pAuxGeom->DrawCone(ppos, slotTM.GetColumn1(), 0.1f, 0.5f, red);
      gEnv->pRenderer->Draw2dLabel(50, 400+10*emitterIt->slot, 1.2f, color, false, "<%s> water fx: slot %i [%s], speed %.1f, sizeScale %.2f, countScale %.2f (pos %.0f,%0.f,%0.f)", pEntity->GetName(), emitterIt->slot, effect, speed, sizeScale, countScale, ppos.x, ppos.y, ppos.z);        
    }                       
    
  }
}


//------------------------------------------------------------------------
void CVehicleMovementStdBoat::Lift(bool lift)
{ 
  if (lift == m_lifted || m_velLift == 0.f)
    return;

  m_lifted = lift;
}

//------------------------------------------------------------------------
bool CVehicleMovementStdBoat::IsLifted()
{
  return m_lifted;
}


//------------------------------------------------------------------------
void CVehicleMovementStdBoat::ProcessMovement(const float deltaTime)
{  
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

  static const float fWaterLevelMaxDiff = 0.15f; // max allowed height difference between propeller center and water level
  static const float fSubmergedMin = 0.01f;
  static const float fMinSpeedForTurn = 0.5f; // min speed so that turning becomes possible
  
  m_netActionSync.UpdateObject(this);

  CVehicleMovementBase::ProcessMovement(deltaTime);

  IEntity* pEntity = m_pVehicle->GetEntity();
  IPhysicalEntity* pPhysics = pEntity->GetPhysics(); 
  float frameTime = min(deltaTime, 0.1f); 

  if (abs(m_movementAction.power) < 0.001f)
    m_movementAction.power = 0.f;
  if (abs(m_movementAction.rotateYaw) < 0.001f)
    m_movementAction.rotateYaw = 0.f;

  const Matrix34& wTM = pEntity->GetWorldTM();  
  Matrix34 wTMInv = wTM.GetInvertedFast();
    
  Vec3 localVel = wTMInv.TransformVector( m_statusDyn.v );
  Vec3 localW = wTMInv.TransformVector( m_statusDyn.w );   
  
  // check if propeller is in water
  Vec3 worldPropPos = wTM * m_pushOffset;  
  float waterLevelWorld = gEnv->p3DEngine->GetWaterLevel( &worldPropPos );
  float fWaterLevelDiff = worldPropPos.z - waterLevelWorld;  
  
  bool submerged = m_statusDyn.submergedFraction > fSubmergedMin;
  m_inWater = submerged && fWaterLevelDiff < fWaterLevelMaxDiff;
    
  float speed = m_statusDyn.v.len2() > 0.001f ? m_statusDyn.v.len() : 0.f;    
  float speedRatio = min(1.f, speed/m_maxSpeed);  
  float absPedal = abs(m_movementAction.power);
  float absSteer = abs(m_movementAction.rotateYaw);
  float velDotForward = (speed > 0.f) ? m_statusDyn.v.GetNormalized()*wTM.GetColumn1() : 1.f;
  
  // wave stuff and related fx
  float waveFreq = 1.f;
  waveFreq += 3.f*speedRatio;

  float waveTimerPrev = m_waveTimer;
  m_waveTimer += frameTime*waveFreq;

  // new randomized amount for this oscillation
  if (m_waveTimer >= gf_PI && waveTimerPrev < gf_PI) 
    m_waveRandomMult = Random();  
  
  if (m_waveTimer >= 2*gf_PI)  
    m_waveTimer -= 2*gf_PI;    

  float kx = m_waveIdleStrength.x*(m_waveRandomMult+0.3f) + m_waveSpeedMult*speedRatio;
  float ky = m_waveIdleStrength.y * (1.f - 0.5f*absPedal - 0.5f*absSteer);

  Vec3 waveLoc = m_massOffset;
  waveLoc.y += speedRatio*min(0.f, m_pushOffset.y-m_massOffset.y);
  waveLoc = wTM * waveLoc;

  if (localW.x >= 0.f)
    m_diving = false;

  Vec3 wakePos;
	if (m_pSplashPos)
		wakePos = m_pSplashPos->GetWorldTM().GetTranslation();
	else
		wakePos = m_pVehicle->GetEntity()->GetWorldTM().GetTranslation();

  bool visible = m_pVehicle->GetGameObject()->IsProbablyVisible();
  bool doWave = visible && submerged && m_statusDyn.submergedFraction < 0.99f;
    
  if (doWave && !m_isEnginePowered)
    m_pVehicle->NeedsUpdate(IVehicle::eVUF_AwakePhysics);
  
  if (m_isEnginePowered || (visible && !m_pVehicle->GetGameObject()->IsProbablyDistant()))
  {
    if (doWave && (m_isEnginePowered || g_pGameCVars->v_rockBoats))
    { 
      pe_action_impulse waveImp;
      waveImp.angImpulse.x = Boosting() ? 0.f : sin(m_waveTimer) * frameTime * m_Inertia.x * kx;
      
      if (isneg(waveImp.angImpulse.x))
        waveImp.angImpulse.x *= (1.f - min(1.f, 2.f*speedRatio)); // less amplitude for negative impulse      

      waveImp.angImpulse.y = sin(m_waveTimer-0.5f*gf_PI) * frameTime * m_Inertia.y * ky;  
      waveImp.angImpulse.z = 0.f;
      waveImp.angImpulse = wTM.TransformVector(waveImp.angImpulse);
      waveImp.point = waveLoc;

      pPhysics->Action(&waveImp, 1);      
    }
    
    float wakeWaterLevel = gEnv->p3DEngine->GetWaterLevel(&wakePos);
    if (!m_diving && localW.x < -0.03f && speed > 10.f && wakePos.z < m_lastWakePos.z && wakeWaterLevel+0.1f >= wakePos.z)
    {
      m_diving = true;          
      
      IParticleEffect* pEffect = gEnv->p3DEngine->FindParticleEffect("vehicle_fx.vehicles_surface_fx.small_boat_hull", "MovementStdBoat");      
      if (pEffect)
      {
        if (IParticleEmitter* pEmitter = pEntity->GetParticleEmitter(m_wakeSlot))
        {
          pEmitter->Activate(false);
          pEntity->FreeSlot(m_wakeSlot);
          m_wakeSlot = -1;
        }

        SpawnParams spawnParams;
        spawnParams.fSizeScale = spawnParams.fCountScale = 0.5f + 0.25f*speedRatio;
        spawnParams.fSizeScale  += 0.4f*m_waveRandomMult;
        spawnParams.fCountScale += 0.4f*Random();
                
        m_wakeSlot = pEntity->LoadParticleEmitter(m_wakeSlot, pEffect, &spawnParams);        
      }

      // handle splash sound
      PlaySound(eSID_Splash, 0.f, Vec3(0,5,1));      
      SetSoundParam(eSID_Splash, "intensity", 0.2f*speedRatio + 0.5f*m_waveRandomMult);     

      if (m_rpmPitchDir == 0)
      {
        m_rpmPitchDir = -1;
        m_waveSoundPitch = 0.f;
        m_waveSoundAmount = 0.02f + m_waveRandomMult*0.08f;
      }      
    }  

    if (m_wakeSlot != -1)
    { 
      // update emitter local pos to short above waterlevel
      Matrix34 tm;
			if (m_pSplashPos)
				tm = m_pSplashPos->GetVehicleTM();
			else
				tm.SetIdentity();

      Vec3 pos = tm.GetTranslation();
      pos.z = wTMInv.TransformPoint(Vec3(wakePos.x,wakePos.y,wakeWaterLevel)).z + 0.2f;
      tm.SetTranslation(pos);
      pEntity->SetSlotLocalTM(m_wakeSlot, tm);

      if (IsProfilingMovement())
      {
        Vec3 wPos = wTM * tm.GetTranslation();
        ColorB col(128, 128, 0, 200);
        gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(wPos, 0.4f, col);
        gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(wPos, col, wPos+Vec3(0,0,1.5f), col);
      }          
    } 
  }

  m_lastWakePos = wakePos;
  // ~wave stuff 

	if (!m_isEnginePowered)
		return;

  pe_action_impulse linearImp, angularImp, dampImp, liftImp; 
  float turnAccel = 0, turnAccelNorm = 0;

  if (m_inWater)
  { 
    SetSoundParam(eSID_Run, "slip", 0.2f*abs(localVel.x)); 

    // optional lifting (catamarans)
    if (m_velLift > 0.f)
    {
      if (localVel.y > m_velLift && !IsLifted())
        Lift(true);
      else if (localVel.y < m_velLift && IsLifted())
        Lift(false);
    }

    if (Boosting() && IsLifted())
    {
      // additional lift force      
      liftImp.impulse = Vec3(0,0,m_statusDyn.mass*frameTime*(localVel.y/m_velMax)*3.f);
      liftImp.point = wTM * m_massOffset;
      pPhysics->Action(&liftImp, 1);
    }
    
    // apply driving force         
    float a = m_movementAction.power;

    if (sgn(a)*sgn(localVel.y) > 0)
    {
      // reduce acceleration with increasing speed
      float ratio = (localVel.y > 0.f) ? localVel.y/m_velMax : -localVel.y/m_velMaxReverse;      
      a = (ratio>1.f) ? 0.f : sgn(a)*min(abs(a), 1.f-((1.f-m_accelVelMax)*sqr(ratio))); 
    }
    
    if (a != 0)
    {
      if (sgn(a) * sgn(localVel.y) < 0) // "braking"
        a *= m_accelCoeff;    
      else
        a = max(a, -m_pedalLimitReverse);

      Vec3 pushDir(FORWARD_DIRECTION);                
      
      // apply force downwards a bit for more realistic response  
      if (a > 0)
        pushDir = Quat_tpl<float>::CreateRotationAA( DEG2RAD(m_pushTilt), Vec3(-1,0,0) ) * pushDir;

      pushDir = wTM.TransformVector( pushDir );  
      linearImp.impulse = pushDir * m_statusDyn.mass * a * m_accel * frameTime;

      linearImp.point = m_pushOffset;
      linearImp.point.x = m_massOffset.x;
      linearImp.point = wTM * linearImp.point;
      
      pPhysics->Action(&linearImp, 1);
    } 
    
    float roll = 0.f;
    
    // apply steering           
    if (m_movementAction.rotateYaw != 0)
    { 
      if (abs(localVel.y) < fMinSpeedForTurn){ // if forward speed too small, no turning possible
        turnAccel = 0; 
      }
      else 
      {
        int iDir = m_movementAction.power != 0.f ? sgn(m_movementAction.power) : sgn(localVel.y);
        turnAccelNorm = m_movementAction.rotateYaw * iDir * max(1.f, m_turnVelocityMult * speedRatio);    

        // steering and current w in same direction?
        int sgnSteerW = sgn(m_movementAction.rotateYaw) * iDir * sgn(-localW.z);

        if (sgnSteerW < 0)
        { 
          // "braking"
          turnAccelNorm *= m_turnAccelCoeff; 
        }
        else 
        {    
          // reduce turn vel towards max
          float maxRatio = 1.f - 0.15f*min(1.f, abs(localW.z)/m_turnRateMax);
          turnAccelNorm = sgn(turnAccelNorm) * min(abs(turnAccelNorm), maxRatio);
        }

        turnAccel = turnAccelNorm * m_turnAccel;
        //roll = 0.2f * turnAccel; // slight roll        
      }        
    }    
    else 
    { 
      // if no steering, damp rotation                
      turnAccel = localW.z * m_turnDamping;
    }
    
    if (turnAccel != 0)
    {
      Vec3& angImp = angularImp.angImpulse; 
      
      angImp.x = 0.f;
      angImp.y = roll * frameTime * m_Inertia.y;
      angImp.z = -turnAccel * frameTime * m_Inertia.z;      
      
      angImp = wTM.TransformVector( angImp );
      pPhysics->Action(&angularImp, 1);
    }   
    
    if (abs(localVel.x) > 0.01f)  
    { 
      // lateral force         
      Vec3& cornerForce = dampImp.impulse;
      
      cornerForce.x = -localVel.x * m_cornerForceCoeff * m_statusDyn.mass * frameTime;
      cornerForce.y = 0.f;
      cornerForce.z = 0.f;
      
      if (m_cornerTilt != 0)
        cornerForce = Quat_tpl<float>::CreateRotationAA( sgn(localVel.x)*DEG2RAD(m_cornerTilt), Vec3(0,1,0) ) * cornerForce;

      dampImp.impulse = wTM.TransformVector(cornerForce);

      dampImp.point = m_cornerOffset;
      dampImp.point.x = m_massOffset.x;
      dampImp.point = wTM.TransformPoint( dampImp.point );
      pPhysics->Action(&dampImp, 1);         
    }  
  }
  
  if (IsProfilingMovement())
  {
    IRenderer* pRenderer = gEnv->pRenderer;
    static float color[4] = {1,1,1,1};    
    float colorRed[4] = {1,0,0,1};
    float colorGreen[4] = {0,1,0,1};
    float y=50.f, step1=15.f, step2=20.f, size1=1.3f, size2=1.5f;

    pRenderer->Draw2dLabel(5.0f,   y, size2, color, false, "Boat movement");
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size1, color, false, "Speed: %.1f (%.1f km/h)", speed, speed*3.6f);
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "LocalW.z norm: %.2f", abs(localW.z)/m_turnRateMax);
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "AccelY: %.1f", m_localAccel.y);
    if (m_velLift > 0.f)
    {
      pRenderer->Draw2dLabel(5.0f,  y+=step2, size1, m_lifted ? colorGreen : color, false, m_lifted ? "Lifted" : "not lifted");
      pRenderer->Draw2dLabel(5.0f,  y+=step2, size1, color, false, "Impulse lift: %.0f", liftImp.impulse.len());               
    }    
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, m_statusDyn.submergedFraction > fSubmergedMin ? color : colorRed, false, "Submerged: %.2f", m_statusDyn.submergedFraction);
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, fWaterLevelDiff < fWaterLevelMaxDiff ? color : colorRed, false, "WaterLevel: %.2f (max: %.2f)", fWaterLevelDiff, fWaterLevelMaxDiff);
    
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size2, color, false, "Driver input");
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size1, color, false, "power: %.2f", m_movementAction.power);
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "steer: %.2f", m_movementAction.rotateYaw); 

    pRenderer->Draw2dLabel(5.0f,  y+=step2, size2, color, false, "Propelling");
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size1, color, false, "turnAccel (norm/real): %.2f / %.2f", turnAccelNorm, turnAccel);         
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "Impulse acc: %.0f", linearImp.impulse.len());         
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "Impulse steer/damp: %.0f", angularImp.angImpulse.len()); 
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "Impulse corner: %.0f", dampImp.impulse.len());

    pRenderer->Draw2dLabel(5.0f,  y+=step2, size2, color, false, "Waves");
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size1, color, false, "timer: %.1f", m_waveTimer); 
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "frequency: %.2f", waveFreq); 
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "random: %.2f", m_waveRandomMult); 
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "kX: %.2f", kx);     
    pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "kY: %.2f", ky); 

    if (Boosting())
      pRenderer->Draw2dLabel(5.0f,  y+=step1, size1, color, false, "Boost: %.2f", m_boostCounter);

    IRenderAuxGeom* pGeom = pRenderer->GetIRenderAuxGeom();
    ColorB colorB(0,255,0,255);

    pRenderer->DrawLabel(worldPropPos, 1.3f, "WL: %.2f", waterLevelWorld);
    
    pGeom->DrawSphere(worldPropPos, 0.15f, colorB);
    pGeom->DrawSphere(waveLoc, 0.25f, colorB);
    pGeom->DrawLine(waveLoc, colorB, waveLoc+Vec3(0,0,2), colorB);

    // impulses
    DrawImpulse(linearImp, Vec3(0,0,1), 3.f/deltaTime, ColorB(255,0,0,255));
    DrawImpulse(angularImp, Vec3(0,0,1), 2.f/deltaTime, ColorB(128,0,0,255));          
    DrawImpulse(liftImp, Vec3(0,0,6), 2.f/deltaTime, ColorB(0,0,255,255));
  }

  if (m_netActionSync.PublishActions( CNetworkMovementStdBoat(this) ))
    m_pVehicle->GetGameObject()->ChangedNetworkState( eEA_GameClientDynamic );
}

//------------------------------------------------------------------------
void CVehicleMovementStdBoat::DrawImpulse(const pe_action_impulse& action, Vec3 offset, float scale, const ColorB& col)
{
  if (action.impulse.len2()>0)
  {
    IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
    Vec3 start = action.point + offset;
    Vec3 end = start - (action.impulse*scale/m_pVehicle->GetMass());
    Vec3 dir = (start-end).GetNormalizedSafe();
    pGeom->DrawCone(start-1.f*dir, dir, 0.5f, 1.f, col);
    pGeom->DrawLine(start, col, end, col);
    pGeom->DrawSphere(end, 0.25f, col);
  }  
}


//------------------------------------------------------------------------
bool CVehicleMovementStdBoat::RequestMovement(CMovementRequest& movementRequest)
{ 
  if (m_pVehicle->IsPlayerDriving(false))
  {
    assert(0 && "AI Movement request on a player-driven vehicle!");
    return false;
  }

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

	if (movementRequest.HasForcedNavigation())
	{
		moveDir = movementRequest.GetForcedNavigation();
		inputSpeed = moveDir.GetLength();
		moveDir.NormalizeSafe();

		if ( inputSpeed > 0.0001f )
		{
			Matrix33 entRotMat(m_pEntity->GetRotation());
			Vec3 forwardDir = entRotMat.TransformVector(Vec3(0.0f, 1.0f, 0.0f));
			forwardDir.z = 0.0f;
			forwardDir.NormalizeSafe();
			if ( moveDir.Dot( forwardDir ) < 0.0f ){
				inputSpeed = inputSpeed * -1.0f;
			}
		}
	}

	float	angle = 0;
	float	sideTiltAngle = 0;
	float	forwTiltAngle = 0;

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
		if (!movementAbilityTable->GetValue("sprintSpeed", maxSpeed))
		{
			GameWarning("CVehicleMovementStdBoat::RequestMovement '%s' is missing 'MovementAbility.sprintSpeed' LUA variable. Required by AI", m_pEntity->GetName());
			return false;
		}

		Matrix33 entRotMat(m_pEntity->GetRotation());
		Vec3 forwardDir = entRotMat.TransformVector(Vec3(0.0f, 1.0f, 0.0f));
		forwardDir.z = 0.0f;
		forwardDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneY);
		Vec3 rightDir = forwardDir.Cross(Vec3(0.0f, 0.0f, 1.0f));

		// Normalize the movement dir so the dot product with it can be used to lookup the angle.
		Vec3 targetDir = moveDir;
		targetDir.z = 0.0f;
		targetDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);

		/// component parallel to target dir
		float cosAngle = forwardDir.Dot(targetDir);
		Limit(cosAngle, -1.0f, 1.0f);
		angle = RAD2DEG((float) acos(cosAngle));
		if (targetDir.Dot(rightDir) < 0.0f)
			angle = -angle;

		if (!(angle < 181.0f && angle > -181.0f))
			angle = 0.0f;

		// Danny no need for PID - just need to get the proportional constant right
		// to turn angle into range 0-1. Predict the angle(=error) between now and 
		// next step - in fact over-predict to account for the lag in steering
		static float nTimesteps = 10.0f;
		static float steerConst = 0.2f;
		float predAngle = angle + nTimesteps * (angle - m_prevAngle);
		m_prevAngle = angle;
		m_steering = steerConst * predAngle;
		Limit(m_steering, -1.0f, 1.0f);
		    
		const SVehicleStatus& status = m_pVehicle->GetStatus();
		float	speed = forwardDir.Dot(status.vel);

		if (!(speed < 100.0f && speed > -100.0f))
		{
			GameWarning("[CVehicleMovementStdBoat]: Bad speed from physics: %5.2f", speed);
			speed = 0.0f;
		}

		// If the desired speed is negative, it means that the maximum speed of the vehicle
		// should be used.

		float	desiredSpeed = inputSpeed;
		if ( desiredSpeed > maxSpeed && !movementRequest.HasForcedNavigation() )
			desiredSpeed = maxSpeed;

		// Allow breaking if the error is too high.
		float	clampMin = 0.0f;
		if (abs( desiredSpeed - speed ) > desiredSpeed * 0.5f)
			clampMin = -1.0f;

		m_direction = m_dirPID.Update( speed, desiredSpeed, clampMin, 1.0f );

		m_movementAction.power = m_direction;
		m_movementAction.rotateYaw = m_steering;
		m_movementAction.isAI = true;
	}
	else
	{
		// let's break - we don't want to move anywhere
    m_direction = 0;
    m_steering = 0;
		m_movementAction.brake = true;
		m_movementAction.power = m_direction;
    m_movementAction.rotateYaw = m_steering;		
	}

	return true;
}


//------------------------------------------------------------------------
void CVehicleMovementStdBoat::Serialize(TSerialize ser, unsigned aspects) 
{
	CVehicleMovementBase::Serialize(ser, aspects);

  if (ser.GetSerializationTarget() == eST_Network) 
  {
    if (aspects & CNetworkMovementStdBoat::CONTROLLED_ASPECT)
      m_netActionSync.Serialize(ser, aspects);
  }
  else
  {
    ser.Value("lifted", m_lifted);
  }
};

void CVehicleMovementStdBoat::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}


//------------------------------------------------------------------------
CNetworkMovementStdBoat::CNetworkMovementStdBoat()
: m_steer(0.0f)
, m_pedal(0.0f)
, m_boost(false)
{
}

//------------------------------------------------------------------------
CNetworkMovementStdBoat::CNetworkMovementStdBoat(CVehicleMovementStdBoat *pMovement)
{
  m_steer = pMovement->m_movementAction.rotateYaw;
  m_pedal = pMovement->m_movementAction.power;  
  m_boost = pMovement->m_boost;
}

//------------------------------------------------------------------------
void CNetworkMovementStdBoat::UpdateObject(CVehicleMovementStdBoat *pMovement)
{
  pMovement->m_movementAction.rotateYaw = m_steer;
  pMovement->m_movementAction.power = m_pedal;  
  pMovement->m_boost = m_boost;
}

//------------------------------------------------------------------------
void CNetworkMovementStdBoat::Serialize(TSerialize ser, unsigned aspects)
{
  if (ser.GetSerializationTarget()==eST_Network && aspects&CONTROLLED_ASPECT)
  {
    ser.Value("pedal", m_pedal, 'vPed');
    ser.Value("steer", m_steer, 'vStr');   
    ser.Value("boost", m_boost, 'bool');
  }
}

