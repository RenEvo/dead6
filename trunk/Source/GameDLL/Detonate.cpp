/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Detonate.h"
#include "WeaponSystem.h"
#include "Item.h"
#include "Weapon.h"
#include "Actor.h"
#include "Projectile.h"


#define DETONATE_MAX_DISTANCE 150.0f

//------------------------------------------------------------------------
CDetonate::CDetonate()
{
}

//------------------------------------------------------------------------
CDetonate::~CDetonate()
{
}

//------------------------------------------------------------------------
struct CDetonate::ExplodeAction
{
	ExplodeAction(CDetonate *_detonate): pDetonate(_detonate) {};
	CDetonate *pDetonate;

	void execute(CItem *_this)
	{
		pDetonate->SelectLast();
	}
};

void CDetonate::Update(float frameTime, uint frameId)
{
	CSingle::Update(frameTime, frameId);

	if (m_detonationTimer>0.0f)
	{
		m_detonationTimer-=frameTime;

		if (m_detonationTimer<=0.0f)
		{
			m_detonationTimer=0.0f;

			bool detonated = Detonate();

			if (detonated && m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsClient())
				m_pWeapon->GetScheduler()->TimerAction(uint(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson)*0.35f), CSchedulerAction<ExplodeAction>::Create(this), false);
		}
		else
			m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CDetonate::ResetParams(const struct IItemParamsNode *params)
{
	CSingle::ResetParams(params);
}

//------------------------------------------------------------------------
void CDetonate::PatchParams(const struct IItemParamsNode *patch)
{
	CSingle::PatchParams(patch);
}

//------------------------------------------------------------------------
void CDetonate::Activate(bool activate)
{
	CSingle::Activate(activate);
	
	m_detonationTimer=0.0f;
}

//------------------------------------------------------------------------
bool CDetonate::CanReload() const
{
	return false;
}

//------------------------------------------------------------------------
bool CDetonate::CanFire(bool considerAmmo) const
{
	return CSingle::CanFire(considerAmmo) && (m_detonationTimer<=0.0f);
}

//------------------------------------------------------------------------
void CDetonate::StartFire(EntityId shooterId)
{
	if (CanFire(false))
	{
		m_pWeapon->RequireUpdate(eIUS_FireMode);
		m_detonationTimer = 0.1f;
		m_pWeapon->PlayAction(m_actions.fire.c_str());
	}
}

//------------------------------------------------------------------------
const char *CDetonate::GetCrosshair() const
{
	return "";
}

//------------------------------------------------------------------------
bool CDetonate::Detonate(bool net)
{
	if (m_pWeapon->IsServer())
	{
		CActor *pOwner=m_pWeapon->GetOwnerActor();
		if (!pOwner)
			return false;

		if (CWeapon *pWeapon=pOwner->GetWeapon(pOwner->GetInventory()->GetItemByClass(CItem::sC4Class)))
		{
			EntityId projectileId=pWeapon->GetFireMode(pWeapon->GetCurrentFireMode())->GetProjectileId();
			if (projectileId)
			{
				if (CProjectile *pProjectile=g_pGame->GetWeaponSystem()->GetProjectile(projectileId))
				{
					IEntity *projectileEnt = pProjectile->GetEntity();
					IEntity *detonatorEnt  = m_pWeapon->GetEntity();
					
					if(projectileEnt && detonatorEnt)
					{
						float lenSqr = (projectileEnt->GetWorldPos()-detonatorEnt->GetWorldPos()).len2();
						if(lenSqr>DETONATE_MAX_DISTANCE*DETONATE_MAX_DISTANCE)
						{
							m_pWeapon->PlayAction(m_actions.empty_clip.c_str());
							return false;
						}
						else
							pProjectile->Explode(true,false);
					}
					else
						pProjectile->Explode(true, false);

					g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(pWeapon->GetOwner(), GameplayEvent(eGE_WeaponShot, GetName(), 1, (void *)pWeapon->GetEntityId()));
				}
			}
		}
	}

	if (!net)
		m_pWeapon->RequestShoot(0, ZERO, ZERO, ZERO, ZERO, 0, false);

	return true;
}

//------------------------------------------------------------------------
void CDetonate::NetShoot(const Vec3 &hit, int ph)
{
	Detonate(true);
}

//------------------------------------------------------------------------
void CDetonate::SelectLast()
{
	CActor *pOwner=m_pWeapon->GetOwnerActor();
	if (!pOwner)
		return;

	pOwner->SelectLastItem(false);
}