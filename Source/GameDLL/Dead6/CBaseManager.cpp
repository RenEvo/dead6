////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CBaseManager.cpp
//
// Purpose: Monitors building logic on the field
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CBaseManager.h"
#include "CTeamManager.h"

////////////////////////////////////////////////////
CBaseManager::CBaseManager(void)
{

}

////////////////////////////////////////////////////
CBaseManager::~CBaseManager(void)
{

}

////////////////////////////////////////////////////
void CBaseManager::SetTeamManager(ITeamManager const* pTM)
{

}

////////////////////////////////////////////////////
ITeamManager const* CBaseManager::GetTeamManager(void) const
{
	return NULL;
}

////////////////////////////////////////////////////
BuildingClassID CBaseManager::CreateTeam(char const* szName, char const* szScript)
{
	return BC_INVALID;
}

////////////////////////////////////////////////////
void CBaseManager::Reset(void)
{

}

////////////////////////////////////////////////////
SBuildingClassDef const* CBaseManager::GetBuildingClassByName(char const* szName) const
{
	return NULL;
}

////////////////////////////////////////////////////
SBuildingClassDef const* CBaseManager::GetBuildingClassByID(BuildingClassID const& nID) const
{
	return NULL;
}