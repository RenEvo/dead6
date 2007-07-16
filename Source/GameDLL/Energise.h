/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Energise Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 03:07:2006:		Created by Marco Koegler

*************************************************************************/
#ifndef __ENERGISE_H__
#define __ENERGISE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Weapon.h"
#include "ItemParamReader.h"
#include "Single.h"

#define ResetValue(name, defaultValue) if (defaultInit) name=defaultValue; reader.Read(#name, name)
#define ResetValueEx(name, var, defaultValue) if (defaultInit) var=defaultValue; reader.Read(name, var)


class CEnergise;
class CEnergise : public IFireMode
{
	struct SEnergiseParams
	{
		SEnergiseParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(range,			1.5f);
			ResetValue(delay,			0.5f);

			energise_effect.Reset(params?params->GetChild("energise_effect"):0, defaultInit);
			collect_effect.Reset(params?params->GetChild("collect_effect"):0, defaultInit);
		}

		void GetMemoryStatistics(ICrySizer * s)
		{
		}

		float						range;
		float						delay;

		CSingle::SEffectParams	energise_effect;
		CSingle::SEffectParams	collect_effect;
	};

	struct SEnergiseActions
	{
		SEnergiseActions() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(energise,	"energise");
			ResetValue(idle,			"idle");
			ResetValue(prefire,		"prefire");
			ResetValue(postfire,	"postfire");
		}

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(energise);
			s->Add(idle);
			s->Add(prefire);
			s->Add(postfire);
		}

		string	energise;
		string	idle;
		string	prefire;
		string	postfire;
	};

public:
	CEnergise();
	virtual ~CEnergise();

	//IFireMode
	virtual void Init(IWeapon *pWeapon, const struct IItemParamsNode *params);
	virtual void Update(float frameTime, uint frameId);
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime);
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
	virtual bool IsFiring() const { return m_energising || m_firing; };

	virtual void NetShoot(const Vec3 &hit, int ph);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int ph);

	virtual void NetStartFire(EntityId shooterId);
	virtual void NetStopFire(EntityId shooterId);

	virtual void Shoot(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	
	virtual EntityId GetProjectileId() const { return 0; };
	virtual void SetProjectileId(EntityId id) {};

	virtual const char *GetType() const;
	virtual IEntityClass* GetAmmoType() const { return 0; };
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

	virtual void PatchSpreadMod(SSpreadModParams &sSMP){};
	virtual void ResetSpreadMod(){};

	virtual void PatchRecoilMod(SRecoilModParams &sRMP){};
	virtual void ResetRecoilMod(){};

	virtual void ResetLock() {};
	virtual void StartLocking(EntityId targetId) {};
	virtual void Lock(EntityId targetId) {};
	virtual void Unlock() {};
	//~IFireMode

	float GetEnergy() const		{ return m_energy; }

protected:
	int	CallStartEnergise(IEntity *pEntity);
	float CallEnergise(IEntity *pEntity, float frameTime);
	float CallGetEnergy(IEntity *pEntity) const;
	bool CallCanEnergise(IEntity *pEntity, float energy) const;
	void	CallStopEnergise(IEntity *pEntity);

	void StopEnergise();
	void Energise(bool enable);
	void EnergyEffect(bool enable);
	void UpdateCollectEffect(IEntity *pEntity);
	IEntity *CanEnergise(Vec3 *pCenter=0, float *pRadius=0, bool *pCharged=0) const;
	CWeapon			*m_pWeapon;

	bool				m_enabled;
	bool				m_energising;
	bool				m_firing;
	float				m_energy;
	float				m_delayTimer;
	EntityId		m_lastEnergisingId;
	EntityId		m_effectEntityId;	
	int					m_effectSlot;

	int					m_effectId;
	tSoundID		m_soundId;

	string			m_name;

	SEnergiseParams		m_energiseparams;
	SEnergiseActions	m_energiseactions;
};


#endif //__ENERGISE_H__