/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Beam Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 2:8:2006   19:06 : Created by Márcio Martins

*************************************************************************/
#ifndef __HEAL_H__
#define __HEAL_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Weapon.h"
#include "ItemParamReader.h"


#define ResetValue(name, defaultValue) if (defaultInit) name=defaultValue; reader.Read(#name, name)
#define ResetValueEx(name, var, defaultValue) if (defaultInit) var=defaultValue; reader.Read(name, var)

class CHeal :
	public IFireMode
{
	struct StopAttackingAction;	

	typedef struct SHealParams
	{
		SHealParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			string ammo_type;
			ResetValue(health,					32);
			ResetValue(crosshair,				"default");
			ResetValue(hit_type,				"heal");
			ResetValue(ammo_type,				"syringe");
			if (defaultInit || !ammo_type.empty())
				ammo_type_class = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo_type.c_str());
			ResetValue(delay,						0.5f);
			ResetValue(range,						1.5f);
			ResetValue(clip_size,				5);
			ResetValue(auto_select_last,true);
		}

		short		health;
		string	crosshair;
		string	hit_type;
		IEntityClass*	ammo_type_class;
		float		delay;
		float		range;
		int			clip_size;
		bool		auto_select_last;

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(crosshair);
			s->Add(hit_type);
		}
	} SHealParams;

	typedef struct SHealActions
	{
		SHealActions() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(heal,				"heal");
			ResetValue(self_heal,		"self_heal");
			ResetValue(next,				"next");
			ResetValue(healed,			"healed");
		}

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(heal);
			s->Add(self_heal);
			s->Add(next);
			s->Add(healed);
		}

		string	heal;
		string	self_heal;
		string	next;
		string	healed;
	} SHealActions;

public:
	CHeal();
	virtual ~CHeal();

	//IFireMode
	virtual void Init(IWeapon *pWeapon, const struct IItemParamsNode *params);
	virtual void Update(float frameTime, uint frameId);
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime) {};
	virtual void Release();
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void ResetParams(const struct IItemParamsNode *params);
	virtual void PatchParams(const struct IItemParamsNode *patch);

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const;
	virtual int GetClipSize() const { return m_healparams.clip_size; };

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
	virtual bool IsFiring() const { return m_healing; };

	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int predictionHandle);

	virtual void NetStartFire(EntityId shooterId) {};
	virtual void NetStopFire(EntityId shooterId) {};

	virtual EntityId GetProjectileId() const { return 0; };
	virtual void SetProjectileId(EntityId id) {};

	virtual const char *GetType() const;
	virtual IEntityClass* GetAmmoType() const;
	virtual int GetDamage() const;

	virtual float GetSpinUpTime() const { return 0.0f; };
	virtual float GetSpinDownTime() const { return 0.0f; };
	virtual float GetNextShotTime() const { return 0.0f; };
	virtual float GetFireRate() const { return 0.0f; };

	virtual void Enable(bool enable) { m_enabled = enable; };
	virtual bool IsEnabled() const { return m_enabled; };

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const {return ZERO;}
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const {return ZERO;}
	virtual void SetName(const char *name) { m_name = name; };
	virtual const char *GetName() { return m_name.empty()?0:m_name.c_str();};

	virtual bool HasFireHelper() const { return false; }
	virtual Vec3 GetFireHelperPos() const { return Vec3(ZERO); }
	virtual Vec3 GetFireHelperDir() const { return FORWARD_DIRECTION; }

  virtual int GetCurrentBarrel() const { return 0; }
	virtual void Serialize(TSerialize ser) {};

	virtual void SetRecoilMultiplier(float recoilMult) { }
	virtual float GetRecoilMultiplier() const { return 1.0f; }

	virtual void PatchSpreadMod(SSpreadModParams &sSMP){};
	virtual void ResetSpreadMod(){};

	virtual void PatchRecoilMod(SRecoilModParams &sRMP){};
	virtual void ResetRecoilMod(){};

	virtual void ResetLock() {};
	virtual void StartLocking(EntityId targetId) {};
	virtual void Lock(EntityId targetId) {};
	virtual void Unlock() {};
	//~IFireMode

	virtual void Heal(EntityId id);
	virtual EntityId GetHealableId() const;

protected:
	CWeapon *m_pWeapon;
	bool		m_enabled;

	bool		m_healing;
	bool		m_selfHeal;

	float		m_delayTimer;
	string	m_name;

	SHealParams		m_healparams;
	SHealActions	m_healactions;
};


#endif //__HEAL_H__