////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_TeamManager.h
//
// Purpose: Script binding for the team manager
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_SCRIPTBIND_TEAMMANAGER_H_
#define _D6C_SCRIPTBIND_TEAMMANAGER_H_

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct ITeamManager;
struct ISystem;

class CScriptBind_TeamManager : public CScriptableBase
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CScriptBind_TeamManager(ISystem *pSystem);
private:
	CScriptBind_TeamManager(CScriptBind_TeamManager const&) {}
	CScriptBind_TeamManager& operator =(CScriptBind_TeamManager const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CScriptBind_TeamManager(void);

	////////////////////////////////////////////////////
	// AttachTo
	//
	// Purpose: Attaches binding to a team manager
	////////////////////////////////////////////////////
	virtual void AttachTo(ITeamManager *pTeamManager);

public:
	////////////////////////////////////////////////////
	// SetEditorTeam
	//
	// Purpose: Set the team to use in the editor game
	//
	// In:	nTeamID - Team ID
	////////////////////////////////////////////////////
	virtual int SetEditorTeam(IFunctionHandler *pH, int nTeamID);

	////////////////////////////////////////////////////
	// SetEditorTeamByName
	//
	// Purpose: Set the team to use in the editor game
	//		using the name
	//
	// In:	szTeam - Team name
	////////////////////////////////////////////////////
	virtual int SetEditorTeamByName(IFunctionHandler *pH, char const* szTeam);

	////////////////////////////////////////////////////
	// SetTeamCredits
	//
	// Purpose: Set everyone on the team's credits
	//
	// In:	nTeamID - team ID
	//		nAmount - Amount to set
	////////////////////////////////////////////////////
	virtual int SetTeamCredits(IFunctionHandler *pH, int nTeamID, unsigned int nAmount);

	////////////////////////////////////////////////////
	// GiveTeamCredits
	//
	// Purpose: Give everyone on the team credits
	//
	// In:	nTeamID - team ID
	//		nAmount - Amount to give
	////////////////////////////////////////////////////
	virtual int GiveTeamCredits(IFunctionHandler *pH, int nTeamID, unsigned int nAmount);

	////////////////////////////////////////////////////
	// TakeTeamCredits
	//
	// Purpose: Take from everyone on the team's credits
	//
	// In:	nTeamID - team ID
	//		nAmount - Amount to take
	////////////////////////////////////////////////////
	virtual int TakeTeamCredits(IFunctionHandler *pH, int nTeamID, unsigned int nAmount);

protected:
	////////////////////////////////////////////////////
	// RegisterGlobals
	//
	// Purpose: Registers any global values to the script
	//	system
	////////////////////////////////////////////////////
	void RegisterGlobals(void);

	////////////////////////////////////////////////////
	// RegisterMethods
	//
	// Purpose: Registers any binding methods to the script
	//	system
	////////////////////////////////////////////////////
	void RegisterMethods(void);

	ITeamManager *m_pTeamManager;
	ISystem *m_pSystem;
	IScriptSystem *m_pSS;
};

#endif //_D6C_SCRIPTBIND_TEAMMANAGER_H_
