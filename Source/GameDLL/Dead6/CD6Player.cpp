////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CD6Game.cpp
//
// Purpose: Dead6 Core Player declarations
//
// File History:
//	- 7/22/07 : File created - Dan
//	- 8/23/07 : File edited - KAK
////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CD6Player.h"

////////////////////////////////////////////////////
CD6Player::CD6Player()
{
	m_nCredits = 0;
}

////////////////////////////////////////////////////
CD6Player::~CD6Player()
{

}

////////////////////////////////////////////////////
void CD6Player::Reset(bool toGame)
{
	// Base reset
	CActor::Reset(toGame);

	m_nCredits = 0;
}

////////////////////////////////////////////////////
bool CD6Player::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	// Base update
	if (false == CPlayer::NetSerialize(ser, aspect, profile, flags))
		return false;

	// Credits aspect update
	if (ASPECT_CREDITS == aspect)
	{
		ser.Value("credits", m_nCredits, 'cred');
	}

	return true;
}

////////////////////////////////////////////////////
void CD6Player::FullSerialize(TSerialize ser)
{
	// Base serialize
	CPlayer::FullSerialize(ser);

	// D6 serialization
	ser.BeginGroup("D6Player");
	ser.Value("credits", m_nCredits);
	ser.EndGroup();
}

////////////////////////////////////////////////////
void CD6Player::SetCredits(unsigned int nCredits)
{
	m_nCredits = nCredits;
	GetGameObject()->ChangedNetworkState(ASPECT_CREDITS);
		CryLog("Player %d credits changed to: %u", GetEntity()->GetId(), m_nCredits);
}

////////////////////////////////////////////////////
unsigned int CD6Player::GetCredits() const
{
	return m_nCredits;
}

////////////////////////////////////////////////////
void CD6Player::GiveCredits(unsigned int nCredits)
{
	m_nCredits += nCredits;
	GetGameObject()->ChangedNetworkState(ASPECT_CREDITS);
		CryLog("Player %d credits changed to: %u", GetEntity()->GetId(), m_nCredits);
}

////////////////////////////////////////////////////
void CD6Player::TakeCredits(unsigned int nCredits)
{
	m_nCredits -= nCredits;
	GetGameObject()->ChangedNetworkState(ASPECT_CREDITS);
		CryLog("Player %d credits changed to: %u", GetEntity()->GetId(), m_nCredits);
}