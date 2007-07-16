#ifndef __AIDEMOINPUT_H__
#define __AIDEMOINPUT_H__

#pragma once

#include "IPlayerInput.h"

class CAIDemoInput : public IPlayerInput
{

public:
	CAIDemoInput(CPlayer *player) : m_pPlayer(player)
	{
	}

	virtual void PreUpdate()	
	{
		if(GetISystem()->IsDemoMode() == 1)
		{
			SMovementState movementState;
			m_pPlayer->GetMovementController()->GetMovementState(movementState);
			m_serializedInput.deltaMovement = movementState.pos - m_lastPos;
			m_lastPos = movementState.pos;
			m_serializedInput.lookDirection = movementState.eyeDirection;
			m_serializedInput.bodyDirection = movementState.bodyDirection;
			m_serializedInput.stance = movementState.stance;
		}
	};

	virtual void Update()	{};

	virtual void PostUpdate() {};

	virtual void OnAction( const ActionId& action, int activationMode, float value ) {};

	virtual void SetState( const SSerializedPlayerInput& input )
	{
		assert(0); //should never happen
	};
	virtual void GetState( SSerializedPlayerInput& input )
	{
		input = m_serializedInput;
	};

	virtual void Reset()
	{		
		SMovementState movementState;
		m_pPlayer->GetMovementController()->GetMovementState(movementState);
		m_lastPos = movementState.pos;
	};

	virtual void DisableXI(bool disabled)
	{
	};

	virtual EInputType GetType() const
	{
		return AIDEMO_INPUT;
	};

	virtual void GetMemoryStatistics(ICrySizer * s) {s->Add(*this);}

private:

	CPlayer *m_pPlayer;
	Vec3		m_lastPos;
	SSerializedPlayerInput m_serializedInput;
};

#endif