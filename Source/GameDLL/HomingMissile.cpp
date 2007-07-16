/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 08:12:2005   14:14 : Created by MichaelR (port from Marcios HomingMissile.lua)

*************************************************************************/
#include "StdAfx.h"
#include "HomingMissile.h"
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"
#include "WeaponSystem.h"

#define HM_TIME_TO_UPDATE 0.0f

//------------------------------------------------------------------------
CHomingMissile::CHomingMissile()
{
  m_isCruising = false;
  m_isDescending = false;
  m_controlled = false;
	m_autoControlled = false;
	m_cruise = true;
  m_destination.zero();
  m_targetId = 0;
	m_maxTargetDistance = 200.0f;		//Default
	m_turnMod = 0.35f;
	m_turnModCruise = 0.7f;
}

//------------------------------------------------------------------------
CHomingMissile::~CHomingMissile()
{

}

//------------------------------------------------------------------------
bool CHomingMissile::Init(IGameObject *pGameObject)
{
	if (CRocket::Init(pGameObject))
	{
		m_cruiseAltitude = GetParam("cruise_altitude", m_cruiseAltitude);

		m_accel = GetParam("accel", m_accel);
		m_turnSpeed = GetParam("turn_speed", m_turnSpeed);
		m_maxSpeed = GetParam("max_speed", m_maxSpeed);
		m_alignAltitude = GetParam("align_altitude", m_alignAltitude);
		m_descendDistance = GetParam("descend_distance", m_descendDistance);
		m_maxTargetDistance = GetParam("max_target_distance", m_maxTargetDistance);
		m_cruise = GetParam("cruise", m_cruise);
		m_controlled = GetParam("controlled", m_controlled);
		m_autoControlled = GetParam("autoControlled",m_autoControlled);
		m_turnMod = GetParam("turnMod",m_turnMod);
		m_turnModCruise = GetParam("turnModCruise", m_turnModCruise);

		return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CHomingMissile::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
  CRocket::Launch(pos, dir, velocity, speedScale);
}

//------------------------------------------------------------------------
void CHomingMissile::Update(SEntityUpdateContext &ctx, int updateSlot)
{

	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

  CRocket::Update(ctx, updateSlot);
  
  // update destination if required
  if (!m_cruise)
		UpdateControlledMissile(ctx.fFrameTime);
  else 
		UpdateCruiseMissile(ctx.fFrameTime);
}

//-----------------------------------------------------------------------------
void CHomingMissile::UpdateControlledMissile(float frameTime)
{

	bool isServer = gEnv->bServer;

	//IRenderer* pRenderer = gEnv->pRenderer;
	//IRenderAuxGeom* pGeom = pRenderer->GetIRenderAuxGeom();
	//float color[4] = {1,1,1,1};
	//const static float step = 15.f;  
	//float y = 20.f;    

	//bool bDebug = g_pGameCVars->i_debug_projectiles > 0;

	float	turnMod = 0.35f;

	if (isServer)
	{
		SetDestination(Vec3Constants<float>::fVec3_Zero);

		//If there's a target, follow the target
		if(m_targetId)
		{
			turnMod = m_turnModCruise;
			// If we are here, there's a target
			IEntity* pTarget = gEnv->pEntitySystem->GetEntity(m_targetId);
			if (pTarget)
			{
				AABB box;
				pTarget->GetWorldBounds(box);
				Vec3 finalDes = 0.5f*(box.GetCenter()+pTarget->GetWorldPos());
				SetDestination(finalDes);
				//SetDestination( box.GetCenter() );

				//if (bDebug)
					//pRenderer->Draw2dLabel(5.0f, y+=step, 1.5f, color, false, "Target Entity: %s", pTarget->GetName());
			}
		} 
		else if (m_controlled && !m_autoControlled)
		{
			//Check if the weapon is still selected
			CWeapon *pWeapon = GetWeapon();

			if(!pWeapon || !pWeapon->IsSelected())
				return;

			//Follow the crosshair
			CActor *pActor=static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(GetOwnerId()));
			if (pActor && pActor->IsPlayer())
			{
				if (IMovementController *pMC=pActor->GetMovementController())
				{
					turnMod = m_turnMod;
					SMovementState state;
					pMC->GetMovementState(state);

					static const int objTypes = ent_all;    
					static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;                      

					IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
					IPhysicalEntity *pSkip = GetEntity()->GetPhysics();
					ray_hit hit;
					int hits = 0;

					hits = pWorld->RayWorldIntersection(state.eyePosition + 1.5f*state.eyeDirection, state.eyeDirection*m_maxTargetDistance, objTypes, flags, &hit, 1, &pSkip, 1);

					if(hits)
						SetDestination(hit.pt);
					else
						SetDestination(state.eyePosition + m_maxTargetDistance*state.eyeDirection);	//Some point in the sky...


					//if (bDebug)
					//{
						//pRenderer->Draw2dLabel(5.0f, y+=step, 1.5f, color, false, "PlayerView Target: %.0f %.0f %.0f", hit.pt.x, hit.pt.y, hit.pt.z);
						//pRenderer->GetIRenderAuxGeom()->DrawCone(m_destination, Vec3(0,0,-1), 2.5f, 7.f, ColorB(255,0,0,255));
					//}
				}
			}
		}
	}

	//This code is shared by both modes above (auto and controlled)
	if(!m_destination.IsZero())
	{
		pe_status_dynamics status;
		if (!GetEntity()->GetPhysics()->GetStatus(&status))
			return;

		float currentSpeed = status.v.len();
		Vec3 currentVel = status.v;
		Vec3 currentPos = GetEntity()->GetWorldPos();
		Vec3 goalDir(ZERO);

		//Just a security check
		if(currentSpeed<1.0f)
		{
			Explode(true,false);
			return;
		}

		goalDir = m_destination - currentPos;
		goalDir.Normalize();

		//Turn more slowly...
		currentVel.Normalize();

		//if(bDebug)
		//{
			//pRenderer->Draw2dLabel(50,55,2.0f,color,false,"Current Dir: %f, %f, %f",currentVel.x,currentVel.y,currentVel.z);
			//pRenderer->Draw2dLabel(50,80,2.0f,color,false,"Goal    Dir: %f, %f, %f",goalDir.x,goalDir.y,goalDir.z);
		//}					

		float cosine = max(min(currentVel.Dot(goalDir), 0.999f), -0.999f);
		float goalAngle = RAD2DEG(acos_tpl(cosine));
		float maxAngle = m_turnSpeed * frameTime;

		if (goalAngle > maxAngle+0.05f)
			goalAngle = maxAngle;

		goalDir = Vec3::CreateLerp(currentVel,goalDir, (maxAngle/goalAngle)*turnMod);

		//if(bDebug)
			//pRenderer->Draw2dLabel(50,105,2.0f,color,false,"Corrected Dir: %f, %f, %f",goalDir.x,goalDir.y,goalDir.z);

		pe_action_set_velocity action;
		action.v = goalDir * currentSpeed;
		GetEntity()->GetPhysics()->Action(&action);
	}
}

//----------------------------------------------------------------------------
void CHomingMissile::UpdateCruiseMissile(float frameTime)
{

	IRenderer* pRenderer = gEnv->pRenderer;
	IRenderAuxGeom* pGeom = pRenderer->GetIRenderAuxGeom();
	float color[4] = {1,1,1,1};
	const static float step = 15.f;  
	float y = 20.f;    

	bool bDebug = g_pGameCVars->i_debug_projectiles > 0;

	if (m_targetId)
	{
		IEntity* pTarget = gEnv->pEntitySystem->GetEntity(m_targetId);
		if (pTarget)
		{
			AABB box;
			pTarget->GetWorldBounds(box);
			SetDestination( box.GetCenter() );

			//if (bDebug)
				//pRenderer->Draw2dLabel(5.0f, y+=step, 1.5f, color, false, "Target Entity: %s", pTarget->GetName());
		}    
	}
	else 
	{
		// update destination pos from weapon
		static IItemSystem* pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
		IItem* pItem = pItemSystem->GetItem(m_weaponId);
		if (pItem && pItem->GetIWeapon())
		{
			const Vec3& dest = pItem->GetIWeapon()->GetDestination();
			SetDestination( dest );

			//if (bDebug)
				//pRenderer->Draw2dLabel(5.0f, y+=step, 1.5f, color, false, "Weapon Destination: (%.1f %.1f %.1f)", dest.x, dest.y, dest.z);
		}
	}

	pe_status_dynamics status;
	if (!GetEntity()->GetPhysics()->GetStatus(&status))
		return;

	float currentSpeed = status.v.len();
	Vec3 currentPos = GetEntity()->GetWorldPos();
	Vec3 goalDir(ZERO);

	if (!m_destination.IsZero())
	{
		if (bDebug)
			pGeom->DrawCone(m_destination, Vec3(0,0,-1), 2.5f, 7.f, ColorB(255,0,0,255));

		float heightDiff = (m_cruiseAltitude-m_alignAltitude) - currentPos.z;

		if (!m_isCruising && heightDiff * sgn(status.v.z) > 0.f)
		{
			// if heading towards align altitude (but not yet reached) accelerate to max speed    
			if (bDebug)
				pRenderer->Draw2dLabel(5.0f,  y+=step,   1.5f, color, false, "[HomingMissile] accelerating (%.1f / %.1f)", currentSpeed, m_maxSpeed);    
		}
		else if (!m_isCruising && heightDiff * sgnnz(status.v.z) < 0.f && (status.v.z<0 || status.v.z>0.25f))
		{
			// align to cruise
			if (currentSpeed != 0)
			{
				goalDir = status.v;
				goalDir.z = 0;
				goalDir.normalize();
			}    

			if (bDebug)
				pRenderer->Draw2dLabel(5.0f,  y+=step, 1.5f, color, false, "[HomingMissile] aligning"); 
		}
		else
		{
			if (bDebug)
				pRenderer->Draw2dLabel(5.0f,  y+=step, 1.5f, color, false, "[HomingMissile] cruising..."); 

			// cruise
			m_isCruising = true;

			if (!m_destination.IsZero())
			{
				float groundDistSq = m_destination.GetSquaredDistance2D(currentPos);
				float distSq = m_destination.GetSquaredDistance(currentPos);
				float descendDistSq = sqr(m_descendDistance);

				if (m_isDescending || groundDistSq <= descendDistSq)
				{
					if (bDebug)
						pRenderer->Draw2dLabel(5.0f,  y+=step, 1.5f, color, false, "[HomingMissile] descending!"); 

					if (distSq != 0)
						goalDir = (m_destination - currentPos).normalized();
					else 
						goalDir.zero();

					m_isDescending = true;
				}              
				else
				{
					Vec3 airPos = m_destination;
					airPos.z = currentPos.z;          
					goalDir = airPos - currentPos;
					if (goalDir.len2() != 0)
						goalDir.Normalize();
				}    
			}
		}
	}  

	float desiredSpeed = currentSpeed;
	if (currentSpeed < m_maxSpeed-0.1f)
	{
		desiredSpeed = min(m_maxSpeed, desiredSpeed + m_accel*frameTime);
	}

	Vec3 currentDir = status.v.GetNormalizedSafe(FORWARD_DIRECTION);
	Vec3 dir = currentDir;

	if (!goalDir.IsZero())
	{ 
		float cosine = max(min(currentDir.Dot(goalDir), 0.999f), -0.999f);
		float goalAngle = RAD2DEG(acos_tpl(cosine));
		float maxAngle = m_turnSpeed * frameTime;

		if (bDebug)
		{ 
			pGeom->DrawCone( currentPos, goalDir, 0.4f, 12.f, ColorB(255,0,0,255) );
			pRenderer->Draw2dLabel(5.0f,  y+=step, 1.5f, color, false, "[HomingMissile] goalAngle: %.2f", goalAngle); 
		}

		if (goalAngle > maxAngle+0.05f)    
			dir = (Vec3::CreateLerp(currentDir, goalDir, maxAngle/goalAngle)).normalize();
		else //if (goalAngle < 0.005f)
			dir = goalDir;
	}

	pe_action_set_velocity action;
	action.v = dir * desiredSpeed;
	GetEntity()->GetPhysics()->Action(&action);

	if (bDebug)
	{
		pGeom->DrawCone( currentPos, dir, 0.4f, 12.f, ColorB(128,128,0,255) );  
		pRenderer->Draw2dLabel(5.0f,  y+=step, 1.5f, color, false, "[HomingMissile] currentSpeed: %.1f (max: %.1f)", currentSpeed, m_maxSpeed); 
	}
}
//-------------------------------------------------------------------------------
void CHomingMissile::Serialize(TSerialize ser, unsigned int aspects)
{
	CRocket::Serialize(ser, aspects);

	if (aspects & eEA_GameServerDynamic)
	{
		bool gotdestination=!m_destination.IsZero();
		if (ser.BeginOptionalGroup("gotdestination", gotdestination))
		{
			ser.Value("destination", m_destination, 'wrld');
			ser.EndGroup();
		}
	}
}