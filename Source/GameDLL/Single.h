/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Single-shot Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 11:9:2004   15:00 : Created by Márcio Martins

*************************************************************************/
#ifndef __SINGLE_H__
#define __SINGLE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Weapon.h"
#include "ItemParamReader.h"
#include "TracerManager.h"


#define ResetValue(name, defaultValue) if (defaultInit) name=defaultValue; reader.Read(#name, name)
#define ResetValueEx(name, var, defaultValue) if (defaultInit) var=defaultValue; reader.Read(name, var)


#define WEAPON_HIT_RANGE				(2000.0f)
#define WEAPON_HIT_MIN_DISTANCE	(1.5f)

#define AUTOAIM_TIME_OUT				(10.0f)


class CSingle :
	public IFireMode
{
	struct EndReloadAction;
	struct StartReload_SliderBack;
	struct RezoomAction;
	struct Shoot_SliderBack;
	struct CockAction;
	class ScheduleReload;
	class SmokeEffectAction;


public:

	struct SEffectParams
	{
		SEffectParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{ 
			if (defaultInit)
			{
				effect[0].clear(); effect[1].clear();
				helper[0].clear(); helper[1].clear();
				scale[0]=scale[1]=1.0f;
				time[0]=time[1]=0.060f;
				light_helper[0].clear(); light_helper[1].clear();
				light_radius[0]=light_radius[1]=2.0f;
				light_color[0]=light_color[1]=Vec3(1,1,1);        
				light_time[0]=light_time[1]=0.060f;        
			}

			if (params)
			{
				const IItemParamsNode *fp=params->GetChild("firstperson");
				if (fp)
				{ 
					effect[0] = fp->GetAttribute("effect");
					helper[0] = fp->GetAttribute("helper");					
					fp->GetAttribute("scale", scale[0]);
					fp->GetAttribute("time", time[0]);
					light_helper[0] = fp->GetAttribute("light_helper");					
					fp->GetAttribute("light_radius", light_radius[0]);
					fp->GetAttribute("light_color", light_color[0]);          
					fp->GetAttribute("light_time", light_time[0]);

					float diffuse_mult = 1.f;
					fp->GetAttribute("light_diffuse_mult", diffuse_mult);
					light_color[0] *= diffuse_mult;
				}

				const IItemParamsNode *tp=params->GetChild("thirdperson");
				if (tp)
				{ 
					effect[1] = tp->GetAttribute("effect");
					helper[1] = tp->GetAttribute("helper");					
					tp->GetAttribute("scale", scale[1]);
					tp->GetAttribute("time", time[1]);
					light_helper[1] = tp->GetAttribute("light_helper");					
					tp->GetAttribute("light_radius", light_radius[1]);
					tp->GetAttribute("light_color", light_color[1]);          
					tp->GetAttribute("light_time", light_time[1]);

					float diffuse_mult = 1.f;
					tp->GetAttribute("light_diffuse_mult", diffuse_mult);
					light_color[1] *= diffuse_mult;
				}

				PreLoadAssets();
			}
		}
		void PreLoadAssets()
		{
			for (int i = 0; i < 2; i++)
				gEnv->p3DEngine->FindParticleEffect(effect[i]);
		}
		void GetMemoryStatistics(ICrySizer * s)
		{
			for (int i=0; i<2; i++)
			{
				s->Add(effect[i]);
				s->Add(helper[i]);
				s->Add(light_helper[i]);
			}
		}

		float	scale[2];
		float	time[2];
		ItemString effect[2];
		ItemString helper[2];
		float light_radius[2];
		float light_time[2];
		Vec3 light_color[2];    
		ItemString light_helper[2];    
	};

protected:

  struct SDustParams
  {
    SDustParams() { Reset(); }
    void Reset(const IItemParamsNode* params=0, bool defaultInit=true)
    {
      CItemParamReader reader(params);
      ResetValue(mfxtag, "");
      ResetValue(maxheight, 2.5f);
      ResetValue(maxheightscale, 1.f);
    }

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(mfxtag);
		}

    ItemString mfxtag;
    float maxheight;
    float maxheightscale;
  };

	struct SRecoilParams
	{
		SRecoilParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(max_recoil,	0.0f);
			ResetValue(attack,			0.0f);
			ResetValue(decay,				0.65f);
			ResetValueEx("maxx", max.x,8.0f);
			ResetValueEx("maxy", max.y,4.0f);
			ResetValue(randomness,	1.0f);
			ResetValue(impulse, 0.f);
			ResetValue(angular_impulse, 0.0f);
			ResetValue(back_impulse, 0.0f);
			ResetValue(hint_loop_start, 7);

			ResetValue(recoil_crouch_m, 0.85f);
			ResetValue(recoil_prone_m, 0.75f);
			ResetValue(recoil_jump_m, 1.5f);
			ResetValue(recoil_zeroG_m, 1.0f);

			ResetValue(recoil_strMode_m, 0.5f);

			if (defaultInit)
				hints.resize(0);

			if (params)
			{
				const IItemParamsNode *phints = params->GetChild("hints");
				if (phints && phints->GetChildCount())
				{
					Vec2 h;
					int n = phints->GetChildCount();
					hints.resize(n);

					for (int i=0; i<n; i++)
					{
						const IItemParamsNode *hint = phints->GetChild(i);
						if (hint->GetAttribute("x", h.x) && hint->GetAttribute("y", h.y))
							hints[i]=h;
					}
				}
			}
		}

		float								max_recoil;
		float								attack;
		float								decay;
		Vec2								max;
		float								randomness;
		std::vector<Vec2>		hints;
		float               impulse;
		float								angular_impulse;
		float								back_impulse;
		int									hint_loop_start;

		//Stance modifiers
		float								recoil_crouch_m;
		float								recoil_prone_m;
		float								recoil_jump_m;
		float								recoil_zeroG_m;

		//Nano suit modifiers
		float								recoil_strMode_m;

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->AddContainer(hints);
		}

	};

	struct SSpreadParams
	{
		SSpreadParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(min,						0.015f);
			ResetValue(max,						0.0f);
			ResetValue(attack,				0.95f);
			ResetValue(decay,					0.65f);
			ResetValue(speed_m,				0.25f);
			ResetValue(rotation_m,    0.35f);

			ResetValue(spread_crouch_m, 0.85f);
			ResetValue(spread_prone_m, 0.75f);
			ResetValue(spread_jump_m, 1.5f);
			ResetValue(spread_zeroG_m, 1.0f);

		}

		float	min;
		float max;
		float attack;
		float decay;
		float speed_m;
		float rotation_m;

		//Stance modifiers
		float	spread_crouch_m;
		float	spread_prone_m;
		float	spread_jump_m;
		float	spread_zeroG_m;

		void GetMemoryStatistics(ICrySizer * s)
		{
		}

	};

	struct STracerParams
	{
		STracerParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(geometry,	"");
			ResetValue(effect,		"");
			ResetValue(speed,			175.0f);
			ResetValue(scale,			1.0f);
			ResetValue(lifetime,	1.5f);
			ResetValue(frequency,	1);
			ResetValueEx("helper_fp", helper[0],	"");
			ResetValueEx("helper_tp",	helper[1],	"");

			PreLoadAssets();
		};

		ItemString	geometry;
		ItemString	effect;
		float		speed;
		float		scale;
		float		lifetime;
		int			frequency;
		ItemString	helper[2];

		void PreLoadAssets()
		{
			gEnv->p3DEngine->FindParticleEffect(effect);
		}
		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(geometry);
			s->Add(effect);
			s->Add(helper[0]);
			s->Add(helper[1]);
		}
	};

	struct SHeatingParams
	{
		SHeatingParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(attack,		0.0f);
			ResetValue(duration,	5.0f);
			ResetValue(decay,			1.0f/3.0f);

			ResetValueEx("helper_fp", helper[0],	"");
			ResetValueEx("helper_tp",	helper[1],	"");
			ResetValueEx("effect_fp", effect[0],	"");
			ResetValueEx("effect_tp",	effect[1],	"");

			PreLoadAssets();
		}

		float		attack;
		float		duration;
		float		decay;

		void PreLoadAssets()
		{
			for (int i = 0; i < 2; i++)
				gEnv->p3DEngine->FindParticleEffect(effect[i]);
		}
		void GetMemoryStatistics(ICrySizer * s)
		{
			for (int i=0; i<2; i++)
			{
				s->Add(helper[i]);
				s->Add(effect[i]);
			}
		}

		ItemString	helper[2];
		ItemString	effect[2];
	};

	struct SFireParams
	{
		SFireParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			string ammo_type;
			ResetValue(rate,					400);
			ResetValue(clip_size,			30);
			ResetValue(max_clips,			20);
			ResetValue(hit_type,			"bullet");
			ResetValue(ammo_type,			"");
			if (defaultInit || !ammo_type.empty())
				ammo_type_class = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo_type.c_str());
			ResetValue(reload_time,		2.0f);
			ResetValue(offset,				0.15f);      
			ResetValue(bullet_chamber,false);
			ResetValue(slider_layer,	"");
			ResetValue(slider_layer_time,	500);
			ResetValue(damage,				32);
			ResetValue(ai_vs_player_damage, 32);
			ResetValue(crosshair,			"default");
			ResetValue(unzoomed_cock,	false);
			ResetValue(no_cock,				false);
			ResetValue(ooatracer_treshold, 0);
			ResetValueEx("helper_fp", helper[0],	"");
			ResetValueEx("helper_tp",	helper[1],	"");
      ResetValue(barrel_count,  1);

			ResetValue(nearmiss_signal, "");

			ResetValue(spin_up_time,	0.0f);
			ResetValue(spin_down_time,0.0f);
			ResetValue(autoaim,		false);
			ResetValue(autoaim_zoom , false);
			ResetValue(autozoom,	false);
			ResetValue(autoaim_distance, 10.f);
			ResetValue(autoaim_tolerance, 1.f);
			ResetValue(autoaim_locktime, 0.85f);
			ResetValue(autoaim_minvolume, 1.f);
			ResetValue(autoaim_maxvolume, 1000.f);
			ResetValue(autoaim_timeout, false);
			ResetValue(autoaim_autofiringdir, true);
			ResetValue(track_projectiles, false);
			ResetValue(aim_helper, false);
			ResetValue(aim_helper_delay, 1.0f);
			ResetValue(damage_drop_per_meter, 0.0f);
			ResetValue(sound_variation,false);
			ResetValue(fake_fire_rate, 0);
			ResetValue(auto_fire, false);
			ResetValue(advanced_AAim,false);
			ResetValue(advanced_AAim_Range,1.0f);


			pierceability[0] = 0.0f;
			pierceability[1] = 0.05f;
			pierceability[2] = 0.05f;
			pierceability[3] = 0.05f;
			pierceability[4] = 0.05f;
			pierceability[5] = 0.05f;
			pierceability[6] = 0.05f;
			pierceability[7] = 0.15f;
			pierceability[8] = 0.15f;
			pierceability[9] = 0.25f;
			pierceability[10] = 0.5f;
			pierceability[11] = 1.0f;
			pierceability[12] = 1.5f;
			pierceability[13] = 2.0f;
			pierceability[14] = 2.0f;
		}

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(helper[0]);
			s->Add(helper[1]);
			s->Add(crosshair);
			s->Add(hit_type);
			s->Add(slider_layer);
			s->Add(nearmiss_signal);
		}

		short		rate;
		short		clip_size;
		short		max_clips;
		float		reload_time;
		float		offset;
		ItemString	helper[2];
    short   barrel_count;

		bool		unzoomed_cock;
		bool		no_cock;
		int			ooatracer_treshold;

		int			damage;
		int     ai_vs_player_damage; //Only for the hurricane

		ItemString	crosshair;

		ItemString	hit_type;
		IEntityClass* ammo_type_class;
		float		pierceability[15];

		bool		bullet_chamber;
		ItemString	slider_layer;
		int			slider_layer_time;

		float		spin_up_time;
		float		spin_down_time;

		ItemString	nearmiss_signal;

		bool	  autoaim;
		bool		autoaim_zoom;
		float   autoaim_distance;
		float   autoaim_tolerance;
		float   autoaim_locktime;
		float   autoaim_minvolume;
		float   autoaim_maxvolume;
		bool    autoaim_timeout;
		bool    autoaim_autofiringdir;
		bool		track_projectiles;
		bool		aim_helper;
		float		aim_helper_delay;

		bool	autozoom;

		float damage_drop_per_meter;
		bool  sound_variation;				//For the FY71 (usual weapon of the enemy)
		int   fake_fire_rate;					//Fake hurricanes fire rate
		bool  auto_fire;							//Single fire mode continue shooting while holding LMB

		bool  advanced_AAim;
		float advanced_AAim_Range;
	};

	struct SSingleActions
	{

		SSingleActions() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(fire,					"fire");
			ResetValue(fire_cock,			"fire");
			ResetValue(cock,					"cock");
			ResetValue(empty_clip,		"empty_clip");
			ResetValue(reload,				"reload");
			ResetValue(reload_chamber_full,	"reload_chamber_full");
			ResetValue(reload_chamber_empty, "reload_chamber_empty");
			ResetValue(spin_up,				"spin_up");
			ResetValue(spin_down,			"spin_down");
			ResetValue(overheating,		"overheating");
			ResetValue(cooldown,			"cooldown");
		}

		void GetMemoryStatistics(ICrySizer * s)
		{
			s->Add(fire);
			s->Add(fire_cock);
			s->Add(cock);
			s->Add(empty_clip);
			s->Add(reload);
			s->Add(reload_chamber_full);
			s->Add(reload_chamber_empty);
			s->Add(spin_up);
			s->Add(spin_down);
			s->Add(overheating);
			s->Add(cooldown);
		}

		ItemString	fire;
		ItemString	fire_cock;
		ItemString	cock;
		ItemString	empty_clip;
		ItemString	reload;
		ItemString	reload_chamber_full;
		ItemString	reload_chamber_empty;
		ItemString	spin_up;
		ItemString	spin_down;
		ItemString	overheating;
		ItemString	cooldown;
	};

public:
	CSingle();
	virtual ~CSingle();

	//IFireMode
	virtual void Init(IWeapon *pWeapon, const IItemParamsNode *params);
	virtual void Update(float frameTime, uint frameId);
	virtual void UpdateFPView(float frameTime);
	virtual void Release();
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void ResetParams(const struct IItemParamsNode *params);
	virtual void PatchParams(const struct IItemParamsNode *patch);

	virtual void Activate(bool activate);

	virtual int GetAmmoCount() const;
	virtual int GetClipSize() const;
	
	virtual bool OutOfAmmo() const;
	virtual bool CanReload() const;
	virtual void Reload(int zoomed);
	virtual void CancelReload();
	virtual bool CanCancelReload() { return true;};

	virtual bool CanFire(bool considerAmmo = true) const;
	virtual void StartFire(EntityId shooterId);
	virtual void StopFire(EntityId shooterId);
	virtual bool IsFiring() const { return m_firing; };
	
	virtual bool AllowZoom() const;
	virtual void Cancel();

	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int predictionHandle);

	virtual void NetStartFire(EntityId shooterId) {};
	virtual void NetStopFire(EntityId shooterId) {};

	virtual bool IsReadyToFire() const { return CanFire(true); };

	virtual EntityId GetProjectileId() const { return m_projectileId; };
	virtual void SetProjectileId(EntityId id) { m_projectileId = id; };

	virtual const char* GetType() const;
	virtual IEntityClass* GetAmmoType() const;

	virtual float GetSpinUpTime() const;
	virtual float GetSpinDownTime() const;
	virtual float GetNextShotTime() const;
	virtual float GetFireRate() const;

	virtual void Enable(bool enable);
	virtual bool IsEnabled() const;
	
  virtual bool HasFireHelper() const;
  virtual Vec3 GetFireHelperPos() const;
  virtual Vec3 GetFireHelperDir() const;

  virtual int GetCurrentBarrel() const { return m_barrelId; }  
	virtual void Serialize(TSerialize ser) {};

	virtual void PatchSpreadMod(SSpreadModParams &sSMP);
	virtual void ResetSpreadMod();

	virtual void PatchRecoilMod(SRecoilModParams &sRMP);
	virtual void ResetRecoilMod();

	virtual void ResetLock();
	virtual void StartLocking(EntityId targetId);
	virtual void Lock(EntityId targetId);
	virtual void Unlock();
  //~IFireMode

	virtual void StartReload(int zoomed);
	virtual void EndReload(int zoomed);
	virtual bool IsReloading();
	virtual bool Shoot(bool resetAnimation, bool autoreload=true, bool noSound=false);

	virtual bool ShootFromHelper(const Vec3 &eyepos, const Vec3 &probableHit) const;
	virtual Vec3 GetProbableHit(float range, bool *pbHit=0, ray_hit *hit=0) const;
	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const;
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	virtual Vec3 GetFiringVelocity(const Vec3& dir) const;
	virtual Vec3 ApplySpread(const Vec3 &dir, float spread);

	virtual Vec3 NetGetFiringPos(const Vec3 &probableHit) const;
	virtual Vec3 NetGetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	virtual Vec3 NetGetFiringVelocity(const Vec3& dir) const;

	virtual Vec3 GetTracerPos(const Vec3 &firingPos, bool ooa);

	virtual int GetDamage() const;

	virtual float GetRecoil() const;
	virtual float GetSpread() const;
	virtual float GetRecoilScale() const;
	virtual const char *GetCrosshair() const;
	virtual float GetHeat() const;

	virtual void MuzzleFlashEffect(bool attach, bool light=true, bool effect=true);
	virtual void SmokeEffect(bool effect=true);
	virtual void SpinUpEffect(bool attach);
	virtual void RejectEffect();
  virtual void DustEffect(const Vec3& pos);
  virtual void RecoilImpulse(const Vec3& firingPos, const Vec3& firingDir);

	virtual void UpdateHeat(float frameTime);

	// recoil/spread
	virtual void UpdateRecoil(float frameTime);
	virtual void ResetRecoil(bool spread=true);

	virtual void SetRecoilMultiplier(float recoilMult) { m_recoilMultiplier = recoilMult; }
	virtual float GetRecoilMultiplier() const { return m_recoilMultiplier; }

	virtual void UpdateAutoAim(float frameTime);
	virtual void PostUpdate(float frameTime);
	virtual void SetName(const char *name);
	virtual const char *GetName() { return m_name.empty()?0:m_name.c_str(); };


	virtual float GetProjectileFiringAngle(float v, float g, float x, float y);

	virtual void SetupEmitters(bool attach);
	// sound and animation flags
	virtual int PlayActionSAFlags(int flags) { return flags | CItem::eIPAF_Sound | CItem::eIPAF_Animation; };

	static int GetSkipEntities(CWeapon* pWeapon, IPhysicalEntity** pSkipEnts, int nMaxSkip);

protected:

	void CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius, const char *signalName);
	void CacheTracer();
	void ClearTracerCache();
	bool IsValidAutoAimTarget(IEntity* pEntity);
	bool CheckAutoAimTolerance(const Vec3& aimPos, const Vec3& aimDir);

	void BackUpOriginalSpreadRecoil();
	void SetAutoFireTimer(float time) { m_autoFireTimer = time; }
	void AutoFire();

	void SetAutoAimHelperTimer(float time) { m_autoAimHelperTimer = time;}

	std::vector<IStatObj *> m_tracerCache;


	CWeapon		*m_pWeapon;

	EntityId	m_shooterId;
	bool			m_fired;
	bool			m_firing;
	bool			m_reloading;
	bool			m_emptyclip;

	float			m_next_shot_dt;
	float			m_next_shot;
  short     m_barrelId;

	EntityId	m_projectileId;

  struct SMuzzleEffectInfo
  {
    uint mfId[2];

    SMuzzleEffectInfo()
    {
      mfId[0] = mfId[1] = 0;
    }
  };
    
  std::vector<SMuzzleEffectInfo> m_mfIds;
	uint			m_mflightId[2];	
	float			m_mflTimer;

	uint			m_suId;
	uint			m_sulightId;
	float			m_suTimer;

	float			m_recoil;
	int				m_recoil_dir_idx;
	Vec2			m_recoil_dir;
	Vec2			m_recoil_offset;
	float			m_spread;

	float			m_recoilMultiplier;			//Increment recoil while holding an object

	float			m_speed_scale;

	bool			m_enabled;

	uint						m_ammoid;

	float						m_spinUpTime;

	SFireParams			m_fireparams;
	STracerParams		m_tracerparams;
	STracerParams		m_ooatracerparams;
	SSingleActions	m_actions;
	SEffectParams		m_muzzleflash;
	SEffectParams		m_muzzlesmoke;
	SEffectParams		m_reject;
	SEffectParams		m_spinup;  
	SRecoilParams		m_recoilparams;
	SRecoilParams   m_recoilparamsCopy;		//Must be better to have a copy (after mult/div could lose precision)
	SSpreadParams		m_spreadparams;
	SSpreadParams   m_spreadparamsCopy;		//Must be better to have a copy (after mult/div could lose precision)
	SHeatingParams	m_heatingparams;
  SDustParams     m_dustparams;

	float						m_heat;
	float						m_overheat;
	int							m_heatEffectId;
  tSoundID        m_heatSoundId;

	EntityId				m_lastNearMissId;
	bool						m_firstShot;

	float						m_zoomtimeout;
	EntityId				m_lockedTarget;
	bool						m_bLocked;
	float						m_fStareTime;
	bool						m_bLocking;

	string					m_name;	
	bool						m_targetSpotSelected;
	Vec3						m_targetSpot;
	Vec3						m_lastAimSpot;
	float						m_autoAimHelperTimer;

	float						m_autoaimTimeOut;
	
	float						m_autoFireTimer;

	float						m_soundVariationParam; //Should be 1.0, 2.0f, 3.0f (or something in between 0 and 5)	

	bool						m_reloadCancelled;
	int							m_reloadStartFrame;
};


#endif //__SINGLE_H__
