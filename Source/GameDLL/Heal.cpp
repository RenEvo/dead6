/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 2:8:2006   19:05 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Heal.h"
#include "Game.h"
#include "Item.h"
#include "Weapon.h"
#include "GameRules.h"
#include "Player.h"
#include <IEntitySystem.h>
#include "IMaterialEffects.h"

//------------------------------------------------------------------------
CHeal::CHeal()
{
}

//------------------------------------------------------------------------
CHeal::~CHeal()
{
}

void CHeal::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);

	m_healing = false;
	m_delayTimer=0.0f;
}

//------------------------------------------------------------------------
void CHeal::Update(float frameTime, uint frameId)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool requireUpdate = false;
	if (m_healing)
	{
		requireUpdate = true;
		if (m_delayTimer>0.0f)
		{
			m_delayTimer-=frameTime;
			if (m_delayTimer<=0.0f)
			{
				m_delayTimer=0.0f;

				if (m_selfHeal)
					Heal(m_pWeapon->GetOwnerId());
				else
					Heal(GetHealableId());
			}
		}
	}

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CHeal::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CHeal::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *heal = params?params->GetChild("heal"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;

	m_healparams.Reset(heal);
	m_healactions.Reset(actions);
}

//------------------------------------------------------------------------
void CHeal::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *heal = patch->GetChild("heal");
	const IItemParamsNode *actions = patch->GetChild("actions");

	m_healparams.Reset(heal, false);
	m_healactions.Reset(actions, false);
}

//------------------------------------------------------------------------
void CHeal::Activate(bool activate)
{
	m_healing = false;
	m_delayTimer=0.0f;
}

//------------------------------------------------------------------------
int CHeal::GetAmmoCount() const
{
	return m_pWeapon->GetAmmoCount(GetAmmoType());
}

//------------------------------------------------------------------------
bool CHeal::OutOfAmmo() const
{
	return m_healparams.ammo_type_class && m_pWeapon->GetAmmoCount(m_healparams.ammo_type_class)<1;
}

//------------------------------------------------------------------------
bool CHeal::CanFire(bool considerAmmo) const
{
	return !m_healing && !OutOfAmmo();
}

//------------------------------------------------------------------------
struct CHeal::StopAttackingAction
{
	CHeal *_this;
	StopAttackingAction(CHeal *heal): _this(heal) {};
	void execute(CItem *pItem)
	{
		_this->m_healing = false;
		_this->m_delayTimer = 0.0f;
		pItem->SetBusy(false);

		int ammoCount = _this->GetAmmoCount();
		if (ammoCount > 0)
			_this->m_pWeapon->PlayAction(_this->m_healactions.next.c_str());
		else if (_this->m_healparams.auto_select_last)
			static_cast<CPlayer*>(_this->m_pWeapon->GetOwnerActor())->SelectLastItem(true);
	}
};

void CHeal::StartFire(EntityId shooterId)
{
	if (!CanFire())
		return;

	m_healing = true;
	m_selfHeal = false;

	if (!GetHealableId())
		m_selfHeal=true;

	if (m_selfHeal)
		m_pWeapon->PlayAction(m_healactions.self_heal.c_str());
	else
		m_pWeapon->PlayAction(m_healactions.heal.c_str());
	
	m_pWeapon->SetBusy(true);
	m_pWeapon->RequireUpdate(eIUS_FireMode);

	m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<StopAttackingAction>::Create(this), true);

	m_delayTimer = m_healparams.delay;

	m_pWeapon->RequestShoot(0, ZERO, ZERO, ZERO, Vec3(0.0f, 0.0f, m_selfHeal?1.0f:0.0f), 0, false);
}

//------------------------------------------------------------------------
void CHeal::StopFire(EntityId shooterId)
{
}

//------------------------------------------------------------------------
void CHeal::NetShoot(const Vec3 &hit, int predictionHandle)
{
	NetShootEx(ZERO, ZERO, ZERO, hit, predictionHandle);
}

//------------------------------------------------------------------------
void CHeal::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int predictionHandle)
{
	if (hit.z>0.0f)
		m_pWeapon->PlayAction(m_healactions.self_heal.c_str());
	else
		m_pWeapon->PlayAction(m_healactions.heal.c_str());
}

//------------------------------------------------------------------------
const char *CHeal::GetType() const
{
	return "Heal";
}

//------------------------------------------------------------------------
IEntityClass* CHeal::GetAmmoType() const
{
	return m_healparams.ammo_type_class;
}

//------------------------------------------------------------------------
int CHeal::GetDamage() const
{
	return -m_healparams.health;
}

//------------------------------------------------------------------------
void CHeal::Heal(EntityId id)
{
	if (!id)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();

	HitInfo info(m_pWeapon->GetOwnerId(), id, m_pWeapon->GetEntityId(), -m_healparams.health, 0.0f,
		0, 0,	pGameRules->GetHitTypeId(m_healparams.hit_type.c_str()), ZERO, ZERO, ZERO);

	pGameRules->ClientHit(info);

	m_pWeapon->PlayAction(m_healactions.healed.c_str());

	if (m_healparams.clip_size!=-1 && m_healparams.ammo_type_class)
	{
		int ammoCount=GetAmmoCount();
		--ammoCount;
		m_pWeapon->SetAmmoCount(GetAmmoType(), ammoCount);
	}
}

//------------------------------------------------------------------------
EntityId CHeal::GetHealableId() const
{
	CActor *pOwner=m_pWeapon->GetOwnerActor();
	
	if (pOwner && pOwner->IsPlayer())
	{
		IMovementController * pMC = pOwner?pOwner->GetMovementController():0;
		if (pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);

			Vec3 pos=info.eyePosition;
			Vec3 dir=info.eyeDirection*m_healparams.range;

			int skipEnts = 1;
			IPhysicalEntity* pSkipEnts[2];
			pSkipEnts[0] = pOwner->GetEntity()->GetPhysics();

			if (pOwner->GetLinkedEntity())
			{
				pSkipEnts[1] = pOwner->GetLinkedEntity()->GetPhysics();
				++skipEnts;
			}

			static ray_hit ray;
			if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir, ent_all&~(ent_rigid|ent_static|ent_terrain),
				rwi_stop_at_pierceable|rwi_colltype_any, &ray, 1, pSkipEnts, skipEnts))
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(ray.pCollider);
				return pEntity?pEntity->GetId():0;
			}
		}
	}

	return 0;
}

void CHeal::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_name);
	m_healparams.GetMemoryStatistics(s);
	m_healactions.GetMemoryStatistics(s);
}
