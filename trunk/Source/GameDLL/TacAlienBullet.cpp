/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Tactical Sleep Alien Bullet Implementation

-------------------------------------------------------------------------
History:
- 22:11:2006   19:55 : Created by Benito Gangoso Rodriguez

*************************************************************************/
#include "StdAfx.h"
#include "TacAlienBullet.h"
#include "TacticalAttachment.h"
#include "Game.h"
#include <IEntitySystem.h>
#include <IGameRulesSystem.h>
#include <IAnimationGraph.h>
#include "GameRules.h"

CTacAlienBullet::CTacAlienBullet(void)
{
}

CTacAlienBullet::~CTacAlienBullet(void)
{
}

void CTacAlienBullet::HandleEvent(const SGameObjectEvent &event)
{
	if (m_destroying)
		return;

	CProjectile::HandleEvent(event);

	if (event.event == eGFE_OnCollision)
	{
		EventPhysCollision *pCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);
		if (!pCollision)
			return;

		IEntity *pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1]:0;
		if (pTarget)
		{
			EntityId targetId = pTarget->GetId();
			CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTarget->GetId()));
			if(pActor && pActor->CanSleep())
			{
				SimpleHitInfo info(m_ownerId, targetId, m_weaponId, 1); // 0=tag,1=tac

				info.remote=IsRemote();

				g_pGame->GetGameRules()->ClientSimpleHit(info);
			}
		}

		Destroy();
	}
}