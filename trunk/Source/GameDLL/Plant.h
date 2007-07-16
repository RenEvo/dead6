/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Throw Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 26ye:10:2005   15:45 : Created by Márcio Martins

*************************************************************************/
#ifndef __PLANT_H__
#define __PLANT_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"


class CPlant : public IFireMode
{
	struct ReleaseButtonAction;
	struct PressButtonAction;
	struct StartPlantAction;

protected:
	typedef struct SPlantParams
	{
		SPlantParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			string	ammo_type;

			ResetValue(ammo_type,			"c4explosive");
			if (defaultInit || !ammo_type.empty())
				ammo_type_class = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo_type.c_str());
			ResetValue(clip_size,			3);
			ResetValue(damage,				100);

			ResetValue(helper,				true);
			ResetValue(impulse,				10.0f);
			ResetValue(delay,					0.25f);
			ResetValue(tick,					5.0f);
			ResetValue(tick_time,			0.45f);
			ResetValue(min_time,			5.0f);
			ResetValue(max_time,			180.0f);
			ResetValue(led_minutes,		true);
			ResetValue(led_layers,		"d%d%d");
			ResetValue(simple,				false);
		};

		IEntityClass*	ammo_type_class;
		int			damage;
		int			clip_size;
		bool		simple;

		string	helper;
		float		impulse;
		float		delay;
		float		tick;
		float		tick_time;
		float		min_time;
		float		max_time;
		bool		led_minutes;
		string	led_layers;

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(helper);
			s->Add(led_layers);
		}
	} SPlantParams;

	typedef struct SPlantActions
	{
		SPlantActions() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(press_button,		"press_button");
			ResetValue(hold_button,			"hold_button");
			ResetValue(release_button,	"release_button");
			ResetValue(tick,						"tick");
			ResetValue(plant,						"plant");
			ResetValue(refill,					"select");
		};

		string press_button;
		string hold_button;
		string release_button;
		string tick;
		string plant;
		string refill;

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(press_button);
			s->Add(hold_button);
			s->Add(release_button);
			s->Add(tick);
			s->Add(plant);
			s->Add(refill);
		}
	} SPlantActions;

public:
	CPlant();
	virtual ~CPlant();

	virtual void Init(IWeapon *pWeapon, const struct IItemParamsNode *params);
	virtual void Update(float frameTime, uint frameId);
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime);
	virtual void Release();
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void ResetParams(const struct IItemParamsNode *params);
	virtual void PatchParams(const struct IItemParamsNode *patch);

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const { return m_pWeapon->GetAmmoCount(m_plantparams.ammo_type_class); };
	virtual int GetClipSize() const { return m_plantparams.clip_size; };

	virtual bool OutOfAmmo() const;
	virtual bool CanReload() const { return false; };
	virtual void Reload(int zoomed) {};
	virtual bool IsReloading() { return false; };
	virtual void CancelReload() {};
	virtual bool CanCancelReload() { return false; };

	virtual bool AllowZoom() const { return true; };
	virtual void Cancel() {};

	virtual float GetRecoil() const { return 0.0f; };
	virtual float GetSpread() const { return 0.0f; };
	virtual const char *GetCrosshair() const { return ""; };
	virtual float GetHeat() const { return 0.0f; };

	virtual bool CanFire(bool considerAmmo=true) const;
	virtual void StartFire(EntityId shooterId);
	virtual void StopFire(EntityId shooterId);
	virtual bool IsFiring() const { return m_planting; };

	virtual void NetShoot(const Vec3 &hit, int ph);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int ph);

	virtual void NetStartFire(EntityId shooterId);
	virtual void NetStopFire(EntityId shooterId);

	virtual EntityId GetProjectileId() const { return m_projectileId; };
	virtual void SetProjectileId(EntityId id) { m_projectileId = id; };

	virtual const char *GetType() const;
	virtual IEntityClass* GetAmmoType() const;
	virtual int GetDamage() const;

	virtual float GetSpinUpTime() const { return 0.0f; };
	virtual float GetSpinDownTime() const { return 0.0f; };
	virtual float GetNextShotTime() const { return 0.0f; };
	virtual float GetFireRate() const { return 0.0f; };

	virtual void Enable(bool enable) { m_enabled = enable; };
	virtual bool IsEnabled() const { return m_enabled; };

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const { return ZERO;}
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const { return ZERO;}
	virtual void SetName(const char *name) {  m_name = name; };
	virtual const char *GetName() { return m_name.empty()?0:m_name.c_str();};

	virtual bool HasFireHelper() const { return false; }
	virtual Vec3 GetFireHelperPos() const { return Vec3(ZERO); }
	virtual Vec3 GetFireHelperDir() const { return FORWARD_DIRECTION; }

	virtual int GetCurrentBarrel() const { return 0; }
	virtual void Serialize(TSerialize ser);

	virtual void SetRecoilMultiplier(float recoilMult) { }
	virtual float GetRecoilMultiplier() const { return 1.0f; }

	virtual void Time() { m_timing=true; };
	virtual void SetTime(float time) { m_time=time; };
	virtual float GetTime() const { return m_time; };

	virtual void PatchSpreadMod(SSpreadModParams &sSMP){};
	virtual void ResetSpreadMod(){};

	virtual void PatchRecoilMod(SRecoilModParams &sRMP){};
	virtual void ResetRecoilMod(){};
	virtual void ResetLock() {};
	virtual void StartLocking(EntityId targetId) {};
	virtual void Lock(EntityId targetId) {};
	virtual void Unlock() {};

protected:
	virtual void TickTimer();
	virtual void Plant(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, bool net=false);
	virtual void SetLED(float time, bool minutes, const char *layerFmt);
	virtual void SelectDetonator();
	virtual void SelectLast();
	virtual void CheckAmmo();

	CWeapon	*m_pWeapon;
	bool		m_enabled;
	string	m_name;

	EntityId m_projectileId;

	SPlantParams	m_plantparams;
	SPlantActions	m_plantactions;

	bool		m_planting;
	bool		m_pressing;
	bool		m_holding;
	bool		m_timing;
	
	float		m_time;

	float		m_plantTimer;
	float		m_tickTimer;
};

#endif 