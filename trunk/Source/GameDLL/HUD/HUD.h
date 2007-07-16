/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 
	Header for G02 CHUD class

-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre
- 01:02:2006: Modified by Jan Müller
- 22:02:2006: Refactored for G04 by Matthew Jack
- 2007: Refactored by Jan Müller

*************************************************************************/
#ifndef __HUD_H__
#define __HUD_H__

//-----------------------------------------------------------------------------------------------------

#include "HUDEnums.h"
#include "HUDCommon.h"
#include "IFlashPlayer.h"
#include "IActionMapManager.h"
#include "IGameTokens.h"
#include "IInput.h"
#include "IMovementController.h"
#include "IVehicleSystem.h"
#include "Item.h"
#include "NanoSuit.h"
#include "Player.h"
#include "Voting.h"
#include "IViewSystem.h"
#include "ISubtitleManager.h"

#include "HUDMissionObjectiveSystem.h"
#include "HUDRadar.h"


// uncomment following lines to support G15 LCD
//#define USE_G15_LCD
//#include "LCD/LCDWrapper.h"

//-----------------------------------------------------------------------------------------------------

// Forward declarations
struct IRenderer;

class CHUDObject;
class CHUDRadar;
class CHUDScore;
class CHUDTextChat;
class CHUDObituary;
class CHUDTextArea;
class CHUDTweakMenu;
class CHUDVehicleInterface;
class CHUDPowerStruggle;
class CHUDScopes;
class CHUDCrosshair;
class CHUDTagNames;

class CWeapon;

//-----------------------------------------------------------------------------------------------------

class CHUD :	public CHUDCommon, 
							public IGameFrameworkListener, 
							public IInputEventListener, 
							public IGameTokenEventListener,
							public IPlayerEventListener, 
							public IItemSystemListener, 
							public IWeaponEventListener, 
							public CNanoSuit::INanoSuitListener,
							public IViewSystemListener,
							public ISubtitleHandler
{
	friend class CFlashMenuObject;
	friend class CHUDPowerStruggle;
	friend class CHUDScopes;

public:
	CHUD();
	virtual	~	CHUD();

	//HUD initialisation
	bool Init();
	//setting local actor / player id
	void PlayerIdSet(EntityId playerId);
	void Serialize(TSerialize ser,unsigned aspects);
	void PostSerialize();
  void GameRulesSet(const char* name);
	//handle game events
	void HandleEvent(const SGameObjectEvent &rGameObjectEvent);
	void WeaponAccessoriesInterface(bool visible, bool force = false);
	void SetFireMode(const char* name);
	void SetGrenade(EntityId id);
	void AutoAimLocking(EntityId id);
	void AutoAimNoText(EntityId id);
	void AutoAimLocked(EntityId id);
	void AutoAimUnlock(EntityId id);
	void ActorDeath(IActor* pActor);
	void VehicleDestroyed(EntityId id);
	void TextMessage(const char* message);

	void UpdateHUDElements();

	void GetMemoryStatistics( ICrySizer * );

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime);
	virtual void OnSaveGame(ISaveGame* pSaveGame);
	virtual void OnLoadGame(ILoadGame* pLoadGame) {};
  virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);
	// ~IInputEventListener

	// IFSCommandHandler
	void HandleFSCommand(const char *strCommand,const char *strArgs);
	// ~IFSCommandHandler

	// FS Command Handlers (as we also call a lot of these externally)
	void OnQuickMenuSpeedPreset();
	void OnQuickMenuStrengthPreset();
	void OnQuickMenuDefensePreset();
	void OnCloak();

	// Overridden for specific functionality
	virtual void UpdateRatio();

	virtual bool OnAction(const ActionId& action, int activationMode, float value);

	// IGameTokenEventListener
	virtual void OnGameTokenEvent( EGameTokenEvent event,IGameToken *pGameToken );
	// ~IGameTokenEventListener

	virtual void BattleLogEvent(int type, const char *msg, const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0);
	
	// IPlayerEventListener
	virtual void OnEnterVehicle(IActor *pActor,const char *strVehicleClassName,const char *strSeatName,bool bThirdPerson);
	virtual void OnExitVehicle(IActor *pActor);
	virtual void OnToggleThirdPerson(IActor *pActor,bool bThirdPerson);
	virtual void OnItemDropped(IActor* pActor, EntityId itemId);
	virtual void OnItemUsed(IActor* pActor, EntityId itemId);
	virtual void OnItemPickedUp(IActor* pActor, EntityId itemId);
	virtual void OnStanceChanged(IActor* pActor, EStance stance) {}
	// ~IPlayerEventListener

	// IItemSystemListener
	virtual void OnSetActorItem(IActor *pActor, IItem *pItem );
	virtual void OnDropActorItem(IActor *pActor, IItem *pItem );
	virtual void OnSetActorAccessory(IActor *pActor, IItem *pItem );
	virtual void OnDropActorAccessory(IActor *pActor, IItem *pItem );
	// ~IItemSystemListener

	// IWeaponEventListener
	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,	const Vec3 &pos, const Vec3 &dir, const Vec3 &vel) {}
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {}
	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {}
	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {}
	virtual void OnReadyToFire(IWeapon *pWeapon) {}
	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed) {}
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId) {}
	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId) {}
	virtual void OnStartTargetting(IWeapon *pWeapon);
	virtual void OnStopTargetting(IWeapon *pWeapon);
	// ~IWeaponEventListener

	// INanoSuitListener
	virtual void ModeChanged(ENanoMode mode);     // nanomode
	virtual void EnergyChanged(float energy);     // energy
	// ~INanoSuitListener

	// ISubTitleHandler
	virtual void ShowSubtitle(ISound* pSound, bool bShow);
	virtual void ShowSubtitle(const char* subtitleLabel, bool bShow);
	// ~ISubTitleHandler

	// ILevelSystemListener - handled by FlashMenuObject
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	// ~ILevelSystemListener

	// IViewSystemListener
	virtual bool OnBeginCutScene(IAnimSequence* pSeq, bool bResetFX);
	virtual bool OnEndCutScene(IAnimSequence* pSeq);
	virtual void OnPlayCutSceneSound(IAnimSequence* pSeq, ISound* pSound);
	virtual bool OnCameraChange(const SCameraParams& cameraParams);
	// ~IViewSystemListener

	// Console Variable Changed callbacks (registered in GameCVars)
	static void OnCrosshairCVarChanged(ICVar *pCVar);
	static void OnSubtitleCVarChanged (ICVar* pCVar);
	static void OnSubtitlePanoramicHeightCVarChanged (ICVar* pCVar);
	// 

	enum HUDSubtitleMode
	{
		eHSM_Off = 0,
		eHSM_All = 1,
		eHSM_CutSceneOnly = 2,
	};

	// enable/disable subtitles
	void SetSubtitleMode(HUDSubtitleMode mode);

	//memory optimization
	void UnloadVehicleHUD(bool bShow);
	void UnloadSimpleHUDElements(bool unload);

  //Game rules specific stuff - load/unload stuff
  void LoadGameRulesHUD(bool load);

	virtual void UpdateCrosshairVisibility();

  void SetInMenu(bool m);

	//sets the color to all hud elements
	void SetHUDColor();
	//set the color of one hud element
	void SetFlashColor(CGameFlashAnimation* pGameFlashAnimation);
	//render boot-up animation
	virtual void ShowBootSequence();
	//render download animation
	virtual void ShowDownloadSequence();
	//render Death effects
	void ShowDeathFX(int type);
	//enable/disable an animation for minimap not available
	void SetMinimapNotAvailable(bool enable) { m_bNoMiniMap = enable; }

	//shows a warning in the flash hud
	void ShowWarningMessage(EWarningMessages message, const char* optionalText = NULL);
	void HandleWarningAnswer(const char* warning = NULL);

	// Airstrike interface
	virtual bool IsAirStrikeAvailable();
	virtual void SetAirStrikeEnabled(bool enabled);
	void SetAirStrikeBinoculars(bool p_bEnabled);
	void AddAirstrikeEntity(EntityId);
	void ClearAirstrikeEntities();
	void NotifyAirstrikeSucceeded(bool p_bSucceeded);
	bool StartAirStrike();
	void UpdateAirStrikeTarget(EntityId target);
	// ~Airtsrike interface

	// display a flash message
	// label can be a plain text message or a localization label
	// @pos   position: 1=top,2=middle,3=bottom
	void DisplayFlashMessage(const char* label, int pos = 1, const ColorF &col = Col_White, bool formatWStringWithParams = false, const char* paramLabel1 = 0, const char* paramLabel2 = 0, const char* paramLabel3 = 0, const char* paramLabel4 = 0);
	void DisplayTempFlashText(const char* label, float seconds, const ColorF &col = Col_White);

	//Get and Set
	void LockTarget(EntityId target, ELockingType type, bool showtext = true, bool multiple = false);
	void UnlockTarget(EntityId target);

	//Scoreboard
	void AddToScoreBoard(EntityId player, int kills, int deaths, int ping);
	void ForceScoreBoard(bool force);
	void ResetScoreBoard();
  void SetVotingState(EVotingState state, int timeout, EntityId id, const char* descr);

	//RadioButtons & Chat
	void SetRadioButtons(bool active, int buttonNo = 0);
	void ShowVirtualKeyboard(bool active);
	void ObituaryMessage(EntityId targetId, EntityId shooterId, EntityId weaponId, bool headshot);

	//Radar
	void AddToRadar(EntityId entityId) const;
	void ShowSoundOnRadar(const Vec3& pos, float intensity = 1.0f) const;
	void SetRadarScanningEffect(bool show);

	//get sub-hud
	ILINE CHUDRadar* GetRadar() {return m_pHUDRadar;}
	ILINE CHUDVehicleInterface* GetVehicleInterface() { return m_pHUDVehicleInterface; }
	ILINE CHUDPowerStruggle* GetPowerStruggleHUD() { return m_pHUDPowerStruggle; }
	ILINE CHUDTextChat* GetMPChat() {return m_pHUDTextChat;}
	ILINE CHUDScopes* GetScopes() { return m_pHUDScopes; }
	ILINE CHUDCrosshair* GetCrosshair() { return m_pHUDCrosshair; }
	ILINE CHUDTagNames* GetTagNames() { return m_pHUDTagNames; }

	//mission objectives
	void UpdateMissionObjectiveIcon(EntityId objective, int friendly, FlashOnScreenIcon iconType);
	void UpdateAllMissionObjectives();
	void UpdateObjective(CHUDMissionObjective* pObjective);
	void SetMainObjective(const char* objectiveKey, bool isGoal);
	void AddOnScreenMissionObjective(IEntity *pEntity, int friendly);
	void SetOnScreenObjective(EntityId pID);
	bool IsUnderAttack(IEntity *pEntity);
	EntityId GetOnScreenObjective();
	ILINE CHUDMissionObjectiveSystem& GetMissionObjectiveSystem() { return m_missionObjectiveSystem; }
	//~mission objectives

	//BattleStatus code : consult Marco C.
	//	Increases the battle status
	void	TickBattleStatus(float fValue);
	void	UpdateBattleStatus();
	//	Queries the battle status, 0=no battle, 1=full battle
	float	QueryBattleStatus() { return (m_fBattleStatus); }
	float GetBattleRange();

	// Add/Remove external hud objects, they're called after the normal hud objects
	void RegisterHUDObject(CHUDObject* pObject);
	void DeregisterHUDObject(CHUDObject* pObject);

	void FadeCinematicBars(int targetVal);

	//PowerStruggle
	void OnPlayerVehicleBuilt(EntityId playerId, EntityId vehicleId);
	void BuyViaFlash(int item);
	void ShowTutorialText(const wchar_t* text, int pos);
	void SetTutorialTextPosition(int pos);
	void SetTutorialTextPosition(float posX, float posY);

	//script function helper
	int CallScriptFunction(IEntity *pEntity, const char *function);

	//HUDPDA.cpp
	//(de)activate buttons in the quick menu 
	void ActivateQuickMenuButton(EQuickMenuButtons button, bool active = true);
	void SetQuickMenuButtonDefect(EQuickMenuButtons button, bool defect = true);
	ILINE bool IsQuickMenuButtonActive(EQuickMenuButtons button) const { return (m_activeButtons & (1<<button))?true:false;}
	ILINE bool IsQuickMenuButtonDefect(EQuickMenuButtons button) const { return (m_defectButtons & (1<<button))?true:false;}

	void GetGPSPosition(SMovementState *pMovementState,char *strN,char *strW);
	void ShowDeltaEnergy(float energy);
	//~HUDPDA.cpp

	// Some special HUD fx
	void BreakHUD(int state = 1);
	void RebootHUD();

	//bool ShowPDA(bool bShow, int iTab=-1);
	ILINE bool ShowBuyMenu(bool show) { return ShowPDA(show, true); }
	bool ShowPDA(bool show, bool buyMenu = false);
	void ShowObjectives(bool bShow);
	void StartPlayerFallAndPlay();
	bool IsPDAActive() const { return m_animPDA.GetVisible(); };
	bool IsBuyMenuActive() const { return m_animBuyMenu.GetVisible(); };
	ILINE bool HasTACWeapon() { return m_hasTACWeapon; };
	void SetTACWeapon(bool hasTACWeapon);

	//interface effects
	void IndicateDamage(EntityId weaponId, Vec3 direction, bool onVehicle = false);
	void IndicateHit();
	void ShowKillAreaWarning(bool active, int timer);
	void ShowTargettingAI(EntityId id);
	void ShowProgress(int progress = -1, bool init = false, int posX = 0, int posY = 0, const char* text = NULL, bool topText = true);
	void FakeDeath(bool revive = false);
	ILINE bool IsFakeDead() { return (m_fPlayerRespawnTimer)?true:false; }
	void ShowDataUpload(bool active);
	void ShowSpectate(bool active);

	// called from CHUDRadar
	void OnEntityAddedToRadar(EntityId entityId);

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent);

	//HUDSoundImpl
	void PlaySound(ESound eSound, bool play = true);
	void PlayStatusSound(const char* identifier, bool forceSuitSound = false);
	//~HUDSoundImpl

	ILINE CGameFlashAnimation* GetModalHUD() { return m_pModalHUD; }
	ILINE CGameFlashAnimation* GetMapAnim() { return &m_animPDA; }

	// hud event listeners
	struct IHUDListener
	{
		virtual void HUDDestroyed() {};
		virtual void PDAOpened() {};
		virtual void PDAClosed() {};
		virtual void PDATabChanged(int tab) {};
		virtual void OnShowObjectives(bool open) {};
		virtual void WeaponAccessoryChanged(CWeapon* pWeapon, const char* accessory, bool bAddAccessory) {};
		virtual void OnAirstrike(int mode, EntityId entityId) {}; // mode: 0=stop, 1=start
		virtual void OnNightVision(bool bEnabled) {};
		virtual void OnBinoculars(bool bShown) {};
		virtual void OnEntityAddedToRadar(EntityId entityId) {};

		// SNH: adding these for PowerStruggle tutorial
		virtual void OnBuyMenuOpen(bool open, FlashRadarType buyZoneType) {};
		virtual void OnMapOpen(bool open) {};
		virtual void OnBuyMenuItemHover(const char* itemname) {};
		virtual void OnShowBuyMenuAmmoPage() {};
		virtual void OnShowScoreBoard() {};
	};
	bool RegisterListener(IHUDListener* pListener);
	bool UnRegisterListener(IHUDListener* pListener);

	//assistance restriction
	bool IsInputAssisted();

	//has the AI the client's species ?
	bool IsFriendlyToClient(EntityId uiEntityId);

private:

	// Get the current weapon object
	CWeapon *GetWeapon();

	//some Update functions
	void UpdateHealth();
	bool UpdateWeaponAccessoriesScreen(bool sendToFlash = true);
	bool UpdateNanoSlotMax();
	void UpdateTimers(float frameTime);
	void UpdateWarningMessages(float frameTime);

	void SetOnScreenTargetter();
	void InitPDA();
	void HandleFSCommandPDA(const char *strCommand,const char *strArgs);

	//HUDInterfaceEffects
	void QuickMenuSnapToMode(ENanoMode mode);
	void AutoSnap();
	void GrenadeDetector(CPlayer* pPlayerActor);
	void Targetting(EntityId p_iTarget, bool p_bStatic);
	bool ShowLockingBrackets(EntityId p_iTarget, std::vector<double> *doubleArray);
	void UpdateVoiceChat();
	//~HUDInterfaceEffects

	int FillUpMOArray(std::vector<double> *doubleArray, double a, double b, double c, double d, double e, double f, double g);

	void UpdateCinematicAnim(float frameTime);
	void UpdateSubtitlesAnim(float frameTime);
	void UpdateSubtitlesManualRender(float frameTime);
	void InternalShowSubtitle(const char* subtitleLabel, ISound* pSound, bool bShow);
	// returns pointer to static buffer! so NOT re-entrant!
	const wchar_t* LocalizeWithParams(const char* label, bool bAdjustActions=true, const char* param1 = 0, const char* param2 = 0, const char* param3 = 0, const char* param4 = 0);
	void RegisterTokens();
	void UnregisterTokens();

	//EHUDGAMERULES GetGameRules();

	//modal hud management
	void SwitchToModalHUD(CGameFlashAnimation* pModalHUD,bool bNeedMouse);
	bool IsModalHUDAvailable() { return m_pModalHUD == 0; }

	//member hud objects (sub huds)
	CHUDRadar			*m_pHUDRadar;
	CHUDScore			*m_pHUDScore;
	CHUDTextChat	*m_pHUDTextChat;
	CHUDObituary	*m_pHUDObituary;
	CHUDTextArea	*m_pHUDTextArea;
	CHUDTweakMenu	*m_pHUDTweakMenu;
	CHUDVehicleInterface *m_pHUDVehicleInterface;
	CHUDPowerStruggle *m_pHUDPowerStruggle;
	CHUDScopes		*m_pHUDScopes;
	CHUDCrosshair *m_pHUDCrosshair;
	CHUDTagNames	*m_pHUDTagNames;

	bool					m_forceScores;

	//cached pointer to renderer
	IRenderer			*m_pRenderer;
	IUIDraw				*m_pUIDraw;

	IFFont *m_pDefaultFont;

	//sound related
	float	m_fLastSoundPlayedCritical;
	float m_fSpeedTimer, m_fStrengthTimer, m_fDefenseTimer;
	float	m_fBattleStatus,m_fBattleStatusDelay;	

	//this manages the mission objectives
	CHUDMissionObjectiveSystem		m_missionObjectiveSystem;

	//status messages that are displayed on screen and trigger vocals
	std::map<string, string> m_statusMessagesMap;

	//this controls which buttons are currently active (Speed, Strength, Cloak, Weapon, Armor)
	int						m_activeButtons;
	int						m_defectButtons;
	bool					m_bHasWeaponAttachments;
	bool					m_acceptNextWeaponCommand;

	//NanoSuit-pointer for suit interaction
	CNanoSuit    *m_pNanoSuit;
	//NanoSuit
	ENanoSlot m_eNanoSlotMax;
	float		m_fHealth;
	float		m_fSuitEnergy;
	
	//interface logic
	CGameFlashAnimation*	m_pModalHUD;
	CGameFlashAnimation*	m_pSwitchScoreboardHUD;
	bool m_bScoreboardCursor;
	bool m_bFirstFrame;
	bool m_bHideCrosshair;
	bool m_bAutosnap;
	bool m_bIgnoreMiddleClick;
	bool m_bLaunchWS;
	bool m_bBreakHUD;
	bool m_bHUDInitialize;
	bool m_bInMenu;
	bool m_bGrenadeLeftOrRight;
	bool m_bGrenadeBehind;
	bool m_bThirdPerson;
	bool m_bNightVisionActive;
	bool m_bTacLock;
	float m_fNightVisionTimer;
	float m_fAlpha;
	float m_fSuitChangeSoundTimer;
	float m_fLastReboot;
	int m_iWeaponAmmo;
	int m_iWeaponInvAmmo;
	int m_iWeaponClipSize;
	int m_iGrenadeAmmo;
	int m_iCatchBuyMenuPage;
	string m_sGrenadeType;
	int m_iFade;
	int m_iVoiceMode;
	bool m_bMiniMapZooming;
	float m_fCatchBuyMenuTimer;
	EntityId m_iPlayerOwnedVehicle;
	bool m_bExclusiveListener;
	float m_fMiddleTextLineTimeout;
	int m_iOpenTextChat;
	bool m_bNoMiniMap;

	//airstrike
	bool m_bAirStrikeAvailable;
	float m_fAirStrikeStarted;
	EntityId m_iAirStrikeTarget;
	std::vector<EntityId> m_possibleAirStrikeTargets;

	//respawn and restart
	float		m_fPlayerDeathTime;
	float		m_fPlayerRespawnTimer;
	float		m_fLastPlayerRespawnEffect;
	bool		m_bRespawningFromFakeDeath;
	string	m_sLastSaveGame;

	EntityId m_uiWeapondID;
	bool m_bShowAllOnScreenObjectives;
	EntityId m_iOnScreenObjective;
	IWeapon *m_pWeapon;
	bool m_hasTACWeapon;

  EHUDGAMERULES m_currentGameRules;

	CGameFlashAnimation	m_animAmmo;
	CGameFlashAnimation	m_animGrenadeDetector;
	CGameFlashAnimation	m_animMissionObjective;
	CGameFlashAnimation	m_animHealthEnergy;
	CGameFlashAnimation	m_animPDA;
	CGameFlashAnimation	m_animQuickMenu;
	CGameFlashAnimation	m_animRadarCompassStealth;
	CGameFlashAnimation m_animSuitIcons;
	CGameFlashAnimation m_animTacLock;
	CGameFlashAnimation m_animTargetAutoAim;
	CGameFlashAnimation m_animGamepadConnected;
	CGameFlashAnimation m_animBuyMenu;
	CGameFlashAnimation m_animCaptureProgress;
	CGameFlashAnimation m_animScoreBoard;
	CGameFlashAnimation m_animWeaponAccessories;
	CGameFlashAnimation m_animWarningMessages;
	CGameFlashAnimation m_animTargetLock;
	CGameFlashAnimation m_animDownloadEntities;
	CGameFlashAnimation	m_animInitialize;
	CGameFlashAnimation m_animBreakHUD;
	CGameFlashAnimation m_animRebootHUD;
	CGameFlashAnimation m_animAirStrike;
	CGameFlashAnimation m_animMessages;
	CGameFlashAnimation m_animChat;
	CGameFlashAnimation m_animVoiceChat;
	CGameFlashAnimation m_animKillAreaWarning;
	CGameFlashAnimation m_animDeathMessage;
	CGameFlashAnimation m_animCinematicBar;
	CGameFlashAnimation m_animObjectivesTab;
	CGameFlashAnimation m_animBuyZoneIcon;
	CGameFlashAnimation m_animBattleLog;
	CGameFlashAnimation m_animSubtitles;
	CGameFlashAnimation m_animRadioButtons;
	CGameFlashAnimation m_animTargetter;
	CGameFlashAnimation m_animPlayerPP;
	CGameFlashAnimation m_animProgress;
	CGameFlashAnimation m_animTutorial;
	CGameFlashAnimation m_animDataUpload;
	CGameFlashAnimation m_animSpectate;

	// HUD objects
	typedef std::list<CHUDObject *> THUDObjectsList;
	THUDObjectsList m_hudObjectsList;
	THUDObjectsList m_externalHUDObjectList;

	// HUD listener
	std::vector<IHUDListener*> m_hudListeners;
	std::vector<IHUDListener*> m_hudTempListeners;
	std::vector<double> m_missionObjectiveValues;
	int m_missionObjectiveNumEntries;

	EntityId m_entityTargetAutoaimId;
	EntityId m_entityTargetLockId;
	EntityId m_entityGrenadeDectectorId;

	//gamepad autosnapping
	float m_fAutosnapCursorRelativeX;
	float m_fAutosnapCursorRelativeY;
	float m_fAutosnapCursorControllerX;
	float m_fAutosnapCursorControllerY;
	bool m_bOnCircle;

	//timers
	float m_fDamageIndicatorTimer;
	float m_fPlayerFallAndPlayTimer;
	float m_fSetAgressorIcon;
	EntityId  m_agressorIconID;
	int		m_lastPlayerPPSet;

	//entity classes for faster comparison
	IEntityClass *m_pSCAR, *m_pFY71, *m_pSMG, *m_pDSG1, *m_pShotgun, *m_pLAW, *m_pGauss;


	//current mission objectives
	struct SHudObjective
	{
		string	message;
		string  description;
		int			status;
		SHudObjective() {}
		SHudObjective(const string &msg, const string &desc, const int stat) : message(msg), description(desc), status(stat) {}
		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(*this);
			s->Add(message);
			s->Add(description);
		}
	};
	typedef std::map<string, SHudObjective> THUDObjectiveList;
	THUDObjectiveList m_hudObjectivesList;

	//this is a list of sound ids to be able to stop them
	tSoundID m_soundIDs[ESound_Hud_Last];

	//cinematic bar
	enum HUDCineState
	{
		eHCS_None = 0,
		eHCS_Fading
	};

	struct SSubtitleEntry
	{
		float      timeRemaining;
		tSoundID   soundId;
		string     key;     // cannot cache LocalizationInfo, would crash on reload dialog data
		wstring    localized;
		bool       bPersistant;

		bool operator==(const char* otherKey) const
		{
			return key == otherKey;
		}
	};
	typedef std::list<SSubtitleEntry> TSubtitleEntries;

	TSubtitleEntries m_subtitleEntries;
	HUDSubtitleMode m_hudSubTitleMode; // not serialized, as set by cvar
	bool m_bSubtitlesNeedUpdate; // need update (mainly for flash re-pumping)
	HUDCineState m_cineState;
	bool m_cineHideHUD;

	float m_lastNonAssistedInput;
	bool m_hitAssistanceAvailable;

#ifdef USE_G15_LCD
	// G15 LCD implementation ...
	CLCDWrapper* m_pLCD;
#endif//USE_G15_LCD
};

//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------
