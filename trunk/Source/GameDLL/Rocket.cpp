/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:10:2005   14:14 : Created by M�rcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Rocket.h"
#include "Game.h"
#include "Bullet.h"


//------------------------------------------------------------------------
CRocket::CRocket()
: m_launchLoc(0,0,0),
	m_safeExplosion(0)
{
}

//------------------------------------------------------------------------
CRocket::~CRocket()
{
	//LAW might be dropped automatically (to be sure that works in MP too)
	if(CWeapon* pWeapon = GetWeapon())
	{
		if(pWeapon->IsAutoDroppable() && (!pWeapon->GetOwnerId() || pWeapon->GetOwnerId()==GetOwnerId()))
			pWeapon->AutoDrop();
	}
}

//------------------------------------------------------------------------
void CRocket::HandleEvent(const SGameObjectEvent &event)
{
	if (m_destroying)
		return;

	CProjectile::HandleEvent(event);

	if (!gEnv->bServer || GetISystem()->IsDemoMode() == 2)
		return;

	if (event.event == eGFE_OnCollision)
	{		
		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;
		if (m_safeExplosion>0.0f)
		{
			float dp2=(m_launchLoc-GetEntity()->GetWorldPos()).len2();
			if (dp2<=m_safeExplosion*m_safeExplosion)
				return;
		}

		if(pCollision && pCollision->pEntity[0]->GetType()==PE_PARTICLE)
		{
			float bouncy, friction;
			uint	pierceabilityMat;
			gEnv->pPhysicalWorld->GetSurfaceParameters(pCollision->idmat[1], bouncy, friction, pierceabilityMat);

			pe_params_particle params;
			pCollision->pEntity[0]->GetParams(&params);
			
			if((params.velocity>1.0f) && (pCollision->idmat[1] != CBullet::GetWaterMaterialId())
				&& (!pCollision->pEntity[1] || (pCollision->pEntity[1]->GetType() != PE_LIVING && pCollision->pEntity[1]->GetType() != PE_ARTICULATED)))
			{
				//Just check entity velocity (should be 0 if not pierceable, even when material is wrong set)
				// hack for pierceability on terrain - zero bounding boxes is terrain, which will cause rocket to explode regardless of pierceability logic
				//pe_params_bbox bbox;
				//pCollision->pEntity[1]->GetParams(&bbox);

				//If not water and not an actor...
				//if( (!bbox.BBox[0].IsZero() || !bbox.BBox[1].IsZero()) && pierceabilityMat>=params.iPierceability )
				if(pierceabilityMat>=params.iPierceability)
					return;
			}

		}

    IEntity* pTarget = pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)pCollision->pForeignData[1] : 0;

    Explode(true, true, pCollision->pt, pCollision->n, pCollision->vloc[0], pTarget?pTarget->GetId():0);
	}
}

//------------------------------------------------------------------------
void CRocket::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CProjectile::Launch(pos, dir, velocity, speedScale);

	m_launchLoc=pos;

	m_safeExplosion = GetParam("safeexplosion", m_safeExplosion);
}

