/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 12:04:2006   17:22 : Created by Márcio Martins
- 18:02:2007	 13:30 : Refactored Offhand by Benito G.R.

*************************************************************************/

#ifndef __OFFHAND_H__
#define __OFFHAND_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IItemSystem.h>
#include "Weapon.h"

#define OH_NO_GRAB					0
#define OH_GRAB_OBJECT			1
#define OH_GRAB_ITEM				2
#define OH_GRAB_NPC					3

enum EOffHandActions
{
	eOHA_NO_ACTION				= 0,
	eOHA_SWITCH_GRENADE		= 1,
	eOHA_THROW_GRENADE		= 2,
	eOHA_USE							= 3,
	eOHA_PICK_ITEM				= 4,
	eOHA_PICK_OBJECT			= 5,
	eOHA_THROW_OBJECT			= 6,
	eOHA_GRAB_NPC					= 7,
	eOHA_THROW_NPC				= 8,
	eOHA_RESET						= 9,
	eOHA_FINISH_AI_THROW_GRENADE = 10,
	eOHA_MELEE_ATTACK			= 11,
	eOHA_FINISH_MELEE			= 12,
	eOHA_XI_USE						= 13
};

enum EOffHandStates
{
	eOHS_INIT_STATE					= 0x00000001,
	eOHS_SWITCHING_GRENADE  = 0x00000002,
	eOHS_HOLDING_GRENADE		= 0x00000004,
	eOHS_THROWING_GRENADE   = 0x00000008,
	eOHS_PICKING						= 0x00000010,
	eOHS_PICKING_ITEM				= 0x00000020,
	eOHS_PICKING_ITEM2			= 0x00000040,
	eOHS_HOLDING_OBJECT			= 0x00000080,
	eOHS_THROWING_OBJECT		= 0x00000100,
	eOHS_GRABBING_NPC				= 0x00000200,
	eOHS_HOLDING_NPC				= 0x00000400,
	eOHS_THROWING_NPC				= 0x00000800,
	eOHS_TRANSITIONING			= 0x00001000,
	eOHS_MELEE							= 0x00002000
};

enum EOffHandSounds
{
	eOHSound_Choking_Trooper	= 0,
	eOHSound_Choking_Human		= 1,
	eOHSound_Kill_Human				= 2, 
	eOHSound_LastSound				= 3
};


class COffHand : public CWeapon
{

	typedef struct SGrabType
	{
		ItemString	helper;
		ItemString	pickup;
		ItemString	idle;
		ItemString	throwFM;
		bool		twoHanded;
	};

	typedef std::vector<SGrabType>				TGrabTypes;

public:

	COffHand();
	virtual ~COffHand();

	virtual void Update(SEntityUpdateContext &ctx, int slot);
	virtual void PostInit(IGameObject * pGameObject );
	virtual void Reset();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	virtual bool CanSelect() const;
	virtual void Select(bool select);
	virtual void Serialize(TSerialize ser, unsigned aspects);
	virtual void PostSerialize();

	virtual void MeleeAttack();

	virtual void PostFilterView(struct SViewParams &viewParams);

	//Only needed because is used in CFists
	virtual void EnterWater(bool enter) {}

	virtual void UpdateFPView(float frameTime);

	//AIGrenades (for AI)
	virtual void PerformThrow(EntityId shooterId, float speedScale);

	//Memory Statistics
	virtual void GetMemoryStatistics(ICrySizer * s) { s->Add(*this); CWeapon::GetMemoryStatistics(s); }

	void SetOffHandState(EOffHandStates eOHS);
	int  GetOffHandState() { return m_currentState; } 
	void FinishAction(EOffHandActions eOHA);

	bool IsHoldingEntity();

	void SetResetTimer(float t) { m_resetTimer = t;}

protected:

	virtual bool ReadItemParams(const IItemParamsNode *root);

private:
	
	void	SelectGrabType(IEntity* pEntity);

	bool EvalutateStateTransition(int requestedAction, int activationMode);
	bool PreExecuteAction(int requestedAction, int activationMode);
	void CancelAction();

	void IgnoreCollisions(bool ignore, EntityId entityId=0);
	void DrawNear(bool drawNear, EntityId entityId=0);
	bool PerformPickUp();
	int  CanPerformPickUp(CActor *pActor, IPhysicalEntity *pPhysicalEntity = NULL, bool getEntityInfo = false);

	void UpdateCrosshairUsability();
	void UpdateHeldObject();
	void UpdateMainWeaponRaising();
	void UpdateGrabbedNPCState();

	void StartSwitchGrenade();
	void EndSwitchGrenade();

	//Offhand (for Player)
	void PerformThrow(int activationMode, EntityId throwableId, int oldFMId = -1, bool isLivingEnt = false);

	void StartPickUpItem();
	void EndPickUpItem();

	void PickUpObject(bool isLivingEnt = false);
	void ThrowObject(int activationMode, bool isLivingEnt = false);

	bool GrabNPC();
	void ThrowNPC(bool kill = true);

	//Special stuff for grabbed NPCs
	void RunEffectOnGrabbedNPC(CActor* pNPC);
	void PlaySound(EOffHandSounds sound, bool play);

private:

	//Grenade info
	int					m_lastFireModeId;
	float				m_nextGrenadeThrow;		

	float				m_lastCHUpdate;

	//All what we need for grabbing
	TGrabTypes		m_grabTypes;
	uint32				m_grabType;
	EntityId			m_heldEntityId, m_preHeldEntityId;
	Matrix34			m_holdOffset;
	int						m_constraintId;
	bool					m_hasHelper;
	int						m_grabbedNPCSpecies;
	float					m_heldEntityMass;
	
	float					m_killTimeOut;
	bool					m_killNPC;
	bool					m_effectRunning;
	bool					m_npcWasDead;
	int						m_grabbedNPCInitialHealth;

	tSoundID			m_sounds[eOHSound_LastSound];

	float					m_range;
	float					m_pickingTimer;
	float					m_resetTimer;

	int						m_usable;

	//Current state and pointers to actor main item(weapon) while offHand is selected
	int						m_currentState;
	CItem					*m_mainHand;
	CWeapon				*m_mainHandWeapon; 
};

#endif
