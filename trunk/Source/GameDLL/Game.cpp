/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 3:8:2004   11:26 : Created by Márcio Martins
  - 17:8:2005        : Modified - NickH: Factory registration moved to GameFactory.cpp

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"
#include "Menus/FlashMenuObject.h"
#include "Menus/OptionsManager.h"

#include "GameRules.h"
#include "HUD/HUD.h"
#include "WeaponSystem.h"

#include <ICryPak.h>
#include <CryPath.h>
#include <IActionMapManager.h>
#include <IViewSystem.h>
#include <ILevelSystem.h>
#include <IItemSystem.h>
#include <IVehicleSystem.h>
#include <IGameTokens.h>
#include <IMovieSystem.h>
#include <IPlayerProfiles.h>

#include "ScriptBind_Actor.h"
#include "ScriptBind_Item.h"
#include "ScriptBind_Weapon.h"
#include "ScriptBind_GameRules.h"
#include "ScriptBind_Game.h"
#include "MusicLogic/ScriptBind_MusicLogic.h"
#include "MusicLogic/MusicLogic.h"
#include "HUD/ScriptBind_HUD.h"

#include "GameFactory.h"

#include "ItemSharedParams.h"

#include "Nodes/G2FlowBaseNode.h"

#include "ServerSynchedStorage.h"
#include "ClientSynchedStorage.h"

#define GAME_DEBUG_MEM  // debug memory usage
#undef  GAME_DEBUG_MEM

#define SAFE_MENU_FUNC(func)\
	if(m_pFlashMenuObject)\
		m_pFlashMenuObject->func

//FIXME: really horrible. Remove ASAP
int OnImpulse( const EventPhys *pEvent ) 
{ 
	//return 1;
	return 0;
}

//

// Needed for the Game02 specific flow node
CG2AutoRegFlowNodeBase *CG2AutoRegFlowNodeBase::m_pFirst=0;
CG2AutoRegFlowNodeBase *CG2AutoRegFlowNodeBase::m_pLast=0;

CGame *g_pGame = 0;
SCVars *g_pGameCVars = 0;

CGame::CGame()
: m_pFramework(0),
	m_pConsole(0),
	m_pWeaponSystem(0),
	m_pFlashMenuObject(0),
	m_pOptionsManager(0),
	m_pScriptBindActor(0),
	m_pScriptBindGame(0),
	m_pScriptBindMusicLogic(0),
	m_pGameActions(new SGameActions()),
	m_pPlayerProfileManager(0),
	m_pMusicLogic(0),
	m_pHUD(0),
	m_pServerSynchedStorage(0),
	m_pClientSynchedStorage(0),
	m_uiPlayerID(-1)

{
	m_pCVars = new SCVars();
	g_pGameCVars = m_pCVars;
	g_pGame = this;
	m_bReload = false;
	m_inDevMode = false;
	GetISystem()->SetIGame( this );
}

CGame::~CGame()
{
  m_pFramework->EndGameContext();
  m_pFramework->UnregisterListener(this);
  ReleaseScriptBinds();
	ReleaseActionMaps();
	SAFE_DELETE(m_pFlashMenuObject);
	SAFE_DELETE(m_pMusicLogic);
	SAFE_DELETE(m_pOptionsManager);
	SAFE_DELETE(m_pHUD);
	m_pWeaponSystem->Release();
	SAFE_DELETE(m_pItemStrings);
	SAFE_DELETE(m_pItemSharedParamsList);
	SAFE_DELETE(m_pCVars);
	g_pGame = 0;
	g_pGameCVars = 0;
}

bool CGame::Init(IGameFramework *pFramework)
{
#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::Init start");
#endif

	m_pFramework = pFramework;
	assert(m_pFramework);

	m_pConsole = gEnv->pConsole;

	RegisterConsoleVars();
	RegisterConsoleCommands();
	RegisterGameObjectEvents();

	// Initialize static item strings
	m_pItemStrings = new SItemStrings();

	m_pItemSharedParamsList = new CItemSharedParamsList();

	LoadActionMaps();

	InitScriptBinds();
	InitGameTokens();

  // Register all the games factory classes e.g. maps "Player" to CPlayer
  InitGameFactory(m_pFramework);

	//FIXME: horrible, remove this ASAP
	gEnv->pPhysicalWorld->AddEventClient( EventPhysImpulse::id,OnImpulse,0 );  

	m_pWeaponSystem = new CWeaponSystem(this, GetISystem());

	string itemFolder = "scripts/entities/items/xml";
	pFramework->GetIItemSystem()->Scan(itemFolder.c_str());
	m_pWeaponSystem->Scan(itemFolder.c_str());

	m_pFlashMenuObject = new CFlashMenuObject;
	m_pFlashMenuObject->Load();

	if (!gEnv->pSystem->IsEditor())
		gEnv->pConsole->ShowConsole(true);

	m_pOptionsManager = COptionsManager::CreateOptionsManager();

  // submit water material to physics
  //IMaterialManager* pMatMan = gEnv->p3DEngine->GetMaterialManager();    
  //gEnv->pPhysicalWorld->SetWaterMat( pMatMan->GetSurfaceTypeByName("mat_water")->GetId() );
  
	gEnv->pConsole->CreateKeyBind("f12", "r_getscreenshot 2");

	//Ivo: initialites the Crysis conversion file.
	//this is a conversion solution for the Crysis game DLL. Other projects don't need it.
	gEnv->pCharacterManager->LoadCharacterConversionFile("Objects/CrysisCharacterConversion.ccc");

	// set game GUID
	const char* gameGUID = g_pGameCVars->p_pp_GUID->GetString();
	if (gameGUID && gameGUID[0])
		m_pFramework->SetGameGUID(gameGUID);

	// TEMP
	// Load the action map beforehand (see above)
	// afterwards load the user's profile whose action maps get merged with default's action map
	m_pPlayerProfileManager = m_pFramework->GetIPlayerProfileManager();

	if (m_pPlayerProfileManager)
	{
		const char* userName = gEnv->pSystem->GetUserName();

		bool bIsFirstTime = false;
		bool ok = m_pPlayerProfileManager->LoginUser(userName, bIsFirstTime);
		if (ok)
		{
			if (bIsFirstTime)
			{
				// run autodetectspec
				gEnv->pSystem->AutoDetectSpec();
			}

			// activate the always present profile "default"
			int profileCount = m_pPlayerProfileManager->GetProfileCount(userName);
			if (profileCount > 0)
			{
				IPlayerProfileManager::SProfileDescription desc;
				ok = m_pPlayerProfileManager->GetProfileInfo(userName, 0, desc);
				if (ok)
				{
					IPlayerProfile* pProfile = m_pPlayerProfileManager->ActivateProfile(userName, desc.name);
          
					if (pProfile == 0)
					{
						GameWarning("[GameProfiles]: Cannot activate profile '%s' for user '%s'", desc.name, userName);
					}
          else
          {
            m_pFramework->GetILevelSystem()->LoadRotation();
          }
				}
				else
				{
					GameWarning("[GameProfiles]: Cannot get profile info for user '%s'", userName);
				}
			}
			else
			{
				GameWarning("[GameProfiles]: User 'dude' has no profiles");
			}
		}
		else
			GameWarning("[GameProfiles]: Cannot login user '%s'", userName);
	}

	m_pOptionsManager->SetProfileManager(m_pPlayerProfileManager);
	SAFE_MENU_FUNC(SetProfile());


	if (!m_pServerSynchedStorage)
		m_pServerSynchedStorage = new CServerSynchedStorage();

	if (!m_pMusicLogic)
	{
		m_pMusicLogic = new CMusicLogic();
		if (!m_pScriptBindMusicLogic)
			m_pScriptBindMusicLogic = new CScriptBind_MusicLogic(m_pMusicLogic);
	}

  m_pFramework->RegisterListener(this,"Game", FRAMEWORKLISTENERPRIORITY_GAME);

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::Init end");
#endif

	return true;
}

bool CGame::CompleteInit()
{
	// Initialize Game02 flow nodes

	if (IFlowSystem *pFlow = m_pFramework->GetIFlowSystem())
	{
		CG2AutoRegFlowNodeBase *pFactory = CG2AutoRegFlowNodeBase::m_pFirst;

		while (pFactory)
		{
			pFlow->RegisterType( pFactory->m_sClassName,pFactory );
			pFactory = pFactory->m_pNext;
		}
	}

	// needs to be initialized after Music graph is created	
	m_pMusicLogic->Init();

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::CompleteInit");
#endif
	return true;
}

int CGame::Update(bool haveFocus, unsigned int updateFlags)
{
	bool bRun = m_pFramework->PreUpdate( true, updateFlags );

	if (m_pFramework->IsGamePaused() == false)
	{
		m_pWeaponSystem->Update(gEnv->pTimer->GetFrameTime());

		m_pMusicLogic->Update();	// this does not need to be called on dedicated server
	}

	m_pFramework->PostUpdate( true, updateFlags );

	if(m_inDevMode != gEnv->pSystem->IsDevMode())
	{
		m_inDevMode = gEnv->pSystem->IsDevMode();
		m_pFramework->GetIActionMapManager()->EnableActionMap("debug", m_inDevMode);
	}

	CheckReloadLevel();

	return bRun ? 1 : 0;
}

void CGame::ConfigureGameChannel(bool isServer, IProtocolBuilder *pBuilder)
{
	if (isServer)
		m_pServerSynchedStorage->DefineProtocol(pBuilder);
	else
	{
		m_pClientSynchedStorage = new CClientSynchedStorage();
		m_pClientSynchedStorage->DefineProtocol(pBuilder);
	}
}

void CGame::EditorResetGame(bool bStart)
{
	CRY_ASSERT(gEnv->pSystem->IsEditor());

	if(bStart)
	{
		m_pHUD = new CHUD;
		m_pHUD->Init();
		m_pHUD->PlayerIdSet(m_uiPlayerID);	
	}
	else
	{
		SAFE_DELETE(m_pHUD);
	}
}

void CGame::PlayerIdSet(EntityId playerId)
{
	if(!gEnv->pSystem->IsEditor() && playerId != 0 && !gEnv->pSystem->IsDedicated())
	{
		// marcok: magic place to create the HUD and be able to release the flashmenu
		// problem was that the menu got destroyed while we were processing a callback
		// of the menu
    SAFE_MENU_FUNC(DestroyIngameMenu());	//else the memory pool gets too big
		SAFE_MENU_FUNC(DestroyStartMenu());	//else the memory pool gets too big
		if (m_pHUD == 0)
    {
			m_pHUD = new CHUD();
		  m_pHUD->Init();
    }
	}

	if(m_pHUD)
	{
		m_pHUD->PlayerIdSet(playerId);	
	}
	else
	{
		m_uiPlayerID = playerId;
	}
}

void CGame::Shutdown()
{
	if (m_pPlayerProfileManager)
	{
		m_pPlayerProfileManager->LogoutUser("dude");
	}

	delete m_pServerSynchedStorage;
	m_pServerSynchedStorage	= 0;

	this->~CGame();
}

const char *CGame::GetLongName()
{
	return GAME_LONGNAME;
}

const char *CGame::GetName()
{
	return GAME_NAME;
}

void CGame::OnPostUpdate(float fDeltaTime)
{

}

void CGame::OnSaveGame(ISaveGame* pSaveGame)
{
	CPlayer *pPlayer = static_cast<CPlayer*>(GetIGameFramework()->GetClientActor());
	GetGameRules()->PlayerPosForRespawn(pPlayer, true);
}

void CGame::OnLoadGame(ILoadGame* pLoadGame)
{

}

void CGame::OnActionEvent(const SActionEvent& event)
{
  SAFE_MENU_FUNC(OnActionEvent(event));

  switch(event.m_event)
  {
  case  eAE_channelDestroyed:
    GameChannelDestroyed(event.m_value == 1);
    break;
  }
}

void CGame::GameChannelDestroyed(bool isServer)
{
  if (!isServer)
  {
    delete m_pClientSynchedStorage;
    m_pClientSynchedStorage=0;
    if(m_pHUD)
      m_pHUD->PlayerIdSet(0);

    //the hud continues existing when the player got diconnected - it's part of the game
    /*if(!gEnv->pSystem->IsEditor())
    {
    SAFE_DELETE(m_pHUD);
    }*/
  }
}

void CGame::DestroyHUD()
{
  SAFE_DELETE(m_pHUD);
}

void CGame::BlockingProcess(BlockingConditionFunction f)
{
  INetwork* pNetwork = gEnv->pNetwork;

  bool ok = false;

  ITimer * pTimer = gEnv->pTimer;
  CTimeValue startTime = pTimer->GetAsyncTime();

  while (!ok)
  {
    pNetwork->SyncWithGame(eNGS_FrameStart);
    pNetwork->SyncWithGame(eNGS_FrameEnd);
    gEnv->pTimer->UpdateOnFrameStart();
    ok |= (*f)();
  }
}

CGameRules *CGame::GetGameRules() const
{
	return static_cast<CGameRules *>(m_pFramework->GetIGameRulesSystem()->GetCurrentGameRules());
}

CHUD *CGame::GetHUD() const
{
	return m_pHUD;
}

CFlashMenuObject *CGame::GetMenu() const
{
	return m_pFlashMenuObject;
}

COptionsManager *CGame::GetOptions() const
{
	return m_pOptionsManager;
}

void CGame::LoadActionMaps(const char* filename)
{
	if(g_pGame->GetIGameFramework()->IsGameStarted())
	{
		CryLogAlways("Can't change configuration while game is running (yet)");
		return;
	}

	IActionMapManager *pActionMapMan = m_pFramework->GetIActionMapManager();

	// make sure that they are also added to the GameActions.actions file!
	XmlNodeRef rootNode = m_pFramework->GetISystem()->LoadXmlFile(filename);
	if(rootNode)
	{
		pActionMapMan->Clear();
		pActionMapMan->LoadFromXML(rootNode);
		m_pDefaultAM = pActionMapMan->GetActionMap("default");
		m_pDebugAM = pActionMapMan->GetActionMap("debug");
		m_pMultiplayerAM = pActionMapMan->GetActionMap("multiplayer");
		m_pNoMoveAF = pActionMapMan->GetActionFilter("no_move");
		m_pNoMouseAF = pActionMapMan->GetActionFilter("no_mouse");
		m_pInVehicleSuitMenu = pActionMapMan->GetActionFilter("in_vehicle_suit_menu");

		// enable defaults
		pActionMapMan->EnableActionMap("default",true);

		// enable debug
		pActionMapMan->EnableActionMap("debug",true);

		// enable player action map
		pActionMapMan->EnableActionMap("player",true);

		m_pFreezeTimeAF=pActionMapMan->CreateActionFilter("freezetime", eAFT_ActionPass);
		m_pFreezeTimeAF->Filter(m_pGameActions->reload);
		m_pFreezeTimeAF->Filter(m_pGameActions->rotateyaw);
		m_pFreezeTimeAF->Filter(m_pGameActions->rotatepitch);
		m_pFreezeTimeAF->Filter(m_pGameActions->drop);
		m_pFreezeTimeAF->Filter(m_pGameActions->modify);
		m_pFreezeTimeAF->Filter(m_pGameActions->jump);
		m_pFreezeTimeAF->Filter(m_pGameActions->crouch);
		m_pFreezeTimeAF->Filter(m_pGameActions->prone);
		m_pFreezeTimeAF->Filter(m_pGameActions->togglestance);
		m_pFreezeTimeAF->Filter(m_pGameActions->leanleft);
		m_pFreezeTimeAF->Filter(m_pGameActions->leanright);

		m_pFreezeTimeAF->Filter(m_pGameActions->rotateyaw);
		m_pFreezeTimeAF->Filter(m_pGameActions->rotatepitch);

		m_pFreezeTimeAF->Filter(m_pGameActions->reload);
		m_pFreezeTimeAF->Filter(m_pGameActions->drop);
		m_pFreezeTimeAF->Filter(m_pGameActions->modify);
		m_pFreezeTimeAF->Filter(m_pGameActions->nextitem);
		m_pFreezeTimeAF->Filter(m_pGameActions->previtem);
		m_pFreezeTimeAF->Filter(m_pGameActions->small);
		m_pFreezeTimeAF->Filter(m_pGameActions->medium);
		m_pFreezeTimeAF->Filter(m_pGameActions->heavy);
		m_pFreezeTimeAF->Filter(m_pGameActions->explosive);
		m_pFreezeTimeAF->Filter(m_pGameActions->handgrenade);
		m_pFreezeTimeAF->Filter(m_pGameActions->holsteritem);

		m_pFreezeTimeAF->Filter(m_pGameActions->utility);
		m_pFreezeTimeAF->Filter(m_pGameActions->debug);
		m_pFreezeTimeAF->Filter(m_pGameActions->firemode);
		m_pFreezeTimeAF->Filter(m_pGameActions->objectives);

		m_pFreezeTimeAF->Filter(m_pGameActions->speedmode);
		m_pFreezeTimeAF->Filter(m_pGameActions->strengthmode);
		m_pFreezeTimeAF->Filter(m_pGameActions->defensemode);

		m_pFreezeTimeAF->Filter(m_pGameActions->switchhud);

		m_pFreezeTimeAF->Filter(m_pGameActions->invert_mouse);

		m_pFreezeTimeAF->Filter(m_pGameActions->gboots);
		m_pFreezeTimeAF->Filter(m_pGameActions->lights);

		m_pFreezeTimeAF->Filter(m_pGameActions->radio_group_0);
		m_pFreezeTimeAF->Filter(m_pGameActions->radio_group_1);
		m_pFreezeTimeAF->Filter(m_pGameActions->radio_group_2);
		m_pFreezeTimeAF->Filter(m_pGameActions->radio_group_3);
		m_pFreezeTimeAF->Filter(m_pGameActions->radio_group_4);

		m_pFreezeTimeAF->Filter(m_pGameActions->transmission_1);
		m_pFreezeTimeAF->Filter(m_pGameActions->transmission_2);
		m_pFreezeTimeAF->Filter(m_pGameActions->transmission_3);

		m_pFreezeTimeAF->Filter(m_pGameActions->voice_chat_talk);

			// XInput specific actions
		m_pFreezeTimeAF->Filter(m_pGameActions->xi_binoculars);
		m_pFreezeTimeAF->Filter(m_pGameActions->xi_rotateyaw);
		m_pFreezeTimeAF->Filter(m_pGameActions->xi_rotatepitch);
		m_pFreezeTimeAF->Filter(m_pGameActions->xi_v_rotateyaw);
		m_pFreezeTimeAF->Filter(m_pGameActions->xi_v_rotatepitch);

			// HUD
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_nanosuit_nextitem);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_nanosuit_minus);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_nanosuit_plus);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mousex);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mousey);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mouseclick);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_suit_menu);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_openchat);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_openteamchat);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mousewheelup);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mousewheeldown);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mouserightbtndown);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_mouserightbtnup);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_show_multiplayer_scoreboard);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_hide_multiplayer_scoreboard);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_toggle_scoreboard_cursor);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_pda_switch);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_show_pda);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_hide_pda);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_show_pda_map);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_hide_pda_map);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_buy_weapons);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_pda_scroll);
		m_pFreezeTimeAF->Filter(m_pGameActions->scores);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_menu);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_night_vision);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_weapon_mod);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_suit_mod);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_select1);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_select2);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_select3);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_select4);
		m_pFreezeTimeAF->Filter(m_pGameActions->hud_select5);

		m_pFreezeTimeAF->Filter("buyammo");
	}
	else
		CryLogAlways("Could not open configuration file");
}

void CGame::ReleaseActionMaps()
{
	SAFE_RELEASE(m_pDefaultAM);
	SAFE_RELEASE(m_pMultiplayerAM);
	SAFE_RELEASE(m_pNoMoveAF);
	SAFE_RELEASE(m_pNoMouseAF);
	SAFE_RELEASE(m_pFreezeTimeAF);
	SAFE_RELEASE(m_pInVehicleSuitMenu);
	SAFE_RELEASE(m_pDebugAM);
	SAFE_DELETE(m_pGameActions);
}

void CGame::InitScriptBinds()
{
	m_pScriptBindActor = new CScriptBind_Actor(m_pFramework->GetISystem());
	m_pScriptBindItem = new CScriptBind_Item(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindWeapon = new CScriptBind_Weapon(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindHUD = new CScriptBind_HUD(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindGameRules = new CScriptBind_GameRules(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindGame = new CScriptBind_Game(m_pFramework->GetISystem(), m_pFramework);
}

void CGame::ReleaseScriptBinds()
{
	SAFE_DELETE(m_pScriptBindActor);
	SAFE_DELETE(m_pScriptBindItem);
	SAFE_DELETE(m_pScriptBindWeapon);
	SAFE_DELETE(m_pScriptBindHUD);
	SAFE_DELETE(m_pScriptBindGameRules);
	SAFE_DELETE(m_pScriptBindGame);
	SAFE_DELETE(m_pScriptBindMusicLogic);
}

void CGame::CheckReloadLevel()
{
	if(!m_bReload)
		return;

	if(GetISystem()->IsEditor() || gEnv->bMultiplayer)
	{
		if(m_bReload)
			m_bReload = false;
		return;
	}

	// Restart interrupts cutscenes
	gEnv->pMovieSystem->StopAllCutScenes();

	m_bReload = false;	//if m_bReload is true - load at levelstart
	GetISystem()->SerializingFile(1);

	//load levelstart
	ILevelSystem* pLevelSystem = m_pFramework->GetILevelSystem();
	ILevel*			pLevel = pLevelSystem->GetCurrentLevel();
	ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(m_pFramework->GetLevelName());
	//**********
	pLevelSystem->OnLoadingStart(pLevelInfo);
	string levelstart("load ");
	levelstart.append(GetIGameFramework()->GetLevelName());
	levelstart.append("_levelstart.CRYSISJMSF");
	gEnv->pConsole->ExecuteString(levelstart.c_str());
	//**********
	pLevelSystem->OnLoadingComplete(pLevel);

	//if paused - start game
	m_pFramework->PauseGame(false, true);

	GetISystem()->SerializingFile(0);
}

void CGame::InitGameTokens()
{
	// save pointer to GameTokenSystem
	m_pGameTokenSystem = m_pFramework->GetIGameTokenSystem();

	// note: when you create tokens via code, these will have to be recreated on quickload (serialization)
#if 0
	// TFlowInputData (value, locked)
	// value must be of correct type (bool, int, float, string) and locked==true means it will always
	// holds values of the assigned type (it's a LOCKED variant)
	IGameToken* token = 0;
	// 'HUD' is the library name, 'Nanosuit' is the group, 'Strength' is the token
	token = m_pGameTokenSystem->SetOrCreateToken("hud.nanosuit.strength", TFlowInputData((float)100.0f, true));

	// string val;
	// bool ok =m_pGameTokenSystem->GetTokenValueAs("hud.nanosuit.strength", val);

	// m_pGameTokenSystem->SetOrCreateToken("hud.weapon.crosshair_hit", TFlowInputData((bool)false, true));
#endif
}

void CGame::RegisterGameObjectEvents()
{
	IGameObjectSystem* pGOS = m_pFramework->GetIGameObjectSystem();

	pGOS->RegisterEvent(eCGE_PostFreeze, "PostFreeze");
	pGOS->RegisterEvent(eCGE_PostShatter,"PostShatter");
	pGOS->RegisterEvent(eCGE_OnShoot,"OnShoot");
	pGOS->RegisterEvent(eCGE_Recoil,"Recoil");
	pGOS->RegisterEvent(eCGE_BeginReloadLoop,"BeginReloadLoop");
	pGOS->RegisterEvent(eCGE_EndReloadLoop,"EndReloadLoop");
	pGOS->RegisterEvent(eCGE_ActorRevive,"ActorRevive");
	pGOS->RegisterEvent(eCGE_VehicleDestroyed,"VehicleDestroyed");
	pGOS->RegisterEvent(eCGE_TurnRagdoll,"TurnRagdoll");
	pGOS->RegisterEvent(eCGE_EnableFallAndPlay,"EnableFallAndPlay");
	pGOS->RegisterEvent(eCGE_DisableFallAndPlay,"DisableFallAndPlay");
	pGOS->RegisterEvent(eCGE_VehicleTransitionEnter,"VehicleTransitionEnter");
	pGOS->RegisterEvent(eCGE_VehicleTransitionExit,"VehicleTransitionExit");
	pGOS->RegisterEvent(eCGE_HUD_PDAMessage,"HUD_PDAMessage");
	pGOS->RegisterEvent(eCGE_HUD_TextMessage,"HUD_TextMessage");
	pGOS->RegisterEvent(eCGE_TextArea,"TextArea");
	pGOS->RegisterEvent(eCGE_HUD_Break,"HUD_Break");
	pGOS->RegisterEvent(eCGE_HUD_Reboot,"HUD_Reboot");
	pGOS->RegisterEvent(eCGE_InitiateAutoDestruction,"InitiateAutoDestruction");
	pGOS->RegisterEvent(eCGE_Event_Collapsing,"Event_Collapsing");
	pGOS->RegisterEvent(eCGE_Event_Collapsed,"Event_Collapsed");
	pGOS->RegisterEvent(eCGE_MultiplayerChatMessage,"MultiplayerChatMessage");
	pGOS->RegisterEvent(eCGE_ResetMovementController,"ResetMovementController");
	pGOS->RegisterEvent(eCGE_AnimateHands,"AnimateHands");
	pGOS->RegisterEvent(eCGE_Ragdoll,"Ragdoll");
	pGOS->RegisterEvent(eCGE_EnablePhysicalCollider,"EnablePhysicalCollider");
	pGOS->RegisterEvent(eCGE_DisablePhysicalCollider,"DisablePhysicalCollider");
	pGOS->RegisterEvent(eCGE_RebindAnimGraphInputs,"RebindAnimGraphInputs");
	pGOS->RegisterEvent(eCGE_OpenParachute, "OpenParachute");

}

void CGame::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	m_pWeaponSystem->GetMemoryStatistics(s);

	s->Add(*m_pScriptBindActor);
	s->Add(*m_pScriptBindItem);
	s->Add(*m_pScriptBindWeapon);
	s->Add(*m_pScriptBindGameRules);
	s->Add(*m_pScriptBindGame);
	s->Add(*m_pScriptBindHUD);
	s->Add(*m_pScriptBindMusicLogic);

	SAFE_MENU_FUNC(GetMemoryStatistics(s));

	/* handled by actionmapmanager
	m_pDefaultAM->GetMemoryStatistics(s);
	m_pDefaultAM->GetMemoryStatistics(s);
	m_pDebugAM->GetMemoryStatistics(s);
	m_pMultiplayerAM->GetMemoryStatistics(s);
	m_pNoMoveAF->GetMemoryStatistics(s);
	m_pNoMouseAF->GetMemoryStatistics(s);
	m_pInVehicleSuitMenu->GetMemoryStatistics(s);
	*/

	s->Add(*m_pGameActions);

	m_pItemSharedParamsList->GetMemoryStatistics(s);
	m_pGameTokenSystem->GetMemoryStatistics(s);
  if (m_pPlayerProfileManager)
	  m_pPlayerProfileManager->GetMemoryStatistics(s);
	if (m_pMusicLogic)
		m_pMusicLogic->GetMemoryStatistics(s);
	if (m_pHUD)
		m_pHUD->GetMemoryStatistics(s);
	if (m_pServerSynchedStorage)
		m_pServerSynchedStorage->GetMemoryStatistics(s);
	if (m_pClientSynchedStorage)
		m_pClientSynchedStorage->GetMemoryStatistics(s);
}

void CGame::OnClearPlayerIds()
{
	if(IActor *pClient = GetIGameFramework()->GetClientActor())
	{
		CPlayer *pPlayer = static_cast<CPlayer*>(pClient);
		if(pPlayer->GetNanoSuit())
			pPlayer->GetNanoSuit()->RemoveListener(m_pHUD);
	}
}

void CGame::DumpMemInfo(const char* format, ...)
{
	CryModuleMemoryInfo memInfo;
	CryGetMemoryInfoForModule(&memInfo);

	va_list args;
	va_start(args,format);
	gEnv->pSystem->GetILog()->LogV( ILog::eAlways,format,args );
	va_end(args);

	gEnv->pSystem->GetILog()->LogWithType( ILog::eAlways, "Alloc=%I64d kb  String=%I64d kb  STL-alloc=%I64d kb  STL-wasted=%I64d kb", (memInfo.allocated - memInfo.freed) >> 10 , memInfo.CryString_allocated >> 10, memInfo.STL_allocated >> 10 , memInfo.STL_wasted >> 10);
	// gEnv->pSystem->GetILog()->LogV( ILog::eAlways, "%s alloc=%llu kb  instring=%llu kb  stl-alloc=%llu kb  stl-wasted=%llu kb", text, memInfo.allocated >> 10 , memInfo.CryString_allocated >> 10, memInfo.STL_allocated >> 10 , memInfo.STL_wasted >> 10);
}

const string& CGame::GetLastSaveGame()
{
	if (m_pPlayerProfileManager)
	{
		const char* userName = GetISystem()->GetUserName();
		IPlayerProfile* pProfile = m_pPlayerProfileManager->GetCurrentProfile(userName);
		if (pProfile)
		{
			ISaveGameEnumeratorPtr pSGE = pProfile->CreateSaveGameEnumerator();
			ISaveGameEnumerator::SGameDescription desc;	
			time_t curLatestTime = (time_t) 0;
			const char* lastSaveGame = "";
			const int nSaveGames = pSGE->GetCount();
			for (int i=0; i<nSaveGames; ++i)
			{
				if (pSGE->GetDescription(i, desc))
				{
					if (desc.metaData.saveTime > curLatestTime)
					{
						lastSaveGame = desc.name;
						curLatestTime = desc.metaData.saveTime;
					}
				}
			}
			m_lastSaveGame = lastSaveGame;
		}
	}
	return m_lastSaveGame;
}
