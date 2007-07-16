/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
		- 7:2:2006   15:38 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_GameRules.h"
#include "GameRules.h"
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"
#include "Player.h"
#include "HUD/HUD.h"
#include "HUD/HUDRadar.h"
#include "HUD/HUDTextChat.h"
#include "Menus/FlashMenuObject.h"
#include "MusicLogic/MusicLogic.h"
	
#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "WeaponSystem.h"
#include "IUIDraw.h"

#include "ServerSynchedStorage.h"

#include "GameActions.h"
#include "Radio.h"
#include "MPTutorial.h"
#include "Voting.h"

#include <StlUtils.h>
#include <StringUtils.h>



//------------------------------------------------------------------------
CGameRules::CGameRules()
: m_pGameFramework(0),
	m_pGameplayRecorder(0),
	m_pSystem(0),
	m_pActorSystem(0),
	m_pEntitySystem(0),
	m_pScriptSystem(0),
	m_pMaterialManager(0),
	m_pClientNetChannel(0),
	m_teamIdGen(0),
	m_hitMaterialIdGen(0),
	m_hitTypeIdGen(0),
	m_currentStateId(0),
	m_endTime(0.0f),
	m_roundEndTime(0.0f),
	m_preRoundEndTime(0.0f),
	m_pRadio(0),
	m_pMPTutorial(0),
  m_pVotingSystem(0)
{
}

//------------------------------------------------------------------------
CGameRules::~CGameRules()
{
	g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
	m_pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(0);
	GetGameObject()->ReleaseActions(this);

	delete m_pRadio;

  delete m_pVotingSystem;
}

//------------------------------------------------------------------------
bool CGameRules::Init( IGameObject * pGameObject )
{
	SetGameObject(pGameObject);

	if (!GetGameObject()->BindToNetwork())
		return false;

	GetGameObject()->EnablePostUpdates(this);

	m_pGameFramework = g_pGame->GetIGameFramework();
	m_pGameplayRecorder = m_pGameFramework->GetIGameplayRecorder();
	m_pSystem = m_pGameFramework->GetISystem();
	m_pActorSystem = m_pGameFramework->GetIActorSystem();
	m_pEntitySystem = m_pSystem->GetIEntitySystem();
	m_pScriptSystem = m_pSystem->GetIScriptSystem();
	m_pMaterialManager = m_pSystem->GetI3DEngine()->GetMaterialManager();
	
	m_script = GetEntity()->GetScriptTable();
	m_script->GetValue("Client", m_clientScript);
	m_script->GetValue("Server", m_serverScript);

	m_clientStateScript = m_clientScript;
	m_serverStateScript = m_serverScript;

	m_scriptHitInfo.Create(gEnv->pScriptSystem);
	m_scriptExplosionInfo.Create(gEnv->pScriptSystem);
  SmartScriptTable affected(gEnv->pScriptSystem);
  m_scriptExplosionInfo->SetValue("AffectedEntities", affected);
  
	m_pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(this);
	g_pGame->GetGameRulesScriptBind()->AttachTo(this);

	// setup animation time scaling (until we have assets that cover the speeds we need timescaling).
	GetISystem()->GetIAnimationSystem()->SetScalingLimits( Vec2(0.5f, 3.0f) );

	bool isMultiplayer=gEnv->bMultiplayer;

	if(gEnv->bClient)
	{
		IActionMapManager *pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("multiplayer",isMultiplayer);

		bool b=GetGameObject()->CaptureActions(this);
		assert(b);

		IActionMap *am=pActionMapMan->GetActionMap("multiplayer");
		if(am)
		{
			am->SetActionListener(GetEntity()->GetId());
		}
	}

	m_pRadio=new CRadio(this);

	if(isMultiplayer && gEnv->bClient && !strcmp(GetEntity()->GetClass()->GetName(), "PowerStruggle"))
	{
		m_pMPTutorial = new CMPTutorial;
	}

  if(g_pGame->GetHUD())
    g_pGame->GetHUD()->GameRulesSet(GetEntity()->GetClass()->GetName());

  if(isMultiplayer && gEnv->bServer)
    m_pVotingSystem = new CVotingSystem;

	return true;
}

//------------------------------------------------------------------------
void CGameRules::PostInit( IGameObject * pGameObject )
{
	pGameObject->EnableUpdateSlot(this, 0);
	pGameObject->SetUpdateSlotEnableCondition(this, 0, eUEC_WithoutAI);
	pGameObject->EnablePostUpdates(this);
	
	IConsole *pConsole=m_pSystem->GetIConsole();
	RegisterConsoleCommands(pConsole);
	RegisterConsoleVars(pConsole);
}

//------------------------------------------------------------------------
void CGameRules::InitClient(int channelId)
{
}

//------------------------------------------------------------------------
void CGameRules::PostInitClient(int channelId)
{
	// update the time
	GetGameObject()->InvokeRMI(ClSetGameTime(), SetGameTimeParams(m_endTime), eRMI_ToClientChannel, channelId);
	GetGameObject()->InvokeRMI(ClSetRoundTime(), SetGameTimeParams(m_roundEndTime), eRMI_ToClientChannel, channelId);
	GetGameObject()->InvokeRMI(ClSetPreRoundTime(), SetGameTimeParams(m_preRoundEndTime), eRMI_ToClientChannel, channelId);

	// update team status on the client
	for (TEntityTeamIdMap::const_iterator tit=m_entityteams.begin(); tit!=m_entityteams.end(); ++tit)
		GetGameObject()->InvokeRMIWithDependentObject(ClSetTeam(), SetTeamParams(tit->first, tit->second), eRMI_ToClientChannel, tit->first, channelId);

	// init spawn groups
	for (TSpawnGroupMap::const_iterator sgit=m_spawnGroups.begin(); sgit!=m_spawnGroups.end(); ++sgit)
		GetGameObject()->InvokeRMIWithDependentObject(ClAddSpawnGroup(), SpawnGroupParams(sgit->first), eRMI_ToClientChannel, sgit->first, channelId);

	// update minimap entities on the client
	for (TMinimap::const_iterator mit=m_minimap.begin(); mit!=m_minimap.end(); ++mit)
		GetGameObject()->InvokeRMIWithDependentObject(ClAddMinimapEntity(), AddMinimapEntityParams(mit->entityId, mit->lifetime, mit->type), eRMI_ToClientChannel, mit->entityId, channelId);
}

//------------------------------------------------------------------------
void CGameRules::Release()
{
	UnregisterConsoleCommands(gEnv->pConsole);
	delete this;
}

//------------------------------------------------------------------------
void CGameRules::Serialize( TSerialize ser, unsigned aspects )
{
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->Serialize(ser,aspects);

	if ((ser.GetSerializationTarget() != eST_Network) && g_pGame->GetMusicLogic())
		if (CMusicLogic* pMusicLogic = g_pGame->GetMusicLogic())
			pMusicLogic->Serialize(ser, aspects);
}

//-----------------------------------------------------------------------------------------------------
void CGameRules::PostSerialize()
{
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->PostSerialize();
}

//------------------------------------------------------------------------
void CGameRules::Update( SEntityUpdateContext& ctx, int updateSlot )
{
	if (updateSlot!=0)
		return;

	//g_pGame->GetServerSynchedStorage()->SetGlobalValue(15, 1026);

	bool server=gEnv->bServer;

	if (server)
  {
    ProcessQueuedExplosions();
		UpdateEntitySchedules(ctx.fFrameTime);    
  }

	UpdateMinimap(ctx.fFrameTime);

	if(m_pMPTutorial)
		m_pMPTutorial->Update();
}

//------------------------------------------------------------------------
void CGameRules::HandleEvent( const SGameObjectEvent& event)
{
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->HandleEvent(event);
}

//------------------------------------------------------------------------
void CGameRules::ProcessEvent( SEntityEvent& event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	switch(event.event)
	{
	case ENTITY_EVENT_RESET:
		ResetFrozen();
    
    while (!m_queuedExplosions.empty())
      m_queuedExplosions.pop();
		
      // TODO: move this from here
		g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
		m_respawns.clear();
		break;

	case ENTITY_EVENT_START_GAME:
		g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
		break;

	case ENTITY_EVENT_ENTER_SCRIPT_STATE:
		m_currentStateId=event.nParam[0];

		m_clientStateScript=0;
		m_serverStateScript=0;

		IEntityScriptProxy *pScriptProxy=static_cast<IEntityScriptProxy *>(GetEntity()->GetProxy(ENTITY_PROXY_SCRIPT));
		if (pScriptProxy)
		{
			const char *stateName=pScriptProxy->GetState();

			m_clientScript->GetValue(stateName, m_clientStateScript);
			m_serverScript->GetValue(stateName, m_serverStateScript);
		}
		break;
	}
}

//------------------------------------------------------------------------
void CGameRules::SetAuthority( bool auth )
{
}

//------------------------------------------------------------------------
void CGameRules::PostUpdate( float frameTime )
{
  if(m_pVotingSystem && m_pVotingSystem->IsInProgress())
  {
    int need_votes = int(ceilf(GetPlayerCount(false)*g_pGame->GetCVars()->sv_votingRatio));
    if(need_votes && m_pVotingSystem->GetNumVotes() >= need_votes)
    {
      EndVoting(true);
    }
    if(int team = m_pVotingSystem->GetTeam())
    {
      int team_votes = int(ceilf(GetTeamPlayerCount(team,false)*g_pGame->GetCVars()->sv_votingRatio));
      if(team_votes && m_pVotingSystem->GetNumTeamVotes() >= team_votes)
      {
        EndVoting(true);
      }
    }
    if(m_pVotingSystem->GetVotingTime().GetSeconds() > g_pGame->GetCVars()->sv_votingTimeout)    
    {
      EndVoting(false);
    }
  }
}

//------------------------------------------------------------------------
CActor *CGameRules::GetActorByChannelId(int channelId) const
{
	return static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId));
}

//------------------------------------------------------------------------
CActor *CGameRules::GetActorByEntityId(EntityId entityId) const
{
	return static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActor(entityId));
}

//------------------------------------------------------------------------
int CGameRules::GetChannelId(EntityId entityId) const
{
	CActor *pActor = static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActor(entityId));
	if (pActor)
		return pActor->GetChannelId();

	return 0;
}

//------------------------------------------------------------------------
bool CGameRules::ShouldKeepClient(int channelId, EDisconnectionCause cause, const char *desc) const
{
	return (!strcmp("timeout", desc) || cause==eDC_Timeout);
}

//------------------------------------------------------------------------
void CGameRules::PrecacheLevel()
{
	CallScript(m_script, "PrecacheLevel");
}

//------------------------------------------------------------------------
void CGameRules::OnConnect(struct INetChannel *pNetChannel)
{
	m_pClientNetChannel=pNetChannel;

	CallScript(m_clientStateScript,"OnConnect");
}


//------------------------------------------------------------------------
void CGameRules::OnDisconnect(EDisconnectionCause cause, const char *desc)
{
	m_pClientNetChannel=0;
	int icause=(int)cause;
	CallScript(m_clientStateScript, "OnDisconnect", icause, desc);
}

//------------------------------------------------------------------------
bool CGameRules::OnClientConnect(int channelId)
{
	m_channelIds.push_back(channelId);

	g_pGame->GetServerSynchedStorage()->OnClientConnect(channelId);

	if (gEnv->bServer && gEnv->bMultiplayer)
	{
		const char* playerName = 0;
		INetChannel* pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId);
		if (pNetChannel)
		{
      playerName = pNetChannel->GetNickname();
		}

    if(playerName)
    {
        CallScript(m_serverStateScript, "OnClientConnect", channelId, (IScriptTable*)0, playerName);
    }
    else
    {
        CallScript(m_serverStateScript, "OnClientConnect", channelId);
    }

    if(m_pVotingSystem && m_pVotingSystem->IsInProgress())
    {
      int time_left = g_pGame->GetCVars()->sv_votingTimeout - int(m_pVotingSystem->GetVotingTime().GetSeconds());
      VotingStatusParams params(m_pVotingSystem->GetType(),time_left,m_pVotingSystem->GetEntityId(),m_pVotingSystem->GetSubject());
      GetGameObject()->InvokeRMIWithDependentObject(ClVotingStatus(),params,eRMI_ToClientChannel,GetActorByChannelId(channelId)->GetEntityId());
    }
	}
	else
	{
		CallScript(m_serverStateScript, "OnClientConnect", channelId);
	}

	IActor *pActor=GetActorByChannelId(channelId);
	if (pActor)
		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Connected));
	return pActor != 0;
}

//------------------------------------------------------------------------
void CGameRules::OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient)
{
	CActor *pActor=GetActorByChannelId(channelId);
	//assert(pActor);

	if (!pActor || !keepClient)
		if (g_pGame->GetServerSynchedStorage())
			g_pGame->GetServerSynchedStorage()->OnClientDisconnect(channelId, false);

	if (!pActor)
		return;

	if (pActor)
		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Disconnected));

	if (keepClient)
	{
		if (g_pGame->GetServerSynchedStorage())
			g_pGame->GetServerSynchedStorage()->OnClientDisconnect(channelId, true);

		pActor->GetGameObject()->SetPhysicalizationProfile(eAP_NotPhysicalized);

		return;
	}

	IVehicle *pVehicle=pActor->GetLinkedVehicle();
	if (pVehicle)
	{
		IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(pActor->GetEntityId());
		if (pSeat)
			pSeat->Reset();
	}

  SetTeam(0, pActor->GetEntityId());

	m_channelIds.erase(std::find(m_channelIds.begin(), m_channelIds.end(), channelId));

	CallScript(m_serverStateScript, "OnClientDisconnect", channelId);

	return;
}

//------------------------------------------------------------------------
bool CGameRules::OnClientEnteredGame(int channelId)
{ 
	CActor *pActor=GetActorByChannelId(channelId);
	if (!pActor)
		return false;

	IScriptTable *pPlayer=pActor->GetEntity()->GetScriptTable();
	int loadingSaveGame=m_pGameFramework->IsLoadingSaveGame()?1:0;
	CallScript(m_serverStateScript, "OnClientEnteredGame", channelId, pPlayer, loadingSaveGame);

	ReconfigureVoiceGroups(pActor->GetEntityId(), -999, 0); /* -999 should never exist :) */

	return true;
}

//------------------------------------------------------------------------
void CGameRules::OnItemDropped(EntityId itemId, EntityId actorId)
{
	ScriptHandle itemIdHandle(itemId);
	ScriptHandle actorIdHandle(actorId);
	CallScript(m_serverStateScript, "OnItemDropped", itemIdHandle, actorIdHandle);
}

//------------------------------------------------------------------------
void CGameRules::OnItemPickedUp(EntityId itemId, EntityId actorId)
{
	ScriptHandle itemIdHandle(itemId);
	ScriptHandle actorIdHandle(actorId);
	CallScript(m_serverStateScript, "OnItemPickedUp", itemIdHandle, actorIdHandle);
}

//------------------------------------------------------------------------
void CGameRules::OnTextMessage(ETextMessageType type, const char *msg,
															 const char *p0, const char *p1, const char *p2, const char *p3)
{
	if(!g_pGame->GetHUD())
		return;

	switch(type)
	{
	case eTextMessageConsole:
		CryLogAlways("%s", msg);
		break;
	case eTextMessageServer:
		{
			string completeMsg("** Server: ");
			completeMsg.append(msg);
			completeMsg.append(" **");

			g_pGame->GetHUD()->DisplayFlashMessage(completeMsg.c_str(), 3, ColorF(1,1,1), p0!=0, p0, p1, p2, p3);

			CryLogAlways("[server] %s", msg);
		}
		break;
	case eTextMessageError:
			g_pGame->GetHUD()->DisplayFlashMessage(msg, 1, ColorF(0.85f,0,0), p0!=0, p0, p1, p2, p3);
			break;
	case eTextMessageInfo:
		g_pGame->GetHUD()->DisplayFlashMessage(msg, 1, ColorF(1,1,1), p0!=0, p0, p1, p2, p3);
		break;
	case eTextMessageCenter:
		g_pGame->GetHUD()->DisplayFlashMessage(msg, 2, ColorF(1,1,1), p0!=0, p0, p1, p2, p3);
		break;
	}
}

//------------------------------------------------------------------------
void CGameRules::OnChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg)
{
	//send chat message to hud
	if(g_pGame->GetHUD())
	{
		int teamFaction = 0;
		if(IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor())
		{
			if(pActor->GetEntityId() != sourceId)
			{
				if(GetTeamCount() > 1)
				{
					if(GetTeam(pActor->GetEntityId()) == GetTeam(sourceId))
						teamFaction = 1;
					else
						teamFaction = 2;
				}
				else
					teamFaction = 2;
			}
		}

		if(CHUDTextChat *pChat = g_pGame->GetHUD()->GetMPChat())
			pChat->AddChatMessage(sourceId, msg, teamFaction);
	}
}

//------------------------------------------------------------------------
void CGameRules::OnRevive(CActor *pActor, const Vec3 &pos, const Quat &rot, int teamId)
{
	ScriptHandle handle(pActor->GetEntityId());
	Vec3 rotVec = Ang3(rot);
	CallScript(m_clientScript, "OnRevive", handle, pos, rotVec, teamId);
}

//------------------------------------------------------------------------
void CGameRules::OnKill(CActor *pActor, EntityId shooterId, EntityId weaponId, int damage, int material, bool headshot)
{
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->ActorDeath(pActor);

	ScriptHandle handleEntity(pActor->GetEntityId()), handleShooter(shooterId), handleWeapon(weaponId);
	CallScript(m_clientStateScript, "OnKill", handleEntity, handleShooter, handleWeapon, damage, material, headshot);
}

//------------------------------------------------------------------------
void CGameRules::OnReviveInVehicle(CActor *pActor, EntityId vehicleId, int seatId, int teamId)
{
	ScriptHandle handle(pActor->GetEntityId());
	ScriptHandle vhandle(pActor->GetEntityId());
	if(g_pGame->GetHUD())
	{
		SGameObjectEvent evt(eCGE_ActorRevive,eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void*)pActor);
		g_pGame->GetHUD()->HandleEvent(evt);
	}
	CallScript(m_clientScript, "OnReviveInVehicle", handle, vhandle, seatId, teamId);
}

//------------------------------------------------------------------------

void CGameRules::OnVehicleDestroyed(EntityId id)
{
	RemoveMinimapEntity(id);

	if(g_pGame->GetHUD())
	{
		g_pGame->GetHUD()->VehicleDestroyed(id);
	}
}

//------------------------------------------------------------------------
void CGameRules::AddTaggedEntity(EntityId shooter, EntityId targetId, bool temporary)
{
	if(!gEnv->bServer) // server sends to all clients
		return;

	EntityParams params(targetId);
	if(GetTeamCount() > 1)
	{
		EntityId teamId = GetTeam(shooter);
		TPlayerTeamIdMap::const_iterator tit=m_playerteams.find(teamId);
		if (tit!=m_playerteams.end())
		{
			for (TPlayers::const_iterator it=tit->second.begin(); it!=tit->second.end(); ++it)
			{
				if(temporary)
					GetGameObject()->InvokeRMI(ClTempRadarEntity(), params, eRMI_ToClientChannel, GetChannelId(*it));
				else
					GetGameObject()->InvokeRMI(ClTaggedEntity(), params, eRMI_ToClientChannel, GetChannelId(*it));
			}
		}
	}
	else
	{
		if(temporary)
			GetGameObject()->InvokeRMI(ClTempRadarEntity(), params, eRMI_ToClientChannel, GetChannelId(shooter));
		else
			GetGameObject()->InvokeRMI(ClTaggedEntity(), params, eRMI_ToClientChannel, GetChannelId(shooter));
	}
}

//------------------------------------------------------------------------
void CGameRules::OnKillMessage(EntityId targetId, EntityId shooterId, EntityId weaponId, float damage, int material, bool headshot)
{
	if(gEnv->bClient && g_pGame->GetIGameFramework()->GetClientActor() && g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId()==targetId)
		m_pRadio->CancelRadio();

	if (g_pGame->GetHUD())
		g_pGame->GetHUD()->ObituaryMessage(targetId, shooterId, weaponId, headshot);
}

//------------------------------------------------------------------------
CActor *CGameRules::SpawnPlayer(int channelId, const char *name, const char *className, const Vec3 &pos, const Ang3 &angles)
{
	if (!gEnv->bServer)
		return 0;

	CActor *pActor=GetActorByChannelId(channelId);
	if (!pActor)
		pActor = static_cast<CActor *>(m_pActorSystem->CreateActor(channelId, VerifyName(name).c_str(), className, pos, Quat(angles), Vec3(1, 1, 1)));

	return pActor;
}

//------------------------------------------------------------------------
CActor *CGameRules::ChangePlayerClass(int channelId, const char *className)
{
	if (!gEnv->bServer)
		return 0;

	CActor *pOldActor = GetActorByChannelId(channelId);
	if (!pOldActor)
		return 0;

	if (!strcmp(pOldActor->GetEntity()->GetClass()->GetName(), className))
		return pOldActor;

	EntityId oldEntityId = pOldActor->GetEntityId();
	string oldName = pOldActor->GetEntity()->GetName();
	Ang3 oldAngles=pOldActor->GetAngles();
	Vec3 oldPos = pOldActor->GetEntity()->GetWorldPos();

	m_pEntitySystem->RemoveEntity(pOldActor->GetEntityId(), true);

	CActor *pActor = static_cast<CActor *>(m_pActorSystem->CreateActor(channelId, oldName.c_str(), className, oldPos, Quat::CreateRotationXYZ(oldAngles), Vec3(1, 1, 1), oldEntityId));
	if (pActor)
		MovePlayer(pActor, oldPos, oldAngles);

	return pActor;
}

//------------------------------------------------------------------------
void CGameRules::RevivePlayer(CActor *pActor, const Vec3 &pos, const Ang3 &angles, int teamId, bool clearInventory)
{
	if (IVehicle *pVehicle=pActor->GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(pActor->GetEntityId()))
			pSeat->Exit(false);
	}

	if (IsFrozen(pActor->GetEntityId()))
		FreezeEntity(pActor->GetEntityId(), false, false);

	pActor->SetHealth(100);
	pActor->SetMaxHealth(100);

	if (!m_pGameFramework->IsChannelOnHold(pActor->GetChannelId()))
		pActor->GetGameObject()->SetPhysicalizationProfile(eAP_Alive);

	Matrix34 tm(pActor->GetEntity()->GetWorldTM());
	tm.SetTranslation(pos);

	pActor->GetEntity()->SetWorldTM(tm);
	pActor->SetAngles(angles);

	if (clearInventory)
	{
		IInventory *pInventory=pActor->GetInventory();
		pInventory->Destroy();
	}

	pActor->NetReviveAt(pos, Quat(angles), clearInventory, teamId);

	pActor->GetGameObject()->InvokeRMI(CActor::ClRevive(), CActor::ReviveParams(pos, angles, teamId, clearInventory), 
		eRMI_ToAllClients|eRMI_NoLocalCalls);

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Revive));
}

//------------------------------------------------------------------------
void CGameRules::RevivePlayerInVehicle(CActor *pActor, EntityId vehicleId, int seatId, int teamId/* =0 */, bool clearInventory/* =true */)
{
	if (IVehicle *pVehicle=pActor->GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(pActor->GetEntityId()))
			pSeat->Exit(false); 
	}

	pActor->SetHealth(100);
	pActor->SetMaxHealth(100);

	if (!m_pGameFramework->IsChannelOnHold(pActor->GetChannelId()))
		pActor->GetGameObject()->SetPhysicalizationProfile(eAP_Alive);

	if (clearInventory)
	{
		IInventory *pInventory=pActor->GetInventory();
		pInventory->Destroy();
	}

	pActor->NetReviveInVehicle(vehicleId, seatId, clearInventory, teamId);

	pActor->GetGameObject()->InvokeRMI(CActor::ClReviveInVehicle(), 
		CActor::ReviveInVehicleParams(vehicleId, seatId, teamId, clearInventory), eRMI_ToAllClients|eRMI_NoLocalCalls);

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Revive));
}

//------------------------------------------------------------------------
void CGameRules::RenamePlayer(CActor *pActor, const char *name)
{
	string fixed=VerifyName(name, pActor->GetEntity());
	RenameEntityParams params(pActor->GetEntityId(), fixed.c_str());
	if (!stricmp(fixed.c_str(), pActor->GetEntity()->GetName()))
		return;

	if (gEnv->bServer)
	{
		if (!gEnv->bClient)
			pActor->GetEntity()->SetName(fixed.c_str());

		GetGameObject()->InvokeRMIWithDependentObject(ClRenameEntity(), params, eRMI_ToAllClients, params.entityId);

		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Renamed, fixed));
	}
	else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
		GetGameObject()->InvokeRMIWithDependentObject(SvRequestRename(), params, eRMI_ToServer, params.entityId);
}

//------------------------------------------------------------------------
string CGameRules::VerifyName(const char *name, IEntity *pEntity)
{
	string nameFormatter(name);

	// size limit is 26
	if (nameFormatter.size()>26)
		nameFormatter.resize(26);

	// no spaces at start/end
	nameFormatter.TrimLeft(' ');
	nameFormatter.TrimRight(' ');

	// no empty names
	if (nameFormatter.empty())
		nameFormatter="empty";

	// no @ signs
	nameFormatter.replace("@", "_");

	// search for duplicates
	if (IsNameTaken(nameFormatter.c_str(), pEntity))
	{
		int n=1;
		string appendix;
		do 
		{
			appendix.Format("(%d)", n++);
		} while(IsNameTaken(nameFormatter+appendix));

		nameFormatter.append(appendix);
	}

	return nameFormatter;
}

//------------------------------------------------------------------------
bool CGameRules::IsNameTaken(const char *name, IEntity *pEntity)
{
	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		CActor *pActor=GetActorByChannelId(*it);
		if (pActor && pActor->GetEntity()!=pEntity && !stricmp(name, pActor->GetEntity()->GetName()))
			return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CGameRules::KillPlayer(CActor *pActor, bool dropItem, bool ragdoll, EntityId shooterId, EntityId weaponId, float damage, int material, bool headshot, const Vec3 &impulse)
{
	if(!gEnv->bServer)
		return;

	IInventory *pInventory=pActor->GetInventory();
	EntityId itemId=pInventory?pInventory->GetCurrentItem():0;
	if (itemId && !pActor->GetLinkedVehicle())
	{
		CItem *pItem=pActor->GetItem(itemId);
		if (pItem && pItem->IsMounted() && pItem->IsUsed())
			pItem->StopUse(pActor->GetEntityId());
		else if (pItem && dropItem)
			pActor->DropItem(itemId, 1.0f, false, true);
	}

	pActor->GetInventory()->Destroy();

	if (ragdoll)
		pActor->GetGameObject()->SetPhysicalizationProfile(eAP_Ragdoll);

	pActor->NetKill(shooterId, weaponId, (int)damage, material, headshot);

	pActor->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClKill(),
		CActor::KillParams(shooterId, weaponId, damage, material, headshot, impulse),
		eRMI_ToAllClients|eRMI_NoLocalCalls, weaponId);

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Death));
	if (shooterId && shooterId!=pActor->GetEntityId())
		if (IActor *pShooter=m_pGameFramework->GetIActorSystem()->GetActor(shooterId))
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_Kill, 0, 0, (void *)weaponId));
}

//------------------------------------------------------------------------
void CGameRules::MovePlayer(CActor *pActor, const Vec3 &pos, const Ang3 &angles)
{
	CActor::MoveParams params(pos, Quat(angles));
	pActor->GetGameObject()->InvokeRMI(CActor::ClMoveTo(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, pActor->GetChannelId());
	pActor->GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1,1,1), params.rot, params.pos));
}

//------------------------------------------------------------------------
void CGameRules::ChangeSpectatorMode(CActor *pActor, uint8 mode)
{
	if (pActor->GetSpectatorMode()==mode)
		return;

	SpectatorModeParams params(pActor->GetEntityId(), mode);

	if (gEnv->bServer)
	{
		ScriptHandle handle(params.entityId);
		CallScript(m_serverStateScript, "OnChangeSpectatorMode", handle, mode);
    m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Spectator, 0, (float)mode));
	}
	else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
		GetGameObject()->InvokeRMIWithDependentObject(SvRequestSpectatorMode(), params, eRMI_ToServer, params.entityId);
}

//------------------------------------------------------------------------
void CGameRules::ChangeTeam(CActor *pActor, int teamId)
{
	if (teamId == GetTeam(pActor->GetEntityId()))
		return;

	ChangeTeamParams params(pActor->GetEntityId(), teamId);

	if (gEnv->bServer)
	{
		ScriptHandle handle(params.entityId);
		CallScript(m_serverStateScript, "OnChangeTeam", handle, params.teamId);
	}
	else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
		GetGameObject()->InvokeRMIWithDependentObject(SvRequestChangeTeam(), params, eRMI_ToServer, params.entityId);
}

//------------------------------------------------------------------------
void CGameRules::ChangeTeam(CActor *pActor, const char *teamName)
{
	if (!teamName)
		return;

	int teamId=GetTeamId(teamName);

	if (!teamId)
	{
		CryLogAlways("Invalid team: %s", teamName);
		return;
	}

	ChangeTeam(pActor, teamId);
}

//------------------------------------------------------------------------
int CGameRules::GetPlayerCount(bool inGame) const
{
	if (!inGame)
		return (int)m_channelIds.size();

	int count=0;
	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		if (IsChannelInGame(*it))
			++count;
	}

	return count;
}

//------------------------------------------------------------------------
int CGameRules::GetSpectatorCount(bool inGame) const
{
	int count=0;
	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		CActor *pActor=GetActorByChannelId(*it);
		if (pActor->GetSpectatorMode()!=0)
		{
			if (!inGame || IsChannelInGame(*it))
				++count;
		}
	}

	return count;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetPlayer(int idx)
{
	if (idx<0||idx>=m_channelIds.size())
		return 0;

	CActor *pActor=GetActorByChannelId(m_channelIds[idx]);
	return pActor?pActor->GetEntityId():0;
}

//------------------------------------------------------------------------
void CGameRules::GetPlayers(TPlayers &players)
{
	players.resize(0);
	players.resize(m_channelIds.size());

	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		CActor *pActor=GetActorByChannelId(*it);
		players.push_back(pActor?pActor->GetEntityId():0);
	}
}

//------------------------------------------------------------------------
bool CGameRules::IsPlayerInGame(EntityId playerId) const
{
	INetChannel *pNetChannel=g_pGame->GetIGameFramework()->GetNetChannel(GetChannelId(playerId));
	if (pNetChannel && pNetChannel->GetContextViewState()>=eCVS_InGame)
		return true;
	return false;
}

//------------------------------------------------------------------------
bool CGameRules::IsChannelInGame(int channelId) const
{
	INetChannel *pNetChannel=g_pGame->GetIGameFramework()->GetNetChannel(channelId);
	if (pNetChannel && pNetChannel->GetContextViewState()>=eCVS_InGame)
		return true;
	return false;
}

//------------------------------------------------------------------------
void CGameRules::StartVoting(CActor *pActor, EVotingState t, EntityId id, const char* param)
{
  if(!pActor)
    return;

  StartVotingParams params(t,id,param);
  EntityId entityId = pActor->GetEntityId();
  
  if (gEnv->bServer)
  {
    if(!m_pVotingSystem)
      return;
    CTimeValue st;
    CTimeValue curr_time = gEnv->pTimer->GetFrameStartTime();

    if(!m_pVotingSystem->GetCooldownTime(entityId,st) || (curr_time-st).GetSeconds()>g_pGame->GetCVars()->sv_votingCooldown)
    {
      if(m_pVotingSystem->StartVoting(entityId,curr_time,t,id,param,GetTeam(id)))
      {
        m_pVotingSystem->Vote(entityId,GetTeam(entityId));
        VotingStatusParams st_param(t,g_pGame->GetCVars()->sv_votingTimeout,id,param);
        GetGameObject()->InvokeRMI(ClVotingStatus(), st_param, eRMI_ToAllClients);
      }
    }
    else
    {
      CryLog("Player %s cannot start voting yet",pActor->GetEntity()->GetName());
    }
  }
  else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
    GetGameObject()->InvokeRMIWithDependentObject(SvStartVoting(), params, eRMI_ToServer, entityId);
}

//------------------------------------------------------------------------
void CGameRules::Vote(CActor* pActor)
{
  if(!pActor)
    return;
  EntityId id = pActor->GetEntityId();

  if (gEnv->bServer)
  {
    if(!m_pVotingSystem)
      return;
    if(m_pVotingSystem->CanVote(id) && m_pVotingSystem->IsInProgress())
    {
      m_pVotingSystem->Vote(id,GetTeam(id));
    }
    else
    {
      CryLog("Player %s cannot vote",pActor->GetEntity()->GetName());
    }
  }
  else if (id == m_pGameFramework->GetClientActor()->GetEntityId())
    GetGameObject()->InvokeRMIWithDependentObject(SvVote(), NoParams(), eRMI_ToServer, id);
}

//------------------------------------------------------------------------
void CGameRules::EndVoting(bool success)
{
  if(!m_pVotingSystem || !gEnv->bServer)
    return;

  if(success)
  {
    CryLog("Voting \'%s\' succeeded.",m_pVotingSystem->GetSubject().c_str());
    switch(m_pVotingSystem->GetType())
    {
    case eVS_consoleCmd:
      gEnv->pConsole->ExecuteString(m_pVotingSystem->GetSubject());
      break;
    case eVS_kick:
      {
        int ch_id = GetChannelId(m_pVotingSystem->GetEntityId());
        if(INetChannel* pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(ch_id))
          pNetChannel->Disconnect(eDC_Kicked,"Kicked from server by voting");
      }
      break;
    case eVS_nextMap:
      NextLevel();
      break;
    case eVS_changeMap:
      m_pGameFramework->ExecuteCommandNextFrame(string("map ")+m_pVotingSystem->GetSubject());
      break;
    case eVS_none:
      break;
    }
  }
  else
    CryLog("Voting \'%s\' ended.",m_pVotingSystem->GetSubject().c_str());

  m_pVotingSystem->EndVoting();
  VotingStatusParams params(eVS_none, 0, GetEntityId(), "");
  GetGameObject()->InvokeRMI(ClVotingStatus(), params, eRMI_ToAllClients);
}

//------------------------------------------------------------------------
int CGameRules::CreateTeam(const char *name)
{
	TTeamIdMap::iterator it = m_teams.find(CONST_TEMP_STRING(name));
	if (it != m_teams.end())
		return it->second;

	m_teams.insert(TTeamIdMap::value_type(name, ++m_teamIdGen));
	m_playerteams.insert(TPlayerTeamIdMap::value_type(m_teamIdGen, TPlayers()));

	return m_teamIdGen;
}

//------------------------------------------------------------------------
void CGameRules::RemoveTeam(int teamId)
{
	TTeamIdMap::iterator it = m_teams.find(CONST_TEMP_STRING(GetTeamName(teamId)));
	if (it == m_teams.end())
		return;

	m_teams.erase(it);

	for (TEntityTeamIdMap::iterator eit=m_entityteams.begin(); eit != m_entityteams.end(); ++eit)
	{
		if (eit->second == teamId)
			eit->second = 0; // 0 is no team
	}

	m_playerteams.erase(m_playerteams.find(teamId));
}

//------------------------------------------------------------------------
const char *CGameRules::GetTeamName(int teamId) const
{
	for (TTeamIdMap::const_iterator it = m_teams.begin(); it!=m_teams.end(); ++it)
	{
		if (teamId == it->second)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetTeamId(const char *name) const
{
	TTeamIdMap::const_iterator it=m_teams.find(CONST_TEMP_STRING(name));
	if (it!=m_teams.end())
		return it->second;

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetTeamCount() const
{
	return (int)m_teams.size();
}

//------------------------------------------------------------------------
int CGameRules::GetTeamPlayerCount(int teamId, bool inGame) const
{
	if (!inGame)
	{
		TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
		if (it!=m_playerteams.end())
			return (int)it->second.size();
		return 0;
	}
	else
	{
		TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
		if (it!=m_playerteams.end())
		{
			int count=0;

			const TPlayers &players=it->second;
			for (TPlayers::const_iterator pit=players.begin(); pit!=players.end(); ++pit)
				if (IsPlayerInGame(*pit))
					++count;

			return count;
		}
		return 0;
	}
}

//------------------------------------------------------------------------
EntityId CGameRules::GetTeamPlayer(int teamId, int idx)
{
	TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
	if (it!=m_playerteams.end())
	{
		if (idx>=0 && idx<it->second.size())
			return it->second[idx];
	}

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetTeamPlayers(int teamId, TPlayers &players)
{
	players.resize(0);
	TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
	if (it!=m_playerteams.end())
		players=it->second;
}

//------------------------------------------------------------------------
void CGameRules::SetTeam(int teamId, EntityId id)
{
	if (!gEnv->bServer )
	{
		assert(0);
		return;
	}

	int oldTeam = GetTeam(id);
	if (oldTeam==teamId)
		return;

	TEntityTeamIdMap::iterator it=m_entityteams.find(id);
	if (it!=m_entityteams.end())
		m_entityteams.erase(it);

	bool isplayer=m_pActorSystem->GetActor(id)!=0;
	if (isplayer && oldTeam)
	{
		TPlayerTeamIdMap::iterator pit=m_playerteams.find(oldTeam);
		assert(pit!=m_playerteams.end());
		stl::find_and_erase(pit->second, id);
	}
	if (teamId)
	{
		m_entityteams.insert(TEntityTeamIdMap::value_type(id, teamId));

		if (isplayer)
		{
			TPlayerTeamIdMap::iterator pit=m_playerteams.find(teamId);
			assert(pit!=m_playerteams.end());
			pit->second.push_back(id);

			UpdateObjectivesForPlayer(GetChannelId(id), teamId);
		}
	}

	if(isplayer)
		ReconfigureVoiceGroups(id,oldTeam,teamId);

	if (gEnv->bClient)
		m_pRadio->SetTeam(GetTeamName(teamId));

	ScriptHandle handle(id);
	CallScript(m_serverStateScript, "OnSetTeam", handle, teamId);

	if (gEnv->bClient)
	{
		ScriptHandle handle(id);
		CallScript(m_clientStateScript, "OnSetTeam", handle, teamId);
	}
	
	// if this is a spawn group, update it's validity
	if (m_spawnGroups.find(id)!=m_spawnGroups.end())
		CheckSpawnGroupValidity(id);

	GetGameObject()->InvokeRMIWithDependentObject(ClSetTeam(), SetTeamParams(id, teamId), eRMI_ToRemoteClients, id);

	if (IEntity *pEntity=m_pEntitySystem->GetEntity(id))
		m_pGameplayRecorder->Event(pEntity, GameplayEvent(eGE_ChangedTeam, 0, (float)teamId));
}

//------------------------------------------------------------------------
int CGameRules::GetTeam(EntityId entityId) const
{
	TEntityTeamIdMap::const_iterator it = m_entityteams.find(entityId);
	if (it != m_entityteams.end())
		return it->second;

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::AddObjective(int teamId, const char *objective, int status, EntityId entityId)
{
	TObjectiveMap *pObjectives=GetTeamObjectives(teamId);
	if (!pObjectives)
		m_objectives.insert(TTeamObjectiveMap::value_type(teamId, TObjectiveMap()));

	if (pObjectives=GetTeamObjectives(teamId))
	{
		if (pObjectives->find(CONST_TEMP_STRING(objective))==pObjectives->end())
			pObjectives->insert(TObjectiveMap::value_type(objective, TObjective(status, entityId)));
	}
}

//------------------------------------------------------------------------
void CGameRules::SetObjectiveStatus(int teamId, const char *objective, int status)
{
	if (TObjective *pObjective=GetObjective(teamId, objective))
		pObjective->status=status;

	if (gEnv->bServer)
	{
		GAMERULES_INVOKE_ON_TEAM(teamId, ClSetObjectiveStatus(), SetObjectiveStatusParams(objective, status))
	}
}

//------------------------------------------------------------------------
void CGameRules::SetObjectiveEntity(int teamId, const char *objective, EntityId entityId)
{
	if (TObjective *pObjective=GetObjective(teamId, objective))
		pObjective->entityId=entityId;

	if (gEnv->bServer)
	{
		GAMERULES_INVOKE_ON_TEAM(teamId, ClSetObjectiveEntity(), SetObjectiveEntityParams(objective, entityId))
	}
}

//------------------------------------------------------------------------
void CGameRules::RemoveObjective(int teamId, const char *objective)
{
	if (TObjectiveMap *pObjectives=GetTeamObjectives(teamId))
	{
		TObjectiveMap::iterator it=pObjectives->find(CONST_TEMP_STRING(objective));
		if (it!=pObjectives->end())
			pObjectives->erase(it);
	}
}

//------------------------------------------------------------------------
void CGameRules::ResetObjectives()
{
	m_objectives.clear();

	if (gEnv->bServer)
		GetGameObject()->InvokeRMI(ClResetObjectives(), NoParams(), eRMI_ToAllClients);
}

//------------------------------------------------------------------------
CGameRules::TObjectiveMap *CGameRules::GetTeamObjectives(int teamId)
{
	TTeamObjectiveMap::iterator it=m_objectives.find(teamId);
	if (it!=m_objectives.end())
		return &it->second;
	return 0;
}

//------------------------------------------------------------------------
CGameRules::TObjective *CGameRules::GetObjective(int teamId, const char *objective)
{
	if (TObjectiveMap *pObjectives=GetTeamObjectives(teamId))
	{
		TObjectiveMap::iterator it=pObjectives->find(CONST_TEMP_STRING(objective));
		if (it!=pObjectives->end())
			return &it->second;
	}
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::UpdateObjectivesForPlayer(int channelId, int teamId)
{
	GetGameObject()->InvokeRMI(ClResetObjectives(), NoParams(), eRMI_ToClientChannel, channelId);

	if (TObjectiveMap *pObjectives=GetTeamObjectives(teamId))
	{
		for (TObjectiveMap::iterator it=pObjectives->begin(); it!=pObjectives->end(); ++it)
		{
			if (it->second.status!=CHUDMissionObjective::DEACTIVATED)
				GetGameObject()->InvokeRMI(ClSetObjective(), SetObjectiveParams(it->first.c_str(), it->second.status, it->second.entityId), eRMI_ToClientChannel, channelId);
		}
	}
}

//------------------------------------------------------------------------
bool CGameRules::IsFrozen(EntityId entityId) const
{
	if (stl::find(m_frozen, entityId))
		return true;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return false;

	if (IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER))
		return (pRenderProxy->GetMaterialLayersMask()&MTL_LAYER_FROZEN) != 0;

	return false;
}

//------------------------------------------------------------------------
void CGameRules::ResetFrozen()
{
	std::vector<EntityId> frozen(m_frozen);
	
	for (std::vector<EntityId>::iterator it=frozen.begin(); it!=frozen.end(); ++it)
		FreezeEntity(*it, false, false);

	m_frozen.resize(0);
}

//------------------------------------------------------------------------
void CGameRules::FreezeEntity(EntityId entityId, bool freeze, bool vapor, bool bForce)
{
	if (IsFrozen(entityId)==freeze)
		return;

	IEntity* pEntity = m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	IGameObject *pGameObject=m_pGameFramework->GetGameObject(entityId);
	IScriptTable *pScriptTable=pEntity->GetScriptTable();
	IActor *pActor=m_pGameFramework->GetIActorSystem()->GetActor(entityId);

	if (freeze && gEnv->bServer)
	{
		// don't freeze if ai doesn't want to
		if (pActor && !pActor->IsPlayer())
		{
			IEntity *pObject=0;
			if (gEnv->pAISystem->SmartObjectEvent("DontFreeze", pEntity, pObject, 0))
				return;
		}

    // if entity implements "GetFrozenAmount", check it and freeze only when >= 1
    HSCRIPTFUNCTION pfnGetFrozenAmount=0;
    if (!bForce && pScriptTable && pScriptTable->GetValue("GetFrozenAmount", pfnGetFrozenAmount))
    {
      float frost = 1.f;
      Script::CallReturn(gEnv->pScriptSystem, pfnGetFrozenAmount, pScriptTable, frost);
      gEnv->pScriptSystem->ReleaseFunc(pfnGetFrozenAmount);

      //CryLog("%s frost amount: %.2f", pEntity->GetName(), frost);

      if (frost < 0.99f)
        return;
    }
	}

	// call script event
	if (pScriptTable && pScriptTable->GetValueType("OnPreFreeze")==svtFunction)
	{
		bool ret=false;
		HSCRIPTFUNCTION func=0;
		pScriptTable->GetValue("OnPreFreeze", func);
		if (Script::CallReturn(pScriptTable->GetScriptSystem(), func, pScriptTable, freeze, vapor, ret) && !ret)
			return;
	}

	// send event to game object
	if (pGameObject)
	{
		SGameObjectEvent event(eCGE_PreFreeze, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void *)freeze);
		pGameObject->SendEvent(event);
	}

/*
	CCustomFreeze *pCustomFreeze;
	if (pGameObject)
		pCustomFreeze=pGameObject->QueryExtension("CustomFreeze");

	if (pCustomFreeze)
		pCustomFreeze->Freeze(freeze);
	else
		*/
	{
		// apply frozen material layer
		IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
		if (pRenderProxy)
		{
			uint8 activeLayers = pRenderProxy->GetMaterialLayersMask();
			if (freeze)
				activeLayers |= MTL_LAYER_FROZEN;
			else
				activeLayers &= ~MTL_LAYER_FROZEN;

			pRenderProxy->SetMaterialLayersMask( activeLayers );
		}

		// set the ice physics material
		
		IPhysicalEntity *pPhysicalEntity = pEntity->GetPhysics();

		int matId = -1;
		if (ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName("mat_ice"))
			matId=pSurfaceType->GetId();
		
		if (pPhysicalEntity && matId>-1)
		{
			pe_status_nparts status_nparts;
			int nparts = pPhysicalEntity->GetStatus(&status_nparts);
			if (nparts)
			{
				for (int i=0; i<nparts; i++)
				{
					pe_params_part part,partset;
					part.ipart = partset.ipart = i;

					pPhysicalEntity->GetParams(&part);
					if (!part.pMatMapping || !part.nMats)
						continue;

					if (freeze)
					{
						if (part.pPhysGeom)
						{
							static int map[255];
							for (int m=0; m<part.nMats; m++)
								map[m]=matId;
							partset.pMatMapping = map;
							partset.nMats = part.nMats;
						}
					}
					else if (part.pPhysGeom)
					{
						partset.pMatMapping = part.pPhysGeom->pMatMapping;
						partset.nMats = part.pPhysGeom->nMats;
					}

					pPhysicalEntity->SetParams(&partset);
				}
			}
		}

		// freeze children
		int n=pEntity->GetChildCount();
		for (int i=0; i<n;i++)
		{
			IEntity *pChild=pEntity->GetChild(i);

			FreezeEntity(pChild->GetId(), freeze, vapor);
		}
	}

	if (freeze)
		stl::push_back_unique(m_frozen, entityId);
	else
		stl::find_and_erase(m_frozen, entityId);

	// send event to game object
	if (pGameObject)
	{
		SGameObjectEvent event(eCGE_PostFreeze, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void *)freeze);
		pGameObject->SendEvent(event);
	}

	// call script event
	if (pScriptTable && pScriptTable->GetValueType("OnPostFreeze")==svtFunction)
		Script::CallMethod(pScriptTable, "OnPostFreeze", freeze);

	if (gEnv->bClient)
	{
		// spawn the vapor
		if (freeze && vapor)
		{
			SpawnParams params;
			params.eAttachForm=GeomForm_Surface;
			params.eAttachType=GeomType_Physics;
			params.bIgnoreLocation=true;

			gEnv->pEntitySystem->GetBreakableManager()->AttachSurfaceEffect(pEntity, 0, SURFACE_BREAKAGE_TYPE("freeze_vapor"), params);
		}
	}

	if (gEnv->bServer)
		GetGameObject()->InvokeRMIWithDependentObject(ClFreezeEntity(), FreezeEntityParams(entityId, freeze, vapor), eRMI_ToRemoteClients, entityId);
}

//------------------------------------------------------------------------
void CGameRules::ShatterEntity(EntityId entityId, const Vec3 &pos, const Vec3 &impulse)
{
	IEntity* pEntity = m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

  // FIXME: Marcio: fix order of Shatter/Freeze on client, otherwise this check fails
  //if (!IsFrozen(entityId)) 
	if (gEnv->bServer && !IsFrozen(entityId)) 
		return;
	pe_params_structural_joint psj;
	psj.idx = 0;
	if (pEntity->GetFlags() & ENTITY_FLAG_MODIFIED_BY_PHYSICS || 
			pEntity->GetPhysics() && pEntity->GetPhysics()->GetParams(&psj))
		return;

	IGameObject *pGameObject=m_pGameFramework->GetGameObject(entityId);
	IScriptTable *pScriptTable=pEntity->GetScriptTable();

	// send event to game object
	if (pGameObject)
	{
		SGameObjectEvent event(eCGE_PreShatter, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, 0);
		pGameObject->SendEvent(event);
	}
	else
	{
		// This is simple entity.
		// So check if stat object at entity slot 0 can be shattered.
		IStatObj *pStatObj = pEntity->GetStatObj(0|ENTITY_SLOT_ACTUAL);
		if (pStatObj)
		{
			if (!gEnv->pEntitySystem->GetBreakableManager()->CanShatter(pStatObj))
				return;
		}
	}

	// call script event
	if (pScriptTable && pScriptTable->GetValueType("OnPreShatter")==svtFunction)
		Script::CallMethod(pScriptTable, "OnPreShatter", pos, impulse);

	int nFrozenSlot=-1;
	if (pScriptTable)
	{
		HSCRIPTFUNCTION pfnGetFrozenSlot=0;
		if (pScriptTable->GetValue("GetFrozenSlot", pfnGetFrozenSlot))
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnGetFrozenSlot, pScriptTable, nFrozenSlot);
			gEnv->pScriptSystem->ReleaseFunc(pfnGetFrozenSlot);
		}
	}

/*
	SpawnParams spawnparams;
	spawnparams.eAttachForm=GeomForm_Surface;
	spawnparams.eAttachType=GeomType_Render;
	spawnparams.bIndependent=true;
	spawnparams.bCountPerUnit=1;
	spawnparams.fCountScale=1.0f;

	gEnv->pEntitySystem->GetBreakableManager()->AttachSurfaceEffect(pEntity, 0, SURFACE_BREAKAGE_TYPE("freeze_shatter"), spawnparams);
*/

	IBreakableManager::BreakageParams breakage;
	breakage.type = IBreakableManager::BREAKAGE_TYPE_FREEZE_SHATTER;
	breakage.bMaterialEffects=true;			// Automatically create "destroy" and "breakage" material effects on pieces.
	breakage.fParticleLifeTime=7.0f;		// Average lifetime of particle pieces.
	breakage.nGenericCount=0;						// If not 0, force particle pieces to spawn generically, this many times.
	breakage.bForceEntity=false;				// Force pieces to spawn as entities.
	breakage.bOnlyHelperPieces=false;	  // Only spawn helper pieces.

	// Impulse params.
	breakage.fExplodeImpulse=10.0f;
	breakage.vHitImpulse=impulse;
	breakage.vHitPoint=pos;

	gEnv->pEntitySystem->GetBreakableManager()->BreakIntoPieces(pEntity, 0, nFrozenSlot, breakage);

	// send event to game object
	if (pGameObject)
	{
		SGameObjectEvent event(eCGE_PostShatter, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, 0);
		pGameObject->SendEvent(event);
	}

	// call script event
	if (pScriptTable && pScriptTable->GetValueType("OnPostShatter")==svtFunction)
		Script::CallMethod(pScriptTable, "OnPostShatter", pos, impulse);

	FreezeEntity(entityId, false, false);

  // shatter children
  int n=pEntity->GetChildCount();
  for (int i=n-1; i>=0; --i)
  {
    if (IEntity *pChild=pEntity->GetChild(i))
      ShatterEntity(pChild->GetId(), pos, impulse);
  }

	if (gEnv->bServer && !m_pGameFramework->IsEditing())
	{
		GetGameObject()->InvokeRMIWithDependentObject(ClShatterEntity(), ShatterEntityParams(entityId, pos, impulse), eRMI_ToRemoteClients, entityId);

		// make sure we don't remove players
		IActor *pActor=m_pGameFramework->GetIActorSystem()->GetActor(entityId);
		if (!pActor || !pActor->IsPlayer())
		{	
			if (gEnv->pSystem->IsEditor()) 
				pEntity->Hide(true);
			else
				m_pEntitySystem->RemoveEntity(entityId);
		}
	}
	else if (m_pGameFramework->IsEditing())
		pEntity->Hide(true);
}


struct compare_spawns
{
	bool operator() (EntityId lhs, EntityId rhs ) const
	{
		int lhsT=g_pGame->GetGameRules()->GetTeam(lhs);
		int rhsT=g_pGame->GetGameRules()->GetTeam(rhs);
		if (lhsT == rhsT)
		{
			EntityId lhsG=g_pGame->GetGameRules()->GetSpawnLocationGroup(lhs);
			EntityId rhsG=g_pGame->GetGameRules()->GetSpawnLocationGroup(rhs);
			if (lhsG==rhsG)
			{
				IEntity *pLhs=gEnv->pEntitySystem->GetEntity(lhs);
				IEntity *pRhs=gEnv->pEntitySystem->GetEntity(rhs);

				return strcmp(pLhs->GetName(), pRhs->GetName())<0;
			}
			return lhsG<rhsG;
		}
		return lhsT<rhsT;
	}
};

//------------------------------------------------------------------------
void CGameRules::AddSpawnLocation(EntityId location)
{
	stl::push_back_unique(m_spawnLocations, location);

	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns());
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpawnLocation(EntityId id)
{
	stl::find_and_erase(m_spawnLocations, id);

	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns());
}

//------------------------------------------------------------------------
int CGameRules::GetSpawnLocationCount() const
{
	return (int)m_spawnLocations.size();
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocation(int idx) const
{
	if (idx>=0 && idx<(int)m_spawnLocations.size())
		return m_spawnLocations[idx];
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetSpawnLocations(TSpawnLocations &locations) const
{
	locations.resize(0);
	locations = m_spawnLocations;
}

//------------------------------------------------------------------------
bool CGameRules::IsSpawnLocationSafe(EntityId playerId, EntityId spawnLocationId, float safeDistance, bool ignoreTeam) const
{
	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	if (safeDistance<=0.01f)
		return true;

	int playerTeamId = GetTeam(playerId);

	SEntityProximityQuery query;
	Vec3	c(pSpawn->GetWorldPos());
	float l(safeDistance*1.25f);

	query.box = AABB(Vec3(c.x-l,c.y-l,c.z-l), Vec3(c.x+l,c.y+l,c.z+l));
	query.nEntityFlags = -1;
	query.pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	gEnv->pEntitySystem->QueryProximity(query);

	for (int i=0; i<query.nCount; i++)
	{
		EntityId entityId=query.pEntities[i]->GetId();
		if (playerId!=entityId)
		{
			IActor *pActor=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId);
			if (pActor)
			{
				if ((pActor->GetEntity()->GetWorldPos()-c).len2()<=2.0f*2.0f)
					return false;

				if (!ignoreTeam && playerTeamId!=GetTeam(entityId))
					return false;
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
static bool IsGoodSpawn(EntityId spawnId, EntityId playerId, float safeDistance, bool ignoreTeam, bool includeNeutral, EntityId groupId)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();

	int teamId = pGameRules->GetTeam(spawnId);
	int playerTeamId = pGameRules->GetTeam(playerId);

	if ((!ignoreTeam && playerTeamId!=teamId) && (!includeNeutral && teamId))
		return false;

	if (groupId && pGameRules->GetSpawnLocationGroup(spawnId)!=groupId)
		return false;

	if (!pGameRules->IsSpawnLocationSafe(playerId, spawnId, safeDistance, ignoreTeam))
		return false;

	return true;
}


//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocation(EntityId playerId, float safeDistance, bool ignoreTeam, bool includeNeutral, EntityId groupId) const
{
	EntityId spawnId=0;
	int n=(int)m_spawnLocations.size();
	int startIdx=0;
	int endIdx=n;
	
	if (n>0)
	{
		int playerTeamId = GetTeam(playerId);

		if (!ignoreTeam)
		{
			startIdx=-1;
			endIdx=-1;
			for (int it=0; it<n; ++it)	// find a good
			{
				int spawnTeamId=GetTeam(m_spawnLocations[it]);
				
				bool sameGroup=(!groupId || GetSpawnLocationGroup(spawnId)==groupId);
				bool sameTeam=((includeNeutral && !spawnTeamId) || (spawnTeamId==playerTeamId));

				if (startIdx==-1 && sameTeam && sameGroup)
					startIdx=it;
				if (startIdx!=-1 && sameTeam && sameGroup)
					endIdx=it;
				if (startIdx!=-1 && (!sameTeam || !sameGroup))
					break;
			}
		}

		if (startIdx<0)
			return 0;
		
		int s=startIdx+Random(endIdx-startIdx);
		int i=s;

		spawnId=m_spawnLocations[i];
		while (!IsGoodSpawn(spawnId, playerId, safeDistance, ignoreTeam, includeNeutral, groupId))
		{
			++i;
			if (i>endIdx)
			{
				i=startIdx;
				if (i==endIdx)
				{
					if (ignoreTeam || playerTeamId==GetTeam(spawnId))
						return spawnId;
					return 0;
				}
			}
			
			if (i==s)
			{
				if (safeDistance>0.001f)
					safeDistance=0.0f;
				else
					return 0;
			}

			spawnId=m_spawnLocations[i];
		}
	}

	CryLogAlways("spawning at %d", spawnId);

	return spawnId;
}
													
//------------------------------------------------------------------------
EntityId CGameRules::GetFirstSpawnLocation(int teamId, EntityId groupId) const
{
	if (!m_spawnLocations.empty())
	{
		for (TSpawnLocations::const_iterator it=m_spawnLocations.begin(); it!=m_spawnLocations.end(); ++it)
		{
			if (teamId==GetTeam(*it) && (!groupId || groupId==GetSpawnLocationGroup(*it)))
				return *it;
		}
	}

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::AddSpawnGroup(EntityId groupId)
{
	if (m_spawnGroups.find(groupId)==m_spawnGroups.end())
		m_spawnGroups.insert(TSpawnGroupMap::value_type(groupId, TSpawnLocations()));

	if (gEnv->bServer)
		GetGameObject()->InvokeRMIWithDependentObject(ClAddSpawnGroup(), SpawnGroupParams(groupId), eRMI_ToAllClients|eRMI_NoLocalCalls, groupId);
}

//------------------------------------------------------------------------
void CGameRules::AddSpawnLocationToSpawnGroup(EntityId groupId, EntityId location)
{
	TSpawnGroupMap::iterator it=m_spawnGroups.find(groupId);
	if (it==m_spawnGroups.end())
		return;

	stl::push_back_unique(it->second, location);
	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns()); // need to resort spawn location
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpawnLocationFromSpawnGroup(EntityId groupId, EntityId location)
{
	TSpawnGroupMap::iterator it=m_spawnGroups.find(groupId);
	if (it==m_spawnGroups.end())
		return;

	stl::find_and_erase(it->second, location);
	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns()); // need to resort spawn location
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpawnGroup(EntityId groupId)
{
	TSpawnGroupMap::iterator it=m_spawnGroups.find(groupId);
	if (it!=m_spawnGroups.end())
		m_spawnGroups.erase(it);

	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns()); // need to resort spawn location

	if (gEnv->bServer)
	{
		GetGameObject()->InvokeRMI(ClRemoveSpawnGroup(), SpawnGroupParams(groupId), eRMI_ToAllClients|eRMI_NoLocalCalls, groupId);

		TTeamIdEntityIdMap::iterator next;
		for (TTeamIdEntityIdMap::iterator dit=m_teamdefaultspawns.begin(); dit!=m_teamdefaultspawns.end(); dit=next)
		{
			next=dit;
			++next;
			if (dit->second==groupId)
				m_teamdefaultspawns.erase(dit);
		}
	}

	CheckSpawnGroupValidity(groupId);
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocationGroup(EntityId spawnId) const
{
	for (TSpawnGroupMap::const_iterator it=m_spawnGroups.begin(); it!=m_spawnGroups.end(); ++it)
	{
		TSpawnLocations::const_iterator sit=std::find(it->second.begin(), it->second.end(), spawnId);
		if (sit!=it->second.end())
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetSpawnGroupCount() const
{
	return (int)m_spawnGroups.size();
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnGroup(int idx) const
{
	if (idx>=0 && idx<(int)m_spawnGroups.size())
	{
		TSpawnGroupMap::const_iterator it=m_spawnGroups.begin();
		std::advance(it, idx);
		return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetSpawnGroups(TSpawnLocations &groups) const
{
	groups.resize(0);
	groups.reserve(m_spawnGroups.size());
	for (TSpawnGroupMap::const_iterator it=m_spawnGroups.begin(); it!=m_spawnGroups.end(); ++it)
		groups.push_back(it->first);
}

//------------------------------------------------------------------------
void CGameRules::RequestSpawnGroup(EntityId spawnGroupId)
{
	CallScript(m_script, "RequestSpawnGroup", ScriptHandle(spawnGroupId));
}

//------------------------------------------------------------------------
void CGameRules::SetPlayerSpawnGroup(EntityId playerId, EntityId spawnGroupId)
{
	CallScript(m_script, "SetPlayerSpawnGroup", ScriptHandle(playerId), ScriptHandle(spawnGroupId));
}

//------------------------------------------------------------------------
EntityId CGameRules::GetPlayerSpawnGroup(CActor *pActor)
{
	if (m_script->GetValueType("GetPlayerSpawnGroup") != svtFunction)
		return 0;

	ScriptHandle ret(0);
	m_pScriptSystem->BeginCall(m_script, "GetPlayerSpawnGroup");
	m_pScriptSystem->PushFuncParam(m_script);
	m_pScriptSystem->PushFuncParam(pActor->GetEntity()->GetScriptTable());
	m_pScriptSystem->EndCall(ret);

	return (EntityId)ret.n;
}

//------------------------------------------------------------------------
void CGameRules::SetTeamDefaultSpawnGroup(int teamId, EntityId spawnGroupId)
{
	TTeamIdEntityIdMap::iterator it=m_teamdefaultspawns.find(teamId);
	
	if (it!=m_teamdefaultspawns.end())
		it->second=spawnGroupId;
	else
		m_teamdefaultspawns.insert(TTeamIdEntityIdMap::value_type(teamId, spawnGroupId));
}

//------------------------------------------------------------------------
EntityId CGameRules::GetTeamDefaultSpawnGroup(int teamId)
{
	TTeamIdEntityIdMap::iterator it=m_teamdefaultspawns.find(teamId);
	if (it!=m_teamdefaultspawns.end())
		return it->second;
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::CheckSpawnGroupValidity(EntityId spawnGroupId)
{
	bool exists=spawnGroupId &&
		(m_spawnGroups.find(spawnGroupId)!=m_spawnGroups.end()) &&
		(gEnv->pEntitySystem->GetEntity(spawnGroupId)!=0);
	bool valid=exists && GetTeam(spawnGroupId)!=0;

	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		CActor *pActor=GetActorByChannelId(*it);
		if (!pActor)
			continue;

		EntityId playerId=pActor->GetEntityId();
		if (GetPlayerSpawnGroup(pActor)==spawnGroupId)
		{
			if (!valid || GetTeam(spawnGroupId)!=GetTeam(playerId))
				CallScript(m_serverScript, "OnSpawnGroupInvalid", ScriptHandle(playerId), ScriptHandle(spawnGroupId));
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::AddSpectatorLocation(EntityId location)
{
	stl::push_back_unique(m_spectatorLocations, location);
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpectatorLocation(EntityId id)
{
	stl::find_and_erase(m_spectatorLocations, id);
}

//------------------------------------------------------------------------
int CGameRules::GetSpectatorLocationCount() const
{
	return (int)m_spectatorLocations.size();
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpectatorLocation(int idx) const
{
	if (idx>=0 && idx<m_spectatorLocations.size())
		return m_spectatorLocations[idx];
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetSpectatorLocations(TSpawnLocations &locations) const
{
	locations.resize(0);
	locations = m_spectatorLocations;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetRandomSpectatorLocation() const
{
	int idx=Random(GetSpectatorLocationCount());
	return GetSpectatorLocation(idx);
}

//------------------------------------------------------------------------
EntityId CGameRules::GetInterestingSpectatorLocation() const
{
	return GetRandomSpectatorLocation();
}

//------------------------------------------------------------------------
void CGameRules::ResetMinimap()
{
	m_minimap.resize(0);

	if (gEnv->bServer)
		GetGameObject()->InvokeRMI(ClResetMinimap(), NoParams(), eRMI_ToAllClients|eRMI_NoLocalCalls);
}

//------------------------------------------------------------------------
void CGameRules::UpdateMinimap(float frameTime)
{
	TMinimap::iterator next;
	for (TMinimap::iterator eit=m_minimap.begin(); eit!=m_minimap.end(); eit=next)
	{
		next=eit;
		++next;
		SMinimapEntity &entity=*eit;
		if (entity.lifetime>0.0f)
		{
			entity.lifetime-=frameTime;
			if (entity.lifetime<=0.0f)
				next=m_minimap.erase(eit);
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::AddMinimapEntity(EntityId entityId, int type, float lifetime)
{
	TMinimap::iterator it=std::find(m_minimap.begin(), m_minimap.end(), SMinimapEntity(entityId, 0, 0.0f));
	if (it!=m_minimap.end())
	{
		if (type>it->type)
			it->type=type;
		if (lifetime==0.0f || lifetime>it->lifetime)
			it->lifetime=lifetime;
	}
	else
		m_minimap.push_back(SMinimapEntity(entityId, type, lifetime));

	if (gEnv->bServer)
		GetGameObject()->InvokeRMIWithDependentObject(ClAddMinimapEntity(), AddMinimapEntityParams(entityId, lifetime, (int)type), eRMI_ToAllClients|eRMI_NoLocalCalls, entityId);
}

//------------------------------------------------------------------------
void CGameRules::RemoveMinimapEntity(EntityId entityId)
{
	stl::find_and_erase(m_minimap, SMinimapEntity(entityId, 0, 0.0f));

	if (gEnv->bServer)
		GetGameObject()->InvokeRMI(ClRemoveMinimapEntity(), EntityParams(entityId), eRMI_ToAllClients|eRMI_NoLocalCalls);
}

//------------------------------------------------------------------------
const CGameRules::TMinimap &CGameRules::GetMinimapEntities() const
{
	return m_minimap;
}

//------------------------------------------------------------------------
void CGameRules::AddHitListener(IHitListener* pHitListener)
{
	stl::push_back_unique(m_hitListeners, pHitListener);
}

//------------------------------------------------------------------------
void CGameRules::RemoveHitListener(IHitListener* pHitListener)
{
	stl::find_and_erase(m_hitListeners, pHitListener);
}

//------------------------------------------------------------------------
void CGameRules::AddGameRulesListener(SGameRulesListener* pRulesListener)
{
	stl::push_back_unique(m_rulesListeners, pRulesListener);
}

//------------------------------------------------------------------------
void CGameRules::RemoveGameRulesListener(SGameRulesListener* pRulesListener)
{
	stl::find_and_erase(m_rulesListeners, pRulesListener);
}

//------------------------------------------------------------------------
int CGameRules::RegisterHitMaterial(const char *materialName)
{
	if (int id=GetHitMaterialId(materialName))
		return id;

	ISurfaceType *pSurfaceType=m_pMaterialManager->GetSurfaceTypeByName(materialName);
	if (pSurfaceType)
	{
		m_hitMaterials.insert(THitMaterialMap::value_type(++m_hitMaterialIdGen, pSurfaceType->GetId()));
		return m_hitMaterialIdGen;
	}
	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetHitMaterialId(const char *materialName) const
{
	ISurfaceType *pSurfaceType=m_pMaterialManager->GetSurfaceTypeByName(materialName);
	if (!pSurfaceType)
		return 0;

	int id=pSurfaceType->GetId();

	for (THitMaterialMap::const_iterator it=m_hitMaterials.begin(); it!=m_hitMaterials.end(); ++it)
	{
		if (it->second==id)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetHitMaterialIdFromSurfaceId(int surfaceId) const
{
	for (THitMaterialMap::const_iterator it=m_hitMaterials.begin(); it!=m_hitMaterials.end(); ++it)
	{
		if (it->second==surfaceId)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
ISurfaceType *CGameRules::GetHitMaterial(int id) const
{
	THitMaterialMap::const_iterator it=m_hitMaterials.find(id);
	if (it==m_hitMaterials.end())
		return 0;

	ISurfaceType *pSurfaceType=m_pMaterialManager->GetSurfaceType(it->second);
	
	return pSurfaceType;
}

//------------------------------------------------------------------------
void CGameRules::ResetHitMaterials()
{
	m_hitMaterials.clear();
	m_hitMaterialIdGen=0;
}

//------------------------------------------------------------------------
int CGameRules::RegisterHitType(const char *type)
{
	if (int id=GetHitTypeId(type))
		return id;

	m_hitTypes.insert(THitTypeMap::value_type(++m_hitTypeIdGen, type));
	return m_hitTypeIdGen;
}

//------------------------------------------------------------------------
int CGameRules::GetHitTypeId(const char *type) const
{
	for (THitTypeMap::const_iterator it=m_hitTypes.begin(); it!=m_hitTypes.end(); ++it)
	{
		if (it->second==type)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
const char *CGameRules::GetHitType(int id) const
{
	THitTypeMap::const_iterator it=m_hitTypes.find(id);
	if (it==m_hitTypes.end())
		return 0;

	return it->second.c_str();
}

//------------------------------------------------------------------------
void CGameRules::ResetHitTypes()
{
	m_hitTypes.clear();
	m_hitTypeIdGen=0;
}

//------------------------------------------------------------------------
void CGameRules::SendTextMessage(ETextMessageType type, const char *msg, unsigned int to, int channelId,
																 const char *p0, const char *p1, const char *p2, const char *p3)
{
	GetGameObject()->InvokeRMI(ClTextMessage(), TextMessageParams(type, msg, p0, p1, p2, p3), to, channelId);
}

//------------------------------------------------------------------------
void CGameRules::SendChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg)
{
	ChatMessageParams params(type, sourceId, targetId, msg);

	if (gEnv->bServer)
	{
		switch(type)
		{
		case eChatToAll:
			GetGameObject()->InvokeRMI(ClChatMessage(), params, eRMI_ToAllClients);
			break;
		case eChatToTarget:
			GetGameObject()->InvokeRMIWithDependentObject(ClChatMessage(), params, eRMI_ToClientChannel, targetId, GetChannelId(targetId));
			break;
		case eChatToTeam:
			{
				int teamId = GetTeam(sourceId);
				if (teamId)
				{
					TPlayerTeamIdMap::const_iterator tit=m_playerteams.find(teamId);
					if (tit!=m_playerteams.end())
					{
						for (TPlayers::const_iterator it=tit->second.begin(); it!=tit->second.end(); ++it)
							GetGameObject()->InvokeRMIWithDependentObject(ClChatMessage(), params, eRMI_ToClientChannel, *it, GetChannelId(*it));
					}
				}
			}
		}
	}
	else
		GetGameObject()->InvokeRMI(SvRequestChatMessage(), params, eRMI_ToServer);
}

//------------------------------------------------------------------------
void CGameRules::ForbiddenAreaWarning(bool active, int timer, EntityId targetId)
{
	GetGameObject()->InvokeRMI(ClForbiddenAreaWarning(), ForbiddenAreaWarningParams(active, timer), eRMI_ToClientChannel, GetChannelId(targetId));
}

//------------------------------------------------------------------------

void CGameRules::ResetGameTime()
{
	m_endTime.SetSeconds(0.0f);

	float timeLimit=g_pGameCVars->g_timelimit;
	if (timeLimit>0.0f)
		m_endTime.SetSeconds(m_pGameFramework->GetServerTime().GetSeconds()+timeLimit*60.0f);

	GetGameObject()->InvokeRMI(ClSetGameTime(), SetGameTimeParams(m_endTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingGameTime() const
{
	return MAX(0, (m_endTime-m_pGameFramework->GetServerTime()).GetSeconds());
}

//------------------------------------------------------------------------
bool CGameRules::IsTimeLimited() const
{
	return m_endTime.GetSeconds()>0.0f;
}

//------------------------------------------------------------------------
void CGameRules::ResetRoundTime()
{
	m_roundEndTime.SetSeconds(0.0f);

	float roundTime=g_pGameCVars->g_roundtime;
	if (roundTime>0.0f)
		m_roundEndTime.SetSeconds(m_pGameFramework->GetServerTime().GetSeconds()+roundTime*60.0f);

	GetGameObject()->InvokeRMI(ClSetRoundTime(), SetGameTimeParams(m_roundEndTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingRoundTime() const
{
	return MAX(0, (m_roundEndTime-m_pGameFramework->GetServerTime()).GetSeconds());
}

//------------------------------------------------------------------------
bool CGameRules::IsRoundTimeLimited() const
{
	return m_roundEndTime.GetSeconds()>0.0f;
}

//------------------------------------------------------------------------
void CGameRules::ResetPreRoundTime()
{
	m_preRoundEndTime.SetSeconds(0.0f);

	int preRoundTime=g_pGameCVars->g_preroundtime;
	if (preRoundTime>0.0f)
		m_preRoundEndTime.SetSeconds(m_pGameFramework->GetServerTime().GetSeconds()+preRoundTime);

	GetGameObject()->InvokeRMI(ClSetPreRoundTime(), SetGameTimeParams(m_preRoundEndTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingPreRoundTime() const
{
	return MAX(0, (m_preRoundEndTime-m_pGameFramework->GetServerTime()).GetSeconds());
}

//------------------------------------------------------------------------
void CGameRules::RegisterConsoleCommands(IConsole *pConsole)
{
	// todo: move to power struggle implementation when there is one
	pConsole->AddCommand("buy",			"if (g_gameRules and g_gameRules.Buy) then g_gameRules:Buy(%1); end");
	pConsole->AddCommand("buyammo", "if (g_gameRules and g_gameRules.BuyAmmo) then g_gameRules:BuyAmmo(%%); end");
	pConsole->AddCommand("avarst_meharties", "if (g_gameRules) then g_gameRules:RestartGame(true); end");
	pConsole->AddCommand("g_debug_spawns", CmdDebugSpawns);
	pConsole->AddCommand("g_debug_minimap", CmdDebugMinimap);
	pConsole->AddCommand("g_debug_teams", CmdDebugTeams);
	pConsole->AddCommand("g_debug_objectives", CmdDebugObjectives);
}

//------------------------------------------------------------------------
void CGameRules::UnregisterConsoleCommands(IConsole *pConsole)
{
	pConsole->RemoveCommand("buy");
	pConsole->RemoveCommand("buyammo");
	pConsole->RemoveCommand("avarst_meharties");
	pConsole->RemoveCommand("g_debug_spawns");
	pConsole->RemoveCommand("g_debug_minimap");
	pConsole->RemoveCommand("g_debug_teams");
	pConsole->RemoveCommand("g_debug_objectives");
}

//------------------------------------------------------------------------
void CGameRules::RegisterConsoleVars(IConsole *pConsole)
{
}


//------------------------------------------------------------------------
void CGameRules::CmdDebugSpawns(IConsoleCmdArgs *pArgs)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_spawnGroups.empty())
	{
		CryLogAlways("// Spawn Groups //");
		for (TSpawnGroupMap::const_iterator sit=pGameRules->m_spawnGroups.begin(); sit!=pGameRules->m_spawnGroups.end(); ++sit)
		{
			IEntity *pEntity=gEnv->pEntitySystem->GetEntity(sit->first);
			int groupTeamId=pGameRules->GetTeam(pEntity->GetId());
			const char *Default="$5*DEFAULT*";
			CryLogAlways("Spawn Group: %s  (eid: %d %08x  team: %d) %s", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), groupTeamId, 
				(sit->first==pGameRules->GetTeamDefaultSpawnGroup(groupTeamId))?Default:"");

			for (TSpawnLocations::const_iterator lit=sit->second.begin(); lit!=sit->second.end(); ++lit)
			{
				int spawnTeamId=pGameRules->GetTeam(pEntity->GetId());
				IEntity *pEntity=gEnv->pEntitySystem->GetEntity(*lit);
				const char *cs="";
				if (spawnTeamId && spawnTeamId!=groupTeamId)
					cs="$4";
				CryLogAlways("    -> Spawn Location: %s  (eid: %d %08x  team: %d)", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), spawnTeamId);
			}
		}
	}

	CryLogAlways("// Spawn Locations //");
	for (TSpawnLocations::const_iterator lit=pGameRules->m_spawnLocations.begin(); lit!=pGameRules->m_spawnLocations.end(); ++lit)
	{
		IEntity *pEntity=gEnv->pEntitySystem->GetEntity(*lit);
		CryLogAlways("Spawn Location: %s  (eid: %d %08x  team: %d)", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), pGameRules->GetTeam(pEntity->GetId()));
	}
}

//------------------------------------------------------------------------
void CGameRules::CmdDebugMinimap(IConsoleCmdArgs *pArgs)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_minimap.empty())
	{
		CryLogAlways("// Minimap Entities //");
		for (TMinimap::const_iterator it=pGameRules->m_minimap.begin(); it!=pGameRules->m_minimap.end(); ++it)
		{
			IEntity *pEntity=gEnv->pEntitySystem->GetEntity(it->entityId);
			CryLogAlways("  -> Entity %s  (eid: %d %08x  class: %s  lifetime: %.3f  type: %d)", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), pEntity->GetClass()->GetName(), it->lifetime, it->type);
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::CmdDebugTeams(IConsoleCmdArgs *pArgs)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_entityteams.empty())
	{
		CryLogAlways("// Teams //");
		for (TTeamIdMap::const_iterator tit=pGameRules->m_teams.begin(); tit!=pGameRules->m_teams.end(); ++tit)
		{
			CryLogAlways("Team: %s  (id: %d)", tit->first.c_str(), tit->second);
			for (TEntityTeamIdMap::const_iterator eit=pGameRules->m_entityteams.begin(); eit!=pGameRules->m_entityteams.end(); ++eit)
			{
				if (eit->second==tit->second)
				{
					IEntity *pEntity=gEnv->pEntitySystem->GetEntity(eit->first);
					CryLogAlways("    -> Entity: %s  class: %s  (eid: %d %08x)", pEntity->GetName(), pEntity->GetClass()->GetName(), pEntity->GetId(), pEntity->GetId());
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::CmdDebugObjectives(IConsoleCmdArgs *pArgs)
{
	const char *status[CHUDMissionObjective::LAST+1];
	status[0]="$5invalid$o";
	status[CHUDMissionObjective::LAST]="$5invalid$o";

	status[CHUDMissionObjective::ACTIVATED]="$3active$o";
	status[CHUDMissionObjective::DEACTIVATED]="$9inactive$o";
	status[CHUDMissionObjective::COMPLETED]="$2complete$o";
	status[CHUDMissionObjective::FAILED]="$4failed$o";

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_objectives.empty())
	{
		CryLogAlways("// Teams //");
		for (TTeamIdMap::const_iterator tit=pGameRules->m_teams.begin(); tit!=pGameRules->m_teams.end(); ++tit)
		{
			if (TObjectiveMap *pObjectives=pGameRules->GetTeamObjectives(tit->second))
			{
				for (TObjectiveMap::const_iterator it=pObjectives->begin(); it!=pObjectives->end(); ++it)
					CryLogAlways("  -> Objective: %s  teamId: %d  status: %s  (eid: %d %08x)", it->first.c_str(), tit->second,
						status[CLAMP(it->second.status, 0, CHUDMissionObjective::LAST)], it->second.entityId, it->second.entityId);
			}
		}
	}
}
//------------------------------------------------------------------------
void CGameRules::CreateScriptHitInfo(SmartScriptTable &scriptHitInfo, const HitInfo &hitInfo)
{
	CScriptSetGetChain hit(scriptHitInfo);
	{
		hit.SetValue("normal", hitInfo.normal);
		hit.SetValue("pos", hitInfo.pos);
		hit.SetValue("dir", hitInfo.dir);
		hit.SetValue("partId", hitInfo.partId);
		hit.SetValue("backface", hitInfo.normal.Dot(hitInfo.dir)>=0.0f);

		hit.SetValue("targetId", ScriptHandle(hitInfo.targetId));		
		hit.SetValue("shooterId", ScriptHandle(hitInfo.shooterId));
		hit.SetValue("weaponId", ScriptHandle(hitInfo.weaponId));
		hit.SetValue("projectileId", ScriptHandle(hitInfo.projectileId));

		IEntity *pTarget=m_pEntitySystem->GetEntity(hitInfo.targetId);
		IEntity *pShooter=m_pEntitySystem->GetEntity(hitInfo.shooterId);
		IEntity *pWeapon=m_pEntitySystem->GetEntity(hitInfo.weaponId);
		IEntity *pProjectile=m_pEntitySystem->GetEntity(hitInfo.projectileId);

		hit.SetValue("projectile", pProjectile?pProjectile->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("target", pTarget?pTarget->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("shooter", pShooter?pShooter->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("weapon", pWeapon?pWeapon->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("projectile_class", pProjectile?pProjectile->GetClass()->GetName():"");

		hit.SetValue("materialId", hitInfo.material);
		
		ISurfaceType *pSurfaceType=GetHitMaterial(hitInfo.material);
		if (pSurfaceType)
		{
			hit.SetValue("material", pSurfaceType->GetName());
			hit.SetValue("material_type", pSurfaceType->GetType());
		}
		else
		{
			hit.SetToNull("material");
			hit.SetToNull("material_type");
		}

		hit.SetValue("damage", hitInfo.damage);
		hit.SetValue("radius", hitInfo.radius);
    	hit.SetValue("frost", hitInfo.frost);
		
		hit.SetValue("typeId", hitInfo.type);
		const char *type=GetHitType(hitInfo.type);
    	hit.SetValue("type", type ? type : "");
		hit.SetValue("remote", hitInfo.remote);
		hit.SetValue("bulletType", hitInfo.bulletType);
	
		// Check for hit assistance
		float assist=0.0f;
		if (pShooter && 
			((g_pGameCVars->hit_assistSingleplayerEnabled && !gEnv->bMultiplayer) ||
			(g_pGameCVars->hit_assistMultiplayerEnabled && gEnv->bMultiplayer)))
		{
			IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pShooter->GetId());

			if (pActor && pActor->IsPlayer())
			{
				CPlayer *player = (CPlayer *)pActor;
				assist=player->HasHitAssistance() ? 1.0f : 0.0f;
			}
		}
		
		hit.SetValue("assistance", assist);		
	}
}

//------------------------------------------------------------------------
void CGameRules::CreateScriptExplosionInfo(SmartScriptTable &scriptExplosionInfo, const ExplosionInfo &explosionInfo, const pe_explosion* pExplosion)
{
	CScriptSetGetChain explosion(scriptExplosionInfo);
	{
		explosion.SetValue("pos", explosionInfo.pos);
		explosion.SetValue("dir", explosionInfo.dir);

		explosion.SetValue("shooterId", ScriptHandle(explosionInfo.shooterId));
		explosion.SetValue("weaponId", ScriptHandle(explosionInfo.weaponId));    
		IEntity *pShooter=m_pEntitySystem->GetEntity(explosionInfo.shooterId);
		IEntity *pWeapon=m_pEntitySystem->GetEntity(explosionInfo.weaponId);    
		explosion.SetValue("shooter", pShooter?pShooter->GetScriptTable():(IScriptTable *)0);
		explosion.SetValue("weapon", pWeapon?pWeapon->GetScriptTable():(IScriptTable *)0);
		explosion.SetValue("materialId", 0);
		explosion.SetValue("damage", explosionInfo.damage);
		explosion.SetValue("radius", explosionInfo.radius);
		explosion.SetValue("pressure", explosionInfo.pressure);
		explosion.SetValue("hole_size", explosionInfo.hole_size);
		explosion.SetValue("effect", explosionInfo.pParticleEffect?explosionInfo.pParticleEffect->GetName():"");
		explosion.SetValue("effectScale", explosionInfo.effect_scale);
		explosion.SetValue("effectClass", explosionInfo.effect_class.c_str());
		explosion.SetValue("typeId", explosionInfo.type);
		const char *type=GetHitType(explosionInfo.type);
		explosion.SetValue("type", type);
		explosion.SetValue("angle", explosionInfo.angle);
		
		explosion.SetValue("impact", explosionInfo.impact);
		explosion.SetValue("impact_velocity", explosionInfo.impact_velocity);
		explosion.SetValue("impact_normal", explosionInfo.impact_normal);
    explosion.SetValue("impact_targetId", ScriptHandle(explosionInfo.impact_targetId));		
	}
  
  SmartScriptTable affected;
  if (scriptExplosionInfo->GetValue("AffectedEntities", affected))
  {
    affected->Clear();

    if (pExplosion && pExplosion->nAffectedEnts)
    {       
      // create unique entity set
      std::set<IEntity*> ents;      
      
      for (int i=0; i<pExplosion->nAffectedEnts; ++i)
      { 
        if (IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pExplosion->pAffectedEnts[i]))
        { 
          if (IScriptTable *pEntityTable = pEntity->GetScriptTable())          
            ents.insert(pEntity);
        }
      }      

      int k=0;      
      for (std::set<IEntity*>::const_iterator it=ents.begin(),end=ents.end(); it!=end; ++it)
      {
        affected->SetAt(++k, (*it)->GetScriptTable());          
        //CryLog("--- %i: adding %s ", k, (*it)->GetName());   
      }
    }  
  }
}

//------------------------------------------------------------------------
void CGameRules::Restart()
{
	if (gEnv->bServer)
		CallScript(m_script, "RestartGame");
}

//------------------------------------------------------------------------
void CGameRules::NextLevel()
{
  if (!gEnv->bServer)
    return;

	ILevelRotation *pLevelRotation=m_pGameFramework->GetILevelSystem()->GetLevelRotation();
	if (!pLevelRotation->GetLength())
		Restart();
	else
		pLevelRotation->ChangeLevel();
}

//------------------------------------------------------------------------
void CGameRules::ResetEntities()
{
	//g_pGame->GetIGameFramework()->Reset(gEnv->bServer);
	SEntityEvent event(ENTITY_EVENT_START_GAME);
	gEnv->pEntitySystem->SendEventToAll(event);
}

//------------------------------------------------------------------------
void CGameRules::OnEndGame()
{
	bool isMultiplayer=gEnv->bMultiplayer ;

	if (isMultiplayer && gEnv->bServer)
		m_teamVoiceGroups.clear();

	if(gEnv->bClient)
	{
		IActionMapManager *pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("multiplayer",!isMultiplayer);

		IActionMap *am=pActionMapMan->GetActionMap("multiplayer");
		if(am)
		{
			am->SetActionListener(0);
		}
	}

}

//------------------------------------------------------------------------
void CGameRules::GameOver(int localWinner)
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while (iter != m_rulesListeners.end())
		{
			(*iter)->GameOver(localWinner);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::EnteredGame()
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while (iter != m_rulesListeners.end())
		{
			(*iter)->EnteredGame();
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::EndGameNear(EntityId id)
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while(iter != m_rulesListeners.end())
		{
			(*iter)->EndGameNear(id);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::CreateEntityRespawnData(EntityId entityId)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	SEntityRespawnData respawn;
	respawn.position = pEntity->GetWorldPos();
	respawn.rotation = pEntity->GetWorldRotation();
	respawn.scale = pEntity->GetScale();
	respawn.flags = pEntity->GetFlags();
	respawn.pClass = pEntity->GetClass();
#ifdef _DEBUG
	respawn.name = pEntity->GetName();
#endif
	
	IScriptTable *pScriptTable = pEntity->GetScriptTable();

	if (pScriptTable)
		pScriptTable->GetValue("Properties", respawn.properties);

	m_respawndata.insert(TEntityRespawnDataMap::value_type(entityId, respawn));
}

//------------------------------------------------------------------------
bool CGameRules::HasEntityRespawnData(EntityId entityId) const
{
	return m_respawndata.find(entityId)!=m_respawndata.end();
}

//------------------------------------------------------------------------
void CGameRules::ScheduleEntityRespawn(EntityId entityId, bool unique, float timer)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	SEntityRespawn respawn;
	respawn.timer = timer;
	respawn.unique = unique;

	m_respawns.insert(TEntityRespawnMap::value_type(entityId, respawn));
}

//------------------------------------------------------------------------
void CGameRules::UpdateEntitySchedules(float frameTime)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	TEntityRespawnMap::iterator next;
	for (TEntityRespawnMap::iterator it=m_respawns.begin(); it!=m_respawns.end(); it=next)
	{
		next=it; ++next;
		EntityId id=it->first;
		SEntityRespawn &respawn=it->second;

		if (respawn.unique)
		{
			IEntity *pEntity=m_pEntitySystem->GetEntity(id);
			if (pEntity)
				continue;
		}

		respawn.timer -= frameTime;
		if (respawn.timer<=0.0f)
		{
			TEntityRespawnDataMap::iterator dit=m_respawndata.find(id);
			
			if (dit==m_respawndata.end())
      {
        m_respawns.erase(it);
				continue;
      }

			SEntityRespawnData &data=dit->second;

			SEntitySpawnParams params;
			params.pClass=data.pClass;
			params.qRotation=data.rotation;
			params.vPosition=data.position;
			params.vScale=data.scale;
			params.nFlags=data.flags;

			string name;
#ifdef _DEBUG
			name=data.name;
			name.append("_repop");
#else
			name=data.pClass->GetName();
#endif
			params.sName = name.c_str();

			IEntity *pEntity=m_pEntitySystem->SpawnEntity(params, false);
			if (pEntity && data.properties.GetPtr())
			{
				SmartScriptTable properties;
				IScriptTable *pScriptTable=pEntity->GetScriptTable();
				if (pScriptTable && pScriptTable->GetValue("Properties", properties))
				{
					if (properties.GetPtr())
						properties->Clone(data.properties, true);
				}
			}

			m_pEntitySystem->InitEntity(pEntity, params);
			m_respawns.erase(it);
			m_respawndata.erase(dit);
		}
	}

	TEntityRemovalMap::iterator rnext;
	for (TEntityRemovalMap::iterator it=m_removals.begin(); it!=m_removals.end(); it=rnext)
	{
		rnext=it; ++rnext;
		EntityId id=it->first;
		SEntityRemovalData &removal=it->second;

		IEntity *pEntity=m_pEntitySystem->GetEntity(id);
		if (!pEntity)
		{
			m_removals.erase(it);
			continue;
		}

		if (removal.visibility)
		{
			AABB aabb;
			pEntity->GetWorldBounds(aabb);

			CCamera &camera=m_pSystem->GetViewCamera();
			if (camera.IsAABBVisible_F(aabb))
			{
				removal.timer=removal.time;
				continue;
			}
		}

		removal.timer-=frameTime;
		if (removal.timer<=0.0f)
		{
			m_pEntitySystem->RemoveEntity(id);
			m_removals.erase(it);
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ForceScoreboard(bool force)
{
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->ForceScoreBoard(force);
}

//------------------------------------------------------------------------
void CGameRules::FreezeInput(bool freeze)
{
	gEnv->pInput->ClearKeyState();

	g_pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("freezetime", freeze);
/*
	if (IActor *pClientIActor=g_pGame->GetIGameFramework()->GetClientActor())
	{
		CActor *pClientActor=static_cast<CActor *>(pClientIActor);
		if (CWeapon *pWeapon=pClientActor->GetWeapon(pClientActor->GetCurrentItemId()))
			pWeapon->StopFire(pClientActor->GetEntityId());
	}
	*/
}

//------------------------------------------------------------------------
void CGameRules::AbortEntityRespawn(EntityId entityId, bool destroyData)
{
	TEntityRespawnMap::iterator it=m_respawns.find(entityId);
	if (it!=m_respawns.end())
		m_respawns.erase(it);

	if (destroyData)
	{
		TEntityRespawnDataMap::iterator dit=m_respawndata.find(entityId);
		if (dit!=m_respawndata.end())
			m_respawndata.erase(dit);
	}
}

//------------------------------------------------------------------------
void CGameRules::ScheduleEntityRemoval(EntityId entityId, float timer, bool visibility)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	SEntityRemovalData removal;
	removal.time = timer;
	removal.timer = timer;
	removal.visibility = visibility;

	m_removals.insert(TEntityRemovalMap::value_type(entityId, removal));
}

//------------------------------------------------------------------------
void CGameRules::AbortEntityRemoval(EntityId entityId)
{
	TEntityRemovalMap::iterator it=m_removals.find(entityId);
	if (it!=m_removals.end())
		m_removals.erase(it);
}

//------------------------------------------------------------------------

void CGameRules::SendRadioMessage(const EntityId sourceId,const int msg)
{
	/*g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId()*/
	RadioMessageParams params(sourceId,msg);

	if(gEnv->bServer)
	{
		if(GetTeamCount()>1)//team DM or PS
		{
			int teamId = GetTeam(sourceId);
			if (teamId)
			{
				TPlayerTeamIdMap::const_iterator tit=m_playerteams.find(teamId);
				if (tit!=m_playerteams.end())
				{
					for (TPlayers::const_iterator it=tit->second.begin(); it!=tit->second.end(); ++it)
						GetGameObject()->InvokeRMIWithDependentObject(ClRadioMessage(), params, eRMI_ToClientChannel, *it, GetChannelId(*it));
				}
			}
		}
		else
			GetGameObject()->InvokeRMI(ClRadioMessage(), params, eRMI_ToAllClients);
	}
	else
		GetGameObject()->InvokeRMI(SvRequestRadioMessage(),params,eRMI_ToServer);
}

void CGameRules::OnRadioMessage(const EntityId sourceId,const int msg)
{
	//CryLog("[radio] from: %s message: %d",,msg);
	m_pRadio->OnRadioMessage(msg,GetActorNameByEntityId(sourceId));
}

void CGameRules::RadioMessageParams::SerializeWith(TSerialize ser)
{
	ser.Value("source",sourceId,'eid');
	ser.Value("msg",msg,'ui8');
}

void CGameRules::OnAction(const ActionId& actionId, int activationMode, float value)
{
	if(m_pRadio)
		m_pRadio->OnAction(actionId,activationMode,value);
}

void CGameRules::ReconfigureVoiceGroups(EntityId id,int old_team,int new_team)
{
	INetContext *pNetContext = g_pGame->GetIGameFramework()->GetNetContext();
	if(!pNetContext)
		return;

	IVoiceContext *pVoiceContext = pNetContext->GetVoiceContext();
	if(!pVoiceContext)
		return; // voice context is now disabled in single player game. talk to me if there are any problems - Lin

	if(old_team==new_team)	
		return;

	TTeamIdVoiceGroupMap::iterator iter=m_teamVoiceGroups.find(old_team);
	if(iter!=m_teamVoiceGroups.end())
		iter->second->RemoveEntity(id);

	iter=m_teamVoiceGroups.find(new_team);
	if(iter==m_teamVoiceGroups.end())
	{
		IVoiceGroup* pVoiceGroup=pVoiceContext->CreateVoiceGroup();
		iter=m_teamVoiceGroups.insert(std::make_pair(new_team,pVoiceGroup)).first;
	}
	iter->second->AddEntity(id);
}

CMPTutorial* CGameRules::GetMPTutorial() const
{
	return m_pMPTutorial;
}

void CGameRules::PlayerPosForRespawn(CPlayer* pPlayer, bool save)
{
	static 	Matrix34	respawnPlayerTM(IDENTITY);
	if (save)
	{
		respawnPlayerTM = pPlayer->GetEntity()->GetWorldTM();
	}
	else
	{
		pPlayer->GetEntity()->SetWorldTM(respawnPlayerTM);
	}
}

void CGameRules::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_channelIds);
	s->AddContainer(m_teams);
	s->AddContainer(m_entityteams);
	s->AddContainer(m_teamdefaultspawns);
	s->AddContainer(m_playerteams);
	s->AddContainer(m_hitMaterials);
	s->AddContainer(m_hitTypes);
	s->AddContainer(m_respawndata);
	s->AddContainer(m_respawns);
	s->AddContainer(m_removals);
	s->AddContainer(m_minimap);
	s->AddContainer(m_objectives);
	s->AddContainer(m_spawnLocations);
	s->AddContainer(m_spawnGroups);
	s->AddContainer(m_hitListeners);
	s->AddContainer(m_teamVoiceGroups);
	s->AddContainer(m_rulesListeners);

	for (TTeamIdMap::iterator iter = m_teams.begin(); iter != m_teams.end(); ++iter)
		s->Add(iter->first);
	for (TPlayerTeamIdMap::iterator iter = m_playerteams.begin(); iter != m_playerteams.end(); ++iter)
		s->AddContainer(iter->second);
	for (THitTypeMap::iterator iter = m_hitTypes.begin(); iter != m_hitTypes.end(); ++iter)
		s->Add(iter->second);
	for (TTeamObjectiveMap::iterator iter = m_objectives.begin(); iter != m_objectives.end(); ++iter)
		s->AddContainer(iter->second);
	for (TSpawnGroupMap::iterator iter = m_spawnGroups.begin(); iter != m_spawnGroups.end(); ++iter)
		s->AddContainer(iter->second);
}
