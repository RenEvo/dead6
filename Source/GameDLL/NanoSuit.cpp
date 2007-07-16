/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 22:11:2005: Created by Filippo De Luca
	- 31:01:2006: taken over by Jan Müller

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "Player.h" 
#include "GameUtils.h"
#include "HUD/HUD.h"
#include "GameRules.h"
#include "NetInputChainDebug.h"

#include <ISound.h>
#include <ISerialize.h>
#include <IGameTokens.h>
#include <IMaterialEffects.h>

CNanoSuit::SNanoMaterial g_USNanoMats[NANOSLOT_LAST];
CNanoSuit::SNanoMaterial g_AsianNanoMats[NANOSLOT_LAST];

void SNanoCloak::Update(CNanoSuit *pNano)
{
	if (!pNano || !pNano->GetOwner() || !pNano->GetOwner()->IsClient())
		return;

	CPlayer *pOwner = const_cast<CPlayer*>(pNano->GetOwner());

	//disable cloaking if health is too low (for the temperature camo) or suit energy goes too low (for any camo type I suppose)
	bool disableNormal(GetState() && pNano->GetSuitEnergy()<20);
	bool disableHeat(GetState()==3 && pOwner->GetHealth()<25);
	
	if (disableNormal || disableHeat)
	{
		CHUD *pHUD = g_pGame->GetHUD();
		if (pHUD)
		{
			string msg = "@" + m_HUDMessage;
			msg.append("_disabled");

			pHUD->TextMessage(msg);
			
			//FIXME:special message for the temperature cloak
			if (disableHeat)
			{
				pHUD->TextMessage("temperature_health_low");
			}
		}

		pNano->SetMode(NANOMODE_DEFENSE);
	}
}

//
CNanoSuit::CNanoSuit()
: m_pGameFramework(0)
, m_pNanoMaterial(0)
{
	for(int i = 0; i < ESound_Suit_Last; ++i)
		m_sounds[i] = 0;

	Reset(NULL);
}

CNanoSuit::~CNanoSuit()
{
}

void CNanoSuit::Reset(CPlayer *owner)
{
	m_healTime = 0;

	m_active = false;
	m_pOwner = owner;
	m_bWasSprinting = false;
	m_bSprintUnderwater = false;

	m_bNightVisionEnabled = false;

	ResetEnergy();

	m_energyRechargeRate = 0.0f;
	m_healthRegenRate = 0.0f;
	m_healthAccError = 0.0f;
	m_fLastSoundPlayedMedical = 0;
	m_startedSprinting = 0;
	m_now = 0;
	m_lastTimeUsedThruster = 0;

	for(int i=0; i<ESound_Suit_Last; ++i)
	{
		if(m_sounds[i])
		{
			if(gEnv->pSoundSystem)
				if(ISound *pSound = gEnv->pSoundSystem->GetSound(m_sounds[i]))
					pSound->Stop();
			m_sounds[i] = 0;
		}
	}

	//reset the cloaking
	m_cloak.Reset();

	m_currentMode = NANOMODE_DEFENSE;
	// needs to be set before call to SetMode
	m_featureMask = 15; //4 features with 3 flags each (0000000000001111)
	SetMode(m_currentMode, true);

	//ActivateMode(NANOMODE_CLOAK, false);
	ActivateMode(NANOMODE_STRENGTH, true);
	ActivateMode(NANOMODE_SPEED, true);
	ActivateMode(NANOMODE_DEFENSE, true);
	ActivateMode(NANOMODE_CLOAK, true);

	Precache();
}

void CNanoSuit::SetParams(SmartScriptTable &rTable,bool resetFirst)
{
	//
	int mode = 1;
	rTable->GetValue("cloakType", mode);
	m_cloak.m_mode = ENanoCloakMode(mode);	
	rTable->GetValue("cloakEnergyCost",m_cloak.m_energyCost);
	rTable->GetValue("cloakHealthCost",m_cloak.m_healthCost);
	rTable->GetValue("cloakVisualDamp",m_cloak.m_visualDamp);
	rTable->GetValue("cloakSoundDamp",m_cloak.m_soundDamp);
	rTable->GetValue("cloakHeatDamp",m_cloak.m_heatDamp);
	
	const char *pHUDMessage;
	if (rTable->GetValue("cloakHudMessage",pHUDMessage))
		m_cloak.m_HUDMessage = string(pHUDMessage);
}

void CNanoSuit::SetCloakLevel(ENanoCloakMode mode)
{
	// Currently only on/off supported!
	ENanoCloakMode oldMode = m_cloak.GetType();
	m_cloak.SetType(mode);
	if(oldMode != mode && m_cloak.IsActive())
	{
		SetCloak(false, true);
		SetCloak(true, true);
	}
}

void CNanoSuit::Update(float frameTime)
{
	if (!m_active || !m_pOwner || m_pOwner->GetHealth()<=0)
		return;

	// the suit can take some time to power up
	if (m_active && m_activationTime>0.0f)
	{
		m_activationTime-=frameTime;
		if (m_activationTime > 0.0f)
			return;
		CHUD* pHUD = g_pGame->GetHUD();
		if (pHUD)
			pHUD->RebootHUD();
	}

	bool isServer=gEnv->bServer;

	bool isAI = (m_pOwner != m_pGameFramework->GetClientActor()) && !gEnv->bMultiplayer;

	//update health
	int32 currentHealth = m_pOwner->GetHealth();
	int32 maxHealth(m_pOwner->GetMaxHealth());
	float recharge = 0.0f;
	if (isAI)
		recharge = NANOSUIT_ENERGY / max(0.01f, g_pGameCVars->g_AiSuitEnergyRechargeTime);
	else
		recharge = NANOSUIT_ENERGY / max(0.01f, g_pGameCVars->g_playerSuitEnergyRechargeTime);

	m_energyRechargeRate = recharge;

	m_now = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();

	if (currentHealth < maxHealth || m_cloak.m_active)
	{
		if(m_cloak.m_active)
			currentHealth = min(currentHealth, 98);

		//check for low health and play sound
		if(currentHealth < maxHealth*0.9f && GetSlotValue(NANOSLOT_MEDICAL, true) > 50)
		{
			if(m_now - m_fLastSoundPlayedMedical > 30000.0f)
			{
				m_fLastSoundPlayedMedical = m_now;
				PlaySound(MEDICAL_SOUND);
			}
		}

		if (isAI)
			m_healthRegenRate = maxHealth / max(0.01f, g_pGameCVars->g_AiSuitHealthRegenTime);
		else
			m_healthRegenRate = maxHealth / max(0.01f, g_pGameCVars->g_playerSuitHealthRegenTime);

		if(m_currentMode == NANOMODE_DEFENSE) //some additional energy in defense mode
		{
			if (isAI)
				m_healthRegenRate = maxHealth / max(0.01f, g_pGameCVars->g_AiSuitArmorModeHealthRegenTime);
			else
				m_healthRegenRate = maxHealth / max(0.01f, g_pGameCVars->g_playerSuitArmorModeHealthRegenTime);
		}

		m_healthRegenRate -= (m_cloak.m_active?m_cloak.m_healthCost:0.0f);
	}

	SPlayerStats stats = *(static_cast<SPlayerStats*>(m_pOwner->GetActorStats()));
	
	//subtract energy from suit for cloaking
	if(m_cloak.m_active)
	{
		float energyCost = m_cloak.m_energyCost * g_pGameCVars->g_suitCloakEnergyDrainAdjuster;
		recharge = min(recharge-max(1.0f, energyCost*(stats.speedFlat * 0.5f)),-max(1.0f, energyCost*(stats.speedFlat * 0.5f)));
	}

	//this deals with sprinting
	UpdateSprinting(recharge, stats, frameTime);

	NETINPUT_TRACE(m_pOwner->GetEntityId(), recharge);
	NETINPUT_TRACE(m_pOwner->GetEntityId(), m_energy);
	SetSuitEnergy(Clamp(m_energy + recharge*frameTime, 0.0f, NANOSUIT_ENERGY));

	for (int i=0;i<NANOSLOT_LAST;++i)
		m_slots[i].realVal = m_slots[i].desiredVal;

	if (isServer)
	{
		//adjust the player health.
		m_healTime -= frameTime;
		if (m_healTime < 0.0f)
		{
			m_healTime += NANOSUIT_HEALTH_REGEN_INTERVAL;

			// Calculate the new health increase
			float healthInc = m_healthAccError + m_healthRegenRate * NANOSUIT_HEALTH_REGEN_INTERVAL;
			int healthIncInt = (int32)healthInc;
			// Since the health is measured as integer, carry on the fractions for the next addition
			// to get more accurate result in the health regeneration rate.
			m_healthAccError = healthInc - healthIncInt;

			int newHealth = min(maxHealth,(int32)(currentHealth + healthIncInt));
			if (currentHealth != newHealth)
				m_pOwner->SetHealth(newHealth);
		}
	}

	if (m_energy!=m_lastEnergy)
	{
		if (isServer)
			m_pOwner->GetGameObject()->ChangedNetworkState(CPlayer::ASPECT_NANO_SUIT_ENERGY);

		// call listeners on nano energy change
		if (m_listeners.empty() == false)
		{
			std::vector<INanoSuitListener*>::iterator iter = m_listeners.begin();
			while (iter != m_listeners.end())
			{
				(*iter)->EnergyChanged(m_energy);
				++iter;
			}
		}
		//CryLogAlways("[nano]-- updating %s's nanosuit energy: %f", m_pOwner->GetEntity()->GetName(), m_energy);
	}

	Balance(m_energy);
	NETINPUT_TRACE(m_pOwner->GetEntityId(), m_slots[NANOSLOT_SPEED].realVal);
	NETINPUT_TRACE(m_pOwner->GetEntityId(), m_slots[NANOSLOT_SPEED].desiredVal);

	m_cloak.Update(this);

	//update object motion blur amount
	float motionBlurAmt(0.0f);
	if (m_currentMode == NANOMODE_SPEED)
		motionBlurAmt = 1.0f;

	IEntityRenderProxy * pRenderProxy = (IEntityRenderProxy*)m_pOwner->GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
	if (pRenderProxy && stats.bSprinting)
	{ 
		float amt(pRenderProxy->GetMotionBlurAmount());
		amt += (motionBlurAmt - amt) * frameTime * 3.3f;
		pRenderProxy->SetMotionBlurAmount(amt);
	}
			
	CItem *currentItem = (CItem *)gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_pOwner->GetInventory()->GetCurrentItem());
	if (currentItem)
	{
		pRenderProxy = (IEntityRenderProxy*)currentItem->GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
		if (pRenderProxy)
		{
			float amt(pRenderProxy->GetMotionBlurAmount());
			amt += (motionBlurAmt - amt) * frameTime * 3.3f;
			pRenderProxy->SetMotionBlurAmount(amt);
		}
	}

	m_lastEnergy = m_energy;
}

void CNanoSuit::Balance(float energy)
{
	for(int i = 0; i < NANOSLOT_LAST; i++)
	{
		float slotPerCent = m_slots[i].desiredVal / NANOSUIT_ENERGY; //computes percentage for NANOSUIT_ENERGY total ...
			m_slots[i].realVal = energy * slotPerCent;
	}
}

void CNanoSuit::SetSuitEnergy(float value)
{
	value=min(max(value,0.0f), NANOSUIT_ENERGY);
	if (m_pOwner && value!=m_energy && gEnv->bServer)
		m_pOwner->GetGameObject()->ChangedNetworkState(CPlayer::ASPECT_NANO_SUIT_ENERGY);

	if (value != m_energy)
	{
		// call listeners on nano energy change
		if (m_listeners.empty() == false)
		{
			std::vector<INanoSuitListener*>::iterator iter = m_listeners.begin();
			while (iter != m_listeners.end())
			{
				(*iter)->EnergyChanged(value);
				++iter;
			}
		}
	}

	//armor mode hit fx (in armor mode energy is decreased by damage
	if(value < m_energy && (m_energy - value) > 20.0f)
	{
		if(m_pOwner && !m_pOwner->IsGod() && !m_pOwner->IsThirdPerson() && m_currentMode == NANOMODE_DEFENSE)
		{
			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
			SMFXRunTimeEffectParams params;
			params.pos = m_pOwner->GetEntity()->GetWorldPos();
			TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", "player_damage_armormode");
			pMaterialEffects->ExecuteEffect(id, params);
		}
	}

	m_energy = value;
}

bool CNanoSuit::SetAllSlots(float armor, float strength, float speed)
{
	float energy = armor + strength + speed;
	if(energy > NANOSUIT_ENERGY)
		return false;
	m_slots[NANOSLOT_ARMOR].desiredVal = armor;
	m_slots[NANOSLOT_STRENGTH].desiredVal = strength;
	m_slots[NANOSLOT_SPEED].desiredVal = speed;
	m_slots[NANOSLOT_MEDICAL].desiredVal = NANOSUIT_ENERGY - energy;

	return true;
}

bool CNanoSuit::SetMode(ENanoMode mode, bool forceUpdate)
{
	static ICVar* time_scale = gEnv->pConsole->GetCVar("time_scale");
	if(m_currentMode == mode && !forceUpdate)
		return false;

	const char* effectName = "";

	ENanoMode lastMode = m_currentMode;

	switch(mode)
	{
		case NANOMODE_SPEED:
			if(!(m_featureMask & (1<<NANOMODE_SPEED)) && !forceUpdate)
				return false;
			m_currentMode = mode;
			SetAllSlots(25.0f, 50.0f, 100.0f);
			if(!forceUpdate)
				PlaySound(ESound_SuitSpeedActivate);
			SetCloak(false);
			effectName = "suit_speedmode";
			//marcok: don't touch please
			if (g_pGameCVars->cl_tryme && g_pGameCVars->cl_tryme_bt_speed && !g_pGameCVars->cl_tryme_bt_ironsight)
			{
				time_scale->Set(0.2f);
			}
			break;
		case NANOMODE_STRENGTH:
			if(!(m_featureMask & (1<<NANOMODE_STRENGTH)) && !forceUpdate)
				return false;
			m_currentMode = mode;
			SetAllSlots(50.0f, 100.0f, 25.0f);
			if(!forceUpdate)
				PlaySound(ESound_SuitStrengthActivate);
			SetCloak(false);
			effectName = "suit_strengthmode";
			break;
		case NANOMODE_DEFENSE:
			if(!(m_featureMask & (1<<NANOMODE_DEFENSE)) && !forceUpdate) //this is the default mode and should always be available
				return false;
			m_currentMode = mode;	
			SetAllSlots(75.0f, 25.0f, 25.0f);
			if(!forceUpdate)
				PlaySound(ESound_SuitArmorActivate);
			SetCloak(false);
			effectName = "suit_armormode";
			break;
		case NANOMODE_CLOAK:
			if(!(m_featureMask & (1<<NANOMODE_CLOAK)) && !forceUpdate)
				return false;
			m_currentMode = mode;
			SetAllSlots(50.0f, 50.0f, 50.0f);
			if(!forceUpdate)
				PlaySound(ESound_SuitCloakActivate);
			SetCloak(true);
			effectName = "suit_cloakmode";
			break;
		default:
			assert(0);
			GameWarning("Non existing NANOMODE selected: %d", mode);
			return false;
	}

	//marcok: don't touch please
	if (g_pGameCVars->cl_tryme && g_pGameCVars->cl_tryme_bt_speed && !g_pGameCVars->cl_tryme_bt_ironsight)
	{
		if (lastMode != m_currentMode)
		{
			if (lastMode == NANOMODE_SPEED)
					time_scale->Set(1.0f);
		}
	}

	if(m_pOwner)
	{
		if (gEnv->bServer)
			m_pGameFramework->GetIGameplayRecorder()->Event(m_pOwner->GetEntity(), GameplayEvent(eGE_SuitModeChanged, 0, (float)mode));

		//draw some screen effect
		if(m_pOwner == m_pGameFramework->GetClientActor() && !m_pOwner->IsThirdPerson())
		{
			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
			SMFXRunTimeEffectParams params;
			params.pos = m_pOwner->GetEntity()->GetWorldPos();
			TMFXEffectId id = pMaterialEffects->GetEffectIdByName("player_fx", effectName);
			pMaterialEffects->ExecuteEffect(id, params);
		}
	}

	// call listeners on nano mode change
	if (m_listeners.empty() == false)
	{
		std::vector<INanoSuitListener*>::iterator iter = m_listeners.begin();
		while (iter != m_listeners.end())
		{
			(*iter)->ModeChanged(mode);
			++iter;
		}
	}

	SelectSuitMaterial();

	if (m_pOwner)
		m_pOwner->GetGameObject()->ChangedNetworkState(CPlayer::ASPECT_NANO_SUIT_SETTING);

	// player's squadmates mimicking nanosuit modifications
	if (gEnv->pAISystem)
	{
		IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();//AI System will be the owner of this data
		pData->iValue = mode;
		if(m_pOwner && m_pOwner->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1,"OnNanoSuitMode",m_pOwner->GetEntity()->GetAI(),pData);
	}

	// Report cloak usage to AI system.
	if (lastMode == NANOMODE_CLOAK && m_currentMode != NANOMODE_CLOAK)
	{
		if (GetOwner()->GetEntity() && GetOwner()->GetEntity()->GetAI())
			GetOwner()->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_UNCLOAK, 0);
	}
	if (lastMode != NANOMODE_CLOAK && m_currentMode == NANOMODE_CLOAK)
	{
		if (GetOwner()->GetEntity() && GetOwner()->GetEntity()->GetAI())
			GetOwner()->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_CLOAK, 0);
	}

	return true;
}

void CNanoSuit::SetCloak(bool on, bool force)
{
	bool switched(m_cloak.m_active!=on);

	m_cloak.m_active = on;

	if (switched || force)
	{
		if (m_pOwner)
		{
			ENanoCloakMode cloakMode = m_cloak.GetType();
			if(cloakMode == CLOAKMODE_REFRACTION || cloakMode == CLOAKMODE_REFRACTION_TEMPERATURE)
				m_pOwner->ReplaceMaterial(on?"Materials/presets/materialtypes/clean/mat_cloak":NULL);
			else if(!on)
				m_pOwner->ReplaceMaterial(NULL);

			m_pOwner->CreateScriptEvent("cloaking",on?cloakMode:0);
								
			// player's squadmates mimicking nanosuit modifications
			if (m_pOwner->GetEntity()->GetAI())
				gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1, (on?"OnNanoSuitCloak":"OnNanoSuitUnCloak"),m_pOwner->GetEntity()->GetAI());
		}
	}
}

void CNanoSuit::SelectSuitMaterial()
{
	if(!m_pOwner)
		return;

	if(m_currentMode == NANOMODE_CLOAK && m_cloak.GetType() != CLOAKMODE_CHAMELEON)
		return;

  int team = 0;
  
  CGameRules* pGameRules = g_pGame->GetGameRules();
  if (pGameRules)
	  team = pGameRules->GetTeam(m_pOwner->GetEntityId());

	SEntitySlotInfo slotInfo;
	if(m_pOwner->GetEntity()->GetSlotInfo(0, slotInfo) && slotInfo.pCharacter!=0)
	{
		SNanoMaterial* pNanoMat = &m_pNanoMaterial[m_currentMode];

		// this should be the legs of the character
		slotInfo.pCharacter->SetMaterial(pNanoMat->body);
		IAttachmentManager* pMan = slotInfo.pCharacter->GetIAttachmentManager();

		IAttachment* pAttachment = pMan->GetInterfaceByName("upper_body");
		if (pAttachment)
		{
			IAttachmentObject* pAttachmentObj = pAttachment->GetIAttachmentObject();
			if (pAttachmentObj)
			{
				ICharacterInstance* pCharInstance = pAttachmentObj->GetICharacterInstance();
				if (pCharInstance)
					pCharInstance->SetMaterial(pNanoMat->body);
			}
		}

		pAttachment = pMan->GetInterfaceByName("helmet");
		if (pAttachment)
		{
			IAttachmentObject* pAttachmentObj = pAttachment->GetIAttachmentObject();
			if (pAttachmentObj)
			{
				ICharacterInstance* pCharInstance = pAttachmentObj->GetICharacterInstance();
				if (pCharInstance)
					pCharInstance->SetMaterial(pNanoMat->helmet);
			}
		}

		// arms ... these indices are a bit hacky
		if (m_pOwner->GetEntity()->GetSlotInfo(3, slotInfo) && slotInfo.pCharacter!=0)
		{
			slotInfo.pCharacter->SetMaterial(pNanoMat->arms);
		}
		// second set of arms for dual socom
		if (m_pOwner->GetEntity()->GetSlotInfo(4, slotInfo) && slotInfo.pCharacter!=0)
		{
			slotInfo.pCharacter->SetMaterial(pNanoMat->arms);
		}
	}
}

void CNanoSuit::Precache()
{
	m_pNanoMaterial = g_USNanoMats;
	bool cacheAsian = false;

	SEntitySlotInfo slotInfo;
	if(m_pOwner && m_pOwner->GetEntity()->GetSlotInfo(0, slotInfo) && slotInfo.pCharacter!=0)
	{
		// default are US suits
		// have to do this hacky check, because we don't have "teams" in singleplayer
		const char* filePath = slotInfo.pCharacter->GetModelFilePath();
		if (strstr(filePath, "/us/") == NULL)
		{
			m_pNanoMaterial = g_AsianNanoMats;
			cacheAsian = true;
		}
	}

	m_pGameFramework = g_pGame->GetIGameFramework();

	// preload materials
	IMaterialManager* matMan = gEnv->p3DEngine->GetMaterialManager();
	if (!g_USNanoMats[NANOMODE_SPEED].body)
	{
		g_USNanoMats[NANOMODE_SPEED].body = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_speed.mtl");
		g_USNanoMats[NANOMODE_SPEED].helmet = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_helmet_speed.mtl");
		g_USNanoMats[NANOMODE_SPEED].arms = matMan->LoadMaterial( "objects/weapons/arms_global/arms_nanosuit_us_speed.mtl");
		g_USNanoMats[NANOMODE_STRENGTH].body = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_strength.mtl");
		g_USNanoMats[NANOMODE_STRENGTH].helmet = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_helmet_strength.mtl");
		g_USNanoMats[NANOMODE_STRENGTH].arms = matMan->LoadMaterial( "objects/weapons/arms_global/arms_nanosuit_us_strength.mtl");
		g_USNanoMats[NANOMODE_CLOAK].body = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_cloak.mtl");
		g_USNanoMats[NANOMODE_CLOAK].helmet = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_helmet_cloak.mtl");
		g_USNanoMats[NANOMODE_CLOAK].arms = matMan->LoadMaterial( "objects/weapons/arms_global/arms_nanosuit_us_cloak.mtl");
		g_USNanoMats[NANOMODE_DEFENSE].body = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us.mtl");
		g_USNanoMats[NANOMODE_DEFENSE].helmet = matMan->LoadMaterial("objects/characters/human/us/nanosuit/nanosuit_us_helmet.mtl");
		g_USNanoMats[NANOMODE_DEFENSE].arms = matMan->LoadMaterial( "objects/weapons/arms_global/arms_nanosuit_us.mtl");
		// strategically leak it
		for (int i=0; i<NANOMODE_LAST; ++i)
		{
			g_USNanoMats[i].body->AddRef();
			g_USNanoMats[i].helmet->AddRef();
			g_USNanoMats[i].arms->AddRef();
		}
	}


	if (cacheAsian && !g_AsianNanoMats[NANOMODE_SPEED].body)
	{
		g_AsianNanoMats[NANOMODE_SPEED].body = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_speed.mtl");
		g_AsianNanoMats[NANOMODE_SPEED].helmet = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_helmet_speed.mtl");
		g_AsianNanoMats[NANOMODE_SPEED].arms = matMan->LoadMaterial("objects/weapons/arms_global/arms_nanosuit_asian_speed.mtl");
		g_AsianNanoMats[NANOMODE_STRENGTH].body = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_strength.mtl");
		g_AsianNanoMats[NANOMODE_STRENGTH].helmet = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_helmet_strength.mtl");
		g_AsianNanoMats[NANOMODE_STRENGTH].arms = matMan->LoadMaterial("objects/weapons/arms_global/arms_nanosuit_asian_strength.mtl");
		g_AsianNanoMats[NANOMODE_CLOAK].body = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_cloak.mtl");
		g_AsianNanoMats[NANOMODE_CLOAK].helmet = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_helmet_cloak.mtl");
		g_AsianNanoMats[NANOMODE_CLOAK].arms = matMan->LoadMaterial("objects/weapons/arms_global/arms_nanosuit_asian_cloak.mtl");
		g_AsianNanoMats[NANOMODE_DEFENSE].body = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian.mtl");
		g_AsianNanoMats[NANOMODE_DEFENSE].helmet = matMan->LoadMaterial("objects/characters/human/asian/nanosuit/nanosuit_asian_helmet.mtl");
		g_AsianNanoMats[NANOMODE_DEFENSE].arms = matMan->LoadMaterial("objects/weapons/arms_global/arms_nanosuit_asian.mtl");
		// strategically leak it
		for (int i=0; i<NANOMODE_LAST; ++i)
		{
			g_AsianNanoMats[i].body->AddRef();
			g_AsianNanoMats[i].helmet->AddRef();
			g_AsianNanoMats[i].arms->AddRef();
		}
	}
}

int CNanoSuit::IDByName(char *slotStr)
{
	if (!strcmp(slotStr,"speed"))
		return NANOSLOT_SPEED;
	else if (!strcmp(slotStr,"armor"))
		return NANOSLOT_ARMOR;
	else if (!strcmp(slotStr,"strength"))
		return NANOSLOT_STRENGTH;
	else if (!strcmp(slotStr,"medical"))
		return NANOSLOT_MEDICAL;
	else
		return NANOSLOT_LAST;
}

float CNanoSuit::GetSlotValue(ENanoSlot slot,bool desired) const
{
	if (m_active && slot>=0 && slot<NANOSLOT_LAST)
		return (desired?m_slots[slot].desiredVal:m_slots[slot].realVal);

	return 0.0f;
}

bool CNanoSuit::GetSoundIsPlaying(ENanoSound sound) const
{
	if(!m_active || !gEnv->pGame->GetIGameFramework()->IsGameStarted())
		return false;

	if(m_sounds[sound] && gEnv->pSoundSystem)
	{
		ISound *pSound = gEnv->pSoundSystem->GetSound(m_sounds[sound]);
		if(pSound)
			return pSound->IsPlaying();
	}
	return false;
}

void CNanoSuit::DeactivateSuit(float time)
{
	SetSuitEnergy(0);
}

void CNanoSuit::PlaySound(ENanoSound sound, float param, bool stopSound)
{
	if(!gEnv->pSoundSystem || !m_pOwner || !m_active)
		return;

	int soundFlag = FLAG_SOUND_3D; //localActor will get 2D sounds
	ISound *pSound = NULL;
	bool	repeating = false;
	bool	setParam = false;
	bool	force3DSound = false;
	bool	bAppendPostfix=true;
	static string soundName;
  soundName.resize(0);

	switch(sound)
	{
	case SPEED_SOUND:
		soundName = "Sounds/interface:suit:suit_speed_use";
		repeating = true;
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 0.0f, 0.6f) );
		break;
	case SPEED_IN_WATER_SOUND:
		soundName = "Sounds/interface:suit:suit_speed_use_underwater";
		repeating = true;
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 0.0f, 0.9f) );
		break;
	case SPEED_SOUND_STOP:
		soundName = "Sounds/interface:suit:suit_speed_stop";
		break;
	case SPEED_IN_WATER_SOUND_STOP:
		soundName = "Sounds/interface:suit:suit_speed_stop_underwater";
		break;
	case STRENGTH_SOUND:
		soundName = "Sounds/interface:suit:suit_strength_use";
		setParam = true;
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 1.0f, 0.5f) );
		break;
	case STRENGTH_LIFT_SOUND:
		soundName = "Sounds/interface:suit:suit_strength_lift";
		setParam = true;
		break;
	case STRENGTH_THROW_SOUND:
		soundName = "Sounds/interface:suit:suit_strength_use";
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.1f, 0.0f, 0.3f*param) );
		setParam = true;
		break;
	case STRENGTH_JUMP_SOUND:
		soundName = "Sounds/interface:suit:suit_strength_jump";
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.10f, 0.2f*param, 0.1f*param) );
		setParam = true;
		break;
	case STRENGTH_MELEE_SOUND:
		soundName = "Sounds/interface:suit:suit_strength_punch";
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 1.0f*param, 0.5f*param) );
		setParam = true;
		break;
	case ARMOR_SOUND:
		soundName = "Sounds/interface:suit:suit_armor_use";
		if(m_pOwner->IsClient())
			gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.02f, 0.8f, 0.0f) );
		break;
	case MEDICAL_SOUND:
		soundName = "Sounds/interface:suit:suit_medical_repair";
		break;
	case ESound_SuitStrengthActivate:
		soundName = "Sounds/interface:suit:suit_strength_activate";
		break;
	case ESound_SuitSpeedActivate:
		soundName = "Sounds/interface:suit:suit_speed_activate";
		break;
	case ESound_SuitArmorActivate:
		soundName = "Sounds/interface:suit:suit_armor_activate";
		break;
	case ESound_SuitCloakActivate:
		soundName = "Sounds/interface:suit:suit_cloak_activate";
		break;
	case ESound_GBootsActivated:
		soundName = "Sounds/interface:suit:suit_gravity_boots_activate";
		break;
	case ESound_GBootsDeactivated:
		soundName = "Sounds/interface:suit:suit_gravity_boots_deactivate";
		break;
	case ESound_ZeroGThruster:
		soundName = "Sounds/interface:suit:thrusters_use";
		setParam = true;
		force3DSound = true;	//the thruster sound is only as 3D version available
		repeating = true;
		break;
	case ESound_GBootsLanded:
		soundName = "Sounds/physics:player_foley:bodyfall_gravity_boots";
		force3DSound = true;
		setParam = true;
		break;
	case ESound_FreeFall:
		soundName = "Sounds/physics:player_foley:falling_deep_loop";
		bAppendPostfix=false;
		repeating = true;
		break;
	case ESound_ColdBreath:
		soundName = "Sounds/physics:player_foley:cold_feedback";
		bAppendPostfix=false;
		break;
	case DROP_VS_THROW_SOUND:
		soundName = "sounds/interface:suit:suit_grab_vs_throw";
		bAppendPostfix = false;
		repeating = false;
	default:
		break;
	}

	if(!force3DSound && m_pOwner == m_pGameFramework->GetClientActor() && !m_pOwner->IsThirdPerson() && soundName.size())
	{
		if (bAppendPostfix)
			soundName.append("_fp");
		soundFlag = FLAG_SOUND_2D|FLAG_SOUND_START_PAUSED;
	}

	if(soundName.size())		//get / create or stop sound
	{
		if(m_sounds[sound])
		{
			pSound = gEnv->pSoundSystem->GetSound(m_sounds[sound]);
			if(stopSound)
			{
				if(pSound)
					pSound->Stop();
				m_sounds[sound] = 0;
				return;
			}
		}
		if(!pSound && !stopSound)
			pSound = gEnv->pSoundSystem->CreateSound(soundName, soundFlag);
	}

	if ( pSound )		//set params and play
	{
		pSound->SetPosition(m_pOwner->GetEntity()->GetWorldPos());

		if(!(repeating && pSound->IsPlaying()))
		{
			pSound->Play();
			m_sounds[sound] = pSound->GetId();
		}

		if(setParam)
		{
			if(sound == STRENGTH_SOUND ||sound == STRENGTH_LIFT_SOUND || sound == STRENGTH_THROW_SOUND)
				pSound->SetParam("mass", param);
			else if(sound == ESound_ZeroGThruster || sound == ESound_GBootsLanded)
				pSound->SetParam("speed", param);
			else
				pSound->SetParam("strength", param);
		}

		pSound->SetPaused(false);
	}
}

void CNanoSuit::Serialize(TSerialize ser, unsigned aspects)
{
	if (ser.GetSerializationTarget() != eST_Network)
	{
		ser.BeginGroup("Nanosuit");
		ser.Value("nanoSuitEnergy", m_energy);
		ser.Value("nanoSuitActive", m_active);
		ser.Value("m_activationTime", m_activationTime);
		ser.Value("m_energyRechargeRate", m_energyRechargeRate);
		ser.Value("m_healthRegenRate", m_healthRegenRate);
		ser.Value("m_healthAccError", m_healthAccError);
		ser.Value("m_healTime", m_healTime);
		ser.Value("m_featureMask", m_featureMask);
		ser.EnumValue("currentMode", m_currentMode, NANOMODE_SPEED, NANOMODE_LAST);
		if(ser.IsReading())
			SetMode(m_currentMode, true);
		ser.EndGroup();

		ser.BeginGroup("NanoCloak");
		ser.Value("CloakActive", m_cloak.m_active);
		int mode = m_cloak.m_mode;
		ser.Value("CloakType", mode);
		ser.Value("CloakEnergyCost", m_cloak.m_energyCost);
		ser.Value("CloakHealthCost", m_cloak.m_healthCost);
		ser.Value("CloakVisDamp", m_cloak.m_visualDamp);
		ser.Value("CloakSoundDamp", m_cloak.m_soundDamp);
		ser.Value("CloakHeatDamp", m_cloak.m_heatDamp);
		ser.Value("HudMessage", m_cloak.m_HUDMessage);

		ser.Value("m_bNightVisionEnabled", m_bNightVisionEnabled);

		if(ser.IsReading())
		{
			if(m_pOwner)
				m_pOwner->ReplaceMaterial(m_cloak.m_active?"Materials/presets/materialtypes/clean/mat_cloak":NULL);

			m_cloak.m_mode = ENanoCloakMode(mode);
			if(m_cloak.IsActive())
			{
				SetCloak(false, true);
				SetCloak(true, true);
			}
		}
		ser.EndGroup();

	}
	else 
	{
		if (aspects&CPlayer::ASPECT_NANO_SUIT_SETTING)
		{
			uint8 mode = m_currentMode;
			ser.Value("mode", mode, 'ui3');
			if(ser.IsReading() && (mode != m_currentMode))
				SetMode((ENanoMode)mode);
		}
		if (aspects&CPlayer::ASPECT_NANO_SUIT_ENERGY)
		{
			ser.Value("energy", m_energy, 'nNRG');
			if (ser.IsReading())
				Balance(m_energy);
			/*float energy = m_energy;
			ser.Value("energy", energy, 'nNRG');
			m_energy = energy;
			Balance(m_energy);*/
		}
	}
}

void CNanoSuit::Death()
{
	if (m_bWasSprinting)
	{
		if (m_bSprintUnderwater)
			PlaySound(SPEED_IN_WATER_SOUND, 0.0f, true);
		else
			PlaySound(SPEED_SOUND, 0.0f, true);

		if (m_bSprintUnderwater)
			PlaySound(SPEED_IN_WATER_SOUND_STOP);
		else
			PlaySound(SPEED_SOUND_STOP);
	}
}

void CNanoSuit::AddListener(CNanoSuit::INanoSuitListener *pListener)
{
	stl::push_back_unique(m_listeners, pListener);
}

void CNanoSuit::RemoveListener(CNanoSuit::INanoSuitListener *pListener)
{
	stl::find_and_erase(m_listeners, pListener);
}

int CNanoSuit::GetButtonFromMode(ENanoMode mode)
{
	switch(mode)
	{
	case NANOMODE_SPEED:
		return EQM_SPEED;
		break;
	case NANOMODE_STRENGTH:
		return EQM_STRENGTH;
		break;
	case NANOMODE_CLOAK:
		return EQM_CLOAK;
		break;
	case NANOMODE_DEFENSE:
		return EQM_ARMOR;
		break;
	default:
		break;
	}
	return EQM_ARMOR;
}

void  CNanoSuit::Activate(bool activate, float activationTime)
{
	m_active = activate;
	m_activationTime = activationTime;
}


void CNanoSuit::ActivateMode(ENanoMode mode, bool active)
{
	if(!m_pOwner)
		return;

	if(active)
	{
		CHUD* pHUD = g_pGame->GetHUD();
		if(pHUD && m_pOwner->IsClient() && !pHUD->IsQuickMenuButtonDefect(EQuickMenuButtons(GetButtonFromMode(mode))))
		{
			m_featureMask |= 1<<mode;
			pHUD->ActivateQuickMenuButton(EQuickMenuButtons(GetButtonFromMode(mode)), true);
		}
	}
	else if(!active)
	{
		m_featureMask &= ~(1<<mode);
		CHUD* pHUD = g_pGame->GetHUD();
		if(pHUD && m_pOwner->IsClient())
			pHUD->ActivateQuickMenuButton(EQuickMenuButtons(GetButtonFromMode(mode)), false);
	}
}

void CNanoSuit::SetModeDefect(ENanoMode mode, bool defect)
{

	CHUD* pHUD = g_pGame->GetHUD();
	if(pHUD && m_pOwner->IsClient())
		pHUD->SetQuickMenuButtonDefect(EQuickMenuButtons(GetButtonFromMode(mode)), defect);

	if(defect)
	{
		if(IsModeActive(mode))
			ActivateMode(mode, false);
	}
	else
	{
		if(!IsModeActive(mode))
			ActivateMode(mode, true);
	}
}

float CNanoSuit::GetSprintMultiplier()
{
	if(m_pOwner && !m_pOwner->GetActorStats()->inZeroG && m_currentMode == NANOMODE_SPEED && m_startedSprinting)
	{
		if(m_energy > NANOSUIT_ENERGY * 0.2f)
		{
			float time = m_now - m_startedSprinting;
			float speedMult = (gEnv->bMultiplayer)?g_pGameCVars->g_suitSpeedMultMultiplayer:g_pGameCVars->g_suitSpeedMult;
			return 1.0f + max(0.3f, g_pGameCVars->g_suitSpeedMult*min(1.0f, time*0.001f));
		}
		else if(m_energy > 0.0f)
			return 1.3f;
	}
	return 1.0f;
}

void CNanoSuit::UpdateSprinting(float &recharge, SPlayerStats &stats, float frametime)
{
	if(!stats.inZeroG)
	{
		if (m_currentMode == NANOMODE_SPEED && stats.bSprinting)
		{
			if(m_energy > NANOSUIT_ENERGY * 0.2f)
			{
				if(!m_bWasSprinting)
				{
					m_bWasSprinting = true;
					if(stats.headUnderWater < 0.0f)
					{
						if(m_pOwner->GetStance() != STANCE_PRONE)
							PlaySound(SPEED_SOUND);

						m_bSprintUnderwater = false;
					}
					else
					{
						PlaySound(SPEED_IN_WATER_SOUND);
						m_bSprintUnderwater = true;
					}
				}
				else
				{
					//when we sprinted into the water -> change sound
					if((stats.headUnderWater > 0.0f) && m_bSprintUnderwater)
					{
						PlaySound(SPEED_IN_WATER_SOUND, 0.0, true);
						PlaySound(SPEED_SOUND);
					}
					else if((stats.headUnderWater > 0.0f) && !m_bSprintUnderwater)
					{
						PlaySound(SPEED_SOUND, 0.0, true);
						PlaySound(SPEED_IN_WATER_SOUND);
					}
				}

				//recharge -= std::max(1.0f, g_pGameCVars->g_suitSpeedEnergyConsumption*frametime);
				recharge -= (stats.headUnderWater > 0.0f)?(g_pGameCVars->g_suitSpeedEnergyConsumption):(g_pGameCVars->g_suitSpeedEnergyConsumption*0.75f);
			}
			else
			{
				if(m_bWasSprinting)
				{
					PlaySound(SPEED_SOUND, 0.0f, true);
					PlaySound(SPEED_IN_WATER_SOUND, 0.0f, true);
					if(stats.headUnderWater < 0.0f)
						PlaySound(SPEED_SOUND_STOP);
					else
						PlaySound(SPEED_IN_WATER_SOUND_STOP);
					m_bWasSprinting = false;
				}
				recharge -= 28.0f;
			}

			if(!m_startedSprinting)
				m_startedSprinting = m_now;
		}
		else if(m_bWasSprinting)
		{
			PlaySound(SPEED_SOUND, 0.0f, true);
			PlaySound(SPEED_IN_WATER_SOUND, 0.0f, true);
			if(stats.headUnderWater < 0.0f)
				PlaySound(SPEED_SOUND_STOP);
			else
				PlaySound(SPEED_IN_WATER_SOUND_STOP);

			m_bWasSprinting = false;
			m_startedSprinting = 0;
		}
		else if(gEnv->bMultiplayer)	//fix me : in mp the running loop apparently can get out of sync
			PlaySound(SPEED_SOUND, 0.0f, true);
	}
}

void CNanoSuit::ResetEnergy()
{
	SetSuitEnergy(NANOSUIT_ENERGY);
	m_energy = m_lastEnergy = NANOSUIT_ENERGY;
	for(int i = 0; i < NANOSLOT_LAST; i++)
		m_slots[i].realVal = m_slots[i].desiredVal;

	if (m_pOwner && gEnv->bServer)
		m_pOwner->GetGameObject()->ChangedNetworkState(CPlayer::ASPECT_NANO_SUIT_ENERGY);
}


void CNanoSuit::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_COMPONENT_NAME(s, "NanoSuit");
	s->Add(*this);
	s->AddContainer(m_listeners);
	m_cloak.GetMemoryStatistics(s);
}
