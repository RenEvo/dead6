/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: HUD radar

-------------------------------------------------------------------------
History:
- 2006/2007: Jan Müller

*************************************************************************/
#include "StdAfx.h"
#include "HUDRadar.h"
#include "IUIDraw.h"
#include "IGameTokens.h"
#include "Actor.h"
#include "Weapon.h"
#include "HUD.h"
#include "Game.h"
#include "IWorldQuery.h"
#include "../GameRules.h"
#include "IVehicleSystem.h"
#include "IAIGroup.h"

#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "GameCVars.h"

#include "HUD/HUDPowerStruggle.h"
#include "HUD/HUDScopes.h"

#define RANDOM() ((((float)cry_rand()/(float)RAND_MAX)*2.0f)-1.0f)

const static float fRadarSizeOverTwo = 47.0f;
const static float fEntitySize = 4.0f;
const static float fEntityMaxDistance = 47.0f - 4.0f;

//-----------------------------------------------------------------------------------------------------

CHUDRadar::CHUDRadar()
{
	m_fX = 93.75f; // RadarCenterX in Flash * 800 (CryEngineRefSizeX) / 1024 (FlashRefSizeX)
	m_fY = 525.0f; // RadarCenterX in Flash * 600 (CryEngineRefSizeY) / 768  (FlashRefSizeY)

	m_radarDotSizes[0] = 1.3f;
	m_radarDotSizes[1] = 2.0f;
	m_radarDotSizes[2] = 2.5f;
	m_radarDotSizes[3] = 3.0f;	//this is currently used as a multiplier for the hunter

	m_fTime = 0.0f;

	m_taggedPointer = 0;
	ResetTaggedEntities();

	m_soundIdCounter = unsigned(1<<26);

	m_iTextureIDEnemy = gEnv->pGame->GetIGameFramework()->GetIUIDraw()->CreateTexture("Textures\\Gui\\HUD\\enemy_triangle.dds");
	m_iTextureIDWaypoint = gEnv->pGame->GetIGameFramework()->GetIUIDraw()->CreateTexture("Textures\\Gui\\HUD\\enemy_circle.dds");
	m_iTextureIDPlayer = gEnv->pGame->GetIGameFramework()->GetIUIDraw()->CreateTexture("Textures\\Gui\\HUD\\arrow.dds");
	m_iTextureIDVehicle	= gEnv->pGame->GetIGameFramework()->GetIUIDraw()->CreateTexture("Textures\\Gui\\HUD\\vehicle.dds");
	m_iTextureIDAirVehicle = gEnv->pGame->GetIGameFramework()->GetIUIDraw()->CreateTexture("Textures\\Gui\\HUD\\arrow2.dds");
	m_iTextureIDSound	= gEnv->pGame->GetIGameFramework()->GetIUIDraw()->CreateTexture("Textures\\Gui\\HUD\\circle.dds");

	m_fLastViewRot = m_fLastCompassRot = m_fLastStealthValue = m_fLastStealthValueStatic = m_fLastFov = -9999.99f;

	m_lookAtObjectID = 0;
	m_lookAtTimer = 0.0f;
	m_scannerObjectID = 0;
	m_scannerTimer = 0.0f;
	m_scannerGatherTimer = 0.50f;
	m_startBroadScanTime = .0f;
	m_lastScan = 0.0f;
	m_jammingValue = 0.0f;

	m_fPDAZoomFactor = 1.0;
	m_fPDATempZoomFactor = 1.0;
	m_vPDAMapTranslation = Vec2(0,0);
	m_vPDATempMapTranslation = Vec2(0,0);
	m_bDragMap = false;

	m_miniMapStartX = m_miniMapStartY = 0;
	m_miniMapEndX = m_miniMapEndY = 2048;
	m_mapGridSizeX = m_mapGridSizeY = 256.0f;

	m_renderMiniMap = false;

	m_bAbsoluteMode = false;

	m_bsRadius = 0.0f;
	m_bsUseParameter = m_bsKeepEntries = false;
	m_bsPosition = Vec3(0,0,0);

	m_flashPDA = NULL;
	m_flashRadar = NULL;
	m_iMultiplayerEnemyNear = 0;

	m_pLevelData = NULL;

	//save some classes for comparison
	m_pVTOL = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "US_vtol" );
	m_pHeli = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_helicopter" );
	m_pHunter = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Hunter" );
	m_pWarrior = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Alien_warrior" );
	m_pAlien = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Alien" );
	m_pTrooper = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Trooper" );
	m_pPlayerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Player" );
	m_pGrunt = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Grunt" );
	m_pScout = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Scout" );
	m_pTankUS = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "US_tank" );
	m_pTankA = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_tank" );
	m_pLTVUS = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "US_ltv" );
	m_pLTVA = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_ltv" );
	m_pAAA = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_aaa" );
	m_pTruck = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_truck" );
	m_pAPCUS = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "US_apc" );
	m_pAPCA = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_apc" );
	m_pBoatCiv = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Civ_speedboat" );
	m_pHover = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "US_hovercraft" );
	m_pBoatUS = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "US_smallboat" );
	m_pBoatA = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Asian_patrolboat" );
	m_pCarCiv = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Civ_car1" );
	m_pParachute = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Parachute" );
}

//-----------------------------------------------------------------------------------------------------

CHUDRadar::~CHUDRadar()
{
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::AddTaggedEntity(EntityId iEntityId)
{
	m_taggedEntities[m_taggedPointer] = iEntityId;
	m_taggedPointer ++;
	if(m_taggedPointer > NUM_TAGGED_ENTITIES)
		m_taggedPointer = 0;
}

void CHUDRadar::AddEntityToRadar(EntityId id)
{
	if (!IsOnRadar(id, 0, m_entitiesOnRadar.size()-1))
	{
		AddToRadar(id);
		g_pGame->GetHUD()->OnEntityAddedToRadar(id);
	}
}

void CHUDRadar::AddEntityTemporarily(EntityId id, float time)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
	if(pEntity)
		ShowEntityTemporarily(ChooseType(pEntity, true), id, time);
}

void CHUDRadar::AddStoryEntity(EntityId id, FlashRadarType type /* = EWayPoint */, const char* text /* = NULL */)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
	if(pEntity)
	{
		std::vector<TempRadarEntity>::iterator it = m_storyEntitiesOnRadar.begin();
		std::vector<TempRadarEntity>::iterator end = m_storyEntitiesOnRadar.end();
		for(; it != end; ++it)
			if((*it).m_id == id)
				return;
		TempRadarEntity temp(id, type, 0.0f, text);
		m_storyEntitiesOnRadar.push_back(temp);
	}
}

void CHUDRadar::RemoveStoryEntity(EntityId id)
{
	std::vector<TempRadarEntity>::iterator it = m_storyEntitiesOnRadar.begin();
	std::vector<TempRadarEntity>::iterator end = m_storyEntitiesOnRadar.end();
	for(; it != end; ++it)
	{
		if((*it).m_id = id)
		{
			m_storyEntitiesOnRadar.erase(it);
			return;
		}
	}
}

void CHUDRadar::ShowSoundOnRadar(Vec3 pos, float intensity)
{
	intensity = std::min(8.0f, intensity); //limit the size of the sound on radar ...
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	m_soundsOnRadar.push_back(RadarSound(pos, intensity, m_soundIdCounter++));

	if(m_soundIdCounter > unsigned(1<<29))
		m_soundIdCounter = unsigned(1<<26);
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::RemoveFromRadar(EntityId id)
{
	{
		std::deque<RadarEntity>::iterator it = m_entitiesOnRadar.begin();
		std::deque<RadarEntity>::iterator end = m_entitiesOnRadar.end();
		for(; it != end; ++it)
		{
			if((*it).m_id == id)
			{
				m_entitiesOnRadar.erase(it);
				return;
			}
		}
	}
	{
		for(int i = 0; i < NUM_TAGGED_ENTITIES; ++i)
		{
			if(m_taggedEntities[i] == id)
			{
				m_taggedEntities[i] = 0;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::PreUpdate()
{
	//don't do anything because its already too late. it would lag.
}

template <typename T, EFlashVariableArrayType flashType, int NUM_VALS> struct ArrayFillHelper
{
	ArrayFillHelper(IFlashPlayer* pFlashPlayer, const char* varName) : m_pFlashPlayer(pFlashPlayer), m_varName(varName), m_nIndex(0), m_nValues(0)
	{
	}

	~ArrayFillHelper()
	{
		Flush();
	}

	ILINE void push_back(const T& v)
	{
		m_values[m_nValues++] = v;
		if (m_nValues == NUM_VALS)
			Flush();
	}

	void Flush()
	{
		if (m_nValues > 0)
		{
			m_pFlashPlayer->SetVariableArray(flashType, m_varName, m_nIndex, m_values, m_nValues);
			m_nIndex+=m_nValues;
			m_nValues = 0;
		}
	}

	IFlashPlayer* m_pFlashPlayer;
	const char* m_varName;
	int m_nIndex;
	int m_nValues;
	T m_values[NUM_VALS];
};


template<typename T, EFlashVariableArrayType flashType, int NUM_VALS>
int FillUpDoubleArray(ArrayFillHelper<T,flashType,NUM_VALS> *fillHelper,
											double a, double b, double c, double d,
											double e, double f, double g = 100.0f, double h = 100.0f, double i = 0.0f, double j = 0.0f)
{
	fillHelper->push_back(a);
	fillHelper->push_back(b);
	fillHelper->push_back(c);
	fillHelper->push_back(d);
	fillHelper->push_back(e);
	fillHelper->push_back(f);
	fillHelper->push_back(g);
	fillHelper->push_back(h);
	fillHelper->push_back(i);
	fillHelper->push_back(j);
	return 10;
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::OnUpdate(float fDeltaTime,float fFadeValue)
{
	FUNCTION_PROFILER(GetISystem(),PROFILE_GAME);

	if(!m_flashRadar)
		return; //we require the flash radar now

	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if (!pActor)
		return;
	if(pActor->GetHealth() <= 0)
	{
		if(gEnv->bMultiplayer)
		{
			//render the mini map
			if(m_renderMiniMap)
				RenderMapOverlay();
		}
		return;
	}

	if(!m_flashRadar->IsAvailable("setObjectArray"))
		return;

	//double array buffer for flash transfer optimization
	ArrayFillHelper<double, FVAT_Double, 100> entityValues (m_flashRadar->GetFlashPlayer(), "m_allValues");
	int numOfValues = 0;

	bool absoluteMode = (g_pGameCVars->hud_radarAbsolute)?true:false;
	float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	float fCos	= cosf(m_fTime); 
	float fRadius = 75.0f;

	//check for a broadscan:
	if(m_startBroadScanTime && now - m_startBroadScanTime > 1.5f)
		StartBroadScan();

	m_fTime += fDeltaTime * 3.0f;
	IRenderer *pRenderer = gEnv->pRenderer;
	float fWidth43 = pRenderer->GetHeight()*1.333f;
	float widthScale = 1.0f; //fWidth43/(float)pRenderer->GetWidth();
	const float newX = m_fX*widthScale;
	float lowerBoundX = newX - fRadarSizeOverTwo;	//used for flash radar position computation

	CPlayer *pPlayer = static_cast<CPlayer*> (pActor);
	IEntity *pPlayerEntity = pActor->GetEntity();

	IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();

	Matrix34	playerViewMtxInverted = pPlayer->GetViewMatrix().GetInverted();
	Vec3			playerViewMtxTranslation = pPlayer->GetViewMatrix().GetTranslation();

	//**********************update flash radar values
	{
		float fStealthValue = 0;
		float fStealthValueStatic = 0;

		if(!gEnv->bMultiplayer)
		{
			//interesting area is currently between 20 and 80 ... walking and running is both 100 when standing
			fStealthValue = gEnv->pAISystem->GetDetectionValue(NULL) * 100.0f;
			fStealthValueStatic = gEnv->pAISystem->GetAmbientDetectionValue(NULL) * 100.0f;
		}
		else if(m_iMultiplayerEnemyNear)
		{
			fStealthValueStatic = fStealthValue = m_iMultiplayerEnemyNear * 10.0f;
			m_iMultiplayerEnemyNear = 0;
		}

		if(m_fLastStealthValue != fStealthValue)
		{
			m_flashRadar->CheckedInvoke("Root.RadarCompassStealth.StealthBar.gotoAndStop", fStealthValue);
			m_fLastStealthValue = fStealthValue;
		}
		if(m_fLastStealthValueStatic != fStealthValueStatic)
		{
			m_fLastStealthValueStatic = fStealthValueStatic;
			m_flashRadar->CheckedInvoke("Root.RadarCompassStealth.StealthExposure.gotoAndStop", fStealthValueStatic);
		}

		if(absoluteMode)
		{
			float viewRot = RAD2DEG(pActor->GetAngles().z);
			if(m_fLastViewRot != viewRot)
			{
				m_flashRadar->Invoke("setView", viewRot);
				m_fLastViewRot = viewRot;
			}
			if(m_fLastCompassRot != 0.0f)
			{
				m_flashRadar->Invoke("setCompassRotation", "0.0");
				m_fLastCompassRot = 0.0f;
			}
		}
		else
		{
			if(m_fLastViewRot != 0.0f)
			{
				m_flashRadar->Invoke("setView", 0.0f);
				m_fLastViewRot = 0.0f;
			}
			float fCompass = pActor->GetAngles().z;
#define COMPASS_EPSILON (0.01f)
			if(	m_fLastCompassRot <= fCompass-COMPASS_EPSILON ||
					m_fLastCompassRot >= fCompass+COMPASS_EPSILON)
			{
				char szCompass[HUD_MAX_STRING_SIZE];
				sprintf(szCompass,"%f",fCompass*180.0f/gf_PI-90.0f);
				m_flashRadar->Invoke("setCompassRotation", szCompass);
				m_fLastCompassRot = fCompass;
			}
		}

		float fFov = g_pGameCVars->cl_fov * pActor->GetActorParams()->viewFoVScale;
		if(m_fLastFov != fFov)
		{
			m_flashRadar->Invoke("setFOV", fFov*0.5f);
			m_fLastFov = fFov;
		}
	}
	//************************* end of flash compass

	//update radar jammer
	if(m_jammerID && m_jammerRadius > 0.0f)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_jammerID);
		if(pEntity)
		{
			Vec3 jamPos = pEntity->GetWorldPos();
			float dist = jamPos.GetDistance(pActor->GetEntity()->GetWorldPos());
			if(dist < m_jammerRadius)
			{
				m_jammingValue = 1.0f - dist/m_jammerRadius;
				m_flashRadar->Invoke("setNoiseValue", m_jammingValue);

				if(m_jammingValue >= 0.8f)
				{
					//update to remove entities
					m_flashRadar->Invoke("setObjectArray");
					if(m_renderMiniMap)
						m_flashPDA->Invoke("Root.PDAArea.Map_M.MapArea.setObjectArray");
					return;
				}
			}
			else if(m_jammingValue)
			{
				m_jammingValue = 0.0f;
				m_flashRadar->Invoke("setNoiseValue", 0.0f);
			}
		}
	}

	Vec3 vOrigin = GetISystem()->GetViewCamera().GetPosition();

	//extra vehicle checks
	bool inVehicle = false;
	bool inAAA = false;
	float aaaDamage = 0.0f;
	if(IVehicle *pVehicle = pPlayer->GetLinkedVehicle())
	{
		fRadius *= 2.0f;
		inVehicle = true;

		if(pVehicle->GetEntity()->GetClass() == m_pAAA)
		{
			inAAA = true;
			if(IVehicleComponent *pAAARadar = pVehicle->GetComponent("radar"))
			{
				aaaDamage = pAAARadar->GetDamageRatio();
				m_flashRadar->Invoke("setDamage", aaaDamage);
			}
		}
	}

	//****************************************************************************
	//singleplay squadmates are now 100% design controlled, see "SetTeamMate"

	//we get the player's team mates for team-based MP
	CGameRules *pGameRules = static_cast<CGameRules*>(gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules());
	int clientTeam = pGameRules->GetTeam(pActor->GetEntityId());
	if(gEnv->bMultiplayer)
	{
		m_teamMates.clear();
		pGameRules->GetTeamPlayers(clientTeam, m_teamMates);
	}
	//****************************************************************************


	//*********************************ACTUAL SCANNING****************************
	//scan proximity for entities
	if(now - m_lastScan > 0.2f) //5hz scanning
	{
		float scanningRadius = fRadius*1.1f;
		ScanProximity(vOrigin, scanningRadius);
		m_lastScan = now;
	}
	//*********************************CHECKED FOUND ENTITIES*********************
	int amount = m_entitiesInProximity.size();
	for(int i = 0; i < amount; ++i)
	{
		EntityId id = m_entitiesInProximity[i];
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);

		if(pEntity)
		{
			//reasons (not) to go on ... *******************************************************************
			if(IsEntityTagged(id))
				continue;

			//is it a corpse ?
			IActor* tempActor = pActorSystem->GetActor(pEntity->GetId());
			if(tempActor && !(tempActor->GetHealth() > 0))
				continue;

			//is it the player ?
			if(pActor->GetEntityId() == id)
				continue;

			//lets find out whether this entity belongs on the radar
			bool isOnRadar = false;
			bool mate = false;
			bool unknownEnemyObject = false;
			bool unknownEnemyActor = false;
			bool airplaneInAAA = false;

			if(inAAA)	//check whether it's an unknown, flying airplane when you are in an AAA (show all airplanes > 3m over ground)
			{
				FlashRadarType eType = ChooseType(pEntity);
				if(eType == EHeli)
				{
					Vec3 pos = pEntity->GetPos();
					if(pos.z - gEnv->p3DEngine->GetTerrainZ((int)pos.x, (int)pos.y) > 5.0f)
					{
						IVehicle *pAirplane = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
						if(pAirplane && pAirplane->IsPlayerDriving(false) && pAirplane->GetAltitude() > 2.5f) //GetAltitude also checks game brushes
							airplaneInAAA = isOnRadar = true;
					}
				}
			}

			//is it a team mate in multiplayer or is it a squad mate in singleplayer ?
			if(!isOnRadar && stl::find(m_teamMates, id))
				isOnRadar = mate = true;

			//has the object been scanned already?
			if(!isOnRadar && IsOnRadar(id, 0, m_entitiesOnRadar.size()-1))
				isOnRadar = true;

			if(!isOnRadar)	//check whether it's an aggressive (non-vehicle) AI (in possible proximity),
											//which is not yet on the radar
			{
				IAIObject *pTemp = pEntity->GetAI();
				if(pTemp && AIOBJECT_VEHICLE != pTemp->GetAIType() && pTemp->IsHostile(pActor->GetEntity()->GetAI(),false))
				{
					isOnRadar = true;
					unknownEnemyObject = true;
				}
				else if(tempActor && gEnv->bMultiplayer)	//not teammate, not AI -> enemy!?
				{
					isOnRadar = true;
					unknownEnemyActor = true;
				}
			}

			if(!isOnRadar)
				continue;
 
			//**********************************************************************************************

			Vec3 vTransformed = pEntity->GetWorldPos();
			if(!absoluteMode)
				vTransformed = playerViewMtxInverted * vTransformed;
			else	//the same without rotation
			{
				Matrix34 m;
				m.SetIdentity();
				m.SetTranslation(playerViewMtxTranslation);
				vTransformed = m.GetInverted() * vTransformed;
			}

			//distance check********************************************************************************

			if(fabsf(vTransformed.z) > 500.0f)
				continue;
			vTransformed.z = 0;

			float sizeScale = GetRadarSize(pEntity, pActor);
			float scaledX = (vTransformed.x/fRadius)*fEntityMaxDistance;
			float scaledY = (vTransformed.y/fRadius)*fEntityMaxDistance;

			float distSq = scaledX*scaledX+scaledY*scaledY;
			if(distSq > (fabsf(fEntityMaxDistance-sizeScale))*(fEntityMaxDistance-sizeScale))
			{
				if(!airplaneInAAA)
					continue;
				else	//AAA: these units are drawn at the edge of the radar, even when outside of the range
				{
					//the aaa can be damaged
					if(aaaDamage == 1.0f || aaaDamage > RANDOM())
						continue;
					vTransformed.SetLength(fEntityMaxDistance);
				}
			}

			//**********************************************************************************************

			float fX = newX + scaledX*widthScale;
			float fY = m_fY - scaledY;

			float fAngle = pActor->GetAngles().z - pEntity->GetWorldAngles().z;
			float fAlpha	= 0.85f * fFadeValue;

			//faction***************************************************************************************

			//AI Object
			IAIObject *pAIObject = pEntity->GetAI();
			//int texture = m_iTextureIDEnemy;
			int texture = m_iTextureIDPlayer;
			int friendly = mate?EFriend:ENeutral;
			bool checkDriver = false;
			if(pAIObject)
			{
				if(AIOBJECT_VEHICLE == pAIObject->GetAIType())	//switch to vehicle texture ?
				{
					texture = m_iTextureIDVehicle;
					if(pEntity->GetClass() == m_pVTOL || pEntity->GetClass() == m_pHeli)
						texture = m_iTextureIDAirVehicle;

					if(IVehicle* pVehicle =gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(id))
					{
						if (IVehicleSeat* pSeat = pVehicle->GetSeatById(1))
							if (pSeat->IsDriver())
							{
								EntityId driverId = pSeat->GetPassenger();
								IEntity *temp = gEnv->pEntitySystem->GetEntity(driverId);
								if(temp && temp->GetAI())
								{
									pAIObject = temp->GetAI(); //check the driver instead of the vehicle
									checkDriver = true;
								}
							}
					}
				}

				if(pAIObject->IsHostile(pActor->GetEntity()->GetAI(),false) && (checkDriver || AIOBJECT_VEHICLE != pAIObject->GetAIType()))
				{
					friendly = EEnemy;

					IUnknownProxy *pUnknownProxy = pAIObject->GetProxy();
					if(pUnknownProxy)
					{
						int iAlertnessState = pUnknownProxy->GetAlertnessState();

						if(unknownEnemyObject) //check whether the object is near enough and alerted
						{
							if(iAlertnessState < 1)
								continue;
							if(distSq > 225.0f)
								continue;
							else if(distSq > 25.0f)
								fAlpha -= 0.5f - ((225.0f - distSq) * 0.0025f); //fade out with distance
						}

						if(1 == iAlertnessState)
						{
							fAlpha = 0.65f + fCos * 0.35f;
							friendly = EAggressor;
						}
						else if(2 == iAlertnessState)
						{
							fAlpha = 0.65f + fCos * 0.35f;
							friendly = ESelf;
						}
					}
					else
						continue;
				}
				else if(gEnv->bMultiplayer)
				{
					if(mate)
						friendly = EFriend;
					else
						friendly = EEnemy;
				}
				else
				{
					if(checkDriver)	//probably the own player
						friendly = EFriend;
					else
						friendly = ENeutral;
				}
			}
			else if(gEnv->bMultiplayer)	//treats factions in multiplayer
			{
				if(tempActor)
				{
					bool scannedEnemy = false;
					if(!mate && !unknownEnemyActor) //probably MP scanned enemy
					{
						friendly = EEnemy;
						scannedEnemy = true;
					}

					if(unknownEnemyActor || scannedEnemy)	//unknown or known enemy in MP !?
					{
						float length = vTransformed.GetLength();
						if(length < 20.0f)
						{
							if(length < 0.0f)
								length = 0.0f;
							const int cLenThres = (int)(100.0f / length);
							if(m_iMultiplayerEnemyNear < cLenThres)
								m_iMultiplayerEnemyNear = cLenThres;
						}
					}
					if(!scannedEnemy && !mate)
						continue;
				}
				else
				{
					IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
					if(pVehicle && !pVehicle->GetDriver())
					{
						int team = g_pGame->GetGameRules()->GetTeam(pEntity->GetId());
						if(team == 0)
							friendly = ENeutral;
						else if(team == clientTeam)
							friendly = EFriend;
						else
							friendly = EEnemy;
					}
					else
						continue;
				}
			}
			else
				continue;
			//**********************************************************************************************

			//draw entity
			float lowerBoundY = m_fY - fRadarSizeOverTwo;
			float dimX = (newX + fRadarSizeOverTwo) - lowerBoundX;
			float dimY = (m_fY + fRadarSizeOverTwo) - lowerBoundY;
			numOfValues += ::FillUpDoubleArray(&entityValues, pEntity->GetId(), ChooseType(pEntity, true), (fX - lowerBoundX) / dimX, (fY - lowerBoundY) / dimY, (absoluteMode)?0.0f:180.0f+RAD2DEG(fAngle), friendly, sizeScale*25.0f, fAlpha*100.0f);
		}
	}

	//binocs***********************************************************************
	bool binocs = false;
	IInventory *pInventory=pActor->GetInventory();
	EntityId itemId = pInventory?pInventory->GetCurrentItem():0;
	if (itemId)
	{
		CWeapon *pWeapon = pActor->GetWeapon(itemId);
		if (pWeapon && pWeapon->GetEntity()->GetClass() == CItem::sBinocularsClass)
			binocs = true;
	}

	if (binocs)
	{
		EntityId lookAtObjectID = pActor->GetGameObject()->GetWorldQuery()->GetLookAtEntityId();
		if(!lookAtObjectID)
			lookAtObjectID = RayCastBinoculars(pPlayer);

		if (lookAtObjectID)
		{
			IEntity *pEntity=gEnv->pEntitySystem->GetEntity(lookAtObjectID);
			if (pEntity)
			{
				float fDistance = (pEntity->GetWorldPos()-pActor->GetEntity()->GetWorldPos()).GetLength();
				g_pGame->GetHUD()->GetScopes()->SetBinocularsDistance(fDistance);
			}
		}
		else
			g_pGame->GetHUD()->GetScopes()->SetBinocularsDistance(0);

		if (lookAtObjectID && lookAtObjectID!=m_lookAtObjectID)
		{
			m_lookAtTimer=0.65f;
			m_lookAtObjectID=lookAtObjectID;
		}
		else if (lookAtObjectID)
		{
			if (m_lookAtTimer>0.0f)
			{
				m_lookAtTimer-=fDeltaTime;
				if (m_lookAtTimer<=0.0f)
				{
					m_lookAtTimer=0.0f;

					if(!IsOnRadar(lookAtObjectID, 0, m_entitiesOnRadar.size()-1) && !IsNextObject(lookAtObjectID))
						m_scannerQueue.push_front(lookAtObjectID);
				}
			}
		}
		else
			m_lookAtObjectID=0;

		UpdateScanner(fDeltaTime);
	}
	else
		ResetScanner();
	//~binocs**********************************************************************

	// Draw MissionObjectives or their target entities
	// we draw after normal targets, so AI guys which are targets show up as Objective
	std::map<EntityId, string>::const_iterator it = m_missionObjectives.begin();
	std::map<EntityId, string>::const_iterator end = m_missionObjectives.end();
	for(; it != end; ++it)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(it->first);
		if (pEntity && !pEntity->IsHidden())
		{
			Vec3 vTransformed = pEntity->GetWorldPos();
			if(!absoluteMode)
				vTransformed = playerViewMtxInverted * vTransformed;
			else	//the same without rotation
			{
				Matrix34 m;
				m.SetIdentity();
				m.SetTranslation(playerViewMtxTranslation);
				vTransformed = m.GetInverted() * vTransformed;
			}
			vTransformed.z = 0;

			float scaledX = (vTransformed.x/fRadius)*fEntityMaxDistance;
			float scaledY = (vTransformed.y/fRadius)*fEntityMaxDistance;

			vTransformed.Set(scaledX, scaledY, 0.0f);

			FlashRadarFaction faction = ENeutral;
			float fAngle = 0.0f;
			float distSq = scaledX*scaledX+scaledY*scaledY;
			if(distSq > fEntityMaxDistance*fEntityMaxDistance)
			{
				vTransformed.SetLength(fEntityMaxDistance);
				faction = EFriend;
			}

			float fX = newX + vTransformed.x*widthScale;
			float fY = m_fY - vTransformed.y;
			float fAlpha	= 0.75f * fFadeValue;

			if(faction == EFriend) //dirty angle computation for arrow
			{
				Vec2 dir(m_fX - fX, m_fY - fY);
				dir.Normalize();
				fAngle = acos(Vec2(0.0f,1.0f).Dot(dir));
				if(m_fX > fX)
					fAngle = 2*gf_PI - fAngle;
			}

			//in flash
			float lowerBoundY = m_fY - fRadarSizeOverTwo;
			float dimX = (newX + fRadarSizeOverTwo) - lowerBoundX;
			float dimY = (m_fY + fRadarSizeOverTwo) - lowerBoundY;
			numOfValues += ::FillUpDoubleArray(&entityValues, pEntity->GetId(), 5 /* MO */, (fX - lowerBoundX) / dimX, (fY - lowerBoundY) / dimY, (absoluteMode)?0.0f:180+RAD2DEG(fAngle), faction, 75.0f, fAlpha*100.0f);
		}
	}

	//temp units on radar
	for(int t = 0; t < m_tempEntitiesOnRadar.size(); ++t)
	{
		TempRadarEntity temp = m_tempEntitiesOnRadar[t];
		float diffTime = now - temp.m_spawnTime;
		if(diffTime > temp.m_timeLimit || diffTime < 0.0f)
		{
			m_tempEntitiesOnRadar.erase(m_tempEntitiesOnRadar.begin() + t);
			--t;
			continue;
		}
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(temp.m_id);
		if (pEntity && !pEntity->IsHidden())
		{
			Vec3 vTransformed = pEntity->GetWorldPos();
			if(!absoluteMode)
				vTransformed = playerViewMtxInverted * vTransformed;
			else	//the same without rotation
			{
				Matrix34 m;
				m.SetIdentity();
				m.SetTranslation(playerViewMtxTranslation);
				vTransformed = m.GetInverted() * vTransformed;
			}
			vTransformed.z = 0;

			float scaledX = (vTransformed.x/fRadius)*fEntityMaxDistance;
			float scaledY = (vTransformed.y/fRadius)*fEntityMaxDistance;

			vTransformed.Set(scaledX, scaledY, 0.0f);

			float distSq = scaledX*scaledX+scaledY*scaledY;
			if(distSq > fEntityMaxDistance*fEntityMaxDistance)
				vTransformed.SetLength(fEntityMaxDistance);

			float fX = newX + vTransformed.x*widthScale;
			float fY = m_fY - vTransformed.y;

			float fAngle = pActor->GetAngles().z - pEntity->GetWorldAngles().z;
			float fAlpha	= 0.85f * fFadeValue;
			float sizeScale = GetRadarSize(pEntity, pActor);

			//in flash
			float lowerBoundY = m_fY - fRadarSizeOverTwo;
			float dimX = (newX + fRadarSizeOverTwo) - lowerBoundX;
			float dimY = (m_fY + fRadarSizeOverTwo) - lowerBoundY;

			int playerTeam = g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId());

			numOfValues += ::FillUpDoubleArray(&entityValues, temp.m_id, temp.m_type, (fX - lowerBoundX) / dimX, (fY - lowerBoundY) / dimY,
				(absoluteMode)?0.0f:180.0f+RAD2DEG(fAngle), FriendOrFoe(gEnv->bMultiplayer, pActor, playerTeam, pEntity,
				g_pGame->GetGameRules()), sizeScale*25.0f, fAlpha*100.0f);
		}
	}

	//tac bullet marked entities
	for(int i = 0; i < NUM_TAGGED_ENTITIES; ++i)
	{
		EntityId id = m_taggedEntities[i];
		if(id)
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);

			if(pEntity)
			{
				Vec3 vTransformed = pEntity->GetWorldPos();
				if(!absoluteMode)
					vTransformed = playerViewMtxInverted * vTransformed;
				else	//the same without rotation
				{
					Matrix34 m;
					m.SetIdentity();
					m.SetTranslation(playerViewMtxTranslation);
					vTransformed = m.GetInverted() * vTransformed;
				}
				vTransformed.z = 0;

				if(vTransformed.len() > fEntityMaxDistance)
				{
					vTransformed.SetLength(fEntityMaxDistance);
				}

				float fX = newX + vTransformed.x * widthScale;
				float fY = m_fY - vTransformed.y;
				float sizeScale = GetRadarSize(pEntity, pActor);

				//in flash
				float lowerBoundY = m_fY - fRadarSizeOverTwo;
				float dimX = (newX + fRadarSizeOverTwo) - lowerBoundX;
				float dimY = (m_fY + fRadarSizeOverTwo) - lowerBoundY;
				numOfValues += ::FillUpDoubleArray(&entityValues, pEntity->GetId(), 7 /*tagged entity*/, (fX - lowerBoundX) / dimX, (fY - lowerBoundY) / dimY, 0.0f, 0, sizeScale*100.0f, 100.0f);
			}
		}
	}

	//sounds on radar
	for(int i = 0; i < m_soundsOnRadar.size(); ++i)
	{
		RadarSound temp = m_soundsOnRadar[i];
		float diffTime = now - temp.m_spawnTime;
		if(diffTime > 2.0f || diffTime < 0.0f) //show only 2 seconds (and catch errors)
		{
			m_soundsOnRadar.erase(m_soundsOnRadar.begin() + i);
			--i;
			continue;
		}
		else
		{
			Vec3 pos = temp.m_pos;
			if(!absoluteMode)
				pos = playerViewMtxInverted * pos;
			else	//the same without rotation
			{
				Matrix34 m;
				m.SetIdentity();
				m.SetTranslation(playerViewMtxTranslation);
				pos = m.GetInverted() * pos;
			}
			
			float intensity = temp.m_intensity;
			if(intensity > 4.0f)
			{
				if(!temp.m_hasChild && diffTime > (0.05 + 0.1f * RANDOM()))
				{
					m_soundsOnRadar[i].m_hasChild = true;
					ShowSoundOnRadar(temp.m_pos, temp.m_intensity * (0.5f + 0.2f * RANDOM()));	//aftermatch
				}
				
				intensity = 4.0f;
			}

			float scaledX = (pos.x/fRadius)*fEntityMaxDistance;
			float scaledY = (pos.y/fRadius)*fEntityMaxDistance;
			float distSq = (scaledX*scaledX+scaledY*scaledY)*0.70f;
			float size = intensity*(4.0f+diffTime*4.0f);
			if(size > fEntityMaxDistance)
				continue;
			if(distSq > (fEntityMaxDistance/*-size*/)*(fEntityMaxDistance/*-size*/)) //-size not necessary when using flash
				continue;
			float fX = newX + scaledX*widthScale;
			float fY = m_fY - scaledY;
			//in flash
			float lowerBoundY = m_fY - fRadarSizeOverTwo;
			float dimX = (newX + fRadarSizeOverTwo) - lowerBoundX;
			float dimY = (m_fY + fRadarSizeOverTwo) - lowerBoundY;
			numOfValues += ::FillUpDoubleArray(&entityValues, temp.m_id, 6 /*sound*/, (fX - lowerBoundX) / dimX, (fY - lowerBoundY) / dimY, 0.0f, 0, size*10.0f, (1.0f-(0.5f*diffTime))*100.0f);
		}
	}
/* commented until proven not performance guilty
	if(true)
	{
		float playerX = 0.5f;
		float playerY = 0.5f;
		if(pActor)
		{
			GetPosOnMap(pActor->GetEntity(), playerX, playerY, true);
			SFlashVarValue args[3] = {playerX, playerY, 270.0f-RAD2DEG(pActor->GetAngles().z)};
			m_flashRadar->Invoke("setPlayer",args, 3);

		}
	}
*/
	//render the mini map
	if(m_renderMiniMap)
		RenderMapOverlay();

	entityValues.Flush();
	m_flashRadar->Invoke("setObjectArray");
}

//-----------------------------------------------------------------------------------------------------

float CHUDRadar::GetRadarSize(IEntity* entity, CActor* actor)
{
	float posEnt = entity->GetWorldPos().z;
	float posPlayer = actor->GetEntity()->GetWorldPos().z;
	float returnValue = 1.0f;

	if((posEnt + 15) < posPlayer)
		returnValue = m_radarDotSizes[0];
	else if((posEnt - 15) > posPlayer)
		returnValue = m_radarDotSizes[2];
	else
		returnValue = m_radarDotSizes[0] + ((posEnt - (posPlayer-15)) / 30.0f) * (m_radarDotSizes[2] - m_radarDotSizes[0]);

	if (entity->GetClass() == m_pHunter || entity->GetClass() == m_pWarrior)
		returnValue *= m_radarDotSizes[3];

	return returnValue;
}

bool CHUDRadar::IsOnRadar(EntityId id, int first, int last)
{
	int size = last - first;
	if(size > 1)
	{
		int mid = first + (size / 2);
		EntityId midId = m_entitiesOnRadar[mid].m_id;
		if(midId == id)
			return true;
		else if(midId > id)
			return IsOnRadar(id, first, mid-1);
		else
			return IsOnRadar(id, mid+1, last);
	}
	else if(size == 1)
		return (m_entitiesOnRadar[first].m_id == id  || m_entitiesOnRadar[last].m_id == id);
	else if(m_entitiesOnRadar.size() > 0)
		return (m_entitiesOnRadar[first].m_id == id);
	return false;
}

void CHUDRadar::AddToRadar(EntityId id)
{
	if(m_entitiesOnRadar.size()>0)
	{
		if(m_entitiesOnRadar[0].m_id > id)
			m_entitiesOnRadar.push_front(RadarEntity(id));
		else if(m_entitiesOnRadar[m_entitiesOnRadar.size()-1].m_id < id)
			m_entitiesOnRadar.push_back(RadarEntity(id));
		else
		{
			std::deque<RadarEntity>::iterator it, itTemp;
			for(it = m_entitiesOnRadar.begin(); it != m_entitiesOnRadar.end(); ++it)
			{
				itTemp = it;
				itTemp++;
				if(itTemp != m_entitiesOnRadar.end())
				{
					if(it->m_id < id && itTemp->m_id > id)
					{
						m_entitiesOnRadar.insert(itTemp, 1, RadarEntity(id));
						break;
					}
				}
				else { assert(false); }
			}
		}
	}
	else
		m_entitiesOnRadar.push_back(RadarEntity(id));
}

bool CHUDRadar::ScanObject(EntityId id)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
	if (!pEntity)
		return false;

	IPhysicalEntity *pPE=pEntity->GetPhysics();
	if (!pPE)
		return false;

	if (IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id))
	{
		if (CActor *pCActor=static_cast<CActor *>(pActor))
		{
			if (pCActor->GetActorClass()==CPlayer::GetActorClassType())
			{
				CPlayer *pPlayer=static_cast<CPlayer *>(pCActor);
				if (CNanoSuit *pNanoSuit=pPlayer->GetNanoSuit())
				{
					if (pNanoSuit->GetMode()==NANOMODE_CLOAK && pNanoSuit->GetCloak()->GetType()==CLOAKMODE_REFRACTION)
						return false;
				}
			}
		}
	}
	

	CCamera camera=GetISystem()->GetViewCamera();
	if (CheckObject(pEntity, camera, id!=m_lookAtObjectID, id!=m_lookAtObjectID) &&
		!IsOnRadar(id, 0, m_entitiesOnRadar.size()-1))
	{
		g_pGame->GetHUD()->AutoAimNoText(id);	

		m_scannerObjectID=id;
		m_scannerTimer=0.55f;

		return true;
	}

	return false;
}

void CHUDRadar::UpdateScanner(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(),PROFILE_GAME);

	if (m_scannerTimer>0.0f)
	{
		m_scannerTimer-=frameTime;
		if (m_scannerTimer<=0.0f)
		{
			m_scannerTimer=0.0f;

			g_pGame->GetHUD()->AutoAimUnlock(0);

			if (m_scannerObjectID)
			{
				EntityId clientId = g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId();
				//AddToRadar(m_scannerObjectID);
				g_pGame->GetGameRules()->ClientSimpleHit(SimpleHitInfo(clientId, m_scannerObjectID, 0, 0));
				g_pGame->GetHUD()->PlaySound(ESound_BinocularsLock);
				//if(gEnv->bMultiplayer)
				//	g_pGame->GetGameRules()->AddTaggedEntity(clientId, m_scannerObjectID, true);
			}

			m_scannerObjectID=0;
		}
	}

	if (m_scannerTimer<=0.0f && !m_scannerQueue.empty())
	{
		EntityId scanId=0;
		do
		{
			scanId = m_scannerQueue.front();
			m_scannerQueue.pop_front();

		} while (!ScanObject(scanId) && !m_scannerQueue.empty());
	}

	if (m_scannerGatherTimer>0.0f)
	{
		m_scannerGatherTimer-=frameTime;
		if (m_scannerGatherTimer<=0.0f)
		{
			m_scannerGatherTimer=0.0f;
			GatherScannableObjects();

			m_scannerGatherTimer=0.50f;
		}
	}
}

void CHUDRadar::Reset()
{
	ResetScanner();

	//remove scanned / tac'd entities
	m_entitiesOnRadar.clear();
	ResetTaggedEntities();
	m_tempEntitiesOnRadar.clear();
	m_storyEntitiesOnRadar.clear();
}

void CHUDRadar::ResetScanner()
{
	m_scannerQueue.clear();

	if (m_scannerTimer>0.0f)
	{
		g_pGame->GetHUD()->AutoAimUnlock(0);
		m_scannerTimer=0.0f;
	}
}

void CHUDRadar::QueueObject(EntityId id)
{
	m_scannerQueue.push_back(id);
}

bool CHUDRadar::IsObjectInQueue(EntityId id)
{
	if (std::find(m_scannerQueue.begin(), m_scannerQueue.end(), id) != m_scannerQueue.end())
		return true;
	return false;
}

bool CHUDRadar::IsNextObject(EntityId id)
{
	if (m_scannerQueue.empty())
		return false;

	return m_scannerQueue.front() == id;
}


bool CHUDRadar::CheckObject(IEntity *pEntity, const CCamera &camera, bool checkVelocity, bool checkVisibility)
{
	if ((pEntity->GetFlags()&ENTITY_FLAG_ON_RADAR) &&
		pEntity->GetPhysics() &&
		(pEntity->GetWorldPos().GetSquaredDistance(camera.GetPosition())<1000.0f*1000.0f) &&
		camera.IsPointVisible(pEntity->GetWorldPos()))
	{
		IPhysicalEntity *pPE=pEntity->GetPhysics();

		pe_status_dynamics dyn;
		pPE->GetStatus(&dyn);

		if (checkVelocity)
		{
			if (dyn.v.len2()<0.5f*0.5f)
				return false;
		}

		if (checkVisibility)
		{
			IActor *pActor=gEnv->pGame->GetIGameFramework()->GetClientActor();
			IPhysicalEntity *pSkipEnt=pActor?pActor->GetEntity()->GetPhysics():0;

			ray_hit hit;
			Vec3 dir=(dyn.centerOfMass-camera.GetPosition())*1.15f;

			if (gEnv->pPhysicalWorld->RayWorldIntersection(camera.GetPosition(), dir, ent_all, (13&rwi_pierceability_mask), &hit, 1, &pSkipEnt, pSkipEnt?1:0))
			{
				if (!hit.bTerrain && hit.pCollider!=pPE)
				{
					IEntity *pHitEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
					if (pHitEntity != pEntity)
						return false;
				}
			}
		}

		return true;
	}

	return false;
}

Vec2i CHUDRadar::GetMapGridPosition(IEntity *pEntity)
{
	if (pEntity)
	{
		Vec3 pos=pEntity->GetWorldPos();
		return GetMapGridPosition(pos.x, pos.y);
	}

	return Vec2i(-1, -1);
}

Vec2i CHUDRadar::GetMapGridPosition(float x, float y)
{
	Vec2i retVal(-1, -1);

	float outX=0.0f, outY=0.0f;
	
	GetPosOnMap(x, y, outX, outY);

	retVal.y=cry_ceilf(outX*8.0f);
	retVal.x=cry_ceilf(outY*8.0f);

	return retVal;
}

void CHUDRadar::GatherScannableObjects()
{
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if (!pActor)
		return;

	CCamera camera=GetISystem()->GetViewCamera();
	/*IEntityItPtr pIt=gEnv->pEntitySystem->GetEntityIterator();

	while (!pIt->IsEnd())
	{
		if (IEntity *pEntity=pIt->Next())
		{
			EntityId id=pEntity->GetId();
			if (CheckObject(pEntity, camera, true, false) && !IsOnRadar(id, 0, m_entitiesOnRadar.size()-1) && !IsObjectInQueue(id))
				QueueObject(id);
		}
	}*/
	IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	int count = pActorSystem->GetActorCount();
	if (count > 0)
	{
		IActorIteratorPtr pIter = pActorSystem->CreateActorIterator();
		while (IActor* pActor = pIter->Next())
		{
			if(IEntity *pEntity = pActor->GetEntity())
			{
				EntityId id = pEntity->GetId();
				if (CheckObject(pEntity, camera, true, false) && !IsOnRadar(id, 0, m_entitiesOnRadar.size()-1) && !IsObjectInQueue(id))
					QueueObject(id);
			}
		}
	}

	IVehicleSystem *pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
	count = pVehicleSystem->GetVehicleCount();
	if (count > 0)
	{
		IVehicleIteratorPtr pIter = pVehicleSystem->CreateVehicleIterator();
		while (IVehicle* pVehicle = pIter->Next())
		{
			if(IEntity *pEntity = pVehicle->GetEntity())
			{
				EntityId id = pEntity->GetId();
				if (CheckObject(pEntity, camera, true, false) && !IsOnRadar(id, 0, m_entitiesOnRadar.size()-1) && !IsObjectInQueue(id))
					QueueObject(id);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
bool CHUDRadar::GetPosOnMap(float inX, float inY, float &outX, float &outY, bool flashCoordinates)
{
	float posX = std::max(0.0f, inX - std::min(m_miniMapStartX, m_miniMapEndX));
	float posY = std::max(0.0f, inY - std::min(m_miniMapStartY, m_miniMapEndY));
	float temp = posX;
	posX = posY;
	posY = temp;
	outX = std::max(m_startOnScreenX, int(m_startOnScreenX + (std::min(posX * m_mapDivX, 1.0f) * m_mapDimX)));
	outY = std::max(m_startOnScreenY, int(m_startOnScreenY + (std::min(posY * m_mapDivY, 1.0f) * m_mapDimY)));
	if(flashCoordinates)
	{
		outX = (outX-m_startOnScreenX) / m_mapDimX;
		outY = (outY-m_startOnScreenY) / m_mapDimY;
	}

/* commented until proven not performance guilty
	float posX = std::max(0.0f, inX - std::min((float)m_miniMapStartX, (float)m_miniMapEndX));
	float posY = std::max(0.0f, inY - std::min((float)m_miniMapStartY, (float)m_miniMapEndY));
	float temp = posX;
	posX = posY;
	posY = temp;
	outX = std::max((float)m_startOnScreenX, (float)m_startOnScreenX + (std::min(posX * m_mapDivX, 1.0f) * (float)m_mapDimX));
	outY = std::max((float)m_startOnScreenY, (float)m_startOnScreenY + (std::min(posY * m_mapDivY, 1.0f) * (float)m_mapDimY));
	if(flashCoordinates)
	{
		outX = (outX-(float)m_startOnScreenX) / (float)m_mapDimX;
		outY = (outY-(float)m_startOnScreenY) / (float)m_mapDimY;
	}
*/
	return true;
}

//-----------------------------------------------------------------------------------------------------
bool CHUDRadar::GetPosOnMap(IEntity* pEntity, float &outX, float &outY, bool flashCoordinates)
{
	if(!pEntity)
		return false;
	Vec3 pos = pEntity->GetWorldPos();

	return GetPosOnMap(pos.x, pos.y, outX, outY, flashCoordinates);
}

//-----------------------------------------------------------------------------------------------------
void CHUDRadar::ComputeMiniMapResolution()
{
	//size of the displayed 3d area
	const float mapSizeX = fabsf(m_miniMapEndX - m_miniMapStartX);
	m_mapDivX = 1.0f / mapSizeX;
	const float mapSizeY = fabsf(m_miniMapEndY - m_miniMapStartY);
	m_mapDivY = 1.0f / mapSizeY;
	//coordinates of the map in screen pixels in 4:3
	m_startOnScreenX = 358;
	m_startOnScreenY = 50;
	m_endOnScreenX = 756;
	m_endOnScreenY = 446;
	float width = (float)gEnv->pRenderer->GetWidth();
	float height = (float)gEnv->pRenderer->GetHeight();
	//adopt x-coordinates to current resolution
	m_startOnScreenX	= (int)(800 - ((800-m_startOnScreenX) / 600.0f * 800 / width * height));
	m_endOnScreenX		= (int)(800 - ((800-m_endOnScreenX) / 600.0f * 800 / width * height));
	//compute screen dimensions
	m_mapDimX = m_endOnScreenX - m_startOnScreenX;
	m_mapDimY = m_endOnScreenY - m_startOnScreenY;

	m_mapGridSizeX = m_mapDimX * 0.125f;
	m_mapGridSizeY = m_mapDimY * 0.125f;
}

//------------------------------------------------------------------------
void CHUDRadar::OnLoadingComplete(ILevel *pLevel)
{
	if(!pLevel)
	{
		if(gEnv->pSystem->IsEditor())
		{
			char *levelName;
			char *levelPath;
			g_pGame->GetIGameFramework()->GetEditorLevel(&levelName, &levelPath);		//this is buggy, returns path for both
			LoadMiniMap(levelPath);
		}
	}
	else
	{
		LoadMiniMap(pLevel->GetLevelInfo()->GetPath()); 
		m_pLevelData = pLevel;
	}
}

//------------------------------------------------------------------------
void CHUDRadar::ReloadMiniMap()
{
	if(m_pLevelData)
		OnLoadingComplete(m_pLevelData);
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::LoadMiniMap(const char* mapPath)
{
	//get the factories (and other buildings)
	//IGameRules *pGameRules = gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules();
	//if(pGameRules && !strcmp(pGameRules->GetEntity()->GetClass()->GetName(), "PowerStruggle"))
	if(gEnv->bMultiplayer)
	{
		IEntityClass *factoryClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Factory" );
		IEntityClass *hqClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "HQ" );
		IEntityClass *alienClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "AlienEnergyPoint" );
		
		m_buildingsOnRadar.clear();

		IEntityItPtr pIt=gEnv->pEntitySystem->GetEntityIterator();

		float minDistance = 0;
		IEntity *pLocalActor = gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntity();

		while (!pIt->IsEnd())
		{
			if (IEntity *pEntity = pIt->Next())
			{
				IEntityClass *cls = pEntity->GetClass();
				if(cls==factoryClass || cls==hqClass || cls==alienClass)
				{
					m_buildingsOnRadar.push_back(RadarEntity(pEntity->GetId()));
				}
				if(cls==factoryClass && pLocalActor && g_pGame->GetHUD()->GetPowerStruggleHUD() && g_pGame->GetHUD()->GetPowerStruggleHUD()->IsFactoryType(pEntity->GetId(), CHUDPowerStruggle::E_PROTOTYPES))
				{
					Vec3 dirvec = (pLocalActor->GetPos()-pEntity->GetPos());
					float distance = dirvec.GetLength2D();
					if(!minDistance || minDistance>distance)
					{
						minDistance = distance;
						g_pGame->GetHUD()->SetOnScreenObjective(pEntity->GetId());
					}
				}
			}
		}
	}

	//now load the actual map
	string fullPath(mapPath);
	int slashPos = fullPath.rfind('\\');
	if(slashPos == -1)
		slashPos = fullPath.rfind('/');
	string mapName = fullPath.substr(slashPos+1, fullPath.length()-slashPos);
	m_currentLevel = mapName;

	fullPath.append("\\");
	fullPath.append(mapName);
	fullPath.append(".xml");
	XmlNodeRef mapInfo = GetISystem()->LoadXmlFile(fullPath.c_str());
	if(mapInfo == 0)
	{
		GameWarning("Did not find a minimap file %s in %s.", fullPath.c_str(), mapName.c_str());
		return;
	}

	//retrieve the coordinates of the map
	if(mapInfo)
	{
		for(int n = 0; n < mapInfo->getChildCount(); ++n)
		{
			XmlNodeRef mapNode = mapInfo->getChild(n);
			const char* name = mapNode->getTag();
			if(!stricmp(name, "MiniMap"))
			{
				int attribs = mapNode->getNumAttributes();
				const char* key;
				const char* value;
				for(int i = 0; i < attribs; ++i)
				{
					mapNode->getAttributeByIndex(i, &key, &value);
					if(!strcmp(key, "Filename") && m_flashPDA)
					{
						string mapDir = mapPath;
						mapDir.append("\\");
						mapDir.append(value);
						CryLog("loading mini map file %s to flash", mapDir.c_str());
						m_flashPDA->Invoke("setMapBackground", mapDir.c_str());
						//commented until proven not performance guilty
						//m_flashRadar->Invoke("setMapBackground", mapDir.c_str());
					}
					else if(!strcmp(key, "startX"))
						m_miniMapStartX = atoi(value);
					else if(!strcmp(key, "startY"))
						m_miniMapStartY = atoi(value);
					else if(!strcmp(key, "endX"))
						m_miniMapEndX = atoi(value);
					else if(!strcmp(key, "endY"))
						m_miniMapEndY = atoi(value);
				}
				//commented until proven not performance guilty
				//float radarRatio = (((float)(m_miniMapEndX - m_miniMapStartX)) / 75.0f) * 54.0f; 
				//m_flashRadar->Invoke("setMapScale", SFlashVarValue(radarRatio / 2048.0f) );
				break;
			}
		}
	}

	//compute the dimensions of the miniMap
	ComputeMiniMapResolution();
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::RenderMapOverlay()
{
	CGameFlashAnimation *m_flashMap = m_flashPDA;
	if(!m_flashMap || !m_flashMap->IsAvailable("Root.PDAArea.Map_M.MapArea"))
		return;
	//LoadMiniMap(m_currentLevel);

	m_possibleOnScreenObjectives.resize(0);
	//double array buffer for flash transfer optimization
	std::vector<double> entityValues;
	int numOfValues = 0;
	//array of text strings
	std::map<EntityId, string> textOnMap;

	float fX = 0;
	float fY = 0;

	//draw vehicles only once
	std::map<EntityId, bool> drawnVehicles;
	
	//draw helper
	IUIDraw* pDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();

	//the current GameRules
	CGameRules *pGameRules = (CGameRules*)(gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules());

	//the local player
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(!pActor ||!pGameRules)
		return;

	EntityId iOnScreenObjective = g_pGame->GetHUD()->GetOnScreenObjective();
	if(iOnScreenObjective)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(iOnScreenObjective);
		if(pEntity)
		{
			GetPosOnMap(pEntity, fX, fY);
			char strCoords[3];
			float value = ceil(fX*8.0f);

			if(fY<=0.125f)
				sprintf(strCoords,"A%d", (int)value);
			else if(fY<=0.25f)
				sprintf(strCoords,"B%d", (int)value);
			else if(fY<=0.375f)
				sprintf(strCoords,"C%d", (int)value);
			else if(fY<=0.5f)
				sprintf(strCoords,"D%d", (int)value);
			else if(fY<=0.625f)
				sprintf(strCoords,"E%d", (int)value);
			else if(fY<=0.75f)
				sprintf(strCoords,"F%d", (int)value);
			else if(fY<=0.875f)
				sprintf(strCoords,"G%d", (int)value);
			else if(fY<=1.0f)
				sprintf(strCoords,"H%d", (int)value);
			m_flashMap->CheckedSetVariable("Root.PDAArea.TextBottom.Colorset.ObjectiveText.text",strCoords);
		}
	}
	EntityId iCurrentSpawnPoint = pGameRules->GetPlayerSpawnGroup(pActor);
	if(iCurrentSpawnPoint)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(iCurrentSpawnPoint);
		if(pEntity)
		{
			GetPosOnMap(pEntity, fX, fY);
			char strCoords[3];
			float value = ceil(fX*8.0f);

			if(fY<=0.125f)
				sprintf(strCoords,"A%d", (int)value);
			else if(fY<=0.25f)
				sprintf(strCoords,"B%d", (int)value);
			else if(fY<=0.375f)
				sprintf(strCoords,"C%d", (int)value);
			else if(fY<=0.5f)
				sprintf(strCoords,"D%d", (int)value);
			else if(fY<=0.625f)
				sprintf(strCoords,"E%d", (int)value);
			else if(fY<=0.75f)
				sprintf(strCoords,"F%d", (int)value);
			else if(fY<=0.875f)
				sprintf(strCoords,"G%d", (int)value);
			else if(fY<=1.0f)
				sprintf(strCoords,"H%d", (int)value);
			m_flashMap->CheckedSetVariable("Root.PDAArea.TextBottom.Colorset.DeploySpotText.text",strCoords);
		}
	}
	
	bool isMultiplayer = gEnv->bMultiplayer;

	int team = pGameRules->GetTeam(pActor->GetEntityId());

	//draw buildings first
	for(int i = 0; i < m_buildingsOnRadar.size(); ++i)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_buildingsOnRadar[i].m_id);
		if(GetPosOnMap(pEntity, fX, fY))
		{
			int friendly = FriendOrFoe(isMultiplayer, pActor, team, pEntity, pGameRules);
			FlashRadarType type = ChooseType(pEntity);
			if((type == EHeadquarter || type == EHeadquarter2) && g_pGame->GetHUD()->HasTACWeapon() && friendly==EEnemy)
			{
				type = EHitZone;
			}
			m_possibleOnScreenObjectives.push_back(pEntity->GetId());
			numOfValues += FillUpDoubleArray(&entityValues, pEntity->GetId(), type, fX, fY, 270.0f-RAD2DEG(pEntity->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==m_buildingsOnRadar[i].m_id, iCurrentSpawnPoint==m_buildingsOnRadar[i].m_id);
		}
	}

	float fCos	= cosf(m_fTime);
	//.. and mission objectives
	{
		std::map<EntityId, string>::const_iterator it = m_missionObjectives.begin();
		std::map<EntityId, string>::const_iterator end = m_missionObjectives.end();

		for(; it != end; ++it)
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(it->first);
			if(GetPosOnMap(pEntity, fX, fY))
			{
				numOfValues += FillUpDoubleArray(&entityValues, pEntity->GetId(), EWayPoint, fX, fY, 180.0f, ENeutral, 100, 100, iOnScreenObjective==it->first, iCurrentSpawnPoint==it->first);
				textOnMap[pEntity->GetId()] = it->second;
			}
		}
	}

	//we need he players position later int the code
	Vec2 vPlayerPos(0.5f,0.5f);

	//draw player position
	if(pActor)
	{
		if(IVehicle* pVehicle = pActor->GetLinkedVehicle())
		{
			if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
			{
				GetPosOnMap(pVehicle->GetEntity(), fX, fY);
				vPlayerPos.x = fX;
				vPlayerPos.y = fY;
				numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ChooseType(pVehicle->GetEntity()), fX, fY, 270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), ESelf, 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
				drawnVehicles[pVehicle->GetEntityId()] = true;
			}
		}
	}

	if(isMultiplayer)
	{
		//now spawn points
		std::vector<EntityId> locations;
		pGameRules->GetSpawnGroups(locations);
		for(int i = 0; i < locations.size(); ++i)
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(locations[i]);
			if(!pEntity)
				continue;
			if (pEntity)
			{
				IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId());
				bool isVehicle = (pVehicle)?true:false;
				if(GetPosOnMap(pEntity, fX, fY))
				{
					int friendly = FriendOrFoe(isMultiplayer, pActor, team, pEntity, pGameRules);
					if(isVehicle /*&& !stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false)*/)
					{
						if(friendly == EFriend)
						{
							numOfValues += FillUpDoubleArray(&entityValues, pEntity->GetId(), ESpawnTruck, fX, fY, 270.0f-RAD2DEG(pEntity->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==locations[i], iCurrentSpawnPoint==locations[i]);
							drawnVehicles[pVehicle->GetEntityId()] = true;
						}
					}
					else
					{
						numOfValues += FillUpDoubleArray(&entityValues, pEntity->GetId(), ESpawnPoint, fX, fY, 270.0f-RAD2DEG(pEntity->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==locations[i], iCurrentSpawnPoint==locations[i]);
						m_possibleOnScreenObjectives.push_back(pEntity->GetId());
					}
				}
			}
		}

		//special units
		const std::vector<CGameRules::SMinimapEntity> synchEntities = pGameRules->GetMinimapEntities();
		for(int m = 0; m < synchEntities.size(); ++m)
		{
			CGameRules::SMinimapEntity mEntity = synchEntities[m];
			FlashRadarType type = GetSynchedEntityType(mEntity.type);
			IEntity *pEntity = NULL;
			if(type == ENuclearWeapon || type == ETechCharger)	//might be a gun
			{
				if(IItem *pWeapon = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(mEntity.entityId))
				{
					if(pWeapon->GetOwnerId())
						pEntity = gEnv->pEntitySystem->GetEntity(pWeapon->GetOwnerId());
				}
				else
					pEntity = gEnv->pEntitySystem->GetEntity(mEntity.entityId);
		}
				
			if(GetPosOnMap(pEntity, fX, fY))
			{
				int friendly = FriendOrFoe(isMultiplayer, pActor, team, pEntity, pGameRules);
				if(friendly == EFriend || friendly == ENeutral)
				{
					if(type == EAmmoTruck && stl::find_in_map(drawnVehicles, mEntity.entityId, false))
					{
						numOfValues += FillUpDoubleArray(&entityValues, pEntity->GetId(), EBarracks, fX, fY, 270.0f-RAD2DEG(pEntity->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==mEntity.entityId, iCurrentSpawnPoint==mEntity.entityId);
					}
					else
						numOfValues += FillUpDoubleArray(&entityValues, pEntity->GetId(), type, fX, fY, 270.0f-RAD2DEG(pEntity->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==mEntity.entityId, iCurrentSpawnPoint==mEntity.entityId);
				}
			}
		}
	}

	//draw player position
	if(pActor)
	{
		IVehicle* pVehicle = pActor->GetLinkedVehicle();
		if(!pVehicle)
		{
			GetPosOnMap(pActor->GetEntity(), fX, fY);
			vPlayerPos.x = fX;
			vPlayerPos.y = fY;
			string name(pActor->GetEntity()->GetName());
			numOfValues += FillUpDoubleArray(&entityValues, pActor->GetEntity()->GetId(), (name.find("Quarantine",0)!=string::npos)?ENuclearWeapon:EPlayer, fX, fY, 270.0f-RAD2DEG(pActor->GetEntity()->GetWorldAngles().z), ESelf, 100, 100, iOnScreenObjective==pActor->GetEntity()->GetId(), iCurrentSpawnPoint==pActor->GetEntity()->GetId());
		}
	}

	//helpers for actor rendering
	IActor* pTempActor = NULL;
	IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	IVehicleSystem *pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();

	//draw tagged guys and vehicles...
	{
		for(int i = 0; i < NUM_TAGGED_ENTITIES; ++i)
		{
			EntityId id = m_taggedEntities[i];
			if(id)
			{
				if(pTempActor = pActorSystem->GetActor(id))
				{
					if(IVehicle* pVehicle = pTempActor->GetLinkedVehicle())
					{
						if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
						{
							GetPosOnMap(pVehicle->GetEntity(), fX, fY);
							numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ETaggedEntity, fX, fY, 270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), ENeutral, 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
							drawnVehicles[pVehicle->GetEntityId()] = true;
						}
					}
					else
					{
						GetPosOnMap(pTempActor->GetEntity(), fX, fY);
						numOfValues += FillUpDoubleArray(&entityValues, pActor->GetEntity()->GetId(), ETaggedEntity, fX, fY, 270.0f-RAD2DEG(pActor->GetEntity()->GetWorldAngles().z), ENeutral, 100, 100, iOnScreenObjective==pActor->GetEntity()->GetId(), iCurrentSpawnPoint==pActor->GetEntity()->GetId());
					}
				}
				else if(IVehicle *pVehicle = pVehicleSystem->GetVehicle(id))
				{
					if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
					{
						GetPosOnMap(pVehicle->GetEntity(), fX, fY);
						numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ETaggedEntity, fX, fY, 270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), ENeutral, 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
						drawnVehicles[pVehicle->GetEntityId()] = true;
					}
				}
			}
		}
	}

	//draw temporarily tagged units
	{
		for(int e = 0; e < m_tempEntitiesOnRadar.size(); ++e)
		{
			EntityId id = m_tempEntitiesOnRadar[e].m_id;
			if(pTempActor = pActorSystem->GetActor(id))
			{
				if(IVehicle* pVehicle = pTempActor->GetLinkedVehicle())
				{
					if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
					{
						GetPosOnMap(pVehicle->GetEntity(), fX, fY);
						numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ChooseType(pVehicle->GetEntity(), false), fX, fY,
							270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), FriendOrFoe(isMultiplayer, pActor, team, pVehicle->GetEntity(), pGameRules), 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
						drawnVehicles[pVehicle->GetEntityId()] = true;
					}
				}
				else
				{
					GetPosOnMap(pTempActor->GetEntity(), fX, fY);
					numOfValues += FillUpDoubleArray(&entityValues, pActor->GetEntity()->GetId(), ChooseType(pActor->GetEntity(), false), fX, fY,
						270.0f-RAD2DEG(pActor->GetEntity()->GetWorldAngles().z), FriendOrFoe(isMultiplayer, pActor, team, pActor->GetEntity(), pGameRules), 100, 100, iOnScreenObjective==pActor->GetEntity()->GetId(), iCurrentSpawnPoint==pActor->GetEntity()->GetId());
				}
			}
			else if(IVehicle *pVehicle = pVehicleSystem->GetVehicle(id))
			{
				if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
				{
					GetPosOnMap(pVehicle->GetEntity(), fX, fY);
					numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ChooseType(pVehicle->GetEntity(), false), fX, fY,
						270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), FriendOrFoe(isMultiplayer, pActor, team, pVehicle->GetEntity(), pGameRules), 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
					drawnVehicles[pVehicle->GetEntityId()] = true;
				}
			}
		}
	}

	//draw story entities (icons with text)
	{
		for(int e = 0; e < m_storyEntitiesOnRadar.size(); ++e)
		{
			EntityId id = m_storyEntitiesOnRadar[e].m_id;
			if(IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id))
			{
				GetPosOnMap(pEntity, fX, fY);
				numOfValues += FillUpDoubleArray(&entityValues, id, m_storyEntitiesOnRadar[e].m_type, fX, fY,
					270.0f-RAD2DEG(pEntity->GetWorldAngles().z), ENeutral, 100, 100, iOnScreenObjective==id, iCurrentSpawnPoint==id);
				if(m_storyEntitiesOnRadar[e].m_text.length())
					textOnMap[id] = m_storyEntitiesOnRadar[e].m_text;
			}
		}
	}

	//draw position of teammates ...
	{
		std::vector<EntityId>::const_iterator it = m_teamMates.begin();
		std::vector<EntityId>::const_iterator end = m_teamMates.end();
		for(;it != end; ++it)
		{
			pTempActor = pActorSystem->GetActor(*it);
			if(pTempActor && pTempActor != pActor)
			{
				if(IVehicle *pVehicle = pTempActor->GetLinkedVehicle())
				{
					if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
					{
						GetPosOnMap(pVehicle->GetEntity(), fX, fY);
						numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ChooseType(pVehicle->GetEntity()), fX, fY, 270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), EFriend, 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
						drawnVehicles[pVehicle->GetEntityId()] = true;
					}
				}
				else
				{
					GetPosOnMap(pTempActor->GetEntity(), fX, fY);
					numOfValues += FillUpDoubleArray(&entityValues, pTempActor->GetEntity()->GetId(), EPlayer, fX, fY, 270.0f-RAD2DEG(pTempActor->GetEntity()->GetWorldAngles().z), EFriend, 100, 100, iOnScreenObjective==pTempActor->GetEntity()->GetId(), iCurrentSpawnPoint==pTempActor->GetEntity()->GetId());
					//draw teammate name if selected
					if(gEnv->bMultiplayer)
					{
						EntityId id = pTempActor->GetEntityId();
						for(int i = 0; i < m_selectedTeamMates.size(); ++i)
						{
							if(m_selectedTeamMates[i] == id)
							{
								textOnMap[id] = pTempActor->GetEntity()->GetName();
								break;
							}
						}
					}
				}
			}
		}
	}

	//draw radar entities on the map (scanned enemies and vehicles) 
	//scanned vehicles have to be drawn last to find the "neutral" ones correctly
	for(int i = 0; i < m_entitiesOnRadar.size(); ++i)
	{
		// TODO: if entity is sleeping, fade from sleep color to normal color as the entity is waking up ...

		pTempActor = pActorSystem->GetActor(m_entitiesOnRadar[i].m_id);
		if(pTempActor)
		{
			if(IVehicle* pVehicle = pTempActor->GetLinkedVehicle())
			{
				if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
				{
					GetPosOnMap(pVehicle->GetEntity(), fX, fY);
					int friendly = FriendOrFoe(isMultiplayer, pActor, team, pVehicle->GetEntity(), pGameRules);
					numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ChooseType(pVehicle->GetEntity()), fX, fY, 270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
					drawnVehicles[pVehicle->GetEntityId()] = true;
				}
			}
			else
			{
				int friendly = FriendOrFoe(isMultiplayer, pActor, team, pTempActor->GetEntity(), pGameRules);
				GetPosOnMap(pTempActor->GetEntity(), fX, fY);
				numOfValues += FillUpDoubleArray(&entityValues, pTempActor->GetEntity()->GetId(), EPlayer, fX, fY, 270.0f-RAD2DEG(pTempActor->GetEntity()->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==pTempActor->GetEntity()->GetId(), iCurrentSpawnPoint==pTempActor->GetEntity()->GetId());
			}
		}
		else if(IVehicle *pVehicle = pVehicleSystem->GetVehicle(m_entitiesOnRadar[i].m_id))
		{
			if(!stl::find_in_map(drawnVehicles, pVehicle->GetEntityId(), false))
			{
				int friendly = FriendOrFoe(isMultiplayer, pActor, team, pVehicle->GetEntity(), pGameRules);
				GetPosOnMap(pVehicle->GetEntity(), fX, fY);
				numOfValues += FillUpDoubleArray(&entityValues, pVehicle->GetEntity()->GetId(), ChooseType(pVehicle->GetEntity()), fX, fY, 270.0f-RAD2DEG(pVehicle->GetEntity()->GetWorldAngles().z), friendly, 100, 100, iOnScreenObjective==pVehicle->GetEntity()->GetId(), iCurrentSpawnPoint==pVehicle->GetEntity()->GetId());
				drawnVehicles[pVehicle->GetEntityId()] = true;
			}
		}
	}

	ComputePositioning(vPlayerPos, &entityValues);

	//tell flash file that we are done ...
	//m_flashMap->Invoke("updateObjects", "");
	if(entityValues.size())
		m_flashMap->GetFlashPlayer()->SetVariableArray(FVAT_Double, "Root.PDAArea.Map_M.MapArea.m_allValues", 0, &entityValues[0], numOfValues);
	m_flashMap->Invoke("Root.PDAArea.Map_M.MapArea.setObjectArray");
	//render text strings
	std::map<EntityId, string>::const_iterator itText = textOnMap.begin();
	for(; itText != textOnMap.end(); ++itText)
	{
		SFlashVarValue args[2] = {itText->first, itText->second.c_str()};
		m_flashMap->Invoke("Root.PDAArea.Map_M.MapArea.setText", args, 2);
	}
}

//-----------------------------------------------------------------------------------------------------

//calculate correct map positions, while the map is zoomed or translated
void CHUDRadar::ComputePositioning(Vec2 playerpos, std::vector<double> *doubleArray)
{

	bool bUpdate = false;
	//update zooming
	if(m_fPDATempZoomFactor!=m_fPDAZoomFactor)
	{
		float fRatio = m_fPDATempZoomFactor-m_fPDAZoomFactor;
		if(cry_fabsf(fRatio) > 0.01)
		{
			m_fPDAZoomFactor += fRatio*0.4;
		}
		else
		{
			m_fPDAZoomFactor = m_fPDATempZoomFactor;
		}
		bUpdate = true;
	}

	float fMapSize = 508.0f * m_fPDAZoomFactor;

	//update dragging
	float fLimit = (1.0f / fMapSize);

	if(m_vPDAMapTranslation.x != m_vPDATempMapTranslation.x)
	{
		float fDiffX = m_vPDATempMapTranslation.x-m_vPDAMapTranslation.x;
		if(cry_fabsf(fDiffX) > fLimit)
		{
			m_vPDAMapTranslation.x += fDiffX * 0.4f;
		}
		else
		{
			m_vPDAMapTranslation.x = m_vPDATempMapTranslation.x;
		}
		bUpdate = true;
	}
	if(m_vPDAMapTranslation.y != m_vPDATempMapTranslation.y)
	{
		float fDiffY = m_vPDATempMapTranslation.y-m_vPDAMapTranslation.y;
		if(cry_fabsf(fDiffY) > fLimit)
		{
			m_vPDAMapTranslation.y += fDiffY * 0.4;
		}
		else
		{
			m_vPDAMapTranslation.y = m_vPDATempMapTranslation.y;
		}
		bUpdate= true;
	}

	//calculate offset
	Vec2 vOffset = Vec2(0,0);
	Vec2 vMapPos = Vec2((254.0f - playerpos.x * fMapSize) + m_vPDAMapTranslation.x, (254.0f + (1.0f - playerpos.y) * fMapSize) + m_vPDAMapTranslation.y);

	if(vMapPos.x>0.0f)
	{
		vOffset.x = -vMapPos.x;
	}
	else if(vMapPos.x<(508.0f-fMapSize))
	{
		vOffset.x = (508.0f-fMapSize) - vMapPos.x;
	}

	if(vMapPos.y<508)
	{
		vOffset.y = 508 - vMapPos.y;
	}
	else if(vMapPos.y>fMapSize)
	{
		vOffset.y = fMapSize - vMapPos.y;
	}
	//if(bUpdate || m_initMap)
	{
		m_initMap = false;

		m_flashPDA->SetVariable("Root.PDAArea.Map_M.MapArea.Map.Map_G._xscale", SFlashVarValue(100.0f * m_fPDAZoomFactor));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.MapArea.Map.Map_G._yscale", SFlashVarValue(100.0f * m_fPDAZoomFactor));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.MapArea.Map.Map_G._x", SFlashVarValue(vMapPos.x + vOffset.x));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.MapArea.Map.Map_G._y", SFlashVarValue(vMapPos.y + vOffset.y));

		float fStep = 63.5f * m_fPDAZoomFactor;
		float fOffsetX = (fStep * 0.5f)-77.0f;
		float fOffsetY = (fStep * 0.5f)-9.0f;
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorA._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*8.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorB._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*7.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorC._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*6.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorD._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*5.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorE._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*4.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorF._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*3.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorG._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*2.0f + fOffsetY));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.SectorH._y", SFlashVarValue((vMapPos.y + vOffset.y)- fStep*1.0f + fOffsetY));

		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector1._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*1.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector2._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*2.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector3._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*3.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector4._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*4.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector5._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*5.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector6._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*6.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector7._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*7.0f - fOffsetX));
		m_flashPDA->SetVariable("Root.PDAArea.Map_M.Sector8._x", SFlashVarValue((vMapPos.x + vOffset.x)+ fStep*8.0f - fOffsetX));
		
		float value = 0;
		value = (vMapPos.x + vOffset.x)+ fStep*4.0f - fOffsetX;
		value = (vMapPos.x + vOffset.x)+ fStep*4.0f - fOffsetX;
	}

	//calculate icon positions
	for(int i(0); i < doubleArray->size(); i+=10)
	{
		if(doubleArray->size() > i+6)
		{
			(*doubleArray)[i+2] = 254.0f - ((playerpos.x - (*doubleArray)[i+2]) *fMapSize ) + m_vPDAMapTranslation.x + vOffset.x;
			(*doubleArray)[i+3] = 254.0f - ((playerpos.y - (*doubleArray)[i+3]) *fMapSize ) + m_vPDAMapTranslation.y + vOffset.y;
			(*doubleArray)[i+6] = (*doubleArray)[i+6] * (m_fPDAZoomFactor+2.0f) * 0.25f;
		}
	}
	if(m_bDragMap)
		m_vPDATempMapTranslation += vOffset;
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::InitMap()
{
	m_initMap = true;
	m_vPDATempMapTranslation = Vec2(0,0);
}

bool CHUDRadar::ZoomPDA(bool bZoomDirection)
{
	float fNextStep = 0.25f - (0.25f / m_fPDATempZoomFactor);

	if(bZoomDirection)
		m_fPDATempZoomFactor *= (1.1f + fNextStep);
	else
		m_fPDATempZoomFactor /= (1.1f + fNextStep);

	m_fPDATempZoomFactor = clamp_tpl(m_fPDATempZoomFactor, 1.0f, 8.0f);
	return true;
}

bool CHUDRadar::ZoomChangePDA(float p_fValue)
{
	m_fPDATempZoomFactor-=(p_fValue*0.03);
	m_fPDATempZoomFactor = clamp_tpl(m_fPDATempZoomFactor, 1.0f, 8.0f);
	return true;
}

bool CHUDRadar::DragMap(Vec2 p_vDir)
{
	m_vPDATempMapTranslation += p_vDir * m_fPDAZoomFactor;
	return true;
}

void CHUDRadar::SetDrag(bool enabled)
{
	m_bDragMap = enabled;
	if(enabled)
	{
		//m_vPDATempMapTranslation = Vec2(0,0);
	}
	else
	{
		//m_vPDATempMapTranslation = Vec2(0,0);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::Serialize(TSerialize ser)
{
	if(ser.GetSerializationTarget() != eST_Network)
	{
		ser.BeginGroup("HUDRadar");
		int amount = 0;
		amount = m_entitiesOnRadar.size();
		ser.Value("AmountOfRadarEntities", amount);
		if(ser.IsReading())
		{
			m_entitiesOnRadar.clear();
			m_entitiesOnRadar.resize(amount);
		}
		for(int h = 0; h < amount; ++h)
		{
			ser.BeginGroup("RadarEntity");
			ser.Value("id", m_entitiesOnRadar[h].m_id);
			ser.EndGroup();
		}

		ser.Value("PDAZoomFactor", m_fPDAZoomFactor);

		for(int h = 0; h < NUM_TAGGED_ENTITIES; ++h)
		{
			ser.BeginGroup("TaggedEntity");
			ser.Value("id", m_taggedEntities[h]);
			ser.EndGroup();
		}

		ser.Value("jammerID", m_jammerID);
		ser.Value("jammerRadius", m_jammerRadius);
		ser.Value("jammerValue", m_jammingValue);

		amount = m_missionObjectives.size();
		ser.Value("AmountOfMissionObjectives", amount);
		if(ser.IsReading())
		{
			m_entitiesInProximity.clear();
			m_itemsInProximity.clear();

			m_missionObjectives.clear();
			EntityId id = 0;
			string text;
			for(int i = 0; i < amount; ++i)
			{
				ser.BeginGroup("MissionObjective");
				ser.Value("Active", id);
				ser.Value("Text", text);
				ser.EndGroup();
				m_missionObjectives[id] = text;
			}

			//reset update timer
			m_lastScan = 0.0f;
			m_startBroadScanTime = 0.0f;
		}
		else //writing
		{
			std::map<EntityId, string>::iterator it = m_missionObjectives.begin();
			for(int i = 0; i < amount; ++i, ++it)
			{
				EntityId id = it->first;
				string text = it->second;
				ser.BeginGroup("MissionObjective");
				ser.Value("Active", id);
				ser.Value("Text", text);
				ser.EndGroup();
			}
		}
		ser.EndGroup(); //HUDRadar;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDRadar::UpdateMissionObjective(EntityId id, bool active, const char* description)
{
	if (id == 0)
		return;

	std::map<EntityId, string>::iterator iter = m_missionObjectives.find(id);
	if (iter != m_missionObjectives.end() && !active)
		m_missionObjectives.erase(iter);
	else if (active)
		m_missionObjectives.insert(std::map<EntityId, string>::value_type (id, description));
}

//-----------------------------------------------------------------------------------------------------

FlashRadarType CHUDRadar::ChooseType(IEntity* pEntity, bool radarOnly)
{
	if(!pEntity)
		return EFirstType;
	const IEntityClass *pCls = pEntity->GetClass();
	const char* cls = pCls->GetName();
	const char* name = pEntity->GetName();

	FlashRadarType returnType = ELTV;

	if(pCls == m_pPlayerClass || pCls == m_pGrunt)
		returnType = EPlayer;
	else if(pCls == m_pAlien || pCls == m_pTrooper)
		returnType = EPlayer;
	else if(pCls == m_pLTVUS || pCls == m_pLTVA)
		returnType = ELTV;
	else if(pCls == m_pTankUS || pCls == m_pTankA)
		returnType = ETank;
	else if(pCls == m_pAAA)
		returnType = EAAA;
	else if(pCls == m_pTruck)
		returnType = ETruck;
	else if(pCls == m_pTruck || pCls == m_pAPCUS || pCls == m_pAPCA)
		returnType = EAPC;
	else if(pCls == m_pHeli)
		returnType = EHeli;
	else if(pCls == m_pVTOL)
		returnType = EVTOL;
	else if(pCls == m_pScout)
		returnType = EHeli;
	else if(pCls == m_pWarrior || pCls == m_pHunter)
		returnType = EINVALID1;
	else if(pCls == m_pBoatCiv)
		returnType = ESmallBoat;
	else if(pCls == m_pHover)
		returnType = EHovercraft;
	else if(pCls == m_pBoatA)
		returnType = EPatrolBoat;
	else if(pCls == m_pBoatUS)
		returnType = ESpeedBoat;
	else if(!stricmp(cls, "HQ"))
	{
		CGameRules *pGameRules = static_cast<CGameRules*>(gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules());
		if(pGameRules->GetTeam(pEntity->GetId()) == 2) //us team
			returnType = EHeadquarter2;
		else
			returnType = EHeadquarter;
	}
	else if(!stricmp(cls, "Factory") && g_pGame->GetHUD()->GetPowerStruggleHUD())	//this should be much bigger and choose out of the different factory versions (not yet existing)
	{
		if(g_pGame->GetHUD()->GetPowerStruggleHUD()->CanBuild(pEntity,"ustank"))
		{
			returnType = EFactoryTank;
		}
		else if(g_pGame->GetHUD()->GetPowerStruggleHUD()->CanBuild(pEntity,"nkhelicopter"))
		{
			returnType = EFactoryAir;
		}
		else if(g_pGame->GetHUD()->GetPowerStruggleHUD()->CanBuild(pEntity,"nkboat"))
		{
			returnType = EFactorySea;
		}
		else if(g_pGame->GetHUD()->GetPowerStruggleHUD()->CanBuild(pEntity,"nk4wd"))
		{
			returnType = EFactoryVehicle;
		}
		else
		{
			returnType = EFactoryPrototype;
		}
	}
	else if(!stricmp(cls,"AlienEnergyPoint"))
	{
		returnType = EAlienEnergySource;
	}
	if(radarOnly)
	{
		if(returnType == EPlayer)
			returnType = ETank; //1
		else if(returnType == EHeli)
			returnType = EAPC; //2
		else if(returnType == EINVALID1) //currently big aliens like hunter
			returnType = ETank;
		else
			returnType = ECivilCar; //3
	}
		
	return returnType;
}

//-----------------------------------------------------------------------------------------------------

FlashRadarFaction CHUDRadar::FriendOrFoe(bool multiplayer, IActor *player, int team, IEntity *entity, CGameRules *pGameRules)
{
	FlashRadarFaction val = ENeutral;

	if(multiplayer)
	{
		int friendly = pGameRules->GetTeam(entity->GetId());
		if(friendly != ENeutral)
		{
			if(friendly == team)
				friendly = EFriend;
			else
				friendly = EEnemy;
		}
		val = (FlashRadarFaction)friendly;
	}
	else if(player->GetEntity()->GetAI())
	{
		IAIObject *pAI = entity->GetAI();
		if(pAI && pAI->GetAIType() == AIOBJECT_VEHICLE)
		{
			if(IVehicle* pVehicle =gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(entity->GetId()))
			{	
				if (IVehicleSeat* pSeat = pVehicle->GetSeatById(1))
					if (pSeat->IsDriver())
					{
						EntityId driverId = pSeat->GetPassenger();
						IEntity *temp = gEnv->pEntitySystem->GetEntity(driverId);
						if(temp && temp->GetAI())
							pAI = temp->GetAI();
					}
			}
		}

		if(pAI && pAI->GetAIType() != AIOBJECT_VEHICLE)
		{
			if(pAI->IsHostile(player->GetEntity()->GetAI(),false))
				val = EEnemy;
			else
				val = EFriend;
		}
	}

	return val;
}

FlashRadarType CHUDRadar::GetSynchedEntityType(int type)
{
	switch(type)
	{
	case 1:
		return EAmmoTruck;
		break;
	case 2:
		return ENuclearWeapon;	//this is a tac entity
		break;
	case 3:
		return ETechCharger;
		break;
	default:
		break;
	}
	return EFirstType;
}

int CHUDRadar::FillUpDoubleArray(std::vector<double> *doubleArray, double a, double b, double c, double d, double e, double f, double g, double h, double i, double j)
{
	doubleArray->push_back(a);
	doubleArray->push_back(b);
	doubleArray->push_back(c);
	doubleArray->push_back(d);
	doubleArray->push_back(e);
	doubleArray->push_back(f);
	doubleArray->push_back(g);
	doubleArray->push_back(h);
	doubleArray->push_back(i);
	doubleArray->push_back(j);

	return 10;
}

void CHUDRadar::StartBroadScan(bool useParameters, bool keepEntities, Vec3 pos, float radius)
{
	IActor *pClient = g_pGame->GetIGameFramework()->GetClientActor();
	if(!pClient)
		return;

	if(!m_startBroadScanTime) //wait (1.5) seconds before actually scanning - delete timer after scan 
	{
		m_startBroadScanTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		//set scanning parameter
		m_bsUseParameter = useParameters;
		m_bsKeepEntries = keepEntities;
		m_bsPosition = pos;
		m_bsRadius = radius;

		return;
	}
	else if(m_bsUseParameter)	//this is a custom scan
	{
		ScanProximity(m_bsPosition, m_bsRadius);
		if(m_bsKeepEntries)
			g_pGame->GetHUD()->ShowDownloadSequence();
	}
	else	//this is a quick proximity scan
		m_bsKeepEntries = false;

	int playerTeam = g_pGame->GetGameRules()->GetTeam(pClient->GetEntityId());
	for(int e = 0; e < m_entitiesInProximity.size(); ++e)
	{
		EntityId id = m_entitiesInProximity[e];
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
		if(IsOnRadar(id, 0, m_entitiesOnRadar.size()-1) || IsEntityTagged(id))
			continue;
		if(stl::find(m_teamMates, id))
			continue;

		g_pGame->GetGameRules()->AddTaggedEntity(pClient->GetEntityId(), id, m_bsKeepEntries?false:true);
	}
	m_startBroadScanTime = 0.0f;
	m_bsKeepEntries = false;
}

void CHUDRadar::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_scannerQueue);
	s->AddContainer(m_teamMates);
	s->AddContainer(m_entitiesInProximity);
	s->AddContainer(m_itemsInProximity);
	s->AddContainer(m_tempEntitiesOnRadar);
	s->AddContainer(m_storyEntitiesOnRadar);
	s->AddContainer(m_entitiesOnRadar);
	s->AddContainer(m_soundsOnRadar);
	s->AddContainer(m_buildingsOnRadar);
	s->AddContainer(m_missionObjectives);
	for (std::map<EntityId, string>::iterator iter = m_missionObjectives.begin(); iter != m_missionObjectives.end(); ++iter)
		s->Add(iter->second);
}

void CHUDRadar::SetTeamMate(EntityId id, bool active)
{
	bool found = false;

	std::vector<EntityId>::iterator it = m_teamMates.begin();
	for(; it != m_teamMates.end(); ++it)
	{
		if(*it == id)
		{
			if(!active)
			{
				m_teamMates.erase(it);
				return;
			}

			found = true;
			break;
		}
	}

	if(!found && active)
		if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id))
			m_teamMates.push_back(id);
}

void CHUDRadar::SelectTeamMate(EntityId id, bool active)
{
	bool found = false;
	std::vector<EntityId>::iterator it = m_selectedTeamMates.begin();
	for(; it != m_selectedTeamMates.end(); ++it)
	{
		if(*it == id)
		{
			if(!active)
			{
				m_selectedTeamMates.erase(it);
				return;
			}
			else
			{
				found = true;
				break;
			}
		}
	}

	if(!found)
		if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id))
			m_selectedTeamMates.push_back(id);
}

void CHUDRadar::ScanProximity(Vec3 &pos, float &radius)
{
	m_entitiesInProximity.clear();
	m_itemsInProximity.clear();

	SEntityProximityQuery query;
	query.box = AABB( Vec3(pos.x-radius,pos.y-radius,pos.z-radius),
		Vec3(pos.x+radius,pos.y+radius,pos.z+radius) );
	query.nEntityFlags = ENTITY_FLAG_ON_RADAR; // Filter by entity flag.
	gEnv->pEntitySystem->QueryProximity( query );

	IEntity *pActorEntity = g_pGame->GetIGameFramework()->GetClientActor()->GetEntity();

	for(int iEntity=0; iEntity<query.nCount; iEntity++)
	{
		IEntity *pEntity = query.pEntities[iEntity];
		if(pEntity && !pEntity->IsHidden() && pEntity != pActorEntity)
		{
			EntityId id = pEntity->GetId();
			if(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(id))
				m_itemsInProximity.push_back(id);
			else
				m_entitiesInProximity.push_back(id);	//create a list of all nearby entities
		}
	}
}

EntityId CHUDRadar::RayCastBinoculars(CPlayer *pPlayer)
{
	if(!pPlayer)
		return 0;

	Vec3 pos, dir, up, right;
	ray_hit rayHit;

	IMovementController * pMC = pPlayer->GetMovementController();
	if (pMC)
	{
		SMovementState s;
		pMC->GetMovementState(s);
		pos = s.eyePosition;
		dir = s.eyeDirection;
		up = s.upDirection;
		right = (dir % up).GetNormalizedSafe();

		IPhysicalEntity * pPhysEnt = pPlayer->GetEntity()->GetPhysics();

		if(!pPhysEnt)
			return 0;

		static const int obj_types = ent_all; // ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid|ent_living;
		static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;

		right = right * 0.3f;
		up = up * 0.3f;

		Vec3 positions[4];
		positions[0] = pos + right + up;
		positions[1] = pos + right - up;
		positions[2] = pos - right - up;
		positions[3] = pos - right + up;
		for(int i = 0; i < 4; ++i)
		{
			if(gEnv->pPhysicalWorld->RayWorldIntersection( positions[i], 300.0f * dir, obj_types, flags, &rayHit, 1, pPhysEnt ))
			{
				IEntity *pLookAt=gEnv->pEntitySystem->GetEntityFromPhysics(rayHit.pCollider);
				//if(gEnv->pSoundSystem)
				//	gEnv->pSoundSystem->CalcDirectionalAttenuation(pPlayer->GetEntity()->GetWorldPos(), pPlayer->GetViewRotation().GetColumn1(), 0.75f);
				if (pLookAt)
					return pLookAt->GetId();
			}
		}
	}
	return 0;
}

bool CHUDRadar::IsEntityTagged(const EntityId &id) const
{
	for(int i = 0; i < NUM_TAGGED_ENTITIES; ++i)
		if(m_taggedEntities[i] == id)
			return true;
	return false;
}

void CHUDRadar::SetJammer(EntityId id, float radius)
{
	m_jammerID = id;
	m_jammerRadius = radius;

	if(!id && m_jammingValue)
	{
		m_jammingValue = 0.0f;
		m_flashRadar->Invoke("setNoiseValue", 0.0f);
	}
}
