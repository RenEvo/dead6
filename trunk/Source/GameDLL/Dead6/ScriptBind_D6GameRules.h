////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_D6GameRules.h
//
// Purpose: Dead6 Core GameRules script binding, replaces
//	ScriptBind_GameRules for D6-specific GameRules controls
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_SCRIPTBIND_D6GAMERULES_H_
#define _D6C_SCRIPTBIND_D6GAMERULES_H_

#include "ScriptBind_GameRules.h"
#include "CD6GameRules.h"

class CScriptBind_D6GameRules : public CScriptBind_GameRules
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CScriptBind_D6GameRules(ISystem *pSystem, IGameFramework *pGameFramework);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CScriptBind_D6GameRules();

	////////////////////////////////////////////////////
	// IsEditor
	//
	// Purpose: Returns if the game is in the Editor
	//	or not
	////////////////////////////////////////////////////
	virtual int IsEditor(IFunctionHandler *pH);

	////////////////////////////////////////////////////
	// IsInEditorGame
	//
	// Purpose: Returns if the game in the Editor is
	//	playing out or not
	////////////////////////////////////////////////////
	virtual int IsInEditorGame(IFunctionHandler *pH);

protected:
	////////////////////////////////////////////////////
	// GetD6GameRules
	//
	// Purpose: Get the D6 GameRules using attached gr
	////////////////////////////////////////////////////
	CD6GameRules *GetD6GameRules(IFunctionHandler *pH);
	
	IGameFramework *m_pGameFW;
};

#endif //_D6C_SCRIPTBIND_D6GAMERULES_H_