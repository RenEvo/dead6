////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CD6Game.h
//
// Purpose: Dead6 Core Player declarations
//
// File History:
//	- 7/22/07 : File created - Dan
////////////////////////////////////////////////////

#ifndef _D6C_D6PLAYER_H_
#define _D6C_D6PLAYER_H_

#include "Player.h"

class CD6Player : public CPlayer
{
private:
	CD6Player& operator=(CD6Player const&) { return *this; }
	CD6Player(CD6Player const&){}

protected:
	int32 m_Credits;

public:
	CD6Player();
	virtual ~CD6Player();

	virtual void SetCredits(int32 credits) { m_Credits = credits; }
	virtual int32 GetCredits() const { return m_Credits; }
	virtual void GiveCredits(int32 credits) { m_Credits += credits; }
	virtual void TakeCredits(int32 credits) { m_Credits -= credits; }
};

#endif //_D6C_D6PLAYER_H_