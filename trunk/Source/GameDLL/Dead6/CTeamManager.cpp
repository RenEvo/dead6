////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CTeamManager.cpp
//
// Purpose: Monitors team fluctuations and
//	callback scripts
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CTeamManager.h"

////////////////////////////////////////////////////
CTeamManager::CTeamManager(void)
{

}

////////////////////////////////////////////////////
CTeamManager::~CTeamManager(void)
{

}

////////////////////////////////////////////////////
TeamID CTeamManager::CreateTeam(char const* szName, char const* szScript)
{
	return TEAMID_INVALID;
}

////////////////////////////////////////////////////
void CTeamManager::Reset(void)
{

}

////////////////////////////////////////////////////
STeamDef const* CTeamManager::GetTeamByName(char const* szName) const
{
	return NULL;
}

////////////////////////////////////////////////////
STeamDef const* CTeamManager::GetTeamByID(TeamID const& nID) const
{
	return NULL;
}