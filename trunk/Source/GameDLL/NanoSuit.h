/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Implements the Nanosuit.
  
 -------------------------------------------------------------------------
  History:
  - 22:11:2005: Created by Filippo De Luca
	- 31:01:2006: taken over by Jan Müller

*************************************************************************/
#ifndef __NANOSUIT_H__
#define __NANOSUIT_H__

#if _MSC_VER > 1000
# pragma once
#endif

static const float NANOSUIT_ENERGY								= 200.0f;
static const float NANOSUIT_HEALTH_REGEN_INTERVAL	= 1.0f;

enum ENanoSlot
{
	NANOSLOT_SPEED = 0,
	NANOSLOT_STRENGTH,
	NANOSLOT_MEDICAL,
	NANOSLOT_ARMOR,
	//
	NANOSLOT_LAST
};

// marcok: if you change this, also change g_USNanoMats and g_AsianNanoMats in nanosuit.cpp
enum ENanoMode
{
	NANOMODE_SPEED = 0,
	NANOMODE_STRENGTH,
	NANOMODE_CLOAK,
	NANOMODE_DEFENSE,
	//
	NANOMODE_LAST
};

enum ENanoCloakMode
{
	CLOAKMODE_CHAMELEON = 1,
	CLOAKMODE_REFRACTION,
	CLOAKMODE_REFRACTION_TEMPERATURE
};

enum ENanoSound
{
	NO_SOUND = 0,
	SPEED_SOUND,
	SPEED_IN_WATER_SOUND,
	SPEED_SOUND_STOP,
	SPEED_IN_WATER_SOUND_STOP,
	STRENGTH_SOUND,
	STRENGTH_LIFT_SOUND,
	STRENGTH_THROW_SOUND,
	STRENGTH_JUMP_SOUND,
	STRENGTH_MELEE_SOUND,
	MEDICAL_SOUND,
	ARMOR_SOUND,
	DROP_VS_THROW_SOUND,
	ESound_SuitStrengthActivate,
	ESound_SuitSpeedActivate,
	ESound_SuitArmorActivate,
	ESound_SuitCloakActivate,
	ESound_GBootsActivated,
	ESound_GBootsDeactivated,
	ESound_ZeroGThruster,
	ESound_GBootsLanded,
	ESound_FreeFall,
	ESound_ColdBreath,
	ESound_Suit_Last
};

struct SNanoSlot
{
	float desiredVal;
	float realVal;
};

class CNanoSuit;
struct SPlayerStats;

struct SNanoCloak
{
	friend class CNanoSuit;

	//
	SNanoCloak()
	{
		Reset();
	}

	void Reset()
	{
		//default is normal mode
		m_mode = CLOAKMODE_CHAMELEON;
		m_active = false;

		m_energyCost = 6.0f;	//this stuff is set from BasicActor.lua
		m_healthCost = 0.0f;

		m_visualDamp = 0.5f;
		m_soundDamp = 0.5f;
		m_heatDamp = 1.0f;

		m_HUDMessage.clear();
	}

	//
	//0 == cloak off, 1/2/3 == cloak mode 1, 2 or 3 (normal,alien tech,temperature camo)
	ILINE uint8 GetState() const  {return m_mode*m_active;}
	ILINE float GetVisualDamp() const {return m_active?m_visualDamp:1.0f;}
	ILINE float GetSoundDamp() const {return m_active?m_soundDamp:1.0f;}
	ILINE float GetHeatDamp() const {return m_active?m_heatDamp:1.0f;}
	ILINE bool IsActive() const { return m_active; }
	ILINE void SetType(ENanoCloakMode mode) { m_mode = mode; }
	ILINE ENanoCloakMode GetType() const { return m_mode; }
	void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(m_HUDMessage);
	}

private:

	void Update(CNanoSuit *pNano);

private:

	bool m_active;
	//1, 2 or 3 (normal,alien tech,temperature camo)
	ENanoCloakMode m_mode;

	float m_energyCost;//points per sec
	float m_healthCost;//health points per sec

	float m_visualDamp;//how much visibility
	float m_soundDamp;//how much of the AI sound is emitted
	float m_heatDamp;//alien visibility

	string m_HUDMessage;
};

class CPlayer;

class CNanoSuit
{
public:
	struct INanoSuitListener
	{
		virtual void ModeChanged(ENanoMode mode) = 0;     // nanomode
		virtual void EnergyChanged(float energy) = 0;     // energy
	};

	// nanosuit material to apply to player models when the mode changes
	struct SNanoMaterial
	{
		IMaterial*	body;
		IMaterial*	helmet;
		IMaterial*	arms;

		SNanoMaterial() : body(0), helmet(0), arms(0){}
	};

	CNanoSuit();
	~CNanoSuit();

	void Activate(bool activate, float activationTime = 0.0f);
	void ActivateMode(ENanoMode mode, bool active);
	void SetModeDefect(ENanoMode mode, bool defect);
  bool IsActive() const { return m_active; }
	bool IsModeActive(ENanoMode mode) const { return (m_featureMask & (1<<mode))?true:false; }
	void Init(CPlayer *owner) { Reset(owner); }
	void Reset(CPlayer *owner);
	void Update(float frameTime);
	void Serialize( TSerialize ser, unsigned aspects );
	void GetMemoryStatistics(ICrySizer * s);

	void Death();

	//setting
	void SetSuitEnergy(float value);
	bool SetMode(ENanoMode mode, bool forceUpdate = false);
	void SetCloakLevel(ENanoCloakMode mode); // currently 0 - 3
	void SetParams(SmartScriptTable &rTable,bool resetFirst);

	//getting
	ENanoMode GetMode() const { return m_currentMode; };
	float GetSlotValue(ENanoSlot slot,bool desired=false) const;
	bool	GetSoundIsPlaying(ENanoSound sound) const;
	bool	OnComboSpot() { return (m_energy > 0.1f * NANOSUIT_ENERGY && m_energy < 0.3f * NANOSUIT_ENERGY)?true:false; }
	void DeactivateSuit(float time);
	ILINE float GetSuitEnergy() { return m_energy; }
  ILINE float GetEnergyRechargeRate() { return m_energyRechargeRate; }
	ILINE float GetHealthRegenRate() { return m_healthRegenRate; }
	ILINE const CPlayer* GetOwner() const { return m_pOwner; }
	ILINE const SNanoCloak *GetCloak() { return &m_cloak; }
	ILINE bool IsNightVisionEnabled() const { return m_bNightVisionEnabled; };
	ILINE void EnableNightVision(bool enable) { m_bNightVisionEnabled=enable; };
	//get sprinting multiplier for ground movement
	float GetSprintMultiplier();

	//set energy back to max
	void ResetEnergy();
	//plays an ENanoSound
	void PlaySound(ENanoSound sound, float param = 1.0f, bool stopSound = false);

	// listener 
	void AddListener(INanoSuitListener* pListener);
	void RemoveListener(INanoSuitListener* pListener);

private:
	void Precache();
	int IDByName(char *slotStr);
	void Balance(float energy);
	bool SetAllSlots(float armor, float strength, float speed);
	void SetCloak(bool on, bool force=false);
	void SelectSuitMaterial();
	int  GetButtonFromMode(ENanoMode mode);
	void UpdateSprinting(float &recharge, SPlayerStats &stats, float frametime);

	IGameFramework *m_pGameFramework;
	
	CPlayer*			m_pOwner;
	SNanoMaterial*m_pNanoMaterial;

	SNanoCloak m_cloak;

	// basic variables
	float m_energy;
	float m_lastEnergy;
	float m_energyRechargeRate;
	float m_healthAccError;
	float m_healthRegenRate;
	float m_activationTime;
	bool m_active;
	bool m_bWasSprinting;
	bool m_bSprintUnderwater;

	bool m_bNightVisionEnabled;

	// timing helpers
	float m_now;
	float m_lastTimeUsedThruster;
	float m_healTime;
	float m_startedSprinting;

	//this is used as a mask for nanosuit features
	uint16 m_featureMask;

	ENanoMode m_currentMode;

	SNanoSlot m_slots[NANOSLOT_LAST];

	//sound playback
	float		m_fLastSoundPlayedMedical;
	tSoundID m_sounds[ESound_Suit_Last];

	std::vector<INanoSuitListener*> m_listeners;
};

#endif //__NANOSUIT_H__
