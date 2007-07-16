#include "StdAfx.h"
#include "TacBullet.h"
#include "TacticalAttachment.h"
#include "Game.h"
#include <IEntitySystem.h>
#include <IGameRulesSystem.h>
#include <IAnimationGraph.h>
#include "GameRules.h"


CTacBullet::CTacBullet()
{
}

CTacBullet::~CTacBullet()
{
}

void CTacBullet::HandleEvent(const SGameObjectEvent &event)
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

			SimpleHitInfo info(m_ownerId, targetId, m_weaponId, 1); // 0=tag,1=tac
			info.remote=IsRemote();

			g_pGame->GetGameRules()->ClientSimpleHit(info);
		}

		Destroy();
	}
}
