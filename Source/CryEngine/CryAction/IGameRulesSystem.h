/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 20:10:2004   10:30 : Created by Craig Tiller

*************************************************************************/
#ifndef __IGAMERULES_H__
#define __IGAMERULES_H__

#pragma once


#include "IGameObject.h"

// Summary
//   Types for the different kind of messages
enum ETextMessageType
{
	eTextMessageCenter=0,
	eTextMessageConsole, // console message
	eTextMessageError, // error message
	eTextMessageInfo, // info message
	eTextMessageServer
};

// Summary
//   Types for the different kind of chat messages
enum EChatMessageType
{
	eChatToTarget=0, // the message is to be sent to the target entity
	eChatToTeam, // the message is to be sent to the team of the sender
	eChatToAll // the message is to be sent to all client connected
};

// Summary
//   Simplified structure to describe an hit
// Description
//   This structure is used with the GameRules to simplify to procedure of creating an hit.
// See Also
//   HitInfo, ExplosionInfo, IGameRules::ClientSimpleHit, IGameRules::ServerSimpleHit
struct SimpleHitInfo
{
	bool			remote;
	EntityId  shooterId; // EntityId of the shooter
	EntityId  weaponId; // EntityId of the weapon
	EntityId	targetId; // EntityId of the target which got shot
	int				type; // type id of the hit, see IGameRules::GetHitTypeId for more information

	SimpleHitInfo(): remote(false), shooterId(0), targetId(0), weaponId(0), type(0) {};
	SimpleHitInfo(EntityId shtId, EntityId trgId, EntityId wpnId, int typ)
		: remote(false),
		shooterId(shtId),
		targetId(trgId),
		weaponId(wpnId),
		type(typ) {};

	void SerializeWith(TSerialize ser)
	{
		ser.Value("shooterId", shooterId, 'eid');
		ser.Value("targetId", targetId, 'eid');
		ser.Value("weaponId", weaponId, 'eid');
		ser.Value("type", type, 'hTyp');
	}
};

// Summary
//   Structure to describe an hit
// Description
//   This structure is used with the GameRules to create an hit.
// See Also
//   SimpleHitInfo, ExplosionInfo,  IGameRules::ClientSimpleHit, IGameRules::ServerSimpleHit
struct HitInfo
{
  EntityId shooterId; // EntityId of the shooter
  EntityId targetId; // EntityId of the weapon
  EntityId weaponId; // EntityId of the target which got shot
  EntityId projectileId;  // 0 if not hit was not caused by a projectile

  float		damage; // damage count of the hit
  float   frost; // frost amount of the hit. 1: full freeze
  float		radius; // radius of the hit
	float		angle;
  int			material; // material id of the surface which got hit
  int			type; // type id of the hit, see IGameRules::GetHitTypeId for more information
	int			bulletType; //type of bullet, if hit was of type bullet
  
  int			partId;

  Vec3		pos; // position of the hit
  Vec3		dir; // direction of the hit
  Vec3		normal;

  bool		remote;

  HitInfo()
    : shooterId(0),
    targetId(0),
    weaponId(0),
    projectileId(0),
    damage(0),
    frost(0),
    radius(0),
    material(-1),
    partId(-1),
    type(0),
    pos(0,0,0),
    dir(FORWARD_DIRECTION),
    normal(FORWARD_DIRECTION),
    remote(false),
		bulletType(-1)
  {}

  HitInfo(EntityId shtId, EntityId trgId, EntityId wpnId, float dmg, float rd, int mat, int part, int typ)
    : shooterId(shtId),
    targetId(trgId),
    weaponId(wpnId),
    projectileId(0),
    damage(dmg),
    frost(0),
    radius(rd),
    material(mat),
    partId(part),
    type(typ),
    pos(0,0,0),
    dir(FORWARD_DIRECTION),
    normal(FORWARD_DIRECTION),
    remote(false),
		bulletType(-1)
  {}

  HitInfo(EntityId shtId, EntityId trgId, EntityId wpnId, float dmg, float rd, int mat, int part, int typ, const Vec3 &p, const Vec3 &d, const Vec3 &n)
    : shooterId(shtId),
    targetId(trgId),
    weaponId(wpnId),
    projectileId(0),
    damage(dmg),
    frost(0),
    radius(rd),
    material(mat),
    partId(part),
    type(typ),
    pos(p),
    dir(d),
    normal(n),
    remote(false),
		bulletType(-1)
  {}

  void SerializeWith(TSerialize ser)
  {
    ser.Value("shooterId", shooterId, 'eid');
    ser.Value("targetId", targetId, 'eid');
    ser.Value("weaponId", weaponId, 'eid');
    ser.Value("projectileId", projectileId, 'eid');
		ser.Value("damage", damage, 'dmg');
    ser.Value("frost", frost, 'frzn');
		ser.Value("radius", radius, 'hRad');
    ser.Value("material", material, 'mat');
    ser.Value("partId", partId, 'part');
    ser.Value("type", type, 'hTyp');
		ser.Value("pos", pos, 'wrld');
    ser.Value("dir", dir, 'dir1');
    ser.Value("normal", normal, 'dir1');
  }
};

// Summary
//   Structure to describe an explosion
// Description
//   This structure is used with the GameRules to create an explosion.
// See Also
//   HitInfo
struct ExplosionInfo
{
  EntityId shooterId; // Id of the shooter who triggered the explosion
  EntityId weaponId; // Id of the weapon used to create the explosion
  
  float		damage; // damage created by the explosion

  Vec3		pos; // position of the explosion
  Vec3		dir; // direction of the explosion
  float		radius; // radius of the explosion
	float		angle;
  float		pressure; // pressure created by the explosion
  float		hole_size;
  IParticleEffect* pParticleEffect;
	string	effect_class;
  float		effect_scale;
	int			type; // type id of the hit, see IGameRules::GetHitTypeId for more information

	bool		impact;
	Vec3		impact_normal;
	Vec3		impact_velocity;
  EntityId impact_targetId; 
	float		maxblurdistance;

  ExplosionInfo()
    : shooterId(0),
    weaponId(0),    
    damage(0),
    pos(0,0,0),
    dir(FORWARD_DIRECTION),
    radius(5.0f),
		angle(0.0f),
    pressure(200.0f),
    hole_size(5.0f),
		pParticleEffect(0),
		type(0),
		effect_scale(1.0f),
		maxblurdistance(0.0f),
		impact(false),
		impact_normal(FORWARD_DIRECTION),
		impact_velocity(ZERO),
    impact_targetId(0)
  {}

  ExplosionInfo(EntityId shtId, EntityId wpnId, float dmg, const Vec3 &p, const Vec3 &d, float r, float a, float press, float holesize, int typ)
   : shooterId(shtId),
    weaponId(wpnId),    
    damage(dmg),
    pos(p),
    dir(d),
    radius(r),
		angle(a),
    pressure(press),
    hole_size(holesize),
		type(typ),
		effect_scale(1.0f),
		maxblurdistance(0.0f),
		impact(false),
		impact_normal(FORWARD_DIRECTION),
		impact_velocity(ZERO),
    impact_targetId(0)
  {}

	void SetImpact(const Vec3 &normal, const Vec3 &velocity, EntityId targetId)
	{
		impact=true;
		impact_normal=normal;
		impact_velocity=velocity;
    impact_targetId=targetId;
	}

	void SetEffect(IParticleEffect* pParticleEffect, float scale, float maxblurdistance)
	{
		this->pParticleEffect=pParticleEffect;
		this->maxblurdistance=maxblurdistance;
		effect_scale=scale;
	}

	void SetEffectClass(const char *cls)
	{
		effect_class = cls;
	}

  void SerializeWith(TSerialize ser)
  {
    ser.Value("shooterId", shooterId, 'eid');
    ser.Value("weaponId", weaponId, 'eid');    
		ser.Value("damage", damage, 'dmg');
    ser.Value("pos", pos, 'wrld');
    ser.Value("dir", dir, 'dir1');
    ser.Value("radius", radius, 'hRad');
		ser.Value("angle", angle, 'hAng');
		ser.Value("pressure", pressure, 'hPrs');
		ser.Value("hole_size", hole_size, 'hHSz');
    ser.Value("type", type, 'hTyp');

		ser.Value("effect_class", effect_class);

		if (ser.BeginOptionalGroup("effect", pParticleEffect!=0))
		{
			if(ser.IsWriting())
			{
				ser.Value("effect", pParticleEffect->GetName());
			}
			else
			{
				string effect;
				ser.Value("effect", effect);
				pParticleEffect = gEnv->p3DEngine->FindParticleEffect(effect.c_str());
			}
			ser.Value("effect_scale", effect_scale, 'hESc');
			ser.Value("maxblurdistance", maxblurdistance, 'iii');
			ser.EndGroup();
		}

		if (ser.BeginOptionalGroup("impact", impact))
		{
			if (ser.IsReading())
				impact=true;
			ser.Value("impact_normal", impact_normal, 'dir1');
			ser.Value("impact_velocity", impact_velocity, 'pPVl');
      ser.Value("impact_targetId", impact_targetId, 'eid');
			ser.EndGroup();
		}
  };
};

// Summary
//   Interface to implement an hit listener
// See Also
//   IGameRules, IGameRules::AddHitListener, IGameRules::RemoveHitListener
struct IHitListener
{
	// Summary
	//   Function called when the GameRules process an hit
	// See Also
	//   IGameRules
	virtual void OnHit(const HitInfo&) = 0;

	// Summary
	//   Function called when the GameRules process an explosion (client side)
	// See Also
	//   IGameRules
	virtual void OnExplosion(const ExplosionInfo&) = 0;

	// Summary 
	//	Function called when the GameRules process an explosion (server side)
	// See Also
	//	IGameRules
	virtual void OnServerExplosion(const ExplosionInfo&)  = 0;
};

// Summary
//   Interface used to implement the game rules
struct IGameRules : public IGameObjectExtension
{
	// Summary
	//   Returns wether the disconnecting client should be kept for a few more moments or not.
	virtual bool ShouldKeepClient(int channelId, EDisconnectionCause cause, const char *desc) const = 0;

	// Summary
	//   Called after level loading, to precache anything needed.
	virtual void PrecacheLevel() = 0;

	// client notification
	virtual void OnConnect(struct INetChannel *pNetChannel) = 0;
	virtual void OnDisconnect(EDisconnectionCause cause, const char *desc) = 0; // notification to the client that he has been disconnected

	// Summary
	//   Notifies the server when a client is connecting
	virtual bool OnClientConnect(int channelId) = 0;

	// Summary
	//   Notifies the server when a client is disconnecting
	virtual void OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient) = 0;

	// Summary
	//   Notifies the server when a client has entered the current game
	virtual bool OnClientEnteredGame(int channelId) = 0;

	// Summary
	//   Broadcasts a message to the clients in the game
	// Parameters
	//   type - indicated the message type
	//   msg - the message to be sent
	virtual void SendTextMessage(ETextMessageType type, const char *msg, uint to=eRMI_ToAllClients, int channelId=-1,
		const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0) = 0;

	// Summary
	//   Broadcasts a chat message to the clients in the game which are part of the target
	// Parameters
	//   type - indicated the message type
	//   sourceId - EntityId of the sender of this message
	//   targetId - EntityId of the target, used in case type is set to eChatToTarget
	//   msg - the message to be sent
	virtual void SendChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg) = 0;

	// Summary
	//   Performs client tasks needed for an hit using a simplified structure
	// Parameters
	//   hitInfo - structure holding all the information about the hit
	// See Also
	//   ClientSimpleHit, ClientHit, ServerHit
	virtual void ClientSimpleHit(const SimpleHitInfo &simpleHitInfo) = 0;

	// Summary
	//   Performs server tasks needed for an hit using a simplified structure
	// Parameters
	//   hitInfo - structure holding all the information about the hit
	// See Also
	//   ClientSimpleHit, ClientHit, ServerHit
	virtual void ServerSimpleHit(const SimpleHitInfo &simpleHitInfo) = 0;

	// Summary
	//   Performs client tasks needed for an hit
	// Parameters
	//   hitInfo - structure holding all the information about the hit
	// See Also
	//   ServerHit, ClientSimpleHit, ServerSimpleHit
  virtual void ClientHit(const HitInfo &hitInfo) = 0;

	// Summary
	//   Performs server tasks needed for an hit
	// Parameters
	//   hitInfo - structure holding all the information about the hit
	// See Also
	//   ClientHit, ClientSimpleHit, ServerSimpleHit
  virtual void ServerHit(const HitInfo &hitInfo) = 0;
  
	// Summary
	//   Returns the Id of an HitType from its name
	// Parameters
	//   type - name of the HitType
	// Returns
	//   Id of the HitType
	// See Also
	//   HitInfo, GetHitType
  virtual int GetHitTypeId(const char *type) const = 0;

	// Summary
	//   Returns the name of an HitType from its id
	// Parameters
	//   id - Id of the HitType
	// Returns
	//   Name of the HitType
	// See Also
	//   HitInfo, GetHitTypeId
  virtual const char *GetHitType(int id) const = 0;

	// Summary
	//   Notifies that a vehicle got destroyed
	virtual void OnVehicleDestroyed(EntityId id) = 0;

	// Summary
	//   Prepares an entity to be allowed to respawn
	// Parameters
	//   entityId - Id of the entity which needs to respawn
	// See Also
	//   HasEntityRespawnData, ScheduleEntityRespawn, AbortEntityRespawn
	virtual void CreateEntityRespawnData(EntityId entityId) = 0;

	// Summary
	//   Indicates if an entity has respawn data
	// Description
	//   Respawn data can be created used the function CreateEntityRespawnData.
	// Parameters
	//   entityId - Id of the entity which needs to respawn
	// See Also
	//   CreateEntityRespawnData, ScheduleEntityRespawn, AbortEntityRespawn 
	virtual bool HasEntityRespawnData(EntityId entityId) const = 0;

	// Summary
	//   Schedules an entity to respawn
	// Parameters
	//   entityId - Id of the entity which needs to respawn
	//   unique - if set to true, this will make sure the entity isn't spawn a 
	//            second time in case it currently exist
	//   timer - time in second until the entity is respawned
	// See Also
	//   CreateEntityRespawnData, HasEntityRespawnData, AbortEntityRespawn
	virtual void ScheduleEntityRespawn(EntityId entityId, bool unique, float timer) = 0;

	// Summary
	//   Aborts a scheduled respawn of an entity
	// Parameters
	//   entityId - Id of the entity which needed to respawn
	//   destroyData - will force the respawn data set by CreateEntityRespawnData 
	//                 to be removed
	// See Also
	//   CreateEntityRespawnData, CreateEntityRespawnData, ScheduleEntityRespawn
	virtual void AbortEntityRespawn(EntityId entityId, bool destroyData) = 0;

	// Summary
	//   Schedules an entity to be removed from the level
	// Parameters
	//   entityId - EntityId of the entity to be removed
	//   timer - time in seconds until which the entity should be removed
	//   visibility - performs a visibility check before removing the entity
	// Remarks
	//   If the visibility check has been requested, the timer will be restarted 
	//   in case the entity is visible at the time the entity should have been 
	//   removed.
	// See Also
	//   AbortEntityRemoval
	virtual void ScheduleEntityRemoval(EntityId entityId, float timer, bool visibility) = 0;

	// Summary
	//   Cancel the scheduled removal of an entity
	// Parameters
	//   entityId - EntityId of the entity which was scheduled to be removed
	virtual void AbortEntityRemoval(EntityId entityId) = 0;

	// Summary
	//   Registers a listener which will be called every time for every hit.
	// Parameters
	//   pHitListener - a pointer to the hit listener
	// See Also
	//   RemoveHitListener, IHitListener
	virtual void AddHitListener(IHitListener* pHitListener) = 0;

	// Summary
	//   Removes a registered hit listener
	// Parameters
	//   pHitListener - a pointer to the hit listener
	// See Also
	//   AddHitListener
	virtual void RemoveHitListener(IHitListener* pHitListener) = 0;

  // Summary
  //   Checks whether entity is frozen.
  // Parameters
  //  entityId - EntityId of the entity to be checked
  virtual bool IsFrozen(EntityId entityId) const = 0;
};

// Summary:
//   Vehicle System interface
// Description:
//   Interface used to implement the vehicle system. 
struct IGameRulesSystem
{
	// Summary
	//   Registers a new GameRules
	// Parameters
	//   pRulesName - The name of the GameRules, which should also be the name of the GameRules script
	//   pExtensionName - The name of the IGameRules implementation which should be used by the GameRules
	// Returns
	//   The value true will be returned if the GameRules could have been registered.
	virtual bool RegisterGameRules( const char *pRulesName, const char *pExtensionName ) = 0;

	// Summary
	//   Creates a new instance for a GameRules
	// Description
	//   The EntitySystem will be requested to spawn a new entity representing the GameRules. 
	// Parameters
	//   pRulesName - Name of the GameRules
	// Returns
	//   If it succeeds, true will be returned.
	virtual bool CreateGameRules( const char *pRulesName ) = 0;
	
	// Summary
	//   Removes the currently active game from memory
	// Descriptions
	//   This function will request the EntitySystem to release the GameRules 
	//   entity. Additionally, the global g_gameRules script table will be freed.
	// Returns
	//   The value true will be returned upon completion.
	virtual bool DestroyGameRules() = 0;

	// Summary
	//   Determines if the specified GameRules has been registered
	// Parameters
	//   pRulesName - Name of the GameRules
	virtual bool HaveGameRules( const char *pRulesName ) = 0;

	// Summary
	//   Sets one GameRules instance as the one which should be currently be used
	// Parameters
	//   pGameRules - a pointer to a GameRules instance
	// Remarks
	//   Be warned that this function won't set the script GameRules table. The 
	//   CreateGameRules will have to be called to do this.
	virtual void SetCurrentGameRules(IGameRules *pGameRules) = 0;
	
	// Summary
	//   Gets the currently used GameRules
	// Returns
	//   A pointer to the GameRules instance which is currently being used.
	virtual IGameRules *GetCurrentGameRules() const = 0;
	
	ILINE IEntity *GetCurrentGameRulesEntity()
	{
		IGameRules *pGameRules = GetCurrentGameRules();
		
		if (pGameRules)
			return pGameRules->GetEntity();

		return 0;
	}
};


#endif
