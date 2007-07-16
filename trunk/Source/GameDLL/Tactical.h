/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Tactical Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 21:11:2006   14:06 : Created by Márcio Martins

-  5:02:2007   11:55 : Not needed, all is handled in TacticalAttahcment/TacBullet (Removed from weapon system)
*************************************************************************/
#ifndef __TACTICAL_H__
#define __TACTICAL_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Single.h"

//=================================NOT USED=================================
typedef struct STacEffect
{
	virtual void Activate(EntityId targetId, EntityId ownerId , EntityId weaponId, const char *effect, const char *defaultEffect) = 0;
} STacEffect;

typedef struct SSleepEffect : public STacEffect {
	virtual void Activate(EntityId targetId, EntityId ownerId , EntityId weaponId, const char *effect, const char *defaultEffect);
} SSleepEffect;


typedef struct SKillEffect : public STacEffect {
	virtual void Activate(EntityId targetId, EntityId ownerId, EntityId weaponId, const char *effect, const char *defaultEffect);
} SKillEffect;

typedef struct SSoundEffect : public STacEffect {
	virtual void Activate(EntityId targetId, EntityId ownerId, EntityId weaponId, const char *effect, const char *defaultEffect);
} SSoundEffect;


class CTactical : public IFireMode
{
public:
	CTactical();
	~CTactical();

	virtual void Init(IWeapon *pWeapon, const struct IItemParamsNode *params);
	virtual void Update(float frameTime, uint frameId) {};
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime) {};
	virtual void Release();
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void ResetParams(const struct IItemParamsNode *params);
	virtual void PatchParams(const struct IItemParamsNode *patch);

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const { return 0; };
	virtual int GetClipSize() const { return 0; };

	virtual bool OutOfAmmo() const { return false; };
	virtual bool CanReload() const { return false; };
	virtual void Reload(int zoomed) {};
	virtual bool IsReloading() { return false; };

	virtual bool AllowZoom() const { return true; };
	virtual void Cancel() {};

	virtual float GetRecoil() const { return 0.0f; };
	virtual float GetSpread() const { return 0.0f; };
	virtual const char *GetCrosshair() const { return ""; };
	virtual float GetHeat() const { return 0.0f; };

	virtual bool CanFire(bool considerAmmo=true) const { return true; };
	virtual void StartFire(EntityId shooterId);
	virtual void StopFire(EntityId shooterId) {};
	virtual bool IsFiring() const { return false; };

	virtual void NetShoot(const Vec3 &hit, int ph) {};
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int ph) {};

	virtual void NetStartFire(EntityId shooterId);
	virtual void NetStopFire(EntityId shooterId) {};

	virtual EntityId GetProjectileId() const { return 0; };
	virtual void SetProjectileId(EntityId id) { };

	virtual const char *GetType() const;
	virtual IEntityClass* GetAmmoType() const { return 0; };
	virtual int GetDamage() const { return 0; };

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


protected:
	typedef struct STacParams
	{
		STacParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);

			ResetValue(tac_effect, "sleep");
			ResetValue(tac_humanEffect, "");
			ResetValue(tac_alienEffect, "");
			ResetValue(tac_defaultEffect, "");
		}

		string tac_effect;
		string tac_humanEffect;
		string tac_alienEffect;
		string tac_defaultEffect;

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(tac_effect);
			s->Add(tac_humanEffect);
			s->Add(tac_alienEffect);
			s->Add(tac_defaultEffect);
		}
	} STacParams;

	STacParams m_tacparams;
	STacEffect *m_tacEffect;

	CWeapon			*m_pWeapon;
	bool				m_enabled;
	string			m_name;
};

#endif //__TACTICAL_H__
