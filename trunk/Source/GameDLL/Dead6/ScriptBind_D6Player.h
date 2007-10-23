////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_D6Player.h
//
// Purpose: Dead6 Core Player script binding, replaces
//	ScriptBind_Actor for D6Player controls
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_SCRIPTBIND_D6PLAYER_H_
#define _D6C_SCRIPTBIND_D6PLAYER_H_

#include "ScriptBind_Actor.h"
#include "CD6Player.h"

class CScriptBind_D6Player : public CScriptBind_Actor
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CScriptBind_D6Player(ISystem *pSystem);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CScriptBind_D6Player();

	////////////////////////////////////////////////////
	// SetCredits
	//
	// Purpose: Set player's credits to amount
	//
	// In:	nCredits - Amount to set
	//
	// Returns player's credits amount
	////////////////////////////////////////////////////
	virtual int SetCredits(IFunctionHandler *pH, int nCredits);

	////////////////////////////////////////////////////
	// GetCredits
	//
	// Purpose: Returns player's credits amount
	////////////////////////////////////////////////////
	virtual int GetCredits(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// GiveCredits
	//
	// Purpose: Give player credits (+)
	//
	// In:	nCredits - Amount to add
	//
	// Returns player's credits amount
	////////////////////////////////////////////////////
	virtual int GiveCredits(IFunctionHandler *pH, int nCredits);

	////////////////////////////////////////////////////
	// TakeCredits
	//
	// Purpose: Take player credits (-)
	//
	// In:	nCredits - Amount to take
	//
	// Returns player's credits amount
	////////////////////////////////////////////////////
	virtual int TakeCredits(IFunctionHandler *pH, int nCredits);

protected:
	////////////////////////////////////////////////////
	// GetD6Player
	//
	// Purpose: Get the D6 player using attached actor
	////////////////////////////////////////////////////
	CD6Player *GetD6Player(IFunctionHandler *pH);
};

#endif //_D6C_SCRIPTBIND_D6PLAYER_H_