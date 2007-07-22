////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
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

#define D6GAME_NAME			"CNC:TD6"
#define D6GAME_LONGNAME		"Command & Crysis: The Dead 6"

class CScriptBind_BaseManager;
class CScriptBind_TeamManager;

////////////////////////////////////////////////////
// Level listener for gamerules
class CD6GameLevelListener : public ILevelSystemListener
{
	IGame *m_pGame;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6GameLevelListener(IGame *pD6Game);

	////////////////////////////////////////////////////
	virtual void OnLevelNotFound(const char *levelName);
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error);
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount);
};

////////////////////////////////////////////////////
class CD6Game : public CGame
{
	friend class CD6GameLevelListener;
	CD6GameLevelListener *m_pLevelListener;

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

public:
	////////////////////////////////////////////////////
	virtual bool Init(IGameFramework *pFramework);
	virtual bool CompleteInit();
	virtual void Shutdown();
	virtual int Update(bool haveFocus, unsigned int updateFlags);
	virtual const char *GetLongName();
	virtual const char *GetName();

	////////////////////////////////////////////////////
	virtual CScriptBind_BaseManager *GetBaseManagerScriptBind() { return m_pScriptBindBaseManager; }
	virtual CScriptBind_TeamManager *GetTeamManagerScriptBind() { return m_pScriptBindTeamManager; }

protected:
	////////////////////////////////////////////////////
	virtual void InitScriptBinds();
	virtual void ReleaseScriptBinds();

	CScriptBind_BaseManager *m_pScriptBindBaseManager;
	CScriptBind_TeamManager *m_pScriptBindTeamManager;
};

#endif //_D6C_CD6GAME_H_