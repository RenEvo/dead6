/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 22:8:2005   12:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include <IEntitySystem.h>
#include <IScriptSystem.h>
#include <IActionMapManager.h>
#include <IGameObject.h>
#include <IGameObjectSystem.h>
#include <IVehicleSystem.h>
#include "WeaponSystem.h"
#include "Weapon.h"
#include "ISerialize.h"
#include "ScriptBind_Weapon.h"
#include "Player.h"
#include "HUD/HUD.h"
#include "GameRules.h"
#include "ItemParamReader.h"
#include "Projectile.h"
#include "OffHand.h"
#include "Lam.h"
#include "GameActions.h"
#include "IronSight.h"
#include "Single.h"


//------------------------------------------------------------------------
CWeapon::CWeapon()
: m_fm(0),
	m_fmId(0),
	m_melee(0),
	m_zm(0),
	m_zmId(0),
	m_fmDefaults(0),
	m_zmDefaults(0),
	m_xmlparams(NULL),
	m_pFiringLocator(0),
	m_fire_alternation(false),
  m_destination(0,0,0),
	m_forcedHitMaterial(-1),
	m_dofSpeed(0.0f),
	m_dofValue(0.0f),
	m_focusValue(0.0f),
	m_currentViewMode(0),
	m_useViewMode(false),
	m_restartZoom(false),
	m_restartZoomStep(0),
	m_targetOn(false),
	m_silencerAttached(false),
	m_weaponRaised(false),
	m_weaponRaising(false),
	m_fireAfterLowering(false),
	m_weaponLowered(false),
	m_switchingFireMode(false),
	m_switchLeverLayers(false)
{
}

//------------------------------------------------------------------------1
CWeapon::~CWeapon()
{
	// deactivate everything
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); it++)
		(*it)->Activate(false);
	// deactivate zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); it++)
		(*it)->Activate(false);

	// clean up firemodes
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); it++)
		(*it)->Release();
	// clean up zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); it++)
		(*it)->Release();

	if (m_pFiringLocator)
		m_pFiringLocator->WeaponReleased();

	if (m_fmDefaults)
		m_fmDefaults->Release();
	if (m_zmDefaults)
		m_zmDefaults->Release();
	if (m_xmlparams)
		m_xmlparams->Release();
}

//------------------------------------------------------------------------
bool CWeapon::ReadItemParams(const IItemParamsNode *root)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (!CItem::ReadItemParams(root))
		return false;

	// read params
	string melee_attack_firemode;
	const IItemParamsNode *params = root->GetChild("params");
	{
		CItemParamReader reader(params);
		reader.Read("melee_attack_firemode", melee_attack_firemode);
	}

	const IItemParamsNode *firemodes = root->GetChild("firemodes");
	InitFireModes(firemodes);

	const IItemParamsNode *zoommodes = root->GetChild("zoommodes");
	InitZoomModes(zoommodes);

	const IItemParamsNode *ammos = root->GetChild("ammos");
	InitAmmos(ammos);

	const IItemParamsNode *aiData = root->GetChild("ai_descriptor");
	InitAIData(aiData);

	m_xmlparams = root;
	m_xmlparams->AddRef();

	if (!melee_attack_firemode.empty())
	{
		m_melee = GetFireMode(melee_attack_firemode.c_str());
		if (m_melee)
			m_melee->Enable(false);
	}

	return true;
}

//------------------------------------------------------------------------
const IItemParamsNode *CWeapon::GetFireModeParams(const char *name)
{
	if (!m_xmlparams)
		return 0;

	const IItemParamsNode *firemodes = m_xmlparams->GetChild("firemodes");
	if (!firemodes)
		return 0;

	int n = firemodes->GetChildCount();
	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *fm = firemodes->GetChild(i);

		const char *fmname = fm->GetAttribute("name");
		if (!fmname || !fmname[0] || stricmp(name, fmname))
			continue;

		return fm;
	}

	return 0;
}

//------------------------------------------------------------------------
const IItemParamsNode *CWeapon::GetZoomModeParams(const char *name)
{
	if (!m_xmlparams)
		return 0;

	const IItemParamsNode *zoommodes = m_xmlparams->GetChild("zoommodes");
	if (!zoommodes)
		return 0;

	int n = zoommodes->GetChildCount();
	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *zm = zoommodes->GetChild(i);

		const char *zmname = zm->GetAttribute("name");
		if (!zmname || !zmname[0] || stricmp(name, zmname))
			continue;

		return zm;
	}

	return 0;
}

//------------------------------------------------------------------------
void CWeapon::InitFireModes(const IItemParamsNode *firemodes)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_firemodes.resize(0);
	m_fmIds.clear();
	m_fmId = 0;
	m_fm = 0;

	if (!firemodes)
		return;

	int n = firemodes->GetChildCount();

	// find the default params
	m_fmDefaults = 0;
	for (int k=0; k<n; k++)
	{
		const IItemParamsNode *fm = firemodes->GetChild(k);
		const char *typ = fm->GetAttribute("type");

		if (typ && !strcmpi(typ, "default"))
		{
			m_fmDefaults = fm;
			m_fmDefaults->AddRef();
			break;
		}
	}

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *fm = firemodes->GetChild(i);

		int enabled = 1;
		const char *name = fm->GetAttribute("name");
		const char *typ = fm->GetAttribute("type");
		fm->GetAttribute("enabled", enabled);
		
		if (!typ || !typ[0])
		{
			GameWarning("Missing type for firemode in weapon '%s'! Skipping...", GetEntity()->GetName());
			continue;
		}

		if (!strcmpi(typ, "default"))
			continue;

		if (!name || !name[0])
		{
			GameWarning("Missing name for firemode in weapon '%s'! Skipping...", GetEntity()->GetName());
			continue;
		}

		IFireMode *pFireMode = g_pGame->GetWeaponSystem()->CreateFireMode(typ);
		if (!pFireMode)
		{
			GameWarning("Cannot create firemode '%s' in weapon '%s'! Skipping...", typ, GetEntity()->GetName());
			continue;
		}
		pFireMode->SetName(name);

		pFireMode->Init(this, m_fmDefaults);
		pFireMode->PatchParams(fm);
		pFireMode->Enable(enabled!=0);

		m_firemodes.push_back(pFireMode);
		m_fmIds.insert(TFireModeIdMap::value_type(name, m_firemodes.size()-1));
	}
}

//------------------------------------------------------------------------
void CWeapon::InitZoomModes(const IItemParamsNode *zoommodes)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_zoommodes.resize(0);
	m_zmIds.clear();
	m_zmId = 0;
	m_zm = 0;

	m_viewModeList.resize(0);
	m_useViewMode = false;

	if (!zoommodes)
		return;

	int n = zoommodes->GetChildCount();

	// find the default params
	m_zmDefaults = 0;
	for (int k=0; k<n; k++)
	{
		const IItemParamsNode *zm = zoommodes->GetChild(k);
		const char *typ = zm->GetAttribute("type");

		if (typ && !strcmpi(typ, "default"))
		{
			m_zmDefaults = zm;
			m_zmDefaults->AddRef();
			break;
		}
	}

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *zm = zoommodes->GetChild(i);

		int enabled = 1;
		const char *name = zm->GetAttribute("name");
		const char *typ = zm->GetAttribute("type");
		zm->GetAttribute("enabled", enabled);
		
		if (!typ || !typ[0])
		{
			GameWarning("Missing type for zoommode in weapon '%s'! Skipping...", GetEntity()->GetName());
			continue;
		}

		if (!strcmpi(typ, "default"))
			continue;

		if (!name || !name[0])
		{
			GameWarning("Missing name for zoommode in weapon '%s'! Skipping...", GetEntity()->GetName());
			continue;
		}

		IZoomMode *pZoomMode = g_pGame->GetWeaponSystem()->CreateZoomMode(typ);
		if (!pZoomMode)
		{
			GameWarning("Cannot create zoommode '%s' in weapon '%s'! Skipping...", typ, GetEntity()->GetName());
			continue;
		}

		// viewmodes - used for binoculars
		const IItemParamsNode *viewModes = zm->GetChild("viewmodes");
		if (viewModes)
		{
			for (int j = 0; j < viewModes->GetChildCount(); j++)
			{
				const IItemParamsNode *viewMode = viewModes->GetChild(j);
				if (viewMode)
				{
					const char *modeValue = 0;
					CItemParamReader reader(viewMode);
					reader.Read("mode", modeValue);

					if (modeValue)
					{
						if (!strcmp(modeValue, "normal"))
						{
							m_currentViewMode = m_viewModeList.size(); // set default view mode
							m_defaultViewMode = m_currentViewMode;
						}

						m_viewModeList.push_back(modeValue);
						m_useViewMode = true;
					}
				}
			}
		}

		pZoomMode->Init(this, m_zmDefaults);
		pZoomMode->PatchParams(zm);
		pZoomMode->Enable(enabled!=0);

		m_zoommodes.push_back(pZoomMode);
		m_zmIds.insert(TZoomModeIdMap::value_type(name, m_zoommodes.size()-1));
	}
}

//------------------------------------------------------------------------
void CWeapon::InitAmmos(const IItemParamsNode *ammos)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	m_ammo.clear();

	if (!ammos)
		return;

	for (int i=0; i<ammos->GetChildCount(); i++)
	{
		const IItemParamsNode *ammo = ammos->GetChild(i);
		if (!strcmpi(ammo->GetName(), "ammo"))
		{
			int extra=0;
			int amount=0;
			int accessoryAmmo=0;

			const char* name = ammo->GetAttribute("name");
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			assert(pClass);

			ammo->GetAttribute("amount", amount);
			ammo->GetAttribute("extra", extra);
			ammo->GetAttribute("accessoryAmmo", accessoryAmmo);

			if(accessoryAmmo)
			{
				m_accessoryAmmo[pClass]=accessoryAmmo;
				m_ammo[pClass]=accessoryAmmo;
			}
			else if (amount)
				m_ammo[pClass]=amount;

			if (extra)
				m_bonusammo[pClass]=extra;

		}
	}
}

//------------------------------------------------------------------------
void CWeapon::InitAIData(const IItemParamsNode *aiDescriptor)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (!aiDescriptor)
		return;
//	<ai_descriptor	hit="instant" speed="20" damage_radius="45" charge_time="2.5" />
	
	aiDescriptor->GetAttribute("speed", m_aiWeaponDescriptor.fSpeed);
	aiDescriptor->GetAttribute("damage_radius", m_aiWeaponDescriptor.fDamageRadius);
	aiDescriptor->GetAttribute("charge_time", m_aiWeaponDescriptor.fChargeTime);
	aiDescriptor->GetAttribute("burstBulletCountMin", m_aiWeaponDescriptor.burstBulletCountMin);
	aiDescriptor->GetAttribute("burstBulletCountMax", m_aiWeaponDescriptor.burstBulletCountMax);
	aiDescriptor->GetAttribute("burstPauseTimeMin", m_aiWeaponDescriptor.burstPauseTimeMin);
	aiDescriptor->GetAttribute("burstPauseTimeMax", m_aiWeaponDescriptor.burstPauseTimeMax);
	aiDescriptor->GetAttribute("singleFireTriggerTime", m_aiWeaponDescriptor.singleFireTriggerTime);
	aiDescriptor->GetAttribute("spreadRadius", m_aiWeaponDescriptor.spreadRadius);
	aiDescriptor->GetAttribute("coverFireTime", m_aiWeaponDescriptor.coverFireTime);
	aiDescriptor->GetAttribute("sweep_width", m_aiWeaponDescriptor.sweepWidth);
	aiDescriptor->GetAttribute("sweep_frequency", m_aiWeaponDescriptor.sweepFrequency);
	aiDescriptor->GetAttribute("draw_time", m_aiWeaponDescriptor.drawTime);
	m_aiWeaponDescriptor.smartObjectClass = aiDescriptor->GetAttribute("smartobject_class");
	m_aiWeaponDescriptor.firecmdHandler = aiDescriptor->GetAttribute("handler");
	int	signalOnShoot(0);
	aiDescriptor->GetAttribute("signal_on_shoot", signalOnShoot);
	m_aiWeaponDescriptor.bSignalOnShoot = signalOnShoot != 0;

	if(m_aiWeaponDescriptor.smartObjectClass != "")
	{
		const char* smartObjectClassProperties = NULL;
		// check if the smartobject class has been overridden in the level
		SmartScriptTable props;
		if (GetEntity()->GetScriptTable()->GetValue("Properties", props))
		{
			if(!props->GetValue("soclasses_SmartObjectClass",smartObjectClassProperties) || 
				(smartObjectClassProperties ==NULL || smartObjectClassProperties[0]==0))
			{
				props->SetValue("soclasses_SmartObjectClass",m_aiWeaponDescriptor.smartObjectClass.c_str());
			}
		}
	}
}


//------------------------------------------------------------------------
bool CWeapon::Init( IGameObject * pGameObject )
{
	if (!CItem::Init(pGameObject))
		return false;

	g_pGame->GetWeaponScriptBind()->AttachTo(this);

	return true;
}

//------------------------------------------------------------------------
void CWeapon::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CWeapon::Serialize( TSerialize ser, unsigned aspects )
{
	CItem::Serialize(ser, aspects);

	if (ser.GetSerializationTarget() == eST_Network)
	{
		if (aspects & ASPECT_FIREMODE) // only the current firemode id, and the current firemode are serialized
		{
			ser.Value("firemode", this, &CWeapon::NetGetCurrentFireMode, &CWeapon::NetSetCurrentFireMode, 'fmod');
			if (m_fm)
				m_fm->Serialize(ser);
		}

		if (aspects & ASPECT_AMMO)
			ser.Value("ammo", this, &CWeapon::NetGetCurrentAmmoCount, &CWeapon::NetSetCurrentAmmoCount, 'ammo');

		// zoom modes are not serialized
	}
	else
	{
		ser.BeginGroup("WeaponAmmo");
		if(ser.IsReading())
			m_ammo.clear();
		TAmmoMap::iterator it = m_ammo.begin();
		int ammoAmount = m_ammo.size();
		ser.Value("AmmoAmount", ammoAmount);
		for(int i = 0; i < ammoAmount; ++i, ++it)
		{
			string name;
			int amount = 0;
			if(ser.IsWriting() && it->first)
			{
				name = it->first->GetName();
				amount = it->second;
			}
			
			ser.BeginGroup("Ammo");
			ser.Value("AmmoName", name);
			ser.Value("Bullets", amount);
			ser.EndGroup();

			if(ser.IsReading())
			{
				IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
				assert(pClass);
				m_ammo[pClass] = amount;
			}
		}
		ser.EndGroup();

		ser.BeginGroup("CrossHairStats");
		ser.Value("CHVisible", m_crosshairstats.visible);
		ser.Value("CHOpacity", m_crosshairstats.opacity);
		ser.Value("CHFading", m_crosshairstats.fading);
		ser.Value("CHFadeFrom", m_crosshairstats.fadefrom);
		ser.Value("CHFadeTo", m_crosshairstats.fadeto);
		ser.Value("CHFadeTime", m_crosshairstats.fadetime);
		ser.Value("CHFadeTimer", m_crosshairstats.fadetimer);
		ser.EndGroup();

		if(GetOwnerId())
		{
			ser.BeginGroup("WeaponStats");
			int firemode = m_fmId;
			ser.Value("FireMode", firemode);
			if(ser.IsReading())
				SetCurrentFireMode(firemode);

			if (m_fm)
        m_fm->Serialize(ser);
      
			bool hasZoom = (m_zm)?true:false;
			ser.Value("hasZoom", hasZoom);
			if(hasZoom)
			{
				int zoomMode = m_zmId;
				ser.Value("ZoomMode", zoomMode);
				bool isZoomed = m_zm->IsZoomed();
				ser.Value("Zoomed", isZoomed);
				int zoomStep = m_zm->GetCurrentStep();
				ser.Value("ZoomStep", zoomStep);

				//m_zm->Serialize(ser); // direct zoommode serialization doesn't work, yet

				if(ser.IsReading())
				{
					if(m_zmId != zoomMode)
						SetCurrentZoomMode(zoomMode);

					m_restartZoom = isZoomed;
					m_restartZoomStep = zoomStep;

          if (!isZoomed && m_zm->IsZoomed())
            m_zm->ExitZoom();
				}
			}

			//serialize special post processing visions
			if(ser.IsWriting())
				m_zoomViewMode = m_currentViewMode;
			ser.Value("currentViewMode", m_zoomViewMode);

			bool reloading = false;
			if(ser.IsWriting())    
				reloading = m_fm ? m_fm->IsReloading() : 0;
			ser.Value("FireModeReloading", reloading);
			if(ser.IsReading() && reloading)
				Reload();
			ser.Value("Alternation", m_fire_alternation);
			ser.Value("ForcedHitMaterial", m_forcedHitMaterial);
			ser.EndGroup();
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::Update( SEntityUpdateContext& ctx, int update)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (m_frozen || IsDestroyed())
		return;

	switch (update)
	{
	case eIUS_FireMode:
		if (m_fm)
			m_fm->Update(ctx.fFrameTime, ctx.nFrameID);
		if (m_melee)
			m_melee->Update(ctx.fFrameTime, ctx.nFrameID);
		break;

	case eIUS_Zooming:
		if (m_zm)
			m_zm->Update(ctx.fFrameTime, ctx.nFrameID);
		break;
	}
  
	CItem::Update(ctx, update);


	if (update==eIUS_General)
	{
		if (fabsf(m_dofSpeed)>0.001f)
		{
			m_dofValue += m_dofSpeed*ctx.fFrameTime;
			m_dofValue = CLAMP(m_dofValue, 0, 1);

			//GameWarning("Actual DOF value = %f",m_dofValue);
			if(m_dofSpeed < 0.0f)
			{
				m_focusValue -= m_dofSpeed*ctx.fFrameTime*150.0f;
				gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", 20.0f + m_focusValue);
			}
			gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", m_dofValue);
		}
	}

	//If press and hold LMB while weapon raised store info in "m_fireAfterLowering"
	//I need to do it here, OnAction is not called in all circumstances...
	if(m_fireAfterLowering)
	{
		if(!IsDualWield() && !IsWeaponRaised())
		{
			m_fireAfterLowering = false;
			OnAction(GetOwnerId(),"attack1",eAAM_OnPress,0.0f);
		}
		else if(IsDualWield() && IsDualWieldMaster())
		{
			if(!IsWeaponRaised())
			{
				m_fireAfterLowering = false;
				OnAction(GetOwnerId(),"attack1",eAAM_OnPress,0.0f);
			}
			IItem *slave = GetDualWieldSlave();
			if(slave && slave->GetIWeapon())
			{
				CWeapon* dualwield = static_cast<CWeapon*>(slave);
				if(!dualwield->IsWeaponRaised())
				{
					m_fireAfterLowering = false;
					OnAction(GetOwnerId(),"attack1",eAAM_OnPress,0.0f);
				}
			}
		}
	}
}

void CWeapon::PostUpdate(float frameTime )
{
	if (m_fm)
		m_fm->PostUpdate(frameTime);
}

//------------------------------------------------------------------------
void CWeapon::HandleEvent( const SGameObjectEvent& event)
{
	CItem::HandleEvent(event);
}

//--------------------------------	----------------------------------------
void CWeapon::SetAuthority(bool auth)
{
	CItem::SetAuthority(auth);
}

//------------------------------------------------------------------------
void CWeapon::Reset()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// deactivate everything
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); it++)
		(*it)->Activate(false);
	// deactivate zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); it++)
		(*it)->Activate(false);

	// clean up firemodes
	for (TFireModeVector::iterator it = m_firemodes.begin(); it != m_firemodes.end(); it++)
		(*it)->Release();
  	
  // clean up zoommodes
	for (TZoomModeVector::iterator it = m_zoommodes.begin(); it != m_zoommodes.end(); it++)
		(*it)->Release();
  

	m_firemodes.resize(0);
  m_fm=0;

	m_zoommodes.resize(0);
  m_zm=0;

	if (m_fmDefaults)
	{
		m_fmDefaults->Release();
		m_fmDefaults=0;
	}
	if (m_zmDefaults)
	{
		m_zmDefaults->Release();
		m_zmDefaults=0;
	}
	if (m_xmlparams)
	{
		m_xmlparams->Release();
		m_xmlparams=0;
	}

	CItem::Reset();

	SetCurrentFireMode(0);
	SetCurrentZoomMode(0);

	
	// have to refix them here.. (they get overriden by SetCurrentFireMode above)
	for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
		FixAccessories(GetAccessoryParams(it->first), true);


}

//------------------------------------------------------------------------
class CWeapon::ScheduleLayer_Leave
{
public:
	ScheduleLayer_Leave(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) {
		_pWeapon->m_transitioning = false;

		gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0.0f);
		//gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", 5.0f); // put back default
		//=========================TESTING========================================
		/*if(g_pGameCVars->i_debug_ladders != 0)
		{
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMin", 1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMax", 1000.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", 1000.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 0.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 0.25f);
		}*/
		//========================================================================
		_pWeapon->m_dofSpeed=0.0f;
	}
private:
	CWeapon *_pWeapon;
};

class CWeapon::ScheduleLayer_Enter
{
public:
	ScheduleLayer_Enter(CWeapon *wep)
	{
		_pWeapon = wep;
	}
	void execute(CItem *item) {
		if(g_pGame->GetHUD())
		{
			g_pGame->GetHUD()->WeaponAccessoriesInterface(true);
		}
		_pWeapon->PlayLayer(g_pItemStrings->modify_layer, eIPAF_Default|eIPAF_NoBlend, false);
		_pWeapon->m_transitioning = false;

		gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 1.0f);
		_pWeapon->m_dofSpeed=0.0f;
	}
private:
	CWeapon *_pWeapon;
};

void CWeapon::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	CItem::OnAction(actorId, actionId, activationMode, value);

	bool isDualWield = IsDualWieldMaster();
	CWeapon *dualWield = NULL;
	COffHand * offHandWeapon = NULL;
	
	if (isDualWield)
	{
		IItem *slave = GetDualWieldSlave();
		if (slave && slave->GetIWeapon())
			dualWield = static_cast<CWeapon *>(slave);
		else
			isDualWield = false;
	}

	bool isOffHandSelected = false;
	if (CActor *pOwnerActor=GetOwnerActor())
	{
		offHandWeapon = static_cast<COffHand*>(pOwnerActor->GetWeaponByClass(CItem::sOffHandClass));
		if(offHandWeapon && offHandWeapon->IsSelected())
		{
			isOffHandSelected = true;
		}
	}

	const SGameActions& actions = g_pGame->Actions();

	// attack
	if ((actionId == actions.attack1) && !m_modifying)
	{
		if (activationMode == eAAM_OnPress)
		{
			if (isDualWield)
			{
				m_fire_alternation = !m_fire_alternation;

				if(IsWeaponRaised() && dualWield->IsWeaponRaised())
					m_fireAfterLowering = true;

				if (m_fire_alternation && CanFire())
				{
					if(!IsWeaponRaised())
						StartFire(actorId);
					else if(!dualWield->IsWeaponRaised())
						dualWield->StartFire(actorId);
				}
				else if (dualWield->CanFire())
				{
					if(!dualWield->IsWeaponRaised())
						dualWield->StartFire(actorId);
					else if(!IsWeaponRaised())
						StartFire(actorId);
				}
			}
			else
			{
				if(!m_weaponRaised&&!m_weaponRaising)
					StartFire(actorId);
				else
					m_fireAfterLowering = true;
			}
		}
		if (activationMode == eAAM_OnRelease)
		{
			if (isDualWield)
			{
				StopFire(actorId);
				dualWield->StopFire(actorId);
			}
			else
				StopFire(actorId);

			m_fireAfterLowering = false;
		}
	}

	// reload
	if ((actionId == actions.reload) &&!IsBusy() && !m_modifying && !isOffHandSelected)
	{
		if(IsWeaponRaised() && m_fm && m_fm->CanReload())
			RaiseWeapon(false);

		Reload();
	
		if (isDualWield)
		{
			if(dualWield->IsWeaponRaised() && dualWield->CanReload())
				dualWield->RaiseWeapon(false);
			dualWield->Reload();
		}
	}

	// zoom
	if(actionId == actions.zoom_in)
	{
		if(m_zm && m_zm->IsZoomed())
		{
			int numSteps = m_zm->GetMaxZoomSteps();
			if((numSteps>1) && (m_zm->GetCurrentStep()<numSteps))
				StartZoom(actorId,1);	
		}
	}
	if(actionId == actions.zoom_out)
	{
		if(m_zm && m_zm->IsZoomed())
		{
			int numSteps = m_zm->GetMaxZoomSteps();
			if((numSteps>1) && (m_zm->GetCurrentStep()>1))
				m_zm->ZoomOut();
		}
	}
	if ((actionId == actions.zoom) && !m_modifying && 
		(!isOffHandSelected || (offHandWeapon->GetOffHandState()&eOHS_TRANSITIONING)))
	{
		if (activationMode == eAAM_OnPress && m_useViewMode)
		{
			IncrementViewmode();
		}
		else
		{
			if (!isDualWield)
			{
				if (activationMode == eAAM_OnPress && m_fm && !m_fm->IsReloading())
				{
					if (m_fm->AllowZoom())
					{
						if(IsWeaponRaised())
							RaiseWeapon(false,true);

						//Use mouse wheel for scopes with several steps/stages
						if(m_zm && m_zm->IsZoomed() && m_zm->GetMaxZoomSteps()>1)
							m_zm->StopZoom();

						StartZoom(actorId,1);		
					}
					else
						m_fm->Cancel();
				}
			}
		}
	}
	else if ((actionId == actions.xi_zoom) && !m_modifying && !isOffHandSelected && !IsWeaponRaised())
	{
		if (m_useViewMode)
		{
			if (activationMode == eAAM_OnPress)
				IncrementViewmode();
		}
		else if (g_pGameCVars->hud_ctrlZoomMode)
		{
			if (activationMode == eAAM_OnPress)
			{
				if (!isDualWield)
				{
					if(m_fm && !m_fm->IsReloading())
					{
						if (m_fm->AllowZoom())
						{
							// The zoom code includes the aim assistance
							StartZoom(actorId,1);
						}
						else
						{
							m_fm->Cancel();
						}
					}
				}
				else
				{
					// If the view does not zoom, we need to force aim assistance
					AssistAiming(1, true);
				}
			}
			else if (activationMode == eAAM_OnRelease)
			{
				if (!isDualWield)
				{
					if(m_fm && !m_fm->IsReloading())
					{
						StopZoom(actorId);
					}
				}
			}
		}
		else
		{
			if (activationMode == eAAM_OnPress && m_fm && !m_fm->IsReloading())
			{
				if (!isDualWield)
				{
					if (m_fm->AllowZoom())
					{
						StartZoom(actorId,1);		
					}
					else
						m_fm->Cancel();
				}
				else
				{
					// If the view does not zoom, we need to force aim assistance
					AssistAiming(1, true);
				}
			}
		}
	}
	
	// fire mode
	if ((actionId == actions.firemode) && !m_modifying)
	{
		if (isDualWield)
		{
			if(IsWeaponRaised())
				RaiseWeapon(false,true);

			if(dualWield->IsWeaponRaised())
				dualWield->RaiseWeapon(false,true);

			StartChangeFireMode();
			dualWield->StartChangeFireMode();
		}
		else
		{
			if(IsWeaponRaised())
				RaiseWeapon(false,true);
			StartChangeFireMode();
		}
	}

	if (actionId == actions.special)
	{
		
		if (activationMode == eAAM_OnPress)
		{
			if(IsWeaponRaised())
				RaiseWeapon(false,true);

			if (CanMeleeAttack() && (!isOffHandSelected || (offHandWeapon->GetOffHandState()&(eOHS_HOLDING_NPC|eOHS_TRANSITIONING))))
				MeleeAttack();
		}
	}

	if ((actionId == actions.modify) && !IsBusy())
	{
		if (m_fm)
			m_fm->StopFire(GetEntity()->GetId());

		if(m_zm && m_zm->IsZoomed())
			m_zm->StopZoom();

		if(IsWeaponRaised())
			RaiseWeapon(false);

		if(isOffHandSelected)
		{
			if(offHandWeapon->GetOffHandState()&(eOHS_HOLDING_NPC|eOHS_HOLDING_OBJECT))
			{
				offHandWeapon->OnAction(m_ownerId,"use",eAAM_OnPress,0);
				offHandWeapon->OnAction(m_ownerId,"use",eAAM_OnRelease,0);
			}
			else if(offHandWeapon->GetOffHandState()&(eOHS_THROWING_NPC|eOHS_THROWING_OBJECT))
			{
				offHandWeapon->OnAction(m_ownerId,"use",eAAM_OnRelease,0);
			}
		}

		if (m_modifying && !m_transitioning)
		{
			//m_dofSpeed = -1.0f/((float)GetCurrentAnimationTime(eIGS_FirstPerson)/1000.0f);
			//m_dofValue = 1.0f;
			StopLayer(g_pItemStrings->modify_layer, eIPAF_Default, false);
			PlayAction(g_pItemStrings->leave_modify, 0);
			m_dofSpeed = -1.0f/((float)GetCurrentAnimationTime(eIGS_FirstPerson)/1000.0f);
			m_dofValue = 1.0f;
			m_focusValue = -1.0f;

			GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<ScheduleLayer_Leave>::Create(this), false);
			m_transitioning = true;

			if(g_pGame->GetHUD())
				g_pGame->GetHUD()->WeaponAccessoriesInterface(false);
			m_modifying = false;

			GetGameObject()->InvokeRMI(CItem::SvRequestLeaveModify(), CItem::EmptyParams(), eRMI_ToServer);
		}
		else if (!m_modifying && !m_transitioning)
		{
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMin", 0.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMax", 5.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", 20.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 0.0f);

			//m_dofSpeed = 1.0f/((float)GetCurrentAnimationTime(eIGS_FirstPerson)/1000.0f);
			//m_dofValue = 0.0f;
			PlayAction(g_pItemStrings->enter_modify, 0, false, eIPAF_Default | eIPAF_RepeatLastFrame);
			m_dofSpeed = 1.0f/((float)GetCurrentAnimationTime(eIGS_FirstPerson)/1000.0f);
			m_dofValue = 0.0f;
			m_transitioning = true;

			GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<ScheduleLayer_Enter>::Create(this), false);
			m_modifying = true;

			GetGameObject()->InvokeRMI(CItem::SvRequestEnterModify(), CItem::EmptyParams(), eRMI_ToServer);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::UpdateFPView(float frameTime)
{
	CItem::UpdateFPView(frameTime);

	UpdateCrosshair(frameTime);
	if (m_fm)
		m_fm->UpdateFPView(frameTime);
	if (m_zm)
		m_zm->UpdateFPView(frameTime);
}

//------------------------------------------------------------------------
void CWeapon::MeleeAttack()
{
	if (!CanMeleeAttack())
		return;

	if (m_melee)
	{
		if (m_fm)
		{
			m_fm->StopFire(m_ownerId);
			if(m_fm->IsReloading())
			{
				if(m_fm->CanCancelReload())
					RequestCancelReload();
				else
					return;
			}
		}
		m_melee->Activate(true);
		m_melee->StartFire(m_ownerId);
		m_melee->StopFire(m_ownerId);
	}
}

//------------------------------------------------------------------------
bool CWeapon::CanMeleeAttack() const
{
	if(m_modifying || m_transitioning)
		return false;

	IActor *act = GetOwnerActor();
	if (act)
	{
		if (IMovementController *pMV = act->GetMovementController())
		{
			SMovementState state;
			pMV->GetMovementState(state);
			if (state.stance == STANCE_PRONE)
				return false;
		}
	}
	return m_melee && m_melee->CanFire();
}

//------------------------------------------------------------------------
void CWeapon::Select(bool select)
{
	CActor *pOwner = static_cast<CActor*>(GetOwnerActor());
	if (select && (IsDestroyed() || (pOwner && pOwner->GetHealth() <= 0)))
		return;

	//If actor is grabbed by player, don't let him select weapon
	if (select && pOwner && pOwner->GetActorStats() && pOwner->GetActorStats()->isGrabbed)
	{
		pOwner->HolsterItem(true);
		return;
	}

	//Reset DOF (avoid problems when entering a vehicle)
	if(!select && (m_modifying || m_transitioning || IsZoomed() || IsZooming()))
	{
		//Reset DOF parameters
		gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0.0f);
		//gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", 5.0f);
	}

	CItem::Select(select);

	if (!select)
	{
		if(IsZoomed() || IsZooming())
			ExitZoom();
		ExitViewmodes();

		m_switchingFireMode = false;

		if(m_weaponRaised || m_weaponRaising)
		{
			m_weaponRaising = false;
			SetWeaponRaised(false);
			ResetAnimation();
			if(IsDualWieldSlave())
			{
				SetDefaultIdleAnimation(eIGS_FirstPerson,g_pItemStrings->idle);
			}
		}

	}

	FadeCrosshair(0, 1.0f, WEAPON_FADECROSSHAIR_SELECT);

	if (m_fm)
	{
		m_fm->Activate(select);
		if (select)
		{
			CActor *pOwner = GetOwnerActor();
			if (pOwner && pOwner->IsClient() && g_pGame->GetHUD())
			{
				g_pGame->GetHUD()->SetFireMode(m_fm->GetName());
			}
		}
	}
	if (m_zm && !IsDualWield())
		m_zm->Activate(select);
}

//------------------------------------------------------------------------
void CWeapon::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	if (byDeath)
	{
		for (TFireModeVector::iterator it=m_firemodes.begin(); it!=m_firemodes.end(); ++it)
		{
			IFireMode *fm=*it;
			if (!fm)
				continue;

			IEntityClass* ammo=fm->GetAmmoType();
			int invCount=GetInventoryAmmoCount(ammo);
			if (invCount)
			{
				SetInventoryAmmoCount(ammo, 0);
				m_bonusammo[ammo]=invCount;
			}
		}
	}

	CItem::Drop(impulseScale, selectNext, byDeath);
}

//------------------------------------------------------------------------
void CWeapon::Freeze(bool freeze)
{
	CItem::Freeze(freeze);

	StopFire(GetOwnerId());
}

//------------------------------------------------------------------------
void CWeapon::SetFiringLocator(IWeaponFiringLocator *pLocator)
{
	if (m_pFiringLocator && m_pFiringLocator != pLocator)
		m_pFiringLocator->WeaponReleased();
	m_pFiringLocator = pLocator;
};

//------------------------------------------------------------------------
IWeaponFiringLocator *CWeapon::GetFiringLocator() const
{
	return m_pFiringLocator;
};

//------------------------------------------------------------------------
void CWeapon::AddEventListener(IWeaponEventListener *pListener, const char *who)
{
	for (TEventListenerVector::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		if (it->pListener == pListener)
			return;
	}

	SListenerInfo info;
	info.pListener=pListener;
#ifdef _DEBUG
	memset(info.who, 0, sizeof(info.who));
	strncpy(info.who, who, 64);
	info.who[63]=0;
#endif
	m_listeners.push_back(info);
}

//------------------------------------------------------------------------
void CWeapon::RemoveEventListener(IWeaponEventListener *pListener)
{
	for (TEventListenerVector::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		if (it->pListener == pListener)
		{
			m_listeners.erase(it);
			return;
		}
	}
}

//------------------------------------------------------------------------
Vec3 CWeapon::GetFiringPos(const Vec3 &probableHit) const
{
	if (m_fm)
		return m_fm->GetFiringPos(probableHit);

	return Vec3(0,0,0);
}

//------------------------------------------------------------------------
Vec3 CWeapon::GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const
{
	if (m_fm)
		return m_fm->GetFiringDir(probableHit, firingPos);

	return Vec3(0,0,0);
}

//------------------------------------------------------------------------
void CWeapon::StartFire(EntityId actorId)
{
	IActor *pOwner = GetOwnerActor();
  if (IsDestroyed() || (pOwner && pOwner->GetHealth() <= 0))
    return;

	if (m_fm)
		m_fm->StartFire(actorId);
}

//------------------------------------------------------------------------
void CWeapon::StopFire(EntityId actorId)
{
	if (m_fm)
		m_fm->StopFire(actorId);
}

//------------------------------------------------------------------------
bool CWeapon::CanFire() const
{
	return m_fm && m_fm->CanFire();
}

//------------------------------------------------------------------------
void CWeapon::StartZoom(EntityId actorId, int zoomed)
{
	IActor *pOwner = GetOwnerActor();
	if (IsDestroyed() || (pOwner && pOwner->GetHealth() <= 0))
		return;

	if (m_zm)
	{
		m_zm->StartZoom(false, true, zoomed);
	}
	else
	{
		// If the view does not zoom, we need to force aim assistance
		AssistAiming(1, true);
	}
}

//------------------------------------------------------------------------
void CWeapon::StopZoom(EntityId actorId)
{
	if (m_zm)
		m_zm->StopZoom();
}

//------------------------------------------------------------------------
bool CWeapon::CanZoom() const
{
	return m_zm && m_zm->CanZoom();
}

//------------------------------------------------------------------------
void CWeapon::ExitZoom()
{
	if (m_zm)
		m_zm->ExitZoom();
}

//------------------------------------------------------------------------
bool CWeapon::IsZoomed() const
{
	return m_zm && m_zm->IsZoomed();
}

//------------------------------------------------------------------------
bool CWeapon::IsZooming() const
{
	return m_zm && m_zm->IsZooming();
}

inline float LinePointDistanceSqr(const Line& line, const Vec3& point, float zScale = 1.0f)
{
	Vec3 x0=point;
	Vec3 x1=line.pointonline;
	Vec3 x2=line.pointonline+line.direction;

	x0.z*=zScale;
	x1.z*=zScale;
	x2.z*=zScale;

	return ((x2-x1).Cross(x1-x0)).GetLengthSquared()/(x2-x1).GetLengthSquared();
}

void CWeapon::AssistAiming(float magnification/*=1.0f*/, bool accurate/*=false*/)
{
	// Check for assistance switches
	if (!accurate && !g_pGameCVars->aim_assistTriggerEnabled)
		return;

	if (accurate && !g_pGameCVars->aim_assistAimEnabled)
		return;
		
	// Check for assistance restriction
	if (!g_pGame->GetHUD() || !g_pGame->GetHUD()->IsInputAssisted())
		return;

	IEntity *res=NULL;

	IActor *pSelfActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pSelfActor)
		return;

	IEntity *pSelf=pSelfActor->GetEntity();
	if (GetOwner() != pSelf)
		return;

	// Do not use in case of vehicle mounted weaponry
	if (pSelfActor->GetLinkedVehicle())
		return;

	IPhysicalEntity *pSelfPhysics=pSelf->GetPhysics();
	IMovementController * pMC = pSelfActor->GetMovementController();
	if(!pMC || !pSelfPhysics)
		return;

	SMovementState info;
	pMC->GetMovementState(info);

	// If already having a valid target, don't do anything
	ray_hit hit;
	if (gEnv->pPhysicalWorld->RayWorldIntersection(info.eyePosition, info.eyeDirection*500.0f, ent_all, (13&rwi_pierceability_mask), &hit, 1, &pSelfPhysics, 1))
	{
		if (!hit.bTerrain && hit.pCollider != pSelfPhysics)
		{
			if (IsValidAssistTarget(gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider), pSelf))
				return;
		}
	}

	const CCamera &camera=GetISystem()->GetViewCamera();
	Vec3 origo=camera.GetPosition();
	// PARAMETER: Autotarget search radius
	const float radius=g_pGameCVars->aim_assistSearchBox;
	// Using radar's proximity check result for optimization
	/*SEntityProximityQuery query;
	query.box= AABB(Vec3(origo.x-radius,origo.y-radius,origo.z-radius),
									Vec3(origo.x+radius,origo.y+radius,origo.z+radius));
	query.nEntityFlags = ENTITY_FLAG_ON_RADAR;
	gEnv->pEntitySystem->QueryProximity(query);*/
	
	// Some other ray may be used too, like weapon aiming or firing ray 
	Line aim=Line(info.eyePosition, info.eyeDirection);

	int betsTarget=-1;
	// PARAMETER: The largest deviance between aiming and target compensated by autoaim
	//	Magnification is used for scopes, but greatly downscaled to avoid random snaps in viewfield
	float maxSnapSqr=g_pGameCVars->aim_assistSnapDistance*sqrt(magnification);
	maxSnapSqr*=maxSnapSqr;
	float bestSnapSqr=maxSnapSqr;
	// PARAMETER: maximum range at which autotargetting works
	float maxDistanceSqr=g_pGameCVars->aim_assistMaxDistance;
	maxDistanceSqr*=maxDistanceSqr;
	Vec3 bestTarget;

	const std::vector<EntityId> *entitiesInProximity = g_pGame->GetHUD()->GetRadar()->GetNearbyEntities();
	IEntitySystem *pEntitySystem = gEnv->pEntitySystem;

	for(int iEntity=0; iEntity<entitiesInProximity->size(); ++iEntity)
	{
		IEntity *pEntity = pEntitySystem->GetEntity((*entitiesInProximity)[iEntity]);
		if(!pEntity)
			continue;

		// Check for target validity
		if (!IsValidAssistTarget(pEntity, pSelf))
			continue;

		IPhysicalEntity *pPhysics=pEntity->GetPhysics();

		if (!pPhysics)
			continue;

		pe_status_dynamics dyn;
		pPhysics->GetStatus(&dyn);
		Vec3 target=dyn.centerOfMass;
		
		if (target.GetSquaredDistance(origo) < maxDistanceSqr &&
			camera.IsPointVisible(target))
		{
			float dst=LinePointDistanceSqr(aim, target, g_pGameCVars->aim_assistVerticalScale);
			
			if (dst<bestSnapSqr)
			{
				// Check if can be hit directly after auto aim
				ray_hit chkhit;
				if (gEnv->pPhysicalWorld->RayWorldIntersection(info.eyePosition, (target-info.eyePosition)*500.0f, ent_all, (13&rwi_pierceability_mask), &chkhit, 1, &pSelfPhysics, 1))
				{
					IEntity *pFound=gEnv->pEntitySystem->GetEntityFromPhysics(chkhit.pCollider);
					if (pFound != pEntity)
						continue;
				}
				else
				{
					continue;
				}

				res=pEntity;
				bestSnapSqr=dst;
				bestTarget=target;
			}
		}
	}

	if (res)
	{
		CMovementRequest req;
		Vec3 v0=info.eyeDirection;
		Vec3 v1=bestTarget-info.eyePosition;
		v0.Normalize();
		v1.Normalize();

		Matrix34 mInvWorld = pSelf->GetWorldTM();
		mInvWorld.Invert();
		//Matrix34 mInvWorld = pSelf->GetLocalTM();

		v0 = mInvWorld.TransformVector( v0 );
		v1 = mInvWorld.TransformVector( v1 );

		Ang3 deltaR=Ang3(Quat::CreateRotationV0V1(v0, v1));
		float scale=1.0f;
		
		if (!accurate)
		{
			IFireMode *pFireMode = GetFireMode(GetCurrentFireMode());
			if (!pFireMode)
				return;

			string modetype (pFireMode->GetType());
			if (modetype.empty())
				return;

			float assistMod=1.0f;

			if (!modetype.compareNoCase("Automatic")||
				!modetype.compareNoCase("Beam") ||
				!modetype.compareNoCase("Burst") ||
				!modetype.compareNoCase("Rapid"))
				assistMod=g_pGameCVars->aim_assistAutoCoeff;
			else
				assistMod=g_pGameCVars->aim_assistSingleCoeff;
				
			scale=sqr(1.0f-bestSnapSqr/maxSnapSqr)*assistMod;
			//GetISystem()->GetILog()->Log("AutoAim mod: %f at mode %s", assistMod, modetype.c_str());
		}

		//GetISystem()->GetILog()->Log("AutoAim scale: %f", scale);
		req.AddDeltaRotation(deltaR*scale);
		pMC->RequestMovement(req);
	}
}

bool CWeapon::IsValidAssistTarget(IEntity *pEntity, IEntity *pSelf,bool includeVehicles/*=false*/)
{
	if(!pEntity)
		return false;

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
	IAIObject *pAI = pEntity->GetAI();

	if(!pActor && includeVehicles && pAI)
	{
		IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pEntity->GetId()); 
		if (pVehicle && pVehicle->GetStatus().health > 0.f && pAI->IsHostile(pSelf->GetAI(),false))
			return true;
	}

	// Check for target validity
	if(!pActor)
		return false;

	if (!pAI)
	{
		if (pActor->IsPlayer() && pEntity != pSelf && !pEntity->IsHidden() &&
				pActor->GetHealth() > 0)
		{
			int ownteam=g_pGame->GetGameRules()->GetTeam(pSelf->GetId());
			int targetteam=g_pGame->GetGameRules()->GetTeam(pEntity->GetId());
			
			// Assist aiming on non-allied players only
			return (targetteam == 0 && ownteam == 0) || (targetteam != 0 && targetteam != ownteam);
		}
		else
		{
			return false;
		}
	}
	
	return (pEntity != pSelf &&!pEntity->IsHidden() && 
		pActor->GetHealth() > 0 &&	pAI->GetAIType() != AIOBJECT_VEHICLE &&
		pAI->IsHostile(pSelf->GetAI(),false));
}

//-----------------------------------------------------------------------
//Slightly modified version on AssistAiming
void CWeapon::AdvancedAssistAiming(float range, const Vec3& pos, Vec3 &dir)
{
	IEntity *res=NULL;

	IActor *pSelfActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pSelfActor)
		return;

	IEntity *pSelf=pSelfActor->GetEntity();
	if (GetOwner() != pSelf)
		return;

	// If already having a valid target, don't do anything
	ray_hit hit;
	IPhysicalEntity* pSkipEnts[10];
	int nSkip = CSingle::GetSkipEntities(this, pSkipEnts, 10);	
	if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir*500.0f, ent_all, (13&rwi_pierceability_mask), &hit, 1, pSkipEnts, nSkip))
	{
		if (!hit.bTerrain)
		{
			if (IsValidAssistTarget(gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider), pSelf, true))
				return;
		}
	}

	const CCamera &camera=GetISystem()->GetViewCamera();
	Vec3 origo=camera.GetPosition();

	// Some other ray may be used too, like weapon aiming or firing ray 
	Line aim=Line(pos,dir);

	float bestSnapSqr=range*range;
	// PARAMETER: maximum range at which autotargetting works
	float maxDistanceSqr=g_pGameCVars->aim_assistMaxDistance;
	maxDistanceSqr*=maxDistanceSqr;
	Vec3 bestTarget;

	if(!g_pGame->GetHUD() || !g_pGame->GetHUD()->GetRadar())
		return;

	const std::vector<EntityId> *entitiesInProximity = g_pGame->GetHUD()->GetRadar()->GetNearbyEntities();

	for(int iEntity=0; iEntity<entitiesInProximity->size(); ++iEntity)
	{
		IEntity *pEntity = m_pEntitySystem->GetEntity((*entitiesInProximity)[iEntity]);
		if(!pEntity)
			continue;

		// Check for target validity
		if (!IsValidAssistTarget(pEntity, pSelf, true))
			continue;

		IPhysicalEntity *pPhysics=pEntity->GetPhysics();

		if (!pPhysics)
			continue;

		pe_status_dynamics dyn;
		pPhysics->GetStatus(&dyn);
		Vec3 target=dyn.centerOfMass;

		if (target.GetSquaredDistance(origo) < maxDistanceSqr &&
			camera.IsPointVisible(target))
		{
			float dst=LinePointDistanceSqr(aim, target, g_pGameCVars->aim_assistVerticalScale);

			if (dst<bestSnapSqr)
			{
				res=pEntity;
				bestSnapSqr=dst;
				bestTarget=target;
			}
		}
	}

	//Correct direction to hit the target
	if (res)
	{
		dir = bestTarget-pos;
		dir.Normalize();
	}
}

//------------------------------------------------------------------------
void CWeapon::RestartZoom(bool force)
{
	if(m_restartZoom || force)
	{
		if(m_zm && !IsBusy() && m_zm->CanZoom())
		{
			m_zm->StartZoom(true, false, m_restartZoomStep);

			m_restartZoom = false;

			if(m_zoomViewMode < m_viewModeList.size())
			{
				const ItemString& curMode = m_viewModeList[m_zoomViewMode];
				if(curMode.length())
				{
					gEnv->p3DEngine->SetPostEffectParam(curMode, 1.0f);
					m_currentViewMode = m_zoomViewMode;
					m_zoomViewMode = 0;
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::MountAt(const Vec3 &pos)
{
	CItem::MountAt(pos);

	AIObjectParameters params;
	GetEntity()->RegisterInAISystem(AIOBJECT_MOUNTEDWEAPON, params);
}

//------------------------------------------------------------------------
void CWeapon::MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles)
{
	CItem::MountAtEntity(entityId, pos, angles);

	if (!m_bonusammo.empty())
	{
		for (TAmmoMap::iterator it=m_bonusammo.begin(); it!=m_bonusammo.end(); ++it)
		{
			SetInventoryAmmoCount(it->first, GetInventoryAmmoCount(it->first)+it->second);
		}

		m_bonusammo.clear();
	}
}

//------------------------------------------------------------------------
void CWeapon::Reload(bool force)
{
	if(CActor *pActor = GetOwnerActor())
	{
		if(IItem *pItem = pActor->GetItemByClass(CItem::sOffHandClass))
		{
			COffHand *pOffHand = static_cast<COffHand*> (pItem);
			CPlayer *pPlayer = static_cast<CPlayer*> (pActor);
			if(pOffHand->GetOffHandState()!=eOHS_INIT_STATE)
				return;
		}
	}
	if (m_fm && (m_fm->CanReload() || force))
	{
		if (m_zm)
			m_fm->Reload(m_zm->GetCurrentStep());
		else
			m_fm->Reload(0);

		RequestReload();
	}
}

//------------------------------------------------------------------------
bool CWeapon::CanReload() const
{
	if (m_fm)
		return m_fm->CanReload();
	return true;
}

//------------------------------------------------------------------------
bool CWeapon::OutOfAmmo(bool allFireModes) const
{
	if (!allFireModes)
		return m_fm && m_fm->OutOfAmmo();

	for (int i=0; i<m_firemodes.size(); i++)
		if (!m_firemodes[i]->OutOfAmmo())
			return false;

	return true;
}

//------------------------------------------------------------------------
int CWeapon::GetAmmoCount(IEntityClass* pAmmoType) const
{
	TAmmoMap::const_iterator it = m_ammo.find(pAmmoType);
	if (it != m_ammo.end())
		return it->second;
	return 0;
}

//------------------------------------------------------------------------
void CWeapon::SetAmmoCount(IEntityClass* pAmmoType, int count)
{
	TAmmoMap::iterator it = m_ammo.find(pAmmoType);
	if (it != m_ammo.end())
		it->second=count;
	else
		m_ammo.insert(TAmmoMap::value_type(pAmmoType, count));

	GetGameObject()->ChangedNetworkState(ASPECT_AMMO);
}

//------------------------------------------------------------------------
int CWeapon::GetInventoryAmmoCount(IEntityClass* pAmmoType) const
{
	if (m_hostId)
	{
		IVehicle *pVehicle=m_pGameFramework->GetIVehicleSystem()->GetVehicle(m_hostId);
		if (pVehicle)
			return pVehicle->GetAmmoCount(pAmmoType);

		return 0;
	}

	IInventory *pInventory=GetActorInventory(GetOwnerActor());
	if (!pInventory)
		return 0;

	return pInventory->GetAmmoCount(pAmmoType);
}

//------------------------------------------------------------------------
void CWeapon::SetInventoryAmmoCount(IEntityClass* pAmmoType, int count)
{
	if (m_hostId)
	{
		IVehicle *pVehicle=m_pGameFramework->GetIVehicleSystem()->GetVehicle(m_hostId);
		if (pVehicle)
			pVehicle->SetAmmoCount(pAmmoType, count);
		
		return;
	}

	IInventory *pInventory=GetActorInventory(GetOwnerActor());
	if (!pInventory)
		return;

	int capacity = pInventory->GetAmmoCapacity(pAmmoType);
	int current = pInventory->GetAmmoCount(pAmmoType);
	if((!gEnv->pSystem->IsEditor()) && (count > capacity))
	{
		if(GetOwnerActor()->IsClient() && g_pGame->GetHUD())
			g_pGame->GetHUD()->DisplayFlashMessage("@ammo_maxed_out", 2, ColorF(1.0f, 0,0), true, pAmmoType->GetName());
		
		//If still there's some place, full inventory to maximum...
		if(current<capacity)
		{
			pInventory->SetAmmoCount(pAmmoType,capacity);
			if(GetOwnerActor()->IsClient() && g_pGame->GetHUD() && capacity - current > 0)
			{
				char buffer[5];
				itoa(capacity - current, buffer, 10);
				g_pGame->GetHUD()->DisplayFlashMessage("@grab_ammo", 3, Col_Wheat, true, (string("@")+pAmmoType->GetName()).c_str(), buffer);
			}
			if (IsServer())
				GetOwnerActor()->GetGameObject()->InvokeRMI(CActor::ClSetAmmo(), CActor::AmmoParams(pAmmoType->GetName(), count), eRMI_ToRemoteClients);

		}
	}
	else
	{
		pInventory->SetAmmoCount(pAmmoType, count);
		if(GetOwnerActor()->IsClient() && g_pGame->GetHUD() && count - current > 0)
		{
			char buffer[5];
			itoa(count - current, buffer, 10);
			g_pGame->GetHUD()->DisplayFlashMessage("@grab_ammo", 3, Col_Wheat, true, (string("@")+pAmmoType->GetName()).c_str(), buffer);
		}
		if (IsServer())
			GetOwnerActor()->GetGameObject()->InvokeRMI(CActor::ClSetAmmo(), CActor::AmmoParams(pAmmoType->GetName(), count), eRMI_ToRemoteClients);
	}
}

//------------------------------------------------------------------------
IFireMode *CWeapon::GetFireMode(int idx) const
{
	if (idx >= 0 && idx < m_firemodes.size())
		return m_firemodes[idx];
	return 0;
}

//------------------------------------------------------------------------
IFireMode *CWeapon::GetFireMode(const char *name) const
{
	TFireModeIdMap::const_iterator it = m_fmIds.find(CONST_TEMP_STRING(name));
	if (it == m_fmIds.end())
		return 0;

	return GetFireMode(it->second);
}

//------------------------------------------------------------------------
int CWeapon::GetFireModeIdx(const char *name) const
{
	TFireModeIdMap::const_iterator it = m_fmIds.find(CONST_TEMP_STRING(name));
	if (it != m_fmIds.end())
		return it->second;
	return -1;
}

//------------------------------------------------------------------------
int CWeapon::GetCurrentFireMode() const
{
	return m_fmId;
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentFireMode(int idx)
{
	if (m_firemodes.empty())
		return;

	if (m_fmId == idx && m_fm)
		return;

	if (m_fm)
		m_fm->Activate(false);

	m_fm = m_firemodes[idx];
	m_fmId = idx;
	if (m_fm)
		m_fm->Activate(true);

	CActor *pOwner = GetOwnerActor();
	if (pOwner && pOwner->IsClient() && g_pGame->GetHUD() && !IsDualWieldSlave())
	{
		g_pGame->GetHUD()->SetFireMode(m_fm->GetName());
	}

	if (IsServer() && GetOwnerId())
		m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponFireModeChanged, m_fm->GetName(), m_fmId, (void *)GetEntityId()));

	GetGameObject()->ChangedNetworkState(ASPECT_FIREMODE);
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentFireMode(const char *name)
{
	TFireModeIdMap::iterator it = m_fmIds.find(CONST_TEMP_STRING(name));
	if (it == m_fmIds.end())
		return;

	SetCurrentFireMode(it->second);
}

//------------------------------------------------------------------------
void CWeapon::ChangeFireMode()
{
	int newId = GetNextFireMode(m_fmId);

	if (newId != m_fmId)
		RequestFireMode(newId);
}

//------------------------------------------------------------------------
int CWeapon::GetNextFireMode(int currMode) const
{
	if (m_firemodes.empty())
		return 0;

	int t = currMode;
	do {
		t++;
		if (t == m_firemodes.size())
			t = 0;
		if (GetFireMode(t)->IsEnabled())
			return t;
	} while(t!=currMode);

	return t;
}

//------------------------------------------------------------------------
void CWeapon::EnableFireMode(int idx, bool enable)
{
	IFireMode *pFireMode = GetFireMode(idx);
	if (pFireMode)
		pFireMode->Enable(enable);
}

//------------------------------------------------------------------------
IZoomMode *CWeapon::GetZoomMode(int idx) const
{
	if (idx >= 0 && idx < m_zoommodes.size())
		return m_zoommodes[idx];
	return 0;
}

//------------------------------------------------------------------------
IZoomMode *CWeapon::GetZoomMode(const char *name) const
{
	TZoomModeIdMap::const_iterator it = m_zmIds.find(CONST_TEMP_STRING(name));
	if (it == m_zmIds.end())
		return 0;

	return GetZoomMode(it->second);
}

//------------------------------------------------------------------------
int CWeapon::GetZoomModeIdx(const char *name) const
{
	TZoomModeIdMap::const_iterator it = m_zmIds.find(CONST_TEMP_STRING(name));
	if (it != m_zmIds.end())
		return it->second;
	return -1;
}

//------------------------------------------------------------------------
int CWeapon::GetCurrentZoomMode() const
{
	return m_zmId;
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentZoomMode(int idx)
{
	if (m_zoommodes.empty())
		return;

	m_zm = m_zoommodes[idx];
	m_zmId = idx;
	m_zm->Activate(true);
}

//------------------------------------------------------------------------
void CWeapon::SetCurrentZoomMode(const char *name)
{
	TZoomModeIdMap::iterator it = m_zmIds.find(CONST_TEMP_STRING(name));
	if (it == m_zmIds.end())
		return;

	SetCurrentZoomMode(it->second);
}

//------------------------------------------------------------------------
void CWeapon::ChangeZoomMode()
{
	if (m_zoommodes.empty())
		return;
/*
	if (m_zmId+1<m_zoommodes.size())
		SetCurrentZoomMode(m_zmId+1);
	else if (m_zoommodes.size()>1)
		SetCurrentZoomMode(0);
		*/
	int t = m_zmId;
	do {
		t++;
		if (t == m_zoommodes.size())
			t = 0;
		if (GetZoomMode(t)->IsEnabled())
			SetCurrentZoomMode(t);
	} while(t!=m_zmId);
}

//------------------------------------------------------------------------
void CWeapon::EnableZoomMode(int idx, bool enable)
{
	IZoomMode *pZoomMode = GetZoomMode(idx);
	if (pZoomMode)
		pZoomMode->Enable(enable);
}

//------------------------------------------------------------------------
bool CWeapon::IsServerSpawn(IEntityClass* pAmmoType) const
{
	return g_pGame->GetWeaponSystem()->IsServerSpawn(pAmmoType);
}

//------------------------------------------------------------------------
CProjectile *CWeapon::SpawnAmmo(IEntityClass* pAmmoType, bool remote)
{
	return g_pGame->GetWeaponSystem()->SpawnAmmo(pAmmoType, remote);
}

//------------------------------------------------------------------------
void CWeapon::SetCrosshairVisibility(bool visible)
{
	m_crosshairstats.visible = visible;
}

//------------------------------------------------------------------------
bool CWeapon::GetCrosshairVisibility() const
{
	return m_crosshairstats.visible;
}

//------------------------------------------------------------------------
void CWeapon::SetCrosshairOpacity(float opacity)
{
	m_crosshairstats.opacity = opacity; 
}

//------------------------------------------------------------------------
float CWeapon::GetCrosshairOpacity() const
{
	return m_crosshairstats.opacity;
}

//------------------------------------------------------------------------
void CWeapon::FadeCrosshair(float from, float to, float time)
{
	m_crosshairstats.fading = true;
	m_crosshairstats.fadefrom = from;
	m_crosshairstats.fadeto = to;
	m_crosshairstats.fadetime = MAX(0, time);
	m_crosshairstats.fadetimer = m_crosshairstats.fadetime;

	SetCrosshairOpacity(from);
}

//------------------------------------------------------------------------
void CWeapon::UpdateCrosshair(float frameTime)
{
	if (m_crosshairstats.fading)
	{
		if (m_crosshairstats.fadetimer>0.0f)
		{
			m_crosshairstats.fadetimer -= frameTime;
			if (m_crosshairstats.fadetimer<0.0f)
				m_crosshairstats.fadetimer=0.0f;

			float t = (m_crosshairstats.fadetime-m_crosshairstats.fadetimer)/m_crosshairstats.fadetime;
			float d = (m_crosshairstats.fadeto-m_crosshairstats.fadefrom);

			if (t >= 1.0f)
				m_crosshairstats.fading = false;

			if (d < 0.0f)
				t = 1.0f-t;

			m_crosshairstats.opacity = fabsf(t*d);
		}
		else
		{
			m_crosshairstats.opacity = m_crosshairstats.fadeto;
			m_crosshairstats.fading = false;
		}
	}

	if(m_restartZoom)
		RestartZoom();
}

//------------------------------------------------------------------------
void CWeapon::AccessoriesChanged()
{
	int i=0;
	for (TFireModeVector::iterator it=m_firemodes.begin(); it!=m_firemodes.end(); ++it)
	{
		for (TFireModeIdMap::iterator iit=m_fmIds.begin(); iit!=m_fmIds.end(); ++iit)
		{
			if (iit->second == i)
			{
				IFireMode *pFireMode = *it;
				pFireMode->ResetParams(0);
				if (m_fmDefaults)
					pFireMode->PatchParams(m_fmDefaults);
				PatchFireModeWithAccessory(pFireMode, "default");

				const IItemParamsNode *fmparams = GetFireModeParams(iit->first.c_str());
				if (fmparams)
					pFireMode->PatchParams(fmparams);

				PatchFireModeWithAccessory(pFireMode, iit->first.c_str());
				break;
			}
		}

		++i;
	}

	i=0;
	for (TZoomModeVector::iterator it=m_zoommodes.begin(); it!=m_zoommodes.end(); ++it)
	{
		for (TZoomModeIdMap::iterator iit=m_zmIds.begin(); iit!=m_zmIds.end(); ++iit)
		{
			if (iit->second == i)
			{
				IZoomMode *pZoomMode = *it;
				pZoomMode->ResetParams(0);
				if (m_zmDefaults)
					pZoomMode->PatchParams(m_zmDefaults);
				PatchZoomModeWithAccessory(pZoomMode, "default");

				const IItemParamsNode *zmparams = GetZoomModeParams(iit->first.c_str());
				if (zmparams)
					pZoomMode->PatchParams(zmparams);

				PatchZoomModeWithAccessory(pZoomMode, iit->first.c_str());
				break;
			}
		}
		++i;
	}

	//Second SOCOM
	bool isDualWield = IsDualWieldMaster();
	CWeapon *dualWield = 0;

	if (isDualWield)
	{
		IItem *slave = GetDualWieldSlave();
		if (slave && slave->GetIWeapon())
			dualWield = static_cast<CWeapon *>(slave);
		else
			isDualWield = false;
	}

	if(isDualWield)
		dualWield->AccessoriesChanged();

}

//------------------------------------------------------------------------
void CWeapon::PatchFireModeWithAccessory(IFireMode *pFireMode, const char *firemodeName)
{
	bool silencerAttached = false;
	bool lamAttached = false;
	m_lamID = 0;

	// patch defaults with accessory defaults
	for (TAccessoryMap::iterator ait=m_accessories.begin(); ait!=m_accessories.end(); ++ait)
	{

		//Attach silencer (and LAM)
		if (ait->first == g_pItemStrings->Silencer || ait->first == g_pItemStrings->SOCOMSilencer)
			silencerAttached = true;
		if (ait->first == g_pItemStrings->LAM || ait->first == g_pItemStrings->LAMRifle)
		{
			lamAttached = true;
			m_lamID = ait->second;
		}

		SAccessoryParams *params=GetAccessoryParams(ait->first);
		if (params && params->params)
		{
			const IItemParamsNode *firemodes = params->params->GetChild("firemodes");
			if (!firemodes)
				continue;

			int n=firemodes->GetChildCount();
			for (int i=0; i<n; i++)
			{
				const IItemParamsNode *firemode = firemodes->GetChild(i);
				const char *name=firemode->GetAttribute("name");
				if (name && !stricmp(name, firemodeName))
				{
					pFireMode->PatchParams(firemode);
					break;
				}
				const char *typ=firemode->GetAttribute("type");
				if (typ && !stricmp(typ, firemodeName))
				{
					pFireMode->PatchParams(firemode);
					break;
				}
			}
		}
	}
	//Fix muzzleflash reseting problem when attaching silencer
	pFireMode->Activate(false);
	pFireMode->Activate(true);

	//Is silencer or LAM attached?
	m_silencerAttached = silencerAttached;
	m_lamAttached = lamAttached;
}

//------------------------------------------------------------------------
void CWeapon::PatchZoomModeWithAccessory(IZoomMode *pZoomMode, const char *zoommodeName)
{
	// patch defaults with accessory defaults
	for (TAccessoryMap::iterator ait=m_accessories.begin(); ait!=m_accessories.end(); ++ait)
	{
		SAccessoryParams *params=GetAccessoryParams(ait->first);
		if (params && params->params)
		{
			const IItemParamsNode *zoommodes = params->params->GetChild("zoommodes");
			if (!zoommodes)
				continue;

			int n=zoommodes->GetChildCount();
			for (int i=0; i<n; i++)
			{
				const IItemParamsNode *zoommode = zoommodes->GetChild(i);
				const char *name=zoommode->GetAttribute("name");
				if (name && !stricmp(name, zoommodeName))
				{
					pZoomMode->PatchParams(zoommode);
					break;
				}
				const char *typ=zoommode->GetAttribute("type");
				if (typ && !stricmp(typ, zoommodeName))
				{
					pZoomMode->PatchParams(zoommode);
					break;
				}
			}
		}
	}
}

//------------------------------------------------------------------------
float CWeapon::GetSpinUpTime() const
{
	if (m_fm)
		return m_fm->GetSpinUpTime();
	return 0.0f;
}

//------------------------------------------------------------------------
float CWeapon::GetSpinDownTime() const
{
	if (m_fm)
		return m_fm->GetSpinDownTime();
	return 0.0f;
}

//------------------------------------------------------------------------
void CWeapon::SetHostId(EntityId hostId)
{
	m_hostId=hostId;
}

//------------------------------------------------------------------------
EntityId CWeapon::GetHostId() const
{
	return m_hostId;
}

//------------------------------------------------------------------------
void CWeapon::FixAccessories(SAccessoryParams *params, bool attach)
{
	if (!attach)
	{
		if (params)
		{
			for (int i = 0; i < params->firemodes.size(); i++)
			{
				if (params->exclusive && GetFireModeIdx(params->firemodes[i]) != -1)
				{
					EnableFireMode(GetFireModeIdx(params->firemodes[i]), false);
				}
			}
			if (!GetFireMode(GetCurrentFireMode())->IsEnabled())
				ChangeFireMode();
			
			if (GetZoomModeIdx(params->zoommode) != -1)
			{
				EnableZoomMode(GetZoomModeIdx(params->zoommode), false);
				ChangeZoomMode();
			}
		}
	}
	else
	{
		
		if (!params->switchToFireMode.empty())
			SetCurrentFireMode(params->switchToFireMode.c_str());

		for (int i = 0; i < params->firemodes.size(); i++)
		{
			if (GetFireModeIdx(params->firemodes[i]) != -1)
			{
				GetFireMode(params->firemodes[i].c_str())->Enable(true);
			}
		}
		if (GetZoomModeIdx(params->zoommode) != -1)
		{
			EnableZoomMode(GetZoomModeIdx(params->zoommode), true);
			SetCurrentZoomMode(GetZoomModeIdx(params->zoommode));
		}
	}	
}

//------------------------------------------------------------------------
void CWeapon::SetDestinationEntity(EntityId targetId)
{
  // default: Set bbox center as destination
  IEntity* pEntity = gEnv->pEntitySystem->GetEntity(targetId);

  if (pEntity)
  {
    AABB box;
    pEntity->GetWorldBounds(box);
    
    SetDestination(box.GetCenter());
  }
}

//------------------------------------------------------------------------
bool CWeapon::PredictProjectileHit(IPhysicalEntity *pShooter, const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speed,
																	 Vec3& predictedPosOut, float& projectileSpeedOut,
																	 Vec3* pTrajectory, unsigned int* trajectorySizeInOut) const
{
	IFireMode *pFireMode = GetFireMode(GetCurrentFireMode());
	if (!pFireMode)
		return false;

	IEntityClass* pAmmoType = pFireMode->GetAmmoType();
	if (!pAmmoType)
		return false;

	CProjectile* pTestProjectile = g_pGame->GetWeaponSystem()->SpawnAmmo(pAmmoType);
	if (!pTestProjectile)
		return false;
	IPhysicalEntity* pProjectilePhysEntity = pTestProjectile->GetEntity()->GetPhysics();
	if (!pProjectilePhysEntity)
		return false;


	projectileSpeedOut = pTestProjectile->GetSpeed();

	pTestProjectile->SetVelocity(pos, dir, velocity, speed/projectileSpeedOut);

	pe_params_flags particleFlags;
	particleFlags.flagsAND = ~(pef_log_collisions & pef_traceable & pef_log_poststep);
	pProjectilePhysEntity->SetParams(&particleFlags);

	pe_params_particle partPar;
	partPar.pColliderToIgnore = pShooter;
	pProjectilePhysEntity->SetParams(&partPar);

	pe_params_pos paramsPos;
	paramsPos.iSimClass = 6;
	paramsPos.pos = pos;
	pProjectilePhysEntity->SetParams(&paramsPos);

	unsigned int n = 0;
	const unsigned int maxSize = trajectorySizeInOut ? *trajectorySizeInOut : 0;

	if (pTrajectory && n < maxSize)
		pTrajectory[n++] = pos;

	float	dt = 0.1f;
	float	lifeTime = 4.0f;

	for (float t = 0.0f; t < lifeTime; t += dt)
	{
		pProjectilePhysEntity->Step(dt);
		if (pTrajectory && n < maxSize)
		{
			pe_status_pos	statusPos;
			pProjectilePhysEntity->GetStatus(&statusPos);
			pTrajectory[n++] = statusPos.pos;
		}
	}

	if (trajectorySizeInOut)
		*trajectorySizeInOut = n;

	pe_status_pos	statusPos;
	pProjectilePhysEntity->GetStatus(&statusPos);
	gEnv->pEntitySystem->RemoveEntity(pTestProjectile->GetEntity()->GetId(), true);

	predictedPosOut = statusPos.pos;

	return true;
}


//------------------------------------------------------------------------
const AIWeaponDescriptor& CWeapon::GetAIWeaponDescriptor( ) const
{
	return m_aiWeaponDescriptor;
}

//------------------------------------------------------------------------
void CWeapon::OnHit(float damage, const char* damageType)
{ 
  CItem::OnHit(damage,damageType);
}

//------------------------------------------------------------------------
void CWeapon::OnDestroyed()
{ 
  CItem::OnDestroyed();

  if (m_fm)
  {
    if (m_fm->IsFiring())
      m_fm->StopFire(GetEntity()->GetId());
  }
}

bool CWeapon::HasAttachmentAtHelper(const char *helper)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer)
	{
		IGameObject *pGameObject = pPlayer->GetGameObject();
		IInventory *pInventory = pPlayer->GetInventory();
		if(pInventory)
		{
			for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
			{
				SAccessoryParams *p = GetAccessoryParams(it->first.c_str());
				if (!strcmp(p->attach_helper.c_str(), helper))
				{	
					// found a child item that can be used
					return true;
				}
			}
			for (int i = 0; i < pInventory->GetCount(); i++)
			{
				EntityId id = pInventory->GetItem(i);
				IItem *cur = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(id);

				if (cur)
				{
					SAccessoryParams *invAccessory = GetAccessoryParams(cur->GetEntity()->GetClass()->GetName());
					if (invAccessory && !strcmp(invAccessory->attach_helper.c_str(), helper))
					{
						// found an accessory in the inventory that can be used
						return true;
					}
				}
			}


		}
	}
	return false;
}

void CWeapon::GetAttachmentsAtHelper(const char *helper, std::vector<string> &rAttachments)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer)
	{
		IInventory *pInventory = pPlayer->GetInventory();
		if(pInventory)
		{
			rAttachments.clear();
			for (int i = 0; i < pInventory->GetCount(); i++)
			{
				EntityId id = pInventory->GetItem(i);
				IItem *cur = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(id);

				if (cur)
				{
					SAccessoryParams *invAccessory = GetAccessoryParams(cur->GetEntity()->GetClass()->GetName());
					if (invAccessory && !strcmp(invAccessory->attach_helper.c_str(), helper))
					{
						rAttachments.push_back(cur->GetEntity()->GetClass()->GetName());
					}
				}
			}
		}
	}
}

void CWeapon::IncrementViewmode()
{
	string oldMode(m_viewModeList[m_currentViewMode].c_str());
	bool oldNormal = m_currentViewMode == m_defaultViewMode;
	m_currentViewMode++;
	if (m_currentViewMode > (m_viewModeList.size() - 1))
		m_currentViewMode = 0;
	string newMode(m_viewModeList[m_currentViewMode].c_str());
	bool newNormal = m_currentViewMode == m_defaultViewMode;

	// deactivate old mode
	if (!oldNormal)
		gEnv->p3DEngine->SetPostEffectParam(oldMode.c_str(), 0.0f);

	// activate new mode
	if (!newNormal)
		gEnv->p3DEngine->SetPostEffectParam(newMode.c_str(), 1.0f);
}

void CWeapon::ExitViewmodes()
{
	if (!m_useViewMode)
		return;

	// deactivate old mode
	if (!m_currentViewMode == m_defaultViewMode)
		gEnv->p3DEngine->SetPostEffectParam(m_viewModeList[m_currentViewMode].c_str(), 0.0f);

	m_currentViewMode = m_defaultViewMode;
}

void CWeapon::EnterWater(bool enter)
{
}

struct CWeapon::EndChangeFireModeAction
{
	EndChangeFireModeAction(CWeapon *_weapon, int _zoomed): weapon(_weapon), zoomed(_zoomed){};
	CWeapon *weapon;
	int zoomed;

	void execute(CItem *_this)
	{
		weapon->EndChangeFireMode(zoomed);
	}
};

struct CWeapon::PlayLeverLayer
{
	PlayLeverLayer(CWeapon *_weapon, bool _activate): weapon(_weapon), activate(_activate){};
	CWeapon *weapon;
	bool activate;

	void execute(CItem *_this)
	{
		if(activate)
		{
			weapon->StopLayer(g_pItemStrings->lever_layer_2);
			weapon->PlayLayer(g_pItemStrings->lever_layer_1);
		}
		else
		{
			weapon->StopLayer(g_pItemStrings->lever_layer_1);
			weapon->PlayLayer(g_pItemStrings->lever_layer_2);
		}
	}
};

void CWeapon::StartChangeFireMode()
{

	if(IsBusy() || IsZooming() || (m_fm && m_fm->IsReloading()))
		return;

	//Check if the weapon has enough firemodes (Melee is always there)
	if(m_fmIds.size()<=2)
		return;

	int zoomed = 0;
	
	if(m_zm)
		zoomed = m_zm->GetCurrentStep();

	//Zoom out if zoomed and SetBusy()
	if (zoomed)
	{
		ExitZoom();
		ExitViewmodes();
	}

	SetBusy(true);
	m_switchingFireMode = true;

	//If NANO speed selected...
	float speedOverride = -1.0f;
	float mult = 1.0f;
	CActor *owner = GetOwnerActor();
	if (owner && owner->GetActorClass() == CPlayer::GetActorClassType())
	{
		CPlayer *pPlayer = (CPlayer *)owner;
		if(pPlayer->GetNanoSuit())
		{
			if (pPlayer->GetNanoSuit()->GetMode() == NANOMODE_SPEED)
			{
				speedOverride = 1.75f;
				mult = 1.0f/1.75f;
			}
		}
	}

	if (speedOverride > 0.0f)
		PlayAction(g_pItemStrings->change_firemode, 0, false, CItem::eIPAF_Default, speedOverride);
	else
		PlayAction(g_pItemStrings->change_firemode);

	GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson)*0.5f, CSchedulerAction<PlayLeverLayer>::Create(PlayLeverLayer(this, m_switchLeverLayers)), false);
	GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<EndChangeFireModeAction>::Create(EndChangeFireModeAction(this, zoomed)), false);

}

void CWeapon::EndChangeFireMode(int zoomed)
{
	//Real change is here
	ChangeFireMode();

	m_switchLeverLayers = !m_switchLeverLayers;	
	
	SetBusy(false);

	//Come back to zoom mode if neccessary
	if (zoomed && IsSelected())
		StartZoom(GetOwnerId(),zoomed);

	m_switchingFireMode = false;
}

//-----------------------------------------------------------------
EntityId CWeapon::GetLAMAttachment()
{
	for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
	{
		if (it->first == g_pItemStrings->LAM || it->first == g_pItemStrings->LAMRifle)
			return it->second;
	}

	return 0;
}

//-----------------------------------------------------------------
bool CWeapon::IsLamAttached()
{
	return GetLAMAttachment() != 0;
}

//-----------------------------------------------------------------
void CWeapon::ActivateLamLaser(bool activate, bool aiRequest /* = true */)
{
	m_lamID = GetLAMAttachment();
	m_lamAttached = m_lamID != 0;

	//Only if LAM is attached
	if (m_lamAttached)
	{
		CLam* pLam = static_cast<CLam*>(m_pItemSystem->GetItem(m_lamID));
		if (pLam)
			pLam->ActivateLaser(activate, aiRequest);
	}
	else
	{
		GameWarning("No LAM attached!! Laser could not be activated/deactivated");
	}
}

//------------------------------------------------------------------
void CWeapon::ActivateLamLight(bool activate, bool aiRequest /* = true */)
{
	m_lamID = GetLAMAttachment();
	m_lamAttached = m_lamID != 0;

	//Only if LAM is attached
	if (m_lamAttached)
	{
		CLam* pLam = static_cast<CLam*>(m_pItemSystem->GetItem(m_lamID));
		if (pLam)
			pLam->ActivateLight(activate,aiRequest);
	}
	else
	{
		GameWarning("No LAM attached!! Light could not be activated/deactivated");
	}
}

//------------------------------------------------------------------
bool CWeapon::IsLamLaserActivated()
{
	m_lamID = GetLAMAttachment();
	m_lamAttached = m_lamID != 0;

	//Only if LAM is attached
	if (m_lamAttached)
	{
		CLam* pLam = static_cast<CLam*>(m_pItemSystem->GetItem(m_lamID));
		if (pLam)
			return pLam->IsLaserActivated();
	}

	return false;
}

//------------------------------------------------------------------
bool CWeapon::IsLamLightActivated()
{
	m_lamID = GetLAMAttachment();
	m_lamAttached = m_lamID != 0;

	//Only if LAM is attached
	if (m_lamAttached)
	{
		CLam* pLam = static_cast<CLam*>(m_pItemSystem->GetItem(m_lamID));
		if (pLam)
			return pLam->IsLightActivated();
	}

	return false;
}

//-----------------------------------------------------------------
struct CWeapon::EndRaiseWeaponAction
{
	EndRaiseWeaponAction(CWeapon *_weapon): weapon(_weapon){};
	CWeapon *weapon;

	void execute(CItem *_this)
	{
		weapon->SetWeaponRaised(true);
		weapon->SetBusy(false);
		weapon->m_weaponRaising = false;
	}
};

void CWeapon::RaiseWeapon(bool reise, bool faster /* = false */)
{
	if(m_params.raiseable)
	{
		if(reise && !IsWeaponRaised())
		{

			//Play the sound anyways if necessary...
			CPlayer *pPlayer = static_cast<CPlayer*>(GetOwnerActor());
			if(pPlayer)
			{
				SPlayerStats *stats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());
				if(stats )
				{
					Vec3 vel = stats->velocity;
					if(vel.z<0.0f)
						vel.z = 0.0f;
					if(vel.len2()>25.0f)
					{
						pPlayer->PlaySound(CPlayer::ESound_Hit_Wall,true,true,"speed",0.6f);
						//FX feedback
						IMovementController *pMC = pPlayer->GetMovementController();
						if(pMC)
						{
							SMovementState state;
							pMC->GetMovementState(state);
							IParticleEffect *pEffect = gEnv->p3DEngine->FindParticleEffect("collisions.footsteps.dirt");
							if (pEffect)
							{
								Matrix34 tm = IParticleEffect::ParticleLoc(state.eyePosition + state.eyeDirection*0.5f);
								pEffect->Spawn(true,tm);
							}	
						}		
					}
				}
			}
			

			//Weapon zoomed, reloading, ...
			if(IsBusy() || IsZooming() || IsZoomed() || IsModifying() || (m_fm && m_fm->IsFiring()))
				return;

			StopFire(GetOwnerId());

			//If NANO speed selected...
			float speedOverride = -1.0f;

			CActor *owner = GetOwnerActor();
			if (owner && owner->GetActorClass() == CPlayer::GetActorClassType())
			{
				CPlayer *pPlayer = (CPlayer *)owner;
				if(pPlayer->GetNanoSuit())
				{
					if (pPlayer->GetNanoSuit()->GetMode() == NANOMODE_SPEED)
					{
						speedOverride = 1.75f;
					}
				}
			}

			if (speedOverride > 0.0f)
				PlayAction(g_pItemStrings->raise, 0, false, CItem::eIPAF_Default, speedOverride);
			else
				PlayAction(g_pItemStrings->raise);

			SetDefaultIdleAnimation(eIGS_FirstPerson,g_pItemStrings->idle_raised);

			GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<EndRaiseWeaponAction>::Create(EndRaiseWeaponAction(this)), false);

			m_weaponRaising = true;
			SetBusy(true);

		}
		else if(!reise && IsWeaponRaised())
		{
			//If NANO speed selected...
			float speedOverride = -1.0f;
		
			CActor *owner = GetOwnerActor();
			if (owner && owner->GetActorClass() == CPlayer::GetActorClassType())
			{
				CPlayer *pPlayer = (CPlayer *)owner;
				if(pPlayer->GetNanoSuit())
				{
					if (pPlayer->GetNanoSuit()->GetMode() == NANOMODE_SPEED || faster)
					{
						speedOverride = 1.75f;
				
					}
				}
			}

			if (speedOverride > 0.0f)
				PlayAction(g_pItemStrings->lower, 0, false, CItem::eIPAF_Default, speedOverride);
			else
				PlayAction(g_pItemStrings->lower);

			SetDefaultIdleAnimation(eIGS_FirstPerson,g_pItemStrings->idle);

			SetWeaponRaised(false);

			//SetBusy(false);

		}
	}
}

//-----------------------------------------------------
void CWeapon::StopUse(EntityId userId)
{
	CItem::StopUse(userId);

	if(m_stats.mounted)
	{
		if(IsZoomed() || IsZooming())
			ExitZoom();

		ExitViewmodes();
	}
}

//-------------------------------------------------------
void CWeapon::AutoDrop()
{
	if(m_fm)
	{
		CActor* pOwner = GetOwnerActor();
		// no need to auto-drop for AI
		if(pOwner && !pOwner->IsPlayer())
			return;
		if(GetAmmoCount(m_fm->GetAmmoType())<=0)
		{
			if( pOwner )
				pOwner->DropItem(GetEntityId(),1.0f,true,false);
			Pickalize(false,true);
			if(!gEnv->bMultiplayer)
				g_pGame->GetGameRules()->ScheduleEntityRemoval(GetEntityId(),5.0f,true);
		}
	}
}

//------------------------------------------------------
bool CWeapon::CheckAmmoRestrictions(EntityId pickerId)
{
	if(g_pGameCVars->i_unlimitedammo != 0)
		return true;

	IActor* pPicker = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pickerId);
	if(pPicker)
	{

		IInventory *pInventory = pPicker->GetInventory();
		if(pInventory)
		{
			if(pInventory->GetCountOfClass(GetEntity()->GetClass()->GetName()) == 0)
				return true;

			if (!m_bonusammo.empty())
			{
				for (TAmmoMap::const_iterator it=m_bonusammo.begin(); it!=m_bonusammo.end(); ++it)
				{
					int invAmmo  = pInventory->GetAmmoCount(it->first);
					int invLimit = pInventory->GetAmmoCapacity(it->first);

					if(invAmmo>=invLimit && (!gEnv->pSystem->IsEditor()))
						return false;

				}
			}

			if(!m_ammo.empty())
			{
				for (TAmmoMap::const_iterator it=m_ammo.begin(); it!=m_ammo.end(); ++it)
				{
					int invAmmo  = pInventory->GetAmmoCount(it->first);
					int invLimit = pInventory->GetAmmoCapacity(it->first);

					if(invAmmo>=invLimit && (!gEnv->pSystem->IsEditor()))
						return false;
				}
			}
		}		
	}

	return true;
}

//-------------------------------------------------------------
int CWeapon::GetMaxZoomSteps()
{
	if(m_zm)
		return m_zm->GetMaxZoomSteps();

	return 0;
}

void CWeapon::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	CItem::GetMemoryStatistics(s);
/*
	if (m_fm)
		m_fm->GetMemoryStatistics(s);
	if (m_zm)
		m_zm->GetMemoryStatistics(s);
*/
	{
		SIZER_COMPONENT_NAME(s, "FireModes");
		s->AddContainer(m_fmIds);
		s->AddContainer(m_firemodes);
		for (TFireModeIdMap::iterator iter = m_fmIds.begin(); iter != m_fmIds.end(); ++iter)
			s->Add(iter->first);
		for (size_t i=0; i<m_firemodes.size(); i++)
			if (m_firemodes[i])
				m_firemodes[i]->GetMemoryStatistics(s);
	}
	{
		SIZER_COMPONENT_NAME(s, "ZoomModes");
		s->AddContainer(m_zmIds);
		s->AddContainer(m_zoommodes);
		for (TZoomModeIdMap::iterator iter = m_zmIds.begin(); iter != m_zmIds.end(); ++iter)
			s->Add(iter->first);
		for (size_t i=0; i<m_zoommodes.size(); i++)
			if (m_zoommodes[i])
				m_zoommodes[i]->GetMemoryStatistics(s);
	}

	{
		SIZER_COMPONENT_NAME(s, "Ammo");
		s->AddContainer(m_ammo);
		s->AddContainer(m_bonusammo);
		s->AddContainer(m_accessoryAmmo);
	}

	s->AddContainer(m_listeners);
	s->Add( m_aiWeaponDescriptor.firecmdHandler );
	s->Add( m_aiWeaponDescriptor.smartObjectClass );
	s->AddContainer( m_viewModeList );
	for (std::vector<ItemString>::iterator iter = m_viewModeList.begin(); iter != m_viewModeList.end(); ++iter)
		s->Add(*iter);
}
