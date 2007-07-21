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

class CD6Game : public CGame
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

public:
	////////////////////////////////////////////////////
	virtual bool  Init(IGameFramework *pFramework);
	virtual bool  CompleteInit();
	virtual void  Shutdown();
	virtual int   Update(bool haveFocus, unsigned int updateFlags);
};

#endif //_D6C_CD6GAME_H_