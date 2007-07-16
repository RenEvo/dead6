/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements Hovercraft movement

-------------------------------------------------------------------------
History:
- 04:04:2005: Created by Michael Rauh

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameUtils.h"
#include <StlUtils.h>
#include "IVehicleSystem.h"
#include "VehicleMovementHovercraft.h"

#define PE_ACTION_THREAD_SAFE 0

void DrawImpulse(const Vec3& impulse, const Vec3& pos);


CVehicleMovementHovercraft::CVehicleMovementHovercraft()
: m_hoverHeight( 0.1f )
, m_hoverVariance( 0.f )
, m_hoverFrequency( 0.f )
, m_numThrusters( 9 )
, m_thrusterMaxHeightCoeff( 1.1f )
, m_velMax( 20 )
, m_accel( 4 )
, m_turnRateMax( 1 )
, m_turnAccel( 1 )
, m_cornerForceCoeff( 1 )
, m_turnAccelCoeff( 2 )
, m_accelCoeff( 2 )
, m_stiffness( 1 )
, m_damping( 1 )
, m_bEngineAlwaysOn( false )
, m_thrusterTilt( 0 )
, m_dampLimitCoeff( 1 )
, m_pushTilt( 0 )
, m_pushOffset(ZERO)
, m_cornerTilt( 0 )
, m_cornerOffset(ZERO)
, m_stabRate(ZERO)
, m_turnDamping( 0 )
, m_massOffset(ZERO)
, m_hoverTimer( 0.f )
, m_linearDamping( 0.1f )
, m_bRetainGravity( false )
, m_bSampleByHelpers( false )
, m_thrusterHeightAdaption( 0.f )
, m_startComplete( 0.f )
, m_thrusterUpdate( 0.1f )
, m_thrusterTimer( 0.f )
, m_contacts( 0 )
{
  m_Inertia.zero();
  m_gravity.zero();

  m_netActionSync.PublishActions( CNetworkMovementHovercraft(this) );
}

//------------------------------------------------------------------------
CVehicleMovementHovercraft::~CVehicleMovementHovercraft()
{
  std::for_each(m_vecThrusters.begin(), m_vecThrusters.end(), stl::container_object_deleter());
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::Release()
{
  CVehicleMovementBase::Release();
  delete this;
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::Reset()
{
  CVehicleMovementBase::Reset();

  m_hoverTimer = 0.f;
  m_startComplete = 0.f;
  m_thrusterTimer = 0.f;
  m_contacts = 0;
  m_prevAction.Clear();

  for (TThrusters::iterator it=m_vecThrusters.begin(); it!=m_vecThrusters.end(); ++it)
  {
    (*it)->enabled = true;
    (*it)->groundContact = false;
  }
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::OnEvent(EVehicleMovementEvent event, const SVehicleMovementEventParams& params)
{
  CVehicleMovementBase::OnEvent(event, params);

  
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::Physicalize()
{
	CVehicleMovementBase::Physicalize();

	pe_simulation_params paramsSim;
	GetPhysics()->GetParams( &paramsSim );
	m_gravity = paramsSim.gravity;	
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::PostPhysicalize()
{
  CVehicleMovementBase::PostPhysicalize();
}

//------------------------------------------------------------------------
bool CVehicleMovementHovercraft::SetParams(const SmartScriptTable &table)
{  
  return true;
}

//------------------------------------------------------------------------
bool CVehicleMovementHovercraft::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
  if (!CVehicleMovementBase::Init(pVehicle, table))
    return false;
     
  MOVEMENT_VALUE("hoverHeight", m_hoverHeight);  
  MOVEMENT_VALUE("hoverVariance", m_hoverVariance);
  MOVEMENT_VALUE("hoverFrequency", m_hoverFrequency);
  table->GetValue("numThrusters", m_numThrusters);
  MOVEMENT_VALUE("thrusterMaxHeightCoeff", m_thrusterMaxHeightCoeff);   
  MOVEMENT_VALUE("stiffness", m_stiffness);     
  MOVEMENT_VALUE("damping", m_damping);
  MOVEMENT_VALUE("velMax", m_velMax);      
  MOVEMENT_VALUE("acceleration", m_accel);      
  MOVEMENT_VALUE("accelerationMultiplier", m_accelCoeff);     
  table->GetValue("pushOffset", m_pushOffset);     
  MOVEMENT_VALUE("pushTilt", m_pushTilt);     
  MOVEMENT_VALUE("linearDamping", m_linearDamping);
  MOVEMENT_VALUE("turnRateMax", m_turnRateMax);
  MOVEMENT_VALUE("turnAccel", m_turnAccel);      
  MOVEMENT_VALUE("cornerForce", m_cornerForceCoeff);    
  table->GetValue("cornerOffset", m_cornerOffset);    
  MOVEMENT_VALUE("cornerTilt", m_cornerTilt);    
  MOVEMENT_VALUE("turnDamping", m_turnDamping); 
  MOVEMENT_VALUE("turnAccelMultiplier", m_turnAccelCoeff); 
  table->GetValue("bEngineAlwaysOn", m_bEngineAlwaysOn);
  table->GetValue("retainGravity", m_bRetainGravity);
  MOVEMENT_VALUE("dampingLimit", m_dampLimitCoeff);
  table->GetValue("stabilizer", m_stabRate);
  table->GetValue("sampleByHelpers", m_bSampleByHelpers);
  MOVEMENT_VALUE("thrusterHeightAdaption", m_thrusterHeightAdaption);
  table->GetValue("thrusterUpdate", m_thrusterUpdate);

	m_movementTweaks.Init(table);
  m_prevAction.Clear();
  
  m_maxSpeed = m_velMax;
  	
  InitThrusters(table);  	
  
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

//--------------------------------------------------------------------------------
bool CVehicleMovementHovercraft::InitThrusters(SmartScriptTable table)
{
  // init thrusters
  // by default put 1 on each corner of bbox and 1 in center. 
  // (equal mass distribution should be given)
  // if sampleByHelpers is used, thrusters are placed at specified helpers

  AABB bbox;
  if (IVehiclePart* massPart = m_pVehicle->GetPart("mass"))
  {
    bbox = massPart->GetLocalBounds();
  }
  else
  {
    GameWarning("[CVehicleMovementStdBoat]: initialization: No \"mass\" geometry found!");
    m_pEntity->GetLocalBounds(bbox);
  }
    
  const Vec3 thrusterDir(0,0,1);

  std::for_each(m_vecThrusters.begin(), m_vecThrusters.end(), stl::container_object_deleter());
  m_vecThrusters.clear();

  SmartScriptTable thrusterTable;
  if (m_bSampleByHelpers)
  {    
    if (table->GetValue("Thrusters", thrusterTable))    
      m_numThrusters = thrusterTable->Count();    
    else
      m_bSampleByHelpers = false;
  }

  m_vecThrusters.reserve(m_numThrusters);

  // center
  Vec3 center = bbox.GetCenter();
  center.z = bbox.min.z;

  if (!m_bSampleByHelpers)
  {
    // distribute thrusters
    assert(m_numThrusters >= 1 && m_numThrusters <= 9);    

    if (m_numThrusters >= 4)
    {
      // min & max
      m_vecThrusters.push_back( new SThruster( bbox.min, thrusterDir ));  
      m_vecThrusters.push_back( new SThruster( Vec3( bbox.max.x, bbox.max.y, bbox.min.z ), thrusterDir ));
      // remaining corners
      m_vecThrusters.push_back( new SThruster( Vec3( bbox.min.x, bbox.max.y, bbox.min.z ), thrusterDir ));
      m_vecThrusters.push_back( new SThruster( Vec3( bbox.max.x, bbox.min.y, bbox.min.z ), thrusterDir ));
    }

    if (m_numThrusters >= 8)
    {
      // center along left and right side
      m_vecThrusters.push_back( new SThruster( Vec3( bbox.min.x, center.y, bbox.min.z ), thrusterDir ));
      m_vecThrusters.push_back( new SThruster( Vec3( bbox.max.x, center.y, bbox.min.z ), thrusterDir ));
      // middle along front and rear edge
      m_vecThrusters.push_back( new SThruster( Vec3( center.x, bbox.max.y, bbox.min.z ), thrusterDir ));
      m_vecThrusters.push_back( new SThruster( Vec3( center.x, bbox.min.y, bbox.min.z ), thrusterDir ));
    }

    // center thruster
    if (m_numThrusters == 1 || m_numThrusters == 5 || m_numThrusters == 9)
      m_vecThrusters.push_back( new SThruster( center, thrusterDir ));      

    for (int i=0; i<m_numThrusters; ++i)
    {
      m_vecThrusters[i]->heightInitial = m_vecThrusters[i]->pos.z;
      m_vecThrusters[i]->hoverHeight = m_hoverHeight;
      m_vecThrusters[i]->hoverVariance = m_hoverVariance;
      m_vecThrusters[i]->heightAdaption = 0.f;
    }
  }  
  else 
  { 
    // place thrusters at helpers    
    for (int i=0; i<m_numThrusters; ++i)
    {
      m_vecThrusters.push_back( new SThruster( Vec3(ZERO), thrusterDir ));    

      SmartScriptTable thruster;
      if (thrusterTable->GetAt(i+1, thruster))
      {
				m_vecThrusters[i]->pHelper = NULL;
				m_vecThrusters[i]->pos.zero();

        const char* pHelperName;
        if (thruster->GetValue("helper", pHelperName))
				{
					if (IVehicleHelper* pHelper = m_pVehicle->GetHelper(pHelperName))
					{
						m_vecThrusters[i]->pHelper = pHelper;
						m_vecThrusters[i]->pos = pHelper->GetVehicleTM().GetTranslation();
					}
				}

        m_vecThrusters[i]->heightInitial = m_vecThrusters[i]->pos.z;

        float hoverHeight = -1.f;
        thruster->GetValue("hoverHeight", hoverHeight);
        m_vecThrusters[i]->hoverHeight = (hoverHeight >= 0.f) ? hoverHeight : m_hoverHeight;

        float variance = -1.f;
        thruster->GetValue("hoverVariance", variance);
        m_vecThrusters[i]->hoverVariance = (variance >= 0.f) ? variance : m_hoverVariance;

        float heightAdaption = -1.f;
        thruster->GetValue("heightAdaption", heightAdaption);
        m_vecThrusters[i]->heightAdaption = (heightAdaption >= 0.f) ? heightAdaption : m_thrusterHeightAdaption;
        
        thruster->GetValue("cylinder", m_vecThrusters[i]->cylinderRadius);        

        if (m_vecThrusters[i]->heightAdaption > 0.f)
        {
          IVehiclePart* pPart;
					if (m_vecThrusters[i]->pHelper)
						pPart = m_vecThrusters[i]->pHelper->GetParentPart();
					else
						pPart = NULL;

          if (pPart)
          {
            IVehiclePart* pParent = pPart->GetParent();

            if (pParent)
            {
              pPart->SetMoveable();
              pParent->SetMoveable();

              m_vecThrusters[i]->pPart = pPart;
              m_vecThrusters[i]->pParentPart = pParent;  

              Vec3 partPos = pParent->GetLocalTM(false).GetTranslation();
              m_vecThrusters[i]->levelOffsetInitial = partPos.z;
            }            
          }          
        }
        
        thruster->GetValue("pushing", m_vecThrusters[i]->pushing);
      }      
    }

    assert(m_vecThrusters.size() == m_numThrusters);
  }

  // tilt thruster direction to outside   
  if (table->GetValue("thrusterTilt", m_thrusterTilt)){
    if (m_thrusterTilt > 0.f && m_thrusterTilt < 90.f)
    {
      m_thrusterTilt = DEG2RAD(m_thrusterTilt);
      for (int i=0; i<m_numThrusters; ++i)
      {
        // tilt towards center.. rays are shot to -dir later
        if (m_vecThrusters[i]->pos == center){          
          continue;
        }
        Vec3 axis = Vec3(m_vecThrusters[i]->pos - center).Cross( thrusterDir );
        axis.Normalize();
        m_vecThrusters[i]->dir = Quat_tpl<float>::CreateRotationAA( m_thrusterTilt, axis ) * m_vecThrusters[i]->dir;
        m_vecThrusters[i]->tiltAngle = m_thrusterTilt;
      }
    }
  }

  // add thruster offset (so they don't touch ground intially)
  float thrusterBottomOffset = 0;
  if (table->GetValue("thrusterBottomOffset", thrusterBottomOffset) && thrusterBottomOffset > 0.f)
  {    
    for (int i=0; i<m_numThrusters; ++i){
      m_vecThrusters[i]->pos.z += thrusterBottomOffset;
    }
    m_hoverHeight += thrusterBottomOffset; 
  }

  float mass = m_pVehicle->GetMass();

  // compute inertia [assumes box]
  float width = bbox.max.x - bbox.min.x;
  float length = bbox.max.y - bbox.min.y;
  float height = bbox.max.z - bbox.min.z;
  m_Inertia.x = mass * (sqr(length)+ sqr(height)) / 12;
  m_Inertia.y = mass * (sqr(width) + sqr(height)) / 12;
  m_Inertia.z = mass * (sqr(width) + sqr(length)) / 12;

  m_massOffset = bbox.GetCenter();
  CryLog("[Hovercraft movement]: got mass offset (%f, %f, %f)", m_massOffset.x, m_massOffset.y, m_massOffset.z);

  float gravity = m_gravity.IsZero() ? 9.81f : m_gravity.len();
  m_liftForce = mass * gravity;

  assert(m_numThrusters == m_vecThrusters.size());

  int pushingThrusters = 0;
  for (int i=0; i<m_numThrusters; ++i)
  {
    if (m_vecThrusters[i]->pushing)
      ++pushingThrusters;
  }    

  float fThrusterForce = (m_numThrusters > 0) ? m_liftForce/pushingThrusters : 0.f;
  
  for (int i=0; i<m_numThrusters; ++i)
  {
    if (m_vecThrusters[i]->pushing)
      m_vecThrusters[i]->maxForce = fThrusterForce;
  }   

  return true;
}


//------------------------------------------------------------------------
bool CVehicleMovementHovercraft::StartEngine(EntityId driverId)
{	
  if (!CVehicleMovementBase::StartEngine(driverId))
    return false;

  m_startComplete = 0.f;
  m_thrusterTimer = 0.f;
  m_contacts = 0;

  for (int i = 0; i < m_numThrusters; i++)
  {	
    m_vecThrusters[i]->hit = false;
    m_vecThrusters[i]->prevDist = -1.f;
  }		

  return true;
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::StopEngine()
{
  CVehicleMovementBase::StopEngine();  
}


//------------------------------------------------------------------------
void CVehicleMovementHovercraft::ProcessMovement(const float deltaTime)
{ 
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::Update(const float deltaTime)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );
  
  CVehicleMovementBase::Update(deltaTime);
  
  static const Vec3 thrusterForceDir(0,0,1); 
  
  m_netActionSync.UpdateObject(this);
    
  bool bPowered = IsPowered();

  if (!bPowered || !m_bMovementProcessingEnabled)
    return;
    
  IPhysicalEntity* pPhysics = GetPhysics();

  pe_status_dynamics statusDyn;    
  pe_action_impulse linearImp, angularImp, dampImp, stabImp;    
    
  ray_hit hit;
  primitives::cylinder cyl;
  static const int objTypes = ent_all & ~ent_living;    
  static const unsigned int flags = rwi_stop_at_pierceable|rwi_ignore_back_faces|rwi_ignore_noncolliding;  
  IPhysicalEntity *pSkip = GetPhysics();  
  int hits = 0;
      
  float frameTime = min(deltaTime, 0.1f); // take care of high frametimes
  
  m_startComplete += frameTime;  
  m_thrusterTimer += frameTime;
  bool bThrusterUpdate = (m_thrusterUpdate == 0.f || m_thrusterTimer >= m_thrusterUpdate);
        
	if (!pPhysics->GetStatus( &statusDyn ))
	{
		GameWarning( "[VehicleMovementHovercraft]: '%s' is missing physics status", m_pEntity->GetName() );
		return;
	}
  
  const Matrix34& wTM = m_pEntity->GetWorldTM();

  Vec3 up = wTM.GetColumn2(); // check attitude
  if (up.z < 0.1f)
    return;

  Matrix34 wTMInv = wTM.GetInvertedFast();
  Vec3 localVel = wTMInv.TransformVector( statusDyn.v );
  Vec3 localW = wTMInv.TransformVector( statusDyn.w );
  
  const float minContacts = 0.33f*m_numThrusters;  

  // update hovering height  
  if (m_hoverFrequency > 0.f)
  {
    m_hoverTimer += deltaTime*m_hoverFrequency;
    if (m_hoverTimer > 2*gf_PI)
      m_hoverTimer -= 2*gf_PI;    
  }
  
  if (bPowered)  
  {    
    if (bThrusterUpdate)
    {
      pe_action_impulse thrusterImp;        
      bool bStartComplete = (m_startComplete > 1.5f);    
      m_contacts = 0;
      
      TThrusters::const_iterator iter;
      for (iter=m_vecThrusters.begin(); iter!=m_vecThrusters.end(); ++iter)
      { 
        SThruster* pThruster = *iter;

        if (m_bSampleByHelpers)
        {
          // update thruster positions        
          if (pThruster->pHelper)
						pThruster->pos = pThruster->pHelper->GetVehicleTM().GetTranslation();
					else
						pThruster->pos.zero();
        }

        if (!pThruster->enabled)
          continue;

        // thruster-dependent hover height alternation
        float hoverHeight = pThruster->hoverHeight;
        if (pThruster->hoverVariance > 0.f && bStartComplete)
        {
          if (abs(m_movementAction.power) > 0.1f)
          {
            // commented the below, use default height during moving
            //hoverHeight += pThruster->hoverVariance*pThruster->hoverHeight;
          }
          else
          {
            if (abs(m_prevAction.power) > 0.1f)
            {
              m_hoverTimer = 0.f;
            }
            hoverHeight += sin(m_hoverTimer)*(pThruster->hoverVariance*pThruster->hoverHeight);
          }
        } 

        Vec3 thrusterPos = wTM.TransformPoint( pThruster->pos );
        Vec3 thrusterDir = wTM.TransformVector( pThruster->dir );

        float cosAngle = cos(pThruster->tiltAngle);            
        float hitDist = pThruster->prevDist;

        if (bThrusterUpdate)
        {
          hitDist = 0.f;
          pThruster->hit = false;
          pThruster->prevHit.zero();

          if (!(pThruster->cylinderRadius > 0.f))
          {
            if (hits = gEnv->pPhysicalWorld->RayWorldIntersection(thrusterPos, -m_thrusterMaxHeightCoeff*(hoverHeight/cosAngle)*thrusterDir, objTypes, flags, &hit, 1, pSkip))        
            {
              pThruster->hit = true;
              // reset hit dist to vertical length
              hitDist = hit.dist * cosAngle;
              pThruster->prevHit = hit.pt;
            }
          }
          else
          {        
            Vec3 ptEnd = thrusterPos - m_thrusterMaxHeightCoeff*hoverHeight*thrusterDir;
            cyl.center = thrusterPos;
            cyl.axis = ptEnd - thrusterPos;
            cyl.axis.NormalizeFast();
            cyl.hh = 0.1f;
            cyl.r = pThruster->cylinderRadius;

            if (IsProfilingMovement())
            {
              IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
              pAuxGeom->DrawCone(thrusterPos, cyl.axis, cyl.r, hoverHeight, ColorB(255,0,0,128));
              pAuxGeom->DrawCone(ptEnd, -cyl.axis, cyl.r, hoverHeight, ColorB(255,0,0,128));
            }

            geom_contact *pContact = 0;          
            hitDist = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(cyl.type, &cyl, ptEnd-thrusterPos, ent_all & ~ent_living, &pContact, 0, geom_colltype0, 0, 0, 0, &pSkip, 1);

            if (hitDist > 0.f)
            {
              pThruster->hit = true;

              if (pContact)
                pThruster->prevHit = pContact[0].pt;
            }
          }

          if (!pThruster->hit)
          {        
            float delta = thrusterPos.z - gEnv->p3DEngine->GetWaterLevel( &thrusterPos );
            if ( delta > 0.f && delta < m_thrusterMaxHeightCoeff * hoverHeight )
            {
              pThruster->hit = true;
              hitDist = delta;
              pThruster->prevHit = thrusterPos - Vec3(0,0,delta);
            }
          }
        }      

        // neutral pos
        float hoverError = 0.f;

        if (pThruster->hit)
        {        
          ++m_contacts;

          if (pThruster->pushing)
          {
            hoverError = min(hitDist, m_thrusterMaxHeightCoeff*hoverHeight)-hoverHeight;              

            // positive force means upward push
            float basicForce = pThruster->maxForce;
            float springForce = -hoverError/hoverHeight * m_stiffness * basicForce;

            // damp movement of thruster WRT ground. only damp when thruster is below hoverheight.
            float relVel = (m_thrusterTimer > 0 && hitDist < hoverHeight && isnonneg(pThruster->prevDist)) ? (hitDist - pThruster->prevDist)/m_thrusterTimer : 0; 

            float dampForce = -1.f*CLAMP(relVel, -m_dampLimitCoeff, m_dampLimitCoeff) * m_damping * basicForce;
            if (abs(dampForce) < 0.05*basicForce)
              dampForce = 0;

            float force = basicForce + springForce + dampForce; 
            Vec3 forceDir = pThruster->heightAdaption > 0.f ? thrusterForceDir : wTM.TransformVector(thrusterForceDir);

            thrusterImp.impulse = forceDir * force * m_thrusterTimer;
            thrusterImp.point = thrusterPos; 
            //thrusterImp.iApplyTime = 2;
            pPhysics->Action(&thrusterImp, PE_ACTION_THREAD_SAFE);        

          }        
        } 

        pThruster->prevDist = hitDist;               

        if (IsProfilingMovement())
        {
          //gEnv->pRenderer->DrawLabel(thrusterPos, 1.4f, "e: %.2f, c: %.2f, a: %.2f", error, correction, RAD2DEG(deltaAngle));
          IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();                  
          ColorB col(0,255,255,255);

          Vec3 current(pThruster->pos);        
          pGeom->DrawSphere(wTM*current, 0.2f, ColorB(0,255,0,255));

          if (pThruster->heightAdaption > 0.f)
          {
            Vec3 center(pThruster->pos.x, pThruster->pos.y, pThruster->heightInitial);
            Vec3 min = center + Vec3(0,0,-pThruster->heightAdaption);
            Vec3 max = center + Vec3(0,0,pThruster->heightAdaption);                  

            pGeom->DrawSphere(wTM*min, 0.2f, col);
            pGeom->DrawSphere(wTM*max, 0.2f, col);
            pGeom->DrawSphere(wTM*center, 0.15f, col);
            pGeom->DrawLine(wTM*min, col, wTM*max, col);
          }

          if (pThruster->hit)
          {
            ColorB col2(255,255,0,255);
            Vec3 lower = pThruster->prevHit + Vec3(sgn(pThruster->pos.x)*0.25f,0,0);
            Vec3 upper = lower + hoverHeight*Vec3(0,0,1);
            pGeom->DrawSphere(upper, 0.1f, col2);
            pGeom->DrawLine(lower, col2, upper, col2);
          }
        }
      }

      if (bThrusterUpdate)
        m_thrusterTimer = (m_thrusterUpdate == 0.f) ? 0.f : m_thrusterTimer-m_thrusterUpdate;
    }
    
    // flight stabilization     
    if (m_contacts < minContacts)
    {      
      Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(wTM));
      Vec3 correction;      
      
      for (int i=0; i<3; ++i)
      {
        // apply stabRate as correction acceleration, but not more than current angular vel 
        //correction[i] = -sgn(localW[i]) * min( abs(localW[i]), m_stabRate[i]*frameTime );
      
        // deviation from desired orientation                   
        correction[i] = m_stabRate[i] * -angles[i] * frameTime;
      }
      correction = correction.CompMul(m_Inertia);
      correction = wTM.TransformVector( correction );      
      stabImp.angImpulse = correction;
      stabImp.point = wTM.TransformPoint( m_massOffset );
      pPhysics->Action(&stabImp, PE_ACTION_THREAD_SAFE);
    }
  }   
    
  // apply driving force  
  float a = 0; 
  if (m_contacts >= minContacts)  
  {     
    a = m_movementAction.power * m_accel;
    Vec3 pushDir(FORWARD_DIRECTION);
    
    if (abs(a) > 0.001f)
    {
      if (sgn(a) * sgn(localVel.y) < 0){ // "braking"
        a *= m_accelCoeff;
      }
      if (abs(localVel.y) > m_velMax && sgn(a)*sgn(localVel.y)>0){ // check max vel
        a = 0;
      }
      if (a > 0){      
        // apply force downwards a bit for more realistic response  
        pushDir = Quat_tpl<float>::CreateRotationAA( DEG2RAD(m_pushTilt), Vec3(-1,0,0) ) * pushDir;
      }
    }
    else
    {
      // damp linear movement            
      pushDir = -localVel;
      pushDir.z = 0;
      a = m_linearDamping;
    }
    
    if (a != 0)
    {
      pushDir = wTM.TransformVector( pushDir );  

      linearImp.impulse = pushDir * statusDyn.mass * a * frameTime;       
      linearImp.point = m_pushOffset;
      linearImp.point.x += m_massOffset.x;
      linearImp.point.y += m_massOffset.y;
      linearImp.point = wTM.TransformPoint( linearImp.point );
      pPhysics->Action(&linearImp, PE_ACTION_THREAD_SAFE);
    }
  }  

  // apply steering 
  // (Momentum = alpha * I)  
  float turnAccel = 0;  
  Vec3 momentum(0,0,-1); // use momentum along -z to keep negative steering to the left      
  
  if (m_contacts >= minContacts && abs(m_movementAction.rotateYaw) > 0.001f)
  {   
    int iDir = m_movementAction.power != 0.f ? sgn(m_movementAction.power) : sgn(localVel.y);
    turnAccel = m_movementAction.rotateYaw * m_turnAccel * iDir;
    
    // steering and current w in same direction?
    int sgnSteerW = sgn(m_movementAction.rotateYaw) * iDir * sgn(-localW.z);

    if (sgnSteerW < 0){ 
      turnAccel *= m_turnAccelCoeff; // "braking"      
    }
    else if (abs(localW.z) > m_turnRateMax){ 
      turnAccel = 0; // check max turn vel
    }
  }
  else 
  { 
    // if no steering, damp rotation                
    turnAccel = localW.z * m_turnDamping;
  }
  
  if (abs(turnAccel) > 0.0001f)
  {
    momentum *= turnAccel * m_Inertia.z * frameTime;
    momentum = wTM.TransformVector( momentum );
    angularImp.angImpulse = momentum;
    angularImp.point = wTM.TransformPoint( m_massOffset );
    pPhysics->Action(&angularImp, PE_ACTION_THREAD_SAFE);
  }          
  
  // lateral force   
  if (localVel.x != 0 && m_cornerForceCoeff > 0.f && m_contacts >= minContacts)  
  {
    Vec3 cornerForce(0,0,0);
    cornerForce.x = -localVel.x * m_cornerForceCoeff * statusDyn.mass * frameTime;    
    cornerForce = Quat_tpl<float>::CreateRotationAA( sgn(localVel.x)*DEG2RAD(m_cornerTilt), Vec3(0,1,0) ) * cornerForce;
    
    dampImp.impulse = wTM.TransformVector( cornerForce );

    dampImp.point = m_cornerOffset;    
    dampImp.point.x += m_massOffset.x;
    dampImp.point.y += m_massOffset.y;
    dampImp.point.x *= sgn(localVel.x);
    dampImp.point = wTM.TransformPoint( dampImp.point );
    
    pPhysics->Action(&dampImp, PE_ACTION_THREAD_SAFE);   
  } 

  if (IsProfilingMovement())
  {
    IRenderer* pRenderer = gEnv->pRenderer;
    static float color[4] = {1,1,1,1};
    static float red[4] = {1,0,0,1};
    float y=50.f, step1=15.f, step2=20.f, size1=1.f, size2=1.5f;

    float speed = statusDyn.v.len();

    pRenderer->Draw2dLabel(5.0f,   y, size2, color, false, "Hovercraft");
    pRenderer->Draw2dLabel(5.0f,  y+=step2, 1.5f, color, false, "Speed: %.1f (%.1f km/h)", speed, speed*3.6f);    
    pRenderer->Draw2dLabel(5.0f,  y+=step2, 1.5f, color, false, "HoverHeight: %.2f", m_hoverHeight);
    pRenderer->Draw2dLabel(5.0f,  y+=step2, 1.5f, (m_contacts < minContacts) ? color : red, false, "Contacts: %i", m_contacts);
    
    pRenderer->Draw2dLabel(5.0f,  y+=step2, size2, color, false, "Driver input");
    pRenderer->Draw2dLabel(5.0f,  y+=step2, 1.5f, color, false, "power: %.2f", m_movementAction.power);
    pRenderer->Draw2dLabel(5.0f,  y+=step1, 1.5f, color, false, "steer: %.2f", m_movementAction.rotateYaw); 

    pRenderer->Draw2dLabel(5.0f,  y+=step2, size2, color, false, "Propelling");    
    
    pRenderer->Draw2dLabel(5.0f,  y+=step2, 1.5f, color, false, "Impulse linear: %.0f", linearImp.impulse.len()); 
    DrawImpulse(linearImp.impulse, linearImp.point);

    pRenderer->Draw2dLabel(5.0f,  y+=step1, 1.5f, color, false, "Momentum steer: %.0f", angularImp.angImpulse.len()); 
    
    pRenderer->Draw2dLabel(5.0f,  y+=step1, 1.5f, color, false, "Impulse corner: %.0f", dampImp.impulse.len());     
    DrawImpulse(dampImp.impulse, dampImp.point);
    
    pRenderer->Draw2dLabel(5.0f,  y+=step1, 1.5f, color, false, "Impulse stabi: %.0f", stabImp.angImpulse.len());
    DrawImpulse(stabImp.angImpulse, stabImp.point);
  }

  if (m_netActionSync.PublishActions( CNetworkMovementHovercraft(this) ))
    m_pVehicle->GetGameObject()->ChangedNetworkState( eEA_GameClientDynamic );

  m_prevAction = m_movementAction;
}


void DrawImpulse(const Vec3& impulse, const Vec3& pos)
{
  if (impulse.len2() < 0.0001f)
    return;

  IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
  ColorB colStart(255,0,0,128);
  ColorB colEnd(255,0,0,255);

  pGeom->DrawLine(pos, colStart, pos+impulse, colEnd);
}


//------------------------------------------------------------------------
bool CVehicleMovementHovercraft::RequestMovement(CMovementRequest& movementRequest)
{
  SmartScriptTable stateTable;
  if (!m_pEntity->GetScriptTable()->GetValue("State", stateTable))
  {
    CryError( "Vehicle '%s' is missing 'State' LUA table. Required by AI", m_pEntity->GetName() );
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

  float	angle = 0;
  float	sideTiltAngle = 0;
  float	forwTiltAngle = 0;

  // If the movement vector is nonzero there is a target to drive at.
  if (moveDir.GetLengthSquared() > 0.01f)
  {
    SmartScriptTable movementAbilityTable;
    if (!m_pEntity->GetScriptTable()->GetValue("AIMovementAbility", movementAbilityTable))
    {
      CryError("Vehicle '%s' is missing 'AIMovementAbility' LUA table. Required by AI", m_pEntity->GetName());
      return false;
    }

    float	maxSpeed = 0.0f;
    if (!movementAbilityTable->GetValue("maxSpeed", maxSpeed))
    {
      CryError("Vehicle '%s' is missing 'MovementAbility.maxSpeed' LUA variable. Required by AI", m_pEntity->GetName());
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
    
    Vec3 vel = m_pVehicle->GetStatus().vel;
    float	speed = forwardDir.Dot(vel);

    // If the desired speed is negative, it means that the maximum speed of the vehicle
    // should be used.
    float	desiredSpeed = inputSpeed;
    if (desiredSpeed > maxSpeed)
      desiredSpeed = maxSpeed;

    // Allow breaking if the error is too high.
    float	clampMin = 0.0f;
    if (abs( desiredSpeed - speed ) > desiredSpeed * 0.5f)
      clampMin = -1.0f;

    m_direction = m_dirPID.Update( speed, desiredSpeed, clampMin, 1.0f );

    stateTable->SetValue( "direction", m_direction );
    stateTable->SetValue( "steering", m_steering );
    m_movementAction.power = m_direction;
    m_movementAction.rotateYaw = m_steering;
    m_movementAction.isAI = true;
  }
  else
  {
    // let's break - we don't want to move anywhere
    m_direction = 0;
    m_steering = 0;
    stateTable->SetValue( "braking", 1 );
    stateTable->SetValue( "direction", m_direction );
    stateTable->SetValue( "steering", m_steering );
    m_movementAction.power = m_direction;
    m_movementAction.rotateYaw = m_steering;
    m_movementAction.brake = true;
  }

  return true;
}

void CVehicleMovementHovercraft::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_vecThrusters);
}

//------------------------------------------------------------------------
void CVehicleMovementHovercraft::Serialize(TSerialize ser, unsigned aspects)
{
  CVehicleMovementBase::Serialize(ser, aspects);

  if (ser.GetSerializationTarget() == eST_Network)
  {
    if (aspects & CNetworkMovementHovercraft::CONTROLLED_ASPECT)
      m_netActionSync.Serialize(ser, aspects);
  }
  else 
  {
    ser.Value("m_startComplete", m_startComplete);
    ser.Value("m_hoverTimer", m_hoverTimer);
    ser.Value("m_bEngineAlwaysOn", m_bEngineAlwaysOn);
		ser.Value("contacts", m_contacts);
		ser.Value("thrusterTimer", m_thrusterTimer);
  }
}


//------------------------------------------------------------------------
CNetworkMovementHovercraft::CNetworkMovementHovercraft()
: m_steer(0.0f)
, m_pedal(0.0f)
, m_boost(false)
{
}

//------------------------------------------------------------------------
CNetworkMovementHovercraft::CNetworkMovementHovercraft(CVehicleMovementHovercraft *pMovement)
{
  m_steer = pMovement->m_movementAction.rotateYaw;
  m_pedal = pMovement->m_movementAction.power;  
  m_boost = pMovement->m_boost;
}

//------------------------------------------------------------------------
void CNetworkMovementHovercraft::UpdateObject(CVehicleMovementHovercraft *pMovement)
{
  pMovement->m_movementAction.rotateYaw = m_steer;
  pMovement->m_movementAction.power = m_pedal;  
  pMovement->m_boost = m_boost;
}

//------------------------------------------------------------------------
void CNetworkMovementHovercraft::Serialize(TSerialize ser, unsigned aspects)
{
  if (ser.GetSerializationTarget()==eST_Network && aspects&CONTROLLED_ASPECT)
  {
    ser.Value("pedal", m_pedal, 'vPed');
    ser.Value("steer", m_steer, 'vStr');    
    ser.Value("boost", m_boost, 'bool');
  }
}
