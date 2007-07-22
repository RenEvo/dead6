////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// CD6GameFactory.h
//
// Purpose: Dead6 Core Game factory for registering
//	our own gamerules and factory classes
//
// Note: Keep up-to-date with changes from GameFactory.cpp!
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CD6GAMEFACTORY_H_
#define _D6C_CD6GAMEFACTORY_H_

////////////////////////////////////////////////////
// InitD6GameFactory
//
// Purpose: Init the D6 factories and gamerules
//
// In:	pFramework - Framework to use
//
// Note: Called from CD6Game::Init()
////////////////////////////////////////////////////
void InitD6GameFactory(struct IGameFramework *pFramework);


#endif //_D6C_CD6GAMEFACTORY_H_