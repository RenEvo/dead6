/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: HUD score board for multiplayer

-------------------------------------------------------------------------
History:
- 27:03:2006: Created by Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "HUDObject.h"
#include "HUDScore.h"
#include "IGameFramework.h"
#include "IUIDraw.h"
#include "Game.h"
#include "GameRules.h"
#include "GameFlashAnimation.h"
#include "HUDCommon.h"

CHUDScore::ScoreEntry::ScoreEntry(EntityId id, int kills, int deaths, int ping): m_entityId(id), m_team(-1)
{
	m_kills = kills;
	m_deaths = deaths;
	m_ping = ping;

	if(int team = g_pGame->GetGameRules()->GetTeam(id))
		m_team = team;
}

bool CHUDScore::ScoreEntry::operator<(const ScoreEntry& entry) const
{
	if(m_team >= 0 && m_team != entry.m_team)
	{
		if(m_team > entry.m_team)
			return false;
		return true;
	}
	else
	{
		if(entry.m_kills > m_kills)
			return false;
		else if(m_kills > entry.m_kills)
			return true;
		else
		{
			if(m_deaths < entry.m_deaths)
				return true;
			else if (m_deaths > entry.m_deaths)
				return false;
			else
			{
				IEntity *pEntity0=gEnv->pEntitySystem->GetEntity(m_entityId);
				IEntity *pEntity1=gEnv->pEntitySystem->GetEntity(entry.m_entityId);

				const char *name0=pEntity0?pEntity0->GetName():"";
				const char *name1=pEntity1?pEntity1->GetName():"";

				if (strcmp(name0, name1)<0)
					return true;
				else
					return false;
			}
		}
		return true;
	}
}

CHUDScore::CHUDScore()
{
	m_bShow = false;
	m_lastUpdate = 0;
	m_lastShowSwitch = 0;
	m_pFlashBoard = NULL;
	m_currentClientTeam = -1;
}

void CHUDScore::OnUpdate(float fDeltaTime,float fFadeValue)
{
	if(m_bShow && gEnv->bMultiplayer)
		Render();
	else
		m_currentClientTeam = -1;
}

void CHUDScore::Reset()
{
	m_scoreBoard.clear();
}

void CHUDScore::AddEntry(EntityId player, int kills, int deaths, int ping)
{
	//doesn't check for existing entries atm. (scoreboard deleted every frame)
	m_scoreBoard.push_back(ScoreEntry(player, kills, deaths, ping));
}

void CHUDScore::Render()
{
	if(!m_pFlashBoard)
		return;

	int lastTeam = -1;
	int clientTeamPoints = 0;
	int enemyTeamPoints = 0;

	CGameRules *pGameRules=g_pGame->GetGameRules();
	IScriptTable *pGameRulesScript=pGameRules->GetEntity()->GetScriptTable();
	int ppKey = 0;
	if(pGameRulesScript)
		pGameRulesScript->GetValue("PP_AMOUNT_KEY", ppKey);

	std::sort(m_scoreBoard.begin(), m_scoreBoard.end());

	//clear the teams stats in flash
	//m_pFlashBoard->Invoke("clearTable");

	std::vector<ScoreEntry>::iterator it;
	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if(!pClientActor)
		return;

	int clientTeam = pGameRules->GetTeam(pClientActor->GetEntityId());
	int teamCount = pGameRules->GetTeamCount();
	if(teamCount > 1)
	{
		if(clientTeam != 1 && clientTeam != 2)		//Spectate mode
			clientTeam = 1;

		if(clientTeam != m_currentClientTeam)
		{
			m_pFlashBoard->CheckedInvoke("setClientTeam", (clientTeam == 1)? "korean" : "us");
			m_currentClientTeam = clientTeam;
		}

		//get the player rank, next rank, CP etc. from gamerules script
		int currentRank = 0;
		HSCRIPTFUNCTION pfnFuHelper = 0;
		if (pGameRulesScript->GetValue("GetPlayerRank", pfnFuHelper) && pfnFuHelper)
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnFuHelper, pGameRulesScript, ScriptHandle(pClientActor->GetEntityId()), currentRank);
			gEnv->pScriptSystem->ReleaseFunc(pfnFuHelper);
		}
		if(currentRank)
		{
			const char* nextRankName = 0;
			int currentRankCP = 0;
			int nextRankCP = 0;
			int playerCP = 0;
			if (pGameRulesScript->GetValue("GetRankName", pfnFuHelper) && pfnFuHelper)
			{
				Script::CallReturn(gEnv->pScriptSystem, pfnFuHelper, pGameRulesScript, currentRank+1, nextRankName);
				gEnv->pScriptSystem->ReleaseFunc(pfnFuHelper);
			}
			if(nextRankName)
			{
				if (pGameRulesScript->GetValue("GetPlayerCP", pfnFuHelper) && pfnFuHelper)
				{
					Script::CallReturn(gEnv->pScriptSystem, pfnFuHelper, pGameRulesScript, ScriptHandle(pClientActor->GetEntityId()), playerCP);
					gEnv->pScriptSystem->ReleaseFunc(pfnFuHelper);
				}

				if(pGameRulesScript->GetValue("GetRankCP", pfnFuHelper) && pfnFuHelper)
				{
					Script::CallReturn(gEnv->pScriptSystem, pfnFuHelper, pGameRulesScript, currentRank, currentRankCP);
					Script::CallReturn(gEnv->pScriptSystem, pfnFuHelper, pGameRulesScript, currentRank+1, nextRankCP);
					gEnv->pScriptSystem->ReleaseFunc(pfnFuHelper);

					//set values to flash
					SFlashVarValue args[2] = {nextRankName, 100.0f * ((playerCP - currentRankCP) / float(nextRankCP - currentRankCP))};
					m_pFlashBoard->Invoke("setClientRankProgress", args, 2);
				}
			}
		}
	}

	for(it = m_scoreBoard.begin(); it != m_scoreBoard.end(); ++it)
	{
		ScoreEntry player = *it;

		IEntity *pPlayer=gEnv->pEntitySystem->GetEntity(player.m_entityId);
		//assert(pPlayer);
		if (!pPlayer)
			continue;

		if((teamCount > 1) && (player.m_team != 1 && player.m_team != 2))
			continue;

		if(lastTeam != player.m_team)	//get next team
		{
			lastTeam = player.m_team;

			if (pGameRulesScript)
			{
				int key0;
				if (pGameRulesScript->GetValue("TEAMSCORE_TEAM0_KEY", key0))
				{
					int teamScore=0;
					pGameRules->GetSynchedGlobalValue(key0+player.m_team, teamScore);

					if(lastTeam == clientTeam)
						clientTeamPoints = teamScore;
					else
						enemyTeamPoints = teamScore;
				}
			}
		}

		const char* rank = 0;
		HSCRIPTFUNCTION pfnGetPlayerRank = 0;
		if (pGameRulesScript->GetValue("GetPlayerRankName", pfnGetPlayerRank) && pfnGetPlayerRank)
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnGetPlayerRank, pGameRulesScript, ScriptHandle(player.m_entityId), rank);
			gEnv->pScriptSystem->ReleaseFunc(pfnGetPlayerRank);
		}

		int playerPP = 0;
		bool alive=true;
		if (IActor *pActor=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(player.m_entityId))
			alive=pActor->GetHealth()>0;
		pGameRules->GetSynchedEntityValue(player.m_entityId, TSynchedKey(ppKey), playerPP);
		SFlashVarValue args[10] = {pPlayer->GetName(), (player.m_team == clientTeam)?1:2, (player.m_entityId==pClientActor->GetEntityId())?true:false, playerPP, player.m_kills, player.m_deaths, player.m_ping, player.m_entityId, rank?rank:" ", alive?0:1};
		m_pFlashBoard->Invoke("addEntry", args, 10);
	}

	//set the teams scores in flash
	{
		SFlashVarValue argsA[2] = {(clientTeam==1)?1:2, enemyTeamPoints};
		m_pFlashBoard->CheckedInvoke("setTeamPoints", argsA, 2);
		SFlashVarValue argsB[2] = {(clientTeam==1)?2:1, clientTeamPoints};
		m_pFlashBoard->CheckedInvoke("setTeamPoints", argsB, 2);
	}

	EntityId playerBeingKicked = 0;
	m_pFlashBoard->Invoke("drawAllEntries", playerBeingKicked);
}

void CHUDScore::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	if(m_pFlashBoard)
		m_pFlashBoard->GetMemoryStatistics(s);
	s->AddContainer(m_scoreBoard);
}
