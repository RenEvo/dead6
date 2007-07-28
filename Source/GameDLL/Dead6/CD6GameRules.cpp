////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
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
	m_teams.clear();

	// Clear the player teams map
	m_playerteams.clear();

	// Set all teamed entities to 0 (no team)
	for (TEntityTeamIdMap::iterator itI = m_entityteams.begin(); itI != m_entityteams.end(); itI++)
	{
		itI->second = 0;
	}
}