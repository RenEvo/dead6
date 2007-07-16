// handles turning actions into CMovementRequests and setting player state
// for the local player

#ifndef __PLAYERINPUT_H__
#define __PLAYERINPUT_H__

#pragma once

#include "IActionMapManager.h"
#include "IPlayerInput.h"

class CPlayer;
struct SPlayerStats;

class CPlayerInput : public IPlayerInput, public IActionListener
{
public:
	CPlayerInput( CPlayer * pPlayer );
	~CPlayerInput();

	// IActionListener
	virtual void OnAction( const ActionId& action, int activationMode, float value );
	// ~IActionListener
	
	// IPlayerInput
	virtual void PreUpdate();
	virtual void Update();
	virtual void PostUpdate();

	virtual void SetState( const SSerializedPlayerInput& input );
	virtual void GetState( SSerializedPlayerInput& input );

	virtual void Reset();
	virtual void DisableXI(bool disabled);

	virtual void GetMemoryStatistics(ICrySizer * s) {s->Add(*this);}

	virtual EInputType GetType() const
	{
		return PLAYER_INPUT;
	};
	// ~IPlayerInput

	void SerializeSaveGame( TSerialize ser );

private:
	enum EMoveButtonMask
	{
		eMBM_Forward	= (1 << 0),
		eMBM_Back			= (1 << 1),
		eMBM_Left			= (1 << 2),
		eMBM_Right		= (1 << 3),
	};

	EStance FigureOutStance();
	void AdjustMoveButtonState( EMoveButtonMask buttonMask, int activationMode );
	bool CheckMoveButtonStateChanged( EMoveButtonMask buttonMask, int activationMode );
	float MapControllerValue(float value, float scale, float curve, bool inverse);

	void ApplyMovement(Vec3 delta);
	const Vec3 &FilterMovement(const Vec3 &desired);

	bool CanMove() const;

	bool OnActionMoveForward(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveBack(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMoveRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionJump(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionCrouch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSprint(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionToggleStance(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionProne(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	//bool OnActionZeroGBrake(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionGyroscope(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionGBoots(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionLeanLeft(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionLeanRight(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionHolsterItem(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionUse(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	// Nanosuit
	bool OnActionSpeedMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionStrengthMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionDefenseMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSuitCloak(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	// Cheats
	bool OnActionThirdPerson(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFlyMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionGodMode(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionXIRotateYaw(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIRotatePitch(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIMoveX(EntityId entityId, const ActionId& actionId, int activationMode, float value);
	bool OnActionXIMoveY(EntityId entityId, const ActionId& actionId, int activationMode, float value);

	bool OnActionInvertMouse(EntityId entityId, const ActionId& actionId, int activationMode, float value);
private:
	Vec3 m_lastPos;

	CPlayer* m_pPlayer;
	SPlayerStats* m_pStats;
	TActionHandler<CPlayerInput>	m_actionHandler;
	uint32 m_actions;
	uint32 m_lastActions;
	Vec3 m_deltaMovement;
	Vec3 m_xi_deltaMovement;
  Vec3 m_deltaMovementPrev;
	Ang3 m_deltaRotation;
	Ang3 m_xi_deltaRotation;
	float m_speedLean;
	float	m_buttonPressure;
	bool m_bDisabledXIRot;
	float	m_fCrouchPressedTime;
	bool m_bUseXIInput;
	uint32 m_moveButtonState;
	Vec3 m_filteredDeltaMovement;
	bool m_checkZoom;
};

#endif
