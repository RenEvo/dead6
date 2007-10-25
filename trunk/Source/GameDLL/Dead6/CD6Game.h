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
class CScriptBind_BuildingController;
class CScriptBind_PortalManager;
class CScriptBind_D6Player;
class CScriptBind_D6GameRules;

class CD6GameRules;

////////////////////////////////////////////////////
class CD6Game : public CGame, public ILevelSystemListener
{
	// TRUE if the game in the editor is going
	bool m_bEditorGameStarted;

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

	////////////////////////////////////////////////////
	// IsEditorGameStarted
	//
	// Purpose: Returns TRUE if the game in the editor
	//	has started
	////////////////////////////////////////////////////
	virtual bool IsEditorGameStarted(void) const;

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
	// GetBuildingControllerScriptBind
	//
	// Purpose: Returns the script bind for the
	//	building controller
	////////////////////////////////////////////////////
	virtual CScriptBind_BuildingController *GetBuildingControllerScriptBind() { return m_pScriptBindBuildingController; }

	////////////////////////////////////////////////////
	// GetPortalManagerScriptBind
	//
	// Purpose: Returns the script bind for the portal manager
	////////////////////////////////////////////////////
	virtual CScriptBind_PortalManager *GetPortalManagerScriptBind() { return m_pScriptBindPortalManager; }

	////////////////////////////////////////////////////
	// GetActorScriptBind
	//
	// Purpose: Returns the D6 Player script bind overload
	////////////////////////////////////////////////////
	virtual CScriptBind_Actor *GetActorScriptBind();

	////////////////////////////////////////////////////
	// GetGameRulesScriptBind
	//
	// Purpose: Returns the D6 GameRules script bind overload
	////////////////////////////////////////////////////
	virtual CScriptBind_GameRules *GetGameRulesScriptBind();

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
	// ParseCNCRules
	//
	// Purpose: Parses the specified CNCRules file
	//
	// In:	szXMLFile - XML File to load
	//
	// Returns TRUE on success or FALSE if an error occured
	////////////////////////////////////////////////////
	bool ParseCNCRules(char const* szXMLFile);

	////////////////////////////////////////////////////
	// CGame overloads
	virtual void InitScriptBinds();
	virtual void ReleaseScriptBinds();

	CScriptBind_BaseManager *m_pScriptBindBaseManager;
	CScriptBind_TeamManager *m_pScriptBindTeamManager;
	CScriptBind_BuildingController *m_pScriptBindBuildingController;
	CScriptBind_PortalManager *m_pScriptBindPortalManager;
	CScriptBind_D6Player *m_pScriptBindD6Player;
	CScriptBind_D6GameRules *m_pScriptBindD6GameRules;
};

#endif //_D6C_CD6GAME_H_