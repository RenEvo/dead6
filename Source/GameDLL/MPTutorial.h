/*************************************************************************
Crytek Source File.
Copyright (C), *Crytek Studios, *2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: MP Tutorial for PowerStruggle

-------------------------------------------------------------------------
History:
- 12:03:2007: Created by Steve Humphreys

*************************************************************************/

#ifndef __MPTUTORIAL_H__
#define __MPTUTORIAL_H__

#pragma once

#include "HUD/HUD.h"

class CPlayer;

// things we show a tutorial message for
enum ETutorialEvent
{
	eTE_NullEvent = -1,

	// initial briefing. Events between eTE_START_BRIEFING and eTE_END_BRIEFING play in order,
	//	all others in whatever order they happen
	eTE_FIRST_BRIEFING = 0,
	eTE_StartGame = eTE_FIRST_BRIEFING,
	eTE_ContinueTutorial,
	eTE_Barracks,
	eTE_BarracksTwo,
	eTE_CloseBarracksBuyMenu,
	eTE_OpenMap,
	eTE_CloseMap,
	eTE_Swingometer,
	eTE_LAST_BRIEFING = eTE_Swingometer,

	// shown when tutorial is disabled - 'you can reenable it in the menus'
	eTE_TutorialDisabled,

	eTE_ScoreBoard,
	eTE_Promotion,

	// capturing a factory
	eTE_NeutralFactory,
	eTE_CaptureFactory,
	eTE_VehicleBuyMenu,
	eTE_WarBuyMenu,
	eTE_PrototypeBuyMenu,
	eTE_NavalBuyMenu,
	eTE_AirBuyMenu,
	eTE_CloseBuyMenu,
	eTE_BuyAmmo,
	eTE_SpawnBunker,

	// prototype factory
	eTE_EnterPrototypeFactory,
	eTE_FindAlienCrashSite,
	eTE_FillCollector,
	eTE_FillReactor,
	eTE_ReactorFull,

	// in game action
	eTE_EnemySpotted,
	eTE_BoardVehicle,
	eTE_EnterHostileFactory,
	eTE_Wounded,
	eTE_Killed,
	eTE_TACTankStarted,
	eTE_TACTankCompleted,
	eTE_TACTankBase,
	eTE_TACLauncherCompleted,
	eTE_SingularityStarted,
	eTE_SingularityCompleted,
	eTE_SingularityBase,
	eTE_EnemyNearBase,
	eTE_TurretUnderAttack,
	eTE_TurretDestroyed,
	eTE_HqUnderAttack,
	eTE_HqCritical,
	eTE_ApproachEnemyBase,
	eTE_ApproachEnemyHq,
	eTE_ApproachEnemySub,
	eTE_ApproachEnemyCarrier,
	eTE_DestroyEnemyHq,
	eTE_GameOverWin,
	eTE_GameOverLose,

	// weapons+items start here. Names are the ids in the PowerStruggleBuying.lua 
	//	(since that is what the HUD fs command reports them as)
	eTE_FIRST_ITEM,
	eTE_pistol = eTE_FIRST_ITEM,
	eTE_smg,
	eTE_shotgun,
	eTE_fy71,
	eTE_macs,
	eTE_dsg1,
	eTE_rpg,
	eTE_gauss,
	eTE_hurricane,
	eTE_moar,
	eTE_moac,
	eTE_fraggren,
	eTE_smokegren,
	eTE_flashbang,
	eTE_c4,
	eTE_avmine,
	eTE_claymore,
	eTE_radarkit,
	eTE_binocs,
	eTE_lockkit,
	eTE_techkit,
	eTE_pchute,
	eTE_psilent,
	eTE_silent,
	eTE_gl,
	eTE_HTScope_TODO,
	eTE_scope,
	eTE_ascope,
	eTE_reflex,
	eTE_plam,
	eTE_lam,
	eTE_tactical,
	eTE_acloak,
	eTE_LAST_ITEM = eTE_acloak,

	// vehicles start here
	eTE_FIRST_VEHICLE,
	eTE_SUV = eTE_FIRST_VEHICLE,
	eTE_nk4wd,
	eTE_us4wd,
	eTE_usapc,
	eTE_AAPC_TODO,
	eTE_nkaaa,
	eTE_ustank,
	eTE_nktank,
	eTE_nkhelicopter,
	eTE_usvtol,
	eTE_cboat,
	eTE_usboat,
	eTE_nkboat,
	eTE_ushovercraft,
	eTE_usspawntruck,
	eTE_usammotruck,
	eTE_usgauss4wd,
	eTE_usmoac4wd,
	eTE_usmoar4wd,
	eTE_MOACBOAT_TODO,
	eTE_NMOARBOAT_TODO,
	eTE_SINGBOAT_TODO,
	eTE_usgausstank,
	eTE_ussingtank,
	eTE_ustactank,
	eTE_SINGVTOL_TODO,
	eTE_LAST_VEHICLE = eTE_SINGVTOL_TODO,

	eTE_NumEvents,
};

// one of these exists for all the above events
struct STutorialEvent
{
	string m_name;				// eg 'StartGame' - basically the ETutorialEvent without the eTE_ prefix. Used to build localised text string.
	string m_soundName;		// eg 'mp_american_bridge_officer_1_brief01' - filenames from the ui_dialog_recording_list.xml
	string m_action;			// some messages need a 'press x key' inserted. m_action is the actionmap name, which is replaced by the currently bound key.

	bool m_shouldCheck;		// whether to check for this event
	bool m_triggered;			// true when this event has happened
};

class CMPTutorial : public CHUD::IHUDListener, public ISoundEventListener
{
public:
	CMPTutorial();
	~CMPTutorial();

	// IHUDListener
	virtual void OnBuyMenuOpen(bool open, FlashRadarType buyZoneType);
	virtual void OnMapOpen(bool open);
	virtual void OnEntityAddedToRadar(EntityId id);
	virtual void OnBuyMenuItemHover(const char* itemname);
	virtual void OnShowBuyMenuAmmoPage();
	virtual void OnShowScoreBoard();
	// ~IHUDListener

	// ISoundEventListener
	virtual void OnSoundEvent( ESoundCallbackEvent event,ISound *pSound );
	// ~ISoundEventListener

	bool TriggerEvent(ETutorialEvent event);
	void EnableTutorialMode(bool enable);
	bool IsEnabled() const			{ return m_enabled; }

	void Update();

private:
	void InitEvents();
	void InitEntityClasses();
	//void SetEventText(string& eventName, string& eventText);

	bool CheckBriefingEvents(const CPlayer* pPlayer);
	bool CheckNearbyEntities(const CPlayer* pPlayer);
	bool CheckVehicles(const CPlayer* pPlayer);
	bool CheckBases(const CPlayer* pPlayer);

	void ShowMessage(const STutorialEvent& event);

	static void ForceTriggerEvent(IConsoleCmdArgs* args);
	static void ResetAllEvents(IConsoleCmdArgs* args);
	const char* GetKeyName(const char* action);

	bool m_enabled;
	bool m_addedListeners;
	STutorialEvent m_events[eTE_NumEvents];
	ETutorialEvent m_currentBriefingEvent;
	float m_entityCheckTimer;					// only check nearby entities every so often
	float m_baseCheckTimer;						// only check bases every so often
	bool m_wasInVehicle;
	float m_msgDisplayTime;						// how long the current message will be shown for

	// stored entity classes to prevent looking them up repeatedly
	IEntityClass* m_pHQClass;
	IEntityClass* m_pFactoryClass;
	IEntityClass* m_pAlienEnergyPointClass;
	IEntityClass* m_pPlayerClass;
	IEntityClass* m_pTankClass;
	IEntityClass* m_pTechChargerClass;
	IEntityClass* m_pSpawnGroupClass;
	IEntityClass* m_pSUVClass;

	// list of bases (HQ) for quicker checking later
	std::list<EntityId> m_baseList;
};


#endif // __MPTUTORIAL_H__