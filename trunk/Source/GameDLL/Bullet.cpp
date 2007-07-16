/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:10:2005   14:14 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Bullet.h"
#include "GameRules.h"
#include <IEntitySystem.h>
#include <IGameTokens.h>
#include "AmmoParams.h"


int CBullet::m_waterMaterialId = -1;
IEntityClass* CBullet::EntityClass = 0;

//------------------------------------------------------------------------
CBullet::CBullet()
{
}

//------------------------------------------------------------------------
CBullet::~CBullet()
{
}

//------------------------------------------------------------------------
void CBullet::HandleEvent(const SGameObjectEvent &event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CProjectile::HandleEvent(event);

	if (event.event == eGFE_OnCollision)
	{
		if (m_destroying)
			return;

		EventPhysCollision *pCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);
		if (!pCollision)
			return;
        
		IEntity *pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1]:0;

		Vec3 dir(0, 0, 0);
		if (pCollision->vloc[0].GetLengthSquared() > 1e-6f)
			dir = pCollision->vloc[0].GetNormalized();

		CGameRules *pGameRules = g_pGame->GetGameRules();

		HitInfo hitInfo(m_ownerId, pTarget?pTarget->GetId():0, m_weaponId,
			m_damage, 0.0f, pGameRules->GetHitMaterialIdFromSurfaceId(pCollision->idmat[1]), pCollision->partid[1],
			m_hitTypeId, pCollision->pt, dir, pCollision->n);

		hitInfo.remote = IsRemote();
		hitInfo.projectileId = GetEntityId();
		hitInfo.bulletType = m_pAmmoParams->bulletType;

		if (m_weaponId)
		{
			CWeapon *pWeapon=GetWeapon();
			if (pWeapon && pWeapon->GetForcedHitMaterial() != -1)
				hitInfo.material=pGameRules->GetHitMaterialIdFromSurfaceId(pWeapon->GetForcedHitMaterial());
		}

//        Vec3 p = GetEntity()->GetWorldPos();
//        CryLog("BulletHit %.3f %.3f %.3f",p.x,p.y,p.z);
		pGameRules->ClientHit(hitInfo);

		if (pCollision->pEntity[0]->GetType() == PE_PARTICLE)
		{
			float bouncy, friction;
			uint	pierceabilityMat;
			gEnv->pPhysicalWorld->GetSurfaceParameters(pCollision->idmat[1], bouncy, friction, pierceabilityMat);
			
			pe_params_particle params;
			pCollision->pEntity[0]->GetParams(&params);

			//Under water trail
			m_underWater = false;
			Vec3 pos=pCollision->pt;
			if ((pCollision->idmat[1] == CBullet::m_waterMaterialId) && (gEnv->p3DEngine->GetWaterLevel(&pos)>=pCollision->pt.z))
			{
				//Reduce drastically bullet velocity (to be able to see the trail effect)
				pe_params_particle pparams;
				m_pPhysicalEntity->GetParams(&pparams);
				pparams.velocity = 25.0f;

				m_pPhysicalEntity->SetParams(&pparams);

				m_underWater = true;
			}

			if (pierceabilityMat<params.iPierceability) //Do not destroy if collides water
				Destroy();

		}
	}
}

void CBullet::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CProjectile::Update(ctx,updateSlot);

	//Underwater trails
	if(m_underWater && m_trailUnderWaterId<0)
		TrailEffect(true,true);
}

void CBullet::SetWaterMaterialId()
{
	m_waterMaterialId = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_water")->GetId();
}


