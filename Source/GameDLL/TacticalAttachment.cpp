/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 21:11:2005   15:45 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "TacticalAttachment.h"
#include "Game.h"
#include "Tactical.h"
#include "Actor.h"

//------------------------------------------------------------------------
CTacticalAttachment::CTacticalAttachment()
{
	//m_targets.resize(0);
}

//------------------------------------------------------------------------
CTacticalAttachment::~CTacticalAttachment()
{
	//m_targets.resize(0);
}

//------------------------------------------------------------------------
bool CTacticalAttachment::Init(IGameObject * pGameObject )
{
	if (!CItem::Init(pGameObject))
		return false;

	return true;
}

//------------------------------------------------------------------------
void CTacticalAttachment::PostInit(IGameObject * pGameObject )
{
	CItem::PostInit(pGameObject);
}

//------------------------------------------------------------------------
void CTacticalAttachment::OnReset()
{
	CItem::OnReset();

	//m_targets.resize(0);

}

/***********************************************************
//------------------------------------------------------------------------
void CTacticalAttachment::AddTarget(EntityId trgId)
{
	stl::push_back_unique(m_targets, trgId);
}
*************************************************************/

//------------------------------------------------------------------------
void CTacticalAttachment::OnAttach(bool attach)
{
	CItem::OnAttach(attach);
}

/************************************************************
//------------------------------------------------------------------------
void CTacticalAttachment::RunEffectOnHumanTargets(STacEffect *effect, EntityId shooterId)
{
	if (!effect)
		return;

	for (std::vector<EntityId>::const_iterator it=m_targets.begin(); it!=m_targets.end(); ++it)
		effect->Activate(*it, shooterId);

	m_targets.resize(0);
}

//------------------------------------------------------------------------
void CTacticalAttachment::RunEffectOnAlienTargets(STacEffect *effect, EntityId shooterId)
{
	RunEffectOnHumanTargets(effect, shooterId);
}
*******************************************************************/

//------------------------------------------------------------------------
void CTacticalAttachment::SleepTarget(EntityId shooterId, EntityId targetId)
{
	CActor *pActor = (CActor *)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetId);

	if (pActor && pActor->CanSleep())
	{
		IAISystem *pAISystem=gEnv->pAISystem;
		if (pAISystem)
		{
			if(IEntity* pEntity=pActor->GetEntity())
			{
				if(IAIObject* pAIObj=pEntity->GetAI())
				{
					IAISignalExtraData *pEData = pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SendAnonymousSignal
					// try to retrieve the shooter position
					if (IEntity* pOwnerEntity = gEnv->pEntitySystem->GetEntity(shooterId))
						pEData->point = pOwnerEntity->GetWorldPos();
					else
						pEData->point = pEntity->GetWorldPos();
					IAIActor* pAIActor = pAIObj->CastToIAIActor();
					if(pAIActor)
						pAIActor->SetSignal(1,"TRANQUILIZED",0,pEData);
				}
			}
		}

		pActor->CreateScriptEvent("sleep", 0);
		pActor->GetGameObject()->SetPhysicalizationProfile(eAP_Sleep);

		// no dropping weapons for AI
		if(pActor->IsPlayer())
			pActor->DropItem(pActor->GetCurrentItemId(), 1.0f, false);

		pActor->SetSleepTimer(12.5f);
	}
}