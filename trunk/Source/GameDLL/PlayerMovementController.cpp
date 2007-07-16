#include "StdAfx.h"
#include "Game.h"
#include "PlayerMovementController.h"
#include "GameUtils.h"
#include "ITimer.h"
#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "GameCVars.h"
#include "NetInputChainDebug.h"
#include "Item.h"

#define ENABLE_NAN_CHECK

#ifdef ENABLE_NAN_CHECK
#define CHECKQNAN_FLT(x) \
	assert(((*(unsigned*)&(x))&0xff000000) != 0xff000000u && (*(unsigned*)&(x) != 0x7fc00000))
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#define CHECKQNAN_VEC(v) \
	CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)

// minimum desired speed accepted
static const float MIN_DESIRED_SPEED = 0.00002f;
// distance from target that is accepted as "there"
static const float TARGET_REACHED_DISTANCE = 0.4f;
// distance to start slowing down before reaching target
static const float TARGET_SLOWDOWN_DISTANCE = 1.0f;

// maximum head turn angle. This is really low, should be nearer 90, but the helmet intersects with the suit for larger values.
static const float MAX_HEAD_TURN_ANGLE = DEG2RAD(45);
// maximum aim angle
static const float MAX_AIM_TURN_ANGLE = DEG2RAD(60);
// anticipation look ik
static const float ANTICIPATION_COSINE_ANGLE = DEG2RAD(45);
// amount of time to aim for
static const CTimeValue AIM_TIME = 0.1f; //2.5f;
// amount of time to look for
static const CTimeValue LOOK_TIME = 2.0f;
// maximum angular velocity for look-at motion (radians/sec)
static const float MAX_LOOK_AT_ANGULAR_VELOCITY = DEG2RAD(75.0f);
// maximum angular velocity for aim-at motion (radians/sec)
static const float MAX_AIM_AT_ANGULAR_VELOCITY = DEG2RAD(75.0f);
static const float MAX_FIRE_TARGET_DELTA_ANGLE = DEG2RAD(10.0f);

// IDLE Checking stuff from here on
#define DEBUG_IDLE_CHECK
#undef  DEBUG_IDLE_CHECK

static const float IDLE_CHECK_TIME_TO_IDLE = 5.0f;
static const float IDLE_CHECK_MOVEMENT_TIMEOUT = 3.0f;
// ~IDLE Checking stuff

CPlayerMovementController::CPlayerMovementController( CPlayer * pPlayer ) : m_pPlayer(pPlayer), m_animTargetSpeed(-1.0f), m_animTargetSpeedCounter(0)
{
	m_lookTarget = m_aimTarget = m_fireTarget = m_pPlayer->GetEntity()->GetWorldPos();
	Reset();
}

void CPlayerMovementController::Reset()
{
	m_state = CMovementRequest();
	m_atTarget = false;
	m_desiredSpeed = 0.0f;
	m_usingAimIK = m_usingLookIK = false;
	m_aimInterpolator.Reset();
	m_lookInterpolator.Reset();
	m_updateFunc = &CPlayerMovementController::UpdateNormal;
	m_targetStance = STANCE_NULL;
	m_idleChecker.Reset(this);

	if(!GetISystem()->IsSerializingFile() == 1)
		UpdateMovementState( m_currentMovementState );

	//
	m_aimTargets.resize(1);
	/*m_aimTargetsCount = m_aimTargetsIterator = m_aimNextTarget = 0;*/
}

bool CPlayerMovementController::RequestMovement( CMovementRequest& request )
{
	if (!m_pPlayer->IsPlayer())
	{
		if (IVehicle* pVehicle = m_pPlayer->GetLinkedVehicle())
		{
			if (IMovementController* pController = pVehicle->GetPassengerMovementController(m_pPlayer->GetEntityId()))
			{
        IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(m_pPlayer->GetEntityId());
        if (!pSeat->IsDriver())
				  pController->RequestMovement(request);
			}
		}
	}
	// because we're not allowed to "commit" values here, we make a backup,
	// perform modifications, and then copy the modifications make if everything
	// was successful
	CMovementRequest state = m_state;
	// we have to process right through and not early out, because otherwise we
	// won't modify request to a "close but correct" state
	bool ok = true;

	bool idleCheck = m_idleChecker.Process(m_currentMovementState, m_state, request);

	if (request.HasMoveTarget())
	{
		// TODO: check validity of getting to that target
		state.SetMoveTarget( request.GetMoveTarget() );
		m_atTarget = false;

		float distanceToEnd(request.GetDistanceToPathEnd());
		if (distanceToEnd>0.001f)
			state.SetDistanceToPathEnd(distanceToEnd);
	}
	else if (request.RemoveMoveTarget())
	{
		state.ClearMoveTarget();
		state.ClearDesiredSpeed();
		state.ClearDistanceToPathEnd();
	}

	if (request.HasStance())
	{
		state.SetStance( request.GetStance() );
	}
	else if (request.RemoveStance())
	{
		state.ClearStance();
	}

	if (request.HasDesiredSpeed())
	{
		if (!state.HasMoveTarget() && request.GetDesiredSpeed() > 0.0f)
		{
			request.SetDesiredSpeed(0.0f);
			ok = false;
		}
		else
		{
			state.SetDesiredSpeed( request.GetDesiredSpeed() );
		}
	}
	else if (request.RemoveDesiredSpeed())
	{
		state.RemoveDesiredSpeed();
	}

	if (request.HasAimTarget())
	{
		state.SetAimTarget( request.GetAimTarget() );
	}
	else if (request.RemoveAimTarget())
	{
		state.ClearAimTarget();
		//state.SetNoAiming();
	}

	if (request.HasBodyTarget())
	{
		state.SetBodyTarget( request.GetBodyTarget() );
	}
	else if (request.RemoveBodyTarget())
	{
		state.ClearBodyTarget();
		//state.SetNoBodying();
	}

	if (request.HasFireTarget())
	{
		state.SetFireTarget( request.GetFireTarget() );
		//forcing fire target here, can not wait for it to be set in the Update.
		//if don't do it - first shot of the weapon might be in some undefined direction (ProsessShooting is done right after this 
		//call in AIProxy). This is particularly problem for RPG shooting
		m_currentMovementState.fireTarget = request.GetFireTarget();
		// the weaponPosition is from last update - might be different at this moment, but should not be too much
		m_currentMovementState.fireDirection = (m_currentMovementState.fireTarget - m_currentMovementState.weaponPosition).GetNormalizedSafe();
	}
	else if (request.RemoveFireTarget())
	{
		state.ClearFireTarget();
		//state.SetNoAiming();
	}

	if (request.HasLookTarget())
	{
		// TODO: check against move direction to validate request
		state.SetLookTarget( request.GetLookTarget(), request.GetLookImportance() );
	}
	else if (request.RemoveLookTarget())
	{
		state.ClearLookTarget();
	}

	if (request.HasStance())
	{
		state.SetStance( request.GetStance() );
	}
	else if (request.RemoveStance())
	{
		state.ClearStance();
	}

	if (request.HasLean())
	{
		state.SetLean( request.GetLean() );
	}
	else if (request.RemoveLean())
	{
		state.ClearLean();
	}

	if (request.ShouldJump())
		state.SetJump();

	state.SetAllowStrafing(request.AllowStrafing());

	if (request.HasNoAiming())
		state.SetNoAiming();

	if (request.HasDeltaMovement())
		state.AddDeltaMovement( request.GetDeltaMovement() );
	if (request.HasDeltaRotation())
		state.AddDeltaRotation( request.GetDeltaRotation() );

	if (request.HasPseudoSpeed())
		state.SetPseudoSpeed(request.GetPseudoSpeed());
	else if (request.RemovePseudoSpeed())
		state.ClearPseudoSpeed();

	if (request.HasPrediction())
		state.SetPrediction( request.GetPrediction() );
	else if (request.RemovePrediction())
		state.ClearPrediction();

	state.SetAlertness(request.GetAlertness());

	// commit modifications
	if (ok)
	{
		m_state = state;
	}

//	if (ICharacterInstance * pChar = m_pPlayer->GetEntity()->GetCharacter(0))
//		pChar->GetISkeleton()->SetFuturePathAnalyser( (m_state.HasPathTarget() || m_state.HasMoveTarget())? 1 : 0 );

/*
	if (m_state.HasPathTarget())
	{
		SAnimationTargetRequest req;
		if (m_state.HasDesiredBodyDirectionAtTarget())
		{
			req.direction = m_state.GetDesiredBodyDirectionAtTarget();
			req.directionRadius = DEG2RAD(1.0f);
		}
		else
		{
			req.direction = FORWARD_DIRECTION;
			req.directionRadius = DEG2RAD(180.0f);
		}
		req.position = m_state.GetPathTarget();

		IAnimationSpacialTrigger * pTrigger = m_pPlayer->GetAnimationGraphState()->SetTrigger( req );
		if (m_state.HasDesiredSpeedAtTarget())
			pTrigger->SetInput("DesiredSpeed", m_state.GetDesiredSpeedAtTarget());
		else
			pTrigger->SetInput("DesiredSpeed", 0.0f); // TODO: hack to test
	}
*/

	if (request.HasActorTarget())
	{
		const SActorTargetParams& p = request.GetActorTarget();

		assert(p.locationRadius > 0.0f);
		assert(p.directionRadius > 0.0f);

		SAnimationTargetRequest req;
		req.position = p.location;
		req.positionRadius = std::max( p.locationRadius, DEG2RAD(0.05f) );
		static float minRadius = 0.05f;
		req.startRadius = std::max(minRadius, 2.0f*p.locationRadius);
		if (p.startRadius > minRadius)
			req.startRadius = p.startRadius;
		req.direction = p.direction;
		req.directionRadius = std::max( p.directionRadius, DEG2RAD(0.05f) );
		req.prepareRadius = 3.0f;


		IAnimationSpacialTrigger * pTrigger = m_pPlayer->GetAnimationGraphState()->SetTrigger(req, p.triggerUser, p.pQueryStart, p.pQueryEnd);
		if (pTrigger)
		{
			if (!p.vehicleName.empty())
			{
				pTrigger->SetInput( "Vehicle", p.vehicleName.c_str() );
				pTrigger->SetInput( "VehicleSeat", p.vehicleSeat );
			}
			if (p.speed >= 0.0f)
			{
				pTrigger->SetInput( m_inputDesiredSpeed, p.speed );
			}
			m_animTargetSpeed = p.speed;
			pTrigger->SetInput( m_inputDesiredTurnAngleZ, 0 );
			if (!p.animation.empty())
			{
				pTrigger->SetInput( p.signalAnimation? "Signal" : "Action", p.animation.c_str() );
			}
			if (p.stance != STANCE_NULL)
			{
				m_targetStance = p.stance;
				pTrigger->SetInput( m_inputStance, m_pPlayer->GetStanceInfo(p.stance)->name );
			}
		}
	}
	else if (request.RemoveActorTarget())
	{
		if(m_pPlayer->GetAnimationGraphState())
			m_pPlayer->GetAnimationGraphState()->ClearTrigger(eAGTU_AI);
	}

	return ok;
}

ILINE static f32 GetYaw( const Vec3& v0, const Vec3& v1 )
{
  float a0 = atan2f(v0.y, v0.x);
  float a1 = atan2f(v1.y, v1.x);
  float a = a1 - a0;
  if (a > gf_PI) a -= gf_PI2;
  else if (a < -gf_PI) a += gf_PI2;
  return a;
}

ILINE static Quat GetTargetRotation( Vec3 oldTarget, Vec3 newTarget, Vec3 origin )
{
	Vec3 oldDir = (oldTarget - origin).GetNormalizedSafe(FORWARD_DIRECTION);
	Vec3 newDir = (newTarget - origin).GetNormalizedSafe(ZERO);
	if (newDir.GetLength() < 0.001f)
		return Quat::CreateIdentity();
	else
		return Quat::CreateRotationV0V1( oldDir, newDir );
}

static void DrawArrow( Vec3 from, Vec3 to, float length, float * clr )
{
	float r = clr[0];
	float g = clr[1];
	float b = clr[2];
	float color[] = {r,g,b,1};
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( from, ColorF(r,g,b,1), to, ColorF(r,g,b,1) );
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( to, 0.05f, ColorF(r,g,b,1) );
}

static float bias( float b, float t )
{
	if (b == 0.0f)
		return 0.0f;
	return cry_powf( b, cry_logf(b)/cry_logf(0.5f) );
}

static float gain( float g, float t )
{
	if (t < 0.5f)
		return bias( 1.0f-g, 2.0f*t );
	else
		return 1.0f - bias( 1.0f-g, 2.0f-2.0f*t );
}

static float blue[4] = {0,0,1,1};
static float red[4] = {1,0,0,1};
static float yellow[4] = {1,1,0,1};
static float green[4] = {0,1,0,1};
static CTimeValue lastTime;
static int y = 100;

void CPlayerMovementController::BindInputs( IAnimationGraphState * pAGState )
{
	if ( pAGState )
	{
		m_inputDesiredSpeed = pAGState->GetInputId("DesiredSpeed");
		m_inputDesiredTurnAngleZ = pAGState->GetInputId("DesiredTurnAngleZ");
		m_inputStance = pAGState->GetInputId("Stance");
		m_inputPseudoSpeed = pAGState->GetInputId("PseudoSpeed");
	}
}

bool CPlayerMovementController::Update( float frameTime, SActorFrameMovementParams& params )
{
	bool ok = false;

	params.lookTarget = Vec3(-1,-1,-1);
	params.aimTarget = Vec3(-1,-1,-1);

/*
	if (m_pPlayer->IsFrozen())
	{
		params.lookTarget = Vec3(-2,-2,-2);
		params.aimTarget = Vec3(-2,-2,-2);

		if (m_state.HasAimTarget())
		{
			Vec3 aimTarget = m_state.GetAimTarget();
			if (!aimTarget.IsValid())
				CryError("");
			m_state.ClearAimTarget();
		}

		if (m_state.HasLookTarget())
		{
			Vec3 lookTarget = m_state.GetLookTarget();
			if (!lookTarget.IsValid())
				CryError("");
			m_state.ClearLookTarget();
		}
	}
*/

	if (m_updateFunc)
	{
		ok = (this->*m_updateFunc)(frameTime, params);
	}


//	m_state.RemoveDeltaMovement();
//	m_state.RemoveDeltaRotation();

	return ok;
}

void CPlayerMovementController::CTargetInterpolator::TargetValue(
	const Vec3& value, 
	CTimeValue now, 
	CTimeValue timeout, 
	float frameTime, 
	bool updateLastValue, 
	const Vec3& playerPos, 
	float maxAngularVelocity )
{
	m_target = value;
	if (updateLastValue)
		m_lastValue = now;
}

static Vec3 ClampDirectionToCone( const Vec3& dir, const Vec3& forward, float maxAngle )
{
	float angle = cry_acosf( dir.Dot(forward) );
	if (angle > maxAngle)
	{
		Quat fromForwardToDir = Quat::CreateRotationV0V1(forward, dir);
		Quat rotate = Quat::CreateSlerp( Quat::CreateIdentity(), fromForwardToDir, min(maxAngle/angle, 1.0f) );
		return rotate * forward;
	}
	return dir;
}

static Vec3 FlattenVector( Vec3 x )
{
	x.z = 0;
	return x;
}

void CPlayerMovementController::CTargetInterpolator::Update(float frameTime)
{
	m_pain += frameTime * (m_painDelta - DEG2RAD(10.0f));
	m_pain = CLAMP(m_pain, 0, 2);
}

bool CPlayerMovementController::CTargetInterpolator::GetTarget(
	Vec3& target, 
	Vec3& rotateTarget, 
	const Vec3& playerPos, 
	const Vec3& moveDirection, 
	const Vec3& bodyDirection,
	const Vec3& entityDirection,
	float maxAngle, 
	float distToEnd, 
	float viewFollowMovement, 
	ColorB * clr,
	const char ** bodyTargetType )
{
  bool retVal = true;
	Vec3 desiredTarget = GetTarget();
	Vec3 desiredDirection = desiredTarget - playerPos;
	float	targetDist = desiredDirection.GetLength();

	// if the target gets very close, then move it to the current body direction (failsafe)
	float desDirLenSq = desiredDirection.GetLengthSquared2D();
	if (desDirLenSq < sqr(0.2f))
	{
		float blend = min(1.0f, 5.0f * (0.2f - sqrtf(desDirLenSq)));
		desiredDirection = LERP(desiredDirection, entityDirection, blend);
	}
	desiredDirection.NormalizeSafe(entityDirection);

	Vec3 clampedDirection = desiredDirection;

	// clamp yaw rotation into acceptable bounds
	float yawFromBodyToDesired = GetYaw( bodyDirection, desiredDirection );
	if (fabsf(yawFromBodyToDesired) > maxAngle)
	{
		float clampAngle = (yawFromBodyToDesired < 0) ? -maxAngle : maxAngle;
		clampedDirection = Quat::CreateRotationZ(clampAngle) * bodyDirection;
		clampedDirection.z = desiredDirection.z;
		clampedDirection.Normalize();

		if (viewFollowMovement > 0.1f)
		{
			clampedDirection = Quat::CreateSlerp( 
			Quat::CreateIdentity(), 
			Quat::CreateRotationV0V1(clampedDirection, moveDirection), 
			min(viewFollowMovement, 1.0f)) * clampedDirection;
      // return false to indicate that the direction is out of range. This stops the gun
      // being held up in the aiming pose, but not pointing at the target, whilst running
      retVal = false;
		}
		// Clamped, rotate the body until it is facing towards the target.
	}

	if(viewFollowMovement > 0.5f)
	{
		// Rotate the body towards the movement direction while moving and strafing is not specified.
		rotateTarget = playerPos + 5.0f * moveDirection.GetNormalizedSafe();
		*bodyTargetType = "ik-movedir";
	}
	else
	{
		rotateTarget = desiredTarget;
		*bodyTargetType = "ik-target";
	}

	// Return the safe aim/look direction.
	target = clampedDirection * targetDist + playerPos;

	return retVal;
}

bool CPlayerMovementController::UpdateNormal( float frameTime, SActorFrameMovementParams& params )
{
	// TODO: Don't update all this crap when a character is dead.

	ITimer * pTimer = gEnv->pTimer;

	IRenderer * pRend = gEnv->pRenderer;
	if (g_pGame->GetCVars()->g_debugaimlook)
	{
		if (pTimer->GetFrameStartTime()!=lastTime)
		{
			y = 100;
			lastTime = pTimer->GetFrameStartTime();
		}
	}

	IEntity * pEntity = m_pPlayer->GetEntity();
	Vec3 playerPos = pEntity->GetWorldPos();

	params.desiredVelocity = Vec3(0,0,0);
	params.lookTarget = m_currentMovementState.eyePosition + m_currentMovementState.eyeDirection * 10.0f;
//	params.aimTarget = m_currentMovementState.handPosition + m_currentMovementState.aimDirection * 10.0f;
	if(m_state.HasAimTarget())
		params.aimTarget = m_state.GetAimTarget();
	else
		params.aimTarget = m_currentMovementState.weaponPosition + m_currentMovementState.aimDirection * 10.0f;
	params.lookIK = false;
	params.aimIK = false;
	params.deltaAngles = Ang3(0,0,0);

	bool allowStrafing = m_state.AllowStrafing();

	if(gEnv->bMultiplayer && m_state.HasStance() && m_state.GetStance() == STANCE_PRONE)
	{
		// send a ray backwards to check we're not intersecting a wall. If we are, go to crouch instead.
		if (gEnv->bClient && m_pPlayer->GetGameObject()->IsProbablyVisible())
		{
			Vec3 dir = -m_currentMovementState.bodyDirection;
			dir.NormalizeSafe();
			dir.z = 0.0f;

			Vec3 pos = pEntity->GetWorldPos();
			pos.z += 0.3f;	// move position up a bit so it doesn't intersect the ground.

			//pRend->GetIRenderAuxGeom()->DrawLine( pos, ColorF(1,1,1,0.5f), pos + dir, ColorF(1,1,1,0.5f) );

			ray_hit hit;
			static const int obj_types = ent_static|ent_rigid|ent_sleeping_rigid|ent_living;
			static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
			bool rayHitAny = 0 != gEnv->pPhysicalWorld->RayWorldIntersection( pos, dir, obj_types, flags, &hit, 1, pEntity->GetPhysics() );
			if (rayHitAny)
			{
				m_state.SetStance(STANCE_CROUCH);
			}				

			// also don't allow prone for hurricane/LAW type weapons.
			if(g_pGameCVars->g_proneNotUsableWeapon_FixType == 2)
			{
				if(CItem* pItem = (CItem*)(m_pPlayer->GetCurrentItem()))
				{
					if(pItem->GetParams().prone_not_usable)
						m_state.SetStance(STANCE_CROUCH);
				}
			}
		}
	}

	if (m_state.AlertnessChanged())
	{
		if (m_state.GetAlertness() == 2)
		{
			if (!m_state.HasStance() || m_state.GetStance() == STANCE_RELAXED)
				m_state.SetStance(STANCE_STAND);
		}
	}

	ICharacterInstance * pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return true;

	ISkeleton * pSkeleton = pCharacter->GetISkeleton();

	CTimeValue now = pTimer->GetFrameStartTime();

	Vec3 currentBodyDirection = pEntity->GetRotation().GetColumn1();
	Vec3 moveDirection = currentBodyDirection;
	if (m_pPlayer->GetLastRequestedVelocity().len2() > 0.01f)
		moveDirection = m_pPlayer->GetLastRequestedVelocity().GetNormalized();

	// potentially override our normal targets due to a targetted animation request
	bool hasMoveTarget = false;
	bool hasBodyTarget = false;
	Vec3 moveTarget;
	Vec3 bodyTarget;

	float viewFollowMovement = 0.0f;
	float additionalViewFollowMovement = 0.0f; // for things that can't be multiplied out under any circumstances

	const char * bodyTargetType = "none";
	const char * moveTargetType = "none";
	if (m_state.HasMoveTarget())
	{
		hasMoveTarget = true;
		moveTarget = m_state.GetMoveTarget();
		moveTargetType = "steering";
	}
	const SAnimationTarget * pAnimTarget = m_pPlayer->GetAnimationGraphState()->GetAnimationTarget();
	if ((pAnimTarget != NULL) && pAnimTarget->preparing)
	{
		bodyTarget = pAnimTarget->position + 3.0f * (pAnimTarget->orientation * FORWARD_DIRECTION) + Vec3(0, 0, 1.5);
		bodyTargetType = "animation";
		hasBodyTarget = true;
		allowStrafing = true;
		//additionalViewFollowMovement = 1.0f;

/*
		if ((pAnimTarget->position.GetDistance(playerPos) < 1.0f) || 
			!m_state.HasDesiredSpeed() || 
			(m_state.HasDesiredSpeed() && (m_state.GetDesiredSpeed() < 0.3f)))
*/
		{
			moveTarget = pAnimTarget->position;
			moveTargetType = "animation";
			hasMoveTarget = true;
		}
	}
	if (pAnimTarget != NULL)
	{
    static float criticalDistance = 5.0f;
    float distance = (pAnimTarget->position - playerPos).GetLength();
		// HACK: DistanceToPathEnd is bogus values. Using true distance to anim target pos instead.
		pAnimTarget->allowActivation = m_state.GetDistanceToPathEnd() < criticalDistance && distance < criticalDistance;
	}

	m_animTargetSpeedCounter -= m_animTargetSpeedCounter!=0;

 	// speed control
	if (hasMoveTarget)
	{
		Vec3 desiredMovement = moveTarget - playerPos;
		if (!m_pPlayer->InZeroG())
		{
			// should do the rotation thing?
			desiredMovement.z = 0;
		}
		float distance = desiredMovement.len();

		if (distance > 0.01f) // need to have somewhere to move, to actually move :)
		{
			// speed at which things are "guaranteed" to stop... 0.03f is a fudge factor to ensure this
			static const float STOP_SPEED = MIN_DESIRED_SPEED-0.03f;

			desiredMovement /= distance;
			float desiredSpeed = 1.0f;
			if (m_state.HasDesiredSpeed())
				desiredSpeed = m_state.GetDesiredSpeed();

			if (pAnimTarget && pAnimTarget->preparing)  
			{
				desiredSpeed = std::max(desiredSpeed, m_desiredSpeed);
				desiredSpeed = std::max(1.0f, desiredSpeed);
				desiredSpeed = std::min(desiredSpeed, distance / frameTime);
			}
			if (pAnimTarget && (pAnimTarget->activated || m_animTargetSpeedCounter) && m_animTargetSpeed >= 0.0f)
			{
				desiredSpeed = m_animTargetSpeed;
        if (pAnimTarget->activated)
  				m_animTargetSpeedCounter = 4;
			}

			float stanceSpeed=m_pPlayer->GetStanceMaxSpeed(m_pPlayer->GetStance());
			NETINPUT_TRACE(m_pPlayer->GetEntityId(), desiredSpeed);
			NETINPUT_TRACE(m_pPlayer->GetEntityId(), stanceSpeed);
			NETINPUT_TRACE(m_pPlayer->GetEntityId(), desiredMovement);
			if ((desiredSpeed > MIN_DESIRED_SPEED) && stanceSpeed>0.001f)
			{
				//pRend->GetIRenderAuxGeom()->DrawLine( playerPos, ColorF(1,1,1,1), playerPos + desiredMovement, ColorF(1,1,1,1) );

				//calculate the desired speed amount (0-1 length) in world space
				params.desiredVelocity = desiredMovement * desiredSpeed / stanceSpeed;
				viewFollowMovement = 5.0f * desiredSpeed / stanceSpeed;
				//
				//and now, after used it for the viewFollowMovement convert it to the Actor local space
				moveDirection = params.desiredVelocity.GetNormalizedSafe(ZERO);
				params.desiredVelocity = m_pPlayer->GetBaseQuat().GetInverted() * params.desiredVelocity;
			}
		}
	}

	if (m_state.HasAimTarget())
	{
		float aimLength = (m_currentMovementState.weaponPosition - m_state.GetAimTarget()).GetLengthSquared();
		float aimLimit = 9.0f * 9.0f;
		if (aimLength < aimLimit)
		{
			float mult = aimLength / aimLimit;
			viewFollowMovement *= mult * mult;
		}
	}
	float distanceToEnd(m_state.GetDistanceToPathEnd()); // TODO: Warning, a comment above say this function returns incorrect values.
	if (distanceToEnd>0.001f)
	{
		float lookOnPathLimit(7.0f);
		if (distanceToEnd<lookOnPathLimit)
		{
			float mult(distanceToEnd/lookOnPathLimit);
			viewFollowMovement *= mult * mult;
			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( playerPos, 0.5f, ColorF(1,1,1,1) );
		}
		//pRend->Draw2dLabel( 100, 100, 1.0f, yellow, false, "distance:%.1f", distanceToEnd);
	}
	if (allowStrafing)
		viewFollowMovement = 0.0f;
	viewFollowMovement = CLAMP(viewFollowMovement + additionalViewFollowMovement, 0, 1);

	if (!hasBodyTarget)
	{
		if (m_state.HasBodyTarget())
		{
			bodyTarget = m_state.GetBodyTarget();
			bodyTargetType = "requested";
		}
		else
		{
			bodyTarget = playerPos + currentBodyDirection;
			bodyTargetType = "current";
		}
	}

	// look and aim direction
	bool upd;
	Vec3 tgt;

	Vec3 eyePosition(playerPos.x, playerPos.y, m_currentMovementState.eyePosition.z);

	upd = false;
	tgt = eyePosition + 1.0f * currentBodyDirection;
	const char * lookType = "current";
	if (pAnimTarget && pAnimTarget->preparing)
	{
		upd = false;
		tgt = pAnimTarget->position + 3.0f * (pAnimTarget->orientation * FORWARD_DIRECTION) + Vec3(0, 0, 1.5);
		lookType = "exactpositioning";
	}
	else if (pAnimTarget && pAnimTarget->activated)
	{
		upd = false;
		tgt = eyePosition + 1.0f * currentBodyDirection;
		lookType = "animation";
	}
	else if (m_state.HasAimTarget())
	{
		upd = true;
		tgt = m_state.GetAimTarget();
		lookType = "aim";

/*		if (m_state.HasFireTarget() && m_state.GetFireTarget().Dot(m_state.GetAimTarget()) < cry_cosf(MAX_FIRE_TARGET_DELTA_ANGLE))
		{
			tgt = m_state.GetFireTarget();
			lookType = "fire";
		}*/
	}
	else if (m_state.HasLookTarget())
	{
		upd = true;
		tgt = m_state.GetLookTarget();
		lookType = "look";
	}
	else if (hasMoveTarget)
	{
		Vec3 desiredMoveDir = (moveTarget - playerPos).GetNormalizedSafe(ZERO);
		if (moveDirection.Dot(desiredMoveDir) < ANTICIPATION_COSINE_ANGLE)
		{
			upd = true;
			tgt = moveTarget;
			lookType = "anticipate";
		}
	}
	else if (m_lookInterpolator.HasTarget(now, LOOK_TIME))
	{
		tgt = m_lookInterpolator.GetTarget();
		lookType = "lastLook";
	}
//	AdjustForMovement( tgt, moveTarget, playerPos, viewFollowMovement );

	// Not all of these arguments are used (stupid function).
	m_lookInterpolator.TargetValue( tgt, now, LOOK_TIME, frameTime, upd, playerPos, MAX_LOOK_AT_ANGULAR_VELOCITY );

	upd = false;
	const char * aimType = lookType;
	if (pAnimTarget && pAnimTarget->preparing)
	{
		upd = true;
		tgt = pAnimTarget->position + 3.0f * (pAnimTarget->orientation * FORWARD_DIRECTION) + Vec3(0, 0, 1.5);
		aimType = "animation";
	}
	else if (m_state.HasAimTarget())
	{
		upd = true;
		tgt = m_state.GetAimTarget();
		aimType = "aim";
//		AdjustForMovement( tgt, moveTarget, playerPos, viewFollowMovement );
	}

	//if ((pAnimTarget == NULL) || (!pAnimTarget->activated)) // (Dejan was not sure about this yet, for leaning and such stuff.)
		m_aimInterpolator.TargetValue( tgt, now, AIM_TIME, frameTime, upd, playerPos, MAX_AIM_AT_ANGULAR_VELOCITY );

	const char * ikType = "none";
	ColorB dbgClr(0,255,0,255);
	ColorB * pDbgClr = g_pGame->GetCVars()->g_debugaimlook? &dbgClr : NULL;
	//if (!m_state.HasNoAiming() && m_aimInterpolator.HasTarget( now, AIM_TIME ))	// AIM IK

	bool swimming = (m_pPlayer->GetStance() == STANCE_SWIM);

	bool hasControl = !pAnimTarget || !pAnimTarget->activated && !pAnimTarget->preparing;
	if (!m_pPlayer->IsClient() && hasControl)
	{
		//changed by ivo: most likely this doesn't work any more
	//	Vec3 animBodyDirection = pEntity->GetRotation() * pSkeleton->GetCurrentBodyDirection();
		Vec3 animBodyDirection = pEntity->GetRotation() * Vec3(0,1,0);


		Vec3 entDirection = pEntity->GetRotation().GetColumn1();
		if (!m_state.HasNoAiming() && m_aimInterpolator.HasTarget( now, AIM_TIME ) && (additionalViewFollowMovement < 0.01f) && !swimming)
		{
			params.aimIK = m_aimInterpolator.GetTarget( 
				params.aimTarget, 
				bodyTarget, 
				Vec3( playerPos.x, playerPos.y, m_currentMovementState.weaponPosition.z ),
				moveDirection, 
				animBodyDirection, 
				entDirection,
				MAX_AIM_TURN_ANGLE, 
				m_state.GetDistanceToPathEnd(), 
				viewFollowMovement, 
				pDbgClr,
				&bodyTargetType );
			ikType = "aim";
		}
		else if (m_lookInterpolator.HasTarget( now, LOOK_TIME ) && (additionalViewFollowMovement < 0.01f))	// Look IK
		{
			params.lookIK = m_lookInterpolator.GetTarget( 
				params.lookTarget, 
				bodyTarget, 
				Vec3( playerPos.x, playerPos.y, m_currentMovementState.eyePosition.z ),
				moveDirection, 
				animBodyDirection, 
				entDirection,
				MAX_HEAD_TURN_ANGLE, 
				m_state.GetDistanceToPathEnd(), 
				viewFollowMovement, 
				pDbgClr,
				&bodyTargetType );
			ikType = "look";
		}

		if (((SPlayerStats*)m_pPlayer->GetActorStats())->mountedWeaponID && !m_pPlayer->IsClient())
		{
			params.lookIK = true;
			params.lookTarget = m_aimInterpolator.GetTarget();
			params.aimIK = false;
			params.aimTarget = m_aimInterpolator.GetTarget();
			bodyTarget = m_aimInterpolator.GetTarget();

			lookType = aimType = bodyTargetType = "mountedweapon";
		}
	}

	Vec3 viewDir = ((bodyTarget - playerPos).GetNormalizedSafe(ZERO));
	if (!m_pPlayer->IsClient() && viewDir.len2() > 0.01f)
	{
		//Vec3 localVDir(m_pPlayer->GetEntity()->GetWorldRotation().GetInverted() * viewDir);
		Vec3 localVDir(m_pPlayer->GetViewQuat().GetInverted() * viewDir);
		
		CHECKQNAN_VEC(localVDir);

		if ((pAnimTarget == NULL) || (!pAnimTarget->activated))
		{
			params.deltaAngles.x += asin(CLAMP(localVDir.z,-1,1));
			params.deltaAngles.z += cry_atan2f(-localVDir.x,localVDir.y);
		}

		CHECKQNAN_VEC(params.deltaAngles);

		static float maxDeltaAngleRateNormal = DEG2RAD(180.0f);
		static float maxDeltaAngleRateAnimTarget = DEG2RAD(360.0f);
		static float maxDeltaAngleMultiplayer = DEG2RAD(3600.0f);

		float maxDeltaAngleRate = maxDeltaAngleRateNormal;

		if (gEnv->bMultiplayer)
		{
			maxDeltaAngleRate = maxDeltaAngleMultiplayer;
		}
		else
		{
			// Less clamping when approaching animation target.
			if (pAnimTarget && pAnimTarget->preparing)
			{
				const float	u = CLAMP(1.0 - pAnimTarget->position.GetDistance(playerPos) / 2.5f, 0.0f, 1.0f);
				maxDeltaAngleRate = maxDeltaAngleRate + u * (maxDeltaAngleRateAnimTarget - maxDeltaAngleRate);
			}
		}

		for (int i=0; i<3; i++)
			Limit(params.deltaAngles[i], -maxDeltaAngleRate * frameTime, maxDeltaAngleRate * frameTime);

		CHECKQNAN_VEC(params.deltaAngles);
	}

	if (m_state.HasDeltaRotation())
	{
		params.deltaAngles += m_state.GetDeltaRotation();
		CHECKQNAN_VEC(params.deltaAngles);
		ikType = "mouse";
	}


	if (g_pGame->GetCVars()->g_debugaimlook)
	{
		pRend->Draw2dLabel( 10, y, 1.0f, green, false, "%s: vfm:%f body:%s look:%s aim:%s ik:%s move:%s (%f,%f,%f)", pEntity->GetName(), viewFollowMovement, bodyTargetType, aimType, lookType, ikType, moveTargetType, params.deltaAngles.x, params.deltaAngles.y, params.deltaAngles.z );
		y += 10;
		if (m_state.GetDistanceToPathEnd() >= 0.0f)
		{
			pRend->Draw2dLabel( 20, y, 1.0f, yellow, false, "distanceToEnd: %f (%f)", m_state.GetDistanceToPathEnd(), moveTarget.GetDistance(playerPos) );
			y += 10;
		}

		if (m_state.HasAimTarget())
		{
			pRend->GetIRenderAuxGeom()->DrawLine( m_currentMovementState.weaponPosition, ColorF(1,1,0,1), params.aimTarget+Vec3(0,0,0.05f), ColorF(0.3f,1,0,1) );
			pRend->GetIRenderAuxGeom()->DrawLine( m_currentMovementState.weaponPosition, ColorF(1,1,0,1), m_state.GetAimTarget(), ColorF(1,0.3f,0,1) );
		}
	}

	// process incremental movement
	if (m_state.HasDeltaMovement())
	{
		params.desiredVelocity += m_state.GetDeltaMovement();
	}

	// stance control
	if (pAnimTarget && pAnimTarget->activated && m_targetStance != STANCE_NULL)
	{
		m_state.SetStance( m_targetStance );
	}

	if (m_state.HasStance())
	{
		params.stance = m_state.GetStance();
	}

	// leaning
	if (m_state.HasLean())
		params.desiredLean = m_state.GetLean();
	else
		params.desiredLean = 0.0f;

	params.jump = m_state.ShouldJump();
	m_state.ClearJump();

	// TODO: This should probably be calculate BEFORE it is used (above), or the previous frame's value is used.
	m_desiredSpeed = params.desiredVelocity.GetLength() * m_pPlayer->GetStanceMaxSpeed( m_pPlayer->GetStance() );

	m_state.RemoveDeltaRotation();
	m_state.RemoveDeltaMovement();

  if (params.aimIK)
  {
    m_aimTarget = params.aimTarget;
    // if aiming force looking as well
    // In spite of what it may look like with eye/look IK, the look/aim target tends
    // to be right on/in the target's head so doesn't need any extra offset
    params.lookTarget = params.aimTarget;
    params.lookIK = true;
    m_lookTarget = params.lookTarget;
  }
  else
  {
    m_aimTarget = m_lookTarget;
    if (params.lookIK)
		  m_lookTarget = params.lookTarget;
	  else
		  m_lookTarget = m_currentMovementState.eyePosition + m_pPlayer->GetEntity()->GetRotation() * FORWARD_DIRECTION * 10.0f;
  }

  m_usingAimIK = params.aimIK;
  m_usingLookIK = params.lookIK;

	if (m_state.HasFireTarget())
	{
		m_fireTarget = m_state.GetFireTarget();
	}

	m_aimInterpolator.Update(frameTime);
	m_lookInterpolator.Update(frameTime);

	if (pAnimTarget && pAnimTarget->preparing)
	{
		if (m_state.HasPseudoSpeed())
			m_pPlayer->GetAnimationGraphState()->SetInput(m_inputPseudoSpeed, std::max(0.4f, m_state.GetPseudoSpeed()));
		else
			m_pPlayer->GetAnimationGraphState()->SetInput(m_inputPseudoSpeed, 0.4f);
	}
	else
	{
		if (m_state.HasPseudoSpeed())
			m_pPlayer->GetAnimationGraphState()->SetInput(m_inputPseudoSpeed, m_state.GetPseudoSpeed());
		else
			m_pPlayer->GetAnimationGraphState()->SetInput(m_inputPseudoSpeed, 0.0f);
	}

	static float PredictionDeltaTime = 0.4f;
	bool hasPrediction = m_state.HasPrediction() && (m_state.GetPrediction().nStates > 0);
	bool hasAnimTarget = (pAnimTarget != NULL) && (pAnimTarget->activated || pAnimTarget->preparing);
	if (hasPrediction && !hasAnimTarget)
	{
		params.prediction = m_state.GetPrediction();
	}
	else
	{
    params.prediction.nStates = 0;
	}

	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.desiredVelocity);
	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.desiredLean);
	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.deltaAngles);
	NETINPUT_TRACE(m_pPlayer->GetEntityId(), params.sprint);

	return true;
}

void CPlayerMovementController::UpdateMovementState( SMovementState& state )
{
	IEntity * pEntity = m_pPlayer->GetEntity();
	ICharacterInstance * pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;
	ISkeleton * pSkeleton = pCharacter->GetISkeleton();
	//FIXME:
	if (!pSkeleton)
		return;

	int boneEyeL = m_pPlayer->GetBoneID(BONE_EYE_L);
	int boneEyeR = m_pPlayer->GetBoneID(BONE_EYE_R);
	int boneHead = m_pPlayer->GetBoneID(BONE_HEAD);
	int boneWeapon = m_pPlayer->GetBoneID(BONE_WEAPON);

	bool isCharacterVisible = pCharacter->IsCharacterVisible() != 0;

	if (m_pPlayer->IsPlayer())
	{
		// PLAYER CHARACTER

		Vec3 viewOfs(m_pPlayer->GetStanceViewOffset(m_pPlayer->GetStance()));
		state.eyePosition = pEntity->GetWorldPos() + m_pPlayer->GetBaseQuat() * viewOfs;

		// E3HAX E3HAX E3HAX
		if (((SPlayerStats *)m_pPlayer->GetActorStats())->mountedWeaponID)
			state.eyePosition = GetISystem()->GetViewCamera().GetPosition();

		state.eyeDirection = m_pPlayer->GetViewQuatFinal().GetColumn1();
		if (!m_pPlayer->IsClient()) // marcio: fixes the eye direction for remote players
			state.eyeDirection = (m_lookTarget-state.eyePosition).GetNormalizedSafe(state.eyeDirection);

		state.animationEyeDirection = state.eyeDirection;
		state.weaponPosition = state.eyePosition;
		state.fireDirection = state.aimDirection = state.eyeDirection;
		state.fireTarget = m_fireTarget;
	}
//	else if (!pCharacter->IsCharacterVisible())
	else		// for AI - get all the positions/dimensions from stance/player, not the animation
	{
		// AI/REMOTE CLIENT CHARACTER IS INVISIBLE: ONLY ROOT BONE UPDATED; NEED TO FAKE ANY VALUES IF THEY'RE NEEDED

		Quat	orientation = m_pPlayer->GetBaseQuat(); //pEntity->GetWorldRotation();
		Vec3	forward = orientation * Vec3Constants<float>::fVec3_OneY;
		Vec3	entityPos = pEntity->GetWorldPos();
		Vec3	constrainedLookDir = m_lookTarget - entityPos;
		Vec3	constrainedAimDir = m_aimTarget - entityPos;
		constrainedLookDir.z = 0.0f;
		constrainedAimDir.z = 0.0f;

		constrainedAimDir = constrainedAimDir.GetNormalizedSafe(constrainedLookDir.GetNormalizedSafe(forward));
		constrainedLookDir = constrainedLookDir.GetNormalizedSafe(forward);

		const SStanceInfo*	stanceInfo = m_pPlayer->GetStanceInfo(m_pPlayer->GetStance());

		Matrix33	lookRot;
		lookRot.SetRotationVDir(constrainedLookDir);
		state.eyePosition = entityPos + lookRot.TransformVector(m_pPlayer->GetEyeOffset());

		Matrix33	aimRot;
		aimRot.SetRotationVDir(constrainedAimDir);
		state.weaponPosition = entityPos + aimRot.TransformVector(m_pPlayer->GetWeaponOffset());

		state.upDirection = m_pPlayer->GetBaseQuat().GetColumn2();

		state.eyeDirection = (m_lookTarget - state.eyePosition).GetNormalizedSafe(forward); //(lEyePos - posHead).GetNormalizedSafe(FORWARD_DIRECTION);
		state.animationEyeDirection = state.eyeDirection;
		state.aimDirection = (m_aimTarget - state.weaponPosition).GetNormalizedSafe((m_lookTarget - state.weaponPosition).GetNormalizedSafe(forward)); //pEntity->GetRotation() * dirWeapon;
		state.fireTarget = m_fireTarget;
		state.fireDirection = (state.fireTarget - state.weaponPosition).GetNormalizedSafe();
	}

	//changed by ivo: most likely this doesn't work any more
	//state.movementDirection = pEntity->GetRotation() * pSkeleton->GetCurrentBodyDirection();
	state.movementDirection = pEntity->GetRotation() * Vec3(0,1,0);


	if (m_pPlayer->GetLastRequestedVelocity().len2() > 0.01f)
		state.movementDirection = m_pPlayer->GetLastRequestedVelocity().GetNormalized();

	//changed by ivo: most likely this doesn't work any more
	//state.bodyDirection = pEntity->GetRotation() * pSkeleton->GetCurrentBodyDirection();
	state.bodyDirection = pEntity->GetRotation() * Vec3(0,1,0);

	state.lean = m_pPlayer->GetActorStats() ? ((SPlayerStats*)m_pPlayer->GetActorStats())->leanAmount : 0.0f;

	state.atMoveTarget = m_atTarget;
	state.desiredSpeed = m_desiredSpeed;
	state.stance = m_pPlayer->GetStance();
	state.upDirection = m_pPlayer->GetBaseQuat().GetColumn2();
//	state.minSpeed = MIN_DESIRED_SPEED;
//	state.normalSpeed = m_pPlayer->GetStanceNormalSpeed(state.stance);
//	state.maxSpeed = m_pPlayer->GetStanceMaxSpeed(state.stance); 

	/*
  Vec2 minmaxSpeed = Vec2(0, 0);
	if(m_pPlayer->GetAnimationGraphState())	//might get here during loading before AG is serialized
		minmaxSpeed = m_pPlayer->GetAnimationGraphState()->GetQueriedStateMinMaxSpeed();
  state.minSpeed = minmaxSpeed[0];
  state.maxSpeed = minmaxSpeed[1];
  state.normalSpeed = 0.5f*(state.minSpeed + state.maxSpeed);
	if (state.maxSpeed < state.minSpeed)
	{
//		assert(state.stance == STANCE_NULL);
		state.maxSpeed = state.minSpeed;
		//if (!g_pGame->GetIGameFramework()->IsEditing())
		//	GameWarning("%s In STANCE_NULL - movement speed is clamped", pEntity->GetName());
	}
	if (state.normalSpeed < state.minSpeed)
		state.normalSpeed = state.minSpeed;
*/

	state.minSpeed = -1.0f;
	state.maxSpeed = -1.0f;
	state.normalSpeed = -1.0f;

	state.stanceSize = m_pPlayer->GetStanceInfo(state.stance)->GetStanceBounds();

	state.pos = pEntity->GetWorldPos();
	//FIXME:some E3 work around
	if (m_pPlayer->GetActorStats() && m_pPlayer->GetActorStats()->mountedWeaponID)
		state.isAiming = true;
	else if (isCharacterVisible)
		state.isAiming = pCharacter->GetISkeleton()->GetAimIKBlend() > 0.99f;
	else
		state.isAiming = true;

	state.isFiring = (m_pPlayer->GetActorStats()->inFiring>=10.f);

	// TODO: remove this
	//if (m_state.HasAimTarget())
	//	state.aimDirection = (m_state.GetAimTarget() - state.handPosition).GetNormalizedSafe();

	state.isAlive = (m_pPlayer->GetHealth()>0);

	IVehicle *pVehicle = m_pPlayer->GetLinkedVehicle();
	if (pVehicle)
	{
		IMovementController *pVehicleMovementController = pVehicle->GetPassengerMovementController(m_pPlayer->GetEntityId());
		if (pVehicleMovementController)
			pVehicleMovementController->GetMovementState(state);
	}	
}

bool CPlayerMovementController::GetStanceState( EStance stance, float lean, bool defaultPose, SStanceState& state )
{
	IEntity * pEntity = m_pPlayer->GetEntity();
	const SStanceInfo*	stanceInfo = m_pPlayer->GetStanceInfo(stance);
	if(!stanceInfo)
		return false;

	Quat	orientation = m_pPlayer->GetBaseQuat(); // pEntity->GetWorldRotation();
	Vec3	forward = orientation * FORWARD_DIRECTION;
	Vec3	entityPos = pEntity->GetWorldPos();

	state.pos = entityPos;
	state.stanceSize = stanceInfo->GetStanceBounds();
	state.colliderSize = stanceInfo->GetColliderBounds();
	state.lean = lean;	// pass through

	if(defaultPose)
	{
		state.eyePosition = entityPos + stanceInfo->GetViewOffsetWithLean(lean);
		state.weaponPosition = entityPos + stanceInfo->GetWeaponOffsetWithLean(lean);
		state.upDirection.Set(0,0,1);
		state.eyeDirection = FORWARD_DIRECTION;
		state.aimDirection = FORWARD_DIRECTION;
		state.fireDirection = FORWARD_DIRECTION;
		state.bodyDirection = FORWARD_DIRECTION;
	}
	else
	{
		Vec3	constrainedLookDir = m_lookTarget - entityPos;
		Vec3	constrainedAimDir = m_aimTarget - entityPos;
		constrainedLookDir.z = 0.0f;
		constrainedAimDir.z = 0.0f;

		constrainedAimDir = constrainedAimDir.GetNormalizedSafe(constrainedLookDir.GetNormalizedSafe(forward));
		constrainedLookDir = constrainedLookDir.GetNormalizedSafe(forward);

		Matrix33	lookRot;
		lookRot.SetRotationVDir(constrainedLookDir);
		state.eyePosition = entityPos + lookRot.TransformVector(stanceInfo->GetViewOffsetWithLean(lean));

		Matrix33	aimRot;
		aimRot.SetRotationVDir(constrainedAimDir);
		state.weaponPosition = entityPos + aimRot.TransformVector(stanceInfo->GetWeaponOffsetWithLean(lean));

		state.upDirection = m_pPlayer->GetBaseQuat().GetColumn2();

		state.eyeDirection = (m_lookTarget - state.eyePosition).GetNormalizedSafe(forward);
		state.aimDirection = (m_aimTarget - state.weaponPosition).GetNormalizedSafe((m_lookTarget - state.weaponPosition).GetNormalizedSafe(forward));
		state.fireDirection = state.aimDirection;
		state.bodyDirection = forward;
	}

	return true;
}

Vec3 CPlayerMovementController::ProcessAimTarget(const Vec3 &newTarget,float frameTime)
{
	Vec3 delta(m_aimTargets[0] - newTarget);
	m_aimTargets[0] = Vec3::CreateLerp(m_aimTargets[0],newTarget,min(1.0f,frameTime*max(1.0f,delta.len2()*1.0f)));

	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( newTarget, ColorF(1,0,0,1), m_aimTargets[0], ColorF(1,0,0,1) );

	return m_aimTargets[0];
	/*int targets(m_aimTargets.size());

	m_aimTargets[m_aimTargetsCount++] = newTarget;
	bool exit;
	do
	{
		exit = true;

		if (m_aimTargetsCount == m_aimTargetsIterator)
		{
			exit = false;
			++m_aimTargetsCount;
		}
		if (m_aimTargetsCount>=targets)
			m_aimTargetsCount = 0;
	}
	while(!exit);

	float holdTime(0.55f);
	m_aimNextTarget -= frameTime;
	if (m_aimNextTarget<0.001f)
	{
		m_aimNextTarget = holdTime;

		++m_aimTargetsIterator;
		if (m_aimTargetsIterator>=targets)
			m_aimTargetsIterator = 0;
	}

	int nextIter(m_aimTargetsIterator+1);
	if (nextIter>=targets)
		nextIter = 0;

	return Vec3::CreateLerp(m_aimTargets[nextIter],m_aimTargets[m_aimTargetsIterator],m_aimNextTarget*(1.0f/holdTime));*/
}

void CPlayerMovementController::Release()
{
	delete this;
}

void CPlayerMovementController::IdleUpdate(float frameTime)
{
	m_idleChecker.Update(frameTime);
}

bool CPlayerMovementController::GetStats(IActorMovementController::SStats& stats)
{
	stats.idle = m_idleChecker.IsIdle();
	return true;
}

CPlayerMovementController::CIdleChecker::CIdleChecker() : m_pMC(0)
{
}


void CPlayerMovementController::CIdleChecker::Reset(CPlayerMovementController* pMC)
{
	m_pMC = pMC;
	m_timeToIdle = IDLE_CHECK_TIME_TO_IDLE;
	m_movementTimeOut = IDLE_CHECK_MOVEMENT_TIMEOUT;
	m_bInIdle = false;
	m_bHaveValidMove = false;
	m_bHaveInteresting = false;
	m_frameID = -1;
}

void CPlayerMovementController::CIdleChecker::Update(float frameTime)
{
	if (m_pMC->m_pPlayer->IsPlayer())
		return;

	bool goToIdle = false;
	bool wakeUp = false;

	if (m_bHaveValidMove)
	{
		m_movementTimeOut = IDLE_CHECK_MOVEMENT_TIMEOUT;
		m_bHaveValidMove = false;
		wakeUp = true;
	}
	else
	{
		m_movementTimeOut-= frameTime;
		if (m_movementTimeOut <= 0.0f)
		{
			goToIdle = true;
		}
	}

#if 0
	if (m_bHaveInteresting)
	{
		m_timeToIdle = IDLE_CHECK_TIME_TO_IDLE;
		m_bHaveInteresting = false;
		wakeUp = true;
	}
	else
	{
		m_timeToIdle -= frameTime;
		if (m_timeToIdle <= 0.0f)
		{
			goToIdle = true;
		}
	}
#endif

	if (goToIdle && !wakeUp && m_bInIdle == false)
	{
		// turn idle on
#ifdef DEBUG_IDLE_CHECK
		CryLogAlways("Turn Idle ON 0x%p %s", this, m_pMC->m_pPlayer->GetEntity()->GetName());
#endif
		m_bInIdle = true;
	}
	else if (wakeUp && m_bInIdle)
	{
		// turn idle off
#ifdef DEBUG_IDLE_CHECK
		CryLogAlways("Turn Idle OFF 0x%p %s", this, m_pMC->m_pPlayer->GetEntity()->GetName());
#endif
		m_bInIdle = false;
	}

	static int sCurFrame = -1;
	static int sIdles = 0;
	static int sAwake = 0;
	static ICVar* pShowIdleStats = gEnv->pConsole->GetCVar("g_showIdleStats");
	if (pShowIdleStats && pShowIdleStats->GetIVal() != 0)
	{
		int frameId = gEnv->pRenderer->GetFrameID(false);
		if (frameId != sCurFrame)
		{
			float fColor[4] = {1,0,0,1};
			gEnv->pRenderer->Draw2dLabel( 1,22, 1.3f, fColor, false,"Idles=%d Awake=%d", sIdles, sAwake );
			sCurFrame = frameId;
			sIdles = sAwake = 0;
		}
		if (m_bInIdle)
			++sIdles;
		else
			++sAwake;
	}
}

bool CPlayerMovementController::CIdleChecker::Process(SMovementState& movementState, 
																										  CMovementRequest& currentReq,
																										  CMovementRequest& newReq)
{
	if (m_pMC->m_pPlayer->IsPlayer())
		return false;

	IRenderer* pRend = gEnv->pRenderer;
	int nCurrentFrameID = pRend->GetFrameID(false);

	// check if we have been called already in this frame
	// if so, and we already have a validMove request -> early return
	if (nCurrentFrameID != m_frameID)
	{
		m_frameID = nCurrentFrameID;
	}
	else
	{
		if (m_bHaveValidMove)
			return true;
	}

	// no valid move request up to now (not in this frame)
	m_bHaveValidMove = false;

	// an empty moverequest 
	if (currentReq.IsEmpty())
	{
		// CryLogAlways("IdleCheck: Req empty 0x%p %s", this, m_pMC->m_pPlayer->GetEntity()->GetName());
		;
	}
	else if (newReq.HasDeltaMovement() && newReq.HasDeltaRotation())
	{
		m_bHaveValidMove = true;
	}
	else if (newReq.HasMoveTarget() && 
		( (newReq.HasDesiredSpeed() && newReq.GetDesiredSpeed() > 0.001f)
		||(newReq.HasPseudoSpeed() && newReq.GetPseudoSpeed() > 0.001f) ))
	{
		m_bHaveValidMove = true;
	}

	return m_bHaveValidMove;
}

void CPlayerMovementController::CIdleChecker::Serialize(TSerialize &ser)
{
	ser.Value("m_timeToIdle", m_timeToIdle);
	ser.Value("m_movementTimeOut", m_movementTimeOut);
	ser.Value("m_frameID", m_frameID);
	ser.Value("m_bHaveInterest", m_bHaveInteresting);
	ser.Value("m_bInIdle", m_bInIdle);
	ser.Value("m_bHaveValidMove", m_bHaveValidMove);
}

void CPlayerMovementController::Serialize(TSerialize &ser)
{
	if(ser.GetSerializationTarget() != eST_Network)	//basic serialization
	{
		ser.Value("DesiredSpeed", m_desiredSpeed);
		ser.Value("atTarget", m_atTarget);
		ser.Value("m_usingLookIK", m_usingLookIK);
		ser.Value("m_usingAimIK", m_usingAimIK);
		ser.Value("m_lookTarget", m_lookTarget);
		ser.Value("m_aimTarget", m_aimTarget);
		ser.Value("m_animTargetSpeed", m_animTargetSpeed);
		ser.Value("m_animTargetSpeedCounter", m_animTargetSpeedCounter);
		ser.Value("m_fireTarget", m_fireTarget);
		ser.EnumValue("targetStance", m_targetStance, STANCE_NULL, STANCE_LAST);

		ser.Value("NumAimTarget", m_aimTargetsCount);
		int numTargets = m_aimTargets.size();
		ser.Value("ActualNumTarget", numTargets);
		if(ser.IsReading())
		{
			m_aimTargets.clear();
			m_aimTargets.resize(numTargets);
		}
		for(int i = 0; i < numTargets; ++i)
			ser.Value("aimTarget", m_aimTargets[i]);

		ser.Value("m_aimTargetIterator", m_aimTargetsIterator);
		ser.Value("m_aimNextTarget", m_aimNextTarget);

		SMovementState m_currentMovementState;

		ser.BeginGroup("m_currentMovementState");

		ser.Value("bodyDir", m_currentMovementState.bodyDirection);
		ser.Value("aimDir", m_currentMovementState.aimDirection);
		ser.Value("fireDir", m_currentMovementState.fireDirection);
		ser.Value("fireTarget", m_currentMovementState.fireTarget);
		ser.Value("weaponPos", m_currentMovementState.weaponPosition);
		ser.Value("desiredSpeed", m_currentMovementState.desiredSpeed);
		ser.Value("moveDir", m_currentMovementState.movementDirection);
		ser.Value("upDir", m_currentMovementState.upDirection);
		ser.EnumValue("stance", m_currentMovementState.stance, STANCE_NULL, STANCE_LAST);
		ser.Value("Pos", m_currentMovementState.pos);
		ser.Value("eyePos", m_currentMovementState.eyePosition);
		ser.Value("eyeDir", m_currentMovementState.eyeDirection);
		ser.Value("animationEyeDirection", m_currentMovementState.animationEyeDirection);
		ser.Value("minSpeed", m_currentMovementState.minSpeed);
		ser.Value("normalSpeed", m_currentMovementState.normalSpeed);
		ser.Value("maxSpeed", m_currentMovementState.maxSpeed);
		ser.Value("stanceSize.Min", m_currentMovementState.stanceSize.min);
		ser.Value("stanceSize.Max", m_currentMovementState.stanceSize.max);
		ser.Value("colliderSize.Min", m_currentMovementState.colliderSize.min);
		ser.Value("colliderSize.Max", m_currentMovementState.colliderSize.max);
		ser.Value("atMoveTarget", m_currentMovementState.atMoveTarget);
		ser.Value("isAlive", m_currentMovementState.isAlive);
		ser.Value("isAiming", m_currentMovementState.isAiming);
		ser.Value("isFiring", m_currentMovementState.isFiring);
		ser.Value("isVis", m_currentMovementState.isVisible);

		ser.EndGroup();

		ser.BeginGroup("AimInterpolator");
		m_aimInterpolator.Serialize(ser);
		ser.EndGroup();

		ser.BeginGroup("LookInterpolator");
		m_lookInterpolator.Serialize(ser);
		ser.EndGroup();

		ser.Value("inputDesiredSpeed", m_inputDesiredSpeed);
		ser.Value("inputDersiredAngleZ", m_inputDesiredTurnAngleZ);
		ser.Value("inputStance", m_inputStance);
		ser.Value("inputPseudoSpeed", m_inputPseudoSpeed);

		if(ser.IsReading())
			m_idleChecker.Reset(this);
		m_idleChecker.Serialize(ser);

		if(ser.IsReading())
			UpdateMovementState(m_currentMovementState);
	}
}
