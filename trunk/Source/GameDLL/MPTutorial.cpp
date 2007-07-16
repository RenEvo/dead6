/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: MP Tutorial for PowerStruggle

-------------------------------------------------------------------------
History:
- 12:03:2007: Created by Steve Humphreys

*************************************************************************/

#include "StdAfx.h"

#include "MPTutorial.h"

#include "Energise.h"
#include "Game.h"
#include "GameActions.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "HUD/HUDPowerStruggle.h"
#include "Player.h"

const float MESSAGE_DISPLAY_TIME = 4.0f;		// default time for msg display (if no audio)
const float MESSAGE_GAP_TIME = 2.0f;				// gap between msgs
const float ENTITY_CHECK_TIME = 0.5f;

// macro to set event names to correspond to the enum value (minus the eTE_ prefix),
//	turn on all event checks
#define SET_TUTORIAL_EVENT_BLANK(eventName, soundName){ \
	m_events[eTE_##eventName].m_name = #eventName; \
	m_events[eTE_##eventName].m_soundName = #soundName; \
	m_events[eTE_##eventName].m_action = ""; \
	m_events[eTE_##eventName].m_shouldCheck = true; \
	m_events[eTE_##eventName].m_triggered = false; \
	}

#define SET_TUTORIAL_EVENT_BLANK_KEY(eventName, soundName, keyName){ \
	m_events[eTE_##eventName].m_name = #eventName; \
	m_events[eTE_##eventName].m_soundName = #soundName; \
	m_events[eTE_##eventName].m_action = #keyName; \
	m_events[eTE_##eventName].m_shouldCheck = true; \
	m_events[eTE_##eventName].m_triggered = false; \
}

CMPTutorial::CMPTutorial()
: m_addedListeners(false)
, m_currentBriefingEvent(eTE_StartGame)
, m_entityCheckTimer(0.0f)
, m_baseCheckTimer(ENTITY_CHECK_TIME / 2.0f)
, m_wasInVehicle(false)
, m_msgDisplayTime(0.0f)
{
	// initialise everything even if disabled - we might be enabled later?
	int enabled = g_pGameCVars->g_PSTutorial_Enabled;
	m_enabled = (enabled != 0);

	// blank out all the events initially, and assign their names
	InitEvents();

	// get entity classes we'll need later
	InitEntityClasses();

	// add console command
	gEnv->pConsole->AddCommand("g_psTutorial_TriggerEvent", ForceTriggerEvent, VF_CHEAT|VF_NOT_NET_SYNCED, "Trigger an MP tutorial event by name");
	gEnv->pConsole->AddCommand("g_psTutorial_Reset", ResetAllEvents, VF_CHEAT|VF_NOT_NET_SYNCED, "Reset powerstruggle tutorial");
}

CMPTutorial::~CMPTutorial()
{
	if(m_addedListeners && g_pGame->GetHUD())
		g_pGame->GetHUD()->UnRegisterListener(this);
}

void CMPTutorial::InitEvents()
{
	SET_TUTORIAL_EVENT_BLANK(StartGame, mp_american_bridge_officer_1_brief01);
	SET_TUTORIAL_EVENT_BLANK(ContinueTutorial, mp_american_bridge_officer_1_brief02);
	SET_TUTORIAL_EVENT_BLANK_KEY(Barracks, mp_american_bridge_officer_1_brief03, hud_buy_weapons);
	SET_TUTORIAL_EVENT_BLANK_KEY(BarracksTwo, mp_american_bridge_officer_1_brief03_split, hud_mouseclick);
	SET_TUTORIAL_EVENT_BLANK_KEY(CloseBarracksBuyMenu, mp_american_bridge_officer_1_brief04, hud_show_pda_map);
	SET_TUTORIAL_EVENT_BLANK(OpenMap, mp_american_bridge_officer_1_brief05);
	SET_TUTORIAL_EVENT_BLANK(CloseMap, mp_american_bridge_officer_1_brief06);
	SET_TUTORIAL_EVENT_BLANK(Swingometer, mp_american_bridge_officer_1_brief07);

	SET_TUTORIAL_EVENT_BLANK(TutorialDisabled,None);

	SET_TUTORIAL_EVENT_BLANK(ScoreBoard, None);
	SET_TUTORIAL_EVENT_BLANK(Promotion, None);

	// capturing a factory
	SET_TUTORIAL_EVENT_BLANK(NeutralFactory, mp_american_bridge_officer_1_factory01);
	SET_TUTORIAL_EVENT_BLANK_KEY(CaptureFactory, mp_american_bridge_officer_1_factory01_split, hud_buy_weapons);
	SET_TUTORIAL_EVENT_BLANK_KEY(VehicleBuyMenu, mp_american_bridge_officer_1_factory02, hud_mouseclick);
	SET_TUTORIAL_EVENT_BLANK_KEY(WarBuyMenu, mp_american_bridge_officer_1_factory03, hud_mouseclick);
	SET_TUTORIAL_EVENT_BLANK_KEY(PrototypeBuyMenu, mp_american_bridge_officer_1_factory04, hud_mouseclick);
	SET_TUTORIAL_EVENT_BLANK_KEY(NavalBuyMenu, mp_american_bridge_officer_1_factory05, hud_mouseclick);
	SET_TUTORIAL_EVENT_BLANK_KEY(AirBuyMenu, mp_american_bridge_officer_1_factory06, hud_mouseclick);
	SET_TUTORIAL_EVENT_BLANK(CloseBuyMenu, mp_american_bridge_officer_1_factory07);
	SET_TUTORIAL_EVENT_BLANK_KEY(BuyAmmo, mp_american_bridge_officer_1_factory08, buyammo);
	SET_TUTORIAL_EVENT_BLANK(SpawnBunker,None);

	// prototype factory
	SET_TUTORIAL_EVENT_BLANK(EnterPrototypeFactory, mp_american_bridge_officer_1_prototype01);
	SET_TUTORIAL_EVENT_BLANK(FindAlienCrashSite, mp_american_bridge_officer_1_prototype02);
	SET_TUTORIAL_EVENT_BLANK(FillCollector, mp_american_bridge_officer_1_prototype03);
	SET_TUTORIAL_EVENT_BLANK(FillReactor, mp_american_bridge_officer_1_prototype04);
	SET_TUTORIAL_EVENT_BLANK(ReactorFull, mp_american_bridge_officer_1_prototype05);

	// in game action
	SET_TUTORIAL_EVENT_BLANK(EnemySpotted, mp_american_bridge_officer_1_ingame01);
	SET_TUTORIAL_EVENT_BLANK(BoardVehicle, mp_american_bridge_officer_1_ingame02);
	SET_TUTORIAL_EVENT_BLANK(EnterHostileFactory, mp_american_bridge_officer_1_ingame03);
	SET_TUTORIAL_EVENT_BLANK(Wounded, mp_american_bridge_officer_1_ingame04);
	SET_TUTORIAL_EVENT_BLANK(Killed, mp_american_bridge_officer_1_ingame05);
	SET_TUTORIAL_EVENT_BLANK(TACTankStarted, mp_american_bridge_officer_1_ingame06);
	SET_TUTORIAL_EVENT_BLANK(TACTankCompleted, mp_american_bridge_officer_1_ingame07);
	SET_TUTORIAL_EVENT_BLANK(TACTankBase, mp_american_bridge_officer_1_ingame08);
	SET_TUTORIAL_EVENT_BLANK(TACLauncherCompleted, None);															// TODO: audio needed
	SET_TUTORIAL_EVENT_BLANK(SingularityStarted, mp_american_bridge_officer_1_ingame09);
	SET_TUTORIAL_EVENT_BLANK(SingularityCompleted, mp_american_bridge_officer_1_ingame10);
	SET_TUTORIAL_EVENT_BLANK(SingularityBase, mp_american_bridge_officer_1_ingame11);
	SET_TUTORIAL_EVENT_BLANK(EnemyNearBase, mp_american_bridge_officer_1_ingame12);
	SET_TUTORIAL_EVENT_BLANK(TurretUnderAttack, mp_american_bridge_officer_1_ingame14);	// nb no 13
	SET_TUTORIAL_EVENT_BLANK(TurretDestroyed, mp_american_bridge_officer_1_ingame15);
	SET_TUTORIAL_EVENT_BLANK(HqUnderAttack, mp_american_bridge_officer_1_ingame16);
	SET_TUTORIAL_EVENT_BLANK(HqCritical, mp_american_bridge_officer_1_ingame17);
	SET_TUTORIAL_EVENT_BLANK(ApproachEnemyBase, mp_american_bridge_officer_1_ingame18);
	SET_TUTORIAL_EVENT_BLANK(ApproachEnemyHq, mp_american_bridge_officer_1_ingame19);
	SET_TUTORIAL_EVENT_BLANK(ApproachEnemySub, None);																							// TODO: new audio
	SET_TUTORIAL_EVENT_BLANK(ApproachEnemyCarrier, None);																					// TODO: new audio
	SET_TUTORIAL_EVENT_BLANK(DestroyEnemyHq, mp_american_bridge_officer_1_ingame20);
	SET_TUTORIAL_EVENT_BLANK(GameOverWin, mp_american_bridge_officer_1_win01);
	SET_TUTORIAL_EVENT_BLANK(GameOverLose, mp_american_bridge_officer_1_lose01);

	// items / weapons
	SET_TUTORIAL_EVENT_BLANK(pistol,None);
	SET_TUTORIAL_EVENT_BLANK(smg,None);
	SET_TUTORIAL_EVENT_BLANK(shotgun,None);
	SET_TUTORIAL_EVENT_BLANK(fy71,None);
	SET_TUTORIAL_EVENT_BLANK(macs,None);
	SET_TUTORIAL_EVENT_BLANK(dsg1,None);
	SET_TUTORIAL_EVENT_BLANK(rpg,None);
	SET_TUTORIAL_EVENT_BLANK(gauss,None);
	SET_TUTORIAL_EVENT_BLANK(hurricane,None);
	SET_TUTORIAL_EVENT_BLANK(moar,None);
	SET_TUTORIAL_EVENT_BLANK(moac,None);
	SET_TUTORIAL_EVENT_BLANK(fraggren,None);
	SET_TUTORIAL_EVENT_BLANK(smokegren,None);
	SET_TUTORIAL_EVENT_BLANK(flashbang,None);
	SET_TUTORIAL_EVENT_BLANK(c4,None);
	SET_TUTORIAL_EVENT_BLANK(avmine,None);
	SET_TUTORIAL_EVENT_BLANK(claymore,None);
	SET_TUTORIAL_EVENT_BLANK(radarkit,None);
	SET_TUTORIAL_EVENT_BLANK(binocs,None);
	SET_TUTORIAL_EVENT_BLANK(lockkit,None);
	SET_TUTORIAL_EVENT_BLANK(techkit,None);
	SET_TUTORIAL_EVENT_BLANK(pchute,None);
	SET_TUTORIAL_EVENT_BLANK(psilent,None);
	SET_TUTORIAL_EVENT_BLANK(silent,None);
	SET_TUTORIAL_EVENT_BLANK(gl,None);
	SET_TUTORIAL_EVENT_BLANK(HTScope_TODO,None);
	SET_TUTORIAL_EVENT_BLANK(scope,None);
	SET_TUTORIAL_EVENT_BLANK(ascope,None);
	SET_TUTORIAL_EVENT_BLANK(reflex,None);
	SET_TUTORIAL_EVENT_BLANK(plam,None);
	SET_TUTORIAL_EVENT_BLANK(lam,None);
	SET_TUTORIAL_EVENT_BLANK(tactical,None);
	SET_TUTORIAL_EVENT_BLANK(acloak,None);

	// vehicles 
	SET_TUTORIAL_EVENT_BLANK(SUV,None);
	SET_TUTORIAL_EVENT_BLANK(nk4wd,None);
	SET_TUTORIAL_EVENT_BLANK(us4wd,None);
	SET_TUTORIAL_EVENT_BLANK(usapc,None);
	SET_TUTORIAL_EVENT_BLANK(AAPC_TODO,None);
	SET_TUTORIAL_EVENT_BLANK(nkaaa,None);
	SET_TUTORIAL_EVENT_BLANK(ustank,None);
	SET_TUTORIAL_EVENT_BLANK(nktank,None);
	SET_TUTORIAL_EVENT_BLANK(nkhelicopter,None);
	SET_TUTORIAL_EVENT_BLANK(usvtol,None);
	SET_TUTORIAL_EVENT_BLANK(cboat,None);
	SET_TUTORIAL_EVENT_BLANK(usboat,None);
	SET_TUTORIAL_EVENT_BLANK(nkboat,None);
	SET_TUTORIAL_EVENT_BLANK(ushovercraft,None);
	SET_TUTORIAL_EVENT_BLANK(usspawntruck,None);
	SET_TUTORIAL_EVENT_BLANK(usammotruck,None);
	SET_TUTORIAL_EVENT_BLANK(usgauss4wd,None);
	SET_TUTORIAL_EVENT_BLANK(usmoac4wd,None);
	SET_TUTORIAL_EVENT_BLANK(usmoar4wd,None);
	SET_TUTORIAL_EVENT_BLANK(MOACBOAT_TODO,None);
	SET_TUTORIAL_EVENT_BLANK(NMOARBOAT_TODO,None);
	SET_TUTORIAL_EVENT_BLANK(SINGBOAT_TODO,None);
	SET_TUTORIAL_EVENT_BLANK(usgausstank,None);
	SET_TUTORIAL_EVENT_BLANK(ussingtank,None);
	SET_TUTORIAL_EVENT_BLANK(ustactank,None);
	SET_TUTORIAL_EVENT_BLANK(SINGVTOL_TODO,None);
}

void CMPTutorial::InitEntityClasses()
{
	IEntityClassRegistry* classReg = gEnv->pEntitySystem->GetClassRegistry();
	m_pHQClass = classReg->FindClass("HQ");
	m_pFactoryClass = classReg->FindClass("Factory");
	m_pAlienEnergyPointClass = classReg->FindClass("AlienEnergyPoint");
	m_pPlayerClass = classReg->FindClass("Player");
	m_pTankClass = classReg->FindClass("US_tank");
	m_pTechChargerClass = classReg->FindClass("TechCharger");
	m_pSpawnGroupClass = classReg->FindClass("SpawnGroup");
	m_pSUVClass = classReg->FindClass("Civ_car1");
}

void CMPTutorial::OnBuyMenuOpen(bool open, FlashRadarType buyZoneType)
{
	if(!m_enabled)
		return;

	if(open)
	{
		// depends on type of factory
		bool showPrompt = false;
		switch(buyZoneType)
		{
			case ELTV:				// this seems to be the default value, and occurs when players are at spawn points.
			case EHeadquarter:
			case EHeadquarter2:
			case EBarracks:
				TriggerEvent(eTE_BarracksTwo);
				break;

			case EFactoryVehicle:
				showPrompt = TriggerEvent(eTE_VehicleBuyMenu);
				break;

			case EFactoryAir:
				showPrompt = TriggerEvent(eTE_AirBuyMenu);
				break;

			case EFactoryPrototype:
				showPrompt = TriggerEvent(eTE_PrototypeBuyMenu);
				break;

			case EFactorySea:
				showPrompt = TriggerEvent(eTE_NavalBuyMenu);
				break;

			case EFactoryTank:
				showPrompt = TriggerEvent(eTE_WarBuyMenu);
				break;

			default:
				// other types don't show help
				break;
		}

		/*if(!showPrompt)
		{
			if(ammoPage)
				TriggerEvent(eTE_BuyAmmo);
		}
		*/
	}
	else
	{
		if(buyZoneType == EBarracks
			|| buyZoneType == EHeadquarter
			|| buyZoneType == EHeadquarter2
			|| buyZoneType == ELTV)
			TriggerEvent(eTE_CloseBarracksBuyMenu);
		else
			TriggerEvent(eTE_CloseBuyMenu);
	}
}

void CMPTutorial::OnMapOpen(bool open)
{
	if(!m_enabled)
		return;

	if(open)
	{
		TriggerEvent(eTE_OpenMap);
	}
	else
	{
		TriggerEvent(eTE_CloseMap);
	}
}

void CMPTutorial::OnEntityAddedToRadar(EntityId id)
{
	// if the entity is an enemy player, show the prompt.
	if(!m_events[eTE_EnemySpotted].m_shouldCheck)
		return;

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id);
	if(pEntity && pEntity->GetClass() == m_pPlayerClass)
	{
		if(g_pGame->GetGameRules()->GetTeam(id) != g_pGame->GetGameRules()->GetTeam(g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId()))
		{
			TriggerEvent(eTE_EnemySpotted);
		}
	}
}

void CMPTutorial::OnBuyMenuItemHover(const char* itemname)
{
//  	for(int i=eTE_FIRST_ITEM; i <= eTE_LAST_ITEM; ++i)
//  	{
//  		if(m_events[i].m_shouldCheck && !strcmp(itemname, m_events[i].m_name))
//  		{
// 			if(CHUD* pHud = g_pGame->GetHUD())
// 			{
// 				if(CHUDPowerStruggle* pHudPS = pHud->GetPowerStruggleHUD())
// 				{
// 					string text = "@mp_Tut";
// 					text += m_events[i].m_name;
// 					pHudPS->SetPDATooltip(text.c_str());
// 					return;
// 				}
// 			}
//  		}
//  	}
}

void CMPTutorial::OnShowBuyMenuAmmoPage()
{
	TriggerEvent(eTE_BuyAmmo);
}

void CMPTutorial::OnShowScoreBoard()
{
	TriggerEvent(eTE_ScoreBoard);
}

void CMPTutorial::OnSoundEvent( ESoundCallbackEvent event,ISound *pSound )
{
	if(event == SOUND_EVENT_ON_PLAYBACK_STOPPED)
	{
		// remove current message
		m_msgDisplayTime = 0.0f;
		if(pSound)
			pSound->RemoveEventListener(this);
	}
}

void CMPTutorial::Update()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if(!m_enabled && g_pGameCVars->g_PSTutorial_Enabled)
		EnableTutorialMode(true);
	else if(m_enabled && !g_pGameCVars->g_PSTutorial_Enabled)
		EnableTutorialMode(false);

	m_msgDisplayTime -= gEnv->pTimer->GetFrameTime();
	if(!m_enabled)
	{	
		if(m_msgDisplayTime < 0.0f && g_pGame->GetHUD())
			g_pGame->GetHUD()->ShowTutorialText(NULL,1);
		
		return;
	}

	CPlayer* pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
	if(!pPlayer)
		return;

	// don't start until game begins
	if(pPlayer->GetSpectatorMode() != 0 || g_pGame->GetGameRules()->GetCurrentStateId() != 3)
		return;

	if(!m_addedListeners && g_pGame->GetHUD())
	{
		// register as a HUD listener
		g_pGame->GetHUD()->RegisterListener(this);
		m_addedListeners = true;
	}

	// go through entity list and pull out the factories.
	if(m_baseList.empty())
	{
		IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
		while (!pIt->IsEnd())
		{
			if (IEntity * pEnt = pIt->Next())
			{
				if(pEnt->GetClass() == m_pHQClass)
				{
					m_baseList.push_back(pEnt->GetId());
				}
			}
		}
	}

	// first the briefing events. These are shown in order.
	bool showPrompt = CheckBriefingEvents(pPlayer);

	// player has been killed
	if(pPlayer->GetHealth() <= 0)
	{
		showPrompt = TriggerEvent(eTE_Killed);
	}
	else if(!showPrompt)
	{
		// check each event type here. Which might take a while.

		// entering a neutral factory
		// enter prototype factory
		// enter hostile factory
		// find alien crash
		m_entityCheckTimer -= gEnv->pTimer->GetFrameTime();
		if(m_entityCheckTimer < 0.0f)
		{
			CheckNearbyEntities(pPlayer);
			m_entityCheckTimer = ENTITY_CHECK_TIME;
		}

		// fill collector from alien
		if(pPlayer->GetCurrentItem())
		{
			IWeapon* pWeapon = pPlayer->GetCurrentItem()->GetIWeapon();
			if(pWeapon)
			{
				int fireModeIndex = pWeapon->GetCurrentFireMode();
				IFireMode* pFireMode = pWeapon->GetFireMode(fireModeIndex);
				if(pFireMode && !strcmp(pFireMode->GetName(), "Energise"))
				{
					CEnergise* pEnergise = static_cast<CEnergise*>(pFireMode);
					if(pEnergise->GetEnergy() > 0.99f)
					{
						TriggerEvent(eTE_FillCollector);
					}
				}
			}
		}

		// board vehicle and vehicle tutorials
		CheckVehicles(pPlayer);

		// player has been wounded
		if(pPlayer->GetHealth() < pPlayer->GetMaxHealth())
			TriggerEvent(eTE_Wounded);

		// bases
		m_baseCheckTimer -= gEnv->pTimer->GetFrameTime();
		if(m_baseCheckTimer < 0.0f)
		{
			CheckBases(pPlayer);
			m_baseCheckTimer = ENTITY_CHECK_TIME;
		}
	}

	bool promptShown = false;
	for(int i=0; i<eTE_NumEvents; ++i)
	{
		if(m_events[i].m_triggered)
		{
			if(m_msgDisplayTime < -MESSAGE_GAP_TIME)
			{
				ShowMessage(m_events[i]);
				m_events[i].m_triggered = false;
			}
			promptShown = true;
			break;
		}
	}

	if(!promptShown && (m_msgDisplayTime < 0.0f))
	{
		g_pGame->GetHUD()->ShowTutorialText(NULL,1);
	}
}

void CMPTutorial::ShowMessage(const STutorialEvent& event)
{
	int pos = 2; // 2=bottom, 1=middle
	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();	
	if(pClientActor && pClientActor->GetLinkedVehicle())
	{
		pos = 1;
	}

	// create the localised text string from enum
	string msg = "@mp_Tut" + event.m_name;
	wstring localizedString;
	wstring finalString;
	ILocalizationManager *pLocalizationMan = gEnv->pSystem->GetLocalizationManager();
	pLocalizationMan->LocalizeString(msg, localizedString);

	// fill out keynames if necessary
	if(!strcmp(event.m_name, "BoardVehicle"))
	{
		// special case for this event - has 5 actions associated...
		string action1 = "@cc_";
		string action2 = "@cc_";
		string action3 = "@cc_";
		string action4 = "@cc_";
		string action5 = "@cc_";

		action1 += GetKeyName("v_changeseat1");
		action2 += GetKeyName("v_changeseat2");
		action3 += GetKeyName("v_changeseat3");
		action4 += GetKeyName("v_changeseat4");
		action5 += GetKeyName("v_changeseat5");

		wstring wActions[5];
		pLocalizationMan->LocalizeString(action1, wActions[0]);
		pLocalizationMan->LocalizeString(action2, wActions[1]);
		pLocalizationMan->LocalizeString(action3, wActions[2]);
		pLocalizationMan->LocalizeString(action4, wActions[3]);
		pLocalizationMan->LocalizeString(action5, wActions[4]);

		const wchar_t* params[] = {wActions[0].c_str(), wActions[1].c_str(), wActions[2].c_str(), wActions[3].c_str(), wActions[4].c_str()};
		pLocalizationMan->FormatStringMessage(finalString, localizedString, params, 5);
	}
	else if(!event.m_action.empty())
	{
		// first get the key name (eg 'mouse1') and make it into a format the localization system understands

		string action = "@cc_";
		action += GetKeyName(event.m_action.c_str());
		wstring wActionString;
		pLocalizationMan->LocalizeString(action, wActionString);

		// now place this in the right place in the string.
		pLocalizationMan->FormatStringMessage(finalString, localizedString, wActionString.c_str());
	}
	else
	{
		finalString = localizedString;
	}

	// pass final string to hud tutorial window
	g_pGame->GetHUD()->ShowTutorialText(finalString.c_str(), pos);

	// also play the sound here
	string soundName = "mp_american/" + event.m_soundName;
	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(soundName.c_str(),0);
	if (pSound.get() && event.m_soundName.compare("None"))
	{
		pSound->AddEventListener(this, "mptutorial");
		m_msgDisplayTime = 1000.0f;	// long time - will be removed by sound finish event
		pSound->Play();
	}
	else
	{
		m_msgDisplayTime = MESSAGE_DISPLAY_TIME;
	}
}

const char* CMPTutorial::GetKeyName(const char* action)
{
	IActionMapManager* pAM = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	IActionMapIteratorPtr iter = pAM->CreateActionMapIterator();
	while (IActionMap* pMap = iter->Next())
	{
		const char* actionMapName = pMap->GetName();
		IActionMapBindInfoIteratorPtr pIter = pMap->CreateBindInfoIterator();
		while (const SActionMapBindInfo* pInfo = pIter->Next())
		{
			if(!strcmp(pInfo->action, action))
				return pInfo->keys[0];
		}
	}

	return "";
}

bool CMPTutorial::TriggerEvent(ETutorialEvent event)
{
	if(!m_events[event].m_shouldCheck)
		return false;

	m_events[event].m_shouldCheck = false;
	m_events[event].m_triggered = true;
	
	return true;
}

void CMPTutorial::ForceTriggerEvent(IConsoleCmdArgs* pArgs)
{
	CMPTutorial* pTutorial = g_pGame->GetGameRules()->GetMPTutorial();
	if(!pTutorial)
		return;

	if(pArgs && pArgs->GetArgCount() > 1)
	{
		// parse arg1 to find which event to trigger
		const char* testname = pArgs->GetArg(1);
		for(int i=0; i<eTE_NumEvents; ++i)
		{
			if(!strcmp(testname, pTutorial->m_events[i].m_name.c_str()))
			{
				pTutorial->m_events[i].m_triggered = true;
				return;
			}
		}
	}
}

void CMPTutorial::ResetAllEvents(IConsoleCmdArgs* args)
{
	CMPTutorial* pTutorial = g_pGame->GetGameRules()->GetMPTutorial();
	if(pTutorial)
	{
		for(int i=0; i<eTE_NumEvents; ++i)
		{
			pTutorial->m_events[i].m_shouldCheck = true;
			pTutorial->m_events[i].m_triggered = false;
		}
	}
}

void CMPTutorial::EnableTutorialMode(bool enable)
{
	if(enable == false && m_enabled)
	{
		// don't trigger this as normal, just fire off the message
		ShowMessage(m_events[eTE_TutorialDisabled]);
		m_events[eTE_TutorialDisabled].m_shouldCheck = false;
		m_msgDisplayTime = MESSAGE_DISPLAY_TIME;
	}

	m_enabled = enable;
}

bool CMPTutorial::CheckBriefingEvents(const CPlayer* pPlayer)
{
	bool showPrompt = false;

	if(m_currentBriefingEvent < eTE_FIRST_BRIEFING 
		|| m_currentBriefingEvent > eTE_LAST_BRIEFING
		|| !m_events[m_currentBriefingEvent].m_shouldCheck)
		return showPrompt;

	switch(m_currentBriefingEvent)
	{
	case eTE_StartGame:
		// trigger as soon as player spawns for the first time. Must be a better way than the state id?
		if(pPlayer->GetSpectatorMode() == 0 && g_pGame->GetGameRules()->GetCurrentStateId() == 3)
		{
			showPrompt = TriggerEvent(m_currentBriefingEvent);
			m_currentBriefingEvent = eTE_ContinueTutorial;
		}
		break;

	case eTE_ContinueTutorial:
		// trigger right after previous, so long as 'end tutorial' hasn't been pressed. (if it has, tutorial will be disabled)
		if(!m_events[eTE_StartGame].m_triggered)
		{
			showPrompt = TriggerEvent(m_currentBriefingEvent);
			m_currentBriefingEvent = eTE_Barracks;
		}
		break;

	case eTE_Barracks:
		// trigger 3s after previous ones dismissed
		if(!m_events[eTE_ContinueTutorial].m_triggered)
		{
			showPrompt = TriggerEvent(m_currentBriefingEvent);
			m_currentBriefingEvent = eTE_CloseBarracksBuyMenu;
		}
		break;

	case eTE_CloseBarracksBuyMenu:
		// trigger when player closes buy menu in barracks. This is triggered by the HUD so doesn't need checking here.
		m_currentBriefingEvent = eTE_OpenMap;
		break;

	case eTE_OpenMap:
		// trigger when player opens map for first time. This is triggered by the HUD so doesn't need checking here.
		m_currentBriefingEvent = eTE_CloseMap;
		break;

	case eTE_CloseMap:
		// trigger when player closes map. This is triggered by the HUD so doesn't need checking here.
		m_currentBriefingEvent = eTE_Swingometer;
		break;

	case eTE_Swingometer:
		// triggered after close map message
		if(!m_events[eTE_CloseMap].m_triggered)
		{
			showPrompt = TriggerEvent(m_currentBriefingEvent);
			m_currentBriefingEvent = eTE_NullEvent;
		}
		break;
	};

	return showPrompt;
}

bool CMPTutorial::CheckNearbyEntities(const CPlayer *pPlayer)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// checks for:
	//	eTE_NeutralFactory
	//	eTE_CaptureFactory
	//	eTE_EnterHostileFactory
	//	eTE_EnterPrototypeFactory
	//	eTE_ApproachEnemyBase
	//	eTE_ApproachEnemyHQ
	//	eTE_DestroyEnemyHQ
	//	eTE_FindAlienCrashSite
	//	eTE_FillReactor
	//	eTE_ReactorFull.
	//	If none of these need checking, don't bother.
	if(! ( m_events[eTE_NeutralFactory].m_shouldCheck
			|| m_events[eTE_CaptureFactory].m_shouldCheck
		  || m_events[eTE_EnterHostileFactory].m_shouldCheck
			|| m_events[eTE_EnterPrototypeFactory].m_shouldCheck
			|| m_events[eTE_ApproachEnemyBase].m_shouldCheck
			|| m_events[eTE_ApproachEnemyHq].m_shouldCheck
			|| m_events[eTE_DestroyEnemyHq].m_shouldCheck
			|| m_events[eTE_FindAlienCrashSite].m_shouldCheck
			|| m_events[eTE_FillReactor].m_shouldCheck
			|| m_events[eTE_ReactorFull].m_shouldCheck ))
	{
		return false;
	}

	bool showPrompt = false;
	if(!pPlayer)
		return showPrompt;

	Vec3 playerPos = pPlayer->GetEntity()->GetWorldPos();
	int playerTeam = g_pGame->GetGameRules()->GetTeam(pPlayer->GetEntityId());
	IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
	bool inPrototypeFactory = false;
	bool nearAlienEnergyPoint = false;
	while (!pIt->IsEnd())
	{
		if (IEntity * pEnt = pIt->Next())
		{
			// proximity check
			Vec3 vec = pEnt->GetWorldPos() - playerPos;
			float distanceSq = vec.GetLengthSquared();
			if(distanceSq > 500.0f)
				continue;		

			IEntityClass* pClass = pEnt->GetClass();
			if(pClass)
			{
				if(pClass == m_pFactoryClass)
				{
					// prompt depends on team and factory type
					inPrototypeFactory = g_pGame->GetHUD()->GetPowerStruggleHUD()->IsFactoryType(pEnt->GetId(), CHUDPowerStruggle::E_PROTOTYPES);
					int team = g_pGame->GetGameRules()->GetTeam(pEnt->GetId());
					if(team == 0)
					{
						showPrompt = TriggerEvent(eTE_NeutralFactory);
					}
					else if(team != playerTeam)
					{
						showPrompt = TriggerEvent(eTE_EnterHostileFactory);
					}
					else // team == playerTeam
					{
						showPrompt = TriggerEvent(eTE_CaptureFactory);

						if(inPrototypeFactory)
						{
							showPrompt = TriggerEvent(eTE_EnterPrototypeFactory);
						}
					}
				}
				else if(pClass == m_pAlienEnergyPointClass)
				{
					// AlienEnergyPoint refers to both crashed aliens and to the
					// reactor in prototype factories... check later after we know which.
					nearAlienEnergyPoint = true;
				}
				else if(pClass == m_pHQClass)
				{
					int team = g_pGame->GetGameRules()->GetTeam(pEnt->GetId());
					if(team != playerTeam)
					{
						if(distanceSq > 300.0f)
							showPrompt = TriggerEvent(eTE_ApproachEnemyBase);
						else if(distanceSq > 100.0f)
							showPrompt = TriggerEvent(eTE_ApproachEnemyHq);
						else
							showPrompt = TriggerEvent(eTE_DestroyEnemyHq);
					}
				}
				else if(pClass == m_pSpawnGroupClass)
				{
					if(distanceSq < 80)
					{
						if(g_pGame->GetGameRules()->GetTeam(pEnt->GetId()) == 0)
							showPrompt = TriggerEvent(eTE_SpawnBunker);
					}
				}
			}
		}

		if(showPrompt)
			break;
	}

	if(!showPrompt && nearAlienEnergyPoint)
	{
		// don't trigger until after player has been in the prototype factory (and been told to collect energy).
		if(!inPrototypeFactory && !m_events[eTE_EnterPrototypeFactory].m_shouldCheck)
		{
			// not a factory so must be a crash site
			TriggerEvent(eTE_FindAlienCrashSite);
		}
		else 
		{
			// factory. Check if we've filled the collector yet
			IItem* pItem = pPlayer->GetItemByClass(m_pTechChargerClass);
			if(pItem)
			{
				IWeapon* pWeapon = pItem->GetIWeapon();
				if(pWeapon)
				{
					CEnergise* pEnergise = static_cast<CEnergise*>(pWeapon->GetFireMode("Energise"));
					if(pEnergise && pEnergise->GetEnergy() > 0.95f)
					{
						// collector is full - prompt to fill the reactor
						TriggerEvent(eTE_FillReactor);
					}
					else if(pEnergise && pEnergise->GetEnergy() < 0.05f && !m_events[eTE_FillReactor].m_shouldCheck)
					{
						// collector is empty but has been full - reactor full now.
						TriggerEvent(eTE_ReactorFull);
					}
				}
			}
		}
	}

	return showPrompt;
}

bool CMPTutorial::CheckVehicles(const CPlayer* pPlayer)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	bool showPrompt = false;
	if(!pPlayer)
		return showPrompt;

	IVehicle* pVehicle = pPlayer->GetLinkedVehicle();

	if(m_wasInVehicle && !pVehicle)
	{
		// got out of vehicle. Move HUD box back to normal place.
		g_pGame->GetHUD()->SetTutorialTextPosition(2);
		m_wasInVehicle = false;
		return showPrompt;
	}
 
 	if(pVehicle && !m_wasInVehicle)
 	{
		// just got in. Move HUD box up so it doesn't overlap vehicle hud.
		g_pGame->GetHUD()->SetTutorialTextPosition(1);

		m_wasInVehicle = true;

		// generic 'boarding a vehicle' message
		TriggerEvent(eTE_BoardVehicle);

		// civ car is a bit different as it isn't consistently named. Check class instead.
		if(m_pSUVClass == pVehicle->GetEntity()->GetClass())
			TriggerEvent(eTE_SUV);

 		// go through vehicle list and see if current vehicle has a tutorial message...
		for(int i=eTE_FIRST_VEHICLE; i<=eTE_LAST_VEHICLE; ++i)
		{
			if(m_events[i].m_shouldCheck)
			{
				if(!strncmp(m_events[i].m_name.c_str(), pVehicle->GetEntity()->GetName(), m_events[i].m_name.length()))
				{
					showPrompt = TriggerEvent((ETutorialEvent)i);
					break;
				}
			}
		}				
	}

	return showPrompt;
}

bool CMPTutorial::CheckBases(const CPlayer *pPlayer)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// checks for:
	//	eTE_HqCritical
	//	eTE_HqUnderAttack
	//	eTE_TACTankBase
	//	eTE_SingularityBase
	//	If none need checking, don't bother
	if(! ( m_events[eTE_HqCritical].m_shouldCheck
			|| m_events[eTE_HqUnderAttack].m_shouldCheck
			|| m_events[eTE_TACTankBase].m_shouldCheck
			|| m_events[eTE_SingularityBase].m_shouldCheck ))
	{
		return false;
	}

	// go through the base list and check how close the player is to each.
	bool showPrompt = false;

	int localPlayerTeam = g_pGame->GetGameRules()->GetTeam(pPlayer->GetEntityId());

	std::list<EntityId>::iterator it = m_baseList.begin();
	for(; it != m_baseList.end(); ++it)
	{
		EntityId id = *it;

		IEntity* pBaseEntity = gEnv->pEntitySystem->GetEntity(id);
		if(!pBaseEntity)
			continue;

		int baseTeam = g_pGame->GetGameRules()->GetTeam(pBaseEntity->GetId());

		// first check if the player's HQ is under attack
		if(baseTeam == localPlayerTeam)
		{
			if(IScriptTable* pScriptTable = pBaseEntity->GetScriptTable())
			{
				HSCRIPTFUNCTION getHealth=0;
				if (!pScriptTable->GetValue("GetHealth", getHealth))
					continue;

				float health = 0;
				Script::CallReturn(gEnv->pScriptSystem, getHealth, pScriptTable, health);
				gEnv->pScriptSystem->ReleaseFunc(getHealth);

				// hmm, magic number. Can't see any way of finding the initial health....?
				if(health < 300)
				{
					TriggerEvent(eTE_HqCritical);
				}
				else if(health < 800)	
				{
					TriggerEvent(eTE_HqUnderAttack);
				}
			}

			SEntityProximityQuery query;
			Vec3	pos = pBaseEntity->GetWorldPos();
			query.nEntityFlags = -1;
			query.pEntityClass = m_pTankClass;
			float dim = 50.0f;
			query.box = AABB(Vec3(pos.x-dim,pos.y-dim,pos.z-dim), Vec3(pos.x+dim,pos.y+dim,pos.z+dim));
			gEnv->pEntitySystem->QueryProximity(query);
			for(int i=0; i<query.nCount; ++i)
			{
				IEntity* pEntity = query.pEntities[i];
				if(pEntity)
				{
					int tankTeam = g_pGame->GetGameRules()->GetTeam(pEntity->GetId());

					if(tankTeam != localPlayerTeam && tankTeam != baseTeam)
					{
						// cmp names here.
						if(!strncmp(pEntity->GetName(), "ustactank", 9))
						{
							TriggerEvent(eTE_TACTankBase);
						}
						else if(!strncmp(pEntity->GetName(), "ussingtank", 10))
						{
							TriggerEvent(eTE_SingularityBase);
						}
					}
				}
			}
		}
	}

	return showPrompt;
}