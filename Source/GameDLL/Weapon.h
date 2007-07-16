/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Weapon Implementation

-------------------------------------------------------------------------
History:
- 22:8:2005   12:50 : Created by Márcio Martins

*************************************************************************/
#ifndef __WEAPON_H__
#define __WEAPON_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IItemSystem.h>
#include <IWeapon.h>
#include <IAgent.h>
#include "Item.h"


#define WEAPON_FADECROSSHAIR_SELECT	(0.250f)
#define WEAPON_FADECROSSHAIR_ZOOM		(0.200f)
#define WEAPON_SHOOT_TIMER					(5000)


class CProjectile;
class CWeapon :
	public CItem,
	public IWeapon
{
	class ScheduleLayer_Leave;
	class ScheduleLayer_Enter;
	struct EndChangeFireModeAction;
	struct EndRaiseWeaponAction;
	struct PlayLeverLayer;

protected:
	typedef std::map<string, int>								TFireModeIdMap;
	typedef std::vector<IFireMode *>						TFireModeVector;
	typedef std::map<string, int>								TZoomModeIdMap;
	typedef std::vector<IZoomMode *>						TZoomModeVector;
	typedef std::map<IEntityClass*, int>				TAmmoMap;

	struct SListenerInfo
	{
		IWeaponEventListener	*pListener;
#ifdef _DEBUG
		char who[64];
#endif
	};

	typedef std::vector<SListenerInfo>					TEventListenerVector;

	typedef struct SWeaponCrosshairStats
	{
		SWeaponCrosshairStats():
			fading(false),
			visible(true),
			fadefrom(1.0f),
			fadeto(1.0f),
			fadetime(0.0f),
			fadetimer(0.0f),
			opacity(1.0f)
		{
		}

		bool fading;
		bool visible;
		float fadefrom;
		float fadeto;
		float fadetime;
		float fadetimer;
		float opacity;
	};
public:
	CWeapon();
	virtual ~CWeapon();

	// IItem, IGameObjectExtension
	virtual bool Init(IGameObject * pGameObject);
	virtual void InitClient(int channelId) { CItem::InitClient(channelId); };
	virtual void Release();
	virtual void Serialize(TSerialize ser, unsigned aspects);
	virtual void Update(SEntityUpdateContext& ctx, int);
	virtual void PostUpdate( float frameTime );
	virtual void HandleEvent(const SGameObjectEvent&);
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority(bool auth);
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void Reset();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void UpdateFPView(float frameTime);

	virtual IWeapon *GetIWeapon() { return this; };

	virtual void MeleeAttack();
	virtual bool CanMeleeAttack() const;
	virtual IFireMode *GetMeleeFireMode() const { return m_melee; };

	virtual void PerformThrow(EntityId shooterId, float speedScale) {};

  virtual void Select(bool select);
	virtual void Drop(float impulseScale, bool selectNext=true, bool byDeath=false);
	virtual void Freeze(bool freeze);

	virtual void OnPickedUp(EntityId actorId, bool destroyed);
	virtual void OnDropped(EntityId actorId);

  virtual void OnHit(float damage, const char* damageType);
  virtual void OnDestroyed();
  virtual void EnterWater(bool enter);

	virtual void StopUse(EntityId userId); //Needed for the mounted weapon

	virtual bool CheckAmmoRestrictions(EntityId pickerId);

	// ~IItem
	virtual bool HasAttachmentAtHelper(const char *helper);
	virtual void GetAttachmentsAtHelper(const char *helper, std::vector<string> &rAttachments);
	// Events
	virtual void OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, 
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(EntityId shooterId);
	virtual void OnStopFire(EntityId shooterId);
	virtual void OnStartReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnEndReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnOutOfAmmo(IEntityClass* pAmmoType);
	virtual void OnReadyToFire();
	virtual void OnMelee(EntityId shooterId);
	virtual void OnStartTargetting(IWeapon *pWeapon);
	virtual void OnStopTargetting(IWeapon *pWeapon);

	// viewmodes
	virtual void IncrementViewmode();
	virtual void ExitViewmodes();

	// IWeapon
	virtual void SetFiringLocator(IWeaponFiringLocator *pLocator);
	virtual IWeaponFiringLocator *GetFiringLocator() const;

	virtual void AddEventListener(IWeaponEventListener *pListener, const char *who);
	virtual void RemoveEventListener(IWeaponEventListener *pListener);

  virtual void SetDestinationEntity(EntityId targetId);
  virtual void SetDestination(const Vec3& pos){ m_destination = pos; }
	virtual const Vec3& GetDestination(){ return m_destination; }

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const;
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	
	virtual void StartFire(EntityId shooterId);
	virtual void StopFire(EntityId shooterId);
	virtual bool CanFire() const;

	virtual void StartZoom(EntityId shooterId, int zoomed = 0);
	virtual void StopZoom(EntityId shooterId);
	virtual bool CanZoom() const;
	virtual void ExitZoom();
	virtual bool IsZoomed() const;
	virtual bool IsZooming() const;
	
	virtual void MountAt(const Vec3 &pos);
	virtual void MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles);

	virtual void Reload(bool force=false);
	virtual bool CanReload() const;

	virtual bool OutOfAmmo(bool allFireModes) const;

	virtual int GetAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetAmmoCount(IEntityClass* pAmmoType, int count);

	virtual int GetInventoryAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetInventoryAmmoCount(IEntityClass* pAmmoType, int count);

	virtual int GetNumOfFireModes() const { return m_firemodes.size(); }
	virtual IFireMode *GetFireMode(int idx) const;
	virtual IFireMode *GetFireMode(const char *name) const;
	virtual int GetFireModeIdx(const char *name) const;
	virtual int GetCurrentFireMode() const;
	virtual void SetCurrentFireMode(int idx);
	virtual void SetCurrentFireMode(const char *name);
	virtual void ChangeFireMode();
	virtual int GetNextFireMode(int currMode) const;
	virtual void EnableFireMode(int idx, bool enable);
	virtual void FixAccessories(SAccessoryParams *newParams, bool attach);

	virtual IZoomMode *GetZoomMode(int idx) const;
	virtual IZoomMode *GetZoomMode(const char *name) const;
	virtual int GetZoomModeIdx(const char *name) const;
	virtual int GetCurrentZoomMode() const;
	virtual void SetCurrentZoomMode(int idx);
	virtual void SetCurrentZoomMode(const char *name);
	virtual void ChangeZoomMode();
	virtual void EnableZoomMode(int idx, bool enable);
	virtual void RestartZoom(bool force = false);

	virtual void SetCrosshairVisibility(bool visible);
	virtual bool GetCrosshairVisibility() const;
	virtual void SetCrosshairOpacity(float opacity);
	virtual float GetCrosshairOpacity() const;
	virtual void FadeCrosshair(float from, float to, float time);
	virtual void UpdateCrosshair(float frameTime);
  
	virtual void AccessoriesChanged();
	virtual void PatchFireModeWithAccessory(IFireMode *pFireMode, const char *firemodeName);
	virtual void PatchZoomModeWithAccessory(IZoomMode *pZoomMode, const char *zoommodeName);

	virtual float GetSpinUpTime() const;
	virtual float GetSpinDownTime() const;

	virtual void SetHostId(EntityId hostId);
	virtual EntityId GetHostId() const;

	virtual bool	PredictProjectileHit(IPhysicalEntity *pShooter, const Vec3 &pos, const Vec3 &dir,
		const Vec3 &velocity, float speed, Vec3& predictedPosOut, float& projectileSpeedOut,
		Vec3* pTrajectory = 0, unsigned int* trajectorySizeInOut = 0) const;

	virtual void ForceHitMaterial(int surfaceId) { m_forcedHitMaterial=surfaceId; };
	virtual int GetForcedHitMaterial() const { return m_forcedHitMaterial; };

	virtual const AIWeaponDescriptor& GetAIWeaponDescriptor( ) const;

	//Activate/Deactivate Laser and Light for the AI (also player if neccessary)
	virtual bool		IsLamAttached();
	virtual void    ActivateLamLaser(bool activate, bool aiRequest = true);
	virtual void		ActivateLamLight(bool activate, bool aiRequest = true);
	virtual bool	  IsLamLaserActivated();
	virtual bool		IsLamLightActivated();

	// ~IWeapon

	int  GetMaxZoomSteps();
	void AssistAiming(float magnification=1.0f, bool accurate=false);
	bool IsValidAssistTarget(IEntity *pEntity, IEntity *pSelf, bool includeVehicles=false);

	void AdvancedAssistAiming(float range, const Vec3& pos, Vec3 &dir);


	bool		IsSilencerAttached() { return m_silencerAttached; }
//	bool		IsLAMAttached() { return m_lamAttached; }

	//Activate/Deactivate Laser and Light for the AI (also player if neccessary)
//	void    ActivateLamLaser(bool activate, bool aiRequest = true);
//	void		ActivateLamLight(bool activate, bool aiRequest = true);

	void		StartChangeFireMode();
	void		EndChangeFireMode(int zoomed);
	bool    IsSwitchingFireMode() { return m_switchingFireMode; };

	//Targeting stuff
	bool		IsTargetOn() { return m_targetOn; }
	void		ActivateTarget(bool activate) { m_targetOn = activate; }
	void		SetAimLocation(Vec3 &location) { m_aimPosition = location; }
	void		SetTargetLocation(Vec3 &location) { m_targetPosition = location; }
	Vec3&   GetAimLocation(){ return m_aimPosition; }
	Vec3&		GetTargetLocation() { return m_targetPosition; }

	//Raise weapon
	virtual	void		RaiseWeapon(bool raise, bool faster = false);
	virtual	bool		IsWeaponRaised() { return (m_weaponRaised); }
	virtual	float		GetRaiseDistance() { return m_params.raise_distance; }
	virtual void		SetWeaponRaised(bool raised) { m_weaponRaised = raised; }
	virtual bool		CanBeRaised() { return m_params.raiseable; }

	virtual void		LowerWeapon(bool lower) { m_weaponLowered = lower; }
	virtual bool		IsWeaponLowered() { return m_weaponLowered; }

	bool		GetFireAlternation() { return m_fire_alternation;}
	void		SetFireAlternation(bool fireAlt) { m_fire_alternation = fireAlt;}

	//LAW special stuff
	void		AutoDrop();

	// network
	struct SvRequestShootParams
	{
		SvRequestShootParams() {};
		SvRequestShootParams(const Vec3& pos_, const Vec3& dir_, const Vec3 &at, int ph) : pos(pos_), dir(dir_), hit(at), predictionHandle(ph) {};

		Vec3 pos;
		Vec3 dir;
		Vec3 hit;
		int predictionHandle;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
			ser.Value("hit", hit, 'sHit');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
	};

	struct ClShootParams
	{
		ClShootParams() {};
		ClShootParams(const Vec3 &at, int ph): hit(at), predictionHandle(ph) {};

		Vec3 hit;
		int predictionHandle;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("hit", hit, 'sHit');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
	};

	struct ClShootXParams
	{
		ClShootXParams() {};
		ClShootXParams(EntityId eid_, bool hit0_, const Vec3& hit_, int ph) : eid(eid_), hit0(hit0_), hit(hit_), predictionHandle(ph) {};

		EntityId eid;
		bool hit0;
		Vec3 hit;
		int predictionHandle;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("ref_eid", eid, 'eid');
			ser.Value("use_hit", hit0, 'bool');
			ser.Value("ref_hit", hit, hit0 ? 'hit0' : 'hit1');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		}
	};

	struct SvRequestShootExParams
	{
		SvRequestShootExParams() {};
		SvRequestShootExParams(const Vec3 &_pos, const Vec3 &_dir, const Vec3 &_vel, const Vec3 &_hit, int ph)
		: pos(_pos), dir(_dir), vel(_vel), hit(_hit), predictionHandle(ph) {};

		Vec3 pos;
		Vec3 dir;
		Vec3 vel;
		Vec3 hit;
		int predictionHandle;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
			ser.Value("vel", vel, 'vel0');
			ser.Value("hit", hit, 'wrld');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
	};

	struct SvRequestFireModeParams
	{
		SvRequestFireModeParams(): id(0) {};
		SvRequestFireModeParams(int fmId): id(fmId) {};

		int id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'fmod');
		};
	};

	struct RequestMeleeAttackParams
	{
		RequestMeleeAttackParams() {};
		RequestMeleeAttackParams(bool _wmelee, const Vec3 &_pos, const Vec3 &_dir): wmelee(_wmelee), pos(_pos), dir(_dir) {};

		bool wmelee; // is this the special weapon melee mode?
		Vec3 pos;
		Vec3 dir;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("wmelee", wmelee, 'bool');
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
		}
	};

	struct RequestStartMeleeAttackParams
	{
		RequestStartMeleeAttackParams() {};
		RequestStartMeleeAttackParams(bool _wmelee): wmelee(_wmelee) {};

		bool wmelee; // is this the special weapon melee mode?

		void SerializeWith(TSerialize ser)
		{
			ser.Value("wmelee", wmelee, 'bool');
		}
	};

	struct ClSetFireModeParams
	{
		ClSetFireModeParams(): id(0) {};
		ClSetFireModeParams(int fmId): id(fmId) {};

		int id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'fmod');
		};
	};

	struct EmptyParams
	{
		EmptyParams() {};
		void SerializeWith(TSerialize ser) {}
	};

	struct LockParams
	{
		LockParams(): entityId(0) {};
		LockParams(EntityId id): entityId(id) {};

		EntityId entityId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct ZoomParams
	{
		ZoomParams(): fov(0) {};
		ZoomParams(float _fov): fov(_fov) {};

		float fov;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("fov", fov, 'frad');
		}
	};


	static const int ASPECT_FIREMODE = eEA_GameServerStatic;
	static const int ASPECT_AMMO = eEA_GameServerDynamic;

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestShoot, SvRequestShootParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestShootEx, SvRequestShootExParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClShoot, ClShootParams, eNRT_UnreliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClShootX, ClShootXParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestStartFire, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestStopFire, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClStartFire, EmptyParams, eNRT_UnreliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClStopFire, EmptyParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestStartMeleeAttack, RequestStartMeleeAttackParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClStartMeleeAttack, RequestStartMeleeAttackParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestMeleeAttack, RequestMeleeAttackParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClMeleeAttack, RequestMeleeAttackParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestZoom, ZoomParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClZoom, ZoomParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestFireMode, SvRequestFireModeParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_POSTATTACH(ClSetFireMode, ClSetFireModeParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestReload, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClReload, EmptyParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestCancelReload, EmptyParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClCancelReload, EmptyParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClStartLocking, LockParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClLock, LockParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClUnlock, EmptyParams, eNRT_ReliableUnordered);
	
	virtual int NetGetCurrentFireMode() const;
	virtual void NetSetCurrentFireMode(int id);

	virtual int NetGetCurrentAmmoCount() const;
	virtual void NetSetCurrentAmmoCount(int count);

	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int predictionHandle);
	
	virtual void NetStartFire(EntityId shooterId);
	virtual void NetStopFire(EntityId shooterId);

	virtual void NetStartMeleeAttack(EntityId shooterId, bool weaponMelee);
	virtual void NetMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir);

	virtual void NetZoom(float fov);

	void RequestShoot(IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int predictionHandle, bool forceExtended);
	void RequestStartFire();
	void RequestStopFire();
	void RequestReload();
	void RequestFireMode(int fmId);

	void RequestStartMeleeAttack(bool weaponMelee);
	void RequestMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir);
	void RequestZoom(float fov);

	void RequestCancelReload();
	
	bool IsServerSpawn(IEntityClass* pAmmoType) const;
	CProjectile *SpawnAmmo(IEntityClass* pAmmoType, bool remote=false);
protected:
	virtual bool ReadItemParams(const IItemParamsNode *params);
	const IItemParamsNode *GetFireModeParams(const char *name);
	const IItemParamsNode *GetZoomModeParams(const char *name);
	void InitFireModes(const IItemParamsNode *firemodes);
	void InitZoomModes(const IItemParamsNode *zoommodes);
	void InitAmmos(const IItemParamsNode *ammo);
	void InitAIData(const IItemParamsNode *aiDescriptor);

	EntityId	GetLAMAttachment();

	IFireMode					*m_fm;
	int								m_fmId;

	IFireMode					*m_melee;

	IZoomMode					*m_zm;
	int								m_zmId;

	TFireModeIdMap		m_fmIds;
	TFireModeVector		m_firemodes;

	TZoomModeIdMap		m_zmIds;
	TZoomModeVector		m_zoommodes;

	TAmmoMap					m_ammo;
	TAmmoMap					m_bonusammo;
	TAmmoMap					m_accessoryAmmo;

	bool							m_fire_alternation;

	bool							m_restartZoom; //this is a serialization helper
	int								m_restartZoomStep;
	int								m_zoomViewMode;

	const IItemParamsNode	*m_fmDefaults;
	const IItemParamsNode *m_zmDefaults;
	const IItemParamsNode	*m_xmlparams;

	TEventListenerVector	m_listeners;
	static TEventListenerVector * m_listenerCache;
	static bool m_listenerCacheInUse;
	IWeaponFiringLocator	*m_pFiringLocator;


	int										m_forcedHitMaterial;

	SWeaponCrosshairStats	m_crosshairstats;


	float	m_dofValue;
	float	m_dofSpeed;
	float m_focusValue;


  Vec3 m_destination;

	AIWeaponDescriptor	m_aiWeaponDescriptor;

	std::vector< ItemString >	m_viewModeList;
	int						m_currentViewMode;
	int						m_defaultViewMode;
	bool					m_useViewMode;

	bool					m_silencerAttached;
	bool					m_lamAttached;
	EntityId			m_lamID;

	bool					m_targetOn;
	Vec3					m_aimPosition;
	Vec3					m_targetPosition;

	bool					m_weaponRaised;
	bool					m_weaponRaising;
	bool					m_fireAfterLowering;
	bool					m_weaponLowered;

	bool					m_switchingFireMode;
	bool          m_switchLeverLayers;
};


#endif //__WEAPON_H__
