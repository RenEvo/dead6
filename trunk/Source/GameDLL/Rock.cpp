/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 5:5:2006   15:26 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Rock.h"
#include "GameRules.h"
#include <IEntitySystem.h>
#include <IGameTokens.h>



//------------------------------------------------------------------------
CRock::CRock()
{
}

//------------------------------------------------------------------------
CRock::~CRock()
{
}

//------------------------------------------------------------------------
void CRock::HandleEvent(const SGameObjectEvent &event)
{
	CProjectile::HandleEvent(event);

	if (event.event == eGFE_OnCollision)
	{
		if (m_destroying)
			return;

		EventPhysCollision *pCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);
		if (!pCollision)
			return;

		IEntity *pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1]:0;

		if (!pTarget || pTarget->GetId()==m_ownerId)
			return;

		Vec3 dir(0, 0, 0);
		if (pCollision->vloc[0].GetLengthSquared() > 1e-6f)
			dir = pCollision->vloc[0].GetNormalized();

		CGameRules *pGameRules = g_pGame->GetGameRules();

		HitInfo hitInfo(m_ownerId, pTarget?pTarget->GetId():0, m_weaponId,
			m_damage, 0.0f, pGameRules->GetHitMaterialIdFromSurfaceId(pCollision->idmat[1]), pCollision->partid[1],
			pGameRules->GetHitTypeId("melee"), pCollision->pt, dir, pCollision->n);

		hitInfo.remote = IsRemote();
		hitInfo.projectileId = GetEntityId();

		if (m_weaponId)
		{
			CWeapon *pWeapon=GetWeapon();
			if (pWeapon && pWeapon->GetForcedHitMaterial() != -1)
				hitInfo.material=pGameRules->GetHitMaterialIdFromSurfaceId(pWeapon->GetForcedHitMaterial());
		}

		pGameRules->ClientHit(hitInfo);

		m_damage =(int)(m_damage*0.25);
	}
}


