////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CD6GameRules.h
//
// Purpose: Dead6 Core Game rules
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CD6GAMERULES_H_
#define _D6C_CD6GAMERULES_H_

#include "GameRules.h"

////////////////////////////////////////////////////
class CD6GameRules : public CGameRules
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6GameRules(void);
private:
	CD6GameRules(CD6GameRules const&) {}
	CD6GameRules& operator =(CD6GameRules const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6GameRules(void);

	////////////////////////////////////////////////////
	// ClearAllTeams
	//
	// Purpose: Clears all teams that are currently
	//	loaded in the gamerules and removes all
	//	actor/player team definitions that may be loaded
	////////////////////////////////////////////////////
	virtual void ClearAllTeams(void);
};

#endif //_D6C_CD6GAMERULES_H_