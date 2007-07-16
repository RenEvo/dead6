/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Fakes a tire blow

-------------------------------------------------------------------------
History:
- 03:28:2006: Created by Michael Rauh

*************************************************************************/
#include "StdAfx.h"
#include "IVehicleSystem.h"
#include "VehicleDamageBehaviorTire.h"
#include "Game.h"
#include "GameRules.h"


#define TIRE_BLOW_EFFECT "vehicle_fx.chinese_truck.blown_tire1"
#define AI_IMMOBILIZED_TIME 4


//------------------------------------------------------------------------
bool CVehicleDamageBehaviorBlowTire::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
	m_pVehicle = pVehicle;  
	m_isActive = false;  
  m_aiImmobilizedTimer = -1;
  
	SmartScriptTable BlowTireParams;
	if (table->GetValue("BlowTire", BlowTireParams))
	{				
	}

  gEnv->p3DEngine->FindParticleEffect(TIRE_BLOW_EFFECT, "CVehicleDamageBehaviorBlowTire::Init");

	return true;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorBlowTire::Reset()
{
  Activate(false);
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorBlowTire::Activate(bool activate)
{
  if (activate == m_isActive)
    return;

  if (activate && m_pVehicle->IsDestroyed())
    return;

  IVehicleComponent* pComponent = m_pVehicle->GetComponent(m_component.c_str());
  if (!pComponent)
    return;

  IVehiclePart* pPart = pComponent->GetPart(0);
  if (!pPart)
    return;

  // if IVehicleWheel available, execute full damage behavior. if null, only apply effects
  IVehicleWheel* pWheel = pPart->GetIWheel();
  
  if (activate)
  {
    IEntity* pEntity = m_pVehicle->GetEntity();
    IPhysicalEntity* pPhysics = pEntity->GetPhysics();
    const Matrix34& wheelTM = pPart->GetLocalTM(false);
    const SVehicleStatus& status = m_pVehicle->GetStatus();

    if (pWheel)
    {    
      //pPart->ChangeState(IVehiclePart::eVGS_Damaged1, IVehiclePart::eVPSF_Physicalize);
      
      const pe_cargeomparams* pParams = pWheel->GetCarGeomParams();  
      pe_params_wheel wheelParams;
      
      // handle destroyed wheel
      wheelParams.iWheel = pWheel->GetWheelIndex();      
      //wheelParams.bDriving = 0;
      //wheelParams.bBlocked = 1;
      wheelParams.minFriction = wheelParams.maxFriction = 0.5f * pParams->maxFriction;

      //CryLog("blocking wheel %i", wheelParams.iWheel);
      pPhysics->SetParams(&wheelParams); 
      
      if (IVehicleMovement* pMovement = m_pVehicle->GetMovement())
      { 
        SVehicleMovementEventParams params;
        params.pComponent = pComponent;
        params.iValue = pWheel->GetWheelIndex();
        pMovement->OnEvent(IVehicleMovement::eVME_TireBlown, params);
      }

      if (status.speed > 0.1f)
      {
        // add angular impulse
        pe_action_impulse angImp;
        float amount = m_pVehicle->GetMass() * status.speed * Random(0.25f, 0.45f) * -sgn(wheelTM.GetTranslation().x);
        angImp.angImpulse = pEntity->GetWorldTM().TransformVector(Vec3(0,0,amount));    
        pPhysics->Action(&angImp);
      }
      
      m_aiImmobilizedTimer = m_pVehicle->SetTimer(-1, AI_IMMOBILIZED_TIME*1000, this);  
    }

    // add linear impulse       
    pe_action_impulse imp;
    imp.point = pPart->GetWorldTM().GetTranslation();

    float amount = m_pVehicle->GetMass() * Random(0.1f, 0.15f);

    if (pWheel)
    {
      amount *= max(0.5f, min(10.f, status.speed));

      if (status.speed < 0.1f)
        amount = -0.5f*amount;
    }
    else    
      amount *= 0.5f;

    imp.impulse = pEntity->GetWorldTM().TransformVector(Vec3(0,0,amount));
    pPhysics->Action(&imp);     
    
    // effect
    IParticleEffect* pEffect = gEnv->p3DEngine->FindParticleEffect(TIRE_BLOW_EFFECT);
    if (pEffect)
    {
      int slot = pEntity->LoadParticleEmitter(-1, pEffect);
      if (slot > -1)
      { 
        float rotation = pWheel ? 0.5f * gf_PI * -sgn(wheelTM.GetTranslation().x) : gf_PI;
        Matrix34 tm = Matrix34::CreateRotationZ(rotation);        
        tm.SetTranslation(wheelTM.GetTranslation());        
        pEntity->SetSlotLocalTM(slot, tm);
      }
    }

    // burst sound
    /* 
    if (IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)m_pVehicle->GetEntity()->GetProxy(ENTITY_PROXY_SOUND))
    {
      string snd = "Sounds/Vehicles:" + string(m_pVehicle->GetEntity()->GetClass()->GetName()) + string(":burst_tire");
      snd.MakeLower();
      pSoundProxy->PlaySound(snd.c_str(), pPart->GetLocalTM(false).GetTranslation(), Vec3(0,1,0), FLAG_SOUND_DEFAULT_3D);            
    }
    */
  }
  else
  { 
    if (pWheel)
    {
      // restore wheel properties        
      IPhysicalEntity* pPhysics = m_pVehicle->GetEntity()->GetPhysics();    
      pe_params_wheel wheelParams;

      for (int i=0; i<m_pVehicle->GetWheelCount(); ++i)
      { 
        const pe_cargeomparams* pParams = m_pVehicle->GetWheelPart(i)->GetIWheel()->GetCarGeomParams();

        wheelParams.iWheel = i;
        wheelParams.bBlocked = 0;
        wheelParams.suspLenMax = pParams->lenMax;
        wheelParams.bDriving = pParams->bDriving;      
        wheelParams.minFriction = pParams->minFriction;
        wheelParams.maxFriction = pParams->maxFriction;
        pPhysics->SetParams(&wheelParams);
      }
    }  
    
    m_aiImmobilizedTimer = -1;
  }

  m_isActive = activate;      
}


//------------------------------------------------------------------------
void CVehicleDamageBehaviorBlowTire::OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams)
{
  if (event == eVDBE_Hit && behaviorParams.componentDamageRatio >= 1.0f && behaviorParams.pVehicleComponent)
  {
    IGameRules* pGameRules = g_pGame->GetGameRules(); 

    static int htBullet = pGameRules->GetHitTypeId("bullet");
    static int htGaussBullet = pGameRules->GetHitTypeId("gaussbullet");
    static int htFire = pGameRules->GetHitTypeId("fire");

    if (behaviorParams.hitType && (behaviorParams.hitType==htBullet || behaviorParams.hitType==htGaussBullet || behaviorParams.hitType==htFire))
    {     
      m_component = behaviorParams.pVehicleComponent->GetComponentName();
      Activate(true);    
    }
  }
  else if (event == eVDBE_Repair && behaviorParams.componentDamageRatio < 1.f)
  {
    Activate(false);
  }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorBlowTire::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
  switch (event)
  {
  case eVE_Timer:
    {
      if (params.iParam == m_aiImmobilizedTimer)
      {
        // notify AI passengers
        IScriptTable* pTable = m_pVehicle->GetEntity()->GetScriptTable();
        HSCRIPTFUNCTION scriptFunction(NULL);
        
        if (pTable && pTable->GetValue("OnVehicleImmobilized", scriptFunction) && scriptFunction)
        {
          Script::Call(gEnv->pScriptSystem, scriptFunction, pTable);
        }

        m_aiImmobilizedTimer = -1;
      }
    }
    break;
  }
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorBlowTire::Update(const float deltaTime)
{	
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorBlowTire::Serialize(TSerialize ser, unsigned aspects)
{
  if (ser.GetSerializationTarget() != eST_Network)
  {
    bool active = m_isActive;
    ser.Value("isActive", m_isActive);
    ser.Value("component", m_component);
    ser.Value("immobilizedTimer", m_aiImmobilizedTimer);
    
    if (active != m_isActive)    
      Activate(m_isActive);    
  }
}

void CVehicleDamageBehaviorBlowTire::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_component);
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorBlowTire);
