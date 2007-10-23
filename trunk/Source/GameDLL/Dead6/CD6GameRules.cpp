////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CD6GameRules.cpp
//
// Purpose: Dead6 Core Game rules
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CD6GameRules.h"

////////////////////////////////////////////////////
CD6GameRules::CD6GameRules(void)
{

}

////////////////////////////////////////////////////
CD6GameRules::~CD6GameRules(void)
{

}

////////////////////////////////////////////////////
void CD6GameRules::ClearAllTeams(void)
{
	// Clear the team map
	//m_teams.clear();

	// Clear the player teams map
	//m_playerteams.clear();

	// Set all teamed entities to 0 (no team)
	/*for (TEntityTeamIdMap::iterator itI = m_entityteams.begin(); itI != m_entityteams.end(); itI++)
	{
		itI->second = 0;
	}*/
}

////////////////////////////////////////////////////
int CD6GameRules::CreateTeam(const char *name)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->CreateTeam(name);
}

////////////////////////////////////////////////////
void CD6GameRules::RemoveTeam(int teamId)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->RemoveTeam(teamId);
}

////////////////////////////////////////////////////
const char *CD6GameRules::GetTeamName(int teamId) const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamName(teamId);
}

////////////////////////////////////////////////////
int CD6GameRules::GetTeamId(const char *name) const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamId(name);
}

////////////////////////////////////////////////////
int CD6GameRules::GetTeamCount() const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamCount();
}

////////////////////////////////////////////////////
int CD6GameRules::GetTeamPlayerCount(int teamId, bool inGame) const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamPlayerCount(teamId, inGame);
}

////////////////////////////////////////////////////
EntityId CD6GameRules::GetTeamPlayer(int teamId, int idx)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamPlayer(teamId, idx);
}

////////////////////////////////////////////////////
void CD6GameRules::GetTeamPlayers(int teamId, TPlayers &players)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->GetTeamPlayers(teamId, players);
}

////////////////////////////////////////////////////
void CD6GameRules::SetTeam(int teamId, EntityId entityId)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->SetTeam(teamId, entityId);

	// Notify script
	ScriptHandle handle(entityId);
	CallScript(GetServerStateScript(), "OnSetTeam", handle, teamId);
	if (gEnv->bClient)
	{
		ScriptHandle handle(entityId);
		CallScript(GetClientStateScript(), "OnSetTeam", handle, teamId);
	}

	//if this is a spawn group, update it's validity
	if (m_spawnGroups.find(entityId)!=m_spawnGroups.end())
		CheckSpawnGroupValidity(entityId);

	GetGameObject()->InvokeRMIWithDependentObject(ClSetTeam(), SetTeamParams(entityId, teamId), eRMI_ToRemoteClients, entityId);

	if (IEntity *pEntity=m_pEntitySystem->GetEntity(entityId))
		m_pGameplayRecorder->Event(pEntity, GameplayEvent(eGE_ChangedTeam, 0, (float)teamId));
}

////////////////////////////////////////////////////
int CD6GameRules::GetTeam(EntityId entityId) const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeam(entityId);
}

////////////////////////////////////////////////////
void CD6GameRules::SetTeamDefaultSpawnGroup(int teamId, EntityId spawnGroupId)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	g_D6Core->pTeamManager->SetTeamDefaultSpawnGroup(teamId, spawnGroupId);
}

////////////////////////////////////////////////////
EntityId CD6GameRules::GetTeamDefaultSpawnGroup(int teamId)
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamDefaultSpawnGroup(teamId);
}

////////////////////////////////////////////////////
int CD6GameRules::GetChannelTeam(int channelId) const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetChannelTeam(channelId);
}

////////////////////////////////////////////////////
int CD6GameRules::GetTeamChannelCount(int teamId, bool inGame) const
{
	// Delegate to team manager
	assert(g_D6Core->pTeamManager);
	return g_D6Core->pTeamManager->GetTeamChannelCount(teamId, inGame);
}

////////////////////////////////////////////////////
void CD6GameRules::ClientHit(const HitInfo &hitInfo)
{
	// Base call
	CGameRules::ClientHit(hitInfo);

	// Let base manager handle it for controllers
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->ClientHit(hitInfo);
}

////////////////////////////////////////////////////
void CD6GameRules::ServerHit(const HitInfo &hitInfo)
{
	// Base call
	CGameRules::ServerHit(hitInfo);

	// Let base manager handle it for controllers
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->ServerHit(hitInfo);
}

////////////////////////////////////////////////////
void CD6GameRules::ServerExplosion(const ExplosionInfo &explosionInfo)
{
	// Base call
	CGameRules::ServerExplosion(explosionInfo);

	// Get affected entities
	TExplosionAffectedEntities affectedEntities;
	pe_explosion explosion;
	explosion.epicenter = explosionInfo.pos;
	explosion.rmin = explosionInfo.minRadius;
	explosion.rmax = (0 == explosionInfo.radius ? 0.0001f : explosionInfo.radius);
	explosion.r = explosion.rmin;
	explosion.impulsivePressureAtR = explosionInfo.pressure;
	explosion.epicenterImp = explosionInfo.pos;
	explosion.explDir = explosionInfo.dir;
	explosion.nGrow = 2;
	explosion.rminOcc = 0.07f;
	explosion.holeSize = 0.0f;
	explosion.nOccRes = (explosion.rmax > 50.0f ? 0 : 32);
	gEnv->pPhysicalWorld->SimulateExplosion(&explosion, 0, 0, ent_static);
	UpdateAffectedEntitiesSet(affectedEntities, &explosion);

	// Let base manager handle it for controllers
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->ServerExplosion(explosionInfo, affectedEntities);
}

////////////////////////////////////////////////////
void CD6GameRules::ClientExplosion(const ExplosionInfo &explosionInfo)
{
	// Base call
	CGameRules::ClientExplosion(explosionInfo);

	// Get affected entities
	TExplosionAffectedEntities affectedEntities;
	pe_explosion explosion;
	explosion.epicenter = explosionInfo.pos;
	explosion.rmin = explosionInfo.minRadius;
	explosion.rmax = (0 == explosionInfo.radius ? 0.0001f : explosionInfo.radius);
	explosion.r = explosion.rmin;
	explosion.impulsivePressureAtR = explosionInfo.pressure;
	explosion.epicenterImp = explosionInfo.pos;
	explosion.explDir = explosionInfo.dir;
	explosion.nGrow = 2;
	explosion.rminOcc = 0.07f;
	explosion.holeSize = 0.0f;
	explosion.nOccRes = (explosion.rmax > 50.0f ? 0 : 32);
	gEnv->pPhysicalWorld->SimulateExplosion(&explosion, 0, 0, ent_static);
	UpdateAffectedEntitiesSet(affectedEntities, &explosion);

	// Let base manager handle it for controllers
	assert(g_D6Core->pBaseManager);
	g_D6Core->pBaseManager->ClientExplosion(explosionInfo, affectedEntities);
}