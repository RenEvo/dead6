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
#include "Plant.h"
#include "Item.h"
#include "Weapon.h"
#include "Projectile.h"
#include "Actor.h"
#include "Game.h"
#include "C4.h"



//------------------------------------------------------------------------
CPlant::CPlant()
: m_projectileId(0)
{
	
}

//------------------------------------------------------------------------
CPlant::~CPlant()
{
}

//------------------------------------------------------------------------
void CPlant::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);
}


//------------------------------------------------------------------------
void CPlant::UpdateFPView(float frameTime)
{
}


//------------------------------------------------------------------------
void CPlant::Release()
{
	delete this;
}


//------------------------------------------------------------------------
void CPlant::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *plant = params?params->GetChild("plant"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;
	m_plantparams.Reset(plant, true);
	m_plantactions.Reset(actions, true);
}

//------------------------------------------------------------------------
void CPlant::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *plant = patch?patch->GetChild("plant"):0;
	const IItemParamsNode *actions = patch?patch->GetChild("actions"):0;
	m_plantparams.Reset(plant, false);
	m_plantactions.Reset(actions, false);
}

//------------------------------------------------------------------------
void CPlant::Activate(bool activate)
{
	m_plantTimer = 0.0f;
	m_planting = false;
	m_pressing = false;
	m_holding = false;
	m_timing = false;

	m_time = 0.0f;
	m_tickTimer = 0.0f;

	CheckAmmo();
	SetLED(0, m_plantparams.led_minutes, m_plantparams.led_layers.c_str());
}

//------------------------------------------------------------------------
bool CPlant::OutOfAmmo() const
{
	if (m_plantparams.clip_size!=0)
		return m_plantparams.ammo_type_class && m_plantparams.clip_size != -1 && m_pWeapon->GetAmmoCount(m_plantparams.ammo_type_class)<1;

	return m_plantparams.ammo_type_class && m_pWeapon->GetInventoryAmmoCount(m_plantparams.ammo_type_class)<1;
}

//------------------------------------------------------------------------
void CPlant::NetShoot(const Vec3 &hit, int ph)
{
	NetShootEx(ZERO, ZERO, ZERO, hit, ph);
}

//------------------------------------------------------------------------
void CPlant::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int ph)
{
	Plant(pos, dir, vel, true);
}

//------------------------------------------------------------------------
void CPlant::NetStartFire(EntityId shooterId)
{
}

//------------------------------------------------------------------------
void CPlant::NetStopFire(EntityId shooterId)
{
}

//------------------------------------------------------------------------
const char *CPlant::GetType() const
{
	return "Plant";
}

//------------------------------------------------------------------------
IEntityClass* CPlant::GetAmmoType() const
{
	return m_plantparams.ammo_type_class;
}

//------------------------------------------------------------------------
int CPlant::GetDamage() const
{
	return m_plantparams.damage;
}


//------------------------------------------------------------------------
struct CPlant::ReleaseButtonAction
{
	ReleaseButtonAction(CPlant *_plant): pPlant(_plant) {};

	CPlant *pPlant;

	void execute(CItem *_this)
	{
		if (pPlant->m_time>=pPlant->m_plantparams.min_time)
		{
			_this->SetBusy(false);
		}
	}
};

void CPlant::Update(float frameTime, uint frameId)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool requireUpdate=false;

	CActor *pActor=m_pWeapon->GetOwnerActor();

	if (m_plantTimer>0.0f)
	{
		m_plantTimer -= frameTime;

		if (m_plantTimer<=0.0f)
		{
			m_plantTimer = 0.0f;
			
			if (m_planting)
			{
				CActor *pActor = m_pWeapon->GetOwnerActor();
				IMovementController *pMC = pActor?pActor->GetMovementController():0;
				SMovementState info;
				if (pMC)
					pMC->GetMovementState(info);

				Vec3 pos;
				Vec3 dir(FORWARD_DIRECTION);
				Vec3 vel(0,0,0);

				if (m_pWeapon->GetStats().fp)
					pos = m_pWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson, m_plantparams.helper.c_str(), true);
				else if (pMC)
					pos = info.eyePosition+info.eyeDirection*0.25f;
				else
					pos = pActor->GetEntity()->GetWorldPos();

				if (pMC)
					dir = info.eyeDirection;
				else
					dir = pActor->GetEntity()->GetWorldRotation().GetColumn1();

				if (IPhysicalEntity *pPE=pActor->GetEntity()->GetPhysics())
				{
					pe_status_dynamics sv;
					if (pPE->GetStatus(&sv))
					{
						if (sv.v.len2()>0.01f)
						{
							float dot=sv.v.GetNormalized().Dot(dir);
							if (dot<0.0f)
								dot=0.0f;
							vel=sv.v*dot;
						}
					}
				}

				Plant(pos, dir, vel);

				requireUpdate = true;
			}
		}
	}

	if (m_timing && m_holding)
	{
		if (m_tickTimer>0.0f)
		{
			m_tickTimer-=frameTime;

			if (m_tickTimer<=0.0f)
				TickTimer();

			requireUpdate = true;
		}
	}

	if (!m_pressing && m_holding)
	{
		requireUpdate = true;


		m_holding = false;
		m_pWeapon->PlayAction(m_plantactions.release_button.c_str());
		m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<ReleaseButtonAction>::Create(this), false);			
	}

	if (requireUpdate)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
bool CPlant::CanFire(bool considerAmmo/* =true */) const
{
	return !m_planting && !m_pressing && !m_pWeapon->IsBusy() && (!considerAmmo || !OutOfAmmo());
}

//------------------------------------------------------------------------
struct CPlant::PressButtonAction
{
	PressButtonAction(CPlant *_plant): pPlant(_plant) {};
	CPlant *pPlant;

	void execute(CItem *_this)
	{
		pPlant->m_holding = true;
		pPlant->m_pWeapon->PlayAction(pPlant->m_plantactions.hold_button.c_str(), 0, true, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);

		pPlant->TickTimer();
	}
};

struct CPlant::StartPlantAction
{
	StartPlantAction(CPlant *_plant): pPlant(_plant) {};
	CPlant *pPlant;

	void execute(CItem *_this)
	{
		pPlant->m_pWeapon->SetBusy(false);
		pPlant->m_planting = false;

		if (pPlant->m_time>0.0f || pPlant->m_plantparams.simple)
		{
			if (! pPlant->m_plantparams.simple)
			{
				pPlant->m_time = 0.0f;

				pPlant->SetLED(0.0f, pPlant->m_plantparams.led_minutes, pPlant->m_plantparams.led_layers.c_str());
			}

			if (pPlant->OutOfAmmo())
				pPlant->SelectLast();
			else
			{
				pPlant->CheckAmmo();

				_this->PlayAction(pPlant->m_plantactions.refill.c_str(), 0, 0, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
			}
		}
		else
			pPlant->SelectDetonator();
	}
};

void CPlant::StartFire(EntityId shooterId)
{
	m_pWeapon->RequireUpdate(eIUS_FireMode);

	if (!CanFire(true))
		return;

	if (m_timing)
	{
		m_pressing = true;

		m_pWeapon->SetBusy(true);
		m_pWeapon->PlayAction(m_plantactions.press_button.c_str());
		m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<PressButtonAction>::Create(this), false);
	}
	else
	{
		m_planting = true;
		m_pWeapon->SetBusy(true);
		m_pWeapon->PlayAction(m_plantactions.plant.c_str());

		m_plantTimer = m_plantparams.delay;

		m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<StartPlantAction>::Create(this), false);
	}
}

//------------------------------------------------------------------------
void CPlant::StopFire(EntityId shooterId)
{
	m_timing=false;
	m_pressing=false;
}

//------------------------------------------------------------------------
void CPlant::Serialize(TSerialize ser)
{
	if (ser.GetSerializationTarget()!=eST_Network)
		ser.Value("projectileId", m_projectileId);
}

//------------------------------------------------------------------------
void CPlant::TickTimer()
{
	m_tickTimer = m_plantparams.tick_time;

	if (m_time==0.0f)
		m_time=m_plantparams.min_time;
	else
		m_time+=m_plantparams.tick;

	if (m_time>m_plantparams.max_time)
		m_time=0.0f;

	m_pWeapon->PlayAction(m_plantactions.tick.c_str());

	SetLED(m_time, m_plantparams.led_minutes, m_plantparams.led_layers.c_str());

	if (!m_plantparams.simple && !m_pWeapon->IsServer())
		m_pWeapon->GetGameObject()->InvokeRMI(CC4::SvRequestTime(), 
			CC4::RequestTimeParams(m_time, m_pWeapon->GetCurrentFireMode()), eRMI_ToServer);
}

//------------------------------------------------------------------------
void CPlant::Plant(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, bool net)
{
	IEntityClass* ammo = m_plantparams.ammo_type_class;
	int ammoCount = m_pWeapon->GetAmmoCount(ammo);
	if (m_plantparams.clip_size==0)
		ammoCount = m_pWeapon->GetInventoryAmmoCount(ammo);

	CProjectile *pAmmo = m_pWeapon->SpawnAmmo(ammo, net);
	if (pAmmo)
	{
		pAmmo->SetParams(m_pWeapon->GetOwnerId(), m_pWeapon->GetHostId(), m_pWeapon->GetEntityId(), m_plantparams.damage, 0);
		pAmmo->SetDestination(m_pWeapon->GetDestination());
		pAmmo->Launch(pos, dir, vel);

		if (m_time>0.0f)
			pAmmo->GetEntity()->SetTimer(CProjectile::ePTIMER_LIFETIME, (int)(m_time*1000));
		else
			pAmmo->GetEntity()->SetTimer(CProjectile::ePTIMER_LIFETIME, 5*60*1000);

		m_projectileId = pAmmo->GetEntity()->GetId();

		if (m_projectileId && !m_plantparams.simple && m_pWeapon->IsServer())
		{
			CActor *pActor=m_pWeapon->GetOwnerActor();
			if (pActor)
			{
				if (m_time>0.0f)
					m_pWeapon->GetGameObject()->InvokeRMI(CC4::ClSetProjectileId(), 
						CC4::SetProjectileIdParams(0, m_pWeapon->GetCurrentFireMode()), eRMI_ToClientChannel, pActor->GetChannelId());
				else
					m_pWeapon->GetGameObject()->InvokeRMIWithDependentObject(CC4::ClSetProjectileId(), 
						CC4::SetProjectileIdParams(m_projectileId, m_pWeapon->GetCurrentFireMode()), eRMI_ToClientChannel, m_projectileId, pActor->GetChannelId());
			}
		}
	}

	m_pWeapon->OnShoot(m_pWeapon->GetOwnerId(), pAmmo?pAmmo->GetEntity()->GetId():0, GetAmmoType(), pos, dir, vel);

	ammoCount--;
	if (m_plantparams.clip_size != -1)
	{
		if (m_plantparams.clip_size!=0)
			m_pWeapon->SetAmmoCount(ammo, ammoCount);
		else
			m_pWeapon->SetInventoryAmmoCount(ammo, ammoCount);
	}

	if (!net)
		m_pWeapon->RequestShoot(ammo, pos, dir, vel, ZERO, pAmmo? pAmmo->GetGameObject()->GetPredictionHandle() : 0, true);

	m_pWeapon->HideItem(true);
}

//------------------------------------------------------------------------
void CPlant::SetLED(float time, bool minutes, const char *layerFmt)
{
	int d[3]={0};

	if (minutes)
	{
		int secs = (int)time%60;
		d[2] = (int)(time/60);
		d[1] = secs/10;
		d[0] = secs%10;
	}
	else
	{
		d[2] = (int)(time/100);
		d[1] = ((int)time%100)/10;
		d[0] = (((int)time%100)/10)%10;
	}

	string layer;
	for (int i=0;i<3;i++)
	{
		layer.Format(layerFmt, i+1, d[i]);
		m_pWeapon->PlayLayer(layer.c_str(), CItem::eIPAF_Default|CItem::eIPAF_NoBlend|CItem::eIPAF_CleanBlending, true);
	}
}

//------------------------------------------------------------------------
void CPlant::SelectDetonator()
{
	if (CActor *pOwner=m_pWeapon->GetOwnerActor())
	{
		EntityId detonatorId = pOwner->GetInventory()->GetItemByClass(CItem::sDetonatorClass);
		if (detonatorId)
			pOwner->SelectItemByName("Detonator", false);
	}
}

//------------------------------------------------------------------------
void CPlant::SelectLast()
{
	CActor *pOwner=m_pWeapon->GetOwnerActor();
	if (!pOwner)
		return;

	EntityId lastId = pOwner->GetInventory()->GetLastItem();

	if (!lastId || lastId==m_pWeapon->GetEntityId())
		pOwner->SelectNextItem(1, true);
	else
		pOwner->SelectLastItem(false);
}

//------------------------------------------------------------------------
void CPlant::CheckAmmo()
{
	m_pWeapon->HideItem(OutOfAmmo());
}

void CPlant::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_name);
	m_plantparams.GetMemoryStatistics(s);
	m_plantactions.GetMemoryStatistics(s);
}