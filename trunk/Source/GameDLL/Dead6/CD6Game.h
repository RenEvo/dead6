////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CD6Game.h
//
// Purpose: Dead6 Core Game class
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CD6GAME_H_
#define _D6C_CD6GAME_H_

#include "Game.h"
#include "ILevelSystem.h"
#include "ISystem.h"

#define D6GAME_NAME			"CNC:TD6"
#define D6GAME_LONGNAME		"Command & Crysis: The Dead 6"

class CScriptBind_BaseManager;
class CScriptBind_TeamManager;

class CD6GameRules;

////////////////////////////////////////////////////
class CD6Game : public CGame, public ILevelSystemListener
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6Game(void);
private:
	CD6Game(CD6Game const&) {}
	CD6Game& operator =(CD6Game const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6Game(void);

	////////////////////////////////////////////////////
	// GetD6GameRules
	//
	// Purpose: Returns OUR game rules
	////////////////////////////////////////////////////
	virtual CD6GameRules *GetD6GameRules() const;

public:
	////////////////////////////////////////////////////
	// GetBaseManagerScriptBind
	//
	// Purpose: Returns the script bind for the base manager
	////////////////////////////////////////////////////
	virtual CScriptBind_BaseManager *GetBaseManagerScriptBind() { return m_pScriptBindBaseManager; }

	////////////////////////////////////////////////////
	// GetTeamManagerScriptBind
	//
	// Purpose: Returns the script bind for the team manager
	////////////////////////////////////////////////////
	virtual CScriptBind_TeamManager *GetTeamManagerScriptBind() { return m_pScriptBindTeamManager; }


	////////////////////////////////////////////////////
	// CGame overloads
	virtual void GetMemoryStatistics(ICrySizer *s);
	virtual bool Init(IGameFramework *pFramework);
	virtual bool CompleteInit();
	virtual void Shutdown();
	virtual int Update(bool haveFocus, unsigned int updateFlags);
	virtual const char *GetLongName();
	virtual const char *GetName();
	virtual void EditorResetGame(bool bStart);
	
	////////////////////////////////////////////////////
	// ILevelSystemListener overloads
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLevelNotFound(const char *levelName) {}
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) {}
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {}

protected:
	////////////////////////////////////////////////////
	// ParseCNCRules_General
	//
	// Purpose: Parses the general settings of the CNC
	//	rules file
	//
	// In:	pNode - XML node containing general settings
	////////////////////////////////////////////////////
	virtual void ParseCNCRules_General(XmlNodeRef &pNode);

	////////////////////////////////////////////////////
	// ParseCNCRules_Teams
	//
	// Purpose: Parses the team settings of the CNC
	//	rules file
	//
	// In:	pNode - XML node containing team settings
	////////////////////////////////////////////////////
	virtual void ParseCNCRules_Teams(XmlNodeRef &pNode);

	////////////////////////////////////////////////////
	// ParseCNCRules_Buildings
	//
	// Purpose: Parses the buildings settings of the CNC
	//	rules file
	//
	// In:	pNode - XML node containing building settings
	////////////////////////////////////////////////////
	virtual void ParseCNCRules_Buildings(XmlNodeRef &pNode);

	////////////////////////////////////////////////////
	// CGame overloads
	virtual void InitScriptBinds();
	virtual void ReleaseScriptBinds();

	CScriptBind_BaseManager *m_pScriptBindBaseManager;
	CScriptBind_TeamManager *m_pScriptBindTeamManager;
};

#endif //_D6C_CD6GAME_H_