/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Tactical Weapon Attachment Item

-------------------------------------------------------------------------
History:
- 21:11:2006   15:45 : Created by Márcio Martins

*************************************************************************/
#ifndef __TACTICALATTACHMENT_H__
#define __TACTICALATTACHMENT_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Weapon.h"


struct STacEffect;


class CTacticalAttachment :
	public CItem
{
public:
	CTacticalAttachment();
	~CTacticalAttachment();

	virtual bool Init(IGameObject * pGameObject );
	virtual void PostInit(IGameObject * pGameObject );
	virtual void OnReset();
	virtual void Update(SEntityUpdateContext& ctx, int slot) {};
	//virtual void AddTarget(EntityId trgId);
	virtual void OnAttach(bool attach);
	//virtual void RunEffectOnHumanTargets(STacEffect *effect, EntityId shooterId);
	//virtual void RunEffectOnAlienTargets(STacEffect *effect, EntityId shooterId);

	virtual void SleepTarget(EntityId shooterId, EntityId targetId);

//private:
	//std::vector< EntityId > m_targets;
};

#endif //__TACTICALATTACHMENT_H__
