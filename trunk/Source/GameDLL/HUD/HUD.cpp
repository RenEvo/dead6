/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	G02 HUD using Flash player
	Code specific to G02 should go here
	For code needed by all games, see CHUDCommon


-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre
- 01:02:2006: Modified by Jan Müller
- 22:02:2006: Refactored for G04 by Matthew Jack
- 2007: Refactored by Jan Müller

*************************************************************************/
#include "StdAfx.h"
#include <StlUtils.h>
#include <ctype.h>

#include "Game.h"
#include "GameActions.h"
#include "GameCVars.h"
#include "MPTutorial.h"

#include "HUD.h"
#include "HUDTextChat.h"
#include "HUDObituary.h"
#include "HUDTextArea.h"
#include "HUDScore.h"

#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "Menus/FlashMenuObject.h"
#include "IPlayerProfiles.h"

#include "IUIDraw.h"
#include "ISound.h"
#include "IPlayerInput.h"
#include "IWorldQuery.h"
#include "IInput.h"
#include "IMaterialEffects.h"
#include "ISaveGame.h"
#include "ICryPak.h"

#include "GameRules.h"
#include "Item.h"
#include "Weapon.h"

#include "Tweaks/HUDTweakMenu.h"

#include "HUDVehicleInterface.h"
#include "HUDPowerStruggle.h"
#include "HUDScopes.h"
#include "HUDCrosshair.h"
#include "HUDTagNames.h"

#include "Menus/OptionsManager.h"

//-----------------------------------------------------------------------------------------------------

// TODO: time based
#define FADE_NUM_UPDATES (100)

#define HUD_CALL_LISTENERS(func) \
{ \
	if (m_hudListeners.empty() == false) \
	{ \
		m_hudTempListeners = m_hudListeners; \
		for (std::vector<IHUDListener*>::iterator tmpIter = m_hudTempListeners.begin(); tmpIter != m_hudTempListeners.end(); ++tmpIter) \
		   (*tmpIter)->func; \
	} \
}

//-----------------------------------------------------------------------------------------------------
void CHUD::OnSubtitleCVarChanged(ICVar *pCVar)
{
	int mode = pCVar->GetIVal();
	HUDSubtitleMode eMode = mode == 1 ? eHSM_All : (mode == 2 ? eHSM_CutSceneOnly : eHSM_Off);
	SAFE_HUD_FUNC(SetSubtitleMode(eMode));
}

//-----------------------------------------------------------------------------------------------------
void CHUD::OnCrosshairCVarChanged(ICVar *pCVar)
{
	SAFE_HUD_FUNC(m_pHUDCrosshair->SetCrosshair(pCVar->GetIVal()));
}

//-----------------------------------------------------------------------------------------------------
void CHUD::OnSubtitlePanoramicHeightCVarChanged(ICVar *pCVar)
{
	CHUD* pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		if (pHUD->m_cineState == eHCS_Fading)
		{
			SAFE_HUD_FUNC(FadeCinematicBars(pCVar->GetIVal()));
		}
	}
}

//-----------------------------------------------------------------------------------------------------

CHUD::CHUD() 
{
	CFlashMenuObject::GetFlashMenuObject()->SetColorChanged();
	// CHUDCommon constructor runs first
	m_pHUDRadar							= NULL;
	m_pHUDTweakMenu 				= NULL;
	m_pHUDScore							= NULL;
	m_pHUDTextChat  				= NULL;
	m_pRenderer							= NULL;
	m_pUIDraw								= NULL;
	m_pHUDVehicleInterface	= NULL;
	m_pHUDPowerStruggle			= NULL;
	m_pHUDScopes						= NULL;
	m_pHUDCrosshair					= NULL;
	m_pHUDTagNames					= NULL;

	m_forceScores = false;
	m_iFade = FADE_NUM_UPDATES;

	m_bLaunchWS = false;
	m_bIgnoreMiddleClick = true;
	m_pModalHUD = 0;
	m_pSwitchScoreboardHUD = 0;
	m_bScoreboardCursor = false;

	m_iVoiceMode = 0;
	m_lastPlayerPPSet = -1;
	m_iOpenTextChat = 0;

	m_pNanoSuit = NULL;
	m_eNanoSlotMax = NANOSLOT_ARMOR;
	m_bFirstFrame = true;
	m_bAutosnap = false;
	m_bHideCrosshair = false;
	m_fLastSoundPlayedCritical = 0;
	m_bHasWeaponAttachments = true;
	m_bInMenu = false;
	m_bBreakHUD = false;
	m_bHUDInitialize = false;
	m_bGrenadeLeftOrRight = false;
	m_bGrenadeBehind = false;
	m_bThirdPerson = false;
	m_bNightVisionActive = false;
	m_bMiniMapZooming = false;
	m_fCatchBuyMenuTimer = 0.0f;
	m_bTacLock = false;
	m_missionObjectiveNumEntries = 0;
	m_missionObjectiveValues.clear();
	m_bAirStrikeAvailable = false;
	m_fAirStrikeStarted = 0.0f;
	m_fDamageIndicatorTimer = 0;
	m_fNightVisionTimer = 0;
	m_fPlayerFallAndPlayTimer = 0.0f;
	m_bExclusiveListener = false;
	m_iPlayerOwnedVehicle = 0;
	m_bShowAllOnScreenObjectives = false;
	m_fMiddleTextLineTimeout = 0.0f;
	m_fPlayerRespawnTimer = 0.0f;
	m_fLastPlayerRespawnEffect = 0.0f;
	m_bRespawningFromFakeDeath = false;
	m_fPlayerDeathTime = 0.0f;
	m_bNoMiniMap = false;
	m_hasTACWeapon = false;

	m_pWeapon = NULL;
	m_uiWeapondID = 0;

	m_fAlpha = 0.0f;
	m_fSuitChangeSoundTimer = 0.0f;
	m_fLastReboot = 0.0f;
	m_fSetAgressorIcon = 0.0f;
	m_agressorIconID = 0;

	m_activeButtons = 31;	//011111
	m_defectButtons = 0;	//011111

	m_pSCAR			= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "SCAR" );
	m_pFY71			= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "FY71" );
	m_pSMG			= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "SMG" );
	m_pDSG1			= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "DSG1" );
	m_pShotgun	= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "Shotgun" );
	m_pLAW			= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "LAW" );
	m_pGauss		= gEnv->pEntitySystem->GetClassRegistry()->FindClass( "GaussRifle" );


	m_fDefenseTimer = m_fStrengthTimer = m_fSpeedTimer = 0;

	XmlNodeRef statusMessages = GetISystem()->LoadXmlFile("Libs/UI/StatusMessages.xml");

	if (statusMessages != NULL)
	{
		for(int j = 0; j < statusMessages->getNumAttributes(); ++j)
		{
			const char *attrib, *value;
			if(statusMessages->getAttributeByIndex(j, &attrib, &value))
				m_statusMessagesMap[string(attrib)] = string(value);
		}
	}

	if (gEnv->pSoundSystem)
	{
		gEnv->pSoundSystem->Precache("Sounds/interface:suit:generic_beep", FLAG_SOUND_2D, FLAG_SOUND_PRECACHE_DEFAULT );
	}

	m_entityTargetAutoaimId = 0;
	m_entityTargetLockId = 0;
	m_entityGrenadeDectectorId = 0;

	m_fAutosnapCursorRelativeX = 0.0f;
	m_fAutosnapCursorRelativeY = 0.0f;
	m_fAutosnapCursorControllerX = 0.0f;
	m_fAutosnapCursorControllerY = 0.0f;
	m_bOnCircle = false;

	//update current mission objectives
	m_missionObjectiveSystem.LoadLevelObjectives(true);
	m_fBattleStatus=0;
	m_fBattleStatusDelay=0;

	m_pDefaultFont = NULL;

	m_bSubtitlesNeedUpdate = false;
	m_hudSubTitleMode = eHSM_Off;
	m_cineState = eHCS_None;
	m_cineHideHUD = false;

	//just a small enough value to have no restrictions at startup
	m_lastNonAssistedInput=-3600.0f;
	m_hitAssistanceAvailable=false;

  m_currentGameRules = EHUD_SINGLEPLAYER;

	//the hud exists as long as the game exists
	gEnv->pGame->GetIGameFramework()->RegisterListener(this, "hud", FRAMEWORKLISTENERPRIORITY_HUD);
}

//-----------------------------------------------------------------------------------------------------

CHUD::~CHUD()
{
  ShowDeathFX(0);

	//stop looping sounds
	for(int i = (int)ESound_Hud_First+1; i < (int)ESound_Hud_Last;++i)
		PlaySound((ESound)i, false);

	HUD_CALL_LISTENERS(HUDDestroyed());

	IGameFramework* pGF = gEnv->pGame->GetIGameFramework();
	pGF->GetIItemSystem()->UnregisterListener(this);
	pGF->UnregisterListener(this);
	pGF->GetIViewSystem()->RemoveListener(this);
  CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
  if(pPlayer)
  {
    pPlayer->UnregisterPlayerEventListener(this);
    if(CNanoSuit *pSuit=pPlayer->GetNanoSuit())
      pSuit->RemoveListener(this);
  }

	ISubtitleManager* pSubtitleManager = pGF->GetISubtitleManager();
	if (pSubtitleManager != 0)
	{
		pSubtitleManager->SetHandler(0);
	}

	UnregisterTokens();

	OnSetActorItem(NULL,NULL);

	// call OnHUDDestroyed on hud objects. we own them, so delete afterwards
	std::for_each(m_hudObjectsList.begin(), m_hudObjectsList.end(), std::mem_fun(&CHUDObject::OnHUDToBeDestroyed));
	// now delete them
	std::for_each(m_hudObjectsList.begin(), m_hudObjectsList.end(), stl::container_object_deleter());
	m_hudObjectsList.clear();

	// call OnHUDDestroyed on external hud objects. we don't own them, so don't delete
	std::for_each(m_externalHUDObjectList.begin(), m_externalHUDObjectList.end(), std::mem_fun(&CHUDObject::OnHUDToBeDestroyed));

	PlayerIdSet(0);	//unregister from game / player

	SAFE_DELETE(m_pHUDScopes);

	gEnv->pInput->RemoveEventListener(this);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::RegisterTokens()
{
	IGameTokenSystem *pGameTokenSystem = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();

	pGameTokenSystem->RegisterListener("hud.weapon.ammo",							this,true,true);
	pGameTokenSystem->RegisterListener("hud.weapon.invammo",					this,true,true);
	pGameTokenSystem->RegisterListener("hud.weapon.clip_size",				this,true,true);
	pGameTokenSystem->RegisterListener("hud.weapon.crosshair_opacity",this,true,true);
	pGameTokenSystem->RegisterListener("hud.weapon.spread",						this,true,true);
	pGameTokenSystem->RegisterListener("hud.weapon.grenade_ammo",			this,true,true);
	pGameTokenSystem->RegisterListener("hud.weapon.grenade_type",			this,true,true);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UnregisterTokens()
{
	IGameTokenSystem *pGameTokenSystem = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();

	pGameTokenSystem->UnregisterListener("hud.weapon.ammo",							this);
	pGameTokenSystem->UnregisterListener("hud.weapon.invammo",					this);
	pGameTokenSystem->UnregisterListener("hud.weapon.clip_size",				this);
	pGameTokenSystem->UnregisterListener("hud.weapon.crosshair_opacity",this);
	pGameTokenSystem->UnregisterListener("hud.weapon.spread",						this);
	pGameTokenSystem->UnregisterListener("hud.weapon.grenade_ammo",			this);
	pGameTokenSystem->UnregisterListener("hud.weapon.grenade_type",			this);
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::Init()
{
	m_pRenderer = gEnv->pRenderer;
	m_pUIDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();

	m_pDefaultFont = gEnv->pCryFont->GetFont("default");
	CRY_ASSERT(m_pDefaultFont);

	bool loadEverything = gEnv->pCryPak->GetLvlResStatus();

	IScriptSystem* pScriptSystem = gEnv->pScriptSystem;

#ifdef USE_G15_LCD
	m_pLCD = new CLCDWrapper();
#endif //USE_G15_LCD

	m_pHUDScopes		= new CHUDScopes(this);
	m_pHUDRadar			= new CHUDRadar;
	m_pHUDObituary	= new CHUDObituary;
	m_pHUDTextArea	= new CHUDTextArea;
	m_pHUDTextArea->SetFadeTime(2.0f);
	m_pHUDTextArea->SetPos(Vec2(200.0f, 450.0f));

	m_pHUDTweakMenu = new CHUDTweakMenu( pScriptSystem );
	m_pHUDCrosshair = new CHUDCrosshair(this);
	m_pHUDTagNames	= new CHUDTagNames;

	if(gEnv->bMultiplayer)
	{
		m_pHUDTextChat	= new CHUDTextChat;
		m_pHUDScore			= new CHUDScore;
		m_hudObjectsList.push_back(m_pHUDTextChat);
		m_hudObjectsList.push_back(m_pHUDScore);
	}

	m_hudObjectsList.push_back(m_pHUDRadar);
	m_hudObjectsList.push_back(m_pHUDObituary);
	m_hudObjectsList.push_back(m_pHUDTextArea);
	m_hudObjectsList.push_back(m_pHUDTweakMenu);
	m_hudObjectsList.push_back(m_pHUDCrosshair);
	m_hudObjectsList.push_back(m_pHUDTagNames);

	m_animAmmo.Load("Libs/UI/HUD_Ammo.gfx", eGFD_Right);
	m_animGrenadeDetector.Load("Libs/UI/HUD_GrenadeDetect.gfx", eGFD_Center, eFAF_Visible);
	m_animMissionObjective.Load("Libs/UI/HUD_MissionObjective_Icon.gfx", eGFD_Center, eFAF_Visible);
	m_animHealthEnergy.Load("Libs/UI/HUD_HealthEnergy.gfx", eGFD_Right, eFAF_Visible);
	m_animQuickMenu.Load("Libs/UI/HUD_QuickMenu.gfx");
	m_animRadarCompassStealth.Load("Libs/UI/HUD_RadarCompassStealth.gfx", eGFD_Left, eFAF_Visible);
	m_pHUDRadar->SetFlashRadar(&m_animRadarCompassStealth);
	m_animSuitIcons.Load("Libs/UI/HUD_SuitIcons.gfx", eGFD_Right);
	m_animTacLock.Load("Libs/UI/HUD_Tac_Lock.gfx", eGFD_Center, eFAF_ThisHandler);
	m_animTargetLock.Load("Libs/UI/HUD_TargetLock.gfx", eGFD_Center, eFAF_ThisHandler);
	m_animTargetAutoAim.Load("Libs/UI/HUD_TargetAutoaim.gfx", eGFD_Center, eFAF_ThisHandler|eFAF_ManualRender);
	m_animWarningMessages.Load("Libs/UI/HUD_ErrorMessages.gfx", eGFD_Center, eFAF_ThisHandler|eFAF_ManualRender);

	//load the map
	m_animPDA.Load("Libs/UI/HUD_PDA_Map.gfx", eGFD_Right, eFAF_ThisHandler);
	m_animDownloadEntities.Load("Libs/UI/HUD_DownloadEntities.gfx");
	m_animInitialize.Load("Libs/UI/HUD_Initialize.gfx", eGFD_Center, eFAF_ManualRender|eFAF_Visible);

	// these are delay-loaded elsewhere!!!
	if(loadEverything)
	{
		m_animKillAreaWarning.Load("Libs/UI/HUD_Area_Warning.gfx", eGFD_Center, true);
		m_animDeathMessage.Load("Libs/UI/HUD_KillEvents.gfx", eGFD_Center, true);
		m_animGamepadConnected.Load("Libs/UI/HUD_GamePad_Stats.gfx");
		m_animAirStrike.Load("Libs/UI/HUD_AirStrikeLocking_Text.gfx");
	}
	else
	{
		m_animKillAreaWarning.Init("Libs/UI/HUD_Area_Warning.gfx", eGFD_Center, true);
		m_animDeathMessage.Init("Libs/UI/HUD_KillEvents.gfx", eGFD_Center, true);
		m_animGamepadConnected.Init("Libs/UI/HUD_GamePad_Stats.gfx");
		m_animAirStrike.Init("Libs/UI/HUD_AirStrikeLocking_Text.gfx");
	}

	m_animWeaponAccessories.Load("Libs/UI/HUD_WeaponAccessories.gfx", eGFD_Center, eFAF_ThisHandler);
	m_animMessages.Load("Libs/UI/HUD_Messages.gfx");
	if(loadEverything)
	{
		m_animCinematicBar.Load("Libs/UI/HUD_CineBar.gfx", eGFD_Center, eFAF_ThisHandler|eFAF_ManualRender|eFAF_Visible);
		m_animSubtitles.Load("Libs/UI/HUD_Subtitle.gfx", eGFD_Center, eFAF_ThisHandler|eFAF_ManualRender|eFAF_Visible);
	}
	else
	{
		m_animCinematicBar.Init("Libs/UI/HUD_CineBar.gfx", eGFD_Center, eFAF_ThisHandler|eFAF_ManualRender|eFAF_Visible);
		m_animSubtitles.Init("Libs/UI/HUD_Subtitle.gfx", eGFD_Center, eFAF_ThisHandler|eFAF_ManualRender|eFAF_Visible);
	}

	RegisterTokens();

	std::list<CHUDObject *>::iterator it;
	for(it = m_hudObjectsList.begin(); it != m_hudObjectsList.end(); ++it)
		(*it)->SetParent(this);

	m_pHUDRadar->SetFlashPDA(&m_animPDA);

	for(int i = 0; i < ESound_Hud_Last; ++i)
		m_soundIDs[i] = 0;

	//reload mission objectives
	m_missionObjectiveSystem.LoadLevelObjectives(true);

	UpdateHUDElements();

	g_pGame->GetIGameFramework()->GetIItemSystem()->RegisterListener(this);
	g_pGame->GetIGameFramework()->GetIViewSystem()->AddListener(this);

	// apply subtitle mode
	SetSubtitleMode((HUDSubtitleMode)g_pGameCVars->hud_subtitles);

	// in Editor mode, the LoadingComplete is not called because the HUD does not exist yet/not initialized yet
	if (gEnv->pSystem->IsEditor())
		m_pHUDRadar->OnLoadingComplete(0);

	m_pHUDVehicleInterface = new CHUDVehicleInterface(this, &m_animAmmo);
	m_hudObjectsList.push_back(m_pHUDVehicleInterface);

	m_pHUDPowerStruggle = new CHUDPowerStruggle(this, &m_animBuyMenu, &m_animBuyZoneIcon, &m_animCaptureProgress);
	m_hudObjectsList.push_back(m_pHUDPowerStruggle);

	m_lastPlayerPPSet = -1;
	m_iCatchBuyMenuPage = -2;
	m_deathFxId = InvalidEffectId;
  
  GameRulesSet(g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName());

	//if wanted, load everything that will be loaded later on
	if(loadEverything)
	{
		m_pHUDVehicleInterface->LoadVehicleHUDs(true);
		m_pHUDScopes->LoadFlashFiles(true);
		m_animBreakHUD.Load("Libs/UI/HUD_Lost.gfx", eGFD_Center, eFAF_ManualRender|eFAF_Visible);
		m_animRebootHUD.Load("Libs/UI/HUD_Reboot.gfx", eGFD_Center, eFAF_ManualRender);
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowBootSequence()
{
	for(TGameFlashAnimationsList::iterator iter=m_gameFlashAnimationsList.begin(); iter!=m_gameFlashAnimationsList.end(); ++iter)
	{
		CGameFlashAnimation *pAnim = (*iter);
		if (!(pAnim->GetFlags() & eFAF_ManualRender))
			pAnim->SetVariable("SkipSequence", SFlashVarValue(false));
	}
	SetHUDColor();
	//m_animInitialize.Invoke("Colorset.gotoAndPlay","1");
	m_animInitialize.SetVisible(true);
	m_iFade = FADE_NUM_UPDATES;
	m_bHUDInitialize = true;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowDownloadSequence()
{
	m_animDownloadEntities.Invoke("startDownload");
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateHUDElements()
{
	//Crosshair
	if(gEnv->pSystem->IsEditor())
		m_pHUDCrosshair->SetCrosshair(1);
	else
	{
		string sCrosshair;
		g_pGame->GetOptions()->GetProfileValue("Crosshair", sCrosshair);
		if(sCrosshair.empty())
			m_pHUDCrosshair->SetCrosshair(1);
		else
		{
			m_pHUDCrosshair->SetCrosshair(atoi(sCrosshair));
		}
	}

	//set special crosshair when having fists up
	if(IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor())
	{
		// No current item or current item are fists
		IItem *pItem = pActor->GetCurrentItem();
		if(!pItem || (pItem->GetEntity()->GetClass() == CItem::sFistsClass))
		{
			m_pHUDCrosshair->SetCrosshair(10);
		}
	}

	//Color
	bool bUpdate = CFlashMenuObject::GetFlashMenuObject()->ColorChanged();

	if(bUpdate)
	{
		SetHUDColor();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetHUDColor()
{
	CFlashMenuObject::GetFlashMenuObject()->UpdateMenuColor();

	TGameFlashAnimationsList::const_iterator iter=m_gameFlashAnimationsList.begin();
	TGameFlashAnimationsList::const_iterator end=m_gameFlashAnimationsList.end();
	for(; iter!=end; ++iter)
	{
		SetFlashColor(*iter);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetFlashColor(CGameFlashAnimation* pAnim)
{
	if(pAnim)
	{
		//re-setting colors
		pAnim->CheckedInvoke("setLineColor", SFlashVarValue(g_pGameCVars->hud_colorLine));
		pAnim->CheckedInvoke("setOverColor", SFlashVarValue(g_pGameCVars->hud_colorOver));
		pAnim->CheckedInvoke("setTextColor", SFlashVarValue(g_pGameCVars->hud_colorText));
		if (pAnim->IsAvailable("resetColor"))
			pAnim->Invoke("resetColor");
		else
			pAnim->CheckedInvoke("Root.gotoAndPlay", "restart");

		pAnim->GetFlashPlayer()->Advance(0.2f);
	}
}

//-----------------------------------------------------------------------------------------------------
// TODO: this function is accessed several times per frame.
// weapons/accessories changes should use a callback system and cache a pointer on the current item.
//-----------------------------------------------------------------------------------------------------

CWeapon *CHUD::GetWeapon()
{
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pActor && pActor->GetInventory())
	{
		EntityId uiItemId = pActor->GetInventory()->GetCurrentItem();
		if(uiItemId)
		{
			return pActor->GetWeapon(uiItemId);
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetTACWeapon(bool hasTACWeapon)
{
	m_hasTACWeapon = hasTACWeapon;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::PlayerIdSet(EntityId playerId)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
	if(pPlayer)
	{
		m_pNanoSuit = pPlayer->GetNanoSuit();
		assert(m_pNanoSuit); //the player requires to have a nanosuit!

		if(m_pNanoSuit)
		{
			m_fSuitEnergy = m_pNanoSuit->GetSuitEnergy();

			pPlayer->RegisterPlayerEventListener(this);
			m_pNanoSuit->AddListener(this);

			switch(m_pNanoSuit->GetMode())
			{
			case NANOMODE_DEFENSE:
				m_animSuitIcons.Invoke("setMode", "Armor");
				break;
			case NANOMODE_SPEED:
				m_animSuitIcons.Invoke("setMode", "Speed");
				break;
			case NANOMODE_STRENGTH:
				m_animSuitIcons.Invoke("setMode", "Strength");
				break;
			default:
				m_animSuitIcons.Invoke("setMode", "Cloak");
				break;
			}
		}

		gEnv->pInput->AddEventListener(this);

		GetMissionObjectiveSystem().DeactivateObjectives(true); //this should remove all "old" objectives
	}
	else
	{
		pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer)
		{
      pPlayer->UnregisterPlayerEventListener(this);
			if(CNanoSuit *pSuit=pPlayer->GetNanoSuit())
				pSuit->RemoveListener(this);
		}
	}
}

void CHUD::GameRulesSet(const char* name)
{
  EHUDGAMERULES gameRules = EHUD_SINGLEPLAYER;

  if(gEnv->bMultiplayer)
  {
    if(!stricmp(name, "InstantAction"))
      gameRules = EHUD_INSTANTACTION;
    else if(!stricmp(name, "PowerStruggle"))
      gameRules = EHUD_POWERSTRUGGLE;
		else if(!stricmp(name, "TeamAction"))
			gameRules = EHUD_TEAMACTION;
  }

  if(m_currentGameRules != gameRules)//unload stuff
  {
    LoadGameRulesHUD(false);
    m_currentGameRules = gameRules;
  }

  LoadGameRulesHUD(true);
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::UpdateNanoSlotMax()
{
	ENanoSlot eNanoSlotOld = m_eNanoSlotMax;

	if(m_pNanoSuit->GetSlotValue(NANOSLOT_ARMOR,false) > m_pNanoSuit->GetSlotValue(m_eNanoSlotMax,false))
	{
		m_eNanoSlotMax = NANOSLOT_ARMOR;
	}
	if(m_pNanoSuit->GetSlotValue(NANOSLOT_SPEED,false) > m_pNanoSuit->GetSlotValue(m_eNanoSlotMax,false))
	{
		m_eNanoSlotMax = NANOSLOT_SPEED;
	}
	if(m_pNanoSuit->GetSlotValue(NANOSLOT_STRENGTH,false) > m_pNanoSuit->GetSlotValue(m_eNanoSlotMax,false))
	{
		m_eNanoSlotMax = NANOSLOT_STRENGTH;
	}
	if(m_pNanoSuit->GetSlotValue(NANOSLOT_MEDICAL,false) > m_pNanoSuit->GetSlotValue(m_eNanoSlotMax,false))
	{
		m_eNanoSlotMax = NANOSLOT_MEDICAL;
	}

	if(m_eNanoSlotMax != eNanoSlotOld)
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SwitchToModalHUD(CGameFlashAnimation* pModalHUD,bool bNeedMouse)
{
	m_pModalHUD = pModalHUD;

	if(m_pModalHUD)
	{
		if(bNeedMouse)
		{
			CursorIncrementCounter();
		}
	}
	else
	{
		CRY_ASSERT(m_iCursorVisibilityCounter >= 0);

		while(m_iCursorVisibilityCounter)
			CursorDecrementCounter();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::Serialize(TSerialize ser,unsigned aspects)
{
	if (ser.GetSerializationTarget() != eST_Network)
	{
		ser.Value("hudFade", m_iFade);
		ser.Value("hudAlpha", m_fAlpha);
		ser.Value("hudShow", m_bShow);
		ser.Value("hudBroken", m_bBreakHUD);
		ser.Value("hudInit", m_bHUDInitialize);
		ser.EnumValue("hudCineState", m_cineState, eHCS_None, eHCS_Fading);
		ser.Value("hudCineHideHUD", m_cineHideHUD);
		ser.Value("thirdPerson", m_bThirdPerson);
		ser.Value("hudGodMode", m_godMode);
		ser.Value("hudBinocular", m_pHUDScopes->m_bShowBinoculars);
		ser.Value("hudBinocularNoHUD", m_pHUDScopes->m_bShowBinocularsNoHUD);
		ser.Value("hudBinocularDistance", m_pHUDScopes->m_fBinocularDistance);
		ser.Value("hudBattleStatus",m_fBattleStatus);
		ser.Value("hudBattleStatusDelay",m_fBattleStatusDelay);
		ser.Value("hudExclusiveInputListener",m_bExclusiveListener);
		ser.Value("hudAirStrikeAvailable",m_bAirStrikeAvailable);
		ser.Value("hudAirStrikeStarted",m_fAirStrikeStarted);
		ser.Value("hudAirStrikeTarget",m_iAirStrikeTarget);
		ser.Value("hudPlayerOwnedVehicle",m_iPlayerOwnedVehicle);
		ser.Value("hudShowAllOnScreenObjectives",m_bShowAllOnScreenObjectives);
		int iSize = m_possibleAirStrikeTargets.size();
		ser.Value("hudPossibleTargetsLength",iSize);
		ser.Value("hudGodDeaths",m_iDeaths);
		ser.Value("GrenadeDetector", m_entityGrenadeDectectorId);
		ser.Value("playerRespawnTimer", m_fPlayerRespawnTimer);
		ser.Value("playerFakeDeath", m_bRespawningFromFakeDeath);
		bool nightVisionActive = m_bNightVisionActive;
		ser.Value("nightVisionActive", nightVisionActive);

		//only for vector serialize
		char strID[64];
		if(ser.IsReading())
		{
			for (int i = 0; i < iSize; i++)
			{
				sprintf(strID,"hudPossibleTargetsID%d",i);
				EntityId id = 0;
				ser.Value(strID,id);
				m_possibleAirStrikeTargets.push_back(id);
			}
		}
		else
		{
			for (int i = 0; i < iSize; i++)
			{
				sprintf(strID,"hudPossibleTargetsID%d",i);
				ser.Value(strID,m_possibleAirStrikeTargets[i]);
			}
		}

		m_pHUDVehicleInterface->Serialize(ser);

		int iScope = m_pHUDScopes->GetCurrentScope();
		ser.Value("hudScope", iScope);
		if(ser.IsReading())
		{
			m_hudObjectivesList.clear();
			UpdateObjective(NULL);

			//remove the weapon accessory screen (can show up after loading if not removed explicitly)
			if(GetModalHUD() == &m_animWeaponAccessories)
				OnAction(g_pGame->Actions().hud_suit_menu, 1, 1.0f);
			//WeaponAccessoriesInterface(false, true);

			m_pHUDScopes->m_eShowScope = (CHUDScopes::EScopeMode) iScope;

			SetAirStrikeEnabled(m_bAirStrikeAvailable);

			BreakHUD((int)m_bBreakHUD);
			m_pHUDCrosshair->SetUsability(false); //sometimes the scripts don't update the usability after loading from mounted gun for example
			
			m_bMiniMapZooming = false;

			//reinit the stuff that should have been initialized on recreation
			RegisterTokens();
			//~reint

			SetGODMode(m_godMode, true);	//reset god mode - only necessary when reading
			m_pHUDScopes->SetBinocularsDistance(m_pHUDScopes->m_fBinocularDistance);
			// QuickMenu is a push holder thing, when we load, we don't hold
			// the middle click so we have to disable this menu !

			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			if (pPlayer)
			{
				// This will show/hide vehicles/scopes interfaces
				if(pPlayer->GetLinkedVehicle())
				{
					if(m_pHUDVehicleInterface->IsParachute())
						OnEnterVehicle(pPlayer,"Parachute","Closed",m_bThirdPerson);
					else
						m_pHUDVehicleInterface->OnEnterVehicle(pPlayer,m_bThirdPerson);
				}
				OnToggleThirdPerson(pPlayer,m_bThirdPerson); //problematic, because it doesn't know whether there is actually an vehicle

				// Reset cursor
				m_bScoreboardCursor = false;
				m_pSwitchScoreboardHUD = NULL;
				if(1 == m_iCursorVisibilityCounter)
				{
					CursorDecrementCounter();
				}
				else if(1 < m_iCursorVisibilityCounter)
				{
					CRY_ASSERT_MESSAGE(0,"This is not possible !");
					m_iCursorVisibilityCounter = 0;
				}
				if (IPlayerInput * pInput = pPlayer->GetPlayerInput())
					pInput->DisableXI(false);
				gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_mouse",false);
			}

			//gEnv->p3DEngine->SetPostEffectParam("NightVision_Active", float(m_bNightVisionActive));
			if(m_bNightVisionActive) //turn always off (safest way)
			{
				m_fNightVisionTimer = 0.0f; //reset timer
				OnAction(g_pGame->Actions().hud_night_vision, 1, 1.0f);	
			}
			if(nightVisionActive)
			{
				m_fNightVisionTimer = 0.0f; //reset timer
				OnAction(g_pGame->Actions().hud_night_vision, 1, 1.0f);	//turn on
			}

			SetGrenade(m_entityGrenadeDectectorId);
			m_animWeaponAccessories.ReInitVariables();
		}

		//**********************************Radar serialization
		m_pHUDRadar->Serialize(ser);

		//weapon setup should be serialized too?!
	}

	//serialize objectives after removing them from the hud
	m_missionObjectiveSystem.Serialize(ser, aspects);
	/*if(ser.GetSerializationTarget() != eST_Network && ser.IsReading())
	{
		m_onScreenMessageBuffer.clear();	//clear old/serialization messages
		m_onScreenMessage.clear();
	}*/
}
//-----------------------------------------------------------------------------------------------------

void CHUD::PostSerialize()
{
	//stop looping sounds
	for(int i = (int)ESound_Hud_First+1; i < (int)ESound_Hud_Last;++i)
		PlaySound((ESound)i, false);

	SwitchToModalHUD(NULL,false);

	if(m_pHUDVehicleInterface->GetHUDType())
	{
		m_pHUDVehicleInterface->UpdateVehicleHUDDisplay();
		m_pHUDVehicleInterface->UpdateDamages();
		m_pHUDVehicleInterface->UpdateSeats();
	}
	m_bFirstFrame = true;
	m_fHealth = 0.0f;
	m_fPlayerDeathTime = 0.0f;
	m_pHUDVehicleInterface->ChooseVehicleHUD();
	UpdateHealth();
	m_fMiddleTextLineTimeout = 0.0f; //has to be reset sometimes after loading
	DisplayFlashMessage("@game_loaded",2);
	m_animWarningMessages.SetVisible(false);
	m_subtitleEntries.clear();
	m_bSubtitlesNeedUpdate = true;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnGameTokenEvent( EGameTokenEvent event,IGameToken *pGameToken )
{
	assert(pGameToken);

	if(EGAMETOKEN_EVENT_CHANGE == event)
	{
		bool bUpdateAmmo = false;
		if(!strcmp("hud.weapon.ammo",pGameToken->GetName()))
		{
			pGameToken->GetValueAs(m_iWeaponAmmo);
			bUpdateAmmo = true;
		}
		else if(!strcmp("hud.weapon.invammo",pGameToken->GetName()))
		{
			pGameToken->GetValueAs(m_iWeaponInvAmmo);
			bUpdateAmmo = true;
		}
		else if(!strcmp("hud.weapon.clip_size",pGameToken->GetName()))
		{
			pGameToken->GetValueAs(m_iWeaponClipSize);
			bUpdateAmmo = true;
		}
		else if(!strcmp("hud.weapon.grenade_ammo",pGameToken->GetName()))
		{
			pGameToken->GetValueAs(m_iGrenadeAmmo);
			bUpdateAmmo = true;
		}
		else if(!strcmp("hud.weapon.grenade_type",pGameToken->GetName()))
		{
			pGameToken->GetValueAs(m_sGrenadeType);
			bUpdateAmmo = true;
		}
		else if(!strcmp("hud.weapon.crosshair_opacity",pGameToken->GetName()))
		{
			float fOpacity;
			pGameToken->GetValueAs(fOpacity);
			m_pHUDCrosshair->GetFlashAnim()->Invoke("setOpacity", fOpacity);
		}
		else if(!strcmp("hud.weapon.spread",pGameToken->GetName()))
		{
			float fSpread;
			pGameToken->GetValueAs(fSpread);
			fSpread = MIN(fSpread,15.0f);
			m_pHUDCrosshair->GetFlashAnim()->Invoke("setRecoil", fSpread*5.0f);
		}

		if(bUpdateAmmo)
		{
			SFlashVarValue args[6] = {m_iWeaponAmmo, m_iWeaponInvAmmo, m_iWeaponClipSize, 0, m_iGrenadeAmmo, (const char *)m_sGrenadeType};
			m_animAmmo.Invoke("setAmmo", args, 6);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnSetActorItem(IActor *pActor, IItem *pItem )
{
	if(pActor && !pActor->IsClient())
	{
		return;
	}

	IItem *pCurrentItem = NULL;
	if(pActor && (pCurrentItem=pActor->GetCurrentItem()) && pCurrentItem != pItem)
	{
		// If new item is different than the current item and is not a weapon,
		// it's certainly an attachment like a grenade launcher
		// In that case, we don't unregister from the weapon, we just ignore
	}
	else
	{
		if(m_uiWeapondID)
		{
			// We can't use m_pWeapon->RemoveEventListener because sometimes the IItem is destroyed before
			// we reach that point. So let's try to retrieve the weapon by its ID with the IItemSystem
			IItem *pLocalItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_uiWeapondID);
			if(pLocalItem)
			{
				IWeapon *pWeapon = pLocalItem->GetIWeapon();
				if(pWeapon)
				{
					pWeapon->RemoveEventListener(this);
				}
			}
			m_uiWeapondID = 0;
		}
		if(pItem)
		{
			m_pWeapon = pItem->GetIWeapon();
			if(m_pWeapon)
			{
				m_uiWeapondID = pItem->GetEntity()->GetId();
				m_pWeapon->AddEventListener(this,__FUNCTION__);
			}
		}
	}

	//notify the buymenu of the item change
	m_pHUDPowerStruggle->PopulateBuyList();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnDropActorItem(IActor *pActor, IItem *pItem )
{
	//notify the buymenu of the item change
	m_pHUDPowerStruggle->PopulateBuyList();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnSetActorAccessory(IActor *pActor, IItem *pItem )
{
	//notify the buymenu of the item change
	m_pHUDPowerStruggle->PopulateBuyList();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnDropActorAccessory(IActor *pActor, IItem *pItem )
{
	//notify the buymenu of the item change
	m_pHUDPowerStruggle->PopulateBuyList();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnStartTargetting(IWeapon *pWeapon)
{
	m_animTacLock.SetVisible(true);
	m_bTacLock = true;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnStopTargetting(IWeapon *pWeapon)
{
	m_animTacLock.SetVisible(false);
	m_bTacLock = false;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ModeChanged(ENanoMode mode)
{
	IAISignalExtraData* pData = NULL;
	CPlayer *pPlayer = NULL;
	switch(mode)
	{
	case NANOMODE_SPEED:
		m_animSuitIcons.Invoke("setMode", "Speed");
		m_fSpeedTimer = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
		m_fSuitChangeSoundTimer = m_fSpeedTimer;
		break;
	case NANOMODE_STRENGTH:
		m_animSuitIcons.Invoke("setMode", "Strength");
		m_fStrengthTimer = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
		m_fSuitChangeSoundTimer = m_fStrengthTimer;
		break;
	case NANOMODE_DEFENSE:
		m_animSuitIcons.Invoke("setMode", "Armor");
		m_fDefenseTimer = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
		m_fSuitChangeSoundTimer = m_fDefenseTimer;
		break;
	case NANOMODE_CLOAK:
		m_animSuitIcons.Invoke("setMode", "Cloak");

		PlaySound(ESound_PresetNavigationBeep);
		if(m_pNanoSuit->GetSlotValue(NANOSLOT_ARMOR, true) != 50 || m_pNanoSuit->GetSlotValue(NANOSLOT_SPEED, true) != 50 ||
			m_pNanoSuit->GetSlotValue(NANOSLOT_STRENGTH, true) != 50 || m_pNanoSuit->GetSlotValue(NANOSLOT_MEDICAL, true) != 50)
		{
			TextMessage("suit_modification_engaged");
		}

		m_fSuitChangeSoundTimer = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();

		pData = gEnv->pAISystem->CreateSignalExtraData();//AI System will be the owner of this data
		pData->iValue = NANOMODE_CLOAK;
		pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer && pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1,"OnNanoSuitMode",pPlayer->GetEntity()->GetAI(),pData);
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::EnergyChanged(float energy)
{
	m_animHealthEnergy.Invoke("setEnergy", energy*0.5f+1.0f);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnLoadingStart(ILevelInfo *pLevel)
{
	m_pHUDScopes->ShowBinoculars(false);

	ShowKillAreaWarning(false, 0);

	GetMissionObjectiveSystem().DeactivateObjectives(false); //deactivate old objectives
	m_animMessages.Invoke("reset");	//reset update texts

	if(m_pHUDVehicleInterface && m_pHUDVehicleInterface->GetHUDType() != CHUDVehicleInterface::EHUD_NONE)
		m_pHUDVehicleInterface->OnExitVehicle(NULL);

	//disable alien fear effect
	if(g_pGameCVars->hud_enableAlienInterference)
		g_pGameCVars->hud_enableAlienInterference = 0;
	m_sLastSaveGame = "";
	m_bThirdPerson = false;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnLoadingComplete(ILevel *pLevel)
{
	SwitchToModalHUD(NULL,false);

	//for the case we loaded from menu
	UnloadSimpleHUDElements(false);

	if(m_pHUDRadar)
		m_pHUDRadar->OnLoadingComplete(pLevel);
}

//-----------------------------------------------------------------------------------------------------

//the vehicle i paid for, was built
void CHUD::OnPlayerVehicleBuilt(EntityId playerId, EntityId vehicleId)
{
	if(!playerId || !vehicleId) return;
	EntityId localActor = gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId();
	if(playerId == localActor)
	{
		m_iPlayerOwnedVehicle = vehicleId;
	}
}
//-----------------------------------------------------------------------------------------------------

//handles all kinds of mouse clicks and flash callbacks
void CHUD::HandleFSCommand(const char *strCommand,const char *strArgs)
{
  if(g_pGameCVars->g_debug_fscommand)
    CryLog("HandleFSCommand : %s %s\n", strCommand, strArgs);
	HandleFSCommandPDA(strCommand,strArgs);

	float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();

	if(!strcmp(strCommand,"WeaponAccessory"))
	{
		CWeapon *pWeapon = GetWeapon();
		if(pWeapon)
		{
			string s(strArgs);
			int sep = s.find("+");

			if(sep==-1) return;

			string helper(s.substr(0,sep));
			string attachment(s.substr(sep+1, s.length()-(sep+1)));
			PlaySound(ESound_WeaponModification);
			if(!strcmp(attachment.c_str(),"NoAttachment"))
			{
				string curAttach = pWeapon->CurrentAttachment(helper.c_str());
				strArgs = curAttach;
			}
			else
			{
				strArgs = attachment.c_str();
			}

			const bool bAddAccessory = pWeapon->GetAccessory(strArgs) == 0;
			pWeapon->SwitchAccessory(strArgs);
			HUD_CALL_LISTENERS(WeaponAccessoryChanged(pWeapon, strArgs, bAddAccessory));

			// player's squadmates mimicking weapon switch accessory
			IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();//AI System will be the owner of this data
			pData->SetObjectName(strArgs);
			pData->iValue = bAddAccessory; // unmount/mount
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			if(pPlayer && pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
				gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,10,"OnSwitchWeaponAccessory",pPlayer->GetEntity()->GetAI(),pData);
		}
		return;
	}
	else if(!stricmp(strCommand, "Suicide"))
	{
		ShowWarningMessage(EHUD_SUICIDE);
	}
	else if(!strcmp(strCommand, "FlashGetKeyFocus"))
	{
		m_iCatchBuyMenuPage = -2;
		m_bExclusiveListener = true;
		GetISystem()->GetIInput()->ClearKeyState();
		GetISystem()->GetIInput()->SetExclusiveListener(this);
	}
	else if(!strcmp(strCommand, "FlashLostKeyFocus"))
	{
		if(m_bExclusiveListener)
		{
			GetISystem()->GetIInput()->ClearKeyState();
			GetISystem()->GetIInput()->SetExclusiveListener(NULL);
		}
	}
	if(!stricmp(strCommand, "menu_highlight"))
	{
		PlaySound(ESound_Highlight);
	}
	else if(!stricmp(strCommand, "menu_select"))
	{
		PlaySound(ESound_Select);
	}
	else if(!strcmp(strCommand, "MapOpened"))
	{
		m_pHUDRadar->InitMap();
	}
	else if(!strcmp(strCommand, "JoinGame"))
	{
		gEnv->pConsole->ExecuteString("join_game");
		ShowPDA(false);
	}
	else if(!strcmp(strCommand, "Autojoin"))
	{
		if (CGameRules *pGameRules=g_pGame->GetGameRules())
		{
			int lt=0;
			int ltn=0;
			int nteams=pGameRules->GetTeamCount();
			for (int i=1;i<=nteams;i++)
			{
				int n=pGameRules->GetTeamPlayerCount(i, true);
				if (!lt || ltn>n)
				{
					lt=i;
					ltn=n;
				}
			}

			if (lt==0)
				lt=1;

			CryFixedStringT<64> cmd("team ");
			cmd.append(pGameRules->GetTeamName(lt));
			gEnv->pConsole->ExecuteString(cmd);
		}
		ShowPDA(false);
	}
	else if(!strcmp(strCommand, "Spectate"))
	{
		ShowWarningMessage(EHUD_SPECTATOR);
	}
	else if(!strcmp(strCommand, "SwitchTeam"))
	{
		CGameRules* pRules = g_pGame->GetGameRules();
		if(pRules && pRules->GetTeamId("black") == pRules->GetTeam(gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId()))
			ShowWarningMessage(EHUD_SWITCHTOTAN);
		else
			ShowWarningMessage(EHUD_SWITCHTOBLACK);
	}
	else if(!strcmp(strCommand, "WarningBox"))
	{
		HandleWarningAnswer(strArgs);
	}
	else if(!strcmp(strCommand, "ChangeTeam"))
	{
		CGameRules* pRules = g_pGame->GetGameRules();
		IActor *pTempActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		if(pRules->GetTeamCount() > 1)
		{
			if(pRules->GetTeamId(strArgs) != pRules->GetTeam(pTempActor->GetEntityId()))
			{
				string command("team ");
				command.append(strArgs);
				gEnv->pConsole->ExecuteString(command.c_str());
				ShowPDA(false);
			}
		}
	}
	else if(!strcmp(strCommand,"StopInitialize"))
	{
		m_bHUDInitialize = false;
	}
	else if(!strcmp(strCommand,"QuickMenuSpeedPreset"))
	{
		OnQuickMenuSpeedPreset();
		QuickMenuSnapToMode(NANOMODE_SPEED);
		return;
	}
	else if(0 == strcmp(strCommand,"QuickMenuStrengthPreset"))
	{
		OnQuickMenuStrengthPreset();
		QuickMenuSnapToMode(NANOMODE_STRENGTH);
		return;
	}
	else if(0 == strcmp(strCommand,"QuickMenuDefensePreset"))
	{
		OnQuickMenuDefensePreset();
		QuickMenuSnapToMode(NANOMODE_DEFENSE);
		return;
	}
	else if(0 == strcmp(strCommand,"QuickMenuDefault")) //this is cloak now
	{
		OnCloak();
		QuickMenuSnapToMode(NANOMODE_CLOAK);
		return;
	}
	else if(!strcmp(strCommand,"QuickMenuSwitchWeaponAccessories"))
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		CWeapon *pWeapon = GetWeapon();
		//if(pWeapon)
			//CryLogAlways("busy/zooming/switching %d/%d/%d",(int)pWeapon->IsBusy(),(int)pWeapon->IsZooming(),(int)pWeapon->IsSwitchingFireMode());
		if(m_acceptNextWeaponCommand && pPlayer && pWeapon && !pWeapon->IsBusy() && 
			!pWeapon->IsZooming() && !pWeapon->IsSwitchingFireMode() && !pWeapon->IsModifying())
		{
			if(!pPlayer->GetActorStats()->isFrozen.Value())
			{
				if(UpdateWeaponAccessoriesScreen())
				{
					SwitchToModalHUD(&m_animWeaponAccessories,true);
					if (IPlayerInput * pPlayerInput = pPlayer->GetPlayerInput())
						pPlayerInput->DisableXI(true);
					m_acceptNextWeaponCommand = false;
					pWeapon->OnAction(pPlayer->GetEntityId(),"modify",0,1);
					m_bLaunchWS = true;
				}
			}
		}
		return;
	}
	else if(!strcmp(strCommand,"Cloak"))
	{	
		OnCloak();	
		return;
	}
	else if(!strcmp(strCommand,"QuickMenuSuitOverload"))
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		pPlayer->GetPlayerInput()->OnAction("suitoverload", eAAM_OnPress, 1.0f);

		return;
	}
	else if(!stricmp(strCommand, "MP_TeamMateSelected"))
	{
		EntityId id = static_cast<EntityId>(atoi(strArgs));
		m_pHUDRadar->SelectTeamMate(id, true);
	}
	else if(!stricmp(strCommand, "MP_KickPlayer"))
	{
		EntityId id = static_cast<EntityId>(atoi(strArgs));
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
		if(pEntity)
		{
			string kick("kick ");
			kick.append(pEntity->GetName());
			gEnv->pConsole->ExecuteString(kick.c_str());
		}
	}
	else if(!stricmp(strCommand, "MuteMember"))
	{
		EntityId id = static_cast<EntityId>(atoi(strArgs));
		if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id))
			if(IVoiceContext *pVoiceContext = gEnv->pGame->GetIGameFramework()->GetNetContext()->GetVoiceContext())
				if(IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor())
					pVoiceContext->Mute(pClientActor->GetEntityId(), id, true);
	}
	else if(!stricmp(strCommand, "UnMuteMember"))
	{
		EntityId id = static_cast<EntityId>(atoi(strArgs));
		if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(id))
			if(IVoiceContext *pVoiceContext = gEnv->pGame->GetIGameFramework()->GetNetContext()->GetVoiceContext())
				if(IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor())
					pVoiceContext->Mute(pClientActor->GetEntityId(), id, false);
	}
	else if(!stricmp(strCommand, "MP_TeamMateDeselected"))
	{
		EntityId id = static_cast<EntityId>(atoi(strArgs));
		m_pHUDRadar->SelectTeamMate(id, false);
	}
	//multiplayer map functions
	if (gEnv->bMultiplayer)
	{
		if(!strcmp(strCommand, "MPMap_SelectObjective"))
		{
			EntityId id = 0;
			if(strArgs)
				id = EntityId(atoi(strArgs));
			SetOnScreenObjective(id);
		}
		else if(!strcmp(strCommand, "MPMap_SelectSpawnPoint"))
		{
			EntityId id = 0;
			if(strArgs)
				id = EntityId(atoi(strArgs));

			CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetClientActor());

			CGameRules *pGameRules = (CGameRules*)(gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules());
			EntityId iCurrentSpawnPoint = 0;
			if(pGameRules)
				iCurrentSpawnPoint = pGameRules->GetPlayerSpawnGroup(pActor);

			if(iCurrentSpawnPoint && iCurrentSpawnPoint==id)
			{
				SetOnScreenObjective(id);
			}
			else
			{
				g_pGame->GetGameRules()->RequestSpawnGroup(id);
			}
		}
		else if(!strcmp(strCommand, "HoverBuyItem"))
		{
			if(strArgs)
			{
				HUD_CALL_LISTENERS(OnBuyMenuItemHover(strArgs));
			}
		}
		else if(!strcmp(strCommand, "RequestNewLoadoutName"))
		{
			if(m_pModalHUD == &m_animBuyMenu)
			{
				string name;
				m_pHUDPowerStruggle->RequestNewLoadoutName(name);
				m_animBuyMenu.SetVariable("_root.POPUP.POPUP_NewPackage.m_modifyPackageName", SFlashVarValue(name));
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnQuickMenuSpeedPreset()
{
	if(m_pNanoSuit->GetMode() != NANOMODE_SPEED)
	{
		m_pNanoSuit->SetMode(NANOMODE_SPEED);

		m_fAutosnapCursorRelativeX = 0.0f;
		m_fAutosnapCursorRelativeY = -30.0f;
	}
	else
		PlaySound(ESound_TemperatureBeep);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnQuickMenuStrengthPreset()
{
	if(m_pNanoSuit->GetMode() != NANOMODE_STRENGTH)
	{
		m_pNanoSuit->SetMode(NANOMODE_STRENGTH);

		m_fAutosnapCursorRelativeX = 30.0f;
		m_fAutosnapCursorRelativeY = 0.0f;
	}
	else
		PlaySound(ESound_TemperatureBeep);
}
//-----------------------------------------------------------------------------------------------------


void CHUD::OnQuickMenuDefensePreset()
{
	if(m_pNanoSuit->GetMode() != NANOMODE_DEFENSE)
	{
		m_pNanoSuit->SetMode(NANOMODE_DEFENSE);

		m_fAutosnapCursorRelativeX = -30.0f;
		m_fAutosnapCursorRelativeY = 0.0f;
	}
	else
		PlaySound(ESound_TemperatureBeep);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnCloak()
{
	char cloakState(m_pNanoSuit->GetCloak()->GetState());
	if (!cloakState)	//if cloak is getting activated
	{
		if(m_pNanoSuit->GetMode() != NANOMODE_CLOAK)
		{
			m_pNanoSuit->SetMode(NANOMODE_CLOAK);

			m_fAutosnapCursorRelativeX = 20.0f;
			m_fAutosnapCursorRelativeY = 30.0f;
		}
		else
			PlaySound(ESound_TemperatureBeep);
	}
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::OnInputEvent(const SInputEvent &rInputEvent)
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return false;

	// Prevent cheating with using mouse and assisted controller at the same time
	if (rInputEvent.deviceId == eDI_Mouse)
		m_lastNonAssistedInput=gEnv->pTimer->GetCurrTime();

	bool assistance=IsInputAssisted();
	if (m_hitAssistanceAvailable != assistance)
	{
		// Notify server on the change
		IActor *pSelfActor=g_pGame->GetIGameFramework()->GetClientActor();
		if (pSelfActor)
			pSelfActor->GetGameObject()->InvokeRMI(CPlayer::SvRequestHitAssistance(), CPlayer::HitAssistanceParams(assistance), eRMI_ToServer);

		m_hitAssistanceAvailable=assistance;
	}

	if(m_iCatchBuyMenuPage<-1)
	{
		if(eDI_Keyboard == rInputEvent.deviceId)
		{
			IFlashPlayer *pFlashPlayer = NULL;
			if(m_pModalHUD == &m_animPDA)
			{
				pFlashPlayer = m_animPDA.GetFlashPlayer();
			}
			else if(m_pModalHUD == &m_animBuyMenu)
			{
				pFlashPlayer = m_animBuyMenu.GetFlashPlayer();
			}
			if(pFlashPlayer)
			{
				bool specialKey;
				int keyCode = CFlashMenuObject::GetFlashMenuObject()->KeyNameToKeyCode(rInputEvent, (uint32)rInputEvent.state, specialKey);
				unsigned char asciiCode = 0;
				if(!specialKey)
				{
					asciiCode = (unsigned char)keyCode;
				}
				if(eIS_Pressed == rInputEvent.state)
				{
					SFlashKeyEvent keyEvent(SFlashKeyEvent::eKeyDown, static_cast<SFlashKeyEvent::EKeyCode>(keyCode), asciiCode);
					pFlashPlayer->SendKeyEvent(keyEvent);
				}
				else if(eIS_Released == rInputEvent.state)
				{
					SFlashKeyEvent keyEvent(SFlashKeyEvent::eKeyUp, static_cast<SFlashKeyEvent::EKeyCode>(keyCode), asciiCode);
						pFlashPlayer->SendKeyEvent(keyEvent);
				}
				return true;
			}
		}
		return false;
	}

	if(m_animRadioButtons.GetVisible())
		return false;

	if(!m_animBuyMenu.IsLoaded())
		return false;

	if (rInputEvent.deviceId!=eDI_Keyboard)
		return false;

	if (rInputEvent.state != eIS_Released)
		return false;

	if(gEnv->pConsole->GetStatus())
		return false;

	if (!gEnv->bMultiplayer)
		return false;

	const char* sKey = rInputEvent.keyName.c_str();
	// nasty check, but fastest early out
	if(sKey && sKey[0] && !sKey[1])
	{
		int iKey = atoi(sKey);
		if(iKey == 0 && sKey[0] != '0')
			iKey = -1;

		if (iKey >= 0)
		{
			if(m_iCatchBuyMenuPage==-1)
			{
				m_animBuyMenu.Invoke("Root.PDAArea.Tabs.gotoTab", SFlashVarValue(sKey));
				m_iCatchBuyMenuPage = 0;
			}
			else if(m_iCatchBuyMenuPage>-1)
			{
				float fNow = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();
				if(m_fCatchBuyMenuTimer != 0.0f)
				{
					if(m_iCatchBuyMenuPage<10)
					{
						m_iCatchBuyMenuPage*=10;
					}
					m_iCatchBuyMenuPage +=iKey;
					BuyViaFlash(m_iCatchBuyMenuPage);
					return false;
				}
				if(iKey==0)
					iKey = 10;

				m_fCatchBuyMenuTimer = fNow;
				m_iCatchBuyMenuPage = iKey;
				m_animBuyMenu.Invoke("Root.PDAArea.selectItem", iKey);
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::BuyViaFlash(int iItem)
{
	m_animBuyMenu.Invoke("Root.PDAArea.buyItem", iItem);
	m_iCatchBuyMenuPage = -2;
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::OnAction(const ActionId& action, int activationMode, float value)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);
	const SGameActions &rGameActions = g_pGame->Actions();
	bool filterOut = true;

	if(action == rGameActions.toggle_airstrike)
	{
		SetAirStrikeEnabled(!IsAirStrikeAvailable());
	}

	if(action == rGameActions.hud_mousex)
	{
		if(m_pHUDRadar->GetDrag())
		{
			m_pHUDRadar->DragMap(Vec2(value,0));
		}
		else if(m_bAutosnap)
		{
			m_fAutosnapCursorRelativeX += value;
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_mousey)
	{
		if(m_pHUDRadar->GetDrag())
		{
			m_pHUDRadar->DragMap(Vec2(0,value));
		}
		if(m_bMiniMapZooming && m_pModalHUD == &m_animPDA)
		{
			m_pHUDRadar->ZoomChangePDA(value);
		}
		else if(m_bAutosnap)
		{
			m_fAutosnapCursorRelativeY += value;
		}
		filterOut = false;
	}
	else if(action == rGameActions.xi_rotatepitch)
	{
		if(m_iCursorVisibilityCounter)
		{
			m_fAutosnapCursorControllerY = value*value*(-value);
		}
		else if(m_bAutosnap)
		{
			if(fabsf(value)>0.5f || (fabsf(m_fAutosnapCursorControllerX) + fabsf(value))>0.5)
				m_fAutosnapCursorControllerY = -value;
		}
		filterOut = false;
	}
	else if(action == rGameActions.xi_rotateyaw)
	{
		if(m_iCursorVisibilityCounter)
		{
			m_fAutosnapCursorControllerX = value*value*value;
		}
		else if(m_bAutosnap)
		{
			if(fabsf(value)>0.5f || (fabsf(m_fAutosnapCursorControllerY) + fabsf(value))>0.5)
				m_fAutosnapCursorControllerX = value;
		}
		filterOut = false;
	}
	else if(action != rGameActions.rotateyaw && action != rGameActions.rotatepitch)
	{
		if(m_fPlayerDeathTime && gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_fPlayerDeathTime > 3.0f)
		{
			if(!g_pGame->GetMenu()->IsActive())
			{
				m_fPlayerDeathTime = m_fPlayerDeathTime -  20.0f; //0.0f;  //load immediately
				DisplayFlashMessage("", 2);
				return true;
			}
		}
	}

	if(action == rGameActions.attack1 && activationMode == eIS_Pressed)
	{
		//revive
		if(m_fPlayerRespawnTimer && !m_bRespawningFromFakeDeath)
		{
			m_bRespawningFromFakeDeath = true;
			m_fPlayerRespawnTimer = gEnv->pTimer->GetFrameStartTime().GetSeconds() + 4.0f;
		}
		//AirStrike
		if (IsAirStrikeAvailable() &&  m_pHUDScopes->IsBinocularsShown())
		{
			if(StartAirStrike())
			{
				m_fAirStrikeStarted = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			}
		}
	}
	
	//hack for Bernd to allow shooting while in quick menu :-(
	if(action == rGameActions.hud_mouseclick  && activationMode == eIS_Pressed)
	{
		if(m_pModalHUD == &m_animQuickMenu)
		{
			IActor *pPlayer = g_pGame->GetIGameFramework()->GetClientActor();
			if(pPlayer && !pPlayer->GetLinkedVehicle())
			{
				IItem *pItem = pPlayer->GetCurrentItem();
				if(pItem->GetIWeapon())
					pItem->GetIWeapon()->OnAction(pPlayer->GetEntityId(), rGameActions.attack1, 1, 1.0f);
			}
		}
	}

	if(action == rGameActions.hud_buy_weapons)
	{
		if(m_pModalHUD == &m_animBuyMenu)
			ShowBuyMenu(false);
		else
			ShowBuyMenu(true);
		return false;
	}

	if(action == rGameActions.hud_suit_menu && activationMode == eIS_Pressed)
	{
		if(!m_bShow || gEnv->pConsole->GetStatus())	//don't process while hud is disabled or console is opened
			return false;

		if(m_pModalHUD == &m_animPDA)
		{
			m_bMiniMapZooming = true;
		}
		else if(IsModalHUDAvailable())
		{
			CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			if(pActor && (pActor->GetHealth() > 0) && !pActor->GetSpectatorMode())
			{
				if(!m_animQuickMenu.IsLoaded())
					m_animQuickMenu.Reload("Libs/UI/HUD_QuickMenu.gfx");
				m_animQuickMenu.Invoke("showQuickMenu");
				UpdateWeaponAccessoriesScreen(false);
				CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
				if(pPlayer)
				{
					QuickMenuSnapToMode(pPlayer->GetNanoSuit()->GetMode());
					pPlayer->GetPlayerInput()->DisableXI(true);
				}
				PlaySound(ESound_SuitMenuAppear);
				pPlayer->GetPlayerInput()->DisableXI(true);
				gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_mouse",true);
				gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("in_vehicle_suit_menu",true);
				m_bAutosnap = true;
				UpdateCrosshairVisibility();
				SwitchToModalHUD(&m_animQuickMenu,false);
			}
			filterOut = false;
		}
		else if(m_pModalHUD == &m_animWeaponAccessories)
		{
			if(!m_bIgnoreMiddleClick)
			{
				SwitchToModalHUD(NULL,false);
				CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
				if(pPlayer)
					pPlayer->GetPlayerInput()->DisableXI(false);
				CWeapon *pWeapon = GetWeapon();
				m_acceptNextWeaponCommand = false;
				if(pPlayer && pWeapon)
				{
					pWeapon->OnAction(pPlayer->GetEntityId(),"modify",0,1);
				}
			}
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_suit_menu && activationMode == eIS_Released)
	{
		if(m_pModalHUD == &m_animPDA)
		{
			m_bMiniMapZooming = false;
		}
		else if(&m_animQuickMenu == m_pModalHUD)
		{
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			pPlayer->GetPlayerInput()->DisableXI(false);
			gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_mouse",false);
			gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("in_vehicle_suit_menu",false);
			m_bAutosnap = false;
			UpdateCrosshairVisibility();
			SwitchToModalHUD(NULL,false);
			m_animQuickMenu.CheckedInvoke("Root.QuickMenu.setAutosnapFunction");
			m_fAutosnapCursorRelativeX = 0;
			m_fAutosnapCursorRelativeY = 0;
			m_fAutosnapCursorControllerX = 0;
			m_fAutosnapCursorControllerY = 0;
			m_bOnCircle = false;

			m_animQuickMenu.Invoke("hideQuickMenu");

			if(!m_bLaunchWS)
				PlaySound(ESound_SuitMenuDisappear);

			m_bLaunchWS = false;
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_openchat || action == rGameActions.hud_openteamchat)
	{
		m_iOpenTextChat = (action == rGameActions.hud_openchat)? 1 : 2;
		return true;
	}
	else if(action == rGameActions.hud_toggle_scoreboard_cursor)
	{
		if(m_pModalHUD == &m_animScoreBoard)
		{
			if(activationMode == eIS_Pressed)
			{
				m_bScoreboardCursor = true;
				CursorIncrementCounter();
				gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_move",true);
				CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
				pPlayer->GetPlayerInput()->DisableXI(true);
			}
			else if(activationMode == eIS_Released)
			{
				if(m_bScoreboardCursor)
				{
					m_bScoreboardCursor = false;
					CursorDecrementCounter();
					gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_move",false);
					CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
					pPlayer->GetPlayerInput()->DisableXI(false);
				}
			}
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_show_multiplayer_scoreboard && activationMode == eIS_Pressed)
	{
		if(gEnv->bMultiplayer)
		{
			if(m_animScoreBoard.IsLoaded() && m_pHUDScore && !m_pHUDScore->m_bShow && GetModalHUD() != &m_animWarningMessages)
			{
				m_pSwitchScoreboardHUD = m_pModalHUD;
				if(m_pSwitchScoreboardHUD && m_pSwitchScoreboardHUD != &m_animQuickMenu)
				{
					// We don't want to have the cursor already active in the scoreboard
					// The player HAS to hit the spacebar to activate the mouse!
					CursorDecrementCounter();
				}
				SwitchToModalHUD(&m_animScoreBoard,false);
				PlaySound(ESound_OpenPopup);
				m_animScoreBoard.Invoke("Root.setVisible", 1);
				if(IActor* pActor = g_pGame->GetIGameFramework()->GetClientActor())
					m_animScoreBoard.Invoke("setOwnTeam", g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId()));
				m_pHUDScore->SetVisible(true, &m_animScoreBoard);
				m_bShow = false;

				HUD_CALL_LISTENERS(OnShowScoreBoard());
			}
		}
		else
		{
			if(m_animObjectivesTab.GetVisible())
				ShowObjectives(false); //with XBOX controller "back" works different: one press: on, second press: off
			else
				ShowObjectives(true);
		}
		filterOut = false;
	}
	else if(	!m_forceScores &&
						((action == rGameActions.hud_show_multiplayer_scoreboard && activationMode == eIS_Released)||
						action == rGameActions.hud_hide_multiplayer_scoreboard))
	{
		if(gEnv->bMultiplayer)
		{
			if(m_animScoreBoard.IsLoaded() && m_pHUDScore && m_pHUDScore->m_bShow)
			{
				if(m_bScoreboardCursor)
				{
					m_bScoreboardCursor = false;
					CursorDecrementCounter();
					gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("no_move",false);
					CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
					pPlayer->GetPlayerInput()->DisableXI(false);
				}
				if(m_pSwitchScoreboardHUD && m_pSwitchScoreboardHUD != &m_animQuickMenu)
				{
					// Restore the mouse if we were coming from a modal hud
					// Seems that only the quick menu does not use the mouse
					CursorIncrementCounter();
				}
				SwitchToModalHUD(m_pSwitchScoreboardHUD,(m_pSwitchScoreboardHUD==&m_animQuickMenu)?true:false);
				PlaySound(ESound_ClosePopup);
				m_animScoreBoard.Invoke("Root.setVisible", 0);
				m_pHUDScore->SetVisible(false);
				m_bShow = true;
			}
		}
		else
		{
			ShowObjectives(false);
		}
	}
	else if(action == rGameActions.hud_show_pda || action == rGameActions.hud_show_pda_map)
	{
		ShowPDA(m_pModalHUD != &m_animPDA);
		filterOut = false;
	}
	else if(action == rGameActions.hud_mousewheelup)
	{
		if(m_pModalHUD == &m_animPDA)
			m_pHUDRadar->ZoomPDA(true);
		filterOut = false;
	}
	else if(action == rGameActions.hud_mousewheeldown)
	{
		if(m_pModalHUD == &m_animPDA)
			m_pHUDRadar->ZoomPDA(false);
		filterOut = false;
	}
	else if(action == rGameActions.hud_mouserightbtndown)
	{
		if(m_pModalHUD == &m_animPDA)
			m_pHUDRadar->SetDrag(true);
		filterOut = false;
	}
	else if(action == rGameActions.hud_mouserightbtnup)
	{
		if(m_pModalHUD == &m_animPDA)
			m_pHUDRadar->SetDrag(false);
		filterOut = false;
	}
	else if(action == rGameActions.hud_night_vision)
	{
		float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		if(now - m_fNightVisionTimer > 0.2f)	//strange bug - action is produces twice "onPress" (even when set to onRelease)
		{
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			CNanoSuit *pSuit=pPlayer?pPlayer->GetNanoSuit():0;

			if (m_bNightVisionActive || (pSuit && pSuit->IsNightVisionEnabled()))
			{
				gEnv->p3DEngine->SetPostEffectParam("NightVision_Active", float(!m_bNightVisionActive));
				m_bNightVisionActive = !m_bNightVisionActive;

				if(m_bNightVisionActive)
				{
					PlaySound(ESound_NightVisionSelect);
					PlaySound(ESound_NightVisionAmbience);
				}
				else
				{
					PlaySound(ESound_NightVisionAmbience, false);
					PlaySound(ESound_NightVisionDeselect);
				}
				HUD_CALL_LISTENERS(OnNightVision(m_bNightVisionActive));
				m_fNightVisionTimer = now;
			}
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_weapon_mod)
	{
		float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();

		if(m_pModalHUD == NULL)
		{
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			CWeapon *pWeapon = GetWeapon();
			if(pPlayer && pWeapon)
			{
				bool busy = pWeapon->IsBusy();
				bool modify = pWeapon->IsModifying();
				bool zoomed = pWeapon->IsZoomed();
				bool zooming = pWeapon->IsZooming();
				bool switchFM = pWeapon->IsSwitchingFireMode();

				if(m_acceptNextWeaponCommand && !busy && !modify && !zoomed && !zooming && !switchFM)
				{
					if(!pPlayer->GetActorStats()->isFrozen.Value())
					{
						if(UpdateWeaponAccessoriesScreen())
						{
							SwitchToModalHUD(&m_animWeaponAccessories,true);
							pPlayer->GetPlayerInput()->DisableXI(true);
							
							m_acceptNextWeaponCommand = false;
							pWeapon->OnAction(pPlayer->GetEntityId(),"modify",0,1);
						}
					}
				}
				filterOut = false;
			}
		}
		else if(m_pModalHUD == &m_animWeaponAccessories)
		{
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			CWeapon *pWeapon = GetWeapon();
			if(m_acceptNextWeaponCommand && pPlayer && pWeapon)
			{
				SPlayerStats *pStats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
				if(!pStats->bSprinting)
				{
					SwitchToModalHUD(NULL,false);
					pPlayer->GetPlayerInput()->DisableXI(false);
					m_acceptNextWeaponCommand = false;
					pWeapon->OnAction(pPlayer->GetEntityId(),"modify",0,1);
				}
			}
		}
		filterOut = false;
	}
	else if(action == rGameActions.objectives)
	{
		if(activationMode == eIS_Pressed)
		{
			m_bShowAllOnScreenObjectives = true;
		}
		else if(activationMode == eIS_Released)
		{
			m_bShowAllOnScreenObjectives = false;
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_suit_mod)
	{
		if(activationMode == 1)
			OnAction(rGameActions.hud_suit_menu, eIS_Pressed, 1);
		else if(activationMode == 2)
			OnAction(rGameActions.hud_suit_menu, eIS_Released, 1);
		filterOut = false;
	}
	else if(action == rGameActions.hud_select1)
	{
		if(m_animRadioButtons.GetVisible()) return false;
		if(m_pModalHUD == &m_animWeaponAccessories)
			m_animWeaponAccessories.Invoke("toggleAttachment", 1);
		else if(m_pModalHUD == &m_animQuickMenu && IsQuickMenuButtonActive(EQM_ARMOR) && !IsQuickMenuButtonDefect(EQM_ARMOR))
		{
			OnQuickMenuDefensePreset();
			m_animQuickMenu.Invoke("makeBlink", EQM_ARMOR);
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_select2)
	{
		if(m_animRadioButtons.GetVisible()) return false;
		if(m_pModalHUD == &m_animWeaponAccessories)
			m_animWeaponAccessories.Invoke("toggleAttachment", 2);
		else if(m_pModalHUD == &m_animQuickMenu && IsQuickMenuButtonActive(EQM_SPEED) && !IsQuickMenuButtonDefect(EQM_SPEED))
		{
			OnQuickMenuSpeedPreset();
			m_animQuickMenu.Invoke("makeBlink", EQM_SPEED);
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_select3)
	{
		if(m_animRadioButtons.GetVisible()) return false;
		if(m_pModalHUD == &m_animWeaponAccessories)
			m_animWeaponAccessories.Invoke("toggleAttachment", 3);
		else if(m_pModalHUD == &m_animQuickMenu && IsQuickMenuButtonActive(EQM_STRENGTH) && !IsQuickMenuButtonDefect(EQM_STRENGTH))
		{
			OnQuickMenuStrengthPreset();
			m_animQuickMenu.Invoke("makeBlink", EQM_STRENGTH);
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_select4)
	{
		if(m_animRadioButtons.GetVisible()) return false;
		if(m_pModalHUD == &m_animWeaponAccessories)
			m_animWeaponAccessories.Invoke("toggleAttachment", 4);
		else if(m_pModalHUD == &m_animQuickMenu && IsQuickMenuButtonActive(EQM_CLOAK) && !IsQuickMenuButtonDefect(EQM_CLOAK))
		{
			OnCloak();
			m_animQuickMenu.Invoke("makeBlink", EQM_CLOAK);
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_select5)
	{
		if(m_animRadioButtons.GetVisible()) return false;
		if(m_pModalHUD == &m_animWeaponAccessories)
			m_animWeaponAccessories.Invoke("toggleAttachment", 5);
		else if(m_pModalHUD == &m_animQuickMenu && IsQuickMenuButtonActive(EQM_WEAPON) && !IsQuickMenuButtonDefect(EQM_WEAPON))
		{
			OnAction(rGameActions.hud_suit_menu, eIS_Released, 1);
			HandleFSCommand("QuickMenuSwitchWeaponAccessories", NULL);
			m_animQuickMenu.Invoke("makeBlink", EQM_WEAPON);
		}
		filterOut = false;
	}
	else if(action == rGameActions.hud_mptutorial_disable)
	{
		if(gEnv->pSystem->GetIInput()->GetModifiers() & eMM_Ctrl)
		{
			if(CMPTutorial* pTutorial = g_pGame->GetGameRules()->GetMPTutorial())
			{
				if(pTutorial->IsEnabled())
				{
					gEnv->pConsole->ExecuteString("g_PSTutorial_Enabled 0");
					g_pGame->GetOptions()->SaveCVarToProfile("Option.g_PSTutorial_Enabled", "0");
				}
			}
		}
	}
	
	if (m_pHUDTweakMenu)
		m_pHUDTweakMenu->OnActionTweak(action.c_str(), activationMode, value);

	return filterOut;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowObjectives(bool bShow)
{
	m_animObjectivesTab.SetVisible(bShow);
	if(bShow)
	{
		PlaySound(ESound_OpenPopup);
		m_animObjectivesTab.Invoke("updateContent");
	}
	else
	{
		PlaySound(ESound_ClosePopup);
	}
	HUD_CALL_LISTENERS(OnShowObjectives(bShow));

	if(gEnv->bMultiplayer && m_currentGameRules == EHUD_POWERSTRUGGLE)
	{
		m_animBattleLog.SetVisible(!bShow);
	}
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::ShowPDA(bool show, bool buyMenu)
{
	if(show && !m_bShow) //don't display map if hud is disabled
		return false;

	if(gEnv->bMultiplayer && m_currentGameRules == EHUD_POWERSTRUGGLE)
	{
		if(!buyMenu)
			ShowObjectives(show);
		else if(!show)
			ShowObjectives(false);
	}

	if(buyMenu)
	{
		if(!m_animBuyMenu.IsLoaded())
			return false;
		if(m_pModalHUD == &m_animPDA)
			ShowPDA(false, false);

		// call listeners
		FlashRadarType type = EFirstType;
		if(m_pHUDPowerStruggle->m_currentBuyZones.size() > 0)
		{
			IEntity* pFactory = gEnv->pEntitySystem->GetEntity(m_pHUDPowerStruggle->m_currentBuyZones[0]);
			if(pFactory)
			{
				type = m_pHUDRadar->ChooseType(pFactory);
			}
		}

		HUD_CALL_LISTENERS(OnBuyMenuOpen(show,type));
	}
	else
	{
		if(!m_animPDA.IsLoaded())
			return false;
		if(m_pModalHUD == &m_animBuyMenu)
			ShowPDA(false, true);

		HUD_CALL_LISTENERS(OnMapOpen(show));
	}

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	CWeapon *pWeapon = GetWeapon();
	if(pWeapon)
		pWeapon->StopFire(pActor->GetEntityId());

	if (show && m_pModalHUD == NULL)
	{
		if(pActor->GetHealth() <= 0 && !gEnv->bMultiplayer)
			return false;

		if (IsModalHUDAvailable())
			SwitchToModalHUD(buyMenu?&m_animBuyMenu:&m_animPDA,true);

		CGameFlashAnimation *anim = buyMenu?&m_animBuyMenu:&m_animPDA;
		anim->SetVisible(true);

		if(!buyMenu)
		{
			m_pHUDRadar->SetRenderMapOverlay(true);
			anim->Invoke("setDisconnect", m_bNoMiniMap);
			CGameRules* pRules = g_pGame->GetGameRules();
			if(pRules && pRules->GetTeamId("black") == pRules->GetTeam(pActor->GetEntityId()))
				anim->SetVariable("PlayerTeam",SFlashVarValue("US"));
			else
				anim->SetVariable("PlayerTeam",SFlashVarValue("KOREAN"));
			CPlayer *pPlayer = static_cast<CPlayer *>(pActor);
			if(pPlayer)
			{
				int team = g_pGame->GetGameRules()->GetTeam(pPlayer->GetEntityId());
				if (!team)
					anim->SetVariable("SpectatorMode",SFlashVarValue(true));
				else
					anim->SetVariable("SpectatorMode",SFlashVarValue(false));
			}
			anim->SetVariable("GameRules",SFlashVarValue(g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName()));
		}
		else	
		{
			//m_animBuyZoneIcon.SetVisible(m_pHUDPowerStruggle->m_bInBuyZone&&m_pModalHUD!=&m_animBuyMenu);
			m_iCatchBuyMenuPage = -1;
		}

		anim->Invoke("showPDA", gEnv->bMultiplayer);

		m_pHUDPowerStruggle->HideSOM(true);

		CPlayer *pPlayer = static_cast<CPlayer *>(pActor);
		if(pPlayer->GetPlayerInput())
			pPlayer->GetPlayerInput()->DisableXI(true);

		PlaySound(ESound_OpenPopup);
		HUD_CALL_LISTENERS(PDAOpened());

		return true;
	}
	else if(!show && ((buyMenu)?m_pModalHUD == &m_animBuyMenu:m_pModalHUD == &m_animPDA))
	{
		SwitchToModalHUD(NULL,false);
		if (CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor()))
			if (IPlayerInput * pPlayerInput = pPlayer->GetPlayerInput())
				pPlayerInput->DisableXI(false);

		if(buyMenu)
		{
			const SFlashVarValue cVal(gEnv->bMultiplayer?"MP":"SP");
			m_animBuyMenu.Invoke("hidePDA",&cVal,1);
			m_iCatchBuyMenuPage = -2;
			//m_animBuyZoneIcon.SetVisible(m_pHUDPowerStruggle->m_bInBuyZone&&m_pModalHUD!=&m_animBuyMenu);
		}
		else
		{
			m_animPDA.SetVisible(false);
			m_pHUDRadar->SetRenderMapOverlay(false);
			m_pHUDRadar->SetDrag(false);
		}

		m_pHUDPowerStruggle->HideSOM(false);

		PlaySound(ESound_ClosePopup);
		HUD_CALL_LISTENERS(PDAClosed());

		return false;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent)
{
	if(!m_iCursorVisibilityCounter)
	{
		return;
	}

	if(HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK == eHardwareMouseEvent)
	{
		if (m_pModalHUD)
		{
			m_pModalHUD->CheckedInvoke("DoubleClick");
		}
	}

	SFlashCursorEvent::ECursorState eCursorState = SFlashCursorEvent::eCursorMoved;
	if(HARDWAREMOUSEEVENT_LBUTTONDOWN == eHardwareMouseEvent)
	{
		eCursorState = SFlashCursorEvent::eCursorPressed;
	}
	else if(HARDWAREMOUSEEVENT_LBUTTONUP == eHardwareMouseEvent)
	{
		eCursorState = SFlashCursorEvent::eCursorReleased;
	}

	if (m_pModalHUD)
	{
		int x(iX), y(iY);
		m_pModalHUD->GetFlashPlayer()->ScreenToClient(x,y);
		m_pModalHUD->GetFlashPlayer()->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
	}

	// Note: this one is special, it overrides all the others
	if(m_animScoreBoard.IsLoaded())
	{
		int x(iX), y(iY);
		m_animScoreBoard.GetFlashPlayer()->ScreenToClient(x,y);
		m_animScoreBoard.GetFlashPlayer()->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::GetGPSPosition(SMovementState *pMovementState,char *strN,char *strW)
{
	float fMultiplier = 100.0f;

	// Note: this is in Hawa�:-)

	int iN = 21291648		+ (int) (pMovementState->eyePosition.x * fMultiplier);
	int iW = 157591231	+ (int) (pMovementState->eyePosition.y * fMultiplier);

	// FIXME: I don't remember which one, but I'm pretty sure there is a function to do that properly

	int iN1 = iN / 1000000;
	int iN2 = (iN - iN1 * 1000000) / 10000;
	int iN3 = (iN - iN1 * 1000000 - iN2 * 10000) / 100;
	int iN4 = (iN - iN1 * 1000000 - iN2 * 10000 - iN3 * 100);

	int iW1 = iW / 1000000;
	int iW2 = (iW - iW1 * 1000000) / 10000;
	int iW3 = (iW - iW1 * 1000000 - iW2 * 10000) / 100;
	int iW4 = (iW - iW1 * 1000000 - iW2 * 10000 - iW3 * 100);

	sprintf(strN,"%.2d\"%.2d'%.2d.%.2d N",iN1,iN2,iN3,iN4);
	sprintf(strW,"%.2d\"%.2d'%.2d.%.2d W",iW1,iW2,iW3,iW4);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::TickBattleStatus(float fValue)
{		
	m_fBattleStatus+=fValue; 
	m_fBattleStatusDelay=g_pGameCVars->g_combatFadeTimeDelay;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateBattleStatus()
{
	float delta = gEnv->pTimer->GetFrameTime();
	m_fBattleStatusDelay-= delta;
	if (m_fBattleStatusDelay<=0)
	{	
		m_fBattleStatus-= delta/(g_pGameCVars->g_combatFadeTime+1.0f);				
		m_fBattleStatusDelay=0;
	}
	m_fBattleStatus=CLAMP(m_fBattleStatus,0.0f,1.0f);		
}

//-----------------------------------------------------------------------------------------------------

float CHUD::GetBattleRange()
{
	return g_pGameCVars->g_battleRange;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnPostUpdate(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(),PROFILE_GAME);

	int width = gEnv->pRenderer->GetWidth();
	int height = gEnv->pRenderer->GetHeight();
	if(width != m_width || height != m_height)
	{
		UpdateRatio();
	}

	// --BEGIN TEMPORARY UNTIL WE GET A PROPER FLASH HUD
	if (gEnv->bMultiplayer)
	{
		if (CGameRules *pGameRules=g_pGame->GetGameRules())
		{
			if (pGameRules->IsRoundTimeLimited() && !stricmp(pGameRules->GetEntity()->GetClass()->GetName(), "TeamAction"))
			{
				IEntityScriptProxy *pScriptProxy=static_cast<IEntityScriptProxy *>(pGameRules->GetEntity()->GetProxy(ENTITY_PROXY_SCRIPT));
				if (pScriptProxy)
				{
					bool preround=false;
					int remainingTime=-1;
					if (!stricmp(pScriptProxy->GetState(), "InGame"))
					{
						remainingTime = (int)(pGameRules->GetRemainingRoundTime());
					}
					else if (!stricmp(pScriptProxy->GetState(), "PreRound"))
					{
						preround=true;
						remainingTime = (int)(pGameRules->GetRemainingPreRoundTime());
					}

					if (remainingTime>-1)
					{
						int minutes = (remainingTime)/60;
						int seconds = (remainingTime - minutes*60);
						string msg;
						msg.Format("%02d:%02d", minutes, seconds);

						Vec3 color=Vec3(1.0f, 1.0f, 1.0f);

						if (remainingTime<g_pGameCVars->g_suddendeathtime || preround)
						{
							float t=fabsf(cry_sinf(gEnv->pTimer->GetCurrTime()*2.5f));
							Vec3 red=Vec3(0.85f, 0.0f, 0.0f);

							color=color*(1.0f-t)+red*t;
						}

						m_pUIDraw->DrawText(m_pDefaultFont,0,12,22,22,msg.c_str(),0.85f,color.x,color.y,color.z,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
					}
					int key0;
					int nkScore = 0;
					int usScore = 0;
					IScriptTable *pGameRulesScript=pGameRules->GetEntity()->GetScriptTable();
					if (pGameRulesScript && pGameRulesScript->GetValue("TEAMSCORE_TEAM0_KEY", key0))
					{
						pGameRules->GetSynchedGlobalValue(key0+1, nkScore);
						pGameRules->GetSynchedGlobalValue(key0+2, usScore);
					}
					if (pGameRules->IsRoundTimeLimited() && !stricmp(pGameRules->GetEntity()->GetClass()->GetName(), "TeamAction"))
					{
						IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
						if(!pClientActor)
							return;
						int clientTeam = pGameRules->GetTeam(pClientActor->GetEntityId());

						CGameRules::TPlayers nkPlayers, usPlayers;
						pGameRules->GetTeamPlayers(1, nkPlayers);
						pGameRules->GetTeamPlayers(2, usPlayers);

						int numNK = 0;
						int numUS = 0;
						for(int i=0; i<nkPlayers.size(); ++i)
						{
							IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(nkPlayers[i]);
							if(pActor && pActor->GetHealth() > 0)
								++numNK;
						}
						for(int i=0; i<usPlayers.size(); ++i)
						{
							IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(usPlayers[i]);
							if(pActor && pActor->GetHealth() > 0)
								++numUS;
						}

						if(clientTeam != 0)
						{
							string nkPlayers;
							string usPlayers;
							string nkScoreText;
							string usScoreText;
							for(int i=0; i<nkScore; ++i)
								nkScoreText += "*";
							for(int i=0; i<usScore; ++i)
								usScoreText += "*";

							if(clientTeam == 1)
							{
								nkPlayers.Format("NK: %d", numNK);
								usPlayers.Format("%d :US", numUS);
								m_pUIDraw->DrawText(m_pDefaultFont, -40, 5, 22, 22, nkScoreText.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_RIGHT,UIDRAWVERTICAL_TOP);
								m_pUIDraw->DrawText(m_pDefaultFont, 40, 5, 22, 22, usScoreText.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
								m_pUIDraw->DrawText(m_pDefaultFont, -40, 12, 22, 22, nkPlayers.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_RIGHT,UIDRAWVERTICAL_TOP);
								m_pUIDraw->DrawText(m_pDefaultFont, 40, 12, 22, 22, usPlayers.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
							}
							else
							{
								nkPlayers.Format("%d :NK", numNK);
								usPlayers.Format("US: %d", numUS);
								m_pUIDraw->DrawText(m_pDefaultFont, -40, 5, 22, 22, usScoreText.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_RIGHT,UIDRAWVERTICAL_TOP);
								m_pUIDraw->DrawText(m_pDefaultFont, 40, 5, 22, 22, nkScoreText.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
								m_pUIDraw->DrawText(m_pDefaultFont, -40, 12, 22, 22, usPlayers.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_RIGHT,UIDRAWVERTICAL_TOP);
								m_pUIDraw->DrawText(m_pDefaultFont, 40, 12, 22, 22, nkPlayers.c_str(), 0.85f, 1.0f, 1.0f, 1.0f, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
							}
						}
					}
				}
			}
		}
		if(m_animSpectate.IsLoaded())
		{
			m_animSpectate.GetFlashPlayer()->Advance(frameTime);
			m_animSpectate.GetFlashPlayer()->Render();
		}
	}
	// --END TEMPORARY UNTIL WE GET A PROPER FLASH HUD

	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());

	/*if(pPlayer)
	{
		Vec2i gridPos = m_pHUDRadar->GetMapGridPosition(pPlayer->GetEntity()->GetWorldPos().x, pPlayer->GetEntity()->GetWorldPos().y);
		CryLogAlways("%i %i", gridPos.x, gridPos.y);
		char text[256];
		sprintf(text, "Grid Position: %i - %i", gridPos.x, gridPos.y);
		DisplayFlashMessage(text, 2);
	}*/

	if (g_pGameCVars->cl_hud <= 0 || m_cineHideHUD)
	{
		// possibility to show the binoculars during cutscene
		if(m_cineHideHUD && m_pHUDScopes->IsBinocularsShown() && m_pHUDScopes->m_bShowBinocularsNoHUD && pPlayer && m_pHUDScopes->m_animBinoculars.GetVisible())
		{
			m_pHUDScopes->DisplayBinoculars(pPlayer);
			m_pHUDScopes->m_animBinoculars.GetFlashPlayer()->Advance(frameTime);
			m_pHUDScopes->m_animBinoculars.GetFlashPlayer()->Render();
		}

		// Even if HUD is off Fader must be able to function if cl_hud is 0.
		if (!m_externalHUDObjectList.empty() && (g_pGameCVars->cl_hud == 0 || m_cineHideHUD))
		{
			m_pUIDraw->PreRender();
			for(THUDObjectsList::iterator iter=m_externalHUDObjectList.begin(); iter!=m_externalHUDObjectList.end(); ++iter)
			{
				(*iter)->Update(frameTime);
			}
			m_pUIDraw->PostRender();
		}

		UpdateCinematicAnim(frameTime);
		UpdateSubtitlesAnim(frameTime);

		return;
	}

	if(!gEnv->pGame->GetIGameFramework()->IsGameStarted() || !pPlayer)
	{
		// Modal dialog box must be always rendered last
		UpdateWarningMessages(frameTime);
		return;
	}

	//Updates all timer-based triggers
	UpdateTimers(frameTime);

	if(m_iOpenTextChat)
	{
		if(GetMPChat())
			GetMPChat()->OpenChat(m_iOpenTextChat);
		m_iOpenTextChat = 0;
	}

	if(gEnv->bMultiplayer)
	{
		int pp = m_pHUDPowerStruggle->GetPlayerPP();
		if(m_lastPlayerPPSet != pp)
		{
			m_animPlayerPP.Invoke("setPPoints", pp);
			m_lastPlayerPPSet = pp;
		}

		if(m_pModalHUD == &m_animBuyMenu)
			m_animBuyMenu.Invoke("setPP", pp);
	}

	if(m_pModalHUD == &m_animQuickMenu)
	{
		SPlayerStats *stats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
		CWeapon *pWeapon = GetWeapon();
		if((stats && (stats->bSprinting || (pPlayer->GetStance() == STANCE_PRONE && stats->speedFlat > 0.01f))) || (pWeapon && pWeapon->IsBusy()))	
		{
			if(IsQuickMenuButtonActive(EQM_WEAPON))	//we are sprinting, but the weapon button is not disabled yet
				ActivateQuickMenuButton(EQM_WEAPON, false);
		}
		else //we disabled the weapon button, but stopped sprinting
			ActivateQuickMenuButton(EQM_WEAPON,m_bHasWeaponAttachments);
	}

	if(!m_bInMenu && m_bHUDInitialize)
	{
		if(m_iFade)
		{
			m_iFade -= 1;
			if(m_iFade > 0 && m_iFade <= 50)
			{
				m_bShow = true;
				m_fAlpha = (50 - m_iFade) / 50.0f;
			}
			else if(m_iFade > 50)
			{
				m_fAlpha = 0.0f;
				m_bShow = false;
			}
		}
	}

	if(!m_bInMenu)
	{
		//////////////////////////////////////////////////////////////////////////
		// update internals
		UpdateBattleStatus();
		//////////////////////////////////////////////////////////////////////////
	}

	if(m_bFirstFrame)
	{
		// FIXME: remove setalpha(0) from all files and remove this block
		for(TGameFlashAnimationsList::iterator iter=m_gameFlashAnimationsList.begin(); iter!=m_gameFlashAnimationsList.end(); ++iter)
		{
			CGameFlashAnimation *pAnim = (*iter);
			if (!(pAnim->GetFlags() & eFAF_ManualRender))
				pAnim->CheckedInvoke("setAlpha", 1.0f);
		}
	}

	if(m_bHUDInitialize)
	{
		if(!m_bInMenu && m_animInitialize.GetVisible())
		{
			m_animInitialize.GetFlashPlayer()->Advance(frameTime);
			m_animInitialize.GetFlashPlayer()->Render();
		}
	}
	else
	{
		if(m_bBreakHUD)
		{
			if(!m_animBreakHUD.IsLoaded())
				m_animBreakHUD.Load("Libs/UI/HUD_Lost.gfx", eGFD_Center, eFAF_ManualRender|eFAF_Visible);
			m_animBreakHUD.GetFlashPlayer()->Advance(frameTime);
			m_animBreakHUD.GetFlashPlayer()->Render();
		}
		else if(!m_bBreakHUD)
		{
			if(gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_fLastReboot < 5.0f)
			{
				if(!m_animRebootHUD.IsLoaded())
					m_animRebootHUD.Load("Libs/UI/HUD_Reboot.gfx", eGFD_Center, eFAF_ManualRender);

				m_animRebootHUD.GetFlashPlayer()->Advance(frameTime);
				m_animRebootHUD.GetFlashPlayer()->Render();
			}
			else if(m_fLastReboot)
			{
				m_animRebootHUD.Unload();
				m_animBreakHUD.Unload();
				m_fLastReboot = 0.0f;
			}
		}
	}

	if(m_pHUDScopes->GetCurrentScope() != CHUDScopes::ESCOPE_NONE)
	{
		CWeapon *pWeapon = GetWeapon();
		if(pWeapon && pPlayer)
		{
			IGameTokenSystem *pGameTokenSystem = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();

			//			CRY_ASSERT_MESSAGE(0,"Not sure that this works in something else than 4/3 aspect ratio!");
			float fScaleX = 8.0f * 1024.0f / 800.0f;
			float fScaleY = 6.0f * 768.0f / 600.0f;

			Vec3 vWorldPos;
			Vec3 vScreenSpace;

			const CItem::THelperVector& helpers = pWeapon->GetAttachmentHelpers();

			int zoommode = m_pHUDScopes->m_oldScopeZoomLevel;

			CItem::SAttachmentHelper helper;
			helper.bone = "attachment_top";
			helper.name = "attachment_top";
			helper.slot = 0;
			vWorldPos = pWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, helper.bone,true);
			if (IMovementController *pMV = pPlayer->GetMovementController())
			{
				SMovementState state;
				pMV->GetMovementState(state);
				vWorldPos += state.eyeDirection * 0.5f;
			}
				
			m_pRenderer->ProjectToScreen(vWorldPos.x,vWorldPos.y,vWorldPos.z,&vScreenSpace.x,&vScreenSpace.y,&vScreenSpace.z);

			float modifier = 1.0f;

			//Scope Offsets

			IEntityClass *weaponClass = pWeapon->GetEntity()->GetClass();

			if(m_pLAW == weaponClass)
			{
				Vec3 vCenter(50.0f,99.759758f,1.0f);
				vScreenSpace -= vCenter;
			}
			else if(m_pGauss == weaponClass)
			{
				if(zoommode==2)
				{
					modifier = 0.5f;
					Vec3 vCenter(48.943108f,101.778900f,1.0f);
					vScreenSpace -= vCenter;
				}
				else if(zoommode==1)
				{
					Vec3 vCenter(49.553017f,70.608650f,1.0f);
					vScreenSpace -= vCenter;
				}
			}
			else if(m_pFY71 == weaponClass)
			{
				if(zoommode==2)
				{
					modifier = 0.5f;
					Vec3 vCenter(50.053f,81.929520f,1.0f);
					vScreenSpace -= vCenter;
				}
				else if(zoommode==1)
				{
					Vec3 vCenter(50.007057f,62.7056f,1.0f);
					vScreenSpace -= vCenter;
				}
			}
			else if(m_pSMG == weaponClass)
			{
				if(zoommode==2)
				{
					modifier = 0.5f;
					Vec3 vCenter(49.217724f,80.769501f,1.0f);
					vScreenSpace -= vCenter;
				}
				else if(zoommode==1)
				{
					Vec3 vCenter(49.707661f,62.248619f,1.0f);
					vScreenSpace -= vCenter;
				}
			}
			else
			{
				if(zoommode==2)
				{
					modifier = 0.5f;
					Vec3 vCenter(50.601f,77.878f,1.0f);
					vScreenSpace -= vCenter;
				}
				else if(zoommode==1)
				{
					Vec3 vCenter(50.25f,61.10f,1.0f);
					vScreenSpace -= vCenter;
				}
			}
			
			float maxmove = m_pRenderer->GetHeight()*1.0f;

			float x = min(vScreenSpace.x*50.0f*modifier, maxmove);
			x = max(x, -maxmove);
			float y = min(vScreenSpace.y*50.0f*modifier, maxmove);
			y = max(y, -maxmove);

			m_pHUDScopes->m_animSniperScope.SetVariable("Root2._x",SFlashVarValue(x));
			m_pHUDScopes->m_animSniperScope.SetVariable("Root2._y",SFlashVarValue(y));

			x = min(vScreenSpace.x*10.0f*modifier, maxmove*0.142f);
			x = max(x, -maxmove*0.2f);
			y = min(vScreenSpace.y*10.0f*modifier, maxmove*0.142f);
			y = max(y, -maxmove*0.2f);

			m_pHUDScopes->m_animSniperScope.SetVariable("Root._x",SFlashVarValue(x));
			m_pHUDScopes->m_animSniperScope.SetVariable("Root._y",SFlashVarValue(y));
		}
	}

	if(m_bShow && pPlayer && !m_bInMenu)
	{
		// Weapon
		CWeapon *pWeapon = GetWeapon();
		if(pWeapon && pWeapon->IsModifying())
		{
			IGameTokenSystem *pGameTokenSystem = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();

//			CRY_ASSERT_MESSAGE(0,"Not sure that this works in something else than 4/3 aspect ratio!");
			float fScaleX = 8.0f * 1024.0f / 800.0f;
			float fScaleY = 6.0f * 768.0f / 600.0f;

			Vec3 vWorldPos;
			Vec3 vScreenSpace;

			bool b169 = m_pRenderer->GetWidth() == m_pRenderer->GetHeight() * 16.0f / 9.0f;

			char tempBuf[HUD_MAX_STRING_SIZE];

			const CItem::THelperVector& helpers = pWeapon->GetAttachmentHelpers();

			for (int i = 0; i < helpers.size(); i++)
			{
				CItem::SAttachmentHelper helper = helpers[i];
				if (helper.slot != CItem::eIGS_FirstPerson)
					continue;

				vWorldPos = pWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, helper.bone,true);
				m_pRenderer->ProjectToScreen(vWorldPos.x,vWorldPos.y,vWorldPos.z,&vScreenSpace.x,&vScreenSpace.y,&vScreenSpace.z);

				vScreenSpace.x = b169 ? ((vScreenSpace.x * fScaleX - 128.0f) / 0.75f) : (vScreenSpace.x * fScaleX);
				vScreenSpace.y *= fScaleY;

				if(!strcmp(helper.name,"silencer_attach"))
				{
					vScreenSpace.x -=50;
					vScreenSpace.y -=60;
				}

				if(!strcmp(helper.name,"magazine"))
				{
					vScreenSpace.x -=90;
					vScreenSpace.y +=25;
				}

				if(!strcmp(helper.name,"attachment_bottom"))
				{
					vScreenSpace.x -=30;
					vScreenSpace.y +=20;
				}

				if(!strcmp(helper.name,"attachment_front"))
				{
					vScreenSpace.x -=90;
					vScreenSpace.y -=200;
				}

				_snprintf(tempBuf, sizeof(tempBuf), "hud.WS%sX", helper.name.c_str());
				tempBuf[sizeof(tempBuf)-1] = '\0';
				pGameTokenSystem->SetOrCreateToken(tempBuf,TFlowInputData(vScreenSpace.x,true));
				_snprintf(tempBuf, sizeof(tempBuf), "hud.WS%sY", helper.name.c_str());
				tempBuf[sizeof(tempBuf)-1] = '\0';
				pGameTokenSystem->SetOrCreateToken(tempBuf,TFlowInputData(vScreenSpace.y,true));
			}
		}

		UpdateVoiceChat();

		// Target autoaim and locking
		Targetting(0, false);

		UpdateHealth();

		// Grenade detector
		GrenadeDetector(pPlayer);

		// Binoculars and Scope
		if(m_pHUDScopes->IsBinocularsShown())
			m_pHUDScopes->DisplayBinoculars(pPlayer);
		else if(m_pHUDScopes->GetCurrentScope()>=0)
			m_pHUDScopes->DisplayScope(pPlayer);

		if(m_pHUDVehicleInterface->GetHUDType()!=CHUDVehicleInterface::EHUD_NONE)
			m_pHUDVehicleInterface->ShowVehicleInterface(m_pHUDVehicleInterface->GetHUDType());

		// Scopes should be the very first thing to be drawn
		m_pHUDScopes->OnUpdate(frameTime,0);

		//*****************************************************

		//render flash animation
		{
			bool bInterference = false;
			if(m_distortionStrength || m_displacementStrength || m_alphaStrength)
			{
				bInterference = true;
				CreateInterference();
			}
			for(TGameFlashAnimationsList::iterator iter=m_gameFlashAnimationsList.begin(); iter!=m_gameFlashAnimationsList.end(); ++iter)
			{
				CGameFlashAnimation *pAnim = (*iter);
				if (pAnim->GetFlags() & eFAF_ManualRender)
					continue;

				if(pAnim->GetVisible())
				{
					if(bInterference)
					{
						RepositionFlashAnimation(pAnim);
					}
					pAnim->GetFlashPlayer()->Advance(frameTime);
					pAnim->GetFlashPlayer()->Render();
				}
			}
		}

		// Autosnap
		if(m_bAutosnap)
			AutoSnap();

		// Non flash stuff

		for(THUDObjectsList::iterator iter=m_hudObjectsList.begin(); iter!=m_hudObjectsList.end(); ++iter)
		{
			(*iter)->PreUpdate();
		}

		m_pUIDraw->PreRender();

		for(THUDObjectsList::iterator iter=m_hudObjectsList.begin(); iter!=m_hudObjectsList.end(); ++iter)
		{
			(*iter)->Update(frameTime);
		}

		if(m_bShowGODMode && strcmp(m_strGODMode,""))
		{
			int fading = g_pGameCVars->hud_godFadeTime;
			float time = gEnv->pTimer->GetAsyncTime().GetSeconds();
			if(fading == -1 || (time - m_fLastGodModeUpdate < fading))
			{
				float alpha = 0.75f;
				if(fading >= 2)
				{
					if(time - m_fLastGodModeUpdate < 0.75f)		//fade in
						alpha = (time - m_fLastGodModeUpdate);
					else if(time - m_fLastGodModeUpdate > (float(fading)-0.75f))		//fade out
						alpha -= ((time - m_fLastGodModeUpdate) - (float(fading)-0.75f));
				}
				m_pUIDraw->DrawText(m_pDefaultFont,10,60,0,0,m_strGODMode,alpha,1,1,1,UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
				//debugging : render number of deaths in God mode ...
				if(!strcmp(m_strGODMode,"GOD") || !strcmp(m_strGODMode,"Team GOD"))
				{
					string died("You died ");
					char aNumber[5];
					itoa(m_iDeaths, aNumber, 10);
					died.append(aNumber);
					died.append(" times.");
					m_pUIDraw->DrawText(m_pDefaultFont,10,80,0,0,died.c_str(),alpha,1,1,1,UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
				}
			}
		}

		m_pUIDraw->PostRender();
	}
	else if(!m_bInMenu && pPlayer)
	{
		// TODO: add to list and use GetVisible()
		if(m_animScoreBoard.IsLoaded())
		{
			if(!m_bShow && (GetModalHUD() == &m_animScoreBoard))	//render sniper scope also when in scoreboard mode
			{
				if(m_pHUDScopes->GetCurrentScope() == CHUDScopes::ESCOPE_SNIPER)
				{
					m_pHUDScopes->DisplayScope(pPlayer);
					m_pHUDScopes->OnUpdate(frameTime, 0);
				}
			}

			if(gEnv->bMultiplayer)
			{
				int iRemainingTime = (int)(g_pGame->GetGameRules()->GetRemainingGameTime());
				int minutes = (iRemainingTime)/60;
				int seconds = (iRemainingTime - minutes*60);
				string msg = "00:00";
				msg.Format("%02d:%02d", minutes, seconds);
				m_animScoreBoard.Invoke("setCountdown", msg.c_str());
			}
			m_animScoreBoard.GetFlashPlayer()->Advance(frameTime);
			m_animScoreBoard.GetFlashPlayer()->Render();
			m_pHUDScore->Render();
		}
	}

	if(m_animTargetAutoAim.GetVisible())
	{
		m_animTargetAutoAim.GetFlashPlayer()->Advance(frameTime);
		m_animTargetAutoAim.GetFlashPlayer()->Render();
	}

	if (!m_bInMenu && !m_externalHUDObjectList.empty())
	{
		m_pUIDraw->PreRender();
		for(THUDObjectsList::iterator iter=m_externalHUDObjectList.begin(); iter!=m_externalHUDObjectList.end(); ++iter)
		{
			(*iter)->Update(frameTime);
		}
		m_pUIDraw->PostRender();
	}

	if(m_bInMenu && pPlayer)
	{
		if(m_pHUDScopes->GetCurrentScope() == CHUDScopes::ESCOPE_SNIPER)
		{
			m_pHUDScopes->DisplayScope(pPlayer);
			m_pHUDScopes->OnUpdate(frameTime, 0);
		}
	}

	// update cinematic bars
	UpdateCinematicAnim(frameTime);

	// update subtitles 
	UpdateSubtitlesAnim(frameTime);

	if(m_bFirstFrame)
	{
		EnergyChanged(m_pNanoSuit->GetSuitEnergy());
		m_bFirstFrame = false;
	}
	m_fSuitEnergy = m_pNanoSuit->GetSuitEnergy();
	m_iVoiceMode = g_pGameCVars->hud_voicemode;

	// MARCIO: temporary informative message until there is a spectator HUD
	if (pPlayer)
	{
		uint8 specMode = pPlayer->GetSpectatorMode();
		if (specMode >= CActor::eASM_FirstMPMode && specMode <= CActor::eASM_LastMPMode)
		{
			m_pUIDraw->DrawText(m_pDefaultFont, 0, 1, 28, 28, "SPECTATING", 1,1,1,1,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
			if(specMode != CActor::eASM_Follow)
				m_pUIDraw->DrawText(m_pDefaultFont, 0, -46, 16, 16, "PRESS M TO OPEN YOUR MAP AND JOIN A TEAM", 1,1,1,1,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_BOTTOM,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
		}
	}

	// Modal dialog box must be always rendered last
	UpdateWarningMessages(frameTime);

#ifdef USE_G15_LCD
	m_pLCD->Update(frameTime);
#endif//USE_G15_LCD
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnSaveGame(ISaveGame* pSaveGame)
{
	if(pSaveGame->GetSaveGameReason() == eSGR_FlowGraph)
		DisplayFlashMessage("@checkpoint", 2);
	else
		DisplayFlashMessage("@game_saved", 2);
	const char* file = pSaveGame->GetFileName();
	if(file)
		m_sLastSaveGame = file;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnActionEvent(const SActionEvent& event)
{
}

//-----------------------------------------------------------------------------------------------------

void CHUD::BreakHUD(int state)
{
	if(state)
	{
		m_bHUDInitialize = false;
		m_bBreakHUD = true;
	}
	else
		m_bBreakHUD = false;

	if(!m_bBreakHUD)		//remove some obsolete stuff
	{
		m_animKillAreaWarning.Unload();
		m_animDeathMessage.Unload();
	}

	for(TGameFlashAnimationsList::iterator iter=m_gameFlashAnimationsList.begin(); iter!=m_gameFlashAnimationsList.end(); ++iter)
	{
		CGameFlashAnimation *pAnim = (*iter);
		if (pAnim->GetFlags() & eFAF_ManualRender)
			continue;
		pAnim->CheckedInvoke("destroy", state);
	}
	m_animBreakHUD.Invoke("gotoAndPlay", 1);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::RebootHUD()
{
	m_bHUDInitialize = false;
	m_bBreakHUD = false;
	for(TGameFlashAnimationsList::iterator iter=m_gameFlashAnimationsList.begin(); iter!=m_gameFlashAnimationsList.end(); ++iter)
	{
		CGameFlashAnimation *pAnim = (*iter);
		if (pAnim->GetFlags() & eFAF_ManualRender)
			continue;
		pAnim->CheckedInvoke("reboot");
	}
	m_animRebootHUD.SetVisible(true);
	m_animRebootHUD.Invoke("gotoAndPlay", 1);
	m_fLastReboot = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	m_bShow = true; // make sure we show the HUD
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateHealth()
{
	CActor *pActor = static_cast<CActor *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pActor)
	{
		float fHealth = (pActor->GetHealth() / (float) pActor->GetMaxHealth()) * 100.0f + 1.0f;

		if(m_fHealth != fHealth || m_bFirstFrame)
		{
			m_animHealthEnergy.Invoke("setHealth", (int)fHealth);
		}

		if(m_bFirstFrame)
			m_fHealth = fHealth;

		m_fHealth = fHealth;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateTimers(float frameTime)
{
	CTimeValue now = gEnv->pTimer->GetFrameStartTime();

	if(m_fPlayerDeathTime && (m_sLastSaveGame.size() || g_pGame->GetLastSaveGame().size()) && m_animWarningMessages.IsLoaded())
	{

		if(g_pGame->GetMenu()->IsActive())
			m_fPlayerDeathTime += frameTime;
		else
		{
			float diff = now.GetSeconds() - m_fPlayerDeathTime;
			if(diff > 15.0f)
			{
				m_fPlayerDeathTime = 0.0f;
				g_pGame->GetMenu()->ShowInGameMenu(false);
				if(m_sLastSaveGame.size())
					g_pGame->GetIGameFramework()->LoadGame(m_sLastSaveGame.c_str());
				else
					g_pGame->GetIGameFramework()->LoadGame(g_pGame->GetLastSaveGame().c_str());

				m_animWarningMessages.GetFlashPlayer()->SetVisible(false);
				return;
			}
			else if(diff > 3.0f)
			{
				char seconds[4];
				sprintf(seconds,"%i",(int)(fabsf(15.0f-diff)));
				//const wchar_t* localizedText = L"";
				//localizedText = LocalizeWithParams("@ui_load_last_save", true, seconds);
				//if(!m_animWarningMessages.GetFlashPlayer()->GetVisible())
				//	m_animWarningMessages.Invoke("showErrorMessage", "Box1");
				//m_animWarningMessages.Invoke("setErrorText", localizedText);
				DisplayFlashMessage("@ui_load_last_save", 2, ColorF(0, 1.0, 0), true, seconds);
			}
		}
	}

	if(m_fPlayerRespawnTimer)
		FakeDeath(true);

	if(m_fSetAgressorIcon!=0.0f)
	{
		if(now.GetSeconds() - m_fSetAgressorIcon < 2.0f)
			SetOnScreenTargetter();
		else if(m_animTargetter.IsLoaded())
		{
			m_animTargetter.SetVisible(false);
			m_fSetAgressorIcon = 0.0f;
		}
	}

	if(m_fCatchBuyMenuTimer!=0.0f)
	{
		if(now.GetMilliSeconds() - m_fCatchBuyMenuTimer > 500)
		{
			m_fCatchBuyMenuTimer = 0.0f;
			BuyViaFlash(m_iCatchBuyMenuPage);
		}
	}

	if(m_fMiddleTextLineTimeout!=0.0f)
	{
		if(now.GetSeconds() - m_fMiddleTextLineTimeout > 0)
		{
			m_fMiddleTextLineTimeout = 0.0f;
			m_animMessages.Invoke("fadeOutMiddleLine");
		}
	}

	if(m_fSuitChangeSoundTimer != 0)
	{
		if(now.GetMilliSeconds() - m_fSuitChangeSoundTimer > 500)
		{
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			if(pPlayer && pPlayer->GetNanoSuit())
			{
				ENanoMode mode = pPlayer->GetNanoSuit()->GetMode();
				switch(mode)
				{
				case NANOMODE_SPEED:
					PlayStatusSound("maximum_speed", true);
					break;
				case NANOMODE_STRENGTH:
					PlayStatusSound("maximum_strength", true);
					break;
				case NANOMODE_DEFENSE:
					PlayStatusSound("maximum_armor", true);
					break;
				case NANOMODE_CLOAK:
					PlayStatusSound("normal_cloak_on", true);
					break;
				default:
					break;
				}
				m_fSuitChangeSoundTimer = 0;
			}
		}
	}

	if(m_fDamageIndicatorTimer && (gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_fDamageIndicatorTimer) < 2.0f)
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer)
		{
			float angle = ((pPlayer->GetAngles().z*180.0f/gf_PI)+180.0f);
			if(angle < 0.0f)
				angle = 360.0f - angle;
			m_pHUDCrosshair->GetFlashAnim()->CheckedSetVariable("DamageDirection._rotation",SFlashVarValue(angle));
		}
	}

	if(m_fSpeedTimer != 0)
	{
		if(now.GetMilliSeconds() - m_fSpeedTimer > 500)
		{
			m_fSpeedTimer = 0;
			TextMessage("maximum_speed");
		}
	}

	if(m_fStrengthTimer != 0)
	{
		if(now.GetMilliSeconds() - m_fStrengthTimer > 500)
		{
			m_fStrengthTimer = 0;
			TextMessage("maximum_strength");
		}
	}

	if(m_fDefenseTimer != 0)
	{
		if(now.GetMilliSeconds() - m_fDefenseTimer > 500)
		{
			m_fDefenseTimer = 0;
			TextMessage("maximum_armor");
		}
	}

	// FIXME: this should be moved to ::EnergyChanged
	if(m_fSuitEnergy > (NANOSUIT_ENERGY*0.25f) && m_pNanoSuit->GetSuitEnergy() < (NANOSUIT_ENERGY*0.25f))
	{
		DisplayFlashMessage("@energy_critical", 3, ColorF(1.0,0,0));
		if(now.GetMilliSeconds() - m_fLastSoundPlayedCritical > 30000)
		{
			m_fLastSoundPlayedCritical = now.GetMilliSeconds();
			PlayStatusSound("energy_critical");
		}
	}

	//check if airstrike has been done
	if(m_fAirStrikeStarted>0.0f && now.GetSeconds()-m_fAirStrikeStarted > 5.0f)
	{
		m_fAirStrikeStarted = 0.0f;
		SetAirStrikeEnabled(false);
	}

	if(m_fPlayerFallAndPlayTimer)
	{
		if(now.GetSeconds() - m_fPlayerFallAndPlayTimer > 12.5f)
		{
			CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
			if(pPlayer)
				pPlayer->StandUp();
			m_fPlayerFallAndPlayTimer = 0.0f;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateWarningMessages(float frameTime)
{
	if(!m_bInMenu && m_animWarningMessages.GetVisible())
	{
		m_animWarningMessages.GetFlashPlayer()->Advance(frameTime);
		m_animWarningMessages.GetFlashPlayer()->Render();
	}
}

//-----------------------------------------------------------------------------------------------------
// TODO: HUD is no more a GameObjectExtension, there is no meaning to use HandleEvent() anymore
//-----------------------------------------------------------------------------------------------------

void CHUD::HandleEvent(const SGameObjectEvent &rGameObjectEvent)
{
	if(rGameObjectEvent.event == eGFE_PauseGame)
	{
		// We hide the HUD during the menu
		SetInMenu(true);
	}
	else if(rGameObjectEvent.event == eGFE_ResumeGame)
	{
		// We show the HUD during the game
		SetInMenu(false);
		UpdateHUDElements();
	}
	else if(rGameObjectEvent.event == eCGE_TextArea)
	{
		m_pHUDTextArea->AddMessage((const char*)rGameObjectEvent.param);
	}
	else if(rGameObjectEvent.event == eCGE_HUD_Break)
	{
		BreakHUD();
	}
	else if(rGameObjectEvent.event == eCGE_HUD_Reboot)
	{
		RebootHUD();
	}
	else if (rGameObjectEvent.event == eCGE_HUD_TextMessage)
	{
		TextMessage((const char*) rGameObjectEvent.param);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::WeaponAccessoriesInterface(bool visible, bool force)
{
	m_acceptNextWeaponCommand = true;
	if (visible)
	{
		m_bIgnoreMiddleClick = false;
		if(m_bHasWeaponAttachments)
		{
			m_animWeaponAccessories.Invoke("showWeaponAccessories");
			m_animWeaponAccessories.SetVisible(true);
		}
	}
	else
	{
		if (!force)
		{
			if(m_animWeaponAccessories.GetVisible())
			{
				m_bIgnoreMiddleClick = true;
				m_animWeaponAccessories.Invoke("hideWeaponAccessories");
				m_animWeaponAccessories.SetVisible(false);
				PlaySound(ESound_SuitMenuDisappear);
			}
		}
		else
		{
			if(&m_animWeaponAccessories == m_pModalHUD)
				SwitchToModalHUD(NULL,false);

			m_animWeaponAccessories.Invoke("hideWeaponAccessories");
			m_animWeaponAccessories.SetVisible(false);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetFireMode(const char* name)
{
	if (!name)
		return;

	int iFireMode = 0;

	if(!strcmp(name,"Single") || !strcmp(name,"Shotgun"))
		iFireMode = 1;
	else if(!strcmp(name,"Burst"))
		iFireMode = 2;
	else if(!strcmp(name,"Automatic") || !strcmp(name,"Rapid"))
		iFireMode = 3;
	else if(!strcmp(name,"GrenadeLauncher"))
		iFireMode = 4;
	else if(!strcmp(name,"Tac Sleep"))
		iFireMode = 5;
	else if(!strcmp(name,"Tac Kill"))
		iFireMode = 6;
	else if(!strcmp(name,"Spread"))
		iFireMode = 7;
	else if(!strcmp(name,"Narrow"))
		iFireMode = 8;

	if(iFireMode)
	{
		m_animAmmo.Invoke("setFireMode", iFireMode);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetGrenade(EntityId id)
{
	m_entityGrenadeDectectorId = id;
	if(m_entityGrenadeDectectorId)
	{
		m_animGrenadeDetector.Invoke("showGrenadeDetector");
	}
	else
	{
		m_animGrenadeDetector.Invoke("hideGrenadeDetector");
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::AutoAimLocking(EntityId id)
{
	if(!IsAirStrikeAvailable())
	{
		LockTarget(id, eLT_Locking);
	}
	PlaySound(ESound_BinocularsLock);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::AutoAimNoText(EntityId id)
{
	if(!IsAirStrikeAvailable())
	{
		LockTarget(id, eLT_Locking, false);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::AutoAimLocked(EntityId id)
{
	if(!IsAirStrikeAvailable())
	{
		m_bHideCrosshair = true;
		UpdateCrosshairVisibility();
		LockTarget(id, eLT_Locked);
	}
	PlaySound(ESound_BinocularsLock);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::AutoAimUnlock(EntityId id)
{
	if(!IsAirStrikeAvailable())
	{
		m_bHideCrosshair = false;
		UpdateCrosshairVisibility();
		UnlockTarget(id);
	}
	PlaySound(ESound_BinocularsLock);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ActorDeath(IActor* pActor)
{
	if (!pActor)
		return;

	if(pActor->IsGod())
		return;

	m_pHUDRadar->RemoveFromRadar(pActor->GetEntityId());
	if(m_pModalHUD == &m_animQuickMenu)
		OnAction(g_pGame->Actions().hud_suit_menu, eIS_Released, 1);

	if(pActor == g_pGame->GetIGameFramework()->GetClientActor())
	{
    if(m_currentGameRules == EHUD_SINGLEPLAYER)
    {
			ShowPDA(false);
			ShowBuyMenu(false);
			m_fPlayerDeathTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		}
		else if(m_currentGameRules == EHUD_POWERSTRUGGLE)
		{
			ShowPDA(true);
    }

		if (m_bNightVisionActive)
			OnAction(g_pGame->Actions().hud_night_vision, eIS_Pressed, 1);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::VehicleDestroyed(EntityId id)
{
	m_pHUDRadar->RemoveFromRadar(id);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::TextMessage(const char* message)
{
	if (!message)
		return;

	//DEBUG : used for balancing
	if(!strcmp(message, "GodMode:died!"))
	{
		m_fLastGodModeUpdate = gEnv->pTimer->GetAsyncTime().GetSeconds();

		m_iDeaths++;
		m_pNanoSuit->ResetEnergy();
		return;
	}

	//find a vocal/sound to a text string ...
	const char* textMessage = 0;
	string temp;
	string statusMsg = stl::find_in_map(m_statusMessagesMap, string(message), string(""));
	if(statusMsg.size())
		textMessage = statusMsg.c_str();
	if(textMessage && strcmp(textMessage, ""))	//if known status message
	{
		PlayStatusSound(message);  //trigger vocals
		message = textMessage; //change to localized message for text rendering
	}

	if(strlen(message) > 1)
		DisplayFlashMessage(message, 3);

	//display message
	//m_onScreenText[string(message)] = gEnv->pTimer->GetCurrTime();
}
//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateObjective(CHUDMissionObjective *pObjective)
{
	const char *status;
	int colorStatus = 0;
	bool active = false;

	if(pObjective)
	{
		switch (pObjective->GetStatus())
		{
		case CHUDMissionObjective::ACTIVATED:
			status = "ACTIVATED";
			active = true;
			colorStatus = 1; //green
			if(pObjective->IsSecondary())
				colorStatus = 2; //yellow

			break;
		case CHUDMissionObjective::DEACTIVATED:
			status = "DEACTIVATED";
			break;
		case CHUDMissionObjective::COMPLETED:
			status = "COMPLETED";
			colorStatus = 4; //grey
			break;
		case CHUDMissionObjective::FAILED:
			status = "FAILED";
			colorStatus = 3; //red
			break;
		}
		string message = pObjective->GetMessage();
		string description = pObjective->GetShortDescription();

		if(message.size())
		{
			if(!pObjective->IsSilent() /*&& pObjective->GetStatus() != CHUDMissionObjective::DEACTIVATED*/)
			{
				const wchar_t* localizedText = L"";
				localizedText = LocalizeWithParams(description);
				wstring text = localizedText;
				localizedText = LocalizeWithParams(status);
				text.append(L" ");
				text.append(localizedText);
				SFlashVarValue args[3] = {text.c_str(), 1, Col_White.pack_rgb888()};
				m_animMessages.Invoke("setMessageText", args, 3);
			}

			if(pObjective->GetStatus() == CHUDMissionObjective::DEACTIVATED)
			{
				for(std::map<string, SHudObjective>::iterator it = m_hudObjectivesList.begin(); it != m_hudObjectivesList.end(); ++it)
				{
					if(!strcmp(it->first.c_str(), pObjective->GetID()))
					{
						m_hudObjectivesList.erase(it);
						break;
					}
				}
			}
			else
				m_hudObjectivesList[pObjective->GetID()] = SHudObjective(message, description, colorStatus);
		}

		//this gives the objective's (tracked) id to the Radar ...
		m_pHUDRadar->UpdateMissionObjective(pObjective->GetTrackedEntity(), active, description);
	}

	if(!gEnv->bMultiplayer || m_currentGameRules == EHUD_POWERSTRUGGLE) //in multiplayer the objectives are set in the miniMap only
	{
		m_animObjectivesTab.Invoke("resetObjectives");
		THUDObjectiveList::iterator it = m_hudObjectivesList.begin();
		for(; it != m_hudObjectivesList.end(); ++it)
		{
			SFlashVarValue args[3] = {it->second.description.c_str(), it->second.status, it->second.message.c_str()};
			m_animObjectivesTab.Invoke("setObjective", args, 3);
		}
		m_animObjectivesTab.Invoke("updateContent");
		m_animObjectivesTab.GetFlashPlayer()->Advance(0.1f);
	}
}
//-----------------------------------------------------------------------------------------------------

void CHUD::SetMainObjective(const char* objectiveKey, bool isGoal)
{
	CHUDMissionObjective *pObjective = m_missionObjectiveSystem.GetMissionObjective(objectiveKey);
	if(pObjective)
	{
		SFlashVarValue args[2] = {pObjective->GetShortDescription(), pObjective->GetMessage()};
		m_animObjectivesTab.Invoke((isGoal)?"setGoal":"setMainObjective", args, 2);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetOnScreenObjective(EntityId pObjectiveID)
{
	if(m_iOnScreenObjective && m_iOnScreenObjective!=pObjectiveID)
		m_pHUDRadar->UpdateMissionObjective(m_iOnScreenObjective, false, " ");

	if(m_iOnScreenObjective!=pObjectiveID)
	{
		m_iOnScreenObjective = pObjectiveID;
		if(m_iOnScreenObjective)
			m_pHUDRadar->UpdateMissionObjective(m_iOnScreenObjective, true, "");
	}
	else
	{
		m_iOnScreenObjective = 0;
	}
}

//-----------------------------------------------------------------------------------------------------

EntityId CHUD::GetOnScreenObjective()
{
	return m_iOnScreenObjective;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ResetScoreBoard()
{
	if(m_pHUDScore)
		m_pHUDScore->Reset();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetVotingState(EVotingState state, int timeout, EntityId id, const char* descr)
{
  IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
  CryLog("%d (%d seconds left) Entity(id %x name %s) description %s", state, timeout, id, pEntity?pEntity->GetName():"",descr);
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::UpdateWeaponAccessoriesScreen(bool sendToFlash)
{
	CWeapon *pWeapon = GetWeapon();

	m_bHasWeaponAttachments = false;

	m_animWeaponAccessories.Invoke("clearAllSlotButtons");
	if(pWeapon)
	{
		const CItem::THelperVector& helpers = pWeapon->GetAttachmentHelpers();
		for (int iHelper=0; iHelper<helpers.size(); iHelper++)
		{
			CItem::SAttachmentHelper helper = helpers[iHelper];
			if (helper.slot != CItem::eIGS_FirstPerson)
				continue;

			if (pWeapon->HasAttachmentAtHelper(helper.name))
			{
				if(sendToFlash)
				{
					// marcok: make sure the vars are correctly mapped
					SFlashVarValue args[3] = {helper.name.c_str(), "", ""};
					m_animWeaponAccessories.Invoke("addSlotButton", args, 3);

					//should really do not use dynamic strings here
					string strControl("Root.slot_");	strControl += helper.name.c_str();
					string strToken("hud.WS");	strToken += helper.name.c_str();	
					string strTokenX = strToken + "X";
					string strTokenY = strToken + "Y";
					m_animWeaponAccessories.AddVariable(strControl.c_str(),"_x",strTokenX.c_str(),1.0f,0.0f);
					m_animWeaponAccessories.AddVariable(strControl.c_str(),"_y",strTokenY.c_str(),1.0f,0.0f);
				}
				string curAttach = pWeapon->CurrentAttachment(helper.name);
				std::vector<string> attachments;
				pWeapon->GetAttachmentsAtHelper(helper.name, attachments);
				int iSelectedIndex = 0;
				int iCount = 0;
				if(attachments.size() > 0)
				{
					if(strcmp(helper.name,"magazine") && strcmp(helper.name,"attachment_front"))
					{
						if(!strcmp(helper.name,"attachment_top"))
						{
							if(sendToFlash)
							{
								SFlashVarValue args[3] = {helper.name.c_str(), "@IronSight", "NoAttachment"};
								m_animWeaponAccessories.Invoke("addSlotButton", args, 3);
							}
							++iCount;
						}
						else
						{
							if(sendToFlash)
							{
								SFlashVarValue args[3] = {helper.name.c_str(), "@NoAttachment", "NoAttachment"};
								m_animWeaponAccessories.Invoke("addSlotButton", args, 3);
							}
							++iCount;
						}
					}
					for(int iAttachment=0; iAttachment<attachments.size(); iAttachment++)
					{
						if(sendToFlash)
						{
							string sName("@");
							sName.append(attachments[iAttachment]);
							SFlashVarValue args[3] = {helper.name.c_str(), sName.c_str(), attachments[iAttachment].c_str()};
							m_animWeaponAccessories.Invoke("addSlotButton", args, 3);
						}
						if(curAttach == attachments[iAttachment])
						{
							iSelectedIndex = iCount;
						}
						m_bHasWeaponAttachments = true;
						++iCount;
					}
				}
				if(curAttach)
				{
					if(sendToFlash)
					{
						SFlashVarValue args[2] = {helper.name.c_str(), iSelectedIndex};
						m_animWeaponAccessories.Invoke("selectSlotButton", args, 2);
					}
				}
			}
			else ; // no attachment found for this helper
		}
		if(sendToFlash)
		{
			m_animWeaponAccessories.GetFlashPlayer()->Advance(0.25f);
		}
	}

	return m_bHasWeaponAttachments;
}
//-----------------------------------------------------------------------------------------------------

void CHUD::AddToScoreBoard(EntityId player, int kills, int deaths, int ping)
{
	if(m_pHUDScore)
		m_pHUDScore->AddEntry(player, kills, deaths, ping);
}

//------------------------------------------------------------------------

void CHUD::ForceScoreBoard(bool force)
{
	if(m_animScoreBoard.IsLoaded())
	{
		m_forceScores = force;

		if(force)
		{
			PlaySound(ESound_OpenPopup);
			m_animScoreBoard.Invoke("Root.setVisible", 1);
		}
		else
		{
			PlaySound(ESound_ClosePopup);
			m_animScoreBoard.Invoke("Root.setVisible", 0);
		}

		if (m_pHUDScore)
			m_pHUDScore->SetVisible(force, &m_animScoreBoard);
		m_bShow = !force;
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::AddToRadar(EntityId entityId) const
{
	m_pHUDRadar->AddEntityToRadar(entityId);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowSoundOnRadar(const Vec3& pos, float intensity) const
{
	m_pHUDRadar->ShowSoundOnRadar(pos, intensity);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetRadarScanningEffect(bool show)
{
	if(!m_animRadarCompassStealth.IsLoaded()) return;
	if(show)
	{
		m_animRadarCompassStealth.Invoke("setScanningEffect",true);
	}
	else
	{
		m_animRadarCompassStealth.Invoke("setScanningEffect",false);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UpdateRatio()
{
	if(m_pHUDRadar)
		m_pHUDRadar->ComputeMiniMapResolution();

	CHUDCommon::UpdateRatio();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::RegisterHUDObject(CHUDObject* pObject)
{
	stl::push_back_unique(m_externalHUDObjectList, pObject);
	pObject->SetParent(this);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::DeregisterHUDObject(CHUDObject* pObject)
{
	stl::find_and_erase(m_externalHUDObjectList, pObject);
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::RegisterListener(IHUDListener* pListener)
{
	return stl::push_back_unique(m_hudListeners, pListener);
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::UnRegisterListener(IHUDListener* pListener)
{
	return stl::find_and_erase(m_hudListeners, pListener);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "HUD");
	s->Add(*this);

#define CHILD_STATISTICS(x) if (x) (x)->GetMemoryStatistics(s)

	CHILD_STATISTICS(m_pHUDRadar);
	CHILD_STATISTICS(m_pHUDScore);
	CHILD_STATISTICS(m_pHUDTextChat);
	CHILD_STATISTICS(m_pHUDObituary);
	CHILD_STATISTICS(m_pHUDTextArea);
	CHILD_STATISTICS(m_pHUDTweakMenu);

	TGameFlashAnimationsList::const_iterator iter=m_gameFlashAnimationsList.begin();
	TGameFlashAnimationsList::const_iterator end=m_gameFlashAnimationsList.end();
	for(; iter!=end; ++iter)
	{
		CHILD_STATISTICS(*iter);
	}

	//s->Add(m_onScreenMessage);
	//s->AddContainer(m_onScreenMessageBuffer);
	//s->AddContainer(m_onScreenText);
	m_missionObjectiveSystem.GetMemoryStatistics(s);

	for (THUDObjectsList::iterator iter = m_hudObjectsList.begin(); iter != m_hudObjectsList.end(); ++iter)
		(*iter)->GetHUDObjectMemoryStatistics(s);
	for (THUDObjectsList::iterator iter = m_externalHUDObjectList.begin(); iter != m_externalHUDObjectList.end(); ++iter)
		(*iter)->GetHUDObjectMemoryStatistics(s);
	s->AddContainer(m_hudListeners);
	s->AddContainer(m_hudTempListeners);
	s->AddContainer(m_missionObjectiveValues);
	for (THUDObjectiveList::iterator iter = m_hudObjectivesList.begin(); iter != m_hudObjectivesList.end(); ++iter)
	{
		s->Add(iter->first);
		iter->second.GetMemoryStatistics(s);
	}

	// TODO: properly handle the strings in these containers
	s->AddContainer(m_possibleAirStrikeTargets);
}	

//-----------------------------------------------------------------------------------------------------

void CHUD::StartPlayerFallAndPlay() 
{ 
	if(!m_fPlayerFallAndPlayTimer)
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer)
		{
			m_fPlayerFallAndPlayTimer = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			pPlayer->Fall();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::IsInputAssisted()
{
	return gEnv->pInput->HasInputDeviceOfType(eIDT_Gamepad) && gEnv->pTimer->GetCurrTime()-m_lastNonAssistedInput > g_pGameCVars->aim_assistRestrictionTimeout;
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnEntityAddedToRadar(EntityId entityId)
{
	HUD_CALL_LISTENERS(OnEntityAddedToRadar(entityId));
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnEnterVehicle(IActor *pActor,const char *strVehicleClassName,const char *strSeatName,bool bThirdPerson)
{
	if(m_pHUDVehicleInterface)
		m_pHUDVehicleInterface->OnEnterVehicle(pActor, strVehicleClassName, strSeatName, bThirdPerson);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnExitVehicle(IActor *pActor)
{
	if (!pActor->IsClient() || !m_pHUDVehicleInterface)
		return;

	if(IVehicle *pVehicle = m_pHUDVehicleInterface->GetVehicle())
	{
		if(pVehicle->GetEntity()->GetClass() == GetRadar()->m_pAAA)
			m_animRadarCompassStealth.Invoke("setDamage", 2.0f);
	}

	m_pHUDVehicleInterface->OnExitVehicle(pActor);

	SFlashVarValue args[6] = {m_iWeaponAmmo,m_iWeaponInvAmmo,m_iWeaponClipSize, 0, m_iGrenadeAmmo, (const char *)m_sGrenadeType};
	m_animAmmo.Invoke("setAmmo", args, 6);
	m_animAmmo.Invoke("setAmmoMode", 0);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnItemDropped(IActor* pActor, EntityId itemId)
{
	m_pHUDPowerStruggle->InitEquipmentPacks();
	m_pHUDPowerStruggle->UpdatePackageList();
	m_pHUDPowerStruggle->UpdateCurrentPackage();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnItemUsed(IActor* pActor, EntityId itemId)
{
}

//-----------------------------------------------------------------------------------------------------

void CHUD::OnItemPickedUp(IActor* pActor, EntityId itemId)
{
	m_pHUDPowerStruggle->InitEquipmentPacks();
	m_pHUDPowerStruggle->UpdatePackageList();
	m_pHUDPowerStruggle->UpdateCurrentPackage();
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UnloadVehicleHUD(bool bShow)
{
	if(m_pHUDVehicleInterface)
		m_pHUDVehicleInterface->UnloadVehicleHUD(bShow);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::UnloadSimpleHUDElements(bool unload)
{
	//unloads simple assets to save memory (smaller peaks in memory pool)

	if(!unload)
	{
		m_animGrenadeDetector.Reload();
		m_animMissionObjective.Reload();
		m_animQuickMenu.Reload();
		m_animTacLock.Reload();
		m_animTargetLock.Reload();
		m_animTargetter.Reload();
		m_animTargetAutoAim.Reload();
		m_animDownloadEntities.Reload();
		m_animInitialize.Reload();
	}
	else
	{
		m_animGrenadeDetector.Unload();
		m_animMissionObjective.Unload();
		m_animQuickMenu.Unload();
		m_animTacLock.Unload();
		m_animTargetLock.Unload();
		m_animTargetter.Unload();
		m_animTargetAutoAim.Unload();
		m_animDownloadEntities.Unload();
		m_animInitialize.Unload();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::LoadGameRulesHUD(bool load)
{
  //LOAD if neccessary

  switch(m_currentGameRules)
  {
  case EHUD_SINGLEPLAYER:
    if(load)
    {
      if(!m_animObjectivesTab.IsLoaded())
      {
        m_animObjectivesTab.Load("Libs/UI/HUD_MissionObjectives.gfx", eGFD_Left, eFAF_Visible);
        m_animObjectivesTab.Invoke("showObjectives", "noAnim");
        m_animObjectivesTab.SetVisible(false);
      }
      if(!m_animTargetter.IsLoaded())
        m_animTargetter.Load("Libs/UI/HUD_EnemyShootingIndicator.gfx", eGFD_Center, eFAF_ThisHandler);
    }
    else
    {
      m_animObjectivesTab.Unload();
      m_animTargetter.Unload();
    }
    break;
  case EHUD_INSTANTACTION:
    if(load)
    {
      if(!m_animScoreBoard.IsLoaded())
      {
        m_animScoreBoard.Load("Libs/UI/HUD_MultiplayerScoreboard_DM.gfx");
        SetFlashColor(&m_animScoreBoard);
      }
      if(!m_animChat.IsLoaded())
      {
        m_animChat.Load("Libs/UI/HUD_ChatSystem.gfx", eGFD_Left);
        m_pHUDTextChat->Init(&m_animChat);
      }
      if(!m_animVoiceChat.IsLoaded())
        m_animVoiceChat.Load("Libs/UI/HUD_MultiPlayer_VoiceChat.gfx", eGFD_Right, eFAF_ThisHandler);
      if(!m_animBattleLog.IsLoaded())
        m_animBattleLog.Load("Libs/UI/HUD_MP_Log.gfx", eGFD_Left);

    }
    else
    {
      m_animScoreBoard.Unload();
      m_pHUDTextChat->Init(0);
      m_animChat.Unload();
      m_animVoiceChat.Unload();
      m_animBattleLog.Unload();
    }
    break;
  case EHUD_POWERSTRUGGLE:
		if(load)
		{
			if(!m_animScoreBoard.IsLoaded())
			{
				m_animScoreBoard.Load("Libs/UI/HUD_MultiplayerScoreboard.gfx");
				SetFlashColor(&m_animScoreBoard);
			}
			if(!m_animChat.IsLoaded())
			{
				m_animChat.Load("Libs/UI/HUD_ChatSystem.gfx", eGFD_Left);
				m_pHUDTextChat->Init(&m_animChat);
			}
			if(!m_animVoiceChat.IsLoaded())
				m_animVoiceChat.Load("Libs/UI/HUD_MultiPlayer_VoiceChat.gfx", eGFD_Right, eFAF_ThisHandler);
			if(!m_animBattleLog.IsLoaded())
				m_animBattleLog.Load("Libs/UI/HUD_MP_Log.gfx", eGFD_Left);

			if(!m_animRadioButtons.IsLoaded())
				m_animRadioButtons.Load("Libs/UI/HUD_MP_Radio_Buttons.gfx", eGFD_Center, eFAF_ThisHandler);

			if(!m_animBuyZoneIcon.IsLoaded())
				m_animBuyZoneIcon.Load("Libs/UI/HUD_BuyZone_Icon.gfx", eGFD_Left, eFAF_ThisHandler);
			if(!m_animBuyMenu.IsLoaded())
				m_animBuyMenu.Load("Libs/UI/HUD_PDA_Buy.gfx", eGFD_Right, eFAF_ThisHandler);
			if(!m_animCaptureProgress.IsLoaded())
				m_animCaptureProgress.Load("Libs/UI/HUD_BuyZone_Icon.gfx", eGFD_Left, eFAF_ThisHandler);
			if(!m_animPlayerPP.IsLoaded())
				m_animPlayerPP.Load("Libs/UI/HUD_MP_PPoints.gfx", eGFD_Right);
			if(!m_animTutorial.IsLoaded())
				m_animTutorial.Load("Libs/UI/HUD_Tutorial.gfx");
			if(!m_animObjectivesTab.IsLoaded())
			{
				m_animObjectivesTab.Load("Libs/UI/HUD_MissionObjectives.gfx", eGFD_Center, eFAF_Visible);
				m_animObjectivesTab.Invoke("showObjectives", "noAnim");
				m_animObjectivesTab.SetVisible(false);
			}
		}
		else
		{
			m_animScoreBoard.Unload();
			m_pHUDTextChat->Init(0);
			m_animChat.Unload();
			m_animVoiceChat.Unload();
			m_animBattleLog.Unload();
			m_animBuyZoneIcon.Unload();
			m_animBuyMenu.Unload();
			m_animPlayerPP.Unload();
			m_animTutorial.Unload();
			m_animObjectivesTab.Unload();
			m_animCaptureProgress.Unload();
		}
		break;
	case EHUD_TEAMACTION:
    if(load)
    {
      if(!m_animScoreBoard.IsLoaded())
      {
        m_animScoreBoard.Load("Libs/UI/HUD_MultiplayerScoreboard_TDM.gfx");
        SetFlashColor(&m_animScoreBoard);
      }
      if(!m_animChat.IsLoaded())
      {
        m_animChat.Load("Libs/UI/HUD_ChatSystem.gfx", eGFD_Left);
        m_pHUDTextChat->Init(&m_animChat);
      }
      if(!m_animVoiceChat.IsLoaded())
        m_animVoiceChat.Load("Libs/UI/HUD_MultiPlayer_VoiceChat.gfx", eGFD_Right, eFAF_ThisHandler);
      if(!m_animBattleLog.IsLoaded())
        m_animBattleLog.Load("Libs/UI/HUD_MP_Log.gfx", eGFD_Left);

      if(!m_animRadioButtons.IsLoaded())
        m_animRadioButtons.Load("Libs/UI/HUD_MP_Radio_Buttons.gfx", eGFD_Center, eFAF_ThisHandler);

      if(!m_animBuyZoneIcon.IsLoaded())
        m_animBuyZoneIcon.Load("Libs/UI/HUD_BuyZone_Icon.gfx", eGFD_Left, eFAF_ThisHandler);
      if(!m_animBuyMenu.IsLoaded())
        m_animBuyMenu.Load("Libs/UI/HUD_PDA_Buy.gfx", eGFD_Right, eFAF_ThisHandler);
      if(!m_animPlayerPP.IsLoaded())
        m_animPlayerPP.Load("Libs/UI/HUD_MP_PPoints.gfx", eGFD_Right);
			if(!m_animObjectivesTab.IsLoaded())
			{
				m_animObjectivesTab.Load("Libs/UI/HUD_MissionObjectives.gfx", eGFD_Center, eFAF_Visible);
				m_animObjectivesTab.Invoke("showObjectives", "noAnim");
				m_animObjectivesTab.SetVisible(false);
			}
    }
    else
    {
      m_animScoreBoard.Unload();
      m_pHUDTextChat->Init(0);
      m_animChat.Unload();
      m_animVoiceChat.Unload();
      m_animBattleLog.Unload();
      m_animBuyZoneIcon.Unload();
      m_animBuyMenu.Unload();
      m_animPlayerPP.Unload();
			m_animObjectivesTab.Unload();
    }
    break;
  }
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetInMenu(bool m)
{
  m_bInMenu = m;
}

//-----------------------------------------------------------------------------------------------------

bool CHUD::IsFriendlyToClient(EntityId uiEntityId)
{
	if(!gEnv->bMultiplayer)
	{
		if(uiEntityId)
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(uiEntityId);
			if(pEntity)
			{
				IAIActor *pAIActor = CastToIAIActorSafe(pEntity->GetAI());
				if(pAIActor)
				{
					if(0 == pAIActor->GetParameters().m_nSpecies)
					{
						return true;
					}
				}
			}
		}
		return false;
	}
	else
	{
		IActor *client = g_pGame->GetIGameFramework()->GetClientActor();
		if(!client || !g_pGame->GetGameRules())
			return false;
		if(g_pGame->GetGameRules()->GetTeam(uiEntityId) == g_pGame->GetGameRules()->GetTeam(client->GetEntityId()))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------