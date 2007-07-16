/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a vtol vehicle movement

-------------------------------------------------------------------------
History:
- 13:03:2006: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"

#include "ICryAnimation.h"
#include "IMovementController.h"

#include "IVehicleSystem.h"
#include "VehicleMovementVTOL.h"
#include "VehicleActionLandingGears.h"

#include "IRenderAuxGeom.h"

#include "GameUtils.h"

//------------------------------------------------------------------------
CVehicleMovementVTOL::CVehicleMovementVTOL()
: m_horizontal(0.0f),
	m_isVTOLMovement(true),
	m_forwardInverseMult(0.2f),
	m_wingsAnimTime(0.0f)
{

}

//------------------------------------------------------------------------
bool CVehicleMovementVTOL::Init(IVehicle* pVehicle, const SmartScriptTable &table)
{
	if (!CVehicleMovementHelicopter::Init(pVehicle, table))
		return false;

	MOVEMENT_VALUE("horizFwdForce", m_horizFwdForce);
	MOVEMENT_VALUE("horizLeftForce", m_horizLeftForce);
	MOVEMENT_VALUE("boostForce", m_boostForce);

	m_playerDampingBase = 0.15f;
	m_playerDampingRotation = 0.3f;
	m_playerDimLowInput = 0.01f;

	m_playerRotationMult.x = 55.0f;
	m_playerRotationMult.y = 40.0f;
	m_playerRotationMult.z = 0.0f;

	m_maxFwdSpeedHorizMode = m_maxFwdSpeed;
	m_maxUpSpeedHorizMode = m_maxUpSpeed;

	m_pWingsAnimation = NULL;
	m_wingHorizontalStateId = InvalidVehicleAnimStateId;
	m_wingVerticalStateId = InvalidVehicleAnimStateId;

	pWingComponentLeft = NULL;
	pWingComponentRight = NULL;

	if (!table->GetValue("timeUntilWingsRotate", m_timeUntilWingsRotate))
		m_timeUntilWingsRotate = 0.65f;

	m_engineUpDir.Set(0.0f, 0.0f, 1.0f);

	if (!table->GetValue("wingsSpeed", m_wingsSpeed))
		m_wingsSpeed = 1.0f;

	char* pWingComponentLeftName = 0;
	if (table->GetValue("WingComponentLeft", pWingComponentLeftName))
		pWingComponentLeft = m_pVehicle->GetComponent(pWingComponentLeftName);

	char* pWingComponentRightName = 0;
	if (table->GetValue("WingComponentRight", pWingComponentRightName))
		pWingComponentRight = m_pVehicle->GetComponent(pWingComponentRightName);

	m_playerDampingBase *= 3.0f;

	m_fwdPID.Reset();
	m_fwdPID.m_kP = 0.66f;
	m_fwdPID.m_kD = 0.2f;
	m_fwdPID.m_kI = 0.0f;

	m_relaxForce = 85.0f;
	m_relaxTime = 0.0f;

	m_playerControls.RegisterValue(&m_forwardAction, false, 0.0f, "forward");
	m_playerControls.RegisterAction(eVAI_MoveForward, CHelicopterPlayerControls::eVM_Positive, &m_forwardAction);
	m_playerControls.RegisterAction(eVAI_MoveBack, CHelicopterPlayerControls::eVM_Negative, &m_forwardAction);

	m_pStabilizeVTOL = gEnv->pConsole->GetCVar("v_stabilizeVTOL");

	return (pWingComponentLeft && pWingComponentRight);
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::PostInit()
{
	CVehicleMovementHelicopter::PostInit();

	m_pWingsAnimation = m_pVehicle->GetAnimation("wings");

	if (m_pWingsAnimation)
	{
		m_wingHorizontalStateId = m_pWingsAnimation->GetStateId("tohorizontal");
		m_wingVerticalStateId = m_pWingsAnimation->GetStateId("tovertical");

		m_pWingsAnimation->ToggleManualUpdate(true);
	}
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::Reset()
{
	CVehicleMovementHelicopter::Reset();

	m_horizontal = 0.0f;
	m_engineUpDir.Set(0.0f, 0.0f, 1.0f);

	m_isVTOLMovement = true;

	m_wingsAnimTime = 0.0f;
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::PreProcessMovement(const float deltaTime)
{
	CVehicleMovementHelicopter::PreProcessMovement(deltaTime);

	if (!m_isVTOLMovement)
		return;
	
	IPhysicalEntity* pPhysics = GetPhysics();
	assert(pPhysics);

	pe_simulation_params paramsSim;
	pPhysics->GetParams(&paramsSim);
	float gravity = abs(paramsSim.gravity.z);

	float vertical = 1.0f - m_horizontal;

	//m_engineForce = max(1.0f, gravity * vertical) * m_enginePower * max(0.25f, vertical);
	m_engineForce = 0.0f;
	m_engineForce += gravity * vertical * m_enginePower;
	m_engineForce += m_horizontal * m_enginePower;
 
	Matrix33 tm(m_pVehicle->GetEntity()->GetWorldTM());
	Ang3 angles = Ang3::GetAnglesXYZ(tm);

 	m_workingUpDir = m_engineUpDir; //Vec3(0.0f, 0.0f, 1.0f);
	
	m_workingUpDir += (vertical * m_rotorDiskTiltScale * Vec3(angles.y, -angles.x, 0.0f));
	m_workingUpDir += (m_horizontal * m_rotorDiskTiltScale * Vec3(0.0f, 0.0f, angles.z));

	m_workingUpDir = tm * m_workingUpDir;
	m_workingUpDir.z += 0.25f;
	m_workingUpDir.NormalizeSafe();

	float strafe = angles.y / DEG2RAD(45.0f);
	strafe = min(1.0f, max(-1.0f, strafe));

	if (m_noHoveringTimer <= 0.0f)
	{
		Vec3 forwardImpulse;

		forwardImpulse = m_currentFwdDir * m_enginePower * m_horizFwdForce * m_horizontal
			* (m_forwardAction + (Boosting() * m_boostForce));

		if (m_forwardAction < 0.0f)
			forwardImpulse *= m_forwardInverseMult;

		forwardImpulse += m_currentUpDir * m_liftAction * m_enginePower * gravity;
		forwardImpulse += m_currentLeftDir * -strafe * m_enginePower * m_horizLeftForce;
		forwardImpulse += m_currentUpDir * gravity * m_horizontal * 0.5f;

		static float horizDamp = 0.25f;
		static float vertDamp = 0.50f;

		m_control.impulse += forwardImpulse;
		m_control.impulse.x -= m_statusDyn.v.x * horizDamp;
		m_control.impulse.y -= m_statusDyn.v.y * horizDamp;
		m_control.impulse.z -= m_statusDyn.v.z * vertDamp;
	}

	m_workingUpDir.z += 0.45f * m_liftAction;
	m_workingUpDir.NormalizeSafe();

	return;
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::ResetActions()
{
	CVehicleMovementHelicopter::ResetActions();

	m_actionPitch = 0.0f;
}

//------------------------------------------------------------------------
bool CVehicleMovementVTOL::StartEngine(EntityId driverId)
{
	if (!CVehicleMovementHelicopter::StartEngine(driverId))
	{
		return false;
	}

	if (IActor* pActor = m_pActorSystem->GetActor(m_actorId))
	{
		if ( pActor->IsPlayer() )
		{
			m_isVTOLMovement = true;
		}
		else
		{
			m_isVTOLMovement = false;
		}
	}

	if (m_pWingsAnimation)
	{
		m_pWingsAnimation->StartAnimation();
		m_pWingsAnimation->ToggleManualUpdate(true);
	}

	return true;
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::StopEngine()
{
	CVehicleMovementHelicopter::StopEngine();

	if (m_pWingsAnimation)
	{
		m_pWingsAnimation->StopAnimation();
	}
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::ProcessActions(const float deltaTime)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	m_playerControls.ProcessActions(deltaTime);

	const float rotationForce = 0.07f;

	Limit(m_forwardAction, -1.0f, 1.0f);
	Limit(m_strafeAction, -1.0f, 1.0f);

	m_actionYaw = 0.0f;

	Vec3 worldPos = m_pEntity->GetWorldPos();

	IPhysicalEntity* pPhysics = GetPhysics();
	pe_status_dynamics dyn;
	pPhysics->GetStatus(&dyn);

	// get the current state

	// roll pitch + yaw

	Matrix34 worldTM;
	if (m_pRotorPart)
		worldTM = m_pRotorPart->GetWorldTM();
	else
		worldTM = m_pEntity->GetWorldTM();

	Vec3 specialPos = worldTM.GetTranslation();
	Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(worldTM));

	Matrix33 tm;
	tm.SetRotationXYZ((angles));

	// +ve pitch means nose up
	const float& currentPitch = angles.x;
	// +ve roll means to the left
	const float& currentRoll = angles.y;
	// +ve direction mean rotation anti-clockwise about the z axis - 0 means along y
	float currentDir = angles.z;

	Vec3 currentFwdDir2D = m_currentFwdDir;
	currentFwdDir2D.z = 0.0f;
	currentFwdDir2D.NormalizeSafe();

	Vec3 currentLeftDir2D(-currentFwdDir2D.y, currentFwdDir2D.x, 0.0f);

	Vec3 currentVel = dyn.v;
	Vec3 currentVel2D = currentVel;
	currentVel2D.z = 0.0f;

	float currentHeight = worldPos.z;
	float currentFwdSpeed = currentVel.Dot(currentFwdDir2D);

	ProcessActions_AdjustActions(deltaTime);

	float damping = m_playerDampingBase;
	float inputMult = m_basicSpeedFraction;

	// desired things
	float turnDecreaseScale = m_yawDecreaseWithSpeed / (m_yawDecreaseWithSpeed + fabs(currentFwdSpeed));
	float turnIncreaseScale = 1.0f - turnDecreaseScale;

	m_desiredDir -= turnDecreaseScale * m_turnAction * m_maxYawRate * deltaTime;

	while (m_desiredDir < -gf_PI)
		m_desiredDir += 2.0f * gf_PI;
	while (m_desiredDir > gf_PI)
		m_desiredDir -= 2.0f * gf_PI;

	while (currentDir < m_desiredDir - gf_PI)
		currentDir += 2.0f * gf_PI;
	while (currentDir > m_desiredDir + gf_PI)
		currentDir -= 2.0f * gf_PI;

	float delta = m_desiredDir - currentDir;

	m_desiredDir = currentDir + delta;

	Vec3 desired_vel2D = 
		currentFwdDir2D * m_forwardAction * m_maxFwdSpeed * inputMult + 
		currentLeftDir2D * m_strafeAction * m_maxLeftSpeed * inputMult;

	// calculate the angle changes

	Vec3 desiredVelChange2D = desired_vel2D - currentVel2D;

	float desiredTiltAngle = m_tiltPerVelDifference * desiredVelChange2D.GetLength();
	Limit(desiredTiltAngle, -m_maxTiltAngle, m_maxTiltAngle);

	float sensivity;
	if (m_pAirControlSensivity)
		sensivity = m_pAirControlSensivity->GetFVal();
	else
		sensivity = 1.0f;

	float goal = abs(m_desiredPitch) + abs(m_desiredRoll);
	goal *= 1.5f;
	Interpolate(m_playerAcceleration, goal, 0.25f, deltaTime);
	Limit(m_playerAcceleration, 0.0f, 5.0f);

	if (m_pInvertPitchVar && m_pInvertPitchVar->GetIVal() == 1)
		m_actionPitch += m_pitchActionPerTilt * m_desiredPitch * (m_playerAcceleration + 1.0f) * sensivity;
	else
		m_actionPitch -= m_pitchActionPerTilt * m_desiredPitch * (m_playerAcceleration + 1.0f) * sensivity;

	float rollAccel = 1.0f;
	if (abs(currentRoll + m_desiredRoll) < abs(currentRoll))
		rollAccel *= 1.25f;

	m_actionRoll += m_pitchActionPerTilt * m_desiredRoll * rollAccel * (m_playerAcceleration + 1.0f) * sensivity;
	Limit(m_actionRoll, -10.0f, 10.0f);
	Limit(m_actionPitch, -10.0f, 10.0f);

	float workingDesiredDir = m_desiredDir;

	// roll as we turn
	if (!m_strafeAction)
	{
		m_actionYaw += m_yawPerRoll * currentRoll;
	}

	if (!iszero(m_turnAction))
	{
		m_actionYaw += (workingDesiredDir - currentDir) * m_yawInputConst;
		m_actionYaw += m_yawInputDamping * (currentDir - m_lastDir) / deltaTime;
		m_actionRoll += m_extraRollForTurn * m_turnAction;

		Limit(m_actionYaw, -10.0f, 10.0f);
	}

	m_desiredDir = currentDir;
	m_lastDir = currentDir;

	float boost = Boosting() ? m_boostMult : 1.0f;

	float liftActionMax = 1.0f;

	if (m_pAltitudeLimitVar)
	{
		float altitudeLimit = m_pAltitudeLimitVar->GetFVal();

		if (!iszero(altitudeLimit))
		{
			float altitudeLowerOffset;

			if (m_pAltitudeLimitLowerOffsetVar)
			{
				float r = 1.0f - min(1.0f, max(0.0f, m_pAltitudeLimitLowerOffsetVar->GetFVal()));
				altitudeLowerOffset = r * altitudeLimit;
			}
			else
				altitudeLowerOffset = altitudeLimit;

			float mult = 1.0f;

			if (currentHeight >= altitudeLimit)
			{
				mult = 0.0f;
			}
			else if (currentHeight >= altitudeLowerOffset)
			{
				float zone = altitudeLimit - altitudeLowerOffset;
				mult = (altitudeLimit - currentHeight) / (zone);
			}

			m_liftAction *= mult;

			if (currentPitch > DEG2RAD(5.0f) && m_forwardAction > 0.0f)
				m_forwardAction *= mult;

			m_desiredHeight = min(altitudeLowerOffset, currentHeight);
		}
	}
	else
	{
		m_desiredHeight = currentHeight;
	}

	if (abs(m_liftAction) > 0.001f)
	{
		m_liftAction = min(liftActionMax, max(-0.2f, m_liftAction));

		m_hoveringPower = (m_powerInputConst * m_liftAction) * boost;
		m_noHoveringTimer = 0.0f;
	}
	else if (!m_isTouchingGround)
	{
		if (m_noHoveringTimer <= 0.0f)
		{
			pe_simulation_params paramsSim;
			pPhysics->GetParams(&paramsSim);
			float gravity = abs(paramsSim.gravity.z);

			float upDirZ = m_workingUpDir.z;

			if (upDirZ > 0.9f)
				upDirZ = 1.0f;

			Vec3& impulse = m_control.impulse;
			impulse += Vec3(0.0f, 0.0f, upDirZ) * gravity;
			impulse.z -= dyn.v.z;
		}
		else
		{
			m_noHoveringTimer -= deltaTime;
		}
	}

	if (m_netActionSync.PublishActions( CNetworkMovementHelicopter(this) ))
		m_pVehicle->GetGameObject()->ChangedNetworkState(eEA_GameClientDynamic);

	if (m_pStabilizeVTOL)
	{
		float stabilizeTime = m_pStabilizeVTOL->GetFVal();

		if (stabilizeTime > 0.0f)
		{
			float pitchTarget;

			if (m_horizontal >= 1.0f)
				pitchTarget = DEG2RAD(3.0f);
			else
				pitchTarget = 0.0f;

			if (abs(m_actionPitch) < 0.001f && abs(m_actionRoll) < 0.001f)
			{
				if (m_relaxTime >= stabilizeTime)
				{
					float relaxRatio = m_relaxTime / stabilizeTime;
					m_actionPitch += (pitchTarget - currentPitch) * deltaTime * m_relaxForce * relaxRatio;
					m_actionRoll += -currentRoll * deltaTime * m_relaxForce * relaxRatio;
				}
				else
				{
					m_relaxTime += deltaTime;
				}
			}
			else
			{
				m_relaxTime = 0.0f;
			}
		}
	}
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::ProcessAI(const float deltaTime)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	if (!m_isVTOLMovement)
	{
		CVehicleMovementHelicopter::ProcessAI(deltaTime);
		return;
	}

	const float maxDirChange = 15.0f;

	// it's useless to progress further if the engine has yet to be turned on
	if (!m_isEnginePowered)
		return;

	m_movementAction.Clear();
	m_movementAction.isAI = true;
	ResetActions();

	// Our current state
	const Vec3 worldPos = m_pEntity->GetWorldPos();
	const Ang3 worldAngles = m_pEntity->GetWorldAngles();
	const Matrix33 worldMat(m_pEntity->GetRotation());

	pe_status_dynamics	status;
	IPhysicalEntity*	pPhysics = m_pEntity->GetPhysics();
	pPhysics->GetStatus(&status);
	const Vec3 currentVel = status.v;
	const Vec3 currentVel2D(currentVel.x, currentVel.y, 0.0f);
	// +ve direction mean rotation anti-clocwise about the z axis - 0 means along y
	float currentDir = worldAngles.z;

	// to avoid singularity
	const Vec3 vWorldDir = worldMat * FORWARD_DIRECTION;
	const Vec3 vWorldDir2D =  Vec3( vWorldDir.x,  vWorldDir.y, 0.0f ).GetNormalizedSafe();

	// Our inputs
	const float desiredSpeed = m_aiRequest.HasDesiredSpeed() ? m_aiRequest.GetDesiredSpeed() : 0.0f;
	const Vec3 desiredMoveDir = m_aiRequest.HasMoveTarget() ? (m_aiRequest.GetMoveTarget() - worldPos).GetNormalizedSafe() : vWorldDir;
	const Vec3 desiredMoveDir2D = Vec3(desiredMoveDir.x, desiredMoveDir.y, 0.0f).GetNormalizedSafe(vWorldDir2D);
	const Vec3 desiredVel = desiredMoveDir * desiredSpeed; 
	const Vec3 desiredVel2D(desiredVel.x, desiredVel.y, 0.0f);
	const Vec3 desiredLookDir = m_aiRequest.HasLookTarget() ? (m_aiRequest.GetLookTarget() - worldPos).GetNormalizedSafe() : desiredMoveDir;
	const Vec3 desiredLookDir2D = Vec3(desiredLookDir.x, desiredLookDir.y, 0.0f).GetNormalizedSafe(vWorldDir2D);

	// Calculate the desired 2D velocity change
	Vec3 desiredVelChange2D = desiredVel2D - currentVel2D;
	float velChangeLength = desiredVelChange2D.GetLength2D();

	bool isLandingMode = false;
	if (m_pLandingGears && m_pLandingGears->AreLandingGearsOpen())
		isLandingMode = true;

	bool isHorizontal = (desiredSpeed >= 5.0f) && (desiredMoveDir.GetLength2D() > desiredMoveDir.z);
	float desiredPitch = 0.0f;
	float desiredRoll = 0.0f;

	float desiredDir = atan2(-desiredLookDir2D.x, desiredLookDir2D.y);

	while (currentDir < desiredDir - gf_PI)
		currentDir += 2.0f * gf_PI;
	while (currentDir > desiredDir + gf_PI)
		currentDir -= 2.0f * gf_PI;

	float diffDir = (desiredDir - currentDir);
	m_actionYaw = diffDir * m_yawInputConst;
	m_actionYaw += m_yawInputDamping * (currentDir - m_lastDir) / deltaTime;
	m_lastDir = currentDir;

	if (isHorizontal && !isLandingMode)
	{
		float desiredFwdSpeed = desiredVelChange2D.GetLength();

		desiredFwdSpeed *= min(1.0f, diffDir / DEG2RAD(maxDirChange));

		if (!iszero(desiredFwdSpeed))
		{
			const Vec3 desiredWorldTiltAxis = Vec3(-desiredVelChange2D.y, desiredVelChange2D.x, 0.0f);
			const Vec3 desiredLocalTiltAxis = worldMat.GetTransposed() * desiredWorldTiltAxis;

			m_forwardAction = m_fwdPID.Update(currentVel.y, desiredLocalTiltAxis.GetLength(), -1.0f, 1.0f);

		float desiredTiltAngle = m_tiltPerVelDifference * desiredVelChange2D.GetLength();
		Limit(desiredTiltAngle, -m_maxTiltAngle, m_maxTiltAngle);

		if (desiredTiltAngle > 0.0001f)
		{
			const Vec3 desiredWorldTiltAxis = Vec3(-desiredVelChange2D.y, desiredVelChange2D.x, 0.0f).GetNormalizedSafe();
			const Vec3 desiredLocalTiltAxis = worldMat.GetTransposed() * desiredWorldTiltAxis;

			Vec3 vVelLocal = worldMat.GetTransposed() * desiredVel;
			vVelLocal.NormalizeSafe();

			float dotup = vVelLocal.Dot(Vec3( 0.0f,0.0f,1.0f ) );
			float currentSpeed = currentVel.GetLength();

			desiredPitch = dotup *currentSpeed / 100.0f;
			desiredRoll = desiredTiltAngle * desiredLocalTiltAxis.y *currentSpeed/30.0f;
		}

		}
	}
	else
	{
		float desiredTiltAngle = m_tiltPerVelDifference * desiredVelChange2D.GetLength();
		Limit(desiredTiltAngle, -m_maxTiltAngle, m_maxTiltAngle);

		if (desiredTiltAngle > 0.0001f)
		{
			const Vec3 desiredWorldTiltAxis = Vec3(-desiredVelChange2D.y, desiredVelChange2D.x, 0.0f).GetNormalizedSafe();
			const Vec3 desiredLocalTiltAxis = worldMat.GetTransposed() * desiredWorldTiltAxis;

			desiredPitch = desiredTiltAngle * desiredLocalTiltAxis.x;
			desiredRoll = desiredTiltAngle * desiredLocalTiltAxis.y;
		}
	}

	float currentHeight = m_pEntity->GetWorldPos().z;
	if ( m_aiRequest.HasMoveTarget() )
	{
		m_hoveringPower = m_powerPID.Update(currentVel.z, desiredVel.z, -1.0f, 4.0f);
		
		//m_hoveringPower = (m_desiredHeight - currentHeight) * m_powerInputConst;
		//m_hoveringPower += m_powerInputDamping * (currentHeight - m_lastHeight) / deltaTime;

		if (isHorizontal)
		{
			if (desiredMoveDir.z > 0.6f || desiredMoveDir.z < -0.85f)
			{
				desiredPitch = max(-0.5f, min(0.5f, desiredMoveDir.z)) * DEG2RAD(35.0f);
				m_forwardAction += abs(desiredMoveDir.z);
			}

			m_liftAction = min(2.0f, max(m_liftAction, m_hoveringPower * 2.0f));
		}
		else
		{
			m_liftAction = 0.0f;
		}
	}
	else
	{
		// to keep hovering at the same place
		m_hoveringPower = m_powerPID.Update(currentVel.z, m_desiredHeight - currentHeight, -1.0f, 1.0f);
		m_liftAction = 0.0f;

		if (m_pVehicle->GetAltitude() > 10.0f)
			m_liftAction = m_forwardAction;
	}

	m_actionPitch += m_pitchActionPerTilt * (desiredPitch - worldAngles.x);
	m_actionRoll += m_pitchActionPerTilt * (desiredRoll - worldAngles.y);

	Limit(m_actionPitch, -1.0f, 1.0f);
	Limit(m_actionRoll, -1.0f, 1.0f);
	Limit(m_actionYaw, -1.0f, 1.0f);

	if (m_horizontal > 0.0001f)
		m_desiredHeight = m_pVehicle->GetEntity()->GetWorldPos().z;

	if (g_pGameCVars->v_profileMovement == 2)
	{
		IRenderer* pRenderer = gEnv->pRenderer;
		Vec3 localAngles = m_pEntity->GetWorldAngles();
		float color[4] = {1,1,1,1};

		pRenderer->Draw2dLabel(5.0f, 325.0f, 1.5f, color, false, "forwardAction: %f", m_forwardAction);
		pRenderer->Draw2dLabel(5.0f, 340.0f, 1.5f, color, false, "desired speed: %f vel: %f, %f %f", desiredSpeed, desiredVel.x, desiredVel.y, desiredVel.z);
		pRenderer->Draw2dLabel(5.0f, 355.0f, 1.5f, color, false, "desiredMoveDir: %f, %f %f", desiredMoveDir.x, desiredMoveDir.y, desiredMoveDir.z);
		pRenderer->Draw2dLabel(5.0f, 370.0f, 1.5f, color, false, "wingsTimer: %f", m_wingsTimer);
	}

	Limit(m_forwardAction, -1.0f, 1.0f);
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::OnEvent(EVehicleMovementEvent event, const SVehicleMovementEventParams& params)
{
	CVehicleMovementHelicopter::OnEvent(event, params);

	if (event == eVME_SetMode)
	{

		bool requestedMovement = (params.iValue == 1);
		if ( requestedMovement != m_isVTOLMovement)
		{
			m_isVTOLMovement = requestedMovement;
			if ( m_isVTOLMovement == true )
				m_pWingsAnimation->ChangeState(m_wingHorizontalStateId);
			else
				m_pWingsAnimation->ChangeState(m_wingVerticalStateId);
		}

	}
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::Update(const float deltaTime)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	CVehicleMovementHelicopter::Update(deltaTime);

	if (!iszero(m_forwardAction))
	{
		m_wingsTimer += deltaTime;
		m_wingsTimer = min(m_wingsTimer, m_timeUntilWingsRotate);

		m_horizontal = 1.0f;

		m_maxFwdSpeed = m_maxFwdSpeedHorizMode * 0.25f;
		m_maxFwdSpeed += m_maxFwdSpeedHorizMode * 0.75f * m_horizontal;

		m_maxUpSpeed = m_maxUpSpeedHorizMode * 0.50f;
		m_maxUpSpeed += m_maxUpSpeedHorizMode * 0.5f * (1.0f - m_horizontal);

		m_engineUpDir.Set(0.0f, 1.0f, 0.0f);
	}
	else
	{
		m_wingsTimer -= deltaTime * 0.65f;
		m_wingsTimer = max(m_wingsTimer, 0.0f);

		m_horizontal = 0.0f;

		m_maxFwdSpeed = m_maxFwdSpeedHorizMode * 0.25f;
		m_maxUpSpeed = m_maxUpSpeedHorizMode;
		
		m_engineUpDir.Set(0.0f, 0.0f, 1.0f);
	}

	if (m_pWingsAnimation)
	{
		Interpolate(m_wingsAnimTime, 1.0f - (m_wingsTimer / m_timeUntilWingsRotate), m_wingsSpeed, deltaTime);
		m_pWingsAnimation->SetTime(m_wingsAnimTime);
	}
}

//------------------------------------------------------------------------
void CVehicleMovementVTOL::UpdateEngine(float deltaTime)
{
	// will update the engine power up to the maximum according to the ignition time

	float damageMult = 0.2f;
	damageMult += (1.0f - min(1.0f, pWingComponentLeft->GetDamageRatio())) * 0.4f;
	damageMult += (1.0f - min(1.0f, pWingComponentRight->GetDamageRatio())) * 0.4f;
	float enginePowerMax = m_enginePowerMax * damageMult; 

	if (m_isEnginePowered && !m_isEngineGoingOff)
	{
		if (m_enginePower < enginePowerMax)
		{
			m_enginePower += deltaTime * (enginePowerMax / m_engineIgnitionTime);
			m_enginePower = min(m_enginePower, enginePowerMax);
		}
		else
		{
			m_enginePower = max(enginePowerMax, m_enginePower);
		}
	}
	else
	{
		if (m_enginePower >= 0.0f)
		{
			float powerReduction = enginePowerMax / m_engineIgnitionTime;
			if (m_damage)
				powerReduction *= 2.0f;

			m_enginePower -= deltaTime * powerReduction;
			m_enginePower = max(m_enginePower, 0.0f);
		}
	}
}

void CVehicleMovementVTOL::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	CVehicleMovementHelicopter::GetMemoryStatistics(s);
}
