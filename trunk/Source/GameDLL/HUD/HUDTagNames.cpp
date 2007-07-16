/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	MP TagNames

-------------------------------------------------------------------------
History:
- 21:05:2007: Created by Julien Darre

*************************************************************************/

#include "StdAfx.h"
#include "GameCVars.h"
#include "GameFlashLogic.h"
#include "GameRules.h"
#include "HUD.h"
#include "HUDTagNames.h"
#include "IUIDraw.h"

//-----------------------------------------------------------------------------------------------------

#define COLOR_DEAD		ColorF(0.4f,0.4f,0.4f)
#define COLOR_ENEMY		ColorF(0.85f,0.0f,0.0f)
#define COLOR_FRIEND	ColorF(0.0353f,0.6235f,0.9137f)

//-----------------------------------------------------------------------------------------------------

CHUDTagNames::CHUDTagNames()
{
	m_pUIDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();

	m_pMPNamesFont = NULL;

	if(gEnv->bMultiplayer)
	{
		m_pMPNamesFont = gEnv->pCryFont->NewFont("MPNames");
		m_pMPNamesFont->Load("fonts/hud.xml");
	}

	// Maximum number of players
	m_tagNamesVector.reserve(32);
}

//-----------------------------------------------------------------------------------------------------

CHUDTagNames::~CHUDTagNames()
{
	SAFE_RELEASE(m_pMPNamesFont);
}

//-----------------------------------------------------------------------------------------------------

const char *CHUDTagNames::GetPlayerRank(EntityId uiEntityId)
{
	const char *szRank = NULL;

	IGameRules *pGameRules = g_pGame->GetGameRules();

	int iPlayerRank = 0;
	if(IScriptTable *pGameRulesTable=pGameRules->GetEntity()->GetScriptTable())
	{
		HSCRIPTFUNCTION pfnGetPlayerRank = NULL;
		if(pGameRulesTable->GetValue("GetPlayerRank",pfnGetPlayerRank) && pfnGetPlayerRank)
		{
			Script::CallReturn(gEnv->pScriptSystem,pfnGetPlayerRank,pGameRulesTable,ScriptHandle(uiEntityId),iPlayerRank);
			gEnv->pScriptSystem->ReleaseFunc(pfnGetPlayerRank);
		}
	}
  
	if(iPlayerRank)
	{
		static string strRank;
		static wstring wRank;
		strRank.Format("@ui_rank_%d",iPlayerRank);
		if(gEnv->pSystem->GetLocalizationManager()->LocalizeLabel(strRank,wRank))
		{
			ConvertWString(wRank,strRank);
		}
		szRank = strRank.c_str();
	}

	return szRank;
}

//-----------------------------------------------------------------------------------------------------
// FIXME: an improvement would be to project on faces of the OBB, it should fix most of the Z fights
//-----------------------------------------------------------------------------------------------------

bool CHUDTagNames::ProjectOnSphere(Vec3 &rvWorldPos,const AABB &rBBox)
{
/*	SAuxGeomRenderFlags oldRF = gEnv->pRenderer->GetIRenderAuxGeom()->GetRenderFlags();
	SAuxGeomRenderFlags newRF;
	newRF.SetAlphaBlendMode(e_AlphaBlended);
	newRF.SetDepthWriteFlag(e_DepthWriteOff);
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(newRF);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere((rBBox.min+rBBox.max)*0.5f,((rBBox.max-rBBox.min)*0.5f).len(),ColorB(255,0,255,64),true);
	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(oldRF);*/

	Vec3 vCamPos = gEnv->pSystem->GetViewCamera().GetPosition();

	Sphere sphere((rBBox.min+rBBox.max)*0.5f,((rBBox.max-rBBox.min)*0.5f).len());

	// Project from helper position to campos
	Lineseg lineSeg(rvWorldPos,vCamPos);
	Vec3 v0,v1;
	if(Intersect::Lineseg_Sphere(lineSeg,sphere,v0,v1) > 1)
	{
		// A result is found, let's use it

		Vec3 vScreenSpace;
		gEnv->pRenderer->ProjectToScreen(v1.x,v1.y,v1.z,&vScreenSpace.x,&vScreenSpace.y,&vScreenSpace.z);

		if(vScreenSpace.z >= 0.0f)
		{
			rvWorldPos = v1;
		}
		else
		{
			return true;
		}
	}

	if((vCamPos-sphere.center).len() <= sphere.radius)
	{
		// We are inside the sphere, we can't project on it, so let's draw on top
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void CHUDTagNames::AddEnemyTagName(EntityId uiEntityId)
{
	for(TEnemyTagNamesList::iterator iter=m_enemyTagNamesList.begin(); iter!=m_enemyTagNamesList.end(); ++iter)
	{
		SEnemyTagName *pActorTagName = &(*iter);
		if(pActorTagName->uiEntityId == uiEntityId)
		{
			// Reset time
			pActorTagName->fSpawnTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
			return;
		}
	}

	SEnemyTagName actorTagName;
	actorTagName.uiEntityId = uiEntityId;
	actorTagName.fSpawnTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
	m_enemyTagNamesList.push_back(actorTagName);
}

//-----------------------------------------------------------------------------------------------------

void CHUDTagNames::DrawTagName(IActor *pActor)
{
	CRY_ASSERT(pActor);
	if(!pActor)
		return;

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	CGameRules *pGameRules = g_pGame->GetGameRules();
	int iClientTeam = pGameRules->GetTeam(pClientActor->GetEntityId());

	if(pActor->GetLinkedVehicle())
		return;
	
	const char *szRank = GetPlayerRank(pActor->GetEntityId());

	IEntity *pEntity = pActor->GetEntity();
	if(!pEntity)
		return;

	char szText[HUD_MAX_STRING_SIZE];
	if(szRank)
	{
		sprintf(szText,"%s %s",szRank,pEntity->GetName());
	}
	else
	{
		sprintf(szText,"%s",pEntity->GetName());
	}

	ICharacterInstance *pCharacterInstance = pEntity->GetCharacter(0);
	if(!pCharacterInstance)
		return;

	ISkeleton *pSkeleton = pCharacterInstance->GetISkeleton();
	if(!pSkeleton)
		return;

	int16 sHeadID = pSkeleton->GetIDByName("Bip01 Head");
	if(-1 == sHeadID)
		return;

	Matrix34 matWorld = pEntity->GetWorldTM() * Matrix34(pSkeleton->GetAbsJQuatByID(sHeadID));

	Vec3 vWorldPos = matWorld.GetTranslation();

	// Who has a bigger head? :)
	vWorldPos.z += 0.4f;

	AABB box; 
	pEntity->GetWorldBounds(box);

	bool bDrawOnTop = false;

	if(ProjectOnSphere(vWorldPos,box))
	{
		bDrawOnTop = true;
	}

	ColorF rgbTagName = COLOR_ENEMY;

	if(0 == iClientTeam)
	{
		if(SAFE_HUD_FUNC_RET(IsFriendlyToClient(pActor->GetEntityId())))
		{
			rgbTagName = COLOR_FRIEND;
		}
	}
	else if(pGameRules->GetTeam(pActor->GetEntityId()) == iClientTeam)
	{
		rgbTagName = COLOR_FRIEND;
	}

	if(pActor->GetHealth() <= 0)
	{
		rgbTagName = COLOR_DEAD;
	}

	m_tagNamesVector.resize(1);

	STagName *pTagName = &m_tagNamesVector[0];

	pTagName->strName			= szText;
	pTagName->vWorld			= vWorldPos;
	pTagName->bDrawOnTop	= bDrawOnTop;
	pTagName->rgb					= rgbTagName;

	DrawTagNames();
}

//-----------------------------------------------------------------------------------------------------

void CHUDTagNames::DrawTagName(IVehicle *pVehicle)
{
	CRY_ASSERT(pVehicle);
	if(!pVehicle)
		return;

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	CGameRules *pGameRules = g_pGame->GetGameRules();
	int iClientTeam = pGameRules->GetTeam(pClientActor->GetEntityId());

	ColorF rgbTagName = COLOR_ENEMY;

	// Driver seat is always 1
	IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(1);
	if(!pVehicleSeat)
		return;

	IVehicleHelper *pVehicleHelper = pVehicleSeat->GetSitHelper();
	if(!pVehicleHelper)
		return;

	Vec3 vWorldPos = pVehicleHelper->GetWorldTM().GetTranslation();

	// Add some offset to be above the driver/pilot
	vWorldPos.z += 1.2f;

	AABB box; 
	pVehicle->GetEntity()->GetWorldBounds(box);

	bool bDrawOnTop = false;

	if(ProjectOnSphere(vWorldPos,box))
	{
		bDrawOnTop = true;
	}

	m_tagNamesVector.resize(0);

	for(int iSeatId=1; iSeatId<pVehicle->GetLastSeatId(); iSeatId++)
	{
		IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(iSeatId);
		if(!pVehicleSeat)
			continue;

		EntityId uiEntityId = pVehicleSeat->GetPassenger();

		IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(uiEntityId);
		if(!pActor)
			continue;

		const char *szRank = GetPlayerRank(uiEntityId);

		IEntity *pEntity = pActor->GetEntity();
		if(!pEntity)
			continue;

		char szText[HUD_MAX_STRING_SIZE];
		if(szRank)
		{
			sprintf(szText,"%s %s",szRank,pEntity->GetName());
		}
		else
		{
			sprintf(szText,"%s",pEntity->GetName());
		}

		if(0 == iClientTeam)
		{
			if(uiEntityId && SAFE_HUD_FUNC_RET(IsFriendlyToClient(uiEntityId)))
			{
				rgbTagName = COLOR_FRIEND;
			}
		}
		else if(uiEntityId && pGameRules->GetTeam(uiEntityId) == iClientTeam)
		{
			rgbTagName = COLOR_FRIEND;
		}
		
		if(pActor->GetHealth() <= 0)
		{
			rgbTagName = COLOR_DEAD;
		}

		m_tagNamesVector.resize(m_tagNamesVector.size()+1);

		STagName *pTagName = &m_tagNamesVector[m_tagNamesVector.size()-1];

		pTagName->strName			= szText;
		pTagName->vWorld			= vWorldPos;
		pTagName->bDrawOnTop	= bDrawOnTop;
		pTagName->rgb					= rgbTagName;
	}

	DrawTagNames();
}

//-----------------------------------------------------------------------------------------------------
// Note: we render in PreUpdate as we MUST not render between PreRender/PostRender block
//-----------------------------------------------------------------------------------------------------

void CHUDTagNames::PreUpdate()
{
	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	CGameRules *pGameRules = g_pGame->GetGameRules();

	if(!pClientActor || !pGameRules || !gEnv->bMultiplayer)
		return;

	int iClientTeam = pGameRules->GetTeam(pClientActor->GetEntityId());

	// Skip enemies, they need to be added only when shot
	int iTeamPlayerCount = pGameRules->GetTeamPlayerCount(iClientTeam);
	for(int iPlayer=0; iPlayer<iTeamPlayerCount; iPlayer++)
	{
		IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pGameRules->GetTeamPlayer(iClientTeam,iPlayer));
		if(!pActor)
			continue;

		// Never display the local player
		if(pActor == pClientActor)
			continue;

		DrawTagName(pActor);
	}

	IVehicleSystem *pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
	if(!pVehicleSystem)
		return;

	IVehicleIteratorPtr pVehicleIter = pVehicleSystem->CreateVehicleIterator();
	while(IVehicle *pVehicle=pVehicleIter->Next())
	{
		SVehicleStatus rVehicleStatus = pVehicle->GetStatus();
		if(0 == rVehicleStatus.passengerCount)
			continue;

		// Skip enemy vehicles, they need to be added only when shot
		bool bEnemyVehicle = true;
		for(int iSeatId=1; iSeatId<pVehicle->GetLastSeatId(); iSeatId++)
		{
			IVehicleSeat *pVehicleSeat = pVehicle->GetSeatById(iSeatId);
			if(!pVehicleSeat)
				continue;

			EntityId uiEntityId = pVehicleSeat->GetPassenger();

			if(0 == iClientTeam)
			{
				if(uiEntityId && SAFE_HUD_FUNC_RET(IsFriendlyToClient(uiEntityId)))
				{
					bEnemyVehicle = false;
				}
			}
			else if(uiEntityId && pGameRules->GetTeam(uiEntityId) == iClientTeam)
			{
				bEnemyVehicle = false;
			}
		}
		if(bEnemyVehicle)
			continue;

		DrawTagName(pVehicle);
	}

	for(TEnemyTagNamesList::iterator iter=m_enemyTagNamesList.begin(); iter!=m_enemyTagNamesList.end(); ++iter)
	{
		SEnemyTagName *pEnemyTagName = &(*iter);
		if(gEnv->pTimer->GetAsyncTime().GetSeconds() >= pEnemyTagName->fSpawnTime+((float) g_pGameCVars->hud_mpNamesDuration))
		{
			// Note: iter=my_list.erase(iter) may not be standard/safe
			TEnemyTagNamesList::iterator iterNext = iter;
			++iterNext;
			m_enemyTagNamesList.erase(iter);
			iter = iterNext;
		}
		else
		{
			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEnemyTagName->uiEntityId);
			if(pActor)
				DrawTagName(pActor);

			IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEnemyTagName->uiEntityId);
			if(pVehicle)
				DrawTagName(pVehicle);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDTagNames::DrawTagNames()
{
	int iTagName = 0;
	for(TTagNamesVector::iterator iter=m_tagNamesVector.begin(); iter!=m_tagNamesVector.end(); ++iter,++iTagName)
	{
		STagName *pTagName = &(*iter);

		const char *szText = pTagName->strName;

		// It's important that the projection is done outside the UIDraw->PreRender/PostRender because of the Set2DMode(true) which is done internally

		Vec3 vScreenSpace;
		gEnv->pRenderer->ProjectToScreen(pTagName->vWorld.x,pTagName->vWorld.y,pTagName->vWorld.z,&vScreenSpace.x,&vScreenSpace.y,&vScreenSpace.z);

		if(vScreenSpace.z < 0.0f || vScreenSpace.z > 1.0f)
		{
			// Ignore out of screen entities
			continue;
		}

		vScreenSpace.x *= gEnv->pRenderer->GetWidth()		* 0.01f;
		vScreenSpace.y *= gEnv->pRenderer->GetHeight()	* 0.01f;

		// Seems that Z is on range [1 .. -1]
		vScreenSpace.z = 1.0f - (vScreenSpace.z * 2.0f);

	/*	if(m_pHUDRadar->m_selectedTeamMates.size())
		{
			for(std::vector<EntityId>::iterator iter=m_pHUDRadar->m_selectedTeamMates.begin(); iter!=m_pHUDRadar->m_selectedTeamMates.end(); ++iter)
			{
				if(uiEntityId == *iter)
				{
					// Teammate is selected in radar, force the visibility of that name					
					bDrawOnTop = true;
					break;
				}
			}
		}*/

		float fDistance = (pTagName->vWorld-gEnv->pSystem->GetViewCamera().GetPosition()).len();

		float fSize = 0.0f;
		float fAlpha = 0.0f;

		float fMinDistance = (float) g_pGameCVars->hud_mpNamesNearDistance;
		float fMaxDistance = (float) g_pGameCVars->hud_mpNamesFarDistance;

		if(fDistance < fMinDistance)
		{
			fAlpha = 1.0f;
		}
		else if(fDistance < fMaxDistance)
		{
			fAlpha = 1.0f - (fDistance - fMinDistance) / (fMaxDistance - fMinDistance);
		}

		if(0.0f == fAlpha)
		{
			continue;
		}

		fAlpha = MIN(fAlpha,0.8f);

		if(fDistance < fMinDistance)
		{
			fSize = 20.0f;
		}
		else if(fDistance < fMaxDistance)
		{
			fSize = 20.0f * (1.0f - (fDistance - fMinDistance) / (fMaxDistance-fMinDistance));
		}

		m_pUIDraw->PreRender();

		float fScaleY = gEnv->pRenderer->GetHeight() / 600.0f;

		fSize *= fScaleY;

		m_pMPNamesFont->UseRealPixels(true);
		m_pMPNamesFont->SetSize(vector2f(fSize,fSize));
		m_pMPNamesFont->SetSameSize(false);

		vector2f vDim = m_pMPNamesFont->GetTextSize(szText);

		float fTextX = vScreenSpace.x - vDim.x * 0.5f;
		float fTextY = vScreenSpace.y - vDim.y * (m_tagNamesVector.size() * 0.5f - iTagName);

		if(pTagName->bDrawOnTop)
		{
			vScreenSpace.z = 1.0f;
		}

		m_pMPNamesFont->SetEffect("simple");
		m_pMPNamesFont->SetColor(ColorF(0,0,0,fAlpha));
		m_pMPNamesFont->DrawString(fTextX+1.0f,fTextY+1.0f,vScreenSpace.z,szText);

		m_pMPNamesFont->SetEffect("default");
		m_pMPNamesFont->SetColor(ColorF(pTagName->rgb.r,pTagName->rgb.g,pTagName->rgb.b,fAlpha));
		m_pMPNamesFont->DrawString(fTextX,fTextY,vScreenSpace.z,szText);

		m_pUIDraw->PostRender();
	}
}

//-----------------------------------------------------------------------------------------------------