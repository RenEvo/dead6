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
//	- 8/23/07 : File edited - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CD6PLAYER_H_
#define _D6C_CD6PLAYER_H_

#include "Player.h"

class CD6Player : public CPlayer
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6Player(void);
private:
	CD6Player& operator=(CD6Player const&) { return *this; }
	CD6Player(CD6Player const&){}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6Player(void);
	
	// Serialization overloads
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void FullSerialize(TSerialize ser);
	virtual void Reset(bool toGame);

	// Class type
	static const char* GetActorClassType() { return "CD6Player"; }
	virtual const char* GetActorClass() const { return CD6Player::GetActorClassType(); }

	////////////////////////////////////////////////////
	// SetCredits
	//
	// Purpose: Set player's credits to amount
	//
	// In:	nCredits - Amount to set
	////////////////////////////////////////////////////
	virtual void SetCredits(unsigned int nCredits);

	////////////////////////////////////////////////////
	// GetCredits
	//
	// Purpose: Returns player's credits to amount
	////////////////////////////////////////////////////
	virtual unsigned int GetCredits() const;

	////////////////////////////////////////////////////
	// GiveCredits
	//
	// Purpose: Give player credits (+)
	//
	// In:	nCredits - Amount to add
	////////////////////////////////////////////////////
	virtual void GiveCredits(unsigned int nCredits);

	////////////////////////////////////////////////////
	// TakeCredits
	//
	// Purpose: Take player credits (-)
	//
	// In:	nCredits - Amount to take
	////////////////////////////////////////////////////
	virtual void TakeCredits(unsigned int nCredits);

public:
	// Net aspect used for updating credits
	static const int ASPECT_CREDITS	= eEA_GameServerStatic;

protected:
	// Player's credits
	unsigned int m_nCredits;
};

#endif //_D6C_D6PLAYER_H_