#include "StdAfx.h"
#include "PlayerMovement.h"
#include "GameUtils.h"
#include "Game.h"
#include "GameCVars.h"
#include "PlayerInput.h"
#include "GameActions.h"
#include "NetInputChainDebug.h"

#define CALL_PLAYER_EVENT_LISTENERS(func) \
{ \
	if (m_player.m_playerEventListeners.empty() == false) \
	{ \
	  CPlayer::TPlayerEventListeners::const_iterator iter = m_player.m_playerEventListeners.begin(); \
	  CPlayer::TPlayerEventListeners::const_iterator cur; \
	  while (iter != m_player.m_playerEventListeners.end()) \
	  { \
	  	cur = iter; \
	  	++iter; \
	  	(*cur)->func; \
	  } \
	} \
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
CPlayerMovement::CPlayerMovement( const CPlayer& player, const SActorFrameMovementParams& movement, float m_frameTime ) : 
	m_frameTime(m_frameTime),
	m_params(player.m_params),
	m_stats(player.m_stats),
	m_viewQuat(player.m_viewQuat),
	m_baseQuat(player.m_baseQuat),
	m_movement(movement),
	m_player(player),
	m_velocity(player.m_velocity),
	m_upVector(player.m_upVector),
	m_detachLadder(false),
	m_onGroundWBoots(player.m_stats.onGroundWBoots),
	m_jumped(player.m_stats.jumped),
	m_actions(player.m_actions),
	m_thrusters(0.0f),
	m_turnTarget(player.m_turnTarget),
	m_thrusterSprint(player.m_stats.thrusterSprint),
	m_hasJumped(false),
	m_waveRandomMult(1.0f)
{
	// derive some values that will be useful later
	m_worldPos = player.GetEntity()->GetWorldPos();

	m_waveTimer = Random()*gf_PI;
}

void CPlayerMovement::Process(CPlayer& player)
{
	//FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (m_stats.spectatorMode || m_stats.flyMode)
		ProcessFlyMode();
	else if (m_stats.isOnLadder)
			ProcessMovementOnLadder(player);
	else if (m_stats.inAir && m_stats.inZeroG)
		ProcessFlyingZeroG();
	else if (m_stats.inFreefall.Value()==1)
	{
		m_request.type = eCMT_Normal;
		m_request.velocity.zero();
	}
	else if (m_stats.inFreefall.Value()==2)
		ProcessParachute();
	else if (player.ShouldSwim())
		ProcessSwimming();
	else
		ProcessOnGroundOrJumping(player);

	// if (!m_player.GetLinkedEntity() && !m_player.GetEntity()->GetParent()) // Leipzig hotfix, these can get out of sync
	if (player.m_linkStats.CanRotate())
		ProcessTurning();
}

void CPlayerMovement::Commit( CPlayer& player )
{
	if (player.m_pAnimatedCharacter)
	{
    if (m_movement.prediction.nStates > 0)
    {
      m_request.prediction = m_movement.prediction;
    }
    else
    {
      m_request.prediction.nStates = 1;
      m_request.prediction.states[0].deltatime = 0.0f;
      m_request.prediction.states[0].velocity = m_request.velocity;
      m_request.prediction.states[0].position = player.GetEntity()->GetWorldPos();
      m_request.prediction.states[0].orientation = player.GetEntity()->GetWorldRotation();
    }

		NETINPUT_TRACE(m_player.GetEntityId(), m_request.rotation * FORWARD_DIRECTION);
		NETINPUT_TRACE(m_player.GetEntityId(), m_request.velocity);

		player.m_pAnimatedCharacter->AddMovement( m_request );
	}

	if (m_detachLadder)
		player.CreateScriptEvent("detachLadder",0);
	if (m_thrusters > .1f && m_stats.onGroundWBoots>-0.01f)
		player.CreateScriptEvent("thrusters",(m_actions & ACTION_SPRINT)?1:0);
	if (m_hasJumped)
		player.CreateScriptEvent("jumped",0);

	NETINPUT_TRACE(m_player.GetEntityId(), m_velocity);
	NETINPUT_TRACE(m_player.GetEntityId(), m_jumped);

	player.m_velocity = m_velocity;
	player.m_stats.jumped = m_jumped;
	player.m_stats.onGroundWBoots = m_onGroundWBoots;
	player.m_turnTarget = m_turnTarget;
	player.m_lastRequestedVelocity = m_request.velocity; 
	player.m_stats.thrusterSprint = m_thrusterSprint;
	
	if(!player.m_stats.isOnLadder)
		player.m_stats.bSprinting = ((m_stats.onGround>0.1f || (m_stats.inWater > 0.0f)) && m_stats.inMovement>0.1f && m_actions & ACTION_SPRINT);
}

//-----------------------------------------------------------------------------------------------
// utility functions
//-----------------------------------------------------------------------------------------------
static Vec3 ProjectPointToLine(const Vec3 &point,const Vec3 &lineStart,const Vec3 &lineEnd)
{
	Lineseg seg(lineStart, lineEnd);
	float t;
	Distance::Point_Lineseg( point, seg, t );
	return seg.GetPoint(t);
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessFlyMode()
{
	Vec3 move = m_viewQuat * m_movement.desiredVelocity;

	float zMove(0.0f);
	if (m_actions & ACTION_JUMP)
		zMove += 1.0f;
	if (m_actions & ACTION_CROUCH)
		zMove -= 1.0f;

	move += m_viewQuat.GetColumn2() * zMove;

	//cap the movement vector to max 1
	float moveModule(move.len());

	if (moveModule > 1.0f)
 		move /= moveModule;

	move *= m_params.speedMultiplier;  // respect speed multiplier as well
	move *= 30.0f;

	if (m_actions & ACTION_SPRINT)
		move *= 10.0f;

	m_request.type = eCMT_Fly;
	m_request.velocity = move;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessFlyingZeroG()
{
	CNanoSuit* pSuit = m_player.GetNanoSuit();
	assert(pSuit);

	//movement
	Vec3 move(0,0,0);//usually 0, except AIs

	Vec3 desiredVelocity(m_movement.desiredVelocity);
	if((m_actions & ACTION_MOVE) && (desiredVelocity.len() > 1.0f))
		desiredVelocity.Normalize();

	if((m_actions & ACTION_SPRINT) && (m_player.GetNanoSuit()->GetMode() == NANOMODE_SPEED))
	{
		move = m_viewQuat.GetColumn1() * desiredVelocity.len() * ((m_actions & ACTION_ZEROGBACK)?(-1.0f):(1.0f));
		CryLogAlways("%i", m_actions & ACTION_ZEROGBACK);
	}
	else
	{
		if (m_actions & ACTION_JUMP)
			desiredVelocity.z += g_pGameCVars->v_zeroGUpDown; 
		if (m_actions & ACTION_CROUCH)
			desiredVelocity.z -= g_pGameCVars->v_zeroGUpDown;

		move += m_baseQuat.GetColumn0() * desiredVelocity.x;
		move += m_baseQuat.GetColumn1() * desiredVelocity.y;
		move += m_baseQuat.GetColumn2() * desiredVelocity.z;
	}

	//cap the movement vector to max 1
	float moveModule(move.len());
	if (moveModule > 1.0f)
	{
		move /= moveModule;
	}

	//afterburner
	if (m_actions & ACTION_SPRINT)
	{
		float mult(1.0f);
		if (m_thrusterSprint>0.001f)
			mult += (m_params.afterburnerMultiplier) * m_thrusterSprint;

		if (move.len2()>0.01f)
			m_thrusterSprint = max(0.0f,m_thrusterSprint - m_frameTime * 5.0f);

		if(pSuit->GetMode() == NANOMODE_SPEED && m_actions & ACTION_MOVE)
		{
			if(pSuit->GetSuitEnergy() >= NANOSUIT_ENERGY * 0.2f)
			{
				mult *= 2.0f;
				pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-100.0f*g_pGameCVars->v_zeroGSpeedModeEnergyConsumption*m_frameTime);
			}
			else if(pSuit->GetSuitEnergy() < NANOSUIT_ENERGY * 0.2f)
			{
				mult *= 1.3f;
				pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-25.0f*g_pGameCVars->v_zeroGSpeedModeEnergyConsumption*m_frameTime);
			}
		}

		move *= mult;
	}

	AdjustMovementForEnvironment( move );

	m_thrusters = moveModule;

	float inertiaMul(1.0f);
	if (move.len2()>0.1)
	{
		inertiaMul = min(1.0f,max(0.0f,1.0f - move * m_stats.velocityUnconstrained));
		inertiaMul = 1.0f + inertiaMul * 2.0f;

		//if(CNanoSuit *pSuit = m_player.GetNanoSuit())
		//	inertiaMul *= 1.0f+pSuit->GetSlotValue(NANOSLOT_SPEED)*0.01f;
	}

	move *= m_params.thrusterImpulse * m_frameTime;

	float stabilize = m_params.thrusterStabilizeImpulse;
	bool autoStabilize = false;
	if(m_player.GetStabilize())
	{
		stabilize = 2.0f;
		if(pSuit->GetMode() == NANOMODE_SPEED)
			stabilize = 3.0f;
		else if(pSuit->GetMode() == NANOMODE_STRENGTH)
			stabilize = 5.0f;
	}
	else if(float len = m_stats.gravity.len()) //we are probably in some gravity stream
	{
		stabilize = len * 0.2f;
		autoStabilize = true;
	}

	//this is important for the player's gravity stream movement
	move += (m_stats.gravity*0.81f - m_stats.velocityUnconstrained) * min(1.0f,m_frameTime*stabilize*inertiaMul);

	if (stabilize > 1.0f && !autoStabilize)
	{
		float velLen = m_velocity.len();
		if(velLen > 0.5f)	//play sound when the stabilize is actually used
		{
			pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-stabilize*10.0f*m_frameTime);
			pSuit->PlaySound(ESound_ZeroGThruster, std::min(1.0f, 1.0f-(velLen * stabilize * 0.02f)));
		}
		else
		{
			pSuit->PlaySound(ESound_ZeroGThruster, 1.0f, true);
		}
	}

	if (m_player.GravityBootsOn() && desiredVelocity.z<0.1f)
	{
		IPhysicalEntity *pSkip = m_player.GetEntity()->GetPhysics();
		ray_hit hit;

		Vec3 ppos(m_player.GetEntity()->GetWorldPos());

		int rayFlags = (COLLISION_RAY_PIERCABILITY & rwi_pierceability_mask);
		float rayLen(10.0f);
		if (gEnv->pPhysicalWorld->RayWorldIntersection(ppos, m_baseQuat.GetColumn2() * -rayLen, ent_terrain|ent_static|ent_rigid, rayFlags, &hit, 1, &pSkip, 1) && m_player.IsMaterialBootable(hit.surface_idx))
		{
			Vec3 delta(hit.pt - ppos);
			float len = delta.len();
			float lenMult(min(1.0f,len/rayLen));
			lenMult = 1.0f - lenMult*lenMult;

			/*Vec3 goalPos = hit.pt + hit.n * (len-rayLen*lenMult);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(hit.pt, ColorB(255,255,0,100), goalPos, ColorB(255,0,255,100));
			delta = goalPos - ppos;
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(ppos, ColorB(0,255,0,100), ppos + delta, ColorB(255,0,0,100));*/

			delta /= max(0.001f,len);

			move += delta * (10.0f * lenMult * m_frameTime);

			m_gBootsSpotNormal = hit.n;
		}
		else
			m_gBootsSpotNormal.Set(0,0,0);
	}
	//

	//FIXME:pretty hacky, needed when passing from zeroG to normalG
	m_velocity = m_stats.velocityUnconstrained;
	
	m_velocity = m_velocity * m_baseQuat.GetInverted();
	m_velocity.z = 0;
	m_velocity = m_velocity * m_baseQuat;

	// Design tweak values for movement speed
	if((m_actions & ACTION_MOVE) || (m_actions & ACTION_JUMP) || (m_actions & ACTION_CROUCH))
	{
		if(m_actions & ACTION_SPRINT && pSuit->GetSuitEnergy() > NANOSUIT_ENERGY * 0.01f)
		{
			if(m_player.GetNanoSuit()->GetMode() == NANOMODE_SPEED)
				move *= g_pGameCVars->v_zeroGSpeedMultSpeedSprint;
			else
				move *= g_pGameCVars->v_zeroGSpeedMultNormalSprint;
		}
		else
		{
			if(m_player.GetNanoSuit()->GetMode() == NANOMODE_SPEED)
				move *= g_pGameCVars->v_zeroGSpeedMultSpeed;
			else
				move *= g_pGameCVars->v_zeroGSpeedMultNormal;
		}
	}

	m_request.type = eCMT_Impulse;
	m_request.velocity = move * m_stats.mass;

	if (pSuit && pSuit->GetMode() == NANOMODE_SPEED)
	{
		if(m_request.velocity.len() > g_pGameCVars->v_zeroGSpeedMaxSpeed)
			if((m_actions & ACTION_MOVE) || (m_actions & ACTION_JUMP) || (m_actions & ACTION_CROUCH))
				m_request.velocity.SetLength(g_pGameCVars->v_zeroGSpeedMaxSpeed);
	}
	else
	{
		if(m_request.velocity.len() > g_pGameCVars->v_zeroGMaxSpeed)
			if((m_actions & ACTION_MOVE) || (m_actions & ACTION_JUMP) || (m_actions & ACTION_CROUCH))
				m_request.velocity.SetLength(g_pGameCVars->v_zeroGMaxSpeed);
	}
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessSwimming()
{
	bool debug = (g_pGameCVars->cl_debugSwimming != 0);
	Vec3 entityPos = m_player.GetEntity()->GetWorldPos();
	Vec3 vRight(m_baseQuat.GetColumn0());

	IPhysicalEntity* pPhysEnt = m_player.GetEntity()->GetPhysics();
	pe_status_dynamics sd;
	if (pPhysEnt->GetStatus(&sd) != 0)
		m_velocity = sd.v;

/*
	if (m_velocity.z < -10.0f)
		m_velocity.z = -10.0f;
	if (m_velocity.z > +10.0f)
		m_velocity.z = +10.0f;
*/

	// Normalize DENormalized values (NOTE: If these trigger something outside this function denormalized the velocity).
	if (((FloatU32(m_velocity.x) & FloatU32ExpMask) == 0) && ((FloatU32(m_velocity.x) & FloatU32FracMask) != 0))
   		m_velocity.x = 0.0f;
	if (((FloatU32(m_velocity.y) & FloatU32ExpMask) == 0) && ((FloatU32(m_velocity.y) & FloatU32FracMask) != 0))
		m_velocity.y = 0.0f;

	Vec3 acceleration(ZERO);

	//--------------------

	// Apply gravity when above the surface.
 	if (m_stats.waterLevel > 0.0f)
	{
		// Apply it gradually when close to surface, to prevent gravity to pull character down to fast.
		float fraction = CLAMP(m_stats.waterLevel / 0.2f, 0.0f, 1.0f);
		if (!m_stats.gravity.IsZero())
			acceleration += m_stats.gravity * 0.01f * fraction;
		else
			acceleration.z += -9.8f * 0.01f * fraction;
	}

	//--------------------

	// Apply jump impulse when below but close to the surface (if in water for long enough).
	bool jump = false;
	if ((m_actions & ACTION_JUMP) && (m_velocity.z >= -0.2f) && (m_stats.waterLevel > -0.1f) && (m_stats.waterLevel < 0.1f))
	{
		jump = true;
		m_velocity.z = 6.0f;
	}

	//--------------------

	// Apply ocean wave impulse when below but close to the surface.
	if ((m_stats.waterLevel > -0.5f) && (m_stats.waterLevel < 0.1f))
	{
		// some wave code stolen from boats
		float waveFreq = 4.f;

		float waveTimerPrev = m_waveTimer;
		m_waveTimer += m_frameTime*waveFreq;

		// new randomized amount for this oscillation
		if (m_waveTimer >= gf_PI && waveTimerPrev < gf_PI) 
			m_waveRandomMult = Random();  

		if (m_waveTimer >= 2*gf_PI)  
			m_waveTimer -= 2*gf_PI;    

		float kx = 12.0f * (m_waveRandomMult+0.3f) + /*m_waveSpeedMult**/m_stats.speed;
		float ky = 20.0f * (1.f - 0.5f*m_stats.speed - 0.5f);

		pe_action_impulse waveImp;
		waveImp.angImpulse.x = sin(m_waveTimer) * m_frameTime * kx;

		if (isneg(waveImp.angImpulse.x))
			waveImp.angImpulse.x *= (1.f - min(1.f, 2.f*m_stats.speed)); // less amplitude for negative impulse      

		waveImp.angImpulse.y = sin(m_waveTimer-0.5f*gf_PI) * m_frameTime * ky;  
		waveImp.angImpulse.z = (waveImp.angImpulse.x + waveImp.angImpulse.y) * 0.2f;
		Vec3 imp = m_player.GetEntity()->GetWorldTM().TransformVector(waveImp.angImpulse);

		// Currently not used, since swimming is WIP, and the waviness is merely decorative cream on top.
		// Also, there might be problems with using client specific (not synced beween clients) calculations to move the entity like this.
		//acceleration += imp;
	}

	//--------------------

	// Apply automatic float up towards surface when not in conflict with desired movement (if in water for long enough).
	if ((m_velocity.z > -0.1f) && (m_velocity.z < 0.2f) && (m_stats.waterLevel < -0.1f) && (m_stats.inWater > 0.5f))
		acceleration.z += (1.0f - sqr(1.0f - CLAMP(-m_stats.waterLevel, 0.0f, 1.0f))) * 0.08f;

	//--------------------

	// Apply desired movement
	Vec3 desiredLocalNormalizedVelocity(ZERO);
	Vec3 desiredLocalVelocity(ZERO);
	Vec3 desiredWorldVelocity(ZERO);

	// Less control when jumping or otherwise above surface
	// (where bottom ray miss geometry that still keeps the collider up).
	float userControlFraction = 1.0f;
	if (m_stats.waterLevel > 0.1f)
		userControlFraction = 0.2f;

	// Calculate desired acceleration (user input)		
	{
		// Apply up/down control when well below surface (if in water for long enough).
		if (m_stats.inWater > 0.5f)
		{
			if ((m_actions & ACTION_JUMP) && (m_velocity.z > -1.5f) && (m_stats.waterLevel < -0.1f))
				desiredLocalNormalizedVelocity.z += 1.0f;
			else if ((m_actions & ACTION_CROUCH) && (m_stats.waterLevel < 0.1f))
				desiredLocalNormalizedVelocity.z -= 1.0f;
		}

		float backwardMultiplier = (m_movement.desiredVelocity.y < 0.0f) ? m_params.backwardMultiplier : 1.0f;
		desiredLocalNormalizedVelocity.x = m_movement.desiredVelocity.x * m_params.strafeMultiplier;
		desiredLocalNormalizedVelocity.y = m_movement.desiredVelocity.y * backwardMultiplier;
		if (debug)
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.4f), 1.5f, "MoveN[%1.3f, %1.3f, %1.3f]", desiredLocalNormalizedVelocity.x, desiredLocalNormalizedVelocity.y, desiredLocalNormalizedVelocity.z);

		// AI can set a custom sprint value, so don't cap the movement vector
		float sprintMultiplier = 1.0f;
		if (m_movement.sprint <= 0.0f)
		{
			//cap the movement vector to max 1
			float len(desiredLocalNormalizedVelocity.len());
			if (len > 1.0f)
				desiredLocalNormalizedVelocity /= len;

			if (m_actions & ACTION_SPRINT)
			{
				sprintMultiplier = m_params.sprintMultiplier;

				CNanoSuit* pSuit = m_player.GetNanoSuit();
				if (pSuit != NULL)
					sprintMultiplier = pSuit->GetSprintMultiplier();
			}

			// Higher speed multiplier when looking up, to get higher dolphin jumps.
			sprintMultiplier *= LERP(1.0f, 2.5f, CLAMP(m_viewQuat.GetFwdZ(), 0.0f, 1.0f)); //swimming speed bonus to adjust dolphin jump
		}

		if (debug)
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.8f), 1.5f, "SprintMul %1.2f", sprintMultiplier);

		float stanceMaxSpeed = m_player.GetStanceMaxSpeed(STANCE_SWIM);
		if (debug)
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.6f), 1.5f, "StanceMax %1.3f", stanceMaxSpeed);

		desiredLocalVelocity.x = desiredLocalNormalizedVelocity.x * sprintMultiplier * stanceMaxSpeed;
		desiredLocalVelocity.y = desiredLocalNormalizedVelocity.y * sprintMultiplier * stanceMaxSpeed;
		desiredLocalVelocity.z = desiredLocalNormalizedVelocity.z * stanceMaxSpeed;

		AdjustMovementForEnvironment(desiredLocalVelocity);

		// The desired movement is applied in viewspace, not in entityspace, since entity does not nessecarily pitch while swimming.
		desiredWorldVelocity += m_viewQuat.GetColumn0() * desiredLocalVelocity.x * 0.1f;
		desiredWorldVelocity += m_viewQuat.GetColumn1() * desiredLocalVelocity.y * 0.1f;
		//desiredVelocity += m_viewQuat.GetColumn2() * desiredMovement.z * 0.1f;
		desiredWorldVelocity.z += desiredLocalVelocity.z * 0.1f;

/*
		Ang3 view(m_viewQuat);
		if (debug)
		{
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.8f), 1.5f, "View[%1.3f, %1.3f, %1.3f]", view.x, view.y, view.z);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos+Vec3(0,0,1), ColorF(1,0,0,1), entityPos+Vec3(0,0,1)+m_viewQuat.GetColumn0(), ColorF(1,0,0,1), 5.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos+Vec3(0,0,1), ColorF(0,1,0,1), entityPos+Vec3(0,0,1)+m_viewQuat.GetColumn1(), ColorF(0,1,0,1), 5.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos+Vec3(0,0,1), ColorF(0,0,1,1), entityPos+Vec3(0,0,1)+m_viewQuat.GetColumn2(), ColorF(0,0,1,1), 5.0f);
		}
*/

		//if ((m_stats.waterLevel > 0.2f) && (desiredWorldVelocity.z > 0.0f)) // WIP: related to jumping out of water
		if ((m_stats.waterLevel > -0.1f) && (desiredWorldVelocity.z > 0.0f))
			desiredWorldVelocity.z = 0.0f;

		if (debug)
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f + Vec3(0,0,0.2f), 1.5f, "Move[%1.3f, %1.3f, %1.3f]", desiredWorldVelocity.x, desiredWorldVelocity.y, desiredWorldVelocity.z);

		acceleration += desiredWorldVelocity * userControlFraction;
	}

	//--------------------

	// Apply acceleration (framerate independent)
	float accelerateDelay = 0.05f;
	m_velocity += acceleration * (m_frameTime / accelerateDelay);

	// Apply velocity dampening (framerate independent)
	Vec3 damping(ZERO);
	{
		if (m_stats.waterLevel <= 0.0f)
		{
			damping.x = abs(m_velocity.x);
			damping.y = abs(m_velocity.y);
		}

		static float zDampWLLo = -2.0f;
		static float zDampWLHi = -1.0f;
		static float zDampWLSpan = (zDampWLHi - zDampWLLo);
		static float zDampWLSpanInv = 1.0f / zDampWLSpan;
		float zDampDepthFraction = CLAMP((m_stats.waterLevel - zDampWLLo) * zDampWLSpanInv, 0.0f, 1.0f);
		float zDampSpeedFraction = CLAMP(m_velocity.z / 2.0f, 0.0f, 1.0f);
		float zDampFraction = min(zDampSpeedFraction, zDampDepthFraction);
		float zDamp = LERP(1.0f, 0.0f, zDampFraction);
		damping.z = abs(m_velocity.z) * zDamp;

		float stopDelay = 0.5f;
		damping *= (m_frameTime / stopDelay);
		m_velocity.x = (abs(m_velocity.x) > damping.x) ? (m_velocity.x - sgn(m_velocity.x) * damping.x) : 0.0f;
		m_velocity.y = (abs(m_velocity.y) > damping.y) ? (m_velocity.y - sgn(m_velocity.y) * damping.y) : 0.0f;
		m_velocity.z = (abs(m_velocity.z) > damping.z) ? (m_velocity.z - sgn(m_velocity.z) * damping.z) : 0.0f;
	}

	//--------------------

	// Normalize DENormalized values (TODO: find out where they come from, might be a bug).
	if (((FloatU32(m_velocity.x) & FloatU32ExpMask) == 0) && ((FloatU32(m_velocity.x) & FloatU32FracMask) != 0))
		m_velocity.x = 0.0f;
	if (((FloatU32(m_velocity.y) & FloatU32ExpMask) == 0) && ((FloatU32(m_velocity.y) & FloatU32FracMask) != 0))
		m_velocity.y = 0.0f;

	//--------------------

	// Set request type and velocity
	m_request.type = eCMT_Fly;
	m_request.velocity = m_velocity;
	
	// DEBUG VELOCITY
	if (debug)
	{
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.0f), 1.5f, "Velo[%1.3f, %1.3f, %1.3f]", m_velocity.x, m_velocity.y, m_velocity.z);
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.2f), 1.5f, " Axx[%1.3f, %1.3f, %1.3f]", acceleration.x, acceleration.y, acceleration.z);
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.4f), 1.5f, "Damp[%1.3f, %1.3f, %1.3f]", damping.x, damping.y, damping.z);
		gEnv->pRenderer->DrawLabel(entityPos - vRight * 1.5f - Vec3(0,0,0.6f), 1.5f, "FrameTime %1.4f", m_frameTime);
		if (jump)
			gEnv->pRenderer->DrawLabel(entityPos - vRight * 0.15f + Vec3(0,0,0.6f), 2.0f, "JUMP");
	}

	if (m_player.m_pAnimatedCharacter != NULL)
	{
		IAnimationGraphState* pAnimGraphState = m_player.m_pAnimatedCharacter->GetAnimationGraphState();
		if (pAnimGraphState != NULL)
		{
			IAnimationGraph::InputID inputSwimControlX = pAnimGraphState->GetInputId("SwimControlX");
			IAnimationGraph::InputID inputSwimControlY = pAnimGraphState->GetInputId("SwimControlY");
			IAnimationGraph::InputID inputSwimControlZ = pAnimGraphState->GetInputId("SwimControlZ");
			pAnimGraphState->SetInput(inputSwimControlX, desiredLocalVelocity.x);
			pAnimGraphState->SetInput(inputSwimControlY, desiredLocalVelocity.y);
			pAnimGraphState->SetInput(inputSwimControlZ, desiredWorldVelocity.z);
		}
	}

	return;






	// Filippos old version of the swimming movement control.

	//process movement
	/*Vec3 move(0,0,0);
	float upDown(0);

	if (m_actions & ACTION_JUMP && m_stats.inWater>0.1f)
		upDown += 1.0f;
	if (m_actions & ACTION_CROUCH)
		upDown -= 1.0f;

	if (m_movement.desiredVelocity.x || m_movement.desiredVelocity.y || upDown)
	{	
		//FIXME: strafe and backward multipler dont work for AIs since they use the m_input.movementVector
		float strafeMul(m_params.strafeMultiplier);
		float backwardMul(1.0f);

		//going back?
		if (m_movement.desiredVelocity.y<0.0f)
			backwardMul = m_params.backwardMultiplier;

		move += m_viewQuat.GetColumn0() * m_movement.desiredVelocity.x * strafeMul;
		move += m_viewQuat.GetColumn1() * m_movement.desiredVelocity.y * backwardMul;
		move += m_viewQuat.GetColumn2() * upDown;
	}

	if (m_stats.waterLevel > -0.01f)
	{
		move.z = min(move.z,-0.01f);
	}
	if(m_stats.waterLevel>-0.15f)
	{
		move += m_stats.gravity * 0.01f; //sink to a certain degree

		// some wave code stolen from boats
		float waveFreq = 4.f;

		float waveTimerPrev = m_waveTimer;
		m_waveTimer += m_frameTime*waveFreq;

		// new randomized amount for this oscillation
		if (m_waveTimer >= gf_PI && waveTimerPrev < gf_PI) 
			m_waveRandomMult = Random();  

		if (m_waveTimer >= 2*gf_PI)  
			m_waveTimer -= 2*gf_PI;    

		float kx = 12.0f * (m_waveRandomMult+0.3f) + m_stats.speed;
		float ky = 20.0f * (1.f - 0.5f*m_stats.speed - 0.5f);

		pe_action_impulse waveImp;
		waveImp.angImpulse.x = sin(m_waveTimer) * m_frameTime * kx;

		if (isneg(waveImp.angImpulse.x))
			waveImp.angImpulse.x *= (1.f - min(1.f, 2.f*m_stats.speed)); // less amplitude for negative impulse      

		waveImp.angImpulse.y = sin(m_waveTimer-0.5f*gf_PI) * m_frameTime * ky;  
		waveImp.angImpulse.z = (waveImp.angImpulse.x + waveImp.angImpulse.y) * 0.2f;
		Vec3 imp = m_player.GetEntity()->GetWorldTM().TransformVector(waveImp.angImpulse);

		move += imp;
	}

	//ai can set a custom sprint value, so dont cap the movement vector
	if (m_movement.sprint<=0.0f)
	{
		//cap the movement vector to max 1
		float moveModule(move.len());

		if (moveModule > 1.0f)
		{
			move /= moveModule;
		}

		float sprintMult = 1.0f;

		CNanoSuit *pSuit = m_player.GetNanoSuit();
		if(pSuit)
			sprintMult = pSuit->GetSprintMultiplier();

		if (m_actions & ACTION_SPRINT)
			move *= m_params.sprintMultiplier * sprintMult;
	}

	move *= m_player.GetStanceMaxSpeed(STANCE_SWIM);

	//apply movement
	Vec3 desiredVel(move);

	//float up if no movement requested
	if (move.z>-0.1f)
		desiredVel.z += min(1.0f,-m_stats.waterLevel*0.1f);

	if (m_velocity.len2()<=0.0f)
		m_velocity = desiredVel;

	if (m_stats.inWater < 0.15f)
		m_velocity.z = m_stats.velocityUnconstrained.z * 0.9f;

	if (m_actions & ACTION_JUMP && m_stats.inWater > 0.5f && m_stats.waterLevel>-0.1f)
	{
 		m_velocity.z += 1.5f;
	}	

	Interpolate(m_velocity,desiredVel,3.0f,m_frameTime);

	m_request.type = eCMT_Fly;
	m_request.velocity = m_velocity;*/
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessParachute()
{
	//Vec3 desiredVelocity(m_stats.velocity);
	float desiredZ(-1.5f + ((m_actions & ACTION_JUMP)?3.0f:0.0f));
	//desiredVelocity.z += (desiredZ - desiredVelocity.z)*min(1.0f,m_frameTime*1.5f);
	
	m_request.type = eCMT_Impulse;//eCMT_Fly;
	m_request.velocity = (Vec3(0,0,desiredZ)-m_stats.velocity) * m_stats.mass * m_frameTime;//desiredVelocity;

	Vec3 forwardComp(m_baseQuat.GetColumn1() * 10.0f);
	forwardComp.z = 0.0f;

	m_request.velocity += forwardComp * m_stats.mass * m_frameTime;//desiredVelocity;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessOnGroundOrJumping(CPlayer& player)
{
	//process movement
	Vec3 move(0,0,0);

	CNanoSuit *pSuit = m_player.GetNanoSuit();

	if (m_movement.desiredVelocity.x || m_movement.desiredVelocity.y)
	{	
		float strafeMul = m_params.strafeMultiplier;
		float backwardMul = 1.0f;

		//going back?
		if (m_player.IsPlayer())	//[Mikko] Do not limit backwards movement when controlling AI.
    {
			if (m_movement.desiredVelocity.y < -0.01f)
				backwardMul = m_params.backwardMultiplier;
		}

		NETINPUT_TRACE(m_player.GetEntityId(), backwardMul);
		NETINPUT_TRACE(m_player.GetEntityId(), strafeMul);

		move += m_baseQuat.GetColumn0() * m_movement.desiredVelocity.x * strafeMul * backwardMul;
		move += m_baseQuat.GetColumn1() * m_movement.desiredVelocity.y * backwardMul;
	}

	//ai can set a custom sprint value, so dont cap the movement vector
	if (m_movement.sprint<=0.0f)
	{
		//cap the movement vector to max 1
		float moveModule(move.len());

		//[Mikko] Do not limit backwards movement when controlling AI, otherwise it will disable sprinting.
		if (m_player.IsPlayer())
		{                       //^^[Stas] Added this hack, other clients are not AIs
			if ( moveModule > 1.0f)
				move /= moveModule;
		}

		NETINPUT_TRACE(m_player.GetEntityId(), moveModule);

		//move *= m_animParams.runSpeed/GetStanceMaxSpeed(m_stance);
		float sprintMult = 1.0f;
		bool speedMode = false;

		if(pSuit)
		{
			sprintMult = pSuit->GetSprintMultiplier();
			speedMode = (pSuit->GetMode() == NANOMODE_SPEED)?true:false;
		}

		NETINPUT_TRACE(m_player.GetEntityId(), sprintMult);

		if (m_actions & ACTION_SPRINT && (!speedMode || sprintMult > 1.0f))// && m_player.GetStance() == STANCE_STAND)
			move *= m_params.sprintMultiplier * sprintMult ;

	}

	//player movement dont need the m_frameTime, its handled already in the physics
  float scale = m_player.GetStanceMaxSpeed(m_player.GetStance());
  //gEnv->pRenderer->Draw2dLabel(10, 25, 2.0f, (float*)&ColorF(1,1,1,1), false, "Pocess scale %.4f",scale);

	NETINPUT_TRACE(m_player.GetEntityId(), scale);

	move *= scale;

	//when using gravity boots speed can be slowed down
	if (m_player.GravityBootsOn())
		move *= m_params.gravityBootsMultipler;

  // Danny todo: This is a temporary workaround (but generally better than nothing I think)
  // to stop the AI movement from getting changed beyond recognition under normal circumstances.
  // If the movement request gets modified then it invalidates the prediciton made by AI, and thus
  // the choice of animation/parameters.
		//-> please adjust the prediction
  //if (m_player.IsPlayer())
    AdjustMovementForEnvironment( move );

	//adjust prone movement for slopes
	if (m_player.GetStance()==STANCE_PRONE && move.len2()>0.01f)
	{
		float slopeRatio(1.0f - m_stats.groundNormal.z*m_stats.groundNormal.z);
		slopeRatio *= slopeRatio;

		Vec3 terrainTangent((Vec3(0,0,1)%m_stats.groundNormal)%m_stats.groundNormal);

		if(slopeRatio > 0.5f && move.z > 0.0f)	//emergence stop when going up extreme walls
		{
			if(slopeRatio > 0.7f)
				move *= 0.0f;
			else
				move *= ((0.7f - slopeRatio) * 5.0f);
		}
		else
		{
			move *= 1.0f - min(1.0f,m_params.slopeSlowdown * slopeRatio * max(0.0f,-(terrainTangent * move.GetNormalizedSafe(ZERO))));
			//
			move += terrainTangent * slopeRatio * m_player.GetStanceMaxSpeed(m_player.GetStance());
		}

	}
	
	//only the Z component of the basematrix, handy with flat speeds,jump and gravity
	Matrix33 baseMtxZ(Matrix33(m_baseQuat) * Matrix33::CreateScale(Vec3(0,0,1)));
	
	m_request.type = eCMT_Normal;

	Vec3 jumpVec(0,0,0);
	//jump?
	//FIXME: I think in zeroG should be possible to jump to detach from the ground, for now its like this since its the easiest fix

	bool allowJump = (strcmp(player.GetAnimationGraphState()->QueryOutput("AllowJump"), "1") == 0);
	
	if (player.IsPlayer())
		allowJump = true;

	if (m_movement.jump && allowJump)
	{
 		if (m_stats.jumpLock < 0.001f && m_stats.onGround>0.5f && (m_player.GetStance() != STANCE_PRONE))
		{
			//float verticalMult(max(0.75f,1.0f-min(1.0f,m_stats.flatSpeed / GetStanceMaxSpeed(STANCE_STAND) * m_params.sprintMultiplier)));
			//mul * gravity * jump height
			float mult = 1.0f;
			//this is used to easily find steep ground
			float slopeDelta = (m_stats.inZeroG)? 0.0f : (m_stats.upVector - m_stats.groundNormal).len();

			if (pSuit && pSuit->GetSuitEnergy() > (0.20f * NANOSUIT_ENERGY))
			{
				ENanoMode mode = pSuit->GetMode();
				if(m_stats.inZeroG)
				{
					if(mode == NANOMODE_SPEED)
						jumpVec += m_viewQuat.GetColumn1() * 15.0f * m_stats.mass;
					else if(mode == (ENanoMode)NANOSLOT_STRENGTH)
						jumpVec += m_viewQuat.GetColumn1() * 25.0f * m_stats.mass;
					else
						jumpVec += m_viewQuat.GetColumn1() * 10.0f * m_stats.mass;
				}
				//else if(mode == NANOSLOT_STRENGTH)
				//	mult = MAX(1.0f,(1.5f*MIN(1.0f, pSuit->GetSuitEnergy()/30.0f)))+((m_actions & ACTION_SPRINT)?3.3f:0.0f);
				else if((int)mode == (int)NANOSLOT_STRENGTH /*&& m_stats.speedFlat > 0.5f*/ && slopeDelta < 0.7f)
					mult = (2.7f*MIN(1.0f, pSuit->GetSuitEnergy()/30.0f))+((m_actions & ACTION_SPRINT)?2.0f:0.0f);
			}
			
			if(m_stats.inZeroG)
				m_request.type = eCMT_Impulse;//eCMT_JumpAccumulate;
			else
			{
				m_request.type = eCMT_JumpAccumulate;//eCMT_Fly;
				jumpVec += m_baseQuat.GetColumn2() * cry_sqrtf( 2.0f * m_stats.gravity.len() * m_params.jumpHeight * mult);// * verticalMult;

				//
				//float slopeAngle((gf_PI*0.5f - asin(m_stats.groundNormal.z))/(gf_PI/180.0f));
				float slopeRatio(1.0f - m_stats.groundNormal.z*m_stats.groundNormal.z);
				float jumpMod(jumpVec.len());

 				slopeRatio *= slopeRatio;
				slopeRatio *= 7.0f;
				jumpVec = (jumpVec + m_stats.groundNormal * jumpMod * slopeRatio).GetNormalizedSafe(ZERO) * jumpMod;
			}

			move = m_stats.velocityUnconstrained * 1.3f;
			move -= move * baseMtxZ;

			//with gravity boots on, jumping will disable them for a bit.
			if (m_onGroundWBoots && m_player.GravityBootsOn())
			{
				m_onGroundWBoots = -0.5f;
				jumpVec += m_baseQuat.GetColumn2() * cry_sqrtf( 2.0f * 9.81f * m_params.jumpHeight );
			}
			else
				m_jumped = true;

			// Set ag action 'jumpMP' cleared in CPlayer::UpdateStats()
			player.GetAnimationGraphState()->SetInput(player.GetAnimationGraphState()->GetInputId("Action"), (pSuit->GetMode() == NANOMODE_STRENGTH && (m_actions & ACTION_SPRINT))?"jumpMPStrength":"jumpMP");

			bool bNormalJump = true;
			CPlayer* pPlayer = const_cast<CPlayer*>(&m_player);
			CNanoSuit *pSuit = m_player.GetNanoSuit();
			if(pSuit && pSuit->GetMode() == NANOMODE_STRENGTH)
			{
				if(pSuit->GetSuitEnergy() > (0.20f * NANOSUIT_ENERGY))
				{
					if(m_stats.inZeroG)
						pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-10.0f);
					else if(m_actions & ACTION_SPRINT /*&& m_stats.speedFlat > 0.5f*/ && slopeDelta < 0.7f)
					{
						pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-50.0f);
						pSuit->PlaySound(STRENGTH_JUMP_SOUND, (pSuit->GetSlotValue(NANOSLOT_STRENGTH))*0.01f);

						// Report super jump to AI system.
						if (m_player.GetEntity() && m_player.GetEntity()->GetAI())
							m_player.GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_JUMP, 0);
						CALL_PLAYER_EVENT_LISTENERS(OnSpecialMove(pPlayer, IPlayerEventListener::eSM_StrengthJump));

						// mark as 'un-normal' jump, so normal sound is NOT played and listeners are NOT called below
						bNormalJump = false;
					}
					else //if(slopeDelta < 0.5f)
					{
						//pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-20.0f);
						(const_cast<CPlayer*>(&m_player))->PlaySound(CPlayer::ESound_Jump);
					}
				}
			}

			if (bNormalJump)
			{
				pPlayer->PlaySound(CPlayer::ESound_Jump);
				CALL_PLAYER_EVENT_LISTENERS(OnSpecialMove(pPlayer, IPlayerEventListener::eSM_Jump));
			}

			if (m_jumped)
				m_hasJumped = true;
		}
	}

	//apply movement
	Vec3 desiredVel(0,0,0);

	if (m_stats.onGround)
	{
		desiredVel = move;

		if (m_stats.jumpLock>0.001f)
			desiredVel *= 0.3f;
	}
	else if (move.len2()>0.01f)//"passive" air control, the player can air control as long as it is to decelerate
	{	
 		Vec3 currVelFlat(m_stats.velocity - m_stats.velocity * baseMtxZ);
		Vec3 moveFlat(move - move * baseMtxZ);

		float dot(currVelFlat.GetNormalizedSafe(ZERO) * moveFlat.GetNormalizedSafe(ZERO));
	
		if (dot<-0.001f)
		{
			desiredVel = (moveFlat-currVelFlat)*max(abs(dot)*0.3f,0.1f);
		}
		else
		{
			desiredVel = moveFlat*max(0.5f,1.0f-dot);
		}

		float currVelModSq(currVelFlat.len2());
		float desiredVelModSq(desiredVel.len2());

		if (desiredVelModSq>currVelModSq)
		{
			desiredVel.Normalize();
			desiredVel *= max(1.5f,sqrtf(currVelModSq));
		}
	}

	//be sure desired velocity is flat to the ground
	desiredVel -= desiredVel * baseMtxZ;

	NETINPUT_TRACE(m_player.GetEntityId(), jumpVec);

	m_request.velocity = desiredVel + jumpVec;
	if(!m_stats.inZeroG && m_movement.jump && m_request.velocity.len() > 22.0f)	//cap maximum velocity when jumping (limits speed jump length)
		m_request.velocity = m_request.velocity.normalized() * 22.0f;
	m_velocity.Set(0,0,0);
}

void CPlayerMovement::AdjustMovementForEnvironment( Vec3& move )
{
	//nanoSuit
	if(CNanoSuit *pSuit = m_player.GetNanoSuit())
	{
		float nanoSpeed = pSuit->GetSlotValue(NANOSLOT_SPEED);
		float nanoSpeedMul = 1.0f+nanoSpeed*0.01f;
		if(pSuit->GetMode() == NANOMODE_SPEED)
			nanoSpeedMul *= 1.1f;
		move *= nanoSpeedMul;
		NETINPUT_TRACE(m_player.GetEntityId(), nanoSpeedMul);
	}

	//player is slowed down by carrying heavy objects (max. 25%)
	float massFactor = m_player.GetMassFactor();
	NETINPUT_TRACE(m_player.GetEntityId(), massFactor);
	move *= massFactor;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
void CPlayerMovement::ProcessTurning()
{
	if (m_stats.isRagDoll || (m_player.m_stats.isFrozen.Value() || m_stats.isOnLadder/*&& !m_player.IsPlayer()*/))
		return;

	static const bool ROTATION_AFFECTS_THIRD_PERSON_MODEL = true;
	static const float ROTATION_SPEED = 23.3f;

	Quat entityRot = m_player.GetEntity()->GetWorldRotation().GetNormalized();
	Quat inverseEntityRot = entityRot.GetInverted();

	// TODO: figure out a way to unify this
	if (m_player.IsClient())
	{
		Vec3 right = m_turnTarget.GetColumn0();
		Vec3 up = m_upVector.GetNormalized();
		Vec3 forward = (up % right).GetNormalized();
		m_turnTarget = GetQuatFromMat33( Matrix33::CreateFromVectors(forward%up, forward, up) );

		if (ROTATION_AFFECTS_THIRD_PERSON_MODEL)
		{
			if (m_stats.inAir && m_stats.inZeroG)
			{
				m_turnTarget = m_baseQuat;
				m_request.rotation = inverseEntityRot * m_turnTarget;
			}
			else
			{
				/*float turn = m_movement.deltaAngles.z;
				m_turnTarget = Quat::CreateRotationZ(turn) * m_turnTarget;
				Quat turnQuat = inverseEntityRot * m_turnTarget;

				Vec3 epos(m_player.GetEntity()->GetWorldPos()+Vec3(1,0,1));
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(epos, ColorB(255,255,0,100), epos + m_turnTarget.GetColumn1()*2.0f, ColorB(255,0,255,100));*/

				/*
				float turnAngle = cry_acosf( CLAMP( FORWARD_DIRECTION.Dot(turn * FORWARD_DIRECTION), -1.0f, 1.0f ) );
				float turnRatio = turnAngle / (ROTATION_SPEED * m_frameTime);
				if (turnRatio > 1)
					turnQuat = Quat::CreateSlerp( Quat::CreateIdentity(), turnQuat, 1.0f/turnRatio );
				*/

				//float white[4] = {1,1,1,1};
				//gEnv->pRenderer->Draw2dLabel( 100, 50, 4, white, false, "%f %f %f %f", turnQuat.w, turnQuat.v.x, turnQuat.v.y, turnQuat.v.z );

				//m_request.turn = turnQuat;
				m_request.rotation = inverseEntityRot * m_baseQuat;
				m_request.rotation.Normalize();
			}
		}
	}
	else
	{
		/*Vec3 right = entityRot.GetColumn0();
		Vec3 up = m_upVector.GetNormalized();
		Vec3 forward = (up % right).GetNormalized();
		m_turnTarget = GetQuatFromMat33( Matrix33::CreateFromVectors(forward%up, forward, up) );
		float turn = m_movement.deltaAngles.z;
		if (fabsf(turn) > ROTATION_SPEED * m_frameTime)
			turn = ROTATION_SPEED * m_frameTime * (turn > 0.0f? 1.0f : -1.0f);*/
		if (1)//m_stats.speedFlat>0.5f)
		{
			m_request.rotation = inverseEntityRot * m_baseQuat;//(m_turnTarget * Quat::CreateRotationZ(turn));
			m_request.rotation.Normalize();
		}
		else
			m_request.rotation = inverseEntityRot * Quat::CreateRotationZ(gf_PI);
	}
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------

void CPlayerMovement::ProcessMovementOnLadder(CPlayer &player)
{
 	Vec3 mypos = m_worldPos;
	Vec3 move(m_stats.ladderTop - m_stats.ladderBottom);
	move.NormalizeSafe();

	float topDist = (m_stats.ladderTop-player.GetEntity()->GetWorldPos()).len();
	float bottomDist = (m_stats.ladderBottom-player.GetEntity()->GetWorldPos()).len();

	//Animation and movement synch
	{
		AdjustPlayerPositionOnLadder(player);
		bottomDist +=0.012f;   //Offset to make the anim match better...		(depends on animation and ladder, they should be all exactly the same)
		float animTime = bottomDist - cry_floorf(bottomDist);
		player.EvaluateMovementOnLadder(m_movement.desiredVelocity.y,animTime);
		CPlayer::ELadderDirection eLDir = (m_movement.desiredVelocity.y >= 0.0f ? CPlayer::eLDIR_Up : CPlayer::eLDIR_Down);
		if(!player.UpdateLadderAnimGraph(CPlayer::eLS_Climb,eLDir,animTime))
		{
			return;
		}
	}

	if (player.IsClient() && ((topDist < 1.85f && m_movement.desiredVelocity.y > 0.01f) || (bottomDist < 0.1f && m_movement.desiredVelocity.y < -0.01f)))
	{
		if(m_movement.desiredVelocity.y>0.01f)
		{
 			// check if player can move forward from top of ladder before getting off. If they can't,
 			//	they'll need to strafe / jump off.
 			ray_hit hit;
 			static const int obj_types = ent_static|ent_terrain;
 			static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
 			Vec3 currentPos = player.m_stats.ladderTop;
 			Vec3 newPos = player.m_stats.ladderTop-player.m_stats.ladderOrientation;
 			currentPos.z += 0.5f;
 			newPos.z += 0.5f;
			if(g_pGameCVars->i_debug_ladders !=0)
			{
 				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(currentPos, ColorF(1,1,1,1), newPos, ColorF(1,1,1,1));
			}
 			bool rayHitAny = 0 != gEnv->pPhysicalWorld->RayWorldIntersection( currentPos, newPos-currentPos, obj_types, flags, &hit, 1, player.GetEntity()->GetPhysics() );
  		if (!rayHitAny || abs(hit.n.z) > 0.1f)
			{
				player.RequestLeaveLadder(CPlayer::eLAT_ReachedEnd);
				return;
			}
 			else
			{
 				m_movement.desiredVelocity.y = 0.0f;
				if(g_pGameCVars->i_debug_ladders !=0)
				{
					float white[] = {1,1,1,1};
					gEnv->pRenderer->Draw2dLabel(50,125,2.0f,white,false,"CLAMPING");
				}
			}
		}
		else
		{
			player.RequestLeaveLadder(CPlayer::eLAT_None);
			return;
		}
	}
	//Strafe
	if(m_stats.ladderAction == CPlayer::eLAT_StrafeRight || m_stats.ladderAction == CPlayer::eLAT_StrafeLeft)
	{
		player.RequestLeaveLadder(static_cast<CPlayer::ELadderActionType>(m_stats.ladderAction));
		return;
	}

	if(g_pGameCVars->i_debug_ladders !=0)
	{
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_stats.ladderBottom,0.12f,ColorB(0,255,0,100) );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_stats.ladderTop,0.12f,ColorB(0,255,0,100) );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(player.GetEntity()->GetWorldPos(),0.12f,ColorB(255,0,0,100) );
		float white[4]={1,1,1,1};
		gEnv->pRenderer->Draw2dLabel(50,50,2.0f,white,false,"Top Dist: %f - Bottom Dist: %f - Desired Vel: %f",topDist,bottomDist,m_movement.desiredVelocity.y);
		gEnv->pRenderer->Draw2dLabel(50,75,2.0f,white,false,"Ladder Orientation (%f, %f, %f) - Ladder Up Direction (%f, %f, %f)", m_stats.ladderOrientation.x,m_stats.ladderOrientation.y,m_stats.ladderOrientation.z,m_stats.ladderUpDir.x,m_stats.ladderUpDir.y,m_stats.ladderUpDir.z);
	}

	move *= m_movement.desiredVelocity.y*0.5f;// * (dirDot>0.0f?1.0f:-1.0f) * min(1.0f,fabs(dirDot)*5);

	//cap the movement vector to max 1
	float moveModule(move.len());

	if (moveModule > 1.0f)
		move /= moveModule;

	move *= m_player.GetStanceMaxSpeed(STANCE_STAND)*0.5f;

	player.m_stats.bSprinting = false;		//Manual update here (if not suit doensn't decrease energy and so on...)
	if (m_actions & ACTION_SPRINT)
	{
		if(move.len2()>0.1f)
		{
			move *= m_params.sprintMultiplier;
			player.m_stats.bSprinting = true;
		}
	}
	if(m_actions & ACTION_JUMP)
	{
		player.RequestLeaveLadder(CPlayer::eLAT_Jump);
		move += Vec3(0.0f,0.0f,3.0f);
	}

	if(g_pGameCVars->i_debug_ladders !=0)
	{
		float white[] = {1,1,1,1};
		gEnv->pRenderer->Draw2dLabel(50,100,2.0f,white,false, "Move (%.2f, %.2f, %.2f)", move.x, move.y, move.z);
	}
		
	m_request.type = eCMT_Fly;
	m_request.velocity = move;

	m_velocity.Set(0,0,0);
}

//---------------------------------------------------------
void CPlayerMovement::AdjustPlayerPositionOnLadder(CPlayer &player)
{
	IEntity *pEntity = player.GetEntity();

	if(pEntity)
	{
		//In some cases the rotation is not correct, force it if neccessary
		if(!pEntity->GetRotation().IsEquivalent(m_stats.playerRotation))
			pEntity->SetRotation(Quat(Matrix33::CreateOrientation(-m_stats.ladderOrientation,m_stats.ladderUpDir,g_PI)));

		Vec3 projected = ProjectPointToLine(pEntity->GetWorldPos(),m_stats.ladderBottom,m_stats.ladderTop);
		//Same problem with the position
		if(!pEntity->GetWorldPos().IsEquivalent(projected))
		{
			Matrix34 finalPos = pEntity->GetWorldTM();
			finalPos.SetTranslation(projected);
			pEntity->SetWorldTM(finalPos);
		}
	}
}
