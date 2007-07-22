/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 7:2:2006   15:38 : Created by M�rcio Martins

*************************************************************************/
#ifndef __GAMERULES_H__
#define __GAMERULES_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IGameObject.h>
#include <IGameRulesSystem.h>
#include "Actor.h"
#include "SynchedStorage.h"
#include <queue>
#include "Voting.h"

class CActor;
class CPlayer;

struct IGameObject;
struct IActorSystem;

class CRadio;
class CMPTutorial;


#define GAMERULES_INVOKE_ON_TEAM(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=m_playerteams.end()) \
	{ \
	const TPlayers &_players=_team->second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
	GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel, GetChannelId(*_player)); \
	} \
} \

#define GAMERULES_INVOKE_ON_TEAM_NOLOCAL(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=m_playerteams.end()) \
	{ \
	const TPlayers &_players=_team->second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
	GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel|eRMI_NoLocalCalls, GetChannelId(*_player)); \
	} \
} \


#define ACTOR_INVOKE_ON_TEAM(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=playerteams.end()) \
	{ \
	const TPlayers &_players=_team.second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
		{ \
		CActor *pActor=GetActorByEntityId(*_player); \
		if (pActor) \
		pActor->GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel, GetChannelId(*_player)); \
		} \
	} \
} \


#define ACTOR_INVOKE_ON_TEAM_NOLOCAL(team, rmi, params)	\
{ \
	TPlayerTeamIdMap::const_iterator _team=m_playerteams.find(team); \
	if (_team!=playerteams.end()) \
	{ \
	const TPlayers &_players=_team.second; \
	for (TPlayers::const_iterator _player=_players.begin();_player!=_players.end(); ++_player) \
		{ \
		CActor *pActor=GetActorByEntityId(*_player); \
		if (pActor) \
		pActor->GetGameObject()->InvokeRMI(rmi, params, eRMI_ToClientChannel|eRMI_NoLocalCalls, GetChannelId(*_player)); \
		} \
	} \
} \


class CGameRules : public CGameObjectExtensionHelper<CGameRules, IGameRules, 64>, public IActionListener
{
public:

	typedef std::vector<EntityId>								TPlayers;
	typedef std::vector<EntityId>								TSpawnLocations;
	typedef std::vector<EntityId>								TSpawnGroups;
	typedef std::map<EntityId, TSpawnLocations>	TSpawnGroupMap;
	typedef std::map<EntityId, int>							TBuildings;

	typedef struct SMinimapEntity
	{
		SMinimapEntity() {};
		SMinimapEntity(EntityId id, int typ, float time)
			: entityId(id),
			type(typ),
			lifetime(time)
		{
		}

		bool operator==(const SMinimapEntity &rhs)
		{
			return (entityId==rhs.entityId);
		}

		EntityId		entityId;
		int					type;
		float				lifetime;
	};
	typedef std::vector<SMinimapEntity>				TMinimap;

	typedef struct TObjective
	{
		TObjective(): status(0), entityId(0) {};
		TObjective(int sts, EntityId eid): status(sts), entityId(eid) {};

		int				status;
		EntityId	entityId;
	} TObjective;

	typedef std::map<string, TObjective> TObjectiveMap;
	typedef std::map<int, TObjectiveMap> TTeamObjectiveMap;

	struct SGameRulesListener
	{
		virtual void GameOver(int localWinner) = 0;
		virtual void EnteredGame() = 0;
		virtual void EndGameNear(EntityId id) = 0;
	};
	typedef std::vector<SGameRulesListener*> TGameRulesListenerVec;

	CGameRules();
	virtual ~CGameRules();
	//IGameObjectExtension
	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject );
	virtual void InitClient(int channelId);
	virtual void PostInitClient(int channelId);
	virtual void Release();
	virtual void Serialize( TSerialize ser, unsigned aspects );
	virtual void PostSerialize();
	virtual void SerializeSpawnInfo( TSerialize ser ) {}
	virtual ISerializableInfoPtr GetSpawnInfo() {return 0;}
	virtual void Update( SEntityUpdateContext& ctx, int updateSlot );
	virtual void HandleEvent( const SGameObjectEvent& );
	virtual void ProcessEvent( SEntityEvent& );
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority( bool auth );
	virtual void PostUpdate( float frameTime );
	virtual void GetMemoryStatistics(ICrySizer * s);
	//~IGameObjectExtension

	//IGameRules
	virtual bool ShouldKeepClient(int channelId, EDisconnectionCause cause, const char *desc) const;
	virtual void PrecacheLevel();
	virtual void OnConnect(struct INetChannel *pNetChannel);
	virtual void OnDisconnect(EDisconnectionCause cause, const char *desc); // notification to the client that he has been disconnected

	virtual bool OnClientConnect(int channelId);
	virtual void OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient);
	virtual bool OnClientEnteredGame(int channelId);
	
	virtual void OnItemDropped(EntityId itemId, EntityId actorId);
	virtual void OnItemPickedUp(EntityId itemId, EntityId actorId);

	virtual void SendTextMessage(ETextMessageType type, const char *msg, uint to=eRMI_ToAllClients, int channelId=-1,
		const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0);
	virtual void SendChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg);

	virtual void ForbiddenAreaWarning(bool active, int timer, EntityId targetId);

	virtual void ResetGameTime();
	virtual float GetRemainingGameTime() const;
	virtual bool IsTimeLimited() const;

	virtual void ResetRoundTime();
	virtual float GetRemainingRoundTime() const;
	virtual bool IsRoundTimeLimited() const;

	virtual void ResetPreRoundTime();
	virtual float GetRemainingPreRoundTime() const;
	//~IGameRules

	virtual void RegisterConsoleCommands(IConsole *pConsole);
	virtual void UnregisterConsoleCommands(IConsole *pConsole);
	virtual void RegisterConsoleVars(IConsole *pConsole);

	virtual void OnRevive(CActor *pActor, const Vec3 &pos, const Quat &rot, int teamId);
	virtual void OnReviveInVehicle(CActor *pActor, EntityId vehicleId, int seatId, int teamId);
	virtual void OnKill(CActor *pActor, EntityId shooterId, EntityId weaponId, int damage, int material, bool headshot);
	virtual void OnVehicleDestroyed(EntityId id);
	virtual void OnTextMessage(ETextMessageType type, const char *msg,
		const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0);
	virtual void OnChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg);
	virtual void OnKillMessage(EntityId targetId, EntityId shooterId, EntityId weaponId, float damage, int material, bool headshot);

	CActor *GetActorByChannelId(int channelId) const;
	CActor *GetActorByEntityId(EntityId entityId) const;
	ILINE const char *GetActorNameByEntityId(EntityId entityId) const
	{
		CActor *pActor=GetActorByEntityId(entityId);
		if (pActor)
			return pActor->GetEntity()->GetName();
		return 0;
	}
	ILINE const char *GetActorName(CActor *pActor) const { return pActor->GetEntity()->GetName(); };
	int GetChannelId(EntityId entityId) const;

	//------------------------------------------------------------------------
	// player
	virtual CActor *SpawnPlayer(int channelId, const char *name, const char *className, const Vec3 &pos, const Ang3 &angles);
	virtual CActor *ChangePlayerClass(int channelId, const char *className);
	virtual void RevivePlayer(CActor *pActor, const Vec3 &pos, const Ang3 &angles, int teamId=0, bool clearInventory=true);
	virtual void RevivePlayerInVehicle(CActor *pActor, EntityId vehicleId, int seatId, int teamId=0, bool clearInventory=true);
	virtual void RenamePlayer(CActor *pActor, const char *name);
	virtual string VerifyName(const char *name, IEntity *pEntity=0);
	virtual bool IsNameTaken(const char *name, IEntity *pEntity=0);
	virtual void KillPlayer(CActor *pActor, bool dropItem, bool ragdoll, EntityId shooterId, EntityId weaponId, float damage, int material, bool headshot, const Vec3 &impulse);
	virtual void MovePlayer(CActor *pActor, const Vec3 &pos, const Ang3 &angles);
	virtual void ChangeSpectatorMode(CActor *pActor, uint8 mode);
	virtual void ChangeTeam(CActor *pActor, int teamId);
	virtual void ChangeTeam(CActor *pActor, const char *teamName);
	virtual void AddTaggedEntity(EntityId shooter, EntityId targetId, bool temporary = false);
	virtual int GetPlayerCount(bool inGame=false) const;
	virtual int GetSpectatorCount(bool inGame=false) const;
	virtual EntityId GetPlayer(int idx);
	virtual void GetPlayers(TPlayers &players);
	virtual bool IsPlayerInGame(EntityId playerId) const;
	virtual bool IsChannelInGame(int channelId) const;
  virtual void StartVoting(CActor *pActor, EVotingState t, EntityId id, const char* param);
  virtual void Vote(CActor *pActor);
  virtual void EndVoting(bool success);

	//------------------------------------------------------------------------
	// teams
	virtual int CreateTeam(const char *name);
	virtual void RemoveTeam(int teamId);
	virtual const char *GetTeamName(int teamId) const;
	virtual int GetTeamId(const char *name) const;
	virtual int GetTeamCount() const;
	virtual int GetTeamPlayerCount(int teamId, bool inGame=false) const;
	virtual EntityId GetTeamPlayer(int teamId, int idx);

	virtual void GetTeamPlayers(int teamId, TPlayers &players);
	
	virtual void SetTeam(int teamId, EntityId entityId);
	virtual int GetTeam(EntityId entityId) const;

	//------------------------------------------------------------------------
	// objectives
	virtual void AddObjective(int teamId, const char *objective, int status, EntityId entityId);
	virtual void SetObjectiveStatus(int teamId, const char *objective, int status);
	virtual void SetObjectiveEntity(int teamId, const char *objective, EntityId entityId);
	virtual void RemoveObjective(int teamId, const char *objective);
	virtual void ResetObjectives();
	virtual TObjectiveMap *GetTeamObjectives(int teamId);
	virtual TObjective *GetObjective(int teamId, const char *objective);
	virtual void UpdateObjectivesForPlayer(int channelId, int teamId);

	//------------------------------------------------------------------------
	// materials
	virtual int RegisterHitMaterial(const char *materialName);
	virtual int GetHitMaterialId(const char *materialName) const;
	virtual ISurfaceType *GetHitMaterial(int id) const;
	virtual int GetHitMaterialIdFromSurfaceId(int surfaceId) const;
	virtual void ResetHitMaterials();

	//------------------------------------------------------------------------
	// hit type
	virtual int RegisterHitType(const char *type);
	virtual int GetHitTypeId(const char *type) const;
	virtual const char *GetHitType(int id) const;
	virtual void ResetHitTypes();

	//------------------------------------------------------------------------
	// freezing
	virtual bool IsFrozen(EntityId entityId) const;
	virtual void ResetFrozen();
	virtual void FreezeEntity(EntityId entityId, bool freeze, bool vapor, bool bForce=false);
	virtual void ShatterEntity(EntityId entityId, const Vec3 &pos, const Vec3 &impulse);

	//------------------------------------------------------------------------
	// spawn
	virtual void AddSpawnLocation(EntityId location);
	virtual void RemoveSpawnLocation(EntityId id);
	virtual int GetSpawnLocationCount() const;
	virtual EntityId GetSpawnLocation(int idx) const;
	virtual void GetSpawnLocations(TSpawnLocations &locations) const;
	virtual bool IsSpawnLocationSafe(EntityId playerId, EntityId spawnLocationId, float safeDistance, bool ignoreTeam) const;
	virtual EntityId GetSpawnLocation(EntityId playerId, float safeDistance, bool ignoreTeam, bool includeNeutral, EntityId groupId=0) const;
	virtual EntityId GetFirstSpawnLocation(int teamId=0, EntityId groupId=0) const;

	//------------------------------------------------------------------------
	// spawn groups
	virtual void AddSpawnGroup(EntityId groupId);
	virtual void AddSpawnLocationToSpawnGroup(EntityId groupId, EntityId location);
	virtual void RemoveSpawnLocationFromSpawnGroup(EntityId groupId, EntityId location);
	virtual void RemoveSpawnGroup(EntityId groupId);
	virtual EntityId GetSpawnLocationGroup(EntityId spawnId) const;
	virtual int GetSpawnGroupCount() const;
	virtual EntityId GetSpawnGroup(int idx) const;
	virtual void GetSpawnGroups(TSpawnLocations &groups) const;

	virtual void RequestSpawnGroup(EntityId spawnGroupId);
	virtual void SetPlayerSpawnGroup(EntityId playerId, EntityId spawnGroupId);
	virtual EntityId GetPlayerSpawnGroup(CActor *pActor);

	virtual void SetTeamDefaultSpawnGroup(int teamId, EntityId spawnGroupId);
	virtual EntityId GetTeamDefaultSpawnGroup(int teamId);
	virtual void CheckSpawnGroupValidity(EntityId spawnGroupId);

	//------------------------------------------------------------------------
	// spectator
	virtual void AddSpectatorLocation(EntityId location);
	virtual void RemoveSpectatorLocation(EntityId id);
	virtual int GetSpectatorLocationCount() const;
	virtual EntityId GetSpectatorLocation(int idx) const;
	virtual void GetSpectatorLocations(TSpawnLocations &locations) const;
	virtual EntityId GetRandomSpectatorLocation() const;
	virtual EntityId GetInterestingSpectatorLocation() const;

	//------------------------------------------------------------------------
	// map
	virtual void ResetMinimap();
	virtual void UpdateMinimap(float frameTime);
	virtual void AddMinimapEntity(EntityId entityId, int type, float lifetime=0.0f);
	virtual void RemoveMinimapEntity(EntityId entityId);
	virtual const TMinimap &GetMinimapEntities() const;

	//------------------------------------------------------------------------
	// game	
	virtual void Restart();
	virtual void NextLevel();
	virtual void ResetEntities();
	virtual void OnEndGame();
	virtual void EnteredGame();
	virtual void GameOver(int localWinner);
	virtual void EndGameNear(EntityId id);

	virtual void ClientSimpleHit(const SimpleHitInfo &simpleHitInfo);
	virtual void ServerSimpleHit(const SimpleHitInfo &simpleHitInfo);
  virtual void ClientHit(const HitInfo &hitInfo);
	virtual void ServerHit(const HitInfo &hitInfo);

	virtual void ServerExplosion(const ExplosionInfo &explosionInfo);
	virtual void ClientExplosion(const ExplosionInfo &explosionInfo);
	
	virtual void CreateEntityRespawnData(EntityId entityId);
	virtual bool HasEntityRespawnData(EntityId entityId) const;
	virtual void ScheduleEntityRespawn(EntityId entityId, bool unique, float timer);
	virtual void AbortEntityRespawn(EntityId entityId, bool destroyData);

	virtual void ScheduleEntityRemoval(EntityId entityId, float timer, bool visibility);
	virtual void AbortEntityRemoval(EntityId entityId);

	virtual void UpdateEntitySchedules(float frameTime);
  virtual void ProcessQueuedExplosions();
	virtual void ProcessServerExplosion(const ExplosionInfo &explosionInfo);
	
	virtual void ForceScoreboard(bool force);
	virtual void FreezeInput(bool freeze);

	void SendRadioMessage(const EntityId sourceId,const int);
	void OnRadioMessage(const EntityId sourceId,const int);

	virtual void OnAction(const ActionId& actionId, int activationMode, float value);

	void ReconfigureVoiceGroups(EntityId id,int old_team,int new_team);

	CMPTutorial* GetMPTutorial() const;

	int GetCurrentStateId() const { return m_currentStateId; }

	template<typename T>
	void SetSynchedGlobalValue(TSynchedKey key, const T &value)
	{
		assert(gEnv->bServer);
		g_pGame->GetSynchedStorage()->SetGlobalValue(key, value);
	};

	template<typename T>
	bool GetSynchedGlobalValue(TSynchedKey key, T &value)
	{
		if (!g_pGame->GetSynchedStorage())
			return false;
		return g_pGame->GetSynchedStorage()->GetGlobalValue(key, value);
	}

	int GetSynchedGlobalValueType(TSynchedKey key) const
	{
		if (!g_pGame->GetSynchedStorage())
			return eSVT_None;
		return g_pGame->GetSynchedStorage()->GetGlobalValueType(key);
	}

	template<typename T>
	void SetSynchedEntityValue(EntityId id, TSynchedKey key, const T &value)
	{
		assert(gEnv->bServer);
		g_pGame->GetSynchedStorage()->SetEntityValue(id, key, value);
	}
	template<typename T>
	bool GetSynchedEntityValue(EntityId id, TSynchedKey key, T &value)
	{
		return g_pGame->GetSynchedStorage()->GetEntityValue(id, key, value);
	}
	
	int GetSynchedEntityValueType(EntityId id, TSynchedKey key) const
	{
		return g_pGame->GetSynchedStorage()->GetEntityValueType(id, key);
	}

	void ResetSynchedStorage()
	{
		g_pGame->GetSynchedStorage()->Reset();
	}

	void PlayerPosForRespawn(CPlayer* pPlayer, bool save);

	struct ChatMessageParams
	{
		uint8 type;
		EntityId sourceId;
		EntityId targetId;
		string msg;

		ChatMessageParams() {};
		ChatMessageParams(EChatMessageType _type, EntityId src, EntityId trg, const char *_msg)
		: type(_type),
			sourceId(src),
			targetId(trg),
			msg(_msg)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("type", type, 'ui3');
			ser.Value("source", sourceId, 'eid');
			if (type == eChatToTarget)
				ser.Value("target", targetId, 'eid');
			ser.Value("message", msg);
		}
	};

	struct ForbiddenAreaWarningParams
	{
		int timer;
		bool active;
		ForbiddenAreaWarningParams() {};
		ForbiddenAreaWarningParams(bool act, int time) : active(act), timer(time)
		{}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("active", active, 'bool');
			ser.Value("timer", timer, 'ui3');
		}
	};

	struct BoolParam
	{
		bool success;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("success", success, 'bool');
		}
	};

	struct RadioMessageParams
	{
		EntityId			sourceId;
		uint8					msg;

		RadioMessageParams(){};
		RadioMessageParams(EntityId src,int _msg):
			sourceId(src),
			msg(_msg)
			{
			};
		void SerializeWith(TSerialize ser);
	};
	struct TextMessageParams
	{
		uint8	type;
		string msg;

		uint8 nparams;
		string params[4];

		TextMessageParams() {};
		TextMessageParams(ETextMessageType _type, const char *_msg)
		: type(_type),
			msg(_msg),
			nparams(0)
		{
		};
		TextMessageParams(ETextMessageType _type, const char *_msg, 
			const char *p0=0, const char *p1=0, const char *p2=0, const char *p3=0)
		: type(_type),
			msg(_msg),
			nparams(0)
		{
			if (!AddParam(p0)) return;
			if (!AddParam(p1)) return;
			if (!AddParam(p2)) return;
			if (!AddParam(p3)) return;
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("type", type, 'ui3');
			ser.Value("message", msg);
			ser.Value("nparams", nparams, 'ui3');

			for (int i=0;i<nparams; ++i)
				ser.Value("param", params[i]);
		}

		bool AddParam(const char *param)
		{
			if (!param || nparams>3)
				return false;
			params[nparams++]=param;
			return true;
		}
	};

	struct SetTeamParams
	{
		int				teamId;
		EntityId	entityId;

		SetTeamParams() {};
		SetTeamParams(EntityId _entityId, int _teamId)
		: entityId(_entityId),
			teamId(_teamId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("teamId", teamId, 'team');
		}
	};

	struct ChangeTeamParams
	{
		EntityId	entityId;
		int				teamId;

		ChangeTeamParams() {};
		ChangeTeamParams(EntityId _entityId, int _teamId)
			: entityId(_entityId),
				teamId(_teamId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("teamId", teamId, 'team');
		}
	};

	struct SpectatorModeParams
	{
		EntityId	entityId;
		uint8			mode;

		SpectatorModeParams() {};
		SpectatorModeParams(EntityId _entityId, uint8 _mode)
			: entityId(_entityId),
				mode(_mode)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("mode", mode, 'ui3');
		}
	};

	struct RenameEntityParams
	{
		EntityId	entityId;
		string		name;

		RenameEntityParams() {};
		RenameEntityParams(EntityId _entityId, const char *name)
			: entityId(_entityId),
				name(name)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("name", name);
		}
	};

	struct SetGameTimeParams
	{
		CTimeValue endTime;

		SetGameTimeParams() {};
		SetGameTimeParams(CTimeValue _endTime)
		: endTime(_endTime)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("endTime", endTime);
		}
	};

  struct StartVotingParams
  {
    string        param;
    EntityId      entityId;
    EVotingState  vote_type;
    StartVotingParams(){}
    StartVotingParams(EVotingState st, EntityId id, const char* cmd):vote_type(st),entityId(id),param(cmd){}
    void SerializeWith(TSerialize ser)
    {
      ser.EnumValue("type",vote_type,eVS_none,eVS_last);
      ser.Value("entityId",entityId,'eid');
      ser.Value("param",param);
    }
  };

  struct VotingStatusParams
  {
    EVotingState  state;
    int           timeout;
    EntityId      entityId;
    string        description;
    VotingStatusParams(){}
    VotingStatusParams(EVotingState s, int t, EntityId e, const char* d):state(s),timeout(t),entityId(e),description(d){}
    void SerializeWith(TSerialize ser)
    {
      ser.EnumValue("state", state, eVS_none, eVS_last);
      ser.Value("timeout", timeout, 'i7');
      ser.Value("entityId", entityId,'eid');
      ser.Value("description", description);
    }
  };

	struct AddMinimapEntityParams
	{
		EntityId entityId;
		float	lifetime;
		int	type;
		AddMinimapEntityParams() {};
		AddMinimapEntityParams(EntityId entId, float ltime, int typ)
		: entityId(entId),
			lifetime(ltime),
			type(typ)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("lifetime", lifetime, 'fsec');
			ser.Value("type", type, 'i8');
		}
	};

	struct EntityParams
	{
		EntityId entityId;
		EntityParams() {};
		EntityParams(EntityId entId)
		: entityId(entId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct NoParams
	{
		NoParams() {};
		void SerializeWith(TSerialize ser) {};
	};


	struct SpawnGroupParams
	{
		EntityId entityId;
		SpawnGroupParams() {};
		SpawnGroupParams(EntityId entId)
			: entityId(entId)
		{
		}

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct SetObjectiveParams
	{
		SetObjectiveParams(): status(0), entityId(0) {};
		SetObjectiveParams(const char *nm, int st, EntityId id): name(nm), status(st), entityId(id) {};

		EntityId entityId;
		int status;
		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
			ser.Value("status", status, 'hSts');
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct SetObjectiveStatusParams
	{
		SetObjectiveStatusParams(): status(0) {};
		SetObjectiveStatusParams(const char *nm, int st): name(nm), status(st) {};
		int status;
		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
			ser.Value("status", status, 'hSts');
		}
	};

	struct SetObjectiveEntityParams
	{
		SetObjectiveEntityParams(): entityId(0) {};
		SetObjectiveEntityParams(const char *nm, EntityId id): name(nm), entityId(id) {};

		EntityId entityId;
		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
			ser.Value("entityId", entityId, 'eid');
		}
	};

	struct RemoveObjectiveParams
	{
		RemoveObjectiveParams() {};
		RemoveObjectiveParams(const char *nm): name(nm) {};

		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
		}
	};

	struct FreezeEntityParams
	{
		FreezeEntityParams() {};
		FreezeEntityParams(EntityId id, bool doit, bool smoke): entityId(id), freeze(doit), vapor(smoke) {};
		EntityId entityId;
		bool freeze;
		bool vapor;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("freeze", freeze, 'bool');
			ser.Value("vapor", vapor, 'bool');
		}
	};

	struct ShatterEntityParams
	{
		ShatterEntityParams() {};
		ShatterEntityParams(EntityId id, const Vec3 &p, const Vec3 &imp): entityId(id), pos(p), impulse(imp) {};
		EntityId entityId;
		Vec3 pos;
		Vec3 impulse;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("pos", pos, 'sHit');
			ser.Value("impulse", impulse, 'vHPs'); // temp policy
		}
	};

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestSimpleHit, SimpleHitInfo, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestHit, HitInfo, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClExplosion, ExplosionInfo, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClFreezeEntity, FreezeEntityParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClShatterEntity, ShatterEntityParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestChatMessage, ChatMessageParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClChatMessage, ChatMessageParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestRadioMessage, RadioMessageParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRadioMessage, RadioMessageParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClTaggedEntity, EntityParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClTempRadarEntity, EntityParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClBoughtItem, BoolParam, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestRename, RenameEntityParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRenameEntity, RenameEntityParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestChangeTeam, ChangeTeamParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSpectatorMode, SpectatorModeParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetTeam, SetTeamParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClTextMessage, TextMessageParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClAddSpawnGroup, SpawnGroupParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRemoveSpawnGroup, SpawnGroupParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClAddMinimapEntity, AddMinimapEntityParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRemoveMinimapEntity, EntityParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClResetMinimap, NoParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClSetObjective, SetObjectiveParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetObjectiveStatus, SetObjectiveStatusParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetObjectiveEntity, SetObjectiveEntityParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClResetObjectives, NoParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClRequestHitIndicator, EntityParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClRequestDamageIndicator, EntityParams, eNRT_ReliableUnordered);

	DECLARE_CLIENT_RMI_NOATTACH(ClForbiddenAreaWarning, ForbiddenAreaWarningParams, eNRT_ReliableUnordered);

	// function where the client receives when the round will end
	DECLARE_CLIENT_RMI_NOATTACH(ClSetGameTime, SetGameTimeParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetRoundTime, SetGameTimeParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSetPreRoundTime, SetGameTimeParams, eNRT_ReliableUnordered);
  DECLARE_SERVER_RMI_NOATTACH(SvVote, NoParams, eNRT_ReliableUnordered);
  DECLARE_SERVER_RMI_NOATTACH(SvStartVoting, StartVotingParams, eNRT_ReliableUnordered);
  DECLARE_CLIENT_RMI_NOATTACH(ClVotingStatus, VotingStatusParams, eNRT_ReliableUnordered);

	virtual void AddHitListener(IHitListener* pHitListener);
	virtual void RemoveHitListener(IHitListener* pHitListener);

	virtual void AddGameRulesListener(SGameRulesListener* pRulesListener);
	virtual void RemoveGameRulesListener(SGameRulesListener* pRulesListener);

	typedef std::map<int, EntityId>				TTeamIdEntityIdMap;
	typedef std::map<EntityId, int>				TEntityTeamIdMap;
	typedef std::map<int, TPlayers>				TPlayerTeamIdMap;
	typedef std::map<string, int>					TTeamIdMap;

	typedef std::map<int, int>						THitMaterialMap;
	typedef std::map<int, string>					THitTypeMap;

	typedef std::map<int, _smart_ptr<IVoiceGroup> >		TTeamIdVoiceGroupMap;

	typedef struct SEntityRespawnData
	{
		SmartScriptTable	properties;
		Vec3							position;
		Quat							rotation;
		Vec3							scale;
		int								flags;
		IEntityClass			*pClass;

#ifdef _DEBUG
		string						name;
#endif
	};

	typedef struct SEntityRespawn
	{
		bool							unique;
		float							timer;
	};

	typedef struct SEntityRemovalData
	{
		float							timer;
		float							time;
		bool							visibility;
	};

	typedef std::map<EntityId, SEntityRespawnData>	TEntityRespawnDataMap;
	typedef std::map<EntityId, SEntityRespawn>			TEntityRespawnMap;
	typedef std::map<EntityId, SEntityRemovalData>	TEntityRemovalMap;

	typedef std::vector<IHitListener*> THitListenerVec;

protected:
	static void CmdDebugSpawns(IConsoleCmdArgs *pArgs);
	static void CmdDebugMinimap(IConsoleCmdArgs *pArgs);
	static void CmdDebugTeams(IConsoleCmdArgs *pArgs);
	static void CmdDebugObjectives(IConsoleCmdArgs *pArgs);

	virtual void CreateScriptHitInfo(SmartScriptTable &scriptHitInfo, const HitInfo &hitInfo);
	virtual void CreateScriptExplosionInfo(SmartScriptTable &scriptExplosionInfo, const ExplosionInfo &explosionInfo, const pe_explosion* pExplosion);

	void CallScript(IScriptTable *pScript, const char *name)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->EndCall();
	};
	template<typename P1>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2, const P3 &p3)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3, typename P4>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3); m_pScriptSystem->PushFuncParam(p4);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3, typename P4, typename P5>
	void CallScript(IScriptTable *pScript, const char *name, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3); m_pScriptSystem->PushFuncParam(p4); m_pScriptSystem->PushFuncParam(p5);
		m_pScriptSystem->EndCall();
	};
	template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	void CallScript(IScriptTable *pScript, const char *name, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6)
	{
		if (!pScript || pScript->GetValueType(name) != svtFunction)
			return;
		m_pScriptSystem->BeginCall(pScript, name); m_pScriptSystem->PushFuncParam(m_script);
		m_pScriptSystem->PushFuncParam(p1); m_pScriptSystem->PushFuncParam(p2); m_pScriptSystem->PushFuncParam(p3); m_pScriptSystem->PushFuncParam(p4); m_pScriptSystem->PushFuncParam(p5); m_pScriptSystem->PushFuncParam(p6);
		m_pScriptSystem->EndCall();
	};

	IGameFramework			*m_pGameFramework;
	IGameplayRecorder		*m_pGameplayRecorder;
	ISystem							*m_pSystem;
	IActorSystem				*m_pActorSystem;
	IEntitySystem				*m_pEntitySystem;
	IScriptSystem				*m_pScriptSystem;
	IMaterialManager		*m_pMaterialManager;
	SmartScriptTable		m_script;
	SmartScriptTable		m_clientScript;
	SmartScriptTable		m_serverScript;
	SmartScriptTable		m_clientStateScript;
	SmartScriptTable		m_serverStateScript;

	INetChannel					*m_pClientNetChannel;

	std::vector<int>		m_channelIds;
	std::vector<EntityId>m_frozen;
	
	TTeamIdMap					m_teams;
	TEntityTeamIdMap		m_entityteams;
	TTeamIdEntityIdMap	m_teamdefaultspawns;
	TPlayerTeamIdMap		m_playerteams;
	int									m_teamIdGen;

	THitMaterialMap			m_hitMaterials;
	int									m_hitMaterialIdGen;

	THitTypeMap					m_hitTypes;
	int									m_hitTypeIdGen;

	SmartScriptTable		m_scriptHitInfo;
	SmartScriptTable		m_scriptExplosionInfo;
  
  typedef std::queue<ExplosionInfo> TExplosionQueue;
  TExplosionQueue     m_queuedExplosions;

	TEntityRespawnDataMap	m_respawndata;
	TEntityRespawnMap			m_respawns;
	TEntityRemovalMap			m_removals;

	TMinimap						m_minimap;
	TTeamObjectiveMap		m_objectives;

	TSpawnLocations			m_spawnLocations;
	TSpawnGroupMap			m_spawnGroups;

	TSpawnLocations			m_spectatorLocations;

	int									m_currentStateId;

	THitListenerVec     m_hitListeners;

	CTimeValue					m_endTime;	// time the game will end. 0 for unlimited
	CTimeValue					m_roundEndTime;	// time the round will end. 0 for unlimited
	CTimeValue					m_preRoundEndTime;	// time the pre round will end. 0 for no preround

	CRadio							*m_pRadio;

	TTeamIdVoiceGroupMap	m_teamVoiceGroups;
	CMPTutorial					*m_pMPTutorial;
  CVotingSystem       *m_pVotingSystem;

	TGameRulesListenerVec	m_rulesListeners;
};

#endif //__GAMERULES_H__