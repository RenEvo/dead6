#include "StdAfx.h"
#include "HUD.h"
#include "GameFlashAnimation.h"
#include "IRenderer.h"
#include "../Actor.h"
#include "IGameTokens.h"
#include "Weapon.h"
#include "../GameRules.h"
#include <IMaterialEffects.h>
#include "Game.h"
#include "GameCVars.h"
#include "IMusicSystem.h"
#include "INetwork.h"
#include "HUDVehicleInterface.h"
#include "Player.h"
#include "PlayerInput.h"
#include "IMovieSystem.h"
#include "HUDScopes.h"
#include "HUDCrosshair.h"

void CHUD::QuickMenuSnapToMode(ENanoMode mode)
{
	switch(mode)
	{
	case NANOMODE_SPEED:
		m_fAutosnapCursorRelativeX = 0.0f;
		m_fAutosnapCursorRelativeY = -30.0f;
		break;
	case NANOMODE_STRENGTH:
		m_fAutosnapCursorRelativeX = 30.0f;
		m_fAutosnapCursorRelativeY = 0.0f;
		break;
	case NANOMODE_DEFENSE:
		m_fAutosnapCursorRelativeX = -30.0f;
		m_fAutosnapCursorRelativeY = 0.0f;
		break;
	case NANOMODE_CLOAK:
		m_fAutosnapCursorRelativeX = 20.0f;
		m_fAutosnapCursorRelativeY = 30.0f;
		break;
	default:
		break;
	}
}


void CHUD::AutoSnap()
{
	const float fRadius = 25.0f;
	static Vec2 s_vCursor = Vec2(0,0);
	if(fabsf(m_fAutosnapCursorControllerX)>0.1 || fabsf(m_fAutosnapCursorControllerY)>0.1)
	{
		s_vCursor.x = m_fAutosnapCursorControllerX * 30.0f;
		s_vCursor.y = m_fAutosnapCursorControllerY * 30.0f;
	}
	else
	{
		s_vCursor.x = m_fAutosnapCursorRelativeX;
		s_vCursor.y = m_fAutosnapCursorRelativeY;
	}
	if(m_bOnCircle && s_vCursor.GetLength() < fRadius*0.5f)
	{
		m_fAutosnapCursorRelativeX = 0;
		m_fAutosnapCursorRelativeY = 0;
		m_bOnCircle = false;
	}
	if(s_vCursor.GetLength() > fRadius)
	{
		s_vCursor.NormalizeSafe();
		m_fAutosnapCursorRelativeX = s_vCursor.x*fRadius;
		m_fAutosnapCursorRelativeY = s_vCursor.y*fRadius;
		m_bOnCircle = true;
	}


	const char* autosnapItem = "Center";

	if(m_bOnCircle)
	{
		Vec2 vCursor = s_vCursor;
		vCursor.NormalizeSafe();

		/*				ColorB col(255,255,255,255);
		int iW=m_pRenderer->GetWidth();
		int iH=m_pRenderer->GetHeight();
		m_pRenderer->Set2DMode(true,iW,iH);
		m_pRenderer->GetIRenderAuxGeom()->DrawLine(Vec3(iW/2,iH/2,0),col,Vec3(iW/2+vCursor.x*100,iH/2+vCursor.y*100,0),col,5);
		m_pRenderer->Set2DMode(false,0,0);*/

		float fAngle;
		if(vCursor.y < 0)
		{
			fAngle = RAD2DEG(acosf(vCursor.x));
		}
		else
		{
			fAngle = RAD2DEG(gf_PI2-acosf(vCursor.x));
		}

		char strAngle[32];
		sprintf(strAngle,"%f",-fAngle+90.0f);

		m_animQuickMenu.CheckedSetVariable("Root.QuickMenu.Circle.Indicator._rotation",strAngle);

		if(fAngle >= 340 || fAngle < 30)
		{
			autosnapItem = "Strength";
		}
		else if(fAngle >= 30 && fAngle < 150)
		{
			autosnapItem = "Speed";
		}
		else if(fAngle >= 150 && fAngle < 200)
		{
			autosnapItem = "Defense";
		}
		else if(fAngle >= 200 && fAngle < 270)
		{
			autosnapItem = "Weapon";
		}
		else if(fAngle >= 270 && fAngle < 340)
		{
			autosnapItem = "Cloak";
		}
	}

	m_animQuickMenu.CheckedInvoke("Root.QuickMenu.setAutosnapItem", autosnapItem);
}

void CHUD::UpdateMissionObjectiveIcon(EntityId objective, int friendly, FlashOnScreenIcon iconType)
{
	IEntity *pObjectiveEntity = GetISystem()->GetIEntitySystem()->GetEntity(objective);
	if(!pObjectiveEntity) return;

	AABB box;
	pObjectiveEntity->GetWorldBounds(box);

	Vec3 vWorldPos = box.GetCenter();
	vWorldPos.z += 2.0f;
	Vec3 vEntityScreenSpace;
	m_pRenderer->ProjectToScreen(	vWorldPos.x, vWorldPos.y,	vWorldPos.z, &vEntityScreenSpace.x, &vEntityScreenSpace.y, &vEntityScreenSpace.z);

	CActor *pActor = (CActor*)(gEnv->pGame->GetIGameFramework()->GetClientActor());
	bool bBack = false;
	if (IMovementController *pMV = pActor->GetMovementController())
	{
		SMovementState state;
		pMV->GetMovementState(state);
		Vec3 vLook = state.eyeDirection;
		Vec3 vDir = vWorldPos - pActor->GetEntity()->GetWorldPos();
		float fDot = vLook.Dot(vDir);
		if(fDot<0.0f)
			bBack = true;
	}

	bool bLeft(false), bRight(false), bTop(false), bBottom(false);

	if(vEntityScreenSpace.z > 1.0f && bBack)
	{
		Vec2 vCenter(50.0f, 50.0f);
		Vec2 vTarget(-vEntityScreenSpace.x, -vEntityScreenSpace.y);
		Vec2 vScreenDir = vTarget - vCenter;
		vScreenDir = vCenter + (100.0f * vScreenDir.NormalizeSafe());
		vEntityScreenSpace.y = vScreenDir.y;
		vEntityScreenSpace.x = vScreenDir.x;
	}
	if(vEntityScreenSpace.x < 2.0f)
	{
		bLeft = true;
		vEntityScreenSpace.x = 2.0f;
	}
	if(vEntityScreenSpace.x > 98.0f)
	{
		bRight = true;
		vEntityScreenSpace.x = 98.0f;
	}
	if(vEntityScreenSpace.y < 2.01f)
	{
		bTop = true;
		vEntityScreenSpace.y = 2.0f;
	}
	if(vEntityScreenSpace.y > 97.99f)
	{
		bBottom = true;
		vEntityScreenSpace.y = 98.0f;
	}

	float fRendererWidth	= (float) m_pRenderer->GetWidth();
	float fRendererHeight	= (float) m_pRenderer->GetHeight();

	float fMovieWidth		= (float) m_animMissionObjective.GetFlashPlayer()->GetWidth();
	float fMovieHeight	= (float) m_animMissionObjective.GetFlashPlayer()->GetHeight();

	float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
	float fScaleY = fMovieHeight / 100.0f;

	float fScale = fMovieHeight / fRendererHeight;
	float fUselessSize = fMovieWidth - fRendererWidth * fScale;
	float fHalfUselessSize = fUselessSize * 0.5f;

	if(bLeft && bTop)
	{
		iconType = eOS_TopLeft;
	}
	else if(bLeft && bBottom)
	{
		iconType = eOS_BottomLeft;
	}
	else if(bRight && bTop)
	{
		iconType = eOS_TopRight;
	}
	else if(bRight && bBottom)
	{
		iconType = eOS_BottomRight;
	}
	else if(bLeft)
	{
		iconType = eOS_Left;
	}
	else if(bRight)
	{
		iconType = eOS_Right;
	}
	else if(bTop)
	{
		iconType = eOS_Top;
	}
	else if(bBottom)
	{
		iconType = eOS_Bottom;
	}

#if 0
	// Note: 18 is the size of the box (coming from Flash)
	char strX[32];
	char strY[32];

	sprintf(strX,"%f",vEntityScreenSpace.x*fScaleX+fHalfUselessSize+16.0f);
	sprintf(strY,"%f",vEntityScreenSpace.y*fScaleY);
#endif

	int		iMinDist = g_pGameCVars->hud_onScreenNearDistance;
	int		iMaxDist = g_pGameCVars->hud_onScreenFarDistance;
	float	fMinSize = g_pGameCVars->hud_onScreenNearSize;
	float	fMaxSize = g_pGameCVars->hud_onScreenFarSize;

	CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());

	float	fDist = (vWorldPos-pPlayerActor->GetEntity()->GetWorldPos()).len();
	float fSize = 1.0;
	if(fDist<=iMinDist)
	{
		fSize = fMinSize;
	}
	else if(fDist>=iMaxDist)
	{
		fSize = fMaxSize;
	}
	else if(iMaxDist>iMinDist)
	{
		float fA = ((float)iMaxDist - fDist);
		float fB = (float)(iMaxDist - iMinDist);
		float fC = (fMinSize - fMaxSize);
		fSize = ((fA / fB) * fC) + fMaxSize;
	}
	m_missionObjectiveNumEntries += FillUpMOArray(&m_missionObjectiveValues, objective, vEntityScreenSpace.x*fScaleX+fHalfUselessSize+16.0f, vEntityScreenSpace.y*fScaleY, iconType, friendly, (int)fDist, fSize*fSize);
}

void CHUD::UpdateAllMissionObjectives()
{
	if(m_missionObjectiveValues.size() && (gEnv->bMultiplayer || m_pHUDScopes->IsBinocularsShown()))
	{
		m_animMissionObjective.SetVisible(true);
		m_animMissionObjective.GetFlashPlayer()->SetVariableArray(FVAT_Double, "m_allValues", 0, &m_missionObjectiveValues[0], m_missionObjectiveNumEntries);
		m_animMissionObjective.Invoke("updateMissionObjectives");
	}
	else
	{
		m_animMissionObjective.SetVisible(false);
	}

	m_missionObjectiveNumEntries = 0;
	m_missionObjectiveValues.clear();
}


int CHUD::FillUpMOArray(std::vector<double> *doubleArray, double a, double b, double c, double d, double e, double f, double g)
{
	doubleArray->push_back(a);
	doubleArray->push_back(b);
	doubleArray->push_back(c);
	doubleArray->push_back(d);
	doubleArray->push_back(e);
	doubleArray->push_back(f);
	doubleArray->push_back(g);
	return 7;
}

void CHUD::GrenadeDetector(CPlayer* pPlayerActor)
{
	if(!m_entityGrenadeDectectorId)
		return;

	if(g_pGameCVars->g_difficultyLevel > 3)
		return;

	if(!m_animGrenadeDetector.IsLoaded())
		m_animGrenadeDetector.Reload();

	IEntity *pEntityGrenadeDetector = GetISystem()->GetIEntitySystem()->GetEntity(m_entityGrenadeDectectorId);
	if(NULL == pEntityGrenadeDetector)
		m_entityGrenadeDectectorId = 0;
	else
	{	
		AABB box;
		pEntityGrenadeDetector->GetWorldBounds(box);

		Vec3 vWorldPos = box.GetCenter();
		Vec3 vEntityScreenSpace;
		m_pRenderer->ProjectToScreen(	vWorldPos.x, vWorldPos.y,	vWorldPos.z, &vEntityScreenSpace.x,	&vEntityScreenSpace.y, &vEntityScreenSpace.z);

		if(vEntityScreenSpace.z > 1.0f)
		{
			m_animGrenadeDetector.Invoke("hideGrenadeDetector");
			m_bGrenadeBehind = true;
		}
		else if(m_bGrenadeBehind)
		{
			m_animGrenadeDetector.Invoke("showGrenadeDetector");
			m_bGrenadeBehind = false;
		}
		if(vEntityScreenSpace.x < 3.0f)
		{
			vEntityScreenSpace.x = 3.0f;
			m_animGrenadeDetector.Invoke("morphLeft");
			m_bGrenadeLeftOrRight = true;
		}
		else if(vEntityScreenSpace.x > 97.0f)
		{
			vEntityScreenSpace.x = 97.0f;
			m_animGrenadeDetector.Invoke("morphRight");
			m_bGrenadeLeftOrRight = true;
		}
		else if(m_bGrenadeLeftOrRight)
		{
			m_animGrenadeDetector.Invoke("morphNone");
			m_bGrenadeLeftOrRight = false;
		}

		float fRendererWidth	= (float) m_pRenderer->GetWidth();
		float fRendererHeight	= (float) m_pRenderer->GetHeight();

		float fMovieWidth		= (float) m_animGrenadeDetector.GetFlashPlayer()->GetWidth();
		float fMovieHeight	= (float) m_animGrenadeDetector.GetFlashPlayer()->GetHeight();

		float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
		float fScaleY = fMovieHeight / 100.0f;

		float fScale = fMovieHeight / fRendererHeight;
		float fUselessSize = fMovieWidth - fRendererWidth * fScale;
		float fHalfUselessSize = fUselessSize * 0.5f;

		// Note: 18 is the size of the box (coming from Flash)
		float fBoxSizeX = 18.0f * fMovieHeight / fRendererHeight;
		float fBoxSizeY = 18.0f * fMovieHeight / fRendererHeight;

		char strX[32];
		char strY[32];
		sprintf(strX,"%f",vEntityScreenSpace.x*fScaleX-fBoxSizeX+fHalfUselessSize);
		sprintf(strY,"%f",vEntityScreenSpace.y*fScaleY-fBoxSizeY);

		m_animGrenadeDetector.SetVariable("Root.GrenadeDetect._x",strX);
		m_animGrenadeDetector.SetVariable("Root.GrenadeDetect._y",strY);

		char strDistance[32];
		sprintf(strDistance,"%.2fM",(vWorldPos-pPlayerActor->GetEntity()->GetWorldPos()).len());
		m_animGrenadeDetector.Invoke("setDistance", strDistance);
		m_animGrenadeDetector.Invoke("setGrenadeType", pEntityGrenadeDetector->GetClass()->GetName());
	}
}

void CHUD::IndicateDamage(EntityId weaponId, Vec3 direction, bool onVehicle)
{
	Vec3 vlookingDirection = FORWARD_DIRECTION;
	CGameFlashAnimation* pAnim = NULL;

	CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayerActor)
		return;

	if(IEntity *pEntity = gEnv->pEntitySystem->GetEntity(weaponId))
	{
		if(pEntity->GetClass() == CItem::sGaussRifleClass)
		{
			m_animRadarCompassStealth.Invoke("GaussHit");
			m_animAmmo.Invoke("GaussHit");
			m_animHealthEnergy.Invoke("GaussHit");
		}
	}

	IMovementController *pMovementController = NULL;

	float fFront = 0.0f;
	float fRight = 0.0f;

	if(!onVehicle)
	{
		if(!g_pGameCVars->hud_chDamageIndicator)
			return;
		pMovementController = pPlayerActor->GetMovementController();
		pAnim = m_pHUDCrosshair->GetFlashAnim();
	}
	else if(IVehicle *pVehicle = pPlayerActor->GetLinkedVehicle())
	{
		pMovementController = pVehicle->GetMovementController();
		pAnim = &(m_pHUDVehicleInterface->m_animStats);
	}

	if(pMovementController && pAnim)
	{
		SMovementState sMovementState;
		pMovementController->GetMovementState(sMovementState);
		vlookingDirection = sMovementState.eyeDirection;
	}
	else
		return;

	if(!onVehicle)
	{
		//we use a static/world damage indicator and add the view rotation to the indicator animation now
		fFront = -(Vec3(0,-1,0).Dot(direction));
		fRight = -(Vec3(-1, 0, 0).Dot(direction));
	}
	else
	{
		Vec3 vRightDir = vlookingDirection.Cross(Vec3(0,0,1));
		fFront = -vlookingDirection.Dot(direction);
		fRight = -vRightDir.Dot(direction);
	}

	if(fabsf(fFront) > 0.35f)
	{
		if(fFront > 0)
			pAnim->Invoke("setDamageDirection", 1);
		else
			pAnim->Invoke("setDamageDirection", 3);
	}
	if(fabsf(fRight) > 0.35f)
	{
		if(fRight > 0)
			pAnim->Invoke("setDamageDirection", 2);
		else
			pAnim->Invoke("setDamageDirection", 4);
	}
	if(fFront == 0.0f && fRight == 0.0f)
	{
		pAnim->Invoke("setDamageDirection", 1);
		pAnim->Invoke("setDamageDirection", 2);
		pAnim->Invoke("setDamageDirection", 3);
		pAnim->Invoke("setDamageDirection", 4);
	}

	m_fDamageIndicatorTimer = gEnv->pTimer->GetFrameStartTime().GetSeconds();
}

bool CHUD::ShowLockingBrackets(EntityId p_iObj, std::vector<double> *doubleArray)
{
	float fRendererWidth	= (float) m_pRenderer->GetWidth();
	float fRendererHeight	= (float) m_pRenderer->GetHeight();

	AABB box;
	IEntity* pEntityTargetAutoaim = gEnv->pEntitySystem->GetEntity(p_iObj);
	pEntityTargetAutoaim->GetWorldBounds(box);

	// We should be outside the screen with these values

	float fMinX = +100000.0f;
	float fMinY = +100000.0f;
	float fMinZ = +100000.0f;

	float fMaxX = -100000.0f;
	float fMaxY = -100000.0f;
	float fMaxZ = -100000.0f;

#define GETMINMAX(fX,fY,fZ)\
	{\
	Vec3 vEntityScreenSpace;\
	m_pRenderer->ProjectToScreen(fX,fY,fZ,&vEntityScreenSpace.x,&vEntityScreenSpace.y,&vEntityScreenSpace.z);\
	fMinX = MIN(fMinX,vEntityScreenSpace.x);\
	fMinY = MIN(fMinY,vEntityScreenSpace.y);\
	fMinZ = MIN(fMinZ,vEntityScreenSpace.z);\
	fMaxX = MAX(fMaxX,vEntityScreenSpace.x);\
	fMaxY = MAX(fMaxY,vEntityScreenSpace.y);\
	fMaxZ = MAX(fMaxZ,vEntityScreenSpace.z);\
	}

	GETMINMAX(box.min.x,box.min.y,box.min.z);
	GETMINMAX(box.min.x,box.min.y,box.max.z);
	GETMINMAX(box.min.x,box.max.y,box.min.z);
	GETMINMAX(box.min.x,box.max.y,box.max.z);
	GETMINMAX(box.max.x,box.min.y,box.min.z);
	GETMINMAX(box.max.x,box.min.y,box.max.z);
	GETMINMAX(box.max.x,box.max.y,box.min.z);
	GETMINMAX(box.max.x,box.max.y,box.max.z);

	bool validCoords = fabs(fMinX)<=1000.0 && fabs(fMaxX)<=1000.0;
	validCoords = validCoords && fabs(fMinY)<=1000.0 && fabs(fMaxY)<=1000.0;

	float fMovieWidth		= (float) m_animTargetLock.GetFlashPlayer()->GetWidth();
	float fMovieHeight	= (float) m_animTargetLock.GetFlashPlayer()->GetHeight();

	float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
	float fScaleY = fMovieHeight / 100.0f;

	Vec3 vEntityScreenSpace;
	m_pRenderer->ProjectToScreen(box.GetCenter().x,box.GetCenter().y,box.GetCenter().z,&vEntityScreenSpace.x,&vEntityScreenSpace.y,&vEntityScreenSpace.z);

	float fScale = fMovieHeight / fRendererHeight;
	float fUselessSize = fMovieWidth - fRendererWidth * fScale;
	float fHalfUselessSize = fUselessSize * 0.5f;

	if(validCoords)
	{
		doubleArray->push_back(p_iObj);
		doubleArray->push_back(fMinX*fScaleX+fHalfUselessSize);
		doubleArray->push_back(fMinY*fScaleY);
		doubleArray->push_back(fMaxX*fScaleX+fHalfUselessSize);
		doubleArray->push_back(fMinY*fScaleY);
		doubleArray->push_back(fMinX*fScaleX+fHalfUselessSize);
		doubleArray->push_back(fMaxY*fScaleY);
		doubleArray->push_back(fMaxX*fScaleX+fHalfUselessSize);
		doubleArray->push_back(fMaxY*fScaleY);
		doubleArray->push_back(vEntityScreenSpace.x*fScaleX+fHalfUselessSize);
		doubleArray->push_back(vEntityScreenSpace.y*fScaleY);
		doubleArray->push_back(vEntityScreenSpace.z < 1.0f);
	}

	return validCoords;
}

void CHUD::IndicateHit()
{
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayer)
		return;

	if(!pPlayer->GetLinkedVehicle())
		m_pHUDCrosshair->GetFlashAnim()->Invoke("indicateHit");
	else
	{
		IVehicleSeat *pSeat = pPlayer->GetLinkedVehicle()->GetSeatForPassenger(pPlayer->GetEntityId());
		if(pSeat && !pSeat->IsDriver())
			m_pHUDCrosshair->GetFlashAnim()->Invoke("indicateHit");
		else
			m_pHUDVehicleInterface->m_animMainWindow.Invoke("indicateHit");
	}
}

void CHUD::Targetting(EntityId p_iTarget, bool p_bStatic)
{
	float fRendererWidth	= (float) m_pRenderer->GetWidth();
	float fRendererHeight	= (float) m_pRenderer->GetHeight();

	IEntity* pEntityTargetAutoaim = gEnv->pEntitySystem->GetEntity(m_entityTargetAutoaimId);
	std::vector<double> entityValues;
	if(IsAirStrikeAvailable() && GetScopes()->IsBinocularsShown())
	{
		std::vector<EntityId>::const_iterator it = m_possibleAirStrikeTargets.begin();
		for(; it != m_possibleAirStrikeTargets.end(); ++it)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
			if(pEntity)
				ShowLockingBrackets(*it, &entityValues);
		}
	}
	else
	{
		IEntity* pEntityTargetAutoaim = gEnv->pEntitySystem->GetEntity(m_entityTargetAutoaimId);
		if (pEntityTargetAutoaim == 0)
		{
			m_entityTargetAutoaimId = 0;
		}
		else
		{
			ShowLockingBrackets(m_entityTargetAutoaimId, &entityValues);
		}
	}

	if(!entityValues.empty())
	{
		m_animTargetAutoAim.GetFlashPlayer()->SetVariableArray(FVAT_Double, "m_allValues", 0, &entityValues[0], entityValues.size());
		if(m_pHUDScopes->IsBinocularsShown() || m_pHUDScopes->GetCurrentScope() != CHUDScopes::ESCOPE_NONE || m_entityTargetAutoaimId)
			m_animTargetAutoAim.Invoke("updateLockBrackets");
	}

	// Target lock
	IEntity* pEntityTargetLock = gEnv->pEntitySystem->GetEntity(m_entityTargetLockId);
	if (pEntityTargetLock == 0)
	{
		m_entityTargetLockId = 0;
	}
	else
	{	
		AABB box;
		pEntityTargetLock->GetWorldBounds(box);

		Vec3 vWorldPos = box.GetCenter();
		Vec3 vEntityScreenSpace;
		m_pRenderer->ProjectToScreen(	vWorldPos.x,
			vWorldPos.y,
			vWorldPos.z,
			&vEntityScreenSpace.x,
			&vEntityScreenSpace.y,
			&vEntityScreenSpace.z);

		if(vEntityScreenSpace.z > 1.0f)
		{
			//TODO:hide it, it's behind
		}

		float fMovieWidth		= (float) m_animTargetLock.GetFlashPlayer()->GetWidth();
		float fMovieHeight	= (float) m_animTargetLock.GetFlashPlayer()->GetHeight();

		float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
		float fScaleY = fMovieHeight / 100.0f;

		float fScale = fMovieHeight / fRendererHeight;
		float fUselessSize = fMovieWidth - fRendererWidth * fScale;
		float fHalfUselessSize = fUselessSize * 0.5f;

		char strX[32];
		char strY[32];
		sprintf(strX,"%f",vEntityScreenSpace.x*fScaleX+fHalfUselessSize);
		sprintf(strY,"%f",vEntityScreenSpace.y*fScaleY);

		m_animTargetLock.SetVariable("Root.Cursor._x",strX);
		m_animTargetLock.SetVariable("Root.Cursor._y",strY);
	}

	// Tac lock
	if(m_pWeapon && m_bTacLock)
	{
		Vec3 vAimPos		= (static_cast<CWeapon *>(m_pWeapon))->GetAimLocation();
		Vec3 vTargetPos	= (static_cast<CWeapon *>(m_pWeapon))->GetTargetLocation();

		Vec3 vAimScreenSpace;		
		m_pRenderer->ProjectToScreen(vAimPos.x,vAimPos.y,vAimPos.z,&vAimScreenSpace.x,&vAimScreenSpace.y,&vAimScreenSpace.z);

		Vec3 vTargetScreenSpace;
		m_pRenderer->ProjectToScreen(vTargetPos.x,vTargetPos.y,vTargetPos.z,&vTargetScreenSpace.x,&vTargetScreenSpace.y,&vTargetScreenSpace.z);

		float fMovieWidth		= (float) m_animTacLock.GetFlashPlayer()->GetWidth();
		float fMovieHeight	= (float) m_animTacLock.GetFlashPlayer()->GetHeight();

		float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
		float fScaleY = fMovieHeight / 100.0f;

		float fScale = fMovieHeight / fRendererHeight;
		float fUselessSize = fMovieWidth - fRendererWidth * fScale;
		float fHalfUselessSize = fUselessSize * 0.5f;

		m_animTacLock.SetVariable("AimSpot._x",SFlashVarValue(vAimScreenSpace.x*fScaleX+fHalfUselessSize));
		m_animTacLock.SetVariable("AimSpot._y",SFlashVarValue(vAimScreenSpace.y*fScaleY));

		m_animTacLock.SetVariable("TargetSpot._x",SFlashVarValue(vTargetScreenSpace.x*fScaleX+fHalfUselessSize));
		m_animTacLock.SetVariable("TargetSpot._y",SFlashVarValue(vTargetScreenSpace.y*fScaleY));
	}

	//OnScreenMissionObjective
	float fX(0.0f), fY(0.0f);
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	int team = 0;
	CGameRules *pGameRules = (CGameRules*)(gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules());
	if(pActor && pGameRules)
		team = pGameRules->GetTeam(pActor->GetEntityId());

	if(m_iPlayerOwnedVehicle)
	{
		IEntity *pEntity = GetISystem()->GetIEntitySystem()->GetEntity(m_iPlayerOwnedVehicle);
		if(!pEntity)
		{
			m_iPlayerOwnedVehicle = 0;
		}
		else
		{
			UpdateMissionObjectiveIcon(m_iPlayerOwnedVehicle,1,eOS_Purchase);
		}
	}

	if(!gEnv->bMultiplayer && GetScopes()->IsBinocularsShown() && g_pGameCVars->g_difficultyLevel < 3)
	{
		//draw single player mission objectives
		std::map<EntityId, string>::const_iterator it = m_pHUDRadar->m_missionObjectives.begin();
		std::map<EntityId, string>::const_iterator end = m_pHUDRadar->m_missionObjectives.end();
		for(; it != end; ++it)
		{
			UpdateMissionObjectiveIcon(it->first, 2, eOS_FactoryPrototypes);
		}
	}

/*	if(m_pHUDRadar->m_selectedTeamMates.size())
	{
		std::vector<EntityId>::iterator it = m_pHUDRadar->m_selectedTeamMates.begin();
		for(; it != m_pHUDRadar->m_selectedTeamMates.end(); ++it)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
			if(pEntity)
				UpdateMissionObjectiveIcon(*it,1,eOS_TeamMate);
		}
	}*/

	if(pActor && pGameRules)
	{
		SetTACWeapon(false);

		const std::vector<CGameRules::SMinimapEntity> synchEntities = pGameRules->GetMinimapEntities();
		for(int m = 0; m < synchEntities.size(); ++m)
		{
			bool vehicle = false;
			CGameRules::SMinimapEntity mEntity = synchEntities[m];
			FlashRadarType type = m_pHUDRadar->GetSynchedEntityType(mEntity.type);
			IEntity *pEntity = NULL;
			if(type == ENuclearWeapon)
			{
				if(IItem *pWeapon = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(mEntity.entityId))
				{
					if(EntityId ownerId=pWeapon->GetOwnerId())
					{
						pEntity = gEnv->pEntitySystem->GetEntity(ownerId);

						if (ownerId==g_pGame->GetIGameFramework()->GetClientActorId())
						{
							SetTACWeapon(true);
							pEntity=0;
						}
					}
				}
				else
				{
					pEntity = gEnv->pEntitySystem->GetEntity(mEntity.entityId);
					if (IVehicle *pVehicle=g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(mEntity.entityId))
					{
						vehicle=true;

						if (pVehicle->GetDriver()==g_pGame->GetIGameFramework()->GetClientActor())
						{
							SetTACWeapon(true);
							pEntity=0;
						}
					}
				}
			}

			if(!pEntity) continue;

			float fX(0.0f), fY(0.0f);

			int friendly = m_pHUDRadar->FriendOrFoe(gEnv->bMultiplayer, pActor, team, pEntity, pGameRules);
			if(vehicle)
				UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_TACTank);
			else
				UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_TACWeapon);

			bool yetdrawn = false;
			if(vehicle)
			{
				IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
				if(pVehicle)
				{
					IActor *pDriverActor = pVehicle->GetDriver();
					if(pDriverActor)
					{
						IEntity *pDriver = pDriverActor->GetEntity();
						if(pDriver)
						{
							SFlashVarValue arg[2] = {(int)pEntity->GetId(),pDriver->GetName()};
							m_animMissionObjective.Invoke("setPlayerName", arg, 2);
							yetdrawn = true;
						}
					}
					else
					{
						SFlashVarValue arg[2] = {(int)pEntity->GetId(),"@ui_osmo_NODRIVER"};
						m_animMissionObjective.Invoke("setPlayerName", arg, 2);
						yetdrawn = true;
					}

				}
			}
			if(!yetdrawn)
			{
				SFlashVarValue arg[2] = {(int)pEntity->GetId(),pEntity->GetName()};
				m_animMissionObjective.Invoke("setPlayerName", arg, 2);
			}
		}
	}

	if(pActor)
	{
		std::vector<EntityId>::iterator it = m_pHUDRadar->GetObjectives()->begin();
		for(; it != m_pHUDRadar->GetObjectives()->end(); ++it)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
			if(pEntity)
			{
				int friendly = m_pHUDRadar->FriendOrFoe(gEnv->bMultiplayer, pActor, team, pEntity, pGameRules);
				FlashRadarType type = m_pHUDRadar->ChooseType(pEntity);
				if(friendly==1 && IsUnderAttack(pEntity))
				{
					friendly = 3;
					AddOnScreenMissionObjective(pEntity, friendly);
				}
				else if(HasTACWeapon() && (type == EHeadquarter || type == EHeadquarter2) && friendly == 2)
				{
					// Show TAC Target icon
					AddOnScreenMissionObjective(pEntity, friendly);
				}
				else if(m_bShowAllOnScreenObjectives || m_iOnScreenObjective==(*it) || g_pGameCVars->hud_showAllObjectives)
				{
					AddOnScreenMissionObjective(pEntity, friendly);
				}
			}
		}
	}

	UpdateAllMissionObjectives();
}

bool CHUD::IsUnderAttack(IEntity *pEntity)
{
	if(!pEntity)
		return false;

	IScriptTable *pScriptTable = g_pGame->GetGameRules()->GetEntity()->GetScriptTable();
	if (!pScriptTable)
		return false;

	bool underAttack=false;
	HSCRIPTFUNCTION scriptFuncHelper = NULL;
	if(pScriptTable->GetValue("IsUncapturing", scriptFuncHelper) && scriptFuncHelper)
	{
		Script::CallReturn(gEnv->pScriptSystem, scriptFuncHelper, pScriptTable, ScriptHandle(pEntity->GetId()), underAttack);
		gEnv->pScriptSystem->ReleaseFunc(scriptFuncHelper);
		scriptFuncHelper = NULL;
	}

	return underAttack;
}

void CHUD::AddOnScreenMissionObjective(IEntity *pEntity, int friendly)
{
	FlashRadarType type = m_pHUDRadar->ChooseType(pEntity);
	if(type == EHeadquarter)
		if(HasTACWeapon() && friendly==2)
			UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_HQTarget);
		else
			UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_HQKorean);
	else if(type == EHeadquarter2)
		if(HasTACWeapon() && friendly==2)
			UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_HQTarget);
		else
			UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_HQUS);
	else if(type == EFactoryTank)
		UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_FactoryTank);
	else if(type == EFactoryAir)
		UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_FactoryAir);
	else if(type == EFactorySea)
		UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_FactoryNaval);
	else if(type == EFactoryVehicle)
		UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_FactoryVehicle);
	else if(type == EFactoryPrototype)
		UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_FactoryPrototypes);
	else if(type == EAlienEnergySource)
		UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_AlienEnergyPoint);

	else
	{
		//now spawn points
		std::vector<EntityId> locations;
		CGameRules *pGameRules = (CGameRules*)(gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules());
		pGameRules->GetSpawnGroups(locations);
		for(int i = 0; i < locations.size(); ++i)
		{
			IEntity *pSpawnEntity = gEnv->pEntitySystem->GetEntity(locations[i]);
			if(!pSpawnEntity) continue;
			else if(pSpawnEntity == pEntity)
			{
				UpdateMissionObjectiveIcon(pEntity->GetId(),friendly,eOS_Spawnpoint);
				break;
			}
		}
	}
}

void CHUD::ShowKillAreaWarning(bool active, int timer)
{
	if(active && timer && !m_animKillAreaWarning.IsLoaded())
	{
		m_animDeathMessage.Unload();
		m_animKillAreaWarning.Reload();
		m_animKillAreaWarning.Invoke("showWarning");
	}
	else if(m_animKillAreaWarning.IsLoaded() && (!active || !timer))
	{
		m_animKillAreaWarning.Unload();
		return;
	}

	//set timer
	if (m_animKillAreaWarning.IsLoaded())
	{
		m_animKillAreaWarning.Invoke("setCountdown", timer);
	}
	else if(timer <= 0 && active)
	{
		if(IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor())
		{
			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
			SMFXRunTimeEffectParams params;
			params.pos = pActor->GetEntity()->GetWorldPos();
			TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", "player_boundry_damage");
			pMaterialEffects->ExecuteEffect(id, params);
		}

		m_animDeathMessage.Reload();
		m_animDeathMessage.Invoke("showKillEvent");
	}
}


void CHUD::ShowDeathFX(int type)
{
	if(m_godMode)
		return;

	IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
	if (m_deathFxId != InvalidEffectId)
	{
		pMaterialEffects->StopEffect(m_deathFxId);
		m_deathFxId = InvalidEffectId;
	}

	if (type<=0)
		return;

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();

	if(pActor->GetHealth() <= 0)
		return;

	SMFXRunTimeEffectParams params;
	params.pos = pActor->GetEntity()->GetWorldPos();

	static const char* deathType[] =
	{
		"playerdeath_generic",
		"playerdeath_headshot",
		"playerdeath_melee",
		"playerdeath_freeze"
	};
	
	static const int maxTypes = sizeof(deathType) / sizeof(deathType[0]);
	type = type < 1 ? 1 : ( type <= maxTypes ? type : maxTypes ); // sets values outside [0, maxTypes) to 0  

	m_deathFxId = pMaterialEffects->GetEffectIdByName("player_fx", deathType[type-1]);
	if (m_deathFxId != InvalidEffectId)
		pMaterialEffects->ExecuteEffect(m_deathFxId, params);

	if(IMusicSystem *pMusic = gEnv->pSystem->GetIMusicSystem())
		pMusic->SetMood("low_health");
}

void CHUD::UpdateVoiceChat()
{
	if(!gEnv->bMultiplayer)
		return;

	IVoiceContext *pVoiceContext = gEnv->pGame->GetIGameFramework()->GetNetContext()->GetVoiceContext();

	if(!pVoiceContext || !pVoiceContext->IsEnabled())
		return;

	IActor *pLocalActor = g_pGame->GetIGameFramework()->GetClientActor();
	if(!pLocalActor)
		return;

	INetChannel *pNetChannel = gEnv->pGame->GetIGameFramework()->GetClientChannel();
	if(!pNetChannel)
		return;

	bool someoneTalking = false;
	EntityId localPlayerId = pLocalActor->GetEntityId();
	//CGameRules::TPlayers players;
	//g_pGame->GetGameRules()->GetPlayers(players); - GameRules has 0 players on client
	//for(CGameRules::TPlayers::iterator it = players.begin(), itEnd = players.end(); it != itEnd; ++it)
	IActorIteratorPtr it = g_pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	while (IActor* pActor = it->Next())
	{
		if (!pActor->IsPlayer())
			continue;

		//IEntity *pEntity = gEnv->pEntitySystem->GetEntity(*it);
		IEntity *pEntity = pActor->GetEntity();
		if(pEntity)
		{
			if(pEntity->GetId() == localPlayerId)
			{			
				if(g_pGame->GetIGameFramework()->IsVoiceRecordingEnabled())
				{
					m_animVoiceChat.Invoke("addVoice", pEntity->GetName());
					someoneTalking = true;
				}
			}
			else
			{
				if(pNetChannel->TimeSinceVoiceReceipt(pEntity->GetId()).GetSeconds() < 0.2f)
				{
					m_animVoiceChat.Invoke("addVoice", pEntity->GetName());
					someoneTalking = true;
				}
			}
		}
	}

	if(someoneTalking)
	{
		m_animVoiceChat.SetVisible(true);
		m_animVoiceChat.Invoke("updateVoiceChat");
	}
	else
	{
		m_animVoiceChat.SetVisible(false);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateCrosshairVisibility()
{
	// marcok: don't touch this, please
	if (g_pGameCVars->cl_tryme)
	{
		m_pHUDCrosshair->GetFlashAnim()->Invoke("setVisible", 1);
		return;
	}

	if(!m_pHUDCrosshair->GetFlashAnim()->IsLoaded())
		return;
	if(m_pHUDCrosshair->GetFlashAnim()->GetFlashPlayer()->GetVisible())
		m_pHUDCrosshair->GetFlashAnim()->GetFlashPlayer()->SetVisible(false);

	if(!m_iCursorVisibilityCounter && !m_bAutosnap && !m_bHideCrosshair && !m_bThirdPerson)
	{
		// Do not show crosshair while in vehicle
		if((!m_pHUDVehicleInterface->GetVehicle() && !m_pHUDVehicleInterface->IsParachute()) || m_pHUDVehicleInterface->ForceCrosshair())
		{
			m_pHUDCrosshair->GetFlashAnim()->GetFlashPlayer()->SetVisible(true);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnToggleThirdPerson(IActor *pActor,bool bThirdPerson)
{
	if (!pActor->IsClient())
		return;

	m_bThirdPerson = bThirdPerson;

	if(m_pHUDVehicleInterface && m_pHUDVehicleInterface->GetHUDType()!=CHUDVehicleInterface::EHUD_NONE)
		m_pHUDVehicleInterface->UpdateVehicleHUDDisplay();

	m_pHUDScopes->OnToggleThirdPerson(bThirdPerson);

	UpdateCrosshairVisibility();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowTargettingAI(EntityId id)
{
	if(!g_pGameCVars->hud_chDamageIndicator)
		return;

	if(g_pGameCVars->g_difficultyLevel > 2)
		return;

	EntityId actorID = id;
	/*if(IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(id))
	{
		if(pVehicle->GetDriver())
			actorID = pVehicle->GetDriver()->GetEntityId();
	}*/

	if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorID))
	{
		m_fSetAgressorIcon = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		m_agressorIconID = actorID;
		m_pHUDRadar->AddEntityTemporarily(actorID);
	}
	else if(id == m_agressorIconID)
	{
		m_animTargetter.SetVisible(false);
		m_fSetAgressorIcon = 0.0f;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetOnScreenTargetter()
{
	Vec3 screenPos;
	if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_agressorIconID))
	{
		if(pActor->GetHealth() <= 0 || GetScopes()->IsBinocularsShown() || GetScopes()->GetCurrentScope() != CHUDScopes::ESCOPE_NONE)
		{
			m_animTargetter.SetVisible(false);
			return;
		}

		AABB bbox;
		pActor->GetEntity()->GetWorldBounds(bbox);
		Vec3 agressorPos = bbox.GetCenter();
		m_pRenderer->ProjectToScreen(agressorPos.x,agressorPos.y,agressorPos.z,&screenPos.x,&screenPos.y,&screenPos.z);

		IActor *pPlayer = g_pGame->GetIGameFramework()->GetClientActor();
		if(!pPlayer)
			return;

		if((pActor->GetEntity()->GetWorldPos() - pPlayer->GetEntity()->GetWorldPos()).len() < 6.0f)
		{
			m_animTargetter.SetVisible(false);
			return;
		}

		float dist = (pPlayer->GetEntity()->GetWorldPos() - agressorPos).len();

		float fRendererWidth	= (float) m_pRenderer->GetWidth();
		float fRendererHeight	= (float) m_pRenderer->GetHeight();

		float fMovieWidth		= (float) m_animTargetter.GetFlashPlayer()->GetWidth();
		float fMovieHeight	= (float) m_animTargetter.GetFlashPlayer()->GetHeight();

		float fScaleX = (fMovieHeight / 100.0f) * fRendererWidth / fRendererHeight;
		float fScaleY = fMovieHeight / 100.0f;

		float fScale = fMovieHeight / fRendererHeight;
		float fUselessSize = fMovieWidth - fRendererWidth * fScale;
		float fHalfUselessSize = fUselessSize * 0.5f;

		if(screenPos.z > 1.0f)
		{
			m_animTargetter.SetVisible(false);
			return;
		}
		else
		{
			m_animTargetter.SetVisible(true);
			m_animTargetter.SetVariable("Root._x", SFlashVarValue(screenPos.x*fScaleX+fHalfUselessSize-20.0f)); //offset 16 pixel
			m_animTargetter.SetVariable("Root._y", SFlashVarValue(screenPos.y*fScaleY-20.0f));
			m_animTargetter.Invoke("setRecoil", 30.0f - min(max(0.0f,dist), 30.0f));
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::FadeCinematicBars(int targetVal)
{
	m_animCinematicBar.Reload();

	m_animCinematicBar.SetVisible(true);
	m_animCinematicBar.Invoke("setBarPos", targetVal<<1); // *2, because in flash its percentage of half size!
	m_cineState = eHCS_Fading;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateCinematicAnim(float frameTime)
{
	if (m_cineState == eHCS_None)
		return;

	if(m_animCinematicBar.IsLoaded())
	{
		IFlashPlayer* pFP = m_animCinematicBar.GetFlashPlayer();
		pFP->Advance(frameTime);
		pFP->Render();
		if (pFP->GetVisible() == false)
		{
			m_cineState = eHCS_None;
			m_animCinematicBar.Unload();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateSubtitlesAnim(float frameTime)
{
	UpdateSubtitlesManualRender(frameTime);
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::OnBeginCutScene(IAnimSequence* pSeq, bool bResetFX)
{
	if (pSeq == 0)
		return false;

	gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("cutscene",true);
	gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("in_vehicle_suit_menu",true);
  if(gEnv->pSoundSystem)
	  gEnv->pSoundSystem->Silence(true, false);

	if(GetModalHUD() == &m_animPDA)
		ShowPDA(false);

	int flags = pSeq->GetFlags();
	if (IAnimSequence::IS_16TO9 & flags)
	{
		FadeCinematicBars(g_pGameCVars->hud_panoramicHeight);
	}

	if(IAnimSequence::NO_HUD & flags)
	{
		m_cineHideHUD = true;
	}

	if(IAnimSequence::NO_PLAYER & flags)
	{
		if (CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor()))
		{
			if (CPlayer* pPlayer = static_cast<CPlayer*> (pPlayerActor))
			{
				if (SPlayerStats* pActorStats = static_cast<SPlayerStats*> (pPlayer->GetActorStats()))
					pActorStats->spectatorMode = CActor::eASM_Cutscene;	// moved up to avoid conflict with the MP spectator modes
				pPlayer->Draw(false);
				if (pPlayer->GetPlayerInput())
					pPlayer->GetPlayerInput()->Reset();
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::OnEndCutScene(IAnimSequence* pSeq)
{
	if (pSeq == 0)
		return false;

	gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("cutscene",false);
	gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("in_vehicle_suit_menu", false);

	int flags = pSeq->GetFlags();
	if (IAnimSequence::IS_16TO9 & flags)
	{
		FadeCinematicBars(0);
	}

	if(IAnimSequence::NO_HUD & flags)
		m_cineHideHUD = false;

	if(IAnimSequence::NO_PLAYER & flags)
	{
		if (CActor *pPlayerActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor()))
		{
			if (CPlayer* pPlayer = static_cast<CPlayer*> (pPlayerActor))
			{
				if (SPlayerStats* pActorStats = static_cast<SPlayerStats*> (pPlayer->GetActorStats()))
					pActorStats->spectatorMode = CActor::eASM_None;
				pPlayer->Draw(true);
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::OnCameraChange(const SCameraParams& cameraParams)
{
	return true;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnPlayCutSceneSound(IAnimSequence* pSeq, ISound* pSound)
{
	if (m_hudSubTitleMode == eHSM_CutSceneOnly)
	{
		if (pSound && pSound->GetFlags() & FLAG_SOUND_VOICE)
			ShowSubtitle(pSound, true);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetSubtitleMode(HUDSubtitleMode mode)
{
	m_hudSubTitleMode = mode;
	ISubtitleManager* pSubtitleManager = g_pGame->GetIGameFramework()->GetISubtitleManager();
	if (pSubtitleManager == 0)
		return;
	if (m_hudSubTitleMode == eHSM_Off || m_hudSubTitleMode == eHSM_CutSceneOnly)
	{
		pSubtitleManager->SetEnabled(false);
		pSubtitleManager->SetHandler(0);
		if (m_hudSubTitleMode == eHSM_Off)
		{
			m_subtitleEntries.clear();
			m_bSubtitlesNeedUpdate = true;
		}
	}
	else // mode == eHSM_All
	{
		pSubtitleManager->SetEnabled(true);
		pSubtitleManager->SetHandler(this);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowProgress(int progress, bool init /* = false */, int posX /* = 0 */, int posY /* = 0 */, const char *text, bool topText)
{
	if(init)
	{
		if(!m_animProgress.IsLoaded())
			m_animProgress.Load("Libs/UI/HUD_ProgressBar.gfx", eGFD_Center, eFAF_Visible);

		m_animProgress.Invoke("showProgressBar", true);
		const wchar_t* localizedText = LocalizeWithParams(text, true);

		SFlashVarValue args[2] = {localizedText, topText ? 1 : 2};
		m_animProgress.Invoke("setText", args, 2);
		SFlashVarValue pos[2] = {posX*1024/800, posY*768/512};
		m_animProgress.Invoke("setPosition", pos, 2);
	}
	else if(progress < 0 && m_animProgress.IsLoaded())
		m_animProgress.Unload();

	if(m_animProgress.IsLoaded())
		m_animProgress.Invoke("setProgressBar", progress);
}

void CHUD::FakeDeath(bool revive)
{
	CPlayer *pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer->IsGod() || gEnv->bMultiplayer)
		return;

	if(revive)
	{
		float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		float diff = now - m_fPlayerRespawnTimer;
		
		if(diff > -3.0f && (now - m_fLastPlayerRespawnEffect > 0.5f))
		{
			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
			SMFXRunTimeEffectParams params;
			params.pos = pPlayer->GetEntity()->GetWorldPos();
			TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", "player_damage_armormode");
			pMaterialEffects->ExecuteEffect(id, params);
			m_fLastPlayerRespawnEffect = now;
		}
		else if(diff > 0.0)
		{
			if(m_bRespawningFromFakeDeath)
			{
				pPlayer->StandUp();
				pPlayer->Revive(false);
				RebootHUD();
				pPlayer->HolsterItem(false);
				if (g_pGameCVars->g_godMode == 3)
				{
					g_pGame->GetGameRules()->PlayerPosForRespawn(pPlayer, false);
				}
				m_fPlayerRespawnTimer = 0.0f;
				//if (IUnknownProxy * pProxy = pPlayer->GetEntity()->GetAI()->GetProxy())
				//	pProxy->EnableUpdate(true); //seems not to work
				pPlayer->CreateScriptEvent("cloaking", 0);
				if (pPlayer->GetEntity()->GetAI())
					gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1, "OnNanoSuitUnCloak",pPlayer->GetEntity()->GetAI());
				m_bRespawningFromFakeDeath = false;
				DisplayFlashMessage(" ",2);
			}
			else
			{
				m_fPlayerRespawnTimer = 0.0f;
				pPlayer->SetHealth(0);
				pPlayer->CreateScriptEvent("kill",0);
			}
		}
		else
		{
			char text[256];
			char seconds[10];
			sprintf(seconds,"%i",(int)(fabsf(diff)));
			if(m_bRespawningFromFakeDeath)
				sprintf(text,"@ui_respawn_counter");
			else
				sprintf(text,"@ui_revive_counter");
			DisplayFlashMessage(text, 2, ColorF(1.0f,0.0f,0.0f), true, seconds);
		}
	}
	else if(!m_fPlayerRespawnTimer)
	{
		if(pPlayer && (g_pGameCVars->g_playerRespawns > 0 || g_pGameCVars->g_godMode == 3))
		{
			g_pGameCVars->g_playerRespawns--;
			if (g_pGameCVars->g_playerRespawns < 0)
				g_pGameCVars->g_playerRespawns = 0;

			pPlayer->HolsterItem(true);
			pPlayer->Fall(Vec3(0,0,0), true);
			pPlayer->SetDeathTimer();
			//if (IUnknownProxy * pProxy = pPlayer->GetEntity()->GetAI()->GetProxy())
			//	pProxy->EnableUpdate(false); //seems not to work - cloak instead
			pPlayer->CreateScriptEvent("cloaking", 1);
			if (pPlayer->GetEntity()->GetAI())
				gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1, "OnNanoSuitCloak",pPlayer->GetEntity()->GetAI());

			ShowDeathFX(0);
			GetRadar()->Reset();
			BreakHUD(2);
			m_fPlayerRespawnTimer = gEnv->pTimer->GetFrameStartTime().GetSeconds() + 10.0f;
			m_fLastPlayerRespawnEffect = 0.0f;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowDataUpload(bool active)
{
	if(active)
	{
		if(!m_animDataUpload.IsLoaded())
		{
			m_animDataUpload.Load("Libs/UI/HUD_Recording.gfx", eGFD_Right);
			m_animDataUpload.Invoke("showUplink", true);
		}
	}
	else
	{
		if(m_animDataUpload.IsLoaded())
			m_animDataUpload.Unload();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowSpectate(bool active)
{
	if(active)
		m_animSpectate.Load("Libs/UI/HUD_Spectate.gfx", eGFD_Center, eFAF_Visible|eFAF_ManualRender);
	else
		m_animSpectate.Unload();
}
