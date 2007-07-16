/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: OffHand Implementation

-------------------------------------------------------------------------
History:
- 26:04:2006   18:35 : Created by Márcio Martins

*************************************************************************/
#ifndef __FISTS_H__
#define __FISTS_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IItemSystem.h>
#include <IViewSystem.h>
#include "OffHand.h"

#define TIMEOUT			6.0f

typedef enum EFistAnimState
{
	eFAS_NOSTATE	= -1,
	eFAS_RELAXED  =  0,
	eFAS_FIGHT		=  1,
	eFAS_RUNNING  =  2,
	eFAS_JUMPING  =  3,
	eFAS_LANDING   =  4,
	eFAS_CRAWL		=  5,
	eFAS_SWIM_IDLE = 6,
	eFAS_SWIM_FORWARD = 7,
	eFAS_SWIM_BACKWARD	 =8,
	eFAS_SWIM_SPEED  = 9,
	eFAS_SWIM_FORWARD_S = 10
};

class CFists : public COffHand
{
	struct EndRaiseWeaponAction;

public:


	CFists();
	virtual ~CFists();

	virtual void Update(SEntityUpdateContext &ctx, int slot);
	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	virtual bool CanSelect() const;
	virtual void Select(bool select);

	virtual void Reset();

	virtual void UpdateFPView(float frameTime);
	virtual void PostFilterView(SViewParams &viewParams);
	virtual void EnterWater(bool enter);
	virtual void GetMemoryStatistics(ICrySizer * s) { s->Add(*this); CWeapon::GetMemoryStatistics(s); }
	virtual void RaiseWeapon(bool raise, bool faster = false);
	virtual void NetStartMeleeAttack(EntityId shooterId, bool weaponMelee);

	virtual void MeleeAttack();

	void	RequestAnimState(EFistAnimState eFAS, bool force=false);
	int   GetCurrentAnimState() { return m_currentAnimState; }

	void  EnterFreeFall(bool enter) { m_inFreeFall = enter;}

protected:
	void	UpdateAnimState(float frameTime);
	void  CollisionFeeback(Vec3 &pos, int eFAS);

private:
	bool OnActionAttack(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSpecial(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	TActionHandler<CFists>	m_actionHandler;

protected:
	float		 m_timeOut;
	int			 m_currentAnimState;

	bool		 m_underWater;
	bool		 m_inFreeFall;
};


#endif//__FISTS_H__