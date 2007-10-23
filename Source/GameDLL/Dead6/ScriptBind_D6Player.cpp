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

#include "Stdafx.h"
#include "ScriptBind_D6Player.h"

////////////////////////////////////////////////////
CScriptBind_D6Player::CScriptBind_D6Player(ISystem *pSystem) :
	CScriptBind_Actor(pSystem)
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_D6Player::
	
	SCRIPT_REG_TEMPLFUNC(SetCredits, "nCredits");
	SCRIPT_REG_FUNC(GetCredits);
	SCRIPT_REG_TEMPLFUNC(GiveCredits, "nCredits");
	SCRIPT_REG_TEMPLFUNC(TakeCredits, "nCredits");
}

////////////////////////////////////////////////////
CScriptBind_D6Player::~CScriptBind_D6Player()
{

}

////////////////////////////////////////////////////
CD6Player *CScriptBind_D6Player::GetD6Player(IFunctionHandler *pH)
{
	// Get the actor
	CActor *pActor = GetActor(pH);
	if (NULL == pActor) return NULL;

	// Cast to D6Player
	return (CD6Player*)(pActor);
}

////////////////////////////////////////////////////
int CScriptBind_D6Player::SetCredits(IFunctionHandler *pH, int nCredits)
{
	CD6Player *pPlayer = GetD6Player(pH);
	if (NULL != pPlayer)
	{
		pPlayer->SetCredits(nCredits);
		return pH->EndFunction(pPlayer->GetCredits());
	}
	return pH->EndFunction(0);
}

////////////////////////////////////////////////////
int CScriptBind_D6Player::GetCredits(IFunctionHandler *pH)
{
	CD6Player *pPlayer = GetD6Player(pH);
	if (NULL != pPlayer) 
		return pH->EndFunction(pPlayer->GetCredits());
	return pH->EndFunction(0);
}

////////////////////////////////////////////////////
int CScriptBind_D6Player::GiveCredits(IFunctionHandler *pH, int nCredits)
{
	CD6Player *pPlayer = GetD6Player(pH);
	if (NULL != pPlayer) 
	{
		pPlayer->GiveCredits(nCredits); 
		return pH->EndFunction(pPlayer->GetCredits());
	}
	return pH->EndFunction(0);
}

////////////////////////////////////////////////////
int CScriptBind_D6Player::TakeCredits(IFunctionHandler *pH, int nCredits)
{
	CD6Player *pPlayer = GetD6Player(pH);
	if (NULL != pPlayer) 
	{
		pPlayer->TakeCredits(nCredits); 
		return pH->EndFunction(pPlayer->GetCredits());
	}
	return pH->EndFunction(0);
}