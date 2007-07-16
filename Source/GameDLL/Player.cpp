/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 29:9:2004: Created by Filippo De Luca

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "Player.h"
#include "PlayerView.h"
#include "GameUtils.h"

#include "Weapon.h"
#include "WeaponSystem.h"
#include "OffHand.h"
#include "Fists.h"
#include "GameRules.h"

#include <IViewSystem.h>
#include <IItemSystem.h>
#include <IPhysics.h>
#include <ICryAnimation.h>
#include "IAISystem.h"
#include "IAgent.h"
#include <IVehicleSystem.h>
#include <ISerialize.h>
#include "IMaterialEffects.h"

#include <IRenderAuxGeom.h>
#include <IWorldQuery.h>

#include <IGameTokens.h>

#include <IDebugHistory.h>

#include "PlayerMovementController.h"

#include "HUD/HUD.h"
#include "HUD/HUDRadar.h"
#include "HUD/HUDCrosshair.h"

#include "PlayerMovement.h"
#include "PlayerRotation.h"
#include "PlayerInput.h"
#include "NetPlayerInput.h"
#include "AIDemoInput.h"

#include "VehicleClient.h"

#include "AVMine.h"
#include "Claymore.h"

#include "Network/VoiceListener.h"
#include "Binocular.h"

// enable this to check nan's on position updates... useful for debugging some weird crashes
#define ENABLE_NAN_CHECK

#ifdef ENABLE_NAN_CHECK
#define CHECKQNAN_FLT(x) \
	assert(((*(unsigned*)&(x))&0xff000000) != 0xff000000u && (*(unsigned*)&(x) != 0x7fc00000))
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#define CHECKQNAN_VEC(v) \
	CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)

#define REUSE_VECTOR(table, name, value)	\
	{ if (table->GetValueType(name) != svtObject) \
	{ \
	table->SetValue(name, (value)); \
	} \
		else \
	{ \
	SmartScriptTable v; \
	table->GetValue(name, v); \
	v->SetValue("x", (value).x); \
	v->SetValue("y", (value).y); \
	v->SetValue("z", (value).z); \
	} \
	}

#define RANDOM() ((((float)cry_rand()/(float)RAND_MAX)*2.0f)-1.0f)
#define RANDOMR(a,b) ((float)a + ((cry_rand()/(float)RAND_MAX)*(float)(b-a)))

#define CALL_PLAYER_EVENT_LISTENERS(func) \
{ \
	if (m_playerEventListeners.empty() == false) \
	{ \
		CPlayer::TPlayerEventListeners::const_iterator iter = m_playerEventListeners.begin(); \
		CPlayer::TPlayerEventListeners::const_iterator cur; \
		while (iter != m_playerEventListeners.end()) \
		{ \
			cur = iter; \
			++iter; \
			(*cur)->func; \
		} \
	} \
}

#define SAFE_HUD_FUNC(func)\
	if(g_pGame && g_pGame->GetHUD()) (g_pGame->GetHUD()->func)

#define SAFE_HUD_FUNC_RET(func)\
	((g_pGame && g_pGame->GetHUD()) ? g_pGame->GetHUD()->func : NULL)

//--------------------
//this function will be called from the engine at the right time, since bones editing must be placed at the right time.
int PlayerProcessBones(ICharacterInstance *pCharacter,void *pPlayer)
{
//	return 1; //freezing and bone processing is not working very well.

	//FIXME: do something to remove gEnv->pTimer->GetFrameTime()
	//process bones specific stuff (IK, torso rotation, etc)
	float timeFrame = gEnv->pTimer->GetFrameTime();

	ISkeleton* pISkeleton = pCharacter->GetISkeleton();
	uint32 numAnim = pISkeleton->GetNumAnimsInFIFO(0);
	if (numAnim)
		((CPlayer *)pPlayer)->ProcessBonesRotation(pCharacter, timeFrame);

	return 1;
}
//--------------------

CPlayer::TAlienInterferenceParams CPlayer::m_interferenceParams;

CPlayer::CPlayer()
{
	m_bCrawling = false;
	m_pInteractor = 0;

	for(int i = 0; i < ESound_Player_Last; ++i)
		m_sounds[i] = 0;
	m_sprintTimer = 0.0f;
	m_bSpeedSprint = false;
	m_bHasAssistance = false;
	m_timedemo = false;
	m_ignoreRecoil = false;
	
	m_bDemoModeSpectator = false;

	m_pNanoSuit = NULL;

	m_nParachuteSlot=0;
	m_fParachuteMorph=0;

	m_pVehicleClient = 0;
  m_pMovementDebug = 0;
  m_pDeltaXDebug = 0;
  m_pDeltaYDebug = 0;
  m_pDebugHistory = 0;

	m_mineDisarmStartTime = 0.0f;
	m_mineBeingDisarmed = 0;
	m_disarmingMine = false;

	m_pVoiceListener = NULL;

	m_lastRequestedVelocity.Set(0,0,0);

	m_openingParachute = false;

	m_drownEffectDelay = 0.0f;
	m_underwaterBubblesDelay = 0.0f;

	m_baseQuat.SetIdentity();
	m_eyeOffset.Set(0,0,0);
	m_weaponOffset.Set(0,0,0);
}

CPlayer::~CPlayer()
{
	//stop looping sounds
	for(int i = (int)ESound_Player_First+1; i < (int)ESound_Player_Last;++i)
		PlaySound((EPlayerSounds)i, false);
	if(m_pNanoSuit)
	{
		for(int i = (int)NO_SOUND+1; i < (int)ESound_Suit_Last;++i)
			m_pNanoSuit->PlaySound((ENanoSound)i, 0.0f, true);
	}

	if(IsClient())
	{
		SAFE_HUD_FUNC(PlayerIdSet(0));
	}

	m_pPlayerInput.reset();
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if(pCharacter)
		pCharacter->GetISkeleton()->SetPostProcessCallback0(0,0);
	if(m_pNanoSuit)
		delete m_pNanoSuit;

	if (m_pVehicleClient)
	{
		g_pGame->GetIGameFramework()->GetIVehicleSystem()->RegisterVehicleClient(0);		
		delete m_pVehicleClient;
	}
  
  if(m_pDebugHistory)
    m_pDebugHistory->Release();

	if(m_pVoiceListener)
		GetGameObject()->ReleaseExtension("VoiceListener");
	if (m_pInteractor)
		GetGameObject()->ReleaseExtension("Interactor");
}

bool CPlayer::Init(IGameObject * pGameObject)
{
	if (!CActor::Init(pGameObject))
		return false;

	Revive(true);

	if(GetEntityId() == LOCAL_PLAYER_ENTITY_ID && !m_pNanoSuit) //client player always has a nanosuit (else the HUD doesn't work)
		m_pNanoSuit = new CNanoSuit();

	return true;
}

void CPlayer::PostInit( IGameObject * pGameObject )
{
	CActor::PostInit(pGameObject);

	ResetAnimGraph();

	if (gEnv->bMultiplayer && !gEnv->bServer)
	{
		GetGameObject()->SetUpdateSlotEnableCondition( this, 0, eUEC_VisibleOrInRange );
		//GetGameObject()->ForceUpdateExtension(this, 0);
	}
}

void CPlayer::InitClient(int channelId )
{
	GetGameObject()->InvokeRMI(CActor::ClSetSpectatorMode(), CActor::SetSpectatorModeParams(GetSpectatorMode(), 0), eRMI_ToClientChannel|eRMI_NoLocalCalls, channelId);
}

void CPlayer::InitLocalPlayer()
{
	GetGameObject()->SetUpdateSlotEnableCondition( this, 0, eUEC_WithoutAI );

	m_pVehicleClient = new CVehicleClient;
	m_pVehicleClient->Init();

	IVehicleSystem* pVehicleSystem = g_pGame->GetIGameFramework()->GetIVehicleSystem();
	assert(pVehicleSystem);

	pVehicleSystem->RegisterVehicleClient(m_pVehicleClient);
}

void CPlayer::InitInterference()
{
  m_interferenceParams.clear();
      
  IEntityClassRegistry* pRegistry = gEnv->pEntitySystem->GetClassRegistry();
  IEntityClass* pClass = 0;

  if (pClass = pRegistry->FindClass("Trooper"))    
    m_interferenceParams.insert(std::make_pair(pClass,SAlienInterferenceParams(7.f)));

  if (pClass = pRegistry->FindClass("Scout"))    
    m_interferenceParams.insert(std::make_pair(pClass,SAlienInterferenceParams(25.f)));

  if (pClass = pRegistry->FindClass("Hunter"))    
    m_interferenceParams.insert(std::make_pair(pClass,SAlienInterferenceParams(50.f)));      
}

void CPlayer::BindInputs( IAnimationGraphState * pAGState )
{
	CActor::BindInputs(pAGState);

	if (pAGState)
	{
		m_inputAction = pAGState->GetInputId("Action");
		m_inputItem = pAGState->GetInputId("Item");
		m_inputUsingLookIK = pAGState->GetInputId("UsingLookIK");
		m_inputAiming = pAGState->GetInputId("Aiming");
		m_inputVehicleName = pAGState->GetInputId("Vehicle");
		m_inputVehicleSeat = pAGState->GetInputId("VehicleSeat");
	}

	ResetAnimGraph();
}

void CPlayer::ResetAnimGraph()
{
	if (m_pAnimatedCharacter)
	{
		IAnimationGraphState* pAGState = m_pAnimatedCharacter->GetAnimationGraphState();
		if (pAGState != NULL)
		{
			pAGState->SetInput( m_inputItem, "nw" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( "Signal", "none" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputVehicleName, "none" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputVehicleSeat, 0 );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( "waterLevel", 0 );
			//marcok: m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputHealth, GetHealth() );
		}
		
		m_pAnimatedCharacter->SetParams( m_pAnimatedCharacter->GetParams().ModifyFlags( eACF_ImmediateStance, 0 ) );
	}

	SetStance(STANCE_RELAXED);
}

void CPlayer::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_HIDE || event.event == ENTITY_EVENT_UNHIDE)
	{
		//
	}
	else
	if (event.event == ENTITY_EVENT_INVISIBLE || event.event == ENTITY_EVENT_VISIBLE)
	{
		//
	}	
	else if (event.event == ENTITY_EVENT_XFORM)
	{
		if(gEnv->bMultiplayer)
		{
			// if our local player is spectating this one, move it to this position
			CPlayer* pPlayer = (CPlayer*)gEnv->pGame->GetIGameFramework()->GetClientActor();
			if(pPlayer && pPlayer->GetSpectatorMode() == CPlayer::eASM_Follow && pPlayer->GetSpectatorTarget() == GetEntityId())
			{
				// local player is spectating us. Move them to our position
				pPlayer->MoveToSpectatorTargetPosition();
			}
		}

		int flags = event.nParam[0];
		if (flags & ENTITY_XFORM_ROT && !(flags & (ENTITY_XFORM_USER|ENTITY_XFORM_PHYSICS_STEP)))
		{
			if (flags & (ENTITY_XFORM_TRACKVIEW|ENTITY_XFORM_EDITOR|ENTITY_XFORM_TIMEDEMO))
				m_forcedRotation = true;
			else
				m_forcedRotation = false;

			Quat rotation = GetEntity()->GetRotation();

			if ((m_linkStats.linkID == 0) || ((m_linkStats.flags & LINKED_FREELOOK) == 0))
				{
					m_linkStats.viewQuatLinked = m_linkStats.baseQuatLinked = rotation;
				m_viewQuatFinal = m_viewQuat = m_baseQuat = rotation;
				}
			}

		if (m_timedemo && !(flags&ENTITY_XFORM_TIMEDEMO))
		{
			// Restore saved position.
			GetEntity()->SetPos(m_lastKnownPosition);
		}
		m_lastKnownPosition = GetEntity()->GetPos();
	}
	else if (event.event == ENTITY_EVENT_PREPHYSICSUPDATE)
	{
		if (m_pPlayerInput.get())
			m_pPlayerInput->PreUpdate();
		PrePhysicsUpdate();
	}

	CActor::ProcessEvent(event);

	// needs to be after CActor::ProcessEvent()
	if (event.event == ENTITY_EVENT_RESET)
	{
		//don't do that! equip them like normal weapons
		/*if (g_pGame->GetIGameFramework()->IsServer() && IsClient() && GetISystem()->IsDemoMode()!=2)
		{
			m_pItemSystem->GiveItem(this, "OffHand", false, false, false);
			m_pItemSystem->GiveItem(this, "Fists", false, false, false);
		}*/
	}

	if (event.event == ENTITY_EVENT_PRE_SERIALIZE)
	{
		SEntityEvent event(ENTITY_EVENT_RESET);
		ProcessEvent(event);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::SetViewRotation( const Quat &rotation )
{
	m_baseQuat = rotation;

	m_viewQuat = rotation;
	m_viewQuatFinal = rotation;
}

//////////////////////////////////////////////////////////////////////////
Quat CPlayer::GetViewRotation() const
{
	return m_viewQuatFinal;
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::EnableTimeDemo( bool bTimeDemo )
{
	if (bTimeDemo)
	{
		m_ignoreRecoil = true;
		m_viewAnglesOffset.Set(0,0,0);
	}
	else
	{
		m_ignoreRecoil = false;
		m_viewAnglesOffset.Set(0,0,0);
	}
	m_timedemo = bTimeDemo;
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::Draw(bool draw)
{
	if (!GetEntity())
		return;

	uint32 slotFlags = GetEntity()->GetSlotFlags(0);

	if (draw)
		slotFlags |= ENTITY_SLOT_RENDER;
	else
		slotFlags &= ~ENTITY_SLOT_RENDER;

	GetEntity()->SetSlotFlags(0,slotFlags);
}

//this class is(actually was) used by the function below
class CDelayedFistsSelection : public ISchedulerAction
{
public:
	//
	CDelayedFistsSelection(CFists *fists, CPlayer *plr,const char *actionStr)
  {
		m_actionStr[0] = 0;
		strcpy(m_actionStr,actionStr);
	  m_pFists = fists;
	  m_pPlayer = plr;
  }
	//
  virtual void execute(CItem *item)
  {
	  m_pFists->EnableAnimations(false);
	  gEnv->pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(m_pPlayer,m_pFists->GetEntityId());
	  m_pFists->EnableAnimations(true);
	  m_pFists->PlayAction(m_actionStr);
	  m_pFists->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson,m_actionStr);
  }

private:
	//
	char m_actionStr[32];
  CFists *m_pFists;
	CPlayer *m_pPlayer;
};


//FIXME: this function will need some cleanup
//MR: thats true, check especially client/server 
void CPlayer::UpdateFirstPersonEffects(float frameTime)
{

	//=========================alien interference effect============================
  bool doInterference = g_pGameCVars->hud_enableAlienInterference && !m_interferenceParams.empty();
  if (doInterference)		
	{
		if(CScreenEffects *pSFX = GetScreenEffects())
		{
			//look whether there is an alien around
			float aiStrength = g_pGameCVars->hud_alienInterferenceStrength;
      float interferenceRatio = 0.f;      
      CHUDRadar *pRad = SAFE_HUD_FUNC_RET(GetRadar());
      
			if(pRad && aiStrength > 0.0f)
			{
				const std::vector<EntityId> *pNearbyEntities = pRad->GetNearbyEntities();
        
				if(pNearbyEntities && !pNearbyEntities->empty())
				{
          Vec3 vPos = GetEntity()->GetWorldPos();
          CNanoSuit* pSuit = GetNanoSuit();      
          
					std::vector<EntityId>::const_iterator it = pNearbyEntities->begin();
					std::vector<EntityId>::const_iterator itEnd = pNearbyEntities->end();
          					          
					while (it != itEnd)
					{
						IEntity *pTempEnt = gEnv->pEntitySystem->GetEntity(*it);            
            IEntityClass* pClass = pTempEnt ? pTempEnt->GetClass() : 0;

						if(pClass)
						{ 
              TAlienInterferenceParams::const_iterator it = m_interferenceParams.find(pClass);
              if (it != m_interferenceParams.end())
						  {
                float minDistSq = sqr(it->second.maxdist);
                float distSq = vPos.GetSquaredDistance(pTempEnt->GetWorldPos());
              
                if (distSq < minDistSq)
							  {
                  IActor* pActor = m_pGameFramework->GetIActorSystem()->GetActor(pTempEnt->GetId());
                  if (pActor && pActor->GetHealth() > 0)
                  {
                  float ratio = 1.f - sqrt(distSq)/it->second.maxdist; // linear falloff
                
                  if (ratio > interferenceRatio)                  
                    interferenceRatio = ratio;                  
                }                
							}
						}
						}
						++it;
					}
				}
			}

			if(interferenceRatio != 0.f)
			{	
        float strength = interferenceRatio * aiStrength;                    
			  gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("AlienInterference_Amount", 0.75f*strength);
        SAFE_HUD_FUNC(StartInterference(20.0f*strength, 10.0f*strength, 100.0f, 0.f));
        
        if (!stl::find(m_clientPostEffects, EEffect_AlienInterference))
        {
					m_clientPostEffects.push_back(EEffect_AlienInterference);
					PlaySound(ESound_Fear);
        }
			}
			else
        doInterference = false;			
		}
	}

  if (!doInterference && !m_clientPostEffects.empty() && GetScreenEffects())
	{
    // turn off
		std::vector<EClientPostEffect>::iterator it = m_clientPostEffects.begin();
		for(; it != m_clientPostEffects.end(); ++it)
		{
			if((*it) == EEffect_AlienInterference)
			{
        float aiStrength = g_pGameCVars->hud_alienInterferenceStrength;
        gEnv->pSystem->GetI3DEngine()->SetPostEffectParam("AlienInterference_Amount", 0.0f);
        SAFE_HUD_FUNC(StartInterference(20.0f*aiStrength, 10.0f*aiStrength, 100.0f, 3.f));

        PlaySound(ESound_Fear, false);
				m_clientPostEffects.erase(it);
				break;
			}
		}
	}

	//================================================================================
/*
	if(!m_bUnderwater && !m_bSwimming && (m_stats.waterLevel < 0.75f && m_stats.waterLevel != 0.0f) && m_stats.bSprinting && !GetLinkedVehicle())	//sprinting through water
	{
		gEnv->p3DEngine->SetPostEffectParam("RainDrops_Amount", 2.0f);

		GetScreenEffects()->ClearBlendGroup(15);
		CPostProcessEffect *pRain = new CPostProcessEffect(pEnt->GetId(),"RainDrops_Amount", 0.0f);
		CLinearBlend *pLinear = new CLinearBlend(1.0f);
		GetScreenEffects()->StartBlend(pRain, pLinear, 1.0f, 15);
	}
	else if(m_stats.waterLevel > 1.0f || m_stats.headUnderWater)
	{
		gEnv->p3DEngine->SetPostEffectParam("RainDrops_Amount", 0.0f);
	}
*/

	//================================Weapon Busy when sprinting=============================
	bool sprinting = m_stats.bSprinting;
	if (sprinting && !m_bSprinting)
	{
		// start sprinting, deactivate weapon
		IItem *curItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem());
		if (curItem && !curItem->IsBusy())
		{
			IWeapon *wep = curItem->GetIWeapon();
			if (wep)
				wep->StopFire(GetEntityId());
			curItem->SetBusy(true);
			if(curItem->IsDualWield() && curItem->GetDualWieldSlave())
				curItem->GetDualWieldSlave()->SetBusy(true);

		}
		else
		{
			// item was busy for some other reason
			sprinting = !sprinting;
		}
	}

	if (!sprinting && m_bSprinting)
	{
		CItem *curItem = static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem()));
		if(curItem)
		{
			curItem->SetBusy(false);
			if(curItem->IsDualWield() && curItem->GetDualWieldSlave())
				curItem->GetDualWieldSlave()->SetBusy(false);

			IWeapon *wep = curItem->GetIWeapon();
			if (wep)
				wep->StopFire(GetEntityId());
		}

	}

	m_bSprinting = sprinting;

	//================================================================================================
	CFists *pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));

	//Some weapons can't be used in prone position. 
	// SNH: so we go to crouch instead (done in PlayerInput.cpp)
//  	if(GetStance() == STANCE_PRONE && pFists && !pFists->IsSelected())
//  	{
//  		CItem *curItem = static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem()));
//  		if(curItem && curItem->GetParams().prone_not_usable)
//  		{
//  			curItem->Select(false);
//  			pFists->EnableAnimations(false);
//  			gEnv->pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this,pFists->GetEntityId());
//  			pFists->EnableAnimations(true);
//  		}
//  	}

	pe_status_dynamics dyn;
	IPhysicalEntity *phys = GetEntity()->GetPhysics();
	if (phys)
		phys->GetStatus(&dyn);

	//Deactivate weapon while firing
	bool crawling  = false;
	if(GetStance() == STANCE_PRONE && dyn.v.len2() > 0.1f)
	{
		crawling = true;
		IItem *curItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem());
		if (curItem && !curItem->IsBusy())
		{
			IWeapon *wep = curItem->GetIWeapon();
			if (wep)
				wep->StopFire(GetEntityId());
			curItem->SetBusy(true);
		}

	}

	if(!crawling && m_bCrawling)
	{
		IItem *curItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem());
		if (curItem)
		{
			curItem->SetBusy(false);
		}
	}

	m_bCrawling = crawling;

	//========================================================================================

	//update freefall/parachute effects
	bool freefallChanged(m_stats.inFreefallLast!=m_stats.inFreefall.Value());
	if (m_stats.inFreefall.Value() && pFists)
  {
		bool freeFall = false;
		char buff[32];
		buff[0] = 0;
		if (m_stats.inFreefall.Value()==1)
			if (freefallChanged)
				strcpy(buff,"freefall_start");
			else
			{
				strcpy(buff,"freefall_idle");
				freeFall = true;
			}
		else if (m_stats.inFreefall.Value()==2)
			if (freefallChanged)
			{
				if (m_pNanoSuit)
					m_pNanoSuit->PlaySound(ESound_FreeFall,1,true); //stop sound
				strcpy(buff,"parachute_start");
			}
			else
				strcpy(buff,"parachute_idle");

		pFists->EnterFreeFall(true);
		if (freefallChanged && m_stats.inFreefall.Value()==1)
		{
			if (m_pNanoSuit)
				m_pNanoSuit->PlaySound(ESound_FreeFall);
			CItem *currentItem = (CItem *)gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem());
			if (!currentItem || pFists->GetEntityId()!=currentItem->GetEntityId())
			{
				if (currentItem) 
					GetInventory()->SetLastItem(currentItem->GetEntityId());
				// we have another item selected
				if(pFists->GetEntity()->GetCharacter(0))
					pFists->GetEntity()->GetCharacter(0)->GetISkeleton()->SetDefaultPose();

				// deselect it
				if (currentItem)
					currentItem->PlayAction(g_pItemStrings->deselect, CItem::eIPAF_FirstPerson, false, CItem::eIPAF_Default | CItem::eIPAF_RepeatLastFrame);
				// schedule to start swimming after deselection is finished
				//fists->GetScheduler()->TimerAction(currentItem->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), new CDelayedFistsSelection(fists,this,buff), true);
				pFists->EnableAnimations(false);
				gEnv->pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this,pFists->GetEntityId());
				pFists->EnableAnimations(true);
				//fists->SetBusy(true);
			}
			else
			{
				// we already have the fists up
				GetInventory()->SetLastItem(pFists->GetEntityId());

				//fists->SetBusy(true);
			}
		}

		//play animation
		pFists->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson,buff);
		pFists->PlayAction(buff);
  }
	else if (freefallChanged && !m_stats.inFreefall.Value() && pFists)
	{
		//fists->SetBusy(false);

		if (m_pNanoSuit)
			m_pNanoSuit->PlaySound(ESound_FreeFall,1,true); //stop sound

	  //reactivate last item
		if(!GetLinkedVehicle())
		{
			EntityId lastItemId = GetInventory()->GetLastItem();
			CItem *lastItem = (CItem *)gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(lastItemId);
			pFists->PlayAction(g_pItemStrings->idle);
			pFists->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, g_pItemStrings->idle);
			pFists->ResetAnimation();
			pFists->GetScheduler()->Reset();
			pFists->EnterFreeFall(false);

			if (lastItemId && lastItem && lastItemId != pFists->GetEntityId() && !GetInventory()->GetCurrentItem())
			{
				lastItem->ResetAnimation();
				gEnv->pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this, lastItemId);
			}
		}
  }

	m_stats.inFreefallLast = m_stats.inFreefall.Value();
	m_bUnderwater = (m_stats.headUnderWater > 0.0f);
}

void CPlayer::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden() && !(GetEntity()->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
		return;

	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	IPhysicalEntity *pPE = pEnt->GetPhysics();
	if (pPE && pPE->GetType() == PE_LIVING)
			m_stats.isRagDoll = false;
	else if(m_stats.isRagDoll)
	{
		if(IsClient() && m_fDeathTime)
		{
			float timeSinceDeath = gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_fDeathTime;
			pe_params_part params;
			int headBoneID = GetBoneID(BONE_HEAD);
			if(timeSinceDeath > 3.0f && timeSinceDeath < 8.0f)
			{
				float timeDelta = timeSinceDeath - 3.0f;
				if(headBoneID > -1)
				{
					params.partid  = headBoneID;
					pPE->GetParams(&params);
					if(params.scale > 3.0f)
					{
						params.scale = 8.0f - timeDelta;
						pPE->SetParams(&params);
					}
				}
			}
			else if(timeSinceDeath > 10.0f)
			{
				m_fDeathTime = 0.0f;
				params.flagsAND = geom_colltype_ray;
				pPE->SetParams(&params);
			}
		}
	}

	if (m_stats.isStandingUp)
	{
		m_actions=0;

		if (ICharacterInstance *pCharacter=pEnt->GetCharacter(0))
			if (ISkeleton *pSkeleton=pCharacter->GetISkeleton())
				if (pSkeleton->StandingUp()>=1.5f || pSkeleton->StandingUp()<0.0f)
				{
					m_stats.followCharacterHead=0;
					m_stats.isStandingUp=false;
				}
	}

	CActor::Update(ctx,updateSlot);

	const float frameTime = ctx.fFrameTime;

	bool client(IsClient());

	EAutoDisablePhysicsMode adpm = eADPM_Never;
	if (m_stats.isRagDoll)
		adpm = eADPM_Never;
	else if (client || (gEnv->bMultiplayer && gEnv->bServer))
		adpm = eADPM_Never;
	else if (IsPlayer())
		adpm = eADPM_WhenInvisibleAndFarAway;
	else
		adpm = eADPM_WhenAIDeactivated;
	GetGameObject()->SetAutoDisablePhysicsMode(adpm);

	if (!m_stats.isRagDoll && GetHealth()>0 && (!m_stats.isFrozen.Value() || IsPlayer()))
	{
		if(m_pNanoSuit)
			m_pNanoSuit->Update(frameTime);

		//		UpdateAsLiveAndMobile(ctx); // Callback for inherited CPlayer classes

		UpdateStats(frameTime);

/*
		{
			float weaponLevel = GetStanceInfo(GetStance())->weaponOffset.z - 0.4f;
			float eyeLevel = GetStanceInfo(GetStance())->viewOffset.z - 0.4f;
			bool weaponWellBelowWater = (weaponLevel + 0.05f) < -m_stats.waterLevel;
			bool weaponWellAboveWater = (weaponLevel - 0.05f) > -m_stats.waterLevel;
			bool hasWeaponEquipped = GetCurrentItemId() != 0;
			if (!hasWeaponEquipped && weaponWellAboveWater)
				HolsterItem(false);
			else if (hasWeaponEquipped && weaponWellBelowWater)
				HolsterItem(true);
		}
/**/

		if(m_stats.bSprinting)
		{
			if(!m_sprintTimer)
				m_sprintTimer = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			else if((m_stats.inWater <= 0.0f) && m_stance == STANCE_STAND && !m_sounds[ESound_Run] && (gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_sprintTimer > 3.0f))
				PlaySound(ESound_Run);
			else if(m_sounds[ESound_Run] && ((m_stats.headUnderWater > 0.0f) || m_stance == STANCE_PRONE || m_stance == STANCE_CROUCH))
			{
				PlaySound(ESound_Run, false);
				m_sprintTimer = 0.0f;
			}

			// Report super sprint to AI system.
			if (!m_bSpeedSprint)
			{
				if (m_pNanoSuit && m_pNanoSuit->GetMode() == NANOMODE_SPEED && m_pNanoSuit->GetSprintMultiplier() > 1.0f)
				{
					if (GetEntity() && GetEntity()->GetAI())
						GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_SPRINT, 0);
					CALL_PLAYER_EVENT_LISTENERS(OnSpecialMove(this, IPlayerEventListener::eSM_SpeedSprint));
					m_bSpeedSprint = true;
				}
				else 
					m_bSpeedSprint = false;
			}
		}
		else 
		{
			if(m_sounds[ESound_Run])
			{
				PlaySound(ESound_Run, false);
				PlaySound(ESound_StopRun);
			}
			m_sprintTimer = 0.0f;
			m_bSpeedSprint = false;
		}
	
		if(client)
		{
			UpdateFirstPersonFists();
			UpdateFirstPersonEffects(frameTime);
		}

		if (client)
			GetGameObject()->AttachDistanceChecker();

		UpdateParachute(frameTime);

		//Vec3 camPos(pEnt->GetSlotWorldTM(0) * GetStanceViewOffset(GetStance()));
		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(camPos, 0.05f, ColorB(0,255,0,255) );
	}

	if (m_pPlayerInput.get())
		m_pPlayerInput->Update();
	else
	{
		int demoMode = GetISystem()->IsDemoMode();
		// init input systems if required
		if (GetGameObject()->GetChannelId())
		{
			if (client) //|| ((demoMode == 2) && this == gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetOriginalDemoSpectator()))
			{
				m_pPlayerInput.reset( new CPlayerInput(this) );
			}
			else
				m_pPlayerInput.reset( new CNetPlayerInput(this) );
		}
			else if (demoMode == 2)
			m_pPlayerInput.reset( new CNetPlayerInput(this) );
			//{
			//	if(demoMode == 1)
			//		m_pPlayerInput.reset( new CAIDemoInput(this) );
			//	else if(demoMode == 2)
			//		m_pPlayerInput.reset( new CNetPlayerInput(this) );
			//}

		if (m_pPlayerInput.get())
			GetGameObject()->EnablePostUpdates(this);
	}

	if(m_disarmingMine)
	{
		if(GetGameObject()->GetWorldQuery()->GetLookAtEntityId() != m_mineBeingDisarmed)
		{
			// only on clients - servers don't know what they are looking at
			if(IsClient())
				EndDisarmMine(m_mineBeingDisarmed);
		}

		if(gEnv->bServer)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_mineBeingDisarmed);
			if(pEntity)
			{
				float fTime = 0.0f;

				if(pEntity->GetClass() == CItem::sClaymoreExplosiveClass)
					fTime = CClaymore::GetDisarmTime();
				else if(pEntity->GetClass() == CItem::sAVExplosiveClass)
					fTime = CAVMine::GetDisarmTime();

				if(gEnv->pTimer->GetCurrTime() >= (fTime + m_mineDisarmStartTime))
				{
					RemoveMineEntity(m_mineBeingDisarmed);
				}
			}
		}
	}

	// update voice listener for non-local players
	if(gEnv->bMultiplayer && !IsClient())
	{
		// remote players need a voice listener
		if(!m_pVoiceListener)
			m_pVoiceListener = GetGameObject()->AcquireExtension("VoiceListener");

		// update it
		if(m_pVoiceListener)
		{
			CVoiceListener* pListener = (CVoiceListener*)m_pVoiceListener;
			pListener->UpdateSound3dPan();
		}
	}
	if (IsClient())
	{
		if (!m_pInteractor)
			m_pInteractor = GetGameObject()->AcquireExtension("Interactor");
	}

	// small hack for ded server: fake a view update
	if (gEnv->bMultiplayer && gEnv->bServer && !IsClient())
	{
		SViewParams viewParams;
		UpdateView(viewParams);
	}
}

void CPlayer::UpdateParachute(float frameTime)
{
	//FIXME:check if the player has the parachute
	if (m_parachuteEnabled && (m_stats.inFreefall.Value()==1) && (m_actions&ACTION_JUMP))
	{
		ChangeParachuteState(3);
	}
	else if (m_stats.inFreefall.Value()==2)
	{
		// update parachute morph
		m_fParachuteMorph+=frameTime;
		if (m_fParachuteMorph>1.0f)
			m_fParachuteMorph=1.0f;
		if (m_nParachuteSlot) 
		{			
			ICharacterInstance *pCharacter=GetEntity()->GetCharacter(m_nParachuteSlot);				
			if (pCharacter)
				pCharacter->SetLinearMorphSequence(m_fParachuteMorph);
		}
	}
	else if(m_stats.inFreefall.Value() == 3)
	{
		if(m_openingParachute)
		{
			m_openParachuteTimer -= frameTime;
		}
		if(m_openingParachute && m_openParachuteTimer<0.0f)
		{
			ChangeParachuteState(2);
			m_openingParachute = false;
		}
	}
	else if (m_stats.inFreefall.Value()==-1)
	{
		ChangeParachuteState(0);
	}
}

bool CPlayer::ShouldUsePhysicsMovement()
{

	//swimming use physics, for everyone
	if (m_stats.inWater>0.01f)
		return true;

	if (m_stats.inAir>0.01f && InZeroG())
		return true;

	//players
	if (IsPlayer())
	{
		//the client will be use physics always but when in thirdperson
		if (IsClient())
		{
			if (!IsThirdPerson()/* || m_stats.inAir>0.01f*/)
				return true;
			else
				return false;
		}

		//other clients will use physics always
		return true;
	}

	//in demo playback - use physics for recorded entities
	if(GetISystem()->IsDemoMode() == 2)
		return true;
	
	//AIs in normal conditions are supposed to use LM
	return false;
}

void CPlayer::ProcessCharacterOffset()
{
	if (m_linkStats.CanMoveCharacter() && !m_stats.followCharacterHead.Value())
	{
		IEntity *pEnt = GetEntity();

		if (IsClient() && !IsThirdPerson()) 
		{
			float lookDown(m_viewQuatFinal.GetColumn1() * m_baseQuat.GetColumn2());
			float offsetZ(m_modelOffset.z);


			//first try to match the character head position to the real camera position
//			Vec3 vLocalEyePos	=	Vec3(0.0f,0.0f,1.6f); 
		//	Vec3 vLocalEyePos	=	GetLocalEyePos();
			Vec3 vLocalEyePos	=	GetLocalEyePos2();
			Vec3 vStanceViewOffset = GetStanceViewOffset(m_stance);
			m_modelOffset = vStanceViewOffset-vLocalEyePos;

			//to make sure the character doesn't sink in the ground
			if (m_stats.onGround > 0.0f)
				m_modelOffset.z = offsetZ;

			if (m_stats.inAir > 0.0f)
				m_modelOffset.z = -1;


			//FIXME:this is unavoidable, the character offset must be adjusted for every stance/situation. 
			//more elegant ways to do this (like check if the hip bone position is inside the view) apparently dont work, its easier to just tweak the offset manually.
			if (m_stats.inFreefall.Value()==2)
			{
				m_modelOffset.y -= max(-lookDown,0.0f)*0.25f;
			}
			else if (m_stats.inWater>0.1f)// && m_stats.speed>0.1f)
			{
				m_modelOffset.y -= max(-lookDown,0.0f)*0.65f;
			}
			else
			{
				m_modelOffset.y -= max(-lookDown,0.0f)*0.25f;
			}
		}

	/*
		SEntitySlotInfo slotInfo; 
		pEnt->GetSlotInfo( 0, slotInfo );

		Matrix34 LocalTM=Matrix34(IDENTITY);	
		if (slotInfo.pLocalTM)
			LocalTM=*slotInfo.pLocalTM;	

		LocalTM.m03 += m_modelOffset.x;
		LocalTM.m13 += m_modelOffset.y;
		LocalTM.m23 += m_modelOffset.z;
	  pEnt->SetSlotLocalTM(0,LocalTM);
	*/
		m_modelOffset.z = 0.0f;

		GetAnimatedCharacter()->SetExtraAnimationOffset(QuatT(m_modelOffset, IDENTITY));
	}
}

void CPlayer::PrePhysicsUpdate()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// TODO: This whole function needs to be optimized.
	// TODO: Especially when characters are dead, alot of stuff here can be skipped.

	if (!m_pAnimatedCharacter)
		return;

	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden() && !(GetEntity()->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
		return;

	//HACK - Avoid collision with grabbed NPC - Beni
	/*if(m_pHumanGrabEntity && !m_throwingNPC)
	{
		IMovementController * pMC = GetMovementController();
		if(pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);

			Matrix34 prePhysics = m_pHumanGrabEntity->GetWorldTM();
			prePhysics.AddTranslation(info.eyeDirection*0.5f);
			m_pHumanGrabEntity->SetWorldTM(prePhysics);
		}

	}*/

	if (m_pMovementController)
	{
		if (g_pGame->GetCVars()->g_enableIdleCheck)
		{
			// if (gEnv->pGame->GetIGameFramework()->IsEditing() == false || pVar->GetIVal() == 2410)
			{
				IActorMovementController::SStats stats;
				if (m_pMovementController->GetStats(stats) && stats.idle == true)
				{
					if (GetGameObject()->IsProbablyVisible()==false && GetGameObject()->IsProbablyDistant() )
					{
						CPlayerMovementController* pMC = static_cast<CPlayerMovementController*> (m_pMovementController);
						float frameTime = gEnv->pTimer->GetFrameTime();
						pMC->IdleUpdate(frameTime);
						return;
					}
				}		
			}
		}
  }

	bool client(IsClient());
	float frameTime = gEnv->pTimer->GetFrameTime();

	//FIXME:
	// small hack to make first person ignore animation speed, and everything else use it
	// will need to be reconsidered for multiplayer
	SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
	params.flags &= ~(eACF_AlwaysAnimation | eACF_PerAnimGraph | eACF_AlwaysPhysics | eACF_ConstrainDesiredSpeedToXY | eACF_NoLMErrorCorrection);

	if (ShouldUsePhysicsMovement())
	{
		params.flags |= eACF_AlwaysPhysics | eACF_ImmediateStance | eACF_NoLMErrorCorrection;

		if ((gEnv->bMultiplayer && !client) || IsThirdPerson())
		{	
			params.physErrorInnerRadiusFactor = 0.05f;
			params.physErrorOuterRadiusFactor = 0.2f;
			params.physErrorMinOuterRadius = 0.2f;
			params.physErrorMaxOuterRadius = 0.5f;
			params.flags |= eACF_UseHumanBlending;
			params.flags &= ~(eACF_NoLMErrorCorrection | eACF_AlwaysPhysics);
			params.flags |= eACF_PerAnimGraph;
		}
	}
	else
		params.flags |= eACF_ImmediateStance | eACF_PerAnimGraph | eACF_UseHumanBlending | eACF_ConstrainDesiredSpeedToXY;

	if (IsPlayer())
	{
		params.flags &= ~(eACF_AlwaysAnimation | eACF_PerAnimGraph);
		params.flags |= eACF_AlwaysPhysics;
	}

	params.flags &= ~eACF_Frozen;
	if (IsFrozen())
		params.flags |= eACF_Frozen;

	m_pAnimatedCharacter->SetParams(params);
	bool fpMode = true;
	if (!IsPlayer())
		fpMode = false;
	m_pAnimatedCharacter->GetAnimationGraphState()->SetFirstPersonMode( fpMode );

	if (m_pMovementController)
	{
		SActorFrameMovementParams frameMovementParams;
		if (m_pMovementController->Update(frameTime, frameMovementParams))
		{
#ifdef _DEBUG
			if(m_pMovementDebug)
				m_pMovementDebug->AddValue(frameMovementParams.desiredVelocity.len());
			if(m_pDeltaXDebug)
				m_pDeltaXDebug->AddValue(frameMovementParams.desiredVelocity.x);
			if(m_pDeltaYDebug)
				m_pDeltaYDebug->AddValue(frameMovementParams.desiredVelocity.y);
#endif

      if (m_linkStats.CanRotate())
			{
				Quat baseQuatBackup(m_baseQuat);

				// process and apply movement requests
				CPlayerRotation playerRotation( *this, frameMovementParams, frameTime );
				playerRotation.Process();
				playerRotation.Commit(*this);

				if (m_forcedRotation)
				{
					//m_baseQuat = baseQuatBackup;
					m_forcedRotation = false;
				}
			}
			else
			{
				Quat baseQuatBackup(m_baseQuat);

				// process and apply movement requests
				frameMovementParams.deltaAngles.x = 0.0f;
				frameMovementParams.deltaAngles.y = 0.0f;
				frameMovementParams.deltaAngles.z = 0.0f;
				CPlayerRotation playerRotation( *this, frameMovementParams, frameTime );
				playerRotation.Process();
				playerRotation.Commit(*this);

				if (m_forcedRotation)
				{
					//m_baseQuat = baseQuatBackup;
					m_forcedRotation = false;
				}
			}

			if (m_linkStats.CanMove())
			{
				CPlayerMovement playerMovement( *this, frameMovementParams, frameTime );
				playerMovement.Process(*this);
				playerMovement.Commit(*this);

				if(m_stats.forceCrouchTime>0.0f)
					m_stats.forceCrouchTime -= frameTime;

				if (m_stats.inAir && m_stats.inZeroG)
					SetStance(STANCE_ZEROG);
				else if (ShouldSwim())
					SetStance(STANCE_SWIM);
				else if (m_stats.bSprinting && m_stance != STANCE_PRONE && m_pNanoSuit && m_pNanoSuit->GetMode()==NANOMODE_SPEED && m_pNanoSuit->GetSuitEnergy()>10.0f)
					SetStance(STANCE_STAND);
				else if(m_stats.forceCrouchTime>0.0f)
					SetStance(STANCE_CROUCH);
				else if (frameMovementParams.stance != STANCE_NULL)
					SetStance(frameMovementParams.stance);

			}
			else
			{
				frameMovementParams.desiredVelocity = ZERO;
				CPlayerMovement playerMovement( *this, frameMovementParams, frameTime );
				playerMovement.Process(*this);
				playerMovement.Commit(*this);
			}

			if (m_linkStats.CanDoIK())
				SetIK(frameMovementParams);
		}
	}

	//offset the character so its hip is at entity's origin
	ICharacterInstance *pCharacter = pEnt ? pEnt->GetCharacter(0) : NULL;

	if (pCharacter)
	{
		ProcessCharacterOffset();

		if (client||(IsPlayer()&&gEnv->bServer))
			pCharacter->GetISkeleton()->SetForceSkeletonUpdate(4);

		if (client)
		{
			// clear the players look target every frame
			if (m_pMovementController)
			{
				CMovementRequest mr;
				mr.ClearLookTarget();
				m_pMovementController->RequestMovement( mr );
			}
		}
	}
	//

	if (m_pMovementController)
		m_pMovementController->PostUpdate(frameTime);

  Debug();

	m_actions &= ~ACTION_MOVE;
}

IActorMovementController * CPlayer::CreateMovementController()
{
	return new CPlayerMovementController(this);
}

void CPlayer::SetIK( const SActorFrameMovementParams& frameMovementParams )
{
	if (!m_pAnimatedCharacter)
		return;

//	if (!IsThirdPerson() && !IsPlayer())
//		return;

	IAnimationGraphState * pGraph = m_pAnimatedCharacter?m_pAnimatedCharacter->GetAnimationGraphState():NULL;
	if (!pGraph)
		return;

	SMovementState curMovementState;
	m_pMovementController->GetMovementState(curMovementState);

	IEntity * pEntity = GetEntity();
	pGraph->SetInput( m_inputUsingLookIK, int(frameMovementParams.lookIK || frameMovementParams.aimIK) );
	if (ICharacterInstance * pCharacter = pEntity->GetCharacter(0))
	{
		ISkeleton * pSkeleton = pCharacter->GetISkeleton();
		if (frameMovementParams.lookIK && !m_stats.isGrabbed && (!GetAnimatedCharacter() || GetAnimatedCharacter()->IsLookIkAllowed()))
		{
			if(IsClient())		
			{
				float lookIKBlends[5];
				// Emphasize the head more for the player.
				lookIKBlends[0] = 0.04f;		// spine 1
				lookIKBlends[1] = 0.06f;		// spine 2
				lookIKBlends[2] = 0.08f;		// spine 3
				lookIKBlends[3] = 0.10f;		// neck
				lookIKBlends[4] = 0.85f;		// head   // 0.60f

				pSkeleton->SetLookIK( true, /*gf_PI*0.7f*/ DEG2RAD(80), frameMovementParams.lookTarget,lookIKBlends );
			}
			else
			{
				//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(curMovementState.eyePosition , ColorB(255,155,155,255), frameMovementParams.lookTarget, ColorB(255,155,155,255));
				//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(curMovementState.eyePosition , ColorB(155,255,155,255), curMovementState.eyePosition + m_viewQuat.GetColumn1() * 2.0f, ColorB(155,255,155,255));
				pSkeleton->SetLookIK( true, /*gf_PI*0.7f*/ DEG2RAD(80), frameMovementParams.lookTarget);
			}
		}
		else
			pSkeleton->SetLookIK( false, 0, frameMovementParams.lookTarget );

		Vec3 aimTarget = frameMovementParams.aimTarget;
		bool aimEnabled = frameMovementParams.aimIK;
		if (IsPlayer())
		{
			SMovementState info;
			m_pMovementController->GetMovementState(info);
			aimTarget = info.eyePosition + info.aimDirection * 5.0f; // If this is too close the aiming will fade out.
			aimEnabled = true;

			// TODO: This should probably be moved somewhere else and not done every frame.
			ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
			if (pCharacter)
				pCharacter->GetISkeleton()->SetAimIKFadeOut(0);
		}
		else
		{
			// Even for AI we need to guarantee a valid aim target position (only needed for LAW/Rockets though).
			// We should not set aimEnabled to true though, because that will force aiming for all weapons.
			// The AG templates will communicate an animation synched "force aim" flag to the animation playback.
			if (!aimEnabled)
			{
				if (frameMovementParams.lookIK)
				{
					// Use look target
					aimTarget = frameMovementParams.lookTarget;
				}
				else
				{
					// Synthesize target
					SMovementState info;
					m_pMovementController->GetMovementState(info);
					aimTarget = info.eyePosition + info.aimDirection * 5.0f; // If this is too close the aiming will fade out.
				}
			}
		}
		pSkeleton->SetAimIK( aimEnabled, aimTarget );
		pGraph->SetInput( m_inputAiming, aimEnabled ? 1 : 0 );


/*
		if (frameMovementParams.aimIK)
		{
			ICVar *pg_aimDebug = gEnv->pConsole->GetCVar("g_aimdebug");
			if (pg_aimDebug && pg_aimDebug->GetIVal()!=0)
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(  frameMovementParams.aimTarget, 0.5f, ColorB(255,0,255,255) );
		}
*/
	}
}

void CPlayer::UpdateView(SViewParams &viewParams)
{
	if (!m_pAnimatedCharacter)
		return;

	CPlayerView playerView(*this,viewParams);
	playerView.Process(viewParams);
	playerView.Commit(*this,viewParams);	

	if (!IsThirdPerson())
  {
    float animControlled = m_pAnimatedCharacter->FilterView(viewParams);
		
    if (animControlled >= 1.f)
    { 
      m_baseQuat = m_viewQuat = m_viewQuatFinal = viewParams.rotation;
      m_lastAnimContPos = viewParams.position;
      m_lastAnimContRot = viewParams.rotation;
    }
    else if (animControlled>0.f && m_lastAnimControlled > animControlled)
    {
      m_viewQuat = m_viewQuatFinal = m_lastAnimContRot;
      viewParams.position = m_lastAnimContPos;
      viewParams.rotation = m_lastAnimContRot;
    }    

    m_lastAnimControlled = animControlled;

		if (CItem *pItem=GetItem(GetInventory()->GetCurrentItem()))
		{
			if (pItem->FilterView(viewParams))
				m_baseQuat = m_viewQuat = m_viewQuatFinal = viewParams.rotation;
		}
  }

	viewParams.blend = m_viewBlending;
	m_viewBlending=true;	// only disable blending for one frame

	//store the view matrix, without vertical component tough, since its going to be used by the VectorToLocal function.
	Vec3 forward(viewParams.rotation.GetColumn1());
	Vec3 up(m_baseQuat.GetColumn2());
	Vec3 right(-(up % forward));

	m_clientViewMatrix.SetFromVectors(right,up%right,up,viewParams.position);
	m_clientViewMatrix.OrthonormalizeFast();

	//keep track for the client fov, used for the concentration mood
	m_stats.viewFov = viewParams.fov;

	// finally, update the network system
	if (gEnv->bMultiplayer)
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedFov( GetEntityId(), viewParams.fov );
}

void CPlayer::PostUpdateView(SViewParams &viewParams)
{

	Vec3 shakeVec(viewParams.currentShakeShift*0.85f);
	m_stats.FPWeaponPos = viewParams.position + m_stats.FPWeaponPosOffset + shakeVec;;

	Quat wQuat(viewParams.rotation*Quat::CreateRotationXYZ(m_stats.FPWeaponAnglesOffset * gf_PI/180.0f));
	wQuat *= Quat::CreateSlerp(viewParams.currentShakeQuat,IDENTITY,0.5f);
	wQuat.Normalize();

	m_stats.FPWeaponAngles = Ang3(wQuat);	
	m_stats.FPSecWeaponPos = m_stats.FPWeaponPos;
	m_stats.FPSecWeaponAngles = m_stats.FPWeaponAngles;

	
	if (CItem *pItem=GetItem(GetInventory()->GetCurrentItem()))
		pItem->PostFilterView(viewParams);

	//Update grabbed NPC if needed
	COffHand * pOffHand=static_cast<COffHand*>(GetWeaponByClass(CItem::sOffHandClass));
	if(pOffHand && (pOffHand->GetOffHandState()&(eOHS_GRABBING_NPC|eOHS_HOLDING_NPC|eOHS_THROWING_NPC)))
		pOffHand->PostFilterView(viewParams);

}

void CPlayer::RegisterPlayerEventListener(IPlayerEventListener *pPlayerEventListener)
{
	stl::push_back_unique(m_playerEventListeners, pPlayerEventListener);
}

void CPlayer::UnregisterPlayerEventListener(IPlayerEventListener *pPlayerEventListener)
{
	stl::find_and_erase(m_playerEventListeners, pPlayerEventListener);
}

IEntity *CPlayer::LinkToVehicle(EntityId vehicleId) 
{
	IEntity *pLinkedEntity = CActor::LinkToVehicle(vehicleId);

	if (pLinkedEntity)
	{
		CHECKQNAN_VEC(m_modelOffset);
		m_modelOffset.Set(0,0,0);
		GetAnimatedCharacter()->SetExtraAnimationOffset(QuatT(m_modelOffset, IDENTITY));
		CHECKQNAN_VEC(m_modelOffset);

//		ResetAnimations();

		IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(vehicleId);
		
		if (IVehicleSeat *pSeat = pVehicle->GetSeatForPassenger(GetEntity()->GetId()))
		{
			CALL_PLAYER_EVENT_LISTENERS(OnEnterVehicle(this,pVehicle->GetEntity()->GetClass()->GetName(),pSeat->GetSeatName(),m_stats.isThirdPerson));
		}
	}
	else
	{
    if (IsThirdPerson())
      ToggleThirdPerson();

		CALL_PLAYER_EVENT_LISTENERS(OnExitVehicle(this));
	}

	return pLinkedEntity;
}

IEntity *CPlayer::LinkToEntity(EntityId entityId) 
{
	IEntity *pLinkedEntity = CActor::LinkToEntity(entityId);

	if (pLinkedEntity)
	{
		CHECKQNAN_VEC(m_modelOffset);
		m_modelOffset.Set(0,0,0);
		GetAnimatedCharacter()->SetExtraAnimationOffset(QuatT(m_modelOffset, IDENTITY));
		CHECKQNAN_VEC(m_modelOffset);

		m_baseQuat.SetIdentity();
		m_viewQuat.SetIdentity();
		m_viewQuatFinal.SetIdentity();
	}

	return pLinkedEntity;
}

void CPlayer::LinkToMountedWeapon(EntityId weaponId) 
{
	m_stats.mountedWeaponID = weaponId;

	SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
	
	if (weaponId)
	{
		if (!IsClient())
			params.flags &= ~eACF_EnableMovementProcessing;
		params.flags |= eACF_NoLMErrorCorrection;
	}
	else
	{
		if (!IsClient())
			params.flags |= eACF_EnableMovementProcessing;
		params.flags &= ~eACF_NoLMErrorCorrection;
	}
	
	m_pAnimatedCharacter->SetParams( params );
}

void CPlayer::StanceChanged(EStance last)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CHECKQNAN_VEC(m_modelOffset);
	if(IsPlayer())
	{
		float delta(GetStanceInfo(last)->modelOffset.z - GetStanceInfo(m_stance)->modelOffset.z);
		if (delta>0.0f)
			m_modelOffset.z -= delta;
	}
	CHECKQNAN_VEC(m_modelOffset);

	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();

	//this is to keep track of the eyes height
	if (pPhysEnt)
	{
		pe_player_dimensions playerDim;
		pPhysEnt->GetParams(&playerDim);
		m_stats.heightPivot = playerDim.heightPivot;
	}

	if(GetEntity() && GetEntity()->GetAI())
	{
		IAIActor* pAIActor = CastToIAIActorSafe(GetEntity()->GetAI());
		if (pAIActor)
		{
			int iGroupID = pAIActor->GetParameters().m_nGroup;
			IAIObject* pLeader = gEnv->pAISystem->GetLeaderAIObject(iGroupID);
			if(pLeader==GetEntity()->GetAI()) // if the leader is changing stance
			{
				IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
				pData->iValue = m_stance;
				gEnv->pAISystem->SendSignal(SIGNALFILTER_LEADER,1,"OnChangeStance",pLeader,pData);
			}
		}
	}

	bool player(IsPlayer());

	//TODO:move the dive impulse in the processmovement function, I want all the movement related there.
	//and remove the client check!
	if (pPhysEnt && player && m_stance == STANCE_PRONE && m_stats.speedFlat>1.0)
	{
		pe_action_impulse actionImp;

		Vec3 diveDir(m_stats.velocity.GetNormalized());
		diveDir += m_baseQuat.GetColumn2() * 0.35f;

		actionImp.impulse = diveDir.GetNormalized() * m_stats.mass * 3.0f;
		actionImp.iApplyTime = 0;
		pPhysEnt->Action(&actionImp);
	}

	CALL_PLAYER_EVENT_LISTENERS(OnStanceChanged(this, m_stance));
/*
	if (!player)
		m_stats.waitStance = 1.0f;
	else if (m_stance == STANCE_PRONE || last == STANCE_PRONE)
		m_stats.waitStance = 0.5f;
*/
}

//------------------------------------------------------
bool CPlayer::TrySetStance(EStance stance)
{
//  if (stance == STANCE_NULL)
//	  return true;

  IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();
  int result = 0;
  if (pPhysEnt)
  {
    const SStanceInfo *sInfo = GetStanceInfo(stance);

    pe_player_dimensions playerDim;
    playerDim.heightEye = 0.0f;
    playerDim.heightCollider = sInfo->heightCollider;
    playerDim.sizeCollider = sInfo->size;
    playerDim.heightPivot = sInfo->heightPivot;
    playerDim.maxUnproj = max(0.0f,sInfo->heightPivot);
    playerDim.bUseCapsule = sInfo->useCapsule;

    result = pPhysEnt->SetParams(&playerDim);

    if (result != 0)
    {
      // we successfully set the stance collider
      m_forceUpdateStanceCollider = false;
      m_stanceCollider = m_stance;
    }
    else
    {
      // we need to continue setting it until we reach a stable state.
      m_forceUpdateStanceCollider = true;
    }
  }

  return (result != 0);
}

float CPlayer::CalculatePseudoSpeed(bool wantSprint) const
{
	CNanoSuit* pSuit = GetNanoSuit();
	bool inSpeedMode = ((pSuit != NULL) && (pSuit->GetMode() == NANOMODE_SPEED));
	bool hasEnergy = (pSuit == NULL) || (pSuit->GetSuitEnergy() > (0.2f * NANOSUIT_ENERGY)); // AI has no suit and can sprint endlessly.

	if (!wantSprint)
		return 1.0f;
	else if (!inSpeedMode || !hasEnergy)
		return 2.0f;
	else
		return 3.0f;
}

float CPlayer::GetStanceMaxSpeed(EStance stance) const
{
	return GetStanceInfo(stance)->maxSpeed * m_params.speedMultiplier;
}

float CPlayer::GetStanceNormalSpeed(EStance stance) const
{
	return GetStanceInfo(stance)->normalSpeed * m_params.speedMultiplier;
}

void CPlayer::SetParams(SmartScriptTable &rTable,bool resetFirst)
{
	//not sure about this
	if (resetFirst)
	{
		m_params = SPlayerParams();
	}

	CActor::SetParams(rTable,resetFirst);

	CScriptSetGetChain params(rTable);
	params.GetValue("sprintMultiplier",m_params.sprintMultiplier);
	params.GetValue("strafeMultiplier",m_params.strafeMultiplier);
	params.GetValue("backwardMultiplier",m_params.backwardMultiplier);
	params.GetValue("grabMultiplier",m_params.grabMultiplier);
	params.GetValue("afterburnerMultiplier",m_params.afterburnerMultiplier);

	params.GetValue("inertia",m_params.inertia);
	params.GetValue("inertiaAccel",m_params.inertiaAccel);

	params.GetValue("jumpHeight",m_params.jumpHeight);

	params.GetValue("slopeSlowdown",m_params.slopeSlowdown);

	params.GetValue("leanShift",m_params.leanShift);
	params.GetValue("leanAngle",m_params.leanAngle);

	params.GetValue("thrusterImpulse",m_params.thrusterImpulse);
	params.GetValue("thrusterStabilizeImpulse",m_params.thrusterStabilizeImpulse);

	params.GetValue("gravityBootsMultipler",m_params.gravityBootsMultipler);

	params.GetValue("headBobbingMultiplier",m_params.headBobbingMultiplier);
	params.GetValue("weaponBobbingMultiplier",m_params.weaponBobbingMultiplier);
	params.GetValue("weaponInertiaMultiplier",m_params.weaponInertiaMultiplier);

	params.GetValue("viewPivot",m_params.viewPivot);
	params.GetValue("viewDistance",m_params.viewDistance);
	params.GetValue("viewHeightOffset",m_params.viewHeightOffset);

	params.GetValue("speedMultiplier",m_params.speedMultiplier);

	//view related
	params.GetValue("viewLimitDir",m_params.vLimitDir);
	params.GetValue("viewLimitYaw",m_params.vLimitRangeH);
	params.GetValue("viewLimitPitch",m_params.vLimitRangeV);

	params.GetValue("hudOffset", m_params.hudOffset);
	params.GetValue("hudAngleOffset", (Vec3 &)m_params.hudAngleOffset);

	params.GetValue("viewFoVScale",m_params.viewFoVScale);
	params.GetValue("viewSensitivity",m_params.viewSensitivity);

	//TODO:move to SetStats()
	int followHead=m_stats.followCharacterHead.Value();
	params.GetValue("followCharacterHead", followHead);
	m_stats.followCharacterHead=followHead;

	//
	SmartScriptTable nanoTable;
	if (m_pNanoSuit && params.GetValue("nanoSuit",nanoTable))
	{
		m_pNanoSuit->SetParams(nanoTable,resetFirst);
	}
}

//fill the status table for the scripts
bool CPlayer::GetParams(SmartScriptTable &rTable)
{
	CScriptSetGetChain params(rTable);

	params.SetValue("sprintMultiplier", m_params.sprintMultiplier);
	params.SetValue("strafeMultiplier", m_params.strafeMultiplier);
	params.SetValue("backwardMultiplier", m_params.backwardMultiplier);
	params.SetValue("grabMultiplier",m_params.grabMultiplier);

	params.SetValue("jumpHeight", m_params.jumpHeight);
	params.SetValue("leanShift", m_params.leanShift);
	params.SetValue("leanAngle", m_params.leanAngle);
	params.SetValue("thrusterImpulse", m_params.thrusterImpulse);
	params.SetValue("thrusterStabilizeImpulse", m_params.thrusterStabilizeImpulse);
	params.SetValue("gravityBootsMultiplier", m_params.gravityBootsMultipler);


	params.SetValue("headBobbingMultiplier",m_params.headBobbingMultiplier);
	params.SetValue("weaponInertiaMultiplier",m_params.weaponInertiaMultiplier);
	params.SetValue("weaponBobbingMultiplier",m_params.weaponBobbingMultiplier);

	params.SetValue("speedMultiplier",m_params.speedMultiplier);

	REUSE_VECTOR(rTable, "viewPivot", m_params.viewPivot);

	params.SetValue("viewDistance",m_params.viewDistance);
	params.SetValue("viewHeightOffset",m_params.viewHeightOffset);

	REUSE_VECTOR(rTable, "hudOffset", m_params.hudOffset);
	REUSE_VECTOR(rTable, "hudAngleOffset", m_params.hudAngleOffset);

	//view related
	REUSE_VECTOR(rTable, "viewLimitDir",m_params.vLimitDir);
	params.SetValue("viewLimitYaw",m_params.vLimitRangeH);
	params.SetValue("viewLimitPitch",m_params.vLimitRangeV);

	params.SetValue("viewFoVScale",m_params.viewFoVScale);
	params.SetValue("viewSensitivity",m_params.viewSensitivity);

	return true;
}
void CPlayer::SetStats(SmartScriptTable &rTable)
{
	CActor::SetStats(rTable);

	rTable->GetValue("inFiring",m_stats.inFiring);
}

//fill the status table for the scripts
void CPlayer::UpdateScriptStats(SmartScriptTable &rTable)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	CActor::UpdateScriptStats(rTable);

	CScriptSetGetChain stats(rTable);

	m_stats.isFrozen.SetDirtyValue(stats, "isFrozen");
	//stats.SetValue("isWalkingOnWater",m_stats.isWalkingOnWater);
	
	//stats.SetValue("shakeAmount",m_stats.shakeAmount);	
	m_stats.followCharacterHead.SetDirtyValue(stats, "followCharacterHead");
	m_stats.firstPersonBody.SetDirtyValue(stats, "firstPersonBody");

	stats.SetValue("gravityBoots",GravityBootsOn());
	m_stats.inFreefall.SetDirtyValue(stats, "inFreeFall");

	//view Fov, in degrees
	stats.SetValue("viewFov",(m_stats.viewFov*(180.0f/gf_PI)));

	//nanosuit stats
	//FIXME:create a CNanoSuit::GetStats instead?
	//stats.SetValue("nanoSuitHeal", (m_pNanoSuit)?m_pNanoSuit->GetHealthRegenRate():0);
	stats.SetValue("nanoSuitArmor",(m_pNanoSuit)?m_pNanoSuit->GetSlotValue(NANOSLOT_ARMOR):0);
	stats.SetValue("nanoSuitStrength", (m_pNanoSuit)?m_pNanoSuit->GetSlotValue(NANOSLOT_STRENGTH):0);
	//stats.SetValue("cloakState",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetState():0);	
	//stats.SetValue("visualDamp",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetVisualDamp():0);
	stats.SetValue("soundDamp",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetSoundDamp():0);
	//stats.SetValue("heatDamp",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetHeatDamp():0);
}

//------------------------------------------------------------------------
bool CPlayer::ShouldSwim()
{
	if (m_stats.bottomDepth < 1.3f)
		return false;

	if ((m_stats.bottomDepth < 2.0f) && (m_stats.inWater > -1.0f) && (m_stats.inWater < -0.0f))
		return false;

	if (m_stats.waterLevel > 3.0f)
		return false;

	if ((m_stats.waterLevel > 0.1f) && (m_stats.inWater < -2.0f))
		return false;

	if ((m_stats.velocity.z < -1.0f) && (m_stats.waterLevel > -1.3f) && (m_stats.inWater < -2.0f))
		return false;

	if (GetLinkedVehicle() != NULL)
		return false;

	//if (m_stats.isOnLadder)
		//return false;

	return true;
}

//------------------------------------------------------------------------
void CPlayer::UpdateSwimStats(float frameTime)
{
	bool isClient(IsClient());

	Vec3 localReferencePos = ZERO;
	int spineID = GetBoneID(BONE_SPINE3);
	if (spineID > -1)
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		ISkeleton* pSkeleton = (pCharacter != NULL) ? pCharacter->GetISkeleton() : NULL;
		if (pSkeleton != NULL)
		{
			localReferencePos = pSkeleton->GetAbsJQuatByID(spineID).t;
			localReferencePos.x = 0.0f;
			localReferencePos.y = 0.0f;
			if (!localReferencePos.IsValid())
				localReferencePos = ZERO;
			if (localReferencePos.GetLengthSquared() > (2.0f * 2.0f))
				localReferencePos = ZERO;
		}
	}

	Vec3 referencePos = GetEntity()->GetWorldPos() + GetEntity()->GetWorldRotation() * localReferencePos;
	float waterLevel = gEnv->p3DEngine->GetWaterLevel(&referencePos);
	float playerWaterLevel = -WATER_LEVEL_UNKNOWN;
	float bottomDepth = 0;
	Vec3 surfacePos(referencePos.x, referencePos.y, waterLevel);
	Vec3 bottomPos(referencePos.x, referencePos.y, gEnv->p3DEngine->GetTerrainElevation(referencePos.x, referencePos.y));

	if (bottomPos.z > referencePos.z)
		bottomPos.z = referencePos.z - 100.0f;

	ray_hit hit;
	int rayFlags = (COLLISION_RAY_PIERCABILITY & rwi_pierceability_mask);
	if (gEnv->pPhysicalWorld->RayWorldIntersection(referencePos + Vec3(0,0,0.2f), bottomPos - referencePos - Vec3(0,0,0.4f), ent_terrain | ent_static/*| ent_rigid*/, rayFlags, &hit, 1))
	{
		bottomPos = hit.pt;
	}

	playerWaterLevel = referencePos.z - surfacePos.z;
	bottomDepth = surfacePos.z - bottomPos.z;

	m_stats.waterLevel = CLAMP(playerWaterLevel, -5, 5);
	m_stats.bottomDepth = bottomDepth;

	Vec3 localHeadPos = GetLocalEyePos() + Vec3(0, 0, 0.2f);
	float headWaterLevel = (GetEntity()->GetWorldPos().z + localHeadPos.z) - surfacePos.z;
	if (headWaterLevel < 0.0f)
	{
		if (m_stats.headUnderWater < 0.0f)
		{
			PlaySound(ESound_DiveIn);
			m_stats.headUnderWater = 0.0f;
		}
		m_stats.headUnderWater += frameTime;
	}
	else
	{
		if (m_stats.headUnderWater > 0.0f)
		{
			PlaySound(ESound_DiveOut);
			m_stats.headUnderWater = 0.0f;
		}
		m_stats.headUnderWater -= frameTime;
	}

	if ((m_stats.headUnderWater > 0.0f) && (!isClient || IsThirdPerson()))
	{
		m_underwaterBubblesDelay += frameTime;
		if (m_underwaterBubblesDelay >= 2.0f)
		{
			m_underwaterBubblesDelay = 0.0f;
			SpawnParticleEffect("misc.underwater.player_breath", referencePos, GetEntity()->GetWorldRotation().GetColumn1());
		}
	}
	else
	{
		m_underwaterBubblesDelay = 0.0f;
	}

	UpdateDrowning(frameTime);

	// Update swimming sounds (some are still played from BasicActor.lua:UpdateSounds()).
	// TODO: Remove these from scripts, move code to C++.

	// Update inWater timer (positive is in water, negative is out of water).
	if (ShouldSwim())
	{
		//by design : AI cannot swim and drowns no matter what
		if(GetHealth() > 0 && !isClient && !gEnv->bMultiplayer)
		{
			// apply damage same way as all the other kinds
			HitInfo hitInfo;
			hitInfo.damage = max(1.0f, 30.0f * frameTime);
			hitInfo.targetId = GetEntity()->GetId();
			g_pGame->GetGameRules()->ServerHit(hitInfo);
//			SetHealth(GetHealth() - max(1.0f, 30.0f * frameTime));
			CreateScriptEvent("splash",0);
		}

		if (m_stats.inWater < 0.0f)
		{
			m_stats.inWater = 0.0f;
		}
		else
		{
			m_stats.inWater += frameTime;

			if (m_stats.inWater > 1000.0f)
				m_stats.inWater = 1000.0f;
		}
	}
	else
	{
		if (m_stats.inWater > 0.0f)
		{
			m_stats.inWater = 0.0f;
		}
		else
		{
			m_stats.inWater -= frameTime;

			if (m_stats.inWater < -1000.0f)
				m_stats.inWater = -1000.0f;
		}
	}

	// This is currently not used anywhere (David: and I don't see what this would be used for. Our hero is not Jesus).
	m_stats.isWalkingOnWater = false;

	// Set underwater level for sound listener.
	if (gEnv->pSoundSystem && isClient)
	{
		CCamera& camera = gEnv->pSystem->GetViewCamera();
		float cameraWaterLevel = (camera.GetPosition().z - surfacePos.z);

		SListener* pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
		pListener->SetUnderwater(cameraWaterLevel); // TODO: Make sure listener interface expects zero at surface and negative under water.

		PlaySound(ESound_Underwater, (cameraWaterLevel < 0.0f));
	}

	// DEBUG RENDERING
	bool debugSwimming = (g_pGameCVars->cl_debugSwimming != 0);
	if (debugSwimming && (playerWaterLevel > -10.0f) && (playerWaterLevel < 10.0f))
	{
		Vec3 vRight(m_baseQuat.GetColumn0());

		static ColorF referenceColor(1,1,1,1);
		static ColorF surfaceColor1(0,0.5f,1,1);
		static ColorF surfaceColor0(0,0,0.5f,0);
		static ColorF bottomColor(0,0.5f,0,1);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(referencePos, 0.1f, referenceColor);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(referencePos, surfaceColor1, surfacePos, surfaceColor1, 2.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(surfacePos, 0.2f, surfaceColor1);
		gEnv->pRenderer->DrawLabel(referencePos + vRight * 0.5f, 1.5f, "WaterLevel %3.2f (HWL %3.2f) (SurfaceZ %3.2f)", playerWaterLevel, headWaterLevel, surfacePos.z);

		static int lines = 16;
		static float radius0 = 0.5f;
		static float radius1 = 1.0f;
		static float radius2 = 2.0f;
		for (int i = 0; i < lines; ++i)
		{
			float radians = ((float)i / (float)lines) * gf_PI2;
			Vec3 offset0(radius0 * cry_cosf(radians), radius0 * cry_sinf(radians), 0);
			Vec3 offset1(radius1 * cry_cosf(radians), radius1 * cry_sinf(radians), 0);
			Vec3 offset2(radius2 * cry_cosf(radians), radius2 * cry_sinf(radians), 0);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(surfacePos+offset0, surfaceColor0, surfacePos+offset1, surfaceColor1, 2.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(surfacePos+offset1, surfaceColor1, surfacePos+offset2, surfaceColor0, 2.0f);
		}

		if (bottomDepth > 0.0f)
		{
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(referencePos, bottomColor, bottomPos, bottomColor, 2.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(bottomPos, 0.2f, bottomColor);
			gEnv->pRenderer->DrawLabel(bottomPos + Vec3(0,0,0.5f) - vRight * 0.5f, 1.5f, "BottomDepth %3.3f", bottomDepth);
		}
	}
}

//------------------------------------------------------------------------
void CPlayer::UpdateDrowning(float frameTime)
{
	if (!IsPlayer())
		return;

	bool breath = (m_stats.headUnderWater > 0.0f);

	if ((m_stats.headUnderWater > 60.0f) && (GetHealth() > 0))	// player drowning
	{
		static float energyDrainDuration = 10.0f;
		float energyDrainRate = NANOSUIT_ENERGY / energyDrainDuration;
		float energy = m_pNanoSuit->GetSuitEnergy();
		energy -= (m_pNanoSuit->GetEnergyRechargeRate() + energyDrainRate) * frameTime;
		if (gEnv->bServer)
			m_pNanoSuit->SetSuitEnergy(energy);

		if (energy <= 0)
		{
			breath = false;

			static float drownEffectDelay = 0.8f;
			m_drownEffectDelay -= frameTime;
			if (m_drownEffectDelay < 0.0f)
			{
				static float healthDrainDuration = 10.0f;
				float healthDrainRate = (float)GetMaxHealth() / healthDrainDuration;
				int damage = (int)(healthDrainRate * drownEffectDelay);
				if (gEnv->bServer)
				{
					HitInfo hitInfo(GetEntityId(), GetEntityId(), GetEntityId(), damage, 0, 0, -1, 0, ZERO, ZERO, ZERO);
					CGameRules* pGameRules = g_pGame->GetGameRules();
					if (pGameRules != NULL)
						pGameRules->ServerHit(hitInfo);
				}

				m_drownEffectDelay = drownEffectDelay; // delay until effect is retriggered (sound and screen flashing).

				PlaySound(ESound_Drowning, true);

				IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
				SMFXRunTimeEffectParams params;
				params.pos = GetEntity()->GetWorldPos();
				TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", "player_damage_armormode");
				pMaterialEffects->ExecuteEffect(id, params);
			}
		}
	}
	else
	{
		m_drownEffectDelay = 0.0f;
	}

	PlaySound(ESound_UnderwaterBreathing, breath);
}

//------------------------------------------------------------------------
//TODO: Clean up this whole function, unify this with CAlien via CActor.
void CPlayer::UpdateStats(float frameTime)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	if (!GetEntity())
		return;

	bool isClient(IsClient());
	//update god mode status
	if (isClient)
	{
		SAFE_HUD_FUNC(SetGODMode(g_pGameCVars->g_godMode));

		//FIXME:move this when the UpdateDraw function in player.lua gets moved here
		/*if (m_stats.inWater>0.1f)
			m_stats.firstPersonBody = 0;//disable first person body when swimming
		else*/
			m_stats.firstPersonBody = (uint8)g_pGameCVars->cl_fpBody;
	}	

	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();

	if (!pPhysEnt)
		return;

  if (gEnv->pSystem->IsDedicated() && GetLinkedVehicle())
  {
    // leipzig: force inactive (was active on ded. servers)
    pe_player_dynamics paramsGet;
    if (pPhysEnt->GetParams(&paramsGet))
    {      
      if (paramsGet.bActive)
				m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game);
    }			
  }  

  bool isPlayer(IsPlayer());

	// Update always the logical representation.
	// [Mikko] The logical representation used to be updated later in the function
	// but since an animation driven movement can cause the physics to be disabled,
	// We update these valued before handling the non-collider case below.
	CHECKQNAN_VEC(m_modelOffset);
	Interpolate(m_modelOffset,GetStanceInfo(m_stance)->modelOffset,5.0f,frameTime);
	CHECKQNAN_VEC(m_modelOffset);

	CHECKQNAN_VEC(m_eyeOffset);
	//players use a faster interpolation, and using only the Z offset. A bit different for AI.
	if (isPlayer)
		Interpolate(m_eyeOffset,GetStanceViewOffset(m_stance),15.0f,frameTime);
	else
		Interpolate(m_eyeOffset,GetStanceViewOffset(m_stance,&m_stats.leanAmount,true),5.0f,frameTime);
	CHECKQNAN_VEC(m_eyeOffset);

	CHECKQNAN_VEC(m_weaponOffset);
	Interpolate(m_weaponOffset,GetStanceInfo(m_stance)->GetWeaponOffsetWithLean(m_stats.leanAmount),5.0f,frameTime);
	CHECKQNAN_VEC(m_weaponOffset);
	
	pe_player_dynamics simPar;
	pPhysEnt->GetParams(&simPar);

	//if (GetLinkedVehicle())
	const SAnimationTarget * pTarget = NULL;
	if(GetAnimationGraphState())
		pTarget = GetAnimationGraphState()->GetAnimationTarget();
	bool forceForAnimTarget = false;
	if (pTarget && pTarget->doingSomething)
		forceForAnimTarget = true;

	//FIXME:
	if ((m_stats.spectatorMode || !simPar.bActive || m_stats.flyMode || GetLinkedVehicle()) && !forceForAnimTarget)
	{
		ChangeParachuteState(0);

		m_stats.velocity = m_stats.velocityUnconstrained.Set(0,0,0);
		m_stats.speed = m_stats.speedFlat = 0.0f;
		m_stats.fallSpeed = 0.0f;
		m_stats.inFiring = 0;
		m_stats.jumpLock = 0;
		m_stats.inWater = -1000.0f;
		m_stats.groundMaterialIdx = -1;

		pe_player_dynamics simParSet;
		simParSet.bSwimming = true;
		simParSet.gravity.zero();
		pPhysEnt->SetParams(&simParSet);

		bool shouldReturn = false;

		if(!m_stats.isOnLadder)  //Underwater ladders ... 8$
		{
			m_stats.waterLevel = 0.0f;
			m_stats.bottomDepth = 0.0f;
			m_stats.headUnderWater = 0.0f;

			shouldReturn = true;
		}
		else
		{
			m_stats.upVector = (m_stats.ladderTop - m_stats.ladderBottom).GetNormalized();
			if(!ShouldSwim())
				shouldReturn = true;
		}

		if(shouldReturn)
		{
			UpdateDrowning(frameTime);
			return;
		}

	}

	//retrieve some information about the status of the player
	pe_status_dynamics dynStat;
	pe_status_living livStat;

	int livStatType = livStat.type;
	memset(&livStat, 0, sizeof(pe_status_living));
	livStat.velGround=Vec3(0,0,1);
	livStat.type = livStatType;

	pPhysEnt->GetStatus(&dynStat);
	pPhysEnt->GetStatus(&livStat);

	m_stats.physCamOffset = livStat.camOffset;
	m_stats.gravity = simPar.gravity;

	//
	if ((livStat.bFlying && (m_stance != STANCE_PRONE)) || m_stats.spectatorMode)
		m_stats.groundNormal.Set(0,0,1);
	else
		m_stats.groundNormal = livStat.groundSlope;
	//

	m_stats.groundMaterialIdx = livStat.groundSurfaceIdx;

	Vec3 ppos(GetWBodyCenter());

	bool bootableSurface(false);

	bool groundMatBootable(IsMaterialBootable(m_stats.groundMaterialIdx));

	if (GravityBootsOn() && m_stats.onGroundWBoots>=0.0f)
	{
		bootableSurface = true;

		Vec3 surfaceNormal(0,0,0);
		int surfaceNum(0);

		ray_hit hit;
		int rayFlags = (COLLISION_RAY_PIERCABILITY & rwi_pierceability_mask);

		Vec3 vUp(m_baseQuat.GetColumn2());

		if (m_stats.onGroundWBoots>0.001f)
		{
			Vec3 testSpots[5];
			testSpots[0] = vUp * -1.3f;

			Vec3 offset(dynStat.v * 0.35f);
			Vec3 vRight(m_baseQuat.GetColumn0());
			Vec3 vForward(m_baseQuat.GetColumn1());
			
			testSpots[1] = testSpots[0] + vRight * 0.75f + offset;
			testSpots[2] = testSpots[0] + vForward * 0.75f + offset;
			testSpots[3] = testSpots[0] - vRight * 0.75f + offset;
			testSpots[4] = testSpots[0] - vForward * 0.75f + offset;

			for(int i=0;i<5;++i)
			{
				if (gEnv->pPhysicalWorld->RayWorldIntersection(ppos, testSpots[i], ent_terrain|ent_static|ent_rigid, rayFlags, &hit, 1, &pPhysEnt, 1) && IsMaterialBootable(hit.surface_idx))
				{
					surfaceNormal += hit.n;
					//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(hit.pt, ColorB(0,255,0,100), hit.pt + hit.n, ColorB(255,0,0,100));
					++surfaceNum;
				}
			}
		}
		else
		{
			if (gEnv->pPhysicalWorld->RayWorldIntersection(ppos, vUp * -1.3f, ent_terrain|ent_static|ent_rigid, rayFlags, &hit, 1, &pPhysEnt, 1) && IsMaterialBootable(hit.surface_idx))
			{
				surfaceNormal = hit.n;
				surfaceNum = 1;
			}
		}

		Vec3 gravityVec;

		if (surfaceNum)
		{
			Vec3 newUp(surfaceNormal/(float)surfaceNum);

			m_stats.upVector = Vec3::CreateSlerp(m_stats.upVector,newUp.GetNormalized(),min(3.0f*frameTime, 1.0f));

			gravityVec = -m_stats.upVector * 9.81f;
			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(ppos, ColorB(255,255,0,255), ppos - gravityVec, ColorB(255,255,0,255));
		}
		else 
		{
			m_stats.upVector = m_upVector;
			gravityVec = -m_stats.upVector * 9.81f;
		}

		if (!livStat.bFlying || m_stats.onGroundWBoots>0.001f)
		{
			if (groundMatBootable)
				m_stats.onGroundWBoots = 0.5f;

			livStat.bFlying = false;

			pe_player_dynamics newGravity;
			newGravity.gravity = gravityVec;
			pPhysEnt->SetParams(&newGravity);
		}
	}
	else if (!InZeroG())
	{
		if (m_stance != STANCE_PRONE)
			m_stats.upVector.Set(0,0,1);
		else
		{
			m_stats.upVector.Set(0,0,1);
			//m_stats.upVector = m_stats.groundNormal;
		}
	}
	else
	{
		m_stats.upVector = m_upVector;
	}

	//
	if (m_stats.forceUpVector.len2()>0.01f)
	{
		m_stats.upVector = m_stats.forceUpVector;
		m_stats.forceUpVector.zero(); 
	}
	//

	IAnimationGraphState* pAGState = GetAnimationGraphState();

	//update status table
	if (livStat.bFlying && !m_stats.spectatorMode && !GetLinkedVehicle())
	{
		if (ShouldSwim())
		{
			if(pAGState)
			{
				char action[256];
				pAGState->GetInput(m_inputAction, action);
				if ( strcmp(action,"jumpMP")==0 || strcmp(action,"jumpMPStrength")==0 )
					pAGState->SetInput(m_inputAction, "idle");
			}
		}
		else
		{
			//no freefalling in zeroG
			if (!InZeroG() && !m_stats.inFreefall.Value() && dynStat.v.z<-15.0f)
				ChangeParachuteState(1);

			m_stats.inAir += frameTime;
			// Danny - changed this since otherwise AI has a fit going down stairs - it seems that when off
			// the ground it can only decelerate.
			// If you revert this (as it's an ugly hack) - test AI on stairs!
			static float minTimeForOffGround = 0.5f;
			if (m_stats.inAir > minTimeForOffGround || isPlayer)
  			m_stats.onGround = 0.0f;

			if (m_stats.inAir>0.13f)
					m_stats.jumped = false;
		}
	}
	else
	{
		if (bootableSurface && m_stats.onGroundWBoots<0.001f && !groundMatBootable)
		{
			bootableSurface = false;
		}
		else
		{
			bool landed(false);
			if (m_stats.inAir>0.1f && m_stats.onGround<0.0001f && !m_stats.isOnLadder)
				landed = true;

			m_stats.onGround += frameTime;
			m_stats.inAir = 0.0f;
			
			if (landed)
			{
				m_stats.landed = true;
				m_stats.jumpLock = 0.2f;
				Landed(m_stats.fallSpeed);
			}

			if ((m_stats.jumped && (m_stats.inAir > 0.0f)) || m_stats.landed || ShouldSwim())
			{
				if(pAGState)
				{
					char action[256];
					pAGState->GetInput(m_inputAction, action);
					if ( strcmp(action,"jumpMP")==0 || strcmp(action,"jumpMPStrength")==0 )
					pAGState->SetInput(m_inputAction, "idle");
				}
			}

			if (landed && m_stats.fallSpeed)
			{
				CreateScriptEvent("landed",m_stats.fallSpeed);
				m_stats.fallSpeed = 0.0f;

				// Clear ag action 'jumpMP' set in CPlayerMovement::ProcessOnGroundOrJumping()
			}

			ChangeParachuteState(0);
		}
	}

	m_stats.velocity = livStat.vel-livStat.velGround;//dynStat.v;
	m_stats.velocityUnconstrained = dynStat.v;

	//calculate the flatsped from the player ground orientation
	Vec3 flatVel(m_baseQuat.GetInverted()*m_stats.velocity);
	flatVel.z = 0;
	m_stats.speedFlat = flatVel.len();

	if (m_stats.inAir && m_stats.velocity*m_stats.gravity>0.0f && (m_stats.inWater <= 0.0f) && !m_stats.isOnLadder)
	{
		if (!m_stats.fallSpeed)
			CreateScriptEvent("fallStart",0);

		m_stats.fallSpeed = -m_stats.velocity.z;
	}
	else
	{
		m_stats.fallSpeed = 0.0f;
		//CryLogAlways( "[player] end falling %f", ppos.z);
	}

	m_stats.mass = dynStat.mass;

	if (m_stats.speedFlat>0.1f)
	{
		m_stats.inMovement += frameTime;
		m_stats.inRest = 0;
	}
	else
	{
		m_stats.inMovement = 0;
		m_stats.inRest += frameTime;
	}

	UpdateSwimStats(frameTime);

	if (ShouldSwim())
	{
		m_stats.inAir = 0.0f;
		ChangeParachuteState(0);
	}

	/*if (m_stats.inAir>0.001f)
	Interpolate(m_modelOffset,GetStanceInfo(m_stance)->modelOffset,3.0f,frameTime,3.0f);
	else*/

	if (livStat.groundHeight>-0.001f)
		m_groundElevation = livStat.groundHeight - ppos*GetEntity()->GetRotation().GetColumn2();

	if (m_stats.inAir<0.5f)
	{
		/*Vec3 touch_point = ppos + GetEntity()->GetRotation().GetColumn2()*m_groundElevation;
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(touch_point, ColorB(0,255,255,255), touch_point - GetEntity()->GetRotation().GetColumn2()*m_groundElevation, ColorB(0,255,255,255));   
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(touch_point,0.12f,ColorB(0,255,0,100) );*/

		//Interpolate(m_modelOffset.z,m_groundElevation,5.0f,frameTime);
		//m_modelOffset.z = m_groundElevation;
	}



	//
	pe_player_dimensions ppd;
	switch (m_stance)
	{
	default:
	case STANCE_STAND:ppd.heightHead = 1.6f;break;
	case STANCE_CROUCH:ppd.heightHead = 0.8f;break;
	case STANCE_PRONE:ppd.heightHead = 0.35f;break;
	}
	ppd.headRadius = 0.0f;
	pPhysEnt->SetParams(&ppd);




	pe_player_dynamics simParSet;
	bool shouldSwim = ShouldSwim();
	simParSet.bSwimming = (m_stats.spectatorMode || m_stats.flyMode || shouldSwim || (InZeroG()&&!bootableSurface) || m_stats.isOnLadder);
	//set gravity to 0 in water
	if ((shouldSwim && m_stats.waterLevel <= 0.0f) || m_stats.isOnLadder)
		simParSet.gravity.zero();

	pPhysEnt->SetParams(&simParSet);

	//update some timers
	m_stats.inFiring = max(0.0f,m_stats.inFiring - frameTime);
	m_stats.jumpLock = max(0.0f,m_stats.jumpLock - frameTime);
//	m_stats.waitStance = max(0.0f,m_stats.waitStance - frameTime);

	if (m_stats.onGroundWBoots < 0.0f)
		m_stats.onGroundWBoots = min(m_stats.onGroundWBoots + frameTime,0.0f);
	else
		m_stats.onGroundWBoots = max(m_stats.onGroundWBoots - frameTime,0.0f);

	m_stats.thrusterSprint = min(m_stats.thrusterSprint + frameTime, 1.0f);
}

void CPlayer::ToggleThirdPerson()
{
	m_stats.isThirdPerson = !m_stats.isThirdPerson;

	CALL_PLAYER_EVENT_LISTENERS(OnToggleThirdPerson(this,m_stats.isThirdPerson));
}

int CPlayer::IsGod()
{
	if (!m_pGameFramework->CanCheat())
		return 0;

	int godMode(g_pGameCVars->g_godMode);

	// Demi-Gods are not Gods
	if (godMode == 3)
		return 0;

	//check if is the player
	if (IsClient())
		return godMode;

	//check if is a squadmate
	IAIActor* pAIActor = CastToIAIActorSafe(GetEntity()->GetAI());
	if (pAIActor)
	{
		int group=pAIActor->GetParameters().m_nGroup;
		if(group>= 0 && group<10)
			return (godMode==2?1:0);
	}
	return 0;
}

bool CPlayer::IsThirdPerson() const
{
	//force thirdperson view for non-clients
	if (!IsClient())
		return true;
	
  return m_stats.isThirdPerson;	
}


void CPlayer::Revive( bool fromInit )
{
	CActor::Revive(fromInit);

	if (GetScreenEffects())
	{
		GetScreenEffects()->ClearBlendGroup(14, true);
		CPostProcessEffect *blend = new CPostProcessEffect(GetEntity()->GetId(), "WaterDroplets_Amount", 0.0f);
		CWaveBlend *wave = new CWaveBlend();
		GetScreenEffects()->StartBlend(blend, wave, 20.0f, 14);

		//reset possible death blur
		GetScreenEffects()->ClearBlendGroup(66, true);
		//reset possible alien fear
		GetScreenEffects()->ClearBlendGroup(15, true);

		//gEnv->p3DEngine->SetPostEffectParam("Global_ColorK", 0.0f);

		if (IsClient())
			SAFE_HUD_FUNC(ShowDeathFX(0));
	}

  InitInterference();

	if (CNanoSuit *pSuit=GetNanoSuit())
	{
		bool active=pSuit->IsActive(); // CNanoSuit::Reset resets the active flag
		
		pSuit->Reset(this);

		if (active)
			ActivateNanosuit(true);
	}

	m_parachuteEnabled = false; // no parachute by default
	m_openParachuteTimer = -1.0f;
	m_openingParachute = false;

	m_bSwimming = false;
	m_actions = 0;
	m_forcedRotation = false;
	m_bStabilize = false;
	m_fSpeedLean = 0.0f;

	m_frozenAmount = 0.0f;

	m_viewAnglesOffset.Set(0,0,0);
	
  if (IsClient() && IsThirdPerson())
    ToggleThirdPerson();

	// HAX: to fix player spawning and floating in dedicated server: Marcio fix me?
	if (gEnv->pSystem->IsEditor() == false)  // AlexL: in editor, we never keep spectator mode
	{
		uint8 spectator=m_stats.spectatorMode;
		m_stats = SPlayerStats();
		m_stats.spectatorMode=spectator;
	}
	else
	{
		m_stats = SPlayerStats();
	}

	m_headAngles.Set(0,0,0);
	m_eyeOffset.Set(0,0,0);
	m_eyeOffsetView.Set(0,0,0);
	m_modelOffset.Set(0,0,0);
	m_weaponOffset.Set(0,0,0);
	m_groundElevation = 0.0f;

	m_velocity.Set(0,0,0);
	m_bobOffset.Set(0,0,0);

	m_FPWeaponOffset.Set(0,0,0);
	m_FPWeaponAngleOffset.Set(0,0,0);
	m_FPWeaponLastDirVec.Set(0,0,0);

	m_feetWpos[0] = ZERO;
	m_feetWpos[1] = ZERO;
	m_lastAnimContPos = ZERO;

	m_angleOffset.Set(0,0,0);

	//m_baseQuat.SetIdentity();
	m_viewQuatFinal = m_baseQuat = m_viewQuat = GetEntity()->GetRotation();
	m_clientViewMatrix.SetIdentity();

	m_turnTarget = GetEntity()->GetRotation();
	m_lastRequestedVelocity.Set(0,0,0);
  m_lastAnimControlled = 0.f;

	m_viewRoll = 0;
	m_upVector.Set(0,0,1);

	m_viewBlending = true;
	m_fDeathTime = 0.0f;

	GetEntity()->SetFlags(GetEntity()->GetFlags() | (ENTITY_FLAG_CASTSHADOW));
	GetEntity()->SetSlotFlags(0,GetEntity()->GetSlotFlags(0)|ENTITY_SLOT_RENDER);

	if (m_pPlayerInput.get())
		m_pPlayerInput->Reset();

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);

	if (pCharacter)
		pCharacter->EnableStartAnimation(true);

	ResetAnimations();
	// if we're coming from initialize, then we're not part of the game object yet -- therefore we can't
	// receive events from the animation graph, and hence, we'll delay the initial update until PostInit()
	// is called...
	//if (!fromInit)
	//	UpdateAnimGraph();

	//	m_nanoSuit.Reset(this);
	//	m_nanoSuit.Activate(m_params.nanoSuitActive);

	if (!fromInit || GetISystem()->IsSerializingFile() == 1)
		ResetAnimGraph();

	if(!fromInit && IsClient())
	{
		SAFE_HUD_FUNC(BreakHUD(0));
		if(gEnv->bMultiplayer)
		{
				IView *pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetViewByEntityId(GetEntityId());
				if(pView)
					pView->ResetShaking();
		}
	}

	if(!GetInventory()->GetCurrentItem())
	{
		CFists *pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));
		if(pFists)
			g_pGame->GetIGameFramework()->GetIItemSystem()->SetActorItem(this, pFists->GetEntityId());
	}

	if (!fromInit && m_pNanoSuit)
	{
		m_pNanoSuit->SetMode(NANOMODE_DEFENSE, true);
		m_pNanoSuit->SetCloakLevel(CLOAKMODE_CHAMELEON);
		//m_pNanoSuit->ActivateMode(NANOMODE_CLOAK, false);	// cloak must be picked up or bought

		if (GetEntity()->GetAI())	//just for the case the player was still cloaked (happens in editor when exiting game cloaked)
			gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1, "OnNanoSuitUnCloak", GetEntity()->GetAI());
	}
}

void CPlayer::Kill()
{
	if (CNanoSuit *pSuit=GetNanoSuit())
		pSuit->Death();

	CActor::Kill();
}

#if 0 // AlexL 14.03.2007: no more bootable materials for now. and scriptable doesn't provide custom params anyway (after optimization)

bool CPlayer::IsMaterialBootable(int matId) const
{
	ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(matId);
	IScriptTable *pTable = pSurfaceType ? pSurfaceType->GetScriptTable() : NULL;

	bool GBootable(false);

	if (pTable)
		pTable->GetValue("GBootable",GBootable);

	return GBootable;
}
#endif

Vec3 CPlayer::GetStanceViewOffset(EStance stance,float *pLeanAmt,bool withY) const
{	
	Vec3 offset(GetStanceInfo(m_stanceCollider)->viewOffset);

	//apply leaning
	float leanAmt;
	if (!pLeanAmt)
		leanAmt = m_stats.leanAmount;
	else
		leanAmt = *pLeanAmt;

	if(IsPlayer())
	{
		if (leanAmt*leanAmt>0.01f)
		{
			offset.x += leanAmt * m_params.leanShift;
			if (stance != STANCE_PRONE)
				offset.z -= fabs(leanAmt) * m_params.leanShift * 0.33f;
		}

		if (m_stats.bSprinting && stance == STANCE_CROUCH && m_stats.inMovement > 0.1f)
			offset.z += 0.35f;
	}
	else
	{
		offset = GetStanceInfo(stance)->GetViewOffsetWithLean(leanAmt);
	}

	if (!withY)
		offset.y = 0.0f;

	return offset;
}		

void CPlayer::RagDollize( bool fallAndPlay )
{
	if (m_stats.isRagDoll)
		return;

	ResetAnimations();

	CActor::RagDollize( fallAndPlay );

	m_stats.followCharacterHead = 1;

	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();

	if (pPhysEnt)
	{
		pe_simulation_params sp;
		sp.gravity = sp.gravityFreefall = m_stats.gravity;
		//sp.damping = 1.0f;
		sp.dampingFreefall = 0.0f;
		sp.mass = m_stats.mass * 2.0f;

		pPhysEnt->SetParams(&sp);

		if(IsClient())
		{
			int headBoneID = GetBoneID(BONE_HEAD);
			if(headBoneID > -1)
			{
				pe_params_part params;
				params.partid  = headBoneID;
				params.scale = 8.0f; //make sure the camera doesn't clip through the ground
				params.flagsAND = ~geom_colltype_ray;
				pPhysEnt->SetParams(&params);
			}
		}
	}

	if (!fallAndPlay)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
		if (pCharacter)
			pCharacter->EnableStartAnimation(false);
	}
	else
	{
		if (ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0))
			pCharacter->GetISkeleton()->Fall();
	}
}

void CPlayer::PostPhysicalize()
{
	CActor::PostPhysicalize();

	if (m_pAnimatedCharacter)
	{
		//set inertia, it will get changed again soon, with slidy surfaces and such
		SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
		params.SetInertia(m_params.inertia,m_params.inertiaAccel);
		params.flags |= eACF_EnableMovementProcessing | eACF_ZCoordinateFromPhysics | eACF_ConstrainDesiredSpeedToXY;
		m_pAnimatedCharacter->SetParams(params);
	}

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if (!pCharacter)
		return;
	pCharacter->GetISkeleton()->SetPostProcessCallback0(PlayerProcessBones,this);
	pe_simulation_params sim;
	sim.maxLoggedCollisions = 5;
	pe_params_flags flags;
	flags.flagsOR = pef_log_collisions;
	pCharacter->GetISkeleton()->GetCharacterPhysics()->SetParams(&sim);
	pCharacter->GetISkeleton()->GetCharacterPhysics()->SetParams(&flags);
	

	//set a default offset for the character, so in the editor the bbox is correct
	if(GetAnimatedCharacter())
		GetAnimatedCharacter()->SetExtraAnimationOffset(QuatT(GetStanceInfo(STANCE_STAND)->modelOffset, IDENTITY));

//	if (m_pGameFramework->IsMultiplayer())
//		GetGameObject()->ForceUpdateExtension(this, 0);
}

void CPlayer::UpdateAnimGraph(IAnimationGraphState * pState)
{
	CActor::UpdateAnimGraph( pState );
}

void CPlayer::PostUpdate(float frameTime)
{ 
	CActor::PostUpdate(frameTime);
	if (m_pPlayerInput.get())
		m_pPlayerInput->PostUpdate();
}

void CPlayer::CameraShake(float angle,float shift,float duration,float frequency,Vec3 pos,int ID,const char* source) 
{
	float angleAmount(max(-90.0f,min(90.0f,angle)) * gf_PI/180.0f);
	float shiftAmount(shift);
  
  if (IVehicle* pVehicle = GetLinkedVehicle())
  {
    if (IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(GetEntityId()))    
      pSeat->OnCameraShake(angleAmount, shiftAmount, pos, source);    
  }

	Ang3 shakeAngle(\
		RANDOMR(0.0f,1.0f)*angleAmount*0.15f, 
		(angleAmount*min(1.0f,max(-1.0f,RANDOM()*7.7f)))*1.15f,
		RANDOM()*angleAmount*0.05f
		);

	Vec3 shakeShift(RANDOM()*shiftAmount,0,RANDOM()*shiftAmount);

	IView *pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetViewByEntityId(GetEntityId());
	if (pView)
		pView->SetViewShake(shakeAngle,shakeShift,duration,frequency,0.5f,ID);

	/*//if a position is defined, execute directional shake
	if (pos.len2()>0.01f)
	{
	Vec3 delta(pos - GetEntity()->GetWorldPos());
	delta.NormalizeSafe();

	float dotSide(delta * m_viewQuatFinal.GetColumn0());
	float dotFront(delta * m_viewQuatFinal.GetColumn1() - delta * m_viewQuatFinal.GetColumn2());

	float randomRatio(0.5f);
	dotSide += RANDOM() * randomRatio;
	dotFront += RANDOM() * randomRatio;

	m_viewShake.angle.Set(dotFront*shakeAngle, -dotSide*shakeAngle*RANDOM()*0.5f, dotSide*shakeAngle);
	}
	else
	{
	m_viewShake.angle.Set(RANDOMR(0.0f,1.0f)*shakeAngle, RANDOM()*shakeAngle*0.15f, RANDOM()*shakeAngle*0.75f);
	}*/
}
  

void CPlayer::ResetAnimations()
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);

	if (pCharacter)
	{
		pCharacter->GetISkeleton()->StopAnimationsAllLayers();

//		if (m_pAnimatedCharacter)
//			m_pAnimatedCharacter->GetAnimationGraphState()->Pause(true, eAGP_StartGame);
		//disable any IK
		//pCharacter->SetLimbIKGoal(LIMB_LEFT_LEG);
		//pCharacter->SetLimbIKGoal(LIMB_RIGHT_LEG);
		//pCharacter->SetLimbIKGoal(LIMB_LEFT_ARM);
		//pCharacter->SetLimbIKGoal(LIMB_RIGHT_ARM);

		//
		for (int i=0;i<BONE_ID_NUM;++i)
		{
			int boneID = GetBoneID(i);
			/*if (boneID>-1)
				pCharacter->GetISkeleton()->SetPlusRotation(boneID, IDENTITY);*/
		}

		pCharacter->GetISkeleton()->SetLookIK(false,0,ZERO);
	}
}

void CPlayer::SetHealth(int health )
{
	float oldHealth = m_health;
	CActor::SetHealth(health);

	if (health <= 0)
	{
		const bool bIsGod = IsGod() > 0;
		if (IsClient())
		{
			SAFE_HUD_FUNC(ActorDeath(this));
	
				if (bIsGod)
				{
				SAFE_HUD_FUNC(TextMessage("GodMode:died!"));
			}
		}
		if (bIsGod)		// report GOD death
			CALL_PLAYER_EVENT_LISTENERS(OnDeath(this, true));
	}

	if (GetHealth() <= 0)	//client deathFX are triggered in the lua gamerules
	{
		ResetAnimations();
		SetDeathTimer();

		m_stats.isOnLadder = false;

		if(IsClient())
		{
			SAFE_HUD_FUNC(GetRadar()->Reset());
			SAFE_HUD_FUNC(BreakHUD(2));

			if(IItem *pItem = GetCurrentItem(false))
				pItem->Select(false);
		}
		// report normal death
		CALL_PLAYER_EVENT_LISTENERS(OnDeath(this, false));
	}
	else if(IsClient())	//damageFX
	{
		if(m_health < oldHealth)
		{
			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();

			const float healthThrHi = g_pGameCVars->g_playerLowHealthThreshold;
			const float healthThrLow = g_pGameCVars->g_playerLowHealthThreshold/2;

			if(!g_pGameCVars->g_godMode && m_health < healthThrHi)
			{
				SMFXRunTimeEffectParams params;
				params.pos = GetEntity()->GetWorldPos();
				if(m_health <= healthThrLow && oldHealth > healthThrLow)
				{
					TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", "player_damage_2");
					pMaterialEffects->ExecuteEffect(id, params);
				}
				else if(m_health <= healthThrHi && oldHealth > healthThrHi)
				{
					TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", "player_damage_1");
					pMaterialEffects->ExecuteEffect(id, params);
				}
			}
		}
	}

	GetGameObject()->ChangedNetworkState(ASPECT_HEALTH);
}

void CPlayer::SerializeXML( XmlNodeRef& node, bool bLoading )
{
}

void CPlayer::SetAuthority( bool auth )
{
}

//------------------------------------------------------------------------
void CPlayer::Freeze(bool freeze)
{
	if (m_stats.isFrozen.Value()==freeze)
		return;

	CActor::Freeze(freeze);

	if (IsPlayer())
		m_params.vLimitDir = (freeze) ? m_viewQuat.GetColumn1() : Vec3(ZERO);

	if (m_pAnimatedCharacter)
	{
		SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();

		if (freeze)
			params.flags &= ~eACF_EnableMovementProcessing;
		else
			params.flags |= eACF_EnableMovementProcessing;

		m_pAnimatedCharacter->SetParams(params);
	}

	m_stats.followCharacterHead = (freeze?2:0);
}



float CPlayer::GetMassFactor() const
{
	/*  //code regarding complete inventory
	IInventory* pInventory = static_cast<IInventory *>(GetGameObject()->QueryExtension("Inventory"));
	if(!pInventory)
	return 1.0f;

	int itemCount = pInventory->GetCount();
	int weight = 0;
	EntityId itemId = 0;
	for(int i = 0; i < itemCount; i++)
	{
	itemId = pInventory->GetItem(i);
	if(itemId)
	weight += GetWeapon(itemId)->GetParams().mass;
	}

	if(weight)
	{
	float massFactor = 1.0f - weight / (GetActorParams()->maxGrabMass*GetActorStrength()*10.0f);
	if(massFactor <= 0)
	massFactor = 0.25f;
	if(IsZeroG())
	massFactor += (1.0f - massFactor) * 0.5f;
	return massFactor;
	}

	return 1.0f;*/

	//code regarding currentItem only
	EntityId itemId = GetInventory()->GetCurrentItem();
	if (itemId)
	{
		float mass = 0;
		if(CWeapon* weap = GetWeapon(itemId))
			mass = weap->GetParams().mass;
		else if(CItem* item = GetItem(itemId))
			mass = item->GetParams().mass;
		float massFactor = 1.0f - (mass / (m_params.maxGrabMass*GetActorStrength()*2.0f));
		if (m_stats.inZeroG)
			massFactor += (1.0f - massFactor) * 0.5f;
		return massFactor;
	}
	return 1.0f;
}

float CPlayer::GetFrozenAmount(bool stages/*=false*/) const
{
	if (stages && IsPlayer())
	{
		float steps = g_pGameCVars->cl_frozenSteps;

		// return the next step value
		return m_frozenAmount>0 ? min(((int)(m_frozenAmount * steps))+1.f, steps)*1.f/steps : 0;
	}
	
	return m_frozenAmount;
}

void CPlayer::SetAngles(const Ang3 &angles) 
{
	Matrix33 rot(Matrix33::CreateRotationXYZ(angles));
	CMovementRequest mr;
	mr.SetLookTarget( GetEntity()->GetWorldPos() + 20.0f * rot.GetColumn(1) );
	m_pMovementController->RequestMovement(mr);
}

Ang3 CPlayer::GetAngles() 
{
	if(IsClient() && GetLinkedVehicle())
		return Ang3(m_clientViewMatrix);

	return Ang3(m_viewQuatFinal.GetNormalized());
}

void CPlayer::AddAngularImpulse(const Ang3 &angular,float deceleration,float duration)
{
	m_stats.angularImpulse = angular;
	m_stats.angularImpulseDeceleration = deceleration;
	m_stats.angularImpulseTimeMax = m_stats.angularImpulseTime = duration;
}

void CPlayer::SelectNextItem(int direction, bool keepHistory, const char *category)
{
	if (m_health && m_stats.animationControlled || ShouldSwim() || /*m_bSprinting || */m_stats.inFreefall.Value())
		return;

	EntityId oldItemId=GetCurrentItemId();

	CActor::SelectNextItem(direction,keepHistory,category);

	if (GetCurrentItemId() && oldItemId!=GetCurrentItemId())
		m_bSprinting=false; // force the weapon disabling code to be 

	GetGameObject()->ChangedNetworkState(ASPECT_CURRENT_ITEM);
}

void CPlayer::HolsterItem(bool holster)
{
	CActor::HolsterItem(holster);

	GetGameObject()->ChangedNetworkState(ASPECT_CURRENT_ITEM);
}

void CPlayer::SelectLastItem(bool keepHistory)
{
	CActor::SelectLastItem(keepHistory);

	GetGameObject()->ChangedNetworkState(ASPECT_CURRENT_ITEM);
}

void CPlayer::SelectItemByName(const char *name, bool keepHistory)
{
	CActor::SelectItemByName(name, keepHistory);

	GetGameObject()->ChangedNetworkState(ASPECT_CURRENT_ITEM);
}

void CPlayer::SelectItem(EntityId itemId, bool keepHistory)
{
	CActor::SelectItem(itemId, keepHistory);

	GetGameObject()->ChangedNetworkState(ASPECT_CURRENT_ITEM);
}

bool CPlayer::SetProfile(uint8 profile )
{
	if (profile==eAP_Alive && GetGameObject()->GetPhysicalizationProfile()==eAP_Sleep)
	{
		m_stats.isStandingUp=true;
		m_stats.isRagDoll=false;
	}

	return CActor::SetProfile(profile);
}

void CPlayer::Serialize( TSerialize ser, unsigned aspects )
{
	CActor::Serialize(ser, aspects);

	if (ser.GetSerializationTarget() != eST_Network)
	{
		m_pMovementController->Serialize(ser);

		ser.BeginGroup( "BasicProperties" );
		//ser.EnumValue("stance", this, &CPlayer::GetStance, &CPlayer::SetStance, STANCE_NULL, STANCE_LAST);		
		// skip matrices... not supported
		ser.Value( "velocity", m_velocity );
		ser.Value( "feetWpos0", m_feetWpos[0] );
		ser.Value( "feetWpos1", m_feetWpos[1] );
		// skip animation to play for now
		// skip currAnimW
		ser.Value( "eyeOffset", m_eyeOffset );
		ser.Value( "bobOffset", m_bobOffset );
		ser.Value( "FPWeaponLastDirVec", m_FPWeaponLastDirVec );
		ser.Value( "FPWeaponOffset", m_FPWeaponOffset );
		ser.Value( "FPWeaponAngleOffset", m_FPWeaponAngleOffset );
		ser.Value( "viewAnglesOffset", m_viewAnglesOffset );
		ser.Value( "angleOffset", m_angleOffset );
		ser.Value( "modelOffset", m_modelOffset );
		ser.Value( "headAngles", m_headAngles );
		ser.Value( "viewRoll", m_viewRoll );
		ser.Value( "upVector", m_upVector );
		ser.Value( "hasHUD", m_bHasHUD);
		ser.Value( "viewQuat", m_viewQuat );
		ser.Value( "viewQuatFinal", m_viewQuatFinal );
		ser.Value( "baseQuat", m_baseQuat );
		ser.Value( "weaponOffset", m_weaponOffset ); 
    ser.Value( "frozenAmount", m_frozenAmount );
		ser.EndGroup();

		//serializing stats
		m_stats.Serialize(ser, aspects);
		//nanosuit
		
		bool haveNanoSuit = (m_pNanoSuit)?true:false;
		ser.Value("haveNanoSuit", haveNanoSuit);
		if(haveNanoSuit)
		{
			if(!m_pNanoSuit)
				m_pNanoSuit = new CNanoSuit();

			if(ser.IsReading())
				m_pNanoSuit->Reset(this);
			m_pNanoSuit->Serialize(ser, aspects);
		}

		//input-actions
		bool hasInput = (m_pPlayerInput.get())?true:false;
		ser.Value("PlayerInputExists", hasInput);
		if(hasInput)
		{
			if(ser.IsReading() && !(m_pPlayerInput.get()))
				m_pPlayerInput.reset( new CPlayerInput(this) );
			((CPlayerInput*)m_pPlayerInput.get())->SerializeSaveGame(ser);
		}

		ser.Value("mountedWeapon", m_stats.mountedWeaponID);
		if(m_stats.mountedWeaponID && this == g_pGame->GetIGameFramework()->GetClientActor()) //re-mounting is done in the item
		{
			if(g_pGame->GetHUD())
				g_pGame->GetHUD()->GetCrosshair()->SetUsability(true); //doesn't update after loading
		}

		ser.Value("parachuteEnabled", m_parachuteEnabled);

		// perform post-reading fixups
		if (ser.IsReading())
		{
			// fixup matrices here

			//correct serialize the parachute
			int8 freefall(m_stats.inFreefall.Value());
			m_stats.inFreefall = -1;
			ChangeParachuteState(freefall);
		}

		if (IsClient())
		{
			// Staging params
			if (ser.IsWriting())
			{
				m_stagingParams.Serialize(ser);
			}
			else
			{
				SStagingParams stagingParams;
				stagingParams.Serialize(ser);
				if (stagingParams.bActive || stagingParams.bActive != m_stagingParams.bActive)
					StagePlayer(stagingParams.bActive, &stagingParams);
			}
		}
	}
	else
	{
		if (aspects & ASPECT_HEALTH)
		{
			ser.Value("health", m_health, 'hlth');
			bool isFrozen = m_stats.isFrozen.Value();
			ser.Value("frozen", isFrozen, 'bool');
			ser.Value("frozenAmount", m_frozenAmount, 'frzn');
		}    
		if (aspects & ASPECT_CURRENT_ITEM)
		{
			ser.Value("currentItemId", static_cast<CActor*>(this), &CActor::NetGetCurrentItem, &CActor::NetSetCurrentItem, 'eid');
		}
		if (aspects & IPlayerInput::INPUT_ASPECT)
		{
			SSerializedPlayerInput serializedInput;
			if (m_pPlayerInput.get() && ser.IsWriting())
				m_pPlayerInput->GetState(serializedInput);

			serializedInput.Serialize(ser);

			if (m_pPlayerInput.get() && ser.IsReading())
			{
				m_pPlayerInput->SetState(serializedInput);
				m_stats.bSprinting = serializedInput.sprint;
				m_stats.jumped = serializedInput.jump;
			}
		}
		if(m_pNanoSuit)
			m_pNanoSuit->Serialize(ser, aspects);
	}
}

void CPlayer::PostSerialize()
{
	CActor::PostSerialize();
	m_drownEffectDelay = 0.0f;

	//stop looping sounds
	for(int i = (int)ESound_Player_First+1; i < (int)ESound_Player_Last;++i)
		PlaySound((EPlayerSounds)i, false);
	if(m_pNanoSuit)
	{
		for(int i = (int)NO_SOUND+1; i < (int)ESound_Suit_Last;++i)
			m_pNanoSuit->PlaySound((ENanoSound)i, 0.0f, true);
	}
}

void SPlayerStats::Serialize(TSerialize ser, unsigned aspects)
{
	assert( ser.GetSerializationTarget() != eST_Network );
	ser.BeginGroup("PlayerStats");

	if (ser.GetSerializationTarget() != eST_Network)
	{
		//when reading, reset the structure first.
		if (ser.IsReading())
			*this = SPlayerStats();

		ser.Value("inAir", inAir);
		ser.Value("onGround", onGround);
		inFreefall.Serialize(ser, "inFreefall");
		if(ser.IsReading())
			inFreefallLast = !inFreefall.Value();
		ser.Value("landed", landed);
		ser.Value("jumped", jumped);
		ser.Value("inMovement", inMovement);
		ser.Value("inRest", inRest);
		ser.Value("inWater", inWater);
		ser.Value("waterLevel", waterLevel);
		ser.Value("flatSpeed", speedFlat);
		ser.Value("gravity", gravity);
		//ser.Value("mass", mass);		//serialized in Actor::SerializeProfile already ...
		ser.Value("bobCycle", bobCycle);
		ser.Value("leanAmount", leanAmount);
		ser.Value("shakeAmount", shakeAmount);
		ser.Value("physCamOffset", physCamOffset);
		ser.Value("fallSpeed", fallSpeed);
		ser.Value("isFiring", isFiring);
		ser.Value("isRagDoll", isRagDoll);
		ser.Value("isWalkingOnWater", isWalkingOnWater);
		followCharacterHead.Serialize(ser, "followCharacterHead");
		firstPersonBody.Serialize(ser, "firstPersonBody");
		ser.Value("velocity", velocity);
		ser.Value("velocityUnconstrained", velocityUnconstrained);

		ser.Value("upVector", upVector);
		ser.Value("groundNormal", groundNormal);
		ser.Value("FPWeaponPos", FPWeaponPos);
		ser.Value("FPWeaponAngles", FPWeaponAngles);
		ser.Value("FPSecWeaponPos", FPSecWeaponPos);
		ser.Value("FPSecWeaponAngles", FPSecWeaponAngles);
		ser.Value("isThirdPerson", isThirdPerson);
    isFrozen.Serialize(ser, "isFrozen");
    
		//FIXME:serialize cheats or not?
		//int godMode(g_pGameCVars->g_godMode);
		//ser.Value("godMode", godMode);
		//g_pGameCVars->g_godMode = godMode;
		//ser.Value("flyMode", flyMode);
		//

		ser.Value("thrusterSprint", thrusterSprint);

		ser.Value("isOnLadder", isOnLadder);
		ser.Value("ladderTop", ladderTop);
		ser.Value("ladderBottom", ladderBottom);
		ser.Value("ladderOrientation", ladderOrientation);
		ser.Value("ladderUpDir", ladderUpDir);
		ser.Value("ladderMovingUp", ladderMovingUp);
		ser.Value("ladderMovingDown", ladderMovingDown);
		ser.Value("playerRotation", playerRotation);

		ser.Value("forceCrouchTime", forceCrouchTime);    
	}

	ser.EndGroup();
}

bool CPlayer::CreateCodeEvent(SmartScriptTable &rTable)
{
	const char *event = NULL;
	rTable->GetValue("event",event);

	if (event && !strcmp(event,"ladder"))
	{
		m_stats.ladderTop.zero();
		m_stats.ladderBottom.zero();

		rTable->GetValue("ladderTop",m_stats.ladderTop);
		rTable->GetValue("ladderBottom",m_stats.ladderBottom);

		m_stats.isOnLadder = (m_stats.ladderTop.len2()>0.01f && m_stats.ladderBottom.len2()>0.01f);

		if (m_stats.isOnLadder)
			m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game);
		else
			m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Undefined, eColliderModeLayer_Game);

		return true;
	}
	else if (event && !strcmp(event,"setCloak"))
	{
		bool on;
		if (rTable->GetValue("on",on) && m_pNanoSuit)
		{
			if(on)
				m_pNanoSuit->SetMode(NANOMODE_CLOAK);
			else
				m_pNanoSuit->SetMode(NANOMODE_DEFENSE);
		}

		return true;
	}
	else if (event && !strcmp(event,"addSuitEnergy") && m_pNanoSuit)
	{
		float amount(0.0f);
		rTable->GetValue("amount",amount);

		m_pNanoSuit->SetSuitEnergy(m_pNanoSuit->GetSuitEnergy() + amount);

		return true;
	}
/* kirill - needed for debugging AI aiming/accuracy 
	else if (event && !strcmp(event,"aiHitDebug"))
	{
		if(!IsPlayer())
			return true;
		ScriptHandle id;
		id.n = 0;
		rTable->GetValue("shooterId",id);
		IEntity *pEntity((id.n)?gEnv->pEntitySystem->GetEntity(id.n):NULL);
		if(pEntity && pEntity->GetAI())
		{
			IAIObject *pAIObj(pEntity->GetAI());
				if(pAIObj->GetProxy())
					++(pAIObj->GetProxy()->DebugValue());
		}
		return true;
	}
*/
	else
		return CActor::CreateCodeEvent(rTable);
}

void CPlayer::PlayAction(const char *action,const char *extension, bool looping) 
{
	if (!m_pAnimatedCharacter)
		return;

	if (extension==NULL || strcmp(extension,"ignore")!=0)
	{
	if (extension && extension[0])
			strncpy(m_params.animationAppendix,extension,32);
		else
			strcpy(m_params.animationAppendix,"nw");

		m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputItem, m_params.animationAppendix );
	}

	if (looping)
		m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( "Action", action );
	else
		m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( "Signal", action );
}

void CPlayer::AnimationControlled(bool activate)
{
	if (m_stats.animationControlled != activate)
	{
		m_stats.animationControlled = activate;
		m_stats.followCharacterHead = activate?1:0;

		HolsterItem(activate);

		/*
		//before the sequence starts, remove any local offset of the player model.
		if (activate)
		{
			if (m_forceWorldTM.GetTranslation().len2()<0.01f)
				m_forceWorldTM = pEnt->GetWorldTM();

			pEnt->SetWorldTM(m_forceWorldTM * Matrix34::CreateTranslationMat(-GetEntity()->GetSlotLocalTM(0,false).GetTranslation()));
		}
		//move the player at the exact position animation brought him during the sequence.
		else
		{
			Vec3 localPos(GetStanceInfo(m_stance)->modelOffset);

			int bip01ID = GetBoneID(BONE_BIP01);
			if (bip01ID>-1)
			{
				Vec3 bipPos = pEnt->GetCharacter(0)->GetISkeleton()->GetAbsJPositionByID(bip01ID);
				bipPos.z = 0;
				localPos -= bipPos;
			}

			pEnt->SetWorldTM(pEnt->GetSlotWorldTM(0) * Matrix34::CreateTranslationMat(-localPos));

			m_forceWorldTM.SetIdentity();
		}*/
	}
}

void CPlayer::HandleEvent( const SGameObjectEvent& event )
{
	/*else if (!stricmp(event.event, "CameraFollowHead"))
	{
		m_stats.followCharacterHead = 1;
	}
	else if (!stricmp(event.event, "NormalCamera"))
	{
		m_stats.followCharacterHead = 0;
	}
	else*/
	if (event.event == eCGE_AnimateHands)
	{
		CreateScriptEvent("AnimateHands",0,(const char *)event.param);
	}
	else if (event.event == eCGE_PreFreeze)
	{
		if (event.param)
			GetGameObject()->SetPhysicalizationProfile(eAP_Frozen);
	}
	else if (event.event == eCGE_PostFreeze)
	{
		if (event.param)
			gEnv->pAISystem->ModifySmartObjectStates(GetEntity(), "Frozen");
		else
		{
			if (gEnv->bServer && GetHealth()>0)
				GetGameObject()->SetPhysicalizationProfile(eAP_Alive);
		}

		if (GetCurrentItemId(false))
			g_pGame->GetGameRules()->FreezeEntity(GetCurrentItemId(false), event.param!=0, true);
	}
	else if (event.event == eCGE_PreShatter)
	{
		// see also CPlayer::HandleEvent
		if (gEnv->bServer)
			GetGameObject()->SetPhysicalizationProfile(eAP_NotPhysicalized);

		m_stats.isHidden=true;
		m_stats.isShattered=true;
		GetEntity()->SetSlotFlags(0, GetEntity()->GetSlotFlags(0)&(~ENTITY_SLOT_RENDER));
	}
	else if (event.event == eCGE_OpenParachute)
	{
		m_openParachuteTimer = 1.0f;
		m_openingParachute = true;
	}
	else
		CActor::HandleEvent(event);
}

void CPlayer::UpdateGrab(float frameTime)
{
	CActor::UpdateGrab(frameTime);
}

float CPlayer::GetActorStrength() const
{
	float strength = 1.0f;
	if(m_pNanoSuit)
		strength += (m_pNanoSuit->GetSlotValue(NANOSLOT_STRENGTH)*0.01f)*g_pGameCVars->cl_strengthscale;
	return strength;
}

void CPlayer::ProcessBonesRotation(ICharacterInstance *pCharacter,float frameTime)
{
	CActor::ProcessBonesRotation(pCharacter,frameTime);

	if (!IsPlayer() || m_stats.isFrozen.Value() || m_stats.isRagDoll || !pCharacter || m_linkStats.GetLinked())
		return;

	//apply procedural leaning, for the third person character.
	Ang3 headAnglesGoal(0,m_viewRoll * 3.0f,0);
	Interpolate(m_headAngles,headAnglesGoal,10.0f,frameTime,30.0f);

	if (m_headAngles.y*m_headAngles.y > 0.001f)
	{
		Ang3 headAngles(m_headAngles);

		headAngles.y *= 0.25f;

		int16 id[3];
		id[0] = GetBoneID(BONE_SPINE);
		id[1] = GetBoneID(BONE_SPINE2);
		id[2] = GetBoneID(BONE_SPINE3);

		float leanValues[3];
		leanValues[0] = m_headAngles.y * 0.9f;
		leanValues[1] = -m_headAngles.y * 0.15f;
		leanValues[2] = -m_headAngles.y * 0.15f;

		for (int i=0;i<3;++i)
		{
			QuatT lQuat;
			int16 JointID = id[i];

			if (JointID>-1)
			{
				lQuat = pCharacter->GetISkeleton()->GetRelJQuatByID(JointID);
				lQuat.q *= Quat::CreateRotationAA(leanValues[i],Vec3(0.0f,1.0f,0.0f));

				pCharacter->GetISkeleton()->SetPostProcessQuat(JointID, lQuat);
			}
		}
	}
}

//TODO: clean it up, less redundancy, more efficiency etc..
void CPlayer::ProcessIKLegs(ICharacterInstance *pCharacter,float frameTime)
{
	if (GetGameObject()->IsProbablyDistant() && !GetGameObject()->IsProbablyVisible())
		return;

	static bool bOnce = true;
	static int nDrawIK = 0;
	static int nNoIK = 0;
	if (bOnce)
	{
		bOnce = false;
		gEnv->pConsole->Register( "player_DrawIK",&nDrawIK,0 );
		gEnv->pConsole->Register( "player_NoIK",&nNoIK,0 );
	}

	if (nNoIK)
	{
		//pCharacter->SetLimbIKGoal(LIMB_LEFT_LEG);
		//pCharacter->SetLimbIKGoal(LIMB_RIGHT_LEG);
		return;
	}

	float stretchLen(0.9f);

	switch(m_stance)
	{
	default: break;
	case STANCE_STAND: stretchLen = 1.1f;break;
	}

	//Vec3 localCenter(GetEntity()->GetSlotLocalTM(0,false).GetTranslation());
	int32 id = GetBoneID(BONE_BIP01);
	Vec3 localCenter(0,0,0);
	if (id>=0)
		//localCenter = bip01->GetBonePosition();
		localCenter = pCharacter->GetISkeleton()->GetAbsJQuatByID(id).t;

	Vec3 feetLpos[2];

	Matrix33 transMtx(GetEntity()->GetSlotWorldTM(0));
	transMtx.Invert();

	for (int i=0;i<2;++i)
	{
	//	int limb = (i==0)?LIMB_LEFT_LEG:LIMB_RIGHT_LEG;
		feetLpos[i] = Vec3(ZERO); //pCharacter->GetLimbEndPos(limb);

		Vec3 feetWpos = GetEntity()->GetSlotWorldTM(0) * feetLpos[i];
		ray_hit hit;

		float testExcursion(localCenter.z);

		int rayFlags = (COLLISION_RAY_PIERCABILITY & rwi_pierceability_mask);
		if (gEnv->pPhysicalWorld->RayWorldIntersection(feetWpos + m_baseQuat.GetColumn2()*testExcursion, m_baseQuat.GetColumn2()*(testExcursion*-0.95f), ent_terrain|ent_static/*|ent_rigid*/, rayFlags, &hit, 1))
		{		
			m_feetWpos[i] = hit.pt; 
			Vec3 footDelta = transMtx * (m_feetWpos[i] - feetWpos);
			Vec3 newLPos(feetLpos[i] + footDelta);

			//pCharacter->SetLimbIKGoal(limb,newLPos,ik_leg,0,transMtx * hit.n);

			//CryLogAlways("%.1f,%.1f,%.1f",hit.n.x,hit.n.y,hit.n.z);
		}
		//else
		//pCharacter->SetLimbIKGoal(limb);
	}
}

void CPlayer::ChangeParachuteState(int8 newState)
{
	bool changed(m_stats.inFreefall.Value()!=newState);
	if (changed)
	{
		switch(newState)
		{
		case 0:
			{
				//add some view kickback on landing
				if (m_stats.inFreefall.Value()>0)
					AddAngularImpulse(Ang3(-0.5f,RANDOM()*0.5f,RANDOM()*0.35f),0.0f,0.5f);

				//remove the parachute, if one was loaded. additional sounds should go in here
				if (m_nParachuteSlot)				
				{		
					int flags = GetEntity()->GetSlotFlags(m_nParachuteSlot)&~ENTITY_SLOT_RENDER;			
					GetEntity()->SetSlotFlags(m_nParachuteSlot,flags );		
				}

				if (!GetISystem()->IsSerializingFile() == 1 && IsClient())
					SAFE_HUD_FUNC(OnExitVehicle(this));
			}
			break;

		case 2:
			{
				IEntity *pEnt = GetEntity();
				// load and draw the parachute
				if (!m_nParachuteSlot)								
					m_nParachuteSlot=pEnt->LoadCharacter(10,"Objects/Vehicles/Parachute/parachute_opening.chr");				
				if (m_nParachuteSlot) // check if it was correctly loaded...dont wanna modify another character slot
				{			
					m_fParachuteMorph=0;
					ICharacterInstance *pCharacter=pEnt->GetCharacter(m_nParachuteSlot);				
					if (pCharacter)
					{
						pCharacter->SetLinearMorphSequence(m_fParachuteMorph);					
					}
					int flags = pEnt->GetSlotFlags(m_nParachuteSlot)|ENTITY_SLOT_RENDER;
					pEnt->SetSlotFlags(m_nParachuteSlot,flags );
				}

				AddAngularImpulse(Ang3(1.35f,RANDOM()*0.5f,RANDOM()*0.5f),0.0f,1.5f);

				if (IPhysicalEntity *pPE = pEnt->GetPhysics())
				{
					pe_action_impulse actionImp;
					actionImp.impulse = Vec3(0,0,9.81f) * m_stats.mass;
					actionImp.iApplyTime = 0;
					pPE->Action(&actionImp);
				}

				//if (IsClient())
					//SAFE_HUD_FUNC(OnEnterVehicle(this,"Parachute","Open",m_stats.isThirdPerson));
			}
			break;

		case 1:
			if (IsClient() && m_parachuteEnabled)
				SAFE_HUD_FUNC(OnEnterVehicle(this,"Parachute","Closed",m_stats.isThirdPerson));
			break;

		case 3://This is opening the parachute
			if (IsClient())
				SAFE_HUD_FUNC(OnEnterVehicle(this,"Parachute","Open",m_stats.isThirdPerson));
			break;
		}
	}

	m_stats.inFreefall = newState;
}

void CPlayer::Landed(float fallSpeed)
{

	static const int objTypes = ent_all;    
	static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
	ray_hit hit;
	Vec3 down = Vec3(0,0,-1.0f);
	IPhysicalEntity *phys = GetEntity()->GetPhysics();
	IMaterialEffects *mfx = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
	int matID = mfx->GetDefaultSurfaceIndex();
	int col = gEnv->pPhysicalWorld->RayWorldIntersection(GetEntity()->GetWorldPos(), (down * 5.0f), objTypes, flags, &hit, 1, phys);
	if (col)
	{
		matID = hit.surface_idx;
	}
	
	TMFXEffectId effectId = mfx->GetEffectId("bodyfall", matID);
	if (effectId != InvalidEffectId)
	{
		SMFXRunTimeEffectParams params;
		Vec3 direction = Vec3(0,0,0);
		if (IMovementController *pMV = GetMovementController())
		{
			SMovementState state;
			pMV->GetMovementState(state);
			direction = state.aimDirection;
		}
		params.pos = GetEntity()->GetWorldPos() + direction;
		float landFallParamVal = (fallSpeed > 7.5f) ? 0.75f : 0.25f;
		if(m_pNanoSuit && m_pNanoSuit->GetMode() == NANOMODE_DEFENSE)
			landFallParamVal = 0.0f;
		params.AddSoundParam("landfall", landFallParamVal);
		const float speedParamVal = min(fabsf((m_stats.velocity.z/10.0f)), 1.0f);
		params.AddSoundParam("speed", speedParamVal);
		if(gEnv->pSystem->IsEditor()) // Temporarily deactivate body fall sounds, to prevent x64 Editor crash in FMOD
		{
			#ifndef WIN64
						mfx->ExecuteEffect(effectId, params);
			#endif		 
		}
		else
			mfx->ExecuteEffect(effectId, params);
	}

	if(m_stats.inZeroG && GravityBootsOn() && m_pNanoSuit)
	{
		m_pNanoSuit->PlaySound(ESound_GBootsLanded, min(fabsf((m_stats.velocity.z/10.0f)), 1.0f));
		if(IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.1f, 0.2f, 1.0f*clamp_tpl(fallSpeed*0.1f, 0.0f, 1.0f) ) );
	}
	else if(IsClient())
		gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.1f, 0.5f, 0.9f*clamp_tpl(fallSpeed*0.1f, 0.0f, 1.0f) ) );
}

//------------------------------------------------------------------------
// animation-based footsteps sound playback
void CPlayer::UpdateFootSteps(float frameTime)
{
	if (GetLinkedEntity())
		return;

	SActorStats *pStats = GetActorStats();

	if(!pStats || (!pStats->onGround && !pStats->inZeroG))
		return;

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	ISkeleton *pSkeleton = NULL;
	if (pCharacter && pCharacter->GetISkeleton())
		pSkeleton = pCharacter->GetISkeleton();
	else
		return;

	uint32 numAnimsLayer0 = pSkeleton->GetNumAnimsInFIFO(0);
	if(!numAnimsLayer0)
		return;

	IAnimationSet* pIAnimationSet = pCharacter->GetIAnimationSet();
	CAnimation& animation=pSkeleton->GetAnimFromFIFO(0,0);
	float normalizedtime = animation.m_fAnimTime; //0.0f=left leg on ground, 0.5f=right leg on ground, 1.0f=left leg on ground

	//which foot is next ?
	EBonesID footID = BONE_FOOT_L;
	if(normalizedtime > 0.0f && normalizedtime <= 0.5f)
		footID = BONE_FOOT_R;

	if(footID == m_currentFootID)	//don't play the same sound twice ...
		return;

	float relativeSpeed = pSkeleton->GetCurrentVelocity().GetLength() / 7.0f; //hardcoded :-( 7m/s max speed

	if(relativeSpeed > 0.03f)	//if fast enough => play sound
	{
		int boneID = GetBoneID(m_currentFootID);

		//CryLogAlways("%f", relativeSpeed);

		// setup sound params
		SMFXRunTimeEffectParams params;
		params.angle = GetEntity()->GetWorldAngles().z + g_PI/2.0f;
		if(boneID == -1)	//no feet found
			params.pos = GetEntity()->GetWorldPos();
		else	//set foot position
		{
			params.pos = params.decalPos = GetEntity()->GetSlotWorldTM(0) * pSkeleton->GetAbsJQuatByID(GetBoneID(m_currentFootID, 0)).t;
		}
		params.AddSoundParam("speed", relativeSpeed);

		if(m_stats.waterLevel < 0.0f)
			CreateScriptEvent("splash",0);
		
		bool playFirstPerson = false;
		if(IsClient() && !IsThirdPerson())
			playFirstPerson = true;
		params.playSoundFP = playFirstPerson;

		//create effects
		EStance stance = GetStance();
		IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
		TMFXEffectId effectId = InvalidEffectId;
		float feetWaterLevel = gEnv->p3DEngine->GetWaterLevel(&params.pos);
		if (feetWaterLevel != WATER_LEVEL_UNKNOWN && feetWaterLevel>params.pos.z)
		{
			effectId = pMaterialEffects->GetEffectIdByName("footsteps", "water");
		}
		else
		{
			effectId = pMaterialEffects->GetEffectId("footstep", pStats->groundMaterialIdx);
		}

		TMFXEffectId gearEffectId = InvalidEffectId;

		float gearEffectPossibility = relativeSpeed*1.3f;
		if(stance == STANCE_CROUCH || stance == STANCE_PRONE)
		{
			gearEffectPossibility *= 10.0f;
			if(stance == STANCE_PRONE)
				relativeSpeed *= 0.5f;
		}
		if(cry_frand() < gearEffectPossibility)
			gearEffectId = pMaterialEffects->GetEffectIdByName("footsteps", "gear");

		Vec3 proxyOffset = Vec3(ZERO);
		Matrix34 tm = GetEntity()->GetWorldTM();
		tm.Invert();
		params.soundProxyOffset = tm.TransformVector(params.pos - GetEntity()->GetWorldPos());

		//play effects
		pMaterialEffects->ExecuteEffect(effectId, params);
		if(gearEffectId != InvalidEffectId)
			pMaterialEffects->ExecuteEffect(gearEffectId, params);

		//switch foot
		m_currentFootID = footID;	

		//handle AI sound recognition *************************************************
		float footstepRadius = 6;
		float proneMult = .1f;
		float crouchMult = .3f;
		float movingMult = 2.0f;

		IMaterialManager *mm = gEnv->p3DEngine->GetMaterialManager();
		ISurfaceType *surface = mm->GetSurfaceTypeManager()->GetSurfaceType(pStats->groundMaterialIdx);
		if (surface) 
		{
			const ISurfaceType::SSurfaceTypeAIParams *aiParams = surface->GetAIParams();
			if (aiParams)
			{
				footstepRadius = aiParams->fFootStepRadius;
				crouchMult = aiParams->crouchMult;
				proneMult = aiParams->proneMult;
				movingMult = aiParams->movingMult;
			}
		}

		if (stance == STANCE_CROUCH)
			footstepRadius *= crouchMult;
		else if (stance == STANCE_PRONE)
			footstepRadius *= proneMult;
		else if (relativeSpeed > .1)
		{
			float maxSpeed = GetStanceInfo(stance)->maxSpeed;
			float mult = 1.0 - (max(0.0f, maxSpeed - relativeSpeed)/maxSpeed);
			footstepRadius *= (1 + mult);
		}

		CPlayer *pPlayer = (this->GetActorClass() == CPlayer::GetActorClassType()) ? (CPlayer*) this : 0;

		float soundDamp = 1.0f;
		if(pPlayer != 0)
			if(CNanoSuit *pSuit = pPlayer->GetNanoSuit())
				soundDamp = pSuit->GetCloak()->GetSoundDamp();
		footstepRadius *= soundDamp;

		IAISystem *aiSys = gEnv->pAISystem;
		if (aiSys)
		{
			if (GetEntity()->GetAI())
				aiSys->SoundEvent(GetEntity()->GetWorldPos(), footstepRadius, AISE_MOVEMENT, GetEntity()->GetAI());
			else
				aiSys->SoundEvent(GetEntity()->GetWorldPos(), footstepRadius, AISE_MOVEMENT, NULL);
		}
	}
}

void CPlayer::SwitchDemoModeSpectator(bool activate)
{
	if(!(GetISystem()->IsDemoMode() == 2))
		return;

	m_bDemoModeSpectator = activate;

	m_stats.isThirdPerson = !activate;
	if(activate)
		m_stats.firstPersonBody = (uint8)g_pGameCVars->cl_fpBody;
	CItem *pItem = GetItem(GetInventory()->GetCurrentItem());
	if(pItem)
		pItem->UpdateFPView(0);

	IVehicle* pVehicle = GetLinkedVehicle();
	if (pVehicle)
	{
		IVehicleSeat* pVehicleSeat = pVehicle->GetSeatForPassenger( GetEntityId() );
		if (pVehicleSeat)
			pVehicleSeat->SetView( activate ? pVehicleSeat->GetNextView(InvalidVehicleViewId) : InvalidVehicleViewId );
	}

	if (activate)
	{
		IScriptSystem * pSS = gEnv->pScriptSystem;
		pSS->SetGlobalValue( "g_localActor", GetGameObject()->GetEntity()->GetScriptTable() );
		pSS->SetGlobalValue( "g_localActorId", ScriptHandle( GetGameObject()->GetEntityId() ) );
	}
}

void CPlayer::ActivateNanosuit(bool active)
{
	if(active)
	{
		if(!m_pNanoSuit)	//created on first activation
			m_pNanoSuit = new CNanoSuit();

		m_pNanoSuit->Reset(this);
		m_pNanoSuit->Activate(true);
	}
	else if(m_pNanoSuit)
	{
		m_pNanoSuit->Activate(false);
	}
}

void CPlayer::SetFlyMode(uint8 flyMode)
{
	if (m_stats.spectatorMode)
		return;

	m_stats.flyMode = flyMode;

	if (m_stats.flyMode>2)
		m_stats.flyMode = 0;

	m_pAnimatedCharacter->RequestPhysicalColliderMode((m_stats.flyMode==2)?eColliderMode_Disabled:eColliderMode_Undefined, eColliderModeLayer_Game);
}

void CPlayer::SetSpectatorMode(uint8 mode, EntityId targetId)
{
	//CryLog("%s setting spectator mode %d", GetEntity()->GetName(), mode);
	uint8 oldSpectatorMode=m_stats.spectatorMode;
	bool server=gEnv->bServer;
	ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0);
	if (mode && !m_stats.spectatorMode)
	{
		Revive(false);

		if (server)
		{
			GetGameObject()->SetPhysicalizationProfile(eAP_Spectator);
			GetGameObject()->InvokeRMI(CActor::ClSetSpectatorMode(), CActor::SetSpectatorModeParams(mode, targetId), eRMI_ToAllClients|eRMI_NoLocalCalls);
		}

		Draw(false);

		m_stats.spectatorTarget = targetId;
		m_stats.spectatorMode=mode;
		m_stats.inAir=0.0f;
		m_stats.onGround=0.0f;
	}
	else if (!mode && m_stats.spectatorMode)
	{
		if (server)
		{
			GetGameObject()->SetPhysicalizationProfile(eAP_Alive);
			GetGameObject()->InvokeRMI(CActor::ClSetSpectatorMode(), CActor::SetSpectatorModeParams(mode, targetId), eRMI_ToAllClients|eRMI_NoLocalCalls);
		}

		Draw(true);

		m_stats.spectatorTarget = 0;
		m_stats.spectatorMode=mode;
		m_stats.inAir=0.0f;
		m_stats.onGround=0.0f;
	}
	else if (oldSpectatorMode!=mode || m_stats.spectatorTarget != targetId)
	{
		m_stats.spectatorTarget = targetId;
		m_stats.spectatorMode=mode;
		
		if (server)
		{
			SetHealth(GetMaxHealth());
			GetGameObject()->InvokeRMI(CActor::ClSetSpectatorMode(), CActor::SetSpectatorModeParams(mode, targetId), eRMI_ToClientChannel|eRMI_NoLocalCalls, GetChannelId());			
		}
	}
/*

	// switch on/off spectator HUD
	if (IsClient())
		SAFE_HUD_FUNC(Show(mode==0));
*/
}

void CPlayer::SetSpectatorTarget(EntityId targetId)
{
	//CryLog("%s spectating %d", GetEntity()->GetName(), GetEntityId());
	SetSpectatorMode(CActor::eASM_Follow, targetId);
	MoveToSpectatorTargetPosition();
}

void CPlayer::MoveToSpectatorTargetPosition()
{
	// called when our target entity moves.
	IEntity* pTarget = gEnv->pEntitySystem->GetEntity(m_stats.spectatorTarget);
	if(!pTarget)
		return;

	Matrix34 tm = pTarget->GetWorldTM();
	tm.AddTranslation(Vec3(0,0,2));
	GetEntity()->SetWorldTM(tm);
}

void CPlayer::Stabilize(bool stabilize)
{
	m_bStabilize = stabilize;
	if(!stabilize && m_pNanoSuit)
		m_pNanoSuit->PlaySound(ESound_ZeroGThruster, 1.0f, true);
}

bool CPlayer::UseItem(EntityId itemId)
{
	const bool bOK = CActor::UseItem(itemId);
	if (bOK)
	{
		CALL_PLAYER_EVENT_LISTENERS(OnItemUsed(this, itemId));
	}
	return bOK;
}

bool CPlayer::PickUpItem(EntityId itemId, bool sound)
{
	const bool bOK = CActor::PickUpItem(itemId, sound);
	if (bOK)
	{
		CALL_PLAYER_EVENT_LISTENERS(OnItemPickedUp(this, itemId));
	}
	return bOK;
}

bool CPlayer::DropItem(EntityId itemId, float impulseScale, bool selectNext, bool byDeath)
{
	const bool bOK = CActor::DropItem(itemId, impulseScale, selectNext, byDeath);
	if (bOK)
	{
		CALL_PLAYER_EVENT_LISTENERS(OnItemDropped(this, itemId));
	}
	return bOK;
}

void CPlayer::UpdateUnfreezeInput(const Ang3 &deltaRotation, const Vec3 &deltaMovement, float mult)
{
	// unfreeze with mouse shaking and movement keys  
	float deltaRot = (abs(deltaRotation.x) + abs(deltaRotation.z)) * mult;
	float deltaMov = abs(deltaMovement.x) + abs(deltaMovement.y);

	static float color[] = {1,1,1,1};    

	if (g_pGameCVars->cl_debugFreezeShake)
	{    
		gEnv->pRenderer->Draw2dLabel(100,50,1.5,color,false,"frozenAmount: %f (actual: %f)", GetFrozenAmount(true), m_frozenAmount);
		gEnv->pRenderer->Draw2dLabel(100,80,1.5,color,false,"deltaRotation: %f (freeze mult: %f)", deltaRot, mult);
		gEnv->pRenderer->Draw2dLabel(100,110,1.5,color,false,"deltaMovement: %f", deltaMov);
	}

	float freezeDelta = deltaRot*g_pGameCVars->cl_frozenMouseMult + deltaMov*g_pGameCVars->cl_frozenKeyMult;

	if (freezeDelta >0)    
	{
		if (CNanoSuit *pSuit = GetNanoSuit())
		{
			if(pSuit && pSuit->IsActive())
			{
				// add suit strength
				float strength = pSuit->GetSlotValue(NANOSLOT_STRENGTH);
				strength = max(-0.75f, (strength-50)/50.f) * freezeDelta;
				freezeDelta += strength;

				if (g_pGameCVars->cl_debugFreezeShake)
				{ 
					gEnv->pRenderer->Draw2dLabel(100,140,1.5,color,false,"freezeDelta: %f (suit mod: %f)", freezeDelta, strength);
				}
			}
		}

		GetGameObject()->InvokeRMI(SvRequestUnfreeze(), UnfreezeParams(freezeDelta), eRMI_ToServer);

		float prevAmt = GetFrozenAmount(true);
		m_frozenAmount-=freezeDelta;
    float newAmt = GetFrozenAmount(true);
		if (freezeDelta > g_pGameCVars->cl_frozenSoundDelta || newAmt!=prevAmt)
			CreateScriptEvent("unfreeze_shake", newAmt!=prevAmt ? freezeDelta : 0);
	}
}

void CPlayer::SpawnParticleEffect(const char* effectName, const Vec3& pos, const Vec3& dir)
{
	IParticleEffect *pEffect = gEnv->p3DEngine->FindParticleEffect(effectName, "CPlayer::SpawnParticleEffect");
	if (pEffect == NULL)
		return;

	pEffect->Spawn(true, IParticleEffect::ParticleLoc(pos, dir, 1.0f));
}

void CPlayer::PlaySound(EPlayerSounds sound, bool play, bool param /*= false*/, const char* paramName /*=NULL*/, float paramValue /*=0.0f*/)
{
	if(!gEnv->pSoundSystem)
		return;
	if(!IsClient()) //currently this is only supposed to be heared by the client (not 3D, not MP)
		return;

	bool repeating = false;

	const char* soundName = NULL;
	switch(sound)
	{
	case ESound_Run:
		soundName = "sounds/physics:player_foley:run_feedback";
		repeating = true;
		break;
	case ESound_StopRun:
		soundName = "sounds/physics:player_foley:catch_breath_feedback";
		break;
	case ESound_Jump:
		soundName = "Sounds/physics:player_foley:jump_feedback";
		gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 0.05f, 0.1f) );
		break;
	case ESound_Fall_Drop:
		soundName = "Sounds/physics:player_foley:bodyfall_feedback";
		gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.2f, 0.3f, 0.2f) );
		break;
	case ESound_Melee:
		soundName = "Sounds/physics:player_foley:melee_feedback";
		gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.15f, 0.6f, 0.2f) );
		break;
	case ESound_Fear:
		soundName = "Sounds/physics:player_foley:alien_feedback";
		repeating = true;
		break;
	case ESound_Choking:
		soundName = "Languages/dialog/ai_korean01/choking_01.wav";
		repeating = true;
		break;
	case ESound_Hit_Wall:
		soundName = "Sounds/physics:player_foley:body_hits_wall";
		repeating = false;
		param = true;
		break;
	case ESound_UnderwaterBreathing:
		soundName = "Sounds/physics:player_foley:underwater_feedback";
		repeating = true;
		break;
	case ESound_Underwater:
		soundName = "sounds/environment:amb_natural:ambience_underwater";
		repeating = true;
		break;
	case ESound_Drowning:
		soundName = "Sounds/physics:player_foley:drowning_feedback";
		repeating = false;
		break;
	case ESound_DiveIn:
		soundName = "Sounds/physics:player_foley:dive_in";
		repeating = false;
		break;
	case ESound_DiveOut:
		soundName = "Sounds/physics:player_foley:dive_out";
		repeating = false;
		break;
	default:
		break;
	}

	if(!soundName)
		return;

	if(play)
	{
		ISound *pSound = NULL;
		if(repeating && m_sounds[sound])
			if(pSound = gEnv->pSoundSystem->GetSound(m_sounds[sound]))
				if(pSound->IsPlaying())
					return;

		if(!pSound)
			pSound = gEnv->pSoundSystem->CreateSound(soundName, 0);
		if(pSound)
		{
			if(repeating)
				m_sounds[sound] = pSound->GetId();
			pSound->Play();
			if(param)
				pSound->SetParam(paramName,paramValue);
		}
	}
	else if(repeating && m_sounds[sound])
	{
		ISound *pSound = gEnv->pSoundSystem->GetSound(m_sounds[sound]);
		if(pSound)
			pSound->Stop();
		m_sounds[sound] = 0;
	}
}

//===========================LADDERS======================

// NB this will store the details of the usable ladder in m_stats
bool CPlayer::IsLadderUsable()
{
	IPhysicalEntity* pPhysicalEntity;

	const ray_hit *pRay = GetGameObject()->GetWorldQuery()->GetLookAtPoint(2.0f);

	//Check collision with a ladder
	if (pRay)
		pPhysicalEntity = pRay->pCollider;
	else 
		return false;

	//Check the Material (first time)
	if(!m_stats.ladderMaterial)
	{
		ISurfaceType *ladderSurface = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_ladder");
		if(ladderSurface)
		{
			m_stats.ladderMaterial = ladderSurface->GetId();
		}
	}

	//Is it a ladder?
	if(pPhysicalEntity && pRay->surface_idx == m_stats.ladderMaterial)
	{
		IStatObj* staticLadder = NULL;
		Matrix34 worldTM;
		Matrix34 partLocalTM;

		//Get Static object from physical entity
		pe_params_part ppart;
		ppart.partid  = pRay->partid;
		ppart.pMtx3x4 = &partLocalTM;
		if (pPhysicalEntity->GetParams( &ppart ))
		{
			if (ppart.pPhysGeom && ppart.pPhysGeom->pGeom)
			{
				void *ptr = ppart.pPhysGeom->pGeom->GetForeignData(0);
				staticLadder = (IStatObj*)ptr;

				pe_status_pos ppos;
				ppos.pMtx3x4 = &worldTM;
				pPhysicalEntity->GetStatus(&ppos);
				worldTM = worldTM * partLocalTM;
			}
		}

		if(!staticLadder)
			return false;

		Vec3 ladderOrientation = worldTM.GetColumn1().normalize();
		Vec3 ladderUp = worldTM.GetColumn2().normalize();

		//Is ladder vertical? 
		if(ladderUp.z<0.85f)
		{
			return false;
		}

		AABB box = staticLadder->GetAABB();
		Vec3 center = worldTM.GetTranslation() + 0.5f*ladderOrientation;

		m_stats.ladderTop = center + (ladderUp*(box.max.z - box.min.z));
		m_stats.ladderBottom = center;
		m_stats.ladderOrientation = ladderOrientation;
		m_stats.ladderUpDir = ladderUp;

		//Test angle between player and ladder	
		IMovementController* pMC = GetMovementController();
		if(pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);

			//Predict if the player try to enter the top of the ladder from the opposite side
			float zDistance = m_stats.ladderTop.z - pRay->pt.z;
			if(zDistance<1.0f && ladderOrientation.Dot(-info.eyeDirection)<0.0f)
			{
				return true;
			}
// 			else if(ladderOrientation.Dot(-info.eyeDirection)<0.5f)
// 			{
// 				return false;
// 			}
		}	
		return true;
	}
	else
	{
		return false;
	}
}

void CPlayer::RequestGrabOnLadder(ELadderActionType reason)
{
	if(gEnv->bServer)
	{
		GrabOnLadder(reason);
	}
	else
	{
		GetGameObject()->InvokeRMI(SvRequestGrabOnLadder(), LadderParams(m_stats.ladderTop, m_stats.ladderBottom, reason), eRMI_ToServer);
	}
}

void CPlayer::GrabOnLadder(ELadderActionType reason)
{
	HolsterItem(true);

	m_stats.isOnLadder = true;
	m_stats.ladderAction = reason;

	m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_PushesPlayersOnly, eColliderModeLayer_Game);
	
	if(!m_stats.isThirdPerson)
		ToggleThirdPerson();

	// SNH: moved this from IsLadderUsable
	//if (!m_pGameFramework->IsServer())	
	{
		IMovementController* pMC = GetMovementController();
		if(pMC)
		{
			const ray_hit *pRay = GetGameObject()->GetWorldQuery()->GetLookAtPoint(2.0f);
			SMovementState info;
			pMC->GetMovementState(info);

			//Predict if the player try to enter the top of the ladder from the opposite side
			float zDistance = m_stats.ladderTop.z - pRay->pt.z;
			if(zDistance<1.0f && m_stats.ladderOrientation.Dot(-info.eyeDirection)<0.0f)
			{
				//Move the player in front of the ladder
				Matrix34 entryPos = GetEntity()->GetWorldTM();
				entryPos.SetTranslation(m_stats.ladderTop-(m_stats.ladderUpDir*1.9f));
				GetEntity()->SetWorldTM(entryPos);

				//Player orientation
				GetEntity()->SetRotation(Quat(Matrix33::CreateOrientation(-m_stats.ladderOrientation,m_stats.ladderUpDir,g_PI)));
				m_stats.playerRotation = GetEntity()->GetRotation();

				//Start to move down
				m_stats.ladderMovingDown = true;
				m_stats.ladderMovingUp = false;
			}
			else
			{
				//Move the player in front of the ladder
				Matrix34 entryPos = GetEntity()->GetWorldTM();
				entryPos.SetTranslation(Vec3(m_stats.ladderBottom.x,m_stats.ladderBottom.y,max(entryPos.GetTranslation().z,m_stats.ladderBottom.z)));
				GetEntity()->SetWorldTM(entryPos);

				//Player orientation
				GetEntity()->SetRotation(Quat(Matrix33::CreateOrientation(-m_stats.ladderOrientation,m_stats.ladderUpDir,g_PI)));
				m_stats.playerRotation = GetEntity()->GetRotation();

				//Start moving up
				m_stats.ladderMovingUp = true;
 				m_stats.ladderMovingDown = false;
			}
		}
	}

	if (gEnv->bServer)
		GetGameObject()->InvokeRMI(ClGrabOnLadder(), LadderParams(m_stats.ladderTop, m_stats.ladderBottom, reason), eRMI_ToRemoteClients);
}

void CPlayer::RequestLeaveLadder(ELadderActionType reason)
{
	if(gEnv->bServer)
	{
		LeaveLadder(reason);
	}
	else
	{
		GetGameObject()->InvokeRMI(SvRequestLeaveLadder(), LadderParams(m_stats.ladderTop, m_stats.ladderBottom, reason), eRMI_ToServer);
	}
}

void CPlayer::LeaveLadder(ELadderActionType reason)
{

	m_stats.isOnLadder  = false;
	m_stats.ladderAction = reason;

	m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Undefined, eColliderModeLayer_Game);

	if(m_stats.isThirdPerson)
		ToggleThirdPerson();

	HolsterItem(false);

 // SNH: moved this here from CPlayerMovement::ProcessExitLadder
	{
		Matrix34 finalPlayerPos = GetEntity()->GetWorldTM();

		Vec3 rightDir = m_stats.ladderUpDir.Cross(m_stats.ladderOrientation);
		rightDir.Normalize();

		switch(reason)
		{
			// top (was 0)
		case eLAT_ReachedEnd:
		{
			// test new position before putting the player there?
 			Vec3 newPos = m_stats.ladderTop-0.5f*m_stats.ladderOrientation;
			finalPlayerPos.SetTranslation(newPos);
			break;
		}

		case eLAT_StrafeRight:
			finalPlayerPos.AddTranslation(rightDir*0.3f);
			break;

		case eLAT_StrafeLeft:
			finalPlayerPos.AddTranslation(-rightDir*0.3f);
			break;

		case eLAT_Jump:
			finalPlayerPos.AddTranslation(m_stats.ladderOrientation*0.5);
			break;
		}

		GetEntity()->SetWorldTM(finalPlayerPos);	
	}
	UpdateLadderAnimGraph(CPlayer::eLS_Exit,CPlayer::eLDIR_Stationary);

	//Finally apply a little impulse to the player (to make it fall)
	if(GetEntity()->GetPhysics())
	{
		pe_action_impulse ip;
		ip.impulse = -m_stats.ladderOrientation * 15.0f;
		GetEntity()->GetPhysics()->Action(&ip);
	}
	if (gEnv->bServer)
		GetGameObject()->InvokeRMI(ClLeaveLadder(), LadderParams(m_stats.ladderTop, m_stats.ladderBottom, reason), eRMI_ToAllClients);

//	m_stats.ladderBottom = Vec3(0,0,0);
//	m_stats.ladderTop = Vec3(0,0,0);
//	m_stats.ladderUpDir = Vec3(0,0,0);
	m_stats.ladderMovingDown = false;
	m_stats.ladderMovingUp = false;

}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, SvRequestGrabOnLadder)
{
	if(IsLadderUsable() && m_stats.ladderTop.IsEquivalent(params.topPos) && m_stats.ladderBottom.IsEquivalent(params.bottomPos))
	{
		GrabOnLadder(static_cast<ELadderActionType>(params.reason));
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, SvRequestLeaveLadder)
{
	if(m_stats.isOnLadder)
	{
		if(m_stats.ladderTop.IsEquivalent(params.topPos) && m_stats.ladderBottom.IsEquivalent(params.bottomPos))
		{
			LeaveLadder(static_cast<ELadderActionType>(params.reason));
		}
	}
	
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, ClGrabOnLadder)
{
	if(IsLadderUsable() && m_stats.ladderTop.IsEquivalent(params.topPos) && m_stats.ladderBottom.IsEquivalent(params.bottomPos))
	{
		GrabOnLadder(static_cast<ELadderActionType>(params.reason));
	}
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, ClLeaveLadder)
{
	// probably not worth checking the positions here - just get off the ladder whatever.
	if(m_stats.isOnLadder)
	{
		LeaveLadder(static_cast<ELadderActionType>(params.reason));	
	}
	return true;
}

//-----------------------------------------------------------------------
bool CPlayer::UpdateLadderAnimGraph(ELadderState eLS, ELadderDirection eLDIR, float time /*=0.0f*/)
{
	switch(eLS)
	{
			case eLS_Exit:			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput("Action","idle");
														break;

			case eLS_Climb:			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput("Action","climbLadderUp");
														break;

			default:						break;
	}
	//Manual animation Update

	if(eLS==eLS_Climb)
	{
		if (ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0))
		{
			ISkeleton *pSkeleton = pCharacter->GetISkeleton();
			assert(pSkeleton);

			if (uint32 numAnimsLayer = pSkeleton->GetNumAnimsInFIFO(0))
			{
				CAnimation &animation = pSkeleton->GetAnimFromFIFO(0, 0);
				if (animation.m_AnimParams.m_nFlags & CA_MANUAL_UPDATE)
				{
					pSkeleton->ManualSeekAnimationInFIFO(0, 0, time, eLDIR == eLDIR_Up);
					return true;
				}
			}
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------
void CPlayer::EvaluateMovementOnLadder(float &verticalSpeed, float animationTime)
{

	//OPTION 1 (it plays whole animation cycles and stops)
	if(g_pGameCVars->i_debug_ladders == 1)
	{
		if((animationTime<0.02f || animationTime>0.98f) && m_stats.ladderMovingDown)
		{
			verticalSpeed = 0.0f;
			m_stats.ladderMovingDown = false;
		}
		else if((animationTime>0.98f || animationTime<0.02f) && m_stats.ladderMovingUp)
		{
			verticalSpeed = 0.0f;
			m_stats.ladderMovingUp = false;
		}
		else if(m_stats.ladderMovingUp)
		{
			verticalSpeed = 0.75f;
		}
		else if(m_stats.ladderMovingDown)
		{
			verticalSpeed = -0.75f;
		}
		else if(verticalSpeed>0.01f && !m_stats.ladderMovingDown && !m_stats.ladderMovingUp)
		{
			verticalSpeed = 0.75f;
			m_stats.ladderMovingUp = true;
		}
		else if(verticalSpeed<-0.01f && !m_stats.ladderMovingDown && !m_stats.ladderMovingUp)
		{
			verticalSpeed = -0.75f;
			m_stats.ladderMovingDown = true;
		}
	}
	else if(g_pGameCVars->i_debug_ladders == 2)
	{
		//OPTION 2 (Automatic movement, but I can change direction at any moment)
		if(verticalSpeed>0.01f)
		{
			m_stats.ladderMovingUp = true;
			m_stats.ladderMovingDown = false;
		}
		else if(verticalSpeed<-0.01f)
		{
			m_stats.ladderMovingDown = true;
			m_stats.ladderMovingUp = false;
		}
		else if(m_stats.ladderMovingUp)
		{
			verticalSpeed = 0.75f;
		}
		else if(m_stats.ladderMovingDown)
		{
			verticalSpeed = -0.75f;
		}
	}


}

void CPlayer::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	CActor::GetActorMemoryStatistics(s);
	if (m_pNanoSuit)
		m_pNanoSuit->GetMemoryStatistics(s);
	if (m_pPlayerInput.get())
		m_pPlayerInput->GetMemoryStatistics(s);
	s->AddContainer(m_clientPostEffects);
  if(m_pDebugHistory)
    m_pDebugHistory->GetMemoryStatistics(s);
}


void CPlayer::Debug()
{
  if(g_pGameCVars->pl_debug_movement == 0)
    return;

  if(!m_pDebugHistory)
    m_pDebugHistory = g_pGame->GetIGameFramework()->CreateDebugHistoryManager();
  
  if(!m_pMovementDebug)
  {
    m_pMovementDebug = m_pDebugHistory->CreateHistory("MovementDebug");
    m_pMovementDebug->SetupLayoutAbs(10, 220, 200, 200, 5);
    m_pMovementDebug->SetupScopeExtent(-360, +360, -1, +1);
  }
  if(!m_pDeltaXDebug)
  {
    m_pDeltaXDebug = m_pDebugHistory->CreateHistory("DeltaMovementX");
    m_pDeltaXDebug->SetupLayoutAbs(220, 220, 200, 200, 5);
    m_pDeltaXDebug->SetupScopeExtent(-360, +360, -1, +1);
  }
  if(!m_pDeltaYDebug)
  {
    m_pDeltaYDebug = m_pDebugHistory->CreateHistory("DeltaMovementY");
    m_pDeltaYDebug->SetupLayoutAbs(430, 220, 200, 200, 5);
    m_pDeltaYDebug->SetupScopeExtent(-360, +360, -1, +1);
  }
  
  bool filter = false;
  const char* filter_str = g_pGameCVars->pl_debug_filter->GetString();
  if(filter_str && strcmp(filter_str,"0"))
    filter = strcmp(GetEntity()->GetName(),filter_str) != 0;
  
  if(m_pMovementDebug)
    m_pMovementDebug->SetVisibility(!filter);
  if(m_pDeltaXDebug)
    m_pDeltaXDebug->SetVisibility(!filter);
  if(m_pDeltaYDebug)
    m_pDeltaYDebug->SetVisibility(!filter);
}

//Try to predict if the player needs to go to crouch stance to pick up a weapon/item
bool CPlayer::NeedToCrouch()
{
	if (IMovementController *pMC = GetMovementController())
	{
		SMovementState state;
		pMC->GetMovementState(state);

		//Maybe this "prediction" is not really accurate...
		//If the player is looking down, probably the item is on the ground
		if(state.aimDirection.z<-0.7f && GetStance()!=STANCE_PRONE)
		{
			m_stats.forceCrouchTime = 0.75f;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
void CPlayer::StartDisarmMine(EntityId entityId)
{
	if(gEnv->bServer)
	{
		m_mineDisarmStartTime = gEnv->pTimer->GetCurrTime();
		m_mineBeingDisarmed = entityId;
		m_disarmingMine = true;
	}
	else
	{
		GetGameObject()->InvokeRMI(SvRequestStartDisarmMine(), ItemIdParam(entityId), eRMI_ToServer);
	}
}

void CPlayer::EndDisarmMine(EntityId entityId)
{
	if(gEnv->bServer)
	{
		assert(m_mineBeingDisarmed == 0 || entityId == m_mineBeingDisarmed);

		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_mineBeingDisarmed);
		if(pEntity)
		{
			float fTime = 0.0f;

			if(pEntity->GetClass() == CItem::sClaymoreExplosiveClass)
				fTime = CClaymore::GetDisarmTime();
			else if(pEntity->GetClass() == CItem::sAVExplosiveClass)
				fTime = CAVMine::GetDisarmTime();

			if(gEnv->pTimer->GetCurrTime() >= (fTime + m_mineDisarmStartTime))
			{
				RemoveMineEntity(m_mineBeingDisarmed);
			}
		}

		m_disarmingMine = false;
		m_mineBeingDisarmed = 0;
		m_mineDisarmStartTime = 0.0f;
	}
	else
	{
		GetGameObject()->InvokeRMI(SvRequestEndDisarmMine(), ItemIdParam(entityId), eRMI_ToServer);
	}
}

void CPlayer::RemoveMineEntity(EntityId entityId)
{
	gEnv->pEntitySystem->RemoveEntity(entityId);
	m_disarmingMine = false;
	m_mineBeingDisarmed = 0;
	m_mineDisarmStartTime = 0.0f;
}

//------------------------------------------------------------------------
void CPlayer::EnterFirstPersonSwimming( )
{
	CFists *pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));
	if(!pFists)
		return;

	COffHand *pOffHand = static_cast<COffHand*>(GetItemByClass(CItem::sOffHandClass));
	//Drop object or NPC
	if ((pOffHand != NULL) && (pOffHand->GetOffHandState()&(eOHS_HOLDING_OBJECT|eOHS_HOLDING_NPC)))
	{
		pOffHand->OnAction(GetEntityId(),"use",eAAM_OnPress,0);
		pOffHand->OnAction(GetEntityId(),"use",eAAM_OnRelease,0);
	}

	//Entering the water (select fists...)
	CItem *currentItem = static_cast<CItem*>(GetCurrentItem());
	if(!currentItem)
	{
		//Player has no item selected... just select fists
		pFists->EnableAnimations(false);
		SelectItem(pFists->GetEntityId(),false);
		pFists->EnableAnimations(true);
	}
	else if(pFists->GetEntityId()==currentItem->GetEntityId())
	{
		//Fists already selected
		GetInventory()->SetLastItem(pFists->GetEntityId());
	}
	else
	{
		//Deselect current item and select fists
		currentItem->Select(false);
		pFists->EnableAnimations(false);
		SelectItem(pFists->GetEntityId(),false);
		pFists->EnableAnimations(true);
	}
	pFists->EnterWater(true);
	pFists->RequestAnimState(eFAS_SWIM_IDLE);

}

//------------------------------------------------------------------------
void CPlayer::ExitFirstPersonSwimming()
{
	CFists *pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));
	if(!pFists)
		return;

	// we probably left water, so reactivate last item
	EntityId lastItemId = GetInventory()->GetLastItem();
	CItem *lastItem = (CItem *)gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(lastItemId);

	pFists->EnterWater(false);
	pFists->ResetAnimation();
	pFists->GetScheduler()->Reset();
	pFists->RequestAnimState(eFAS_NOSTATE);
	pFists->RequestAnimState(eFAS_FIGHT);

	if (GetLinkedVehicle())
		HolsterItem(false);

	if (lastItemId && lastItem && (lastItemId != pFists->GetEntityId()))
	{
		CBinocular *pBinoculars = static_cast<CBinocular*>(GetItemByClass(CItem::sBinocularsClass));
		if ((pBinoculars == NULL) || (pBinoculars->GetEntityId() != lastItemId))
		{
			//Select last item, except binoculars
			pFists->Select(false);
			SelectItem(lastItemId, true);
		}
	}

	if (GetLinkedVehicle())
		HolsterItem(true);   

	UpdateFirstPersonSwimmingEffects(false,0.0f);

	m_bUnderwater = false;
}

//------------------------------------------------------------------------
void CPlayer::UpdateFirstPersonSwimming()
{
	bool swimming = false;

	if (ShouldSwim())
		swimming = true;

	if (m_stats.isOnLadder)
	{
		UpdateFirstPersonSwimmingEffects(false,0.0f);
		return;
	}
		
	if(swimming && !m_bSwimming)
	{
		EnterFirstPersonSwimming();
	}
	else if(!swimming && m_bSwimming)
	{
		ExitFirstPersonSwimming();
		m_bSwimming = swimming;
		return;
	}
	else if (swimming && m_bSwimming)
	{
		CFists *pFists = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));
		if(!pFists)
			return;

		//Retrieve some player info...
		pe_status_dynamics dyn;
		IPhysicalEntity *phys = GetEntity()->GetPhysics();
		if (phys)
			phys->GetStatus(&dyn);

		Vec3 direction(0,0,1);
		if (IMovementController *pMC = GetMovementController())
		{
			SMovementState state;
			pMC->GetMovementState(state);
			direction = state.aimDirection;
		}

		bool moving = (dyn.v.GetLengthSquared() > 1.0f);
		float  dotP = dyn.v.GetNormalized().Dot(direction);
		if (moving && fabs_tpl(dotP)>0.1f)
		{
			if (dotP >= 0.1f)
			{

				CNanoSuit *pSuit = GetNanoSuit();
				if(pSuit && pSuit->GetMode()==NANOMODE_SPEED && m_stats.bSprinting)
				{
					pFists->RequestAnimState(eFAS_SWIM_SPEED);
				}
				else
				{
					pFists->RequestAnimState(eFAS_SWIM_FORWARD);
				}

				m_stats.bSprinting = false;
			}
			else
			{
				pFists->RequestAnimState(eFAS_SWIM_BACKWARD);
			}
		}
		else
		{
			// idling in water
			if (m_stats.headUnderWater > 0.0f)
			{
				pFists->RequestAnimState(eFAS_SWIM_IDLE);
			}
			else
			{
				pFists->RequestAnimState(eFAS_SWIM_IDLE);
			}
		}

		UpdateFirstPersonSwimmingEffects(false,dyn.v.len2());
	}
	m_bSwimming = swimming;

}
//------------------------------------------------------------------------
void CPlayer::UpdateFirstPersonSwimmingEffects(bool exitWater, float velSqr)
{
	CScreenEffects *pSE = GetScreenEffects();
	if (pSE && (((m_stats.headUnderWater < 0.0f) && m_bUnderwater)||exitWater)) //going in and out of water
	{
		pSE->ClearBlendGroup(14);
		CPostProcessEffect *blend = new CPostProcessEffect(GetEntityId(), "WaterDroplets_Amount", 1.0f);
		CWaveBlend *wave = new CWaveBlend();
		CPostProcessEffect *blend2 = new CPostProcessEffect(GetEntityId(), "WaterDroplets_Amount", 0.0f);
		CWaveBlend *wave2 = new CWaveBlend();
		pSE->StartBlend(blend, wave, 1.0f/.25f, 14);
		pSE->StartBlend(blend2, wave2, 1.0f/3.0f, 14);
		SAFE_HUD_FUNC(PlaySound(ESound_WaterDroplets));
	}
	else if(pSE && (m_stats.headUnderWater > 0.0f) && !m_bUnderwater)
	{
		pSE->ClearBlendGroup(14);
		CPostProcessEffect *blend = new CPostProcessEffect(GetEntityId(), "WaterDroplets_Amount", 0.0f);
		CWaveBlend *wave = new CWaveBlend();
		pSE->StartBlend(blend,wave,4.0f,14);
		SAFE_HUD_FUNC(PlaySound(ESound_WaterDroplets));	
	}

	if(velSqr>40.0f && pSE)
	{
		//Only when sprinting in speed mode under water
		pSE->ClearBlendGroup(98);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Radius", 0.3f);
		gEnv->p3DEngine->SetPostEffectParam("FilterRadialBlurring_Amount", 1.0f);

		CPostProcessEffect *pBlur = new CPostProcessEffect(GetEntityId(),"FilterRadialBlurring_Amount", 0.0f);
		CLinearBlend *pLinear = new CLinearBlend(1.0f);
		pSE->StartBlend(pBlur, pLinear, 1.0f, 98);

	}
	m_bUnderwater = (m_stats.headUnderWater > 0.0f);
}

//------------------------------------------------------------------------
void CPlayer::UpdateFirstPersonFists()
{
	if(m_stats.inFreefall.Value())
	{
		return;
	}
	else if (ShouldSwim() || m_bSwimming)
	{
		UpdateFirstPersonSwimming();
		return;
	}

	COffHand *pOffHand = static_cast<COffHand*>(GetItemByClass(CItem::sOffHandClass));
	CFists   *pFists   = static_cast<CFists*>(GetItemByClass(CItem::sFistsClass));

	if(pFists && pFists->IsSelected() && pOffHand && !pOffHand->IsSelected())
	{

		//Retrieve some player info...
		pe_status_dynamics dyn;
		IPhysicalEntity *phys = GetEntity()->GetPhysics();
		if (phys)
			phys->GetStatus(&dyn);

		int stance = GetStance();

		//Select fists state...
		if(pFists->GetCurrentAnimState()==eFAS_JUMPING && (m_stats.landed || (m_stats.onGround>0.0001f && m_stats.inAir<=0.5f)))
			pFists->RequestAnimState(eFAS_LANDING);

		if(stance==STANCE_PRONE && dyn.v.GetLengthSquared()>0.2f)
			pFists->RequestAnimState(eFAS_CRAWL);
		else if(stance==STANCE_PRONE)
			pFists->RequestAnimState(eFAS_RELAXED);
		else if(stance!=STANCE_PRONE && pFists->GetCurrentAnimState()==eFAS_CRAWL)
			pFists->RequestAnimState(eFAS_RELAXED);
		

		if(m_stats.bSprinting && stance==STANCE_STAND)
		{
			//m_stats.bSprinting = false;		//Need to do this, or you barely saw the fists on screen...
			pFists->RequestAnimState(eFAS_RUNNING, true);
		}
		else if(pFists->GetCurrentAnimState()==eFAS_RUNNING)
		{
			if(m_stats.jumped || m_stats.inAir>0.15f)
				pFists->RequestAnimState(eFAS_JUMPING);
			else
				pFists->RequestAnimState(eFAS_RELAXED);
		}

	}		
}

bool CPlayer::HasHitAssistance()
{
	return m_bHasAssistance;
}

//------------------------------------------------------------------------
int32 CPlayer::GetArmor() const
{
	if(IsClient())
	{
		if(m_pNanoSuit && m_pNanoSuit->GetMode() == NANOMODE_DEFENSE)
		{
			return int32((m_pNanoSuit->GetSuitEnergy()/NANOSUIT_ENERGY)*g_pGameCVars->g_suitArmorHealthValue);
		}
	}
	return 0;
}

//------------------------------------------------------------------------
bool CPlayer::IsCloaked() const 
{ 
  if (m_pNanoSuit && m_pNanoSuit->GetCloak()->GetState() != 0)
    return true;

  return false;
}

//------------------------------------------------------------------------
int32 CPlayer::GetMaxArmor() const
{
	if(IsClient())
	{
		if(m_pNanoSuit && m_pNanoSuit->GetMode() == NANOMODE_DEFENSE)
		{
			return int32(g_pGameCVars->g_suitArmorHealthValue);
		}
	}
	return int32(0);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, SvRequestStartDisarmMine)
{
	StartDisarmMine(params.itemId);
	//RemoveMineEntity(params.itemId);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, SvRequestEndDisarmMine)
{
	EndDisarmMine(params.itemId);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, SvRequestUnfreeze)
{
	if (params.delta>0.0f && params.delta <=1.0f)
	{
		SetFrozenAmount(m_frozenAmount-params.delta);

		if (m_frozenAmount <= 0.0f)
			g_pGame->GetGameRules()->FreezeEntity(GetEntityId(), false, false);

		GetGameObject()->ChangedNetworkState(ASPECT_FROZEN);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, SvRequestHitAssistance)
{
	m_bHasAssistance=params.assistance;
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, ClEMP)
{
	if (params.entering)
	{
		CNanoSuit* pSuit = GetNanoSuit();
		pSuit->Activate(false);
		pSuit->SetSuitEnergy(0);

		if (IsClient())
		{
			CHUD* pHUD = g_pGame->GetHUD();
			if (pHUD)
			{
				pHUD->Show(false);
			}
		}
	}
	else
	{
		CNanoSuit* pSuit = GetNanoSuit();
		pSuit->Activate(true, 10);

		if (IsClient())
		{
			CHUD* pHUD = g_pGame->GetHUD();
			if (pHUD)
			{
				pHUD->Show(true);
				pHUD->BreakHUD();
			}
		}
	}

	return true;
}

void
CPlayer::StagePlayer(bool bStage, SStagingParams* pStagingParams /* = 0 */)
{
	if (IsClient() == false)
		return;

	bool bLock = false;
	if (bStage == false)
	{
		m_params.vLimitDir.zero();
		m_params.vLimitRangeH = 0.0f;
		m_params.vLimitRangeV = 0.0f;
	}
	else if (pStagingParams != 0)
	{
		bLock = pStagingParams->bLocked;
		m_params.vLimitDir = pStagingParams->vLimitDir;
		m_params.vLimitRangeH = pStagingParams->vLimitRangeH;
		m_params.vLimitRangeV = pStagingParams->vLimitRangeV;
		if (bLock)
		{
			IPlayerInput* pPlayerInput = GetPlayerInput();
			if(pPlayerInput)
				pPlayerInput->Reset();
		}
	}
	else
	{
		bStage = false;
	}

	if (bLock || m_stagingParams.bLocked)
	{
		IActionMapManager* pAmMgr = g_pGame->GetIGameFramework()->GetIActionMapManager();
		if (pAmMgr)
			pAmMgr->EnableFilter("no_move", bLock);
	}
	m_stagingParams.bActive = bStage;
	m_stagingParams.bLocked = bLock;
	m_stagingParams.vLimitDir = m_params.vLimitDir;
	m_stagingParams.vLimitRangeH = m_params.vLimitRangeH;
	m_stagingParams.vLimitRangeV = m_params.vLimitRangeV;
}
