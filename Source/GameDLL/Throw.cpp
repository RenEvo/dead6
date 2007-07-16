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
#include "Throw.h"
#include "Actor.h"
#include "Player.h"
#include "Game.h"
#include "Projectile.h"
#include "WeaponSystem.h"
#include "OffHand.h"


//------------------------------------------------------------------------
CThrow::CThrow()
: m_throwableId(0),
	m_throwableAction(0),
	m_usingGrenade(true)
{
}

//------------------------------------------------------------------------
CThrow::~CThrow()
{
}


//------------------------------------------------------------------------
void CThrow::Update(float frameTime, uint frameId)
{
	CSingle::Update(frameTime, frameId);

//	float white[4]={1,1,1,1};
//	gEnv->pRenderer->Draw2dLabel(50,50,2.0f, white, false, "m_throwing %d - m_throw_time %f", m_throwing, m_throw_time);
	if (m_firing)
	{
		if (!m_pulling && !m_throwing && !m_thrown)
		{
			if (m_auto_throw)
			{
				DoThrow(false);
				m_auto_throw=false;
			}

			if(m_hold_timer>0.0f)
			{
				m_hold_timer -= frameTime;
				if (m_hold_timer<0.0f)
				{
					m_hold_timer=0.0f;
					PlayDropThrowSound();
				}
			}
		}
		else if (m_throwing && m_throw_time<=0.0f)
		{
			float strengthScale=1.0f;
			CActor *pOwner = m_pWeapon->GetOwnerActor();
			if (pOwner && pOwner->IsPlayer())
			{
				// Calculate the speed_scale here only for the player.
				// AI execution path will set the value explicitly via the SetSpeedScale method.
				strengthScale = 1.0f+(pOwner->GetActorStrength()-1.0f)/5.0f;

				float t=(1.0f-m_hold_timer/m_throwparams.hold_duration);
				float ds=((m_throwparams.hold_max_scale*strengthScale)-m_throwparams.hold_min_scale);

				m_speed_scale = m_throwparams.hold_min_scale+ds*t; // CSingle::m_speed_scale

				float mult = 1.0f;
				CPlayer *pPlayer = (CPlayer *)pOwner;
				if(CNanoSuit *pSuit = pPlayer->GetNanoSuit())
				{
					ENanoMode curMode = pSuit->GetMode();
					if (curMode == NANOMODE_STRENGTH)
						mult = 1.0f + pSuit->GetSlotValue(NANOSLOT_STRENGTH)*0.002f;
				}
				m_speed_scale *= mult;
				if(m_speed_scale<0.8f)
					m_speed_scale=0.8f;		//To avoid throw grenades too near to the player
			}

			m_pWeapon->HideItem(true);

			if (m_throwableId)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_throwableId);
				if (pEntity)
				{
					IPhysicalEntity *pPE=pEntity->GetPhysics();
					if (pPE&&(pPE->GetType()==PE_RIGID||pPE->GetType()==PE_PARTICLE))
					{
						Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
						Vec3 pos = GetFiringPos(hit);
						Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
						Vec3 vel = GetFiringVelocity(dir);

						// Use the speed of the desired projectile if available.
						float speed = 12.0f * m_speed_scale;
						CProjectile* pProjectile = static_cast<CProjectile*>(g_pGame->GetWeaponSystem()->GetProjectile(m_throwableId));
						if (pProjectile)
							speed = pProjectile->GetSpeed() * m_speed_scale;

						speed = max(2.0f, speed);

						pe_params_pos ppos;
						if(pProjectile)
						{
							ppos.pos = pos;
							pPE->SetParams(&ppos);
						}
						else
						{
							ppos.pos = pEntity->GetWorldPos();
							pPE->SetParams(&ppos);
						}

						pe_action_set_velocity asv;
						asv.v = (dir*speed)+vel;
						pPE->Action(&asv);

						CPlayer *pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());
						if(pPlayer && pPlayer->GetNanoSuit())
						{
							bool strengthMode = pPlayer->GetNanoSuit()->GetMode() == NANOMODE_STRENGTH;
							// Report throw to AI system.
							if (pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
							{
								SAIEVENT AIevent;
								AIevent.targetId = pEntity->GetId();
								pPlayer->GetEntity()->GetAI()->Event(strengthMode ? AIEVENT_PLAYER_STUNT_THROW : AIEVENT_PLAYER_THROW, &AIevent);
							}
						}
					}
					else if (pPE&&(pPE->GetType()==PE_LIVING||pPE->GetType()==PE_ARTICULATED))
					{
						
						Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
						Vec3 pos = GetFiringPos(hit);
						Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
						Vec3 vel = GetFiringVelocity(dir);

						CPlayer *pPlayer = static_cast<CPlayer*>(m_pWeapon->GetOwnerActor());
						if(pPlayer && pPlayer->GetNanoSuit())
						{
							float heavyness(0),vol(0),impulseMult(1.0f);
							pPlayer->CanPickUpObject(gEnv->pEntitySystem->GetEntityFromPhysics(pPE),heavyness,vol);
							if(vol>3.0f)
								impulseMult *= 2.5f;

							dir.Normalize();
							pe_action_impulse ip;
							if(pPlayer->GetNanoSuit()->GetMode()==NANOMODE_STRENGTH)
								ip.impulse=dir*1400.0f*impulseMult;
							else
								ip.impulse=dir*600.0f*impulseMult;

							pPE->Action(&ip);

							// Report throw to AI system.
							if (pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
							{
								SAIEVENT AIevent;
								AIevent.targetId = pEntity->GetId();
								pPlayer->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_THROW_NPC, &AIevent);
							}
						}
					}
				}

				if (m_throwableAction)
					m_throwableAction->execute(m_pWeapon);
			}
			else
			{
				m_pWeapon->SetBusy(false);
				Shoot(true);
				m_pWeapon->SetBusy(true);
			}
			m_throwing = false;
		}
		else if (m_thrown && m_throw_time<=0.0f)
		{
			m_pWeapon->SetBusy(false);
			m_pWeapon->HideItem(false);

			int ammoCount = m_pWeapon->GetAmmoCount(m_fireparams.ammo_type_class);
			if (ammoCount > 0)
				m_pWeapon->PlayAction(m_throwactions.next);
			else if (m_throwparams.auto_select_last)
				static_cast<CPlayer*>(m_pWeapon->GetOwnerActor())->SelectLastItem(true);

			m_firing = false;
			m_throwing = false;
			m_thrown = false;
		}

		m_throw_time -= frameTime;
		if (m_throw_time<0.0f)
			m_throw_time=0.0f;

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::ResetParams(const struct IItemParamsNode *params)
{
	CSingle::ResetParams(params);

	const IItemParamsNode *throwp = params?params->GetChild("throw"):0;
	const IItemParamsNode *throwa = params?params->GetChild("actions"):0;

	m_throwparams.Reset(throwp);
	m_throwactions.Reset(throwa);
}

//------------------------------------------------------------------------
void CThrow::PatchParams(const struct IItemParamsNode *patch)
{
	CSingle::PatchParams(patch);

	const IItemParamsNode *throwp = patch->GetChild("throw");
	const IItemParamsNode *throwa = patch->GetChild("actions");

	m_throwparams.Reset(throwp, false);
	m_throwactions.Reset(throwa, false);
}

//------------------------------------------------------------------------
void CThrow::Activate(bool activate)
{
	CSingle::Activate(activate);

	m_hold_timer=0.0f;

	m_thrown=false;
	m_pulling=false;
	m_throwing=false;
	m_firing=false;
	m_auto_throw=false;

	m_throwableId=0;
	m_throwableAction=0;

	CheckAmmo();
}

//------------------------------------------------------------------------
bool CThrow::CanFire(bool considerAmmo) const
{
	return CSingle::CanFire(considerAmmo);// cannot be changed. it's used in CSingle::Shoot()
}

//------------------------------------------------------------------------
bool CThrow::CanReload() const
{
	return CSingle::CanReload() && !m_throwing;
}

//------------------------------------------------------------------------
bool CThrow::IsReadyToFire() const
{
	return CanFire(true) && !m_firing && !m_throwing && !m_pulling && !m_thrown;
}

//------------------------------------------------------------------------
struct CThrow::StartThrowAction
{
	StartThrowAction(CThrow *_throw): pthrow(_throw) {};
	CThrow *pthrow;

	void execute(CItem *_this)
	{
		pthrow->m_pulling = false;
		pthrow->m_pWeapon->PlayAction(pthrow->m_throwactions.hold, 0, true, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}
};

void CThrow::StartFire(EntityId shooterId)
{
	if (m_pWeapon->IsBusy())
		return;

	m_shooterId = shooterId;

	if (CanFire(true) && !m_firing && !m_throwing && !m_pulling)
	{
		m_firing = true;
		m_pulling = true;
		m_throwing = false;
		m_thrown = false;
		m_auto_throw = false;

		m_pWeapon->SetBusy(true);
		m_pWeapon->PlayAction(m_throwactions.pull);

		m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson)+1, CSchedulerAction<StartThrowAction>::Create(this), false);
		m_pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, m_throwactions.hold);
		
		m_hold_timer=m_throwparams.hold_duration;

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::StopFire(EntityId shooterId)
{
	if (m_firing && !m_throwing && !m_thrown)
	{
		if (!m_pulling && !m_auto_throw)
		{
			m_auto_throw = false;
			if(m_hold_timer>0.0f && !m_usingGrenade)
				DoThrow(true);
			else
				DoThrow(false);
		}
		else if (m_pulling)
			m_auto_throw = true;

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::SetThrowable(EntityId entityId, ISchedulerAction *action)
{
	m_throwableId = entityId;
	m_throwableAction = action;
}

//------------------------------------------------------------------------
EntityId CThrow::GetThrowable() const
{
	return m_throwableId;
}

//------------------------------------------------------------------------
void CThrow::CheckAmmo()
{
	m_pWeapon->HideItem(!m_pWeapon->GetAmmoCount(m_fireparams.ammo_type_class) && m_throwparams.hide_ammo);
}

//------------------------------------------------------------------------
struct CThrow::ThrowAction
{
	ThrowAction(CThrow *_throw): pthrow(_throw) {};
	CThrow *pthrow;

	void execute(CItem *_this)
	{
		pthrow->m_thrown = true;
	}
};

void CThrow::DoThrow(bool drop)
{
	m_throw_time = m_throwparams.delay;
	m_throwing = true;
	m_thrown = false;
	m_auto_throw = false;

	if(!drop)
	{
		CItem::TempResourceName tmp(m_throwactions.throwit.c_str(), m_throwactions.throwit.length());
		tmp.replace("drop","throw");
		m_throwactions.throwit = tmp;
		m_pWeapon->PlayAction(m_throwactions.throwit);
	}
	else
	{
		CItem::TempResourceName tmp(m_throwactions.throwit.c_str(), m_throwactions.throwit.length());
		tmp.replace("throw","drop");
		m_throwactions.throwit = tmp;
		m_pWeapon->PlayAction(m_throwactions.throwit);
		m_throwing = false;
		DoDrop();
	}

	if(m_pWeapon->GetOwner())
	{
		CActor *owner = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_pWeapon->GetOwner()->GetId()));
		if (owner)
		{
			CPlayer *pPlayer = (CPlayer *)owner;
			if(CNanoSuit *pSuit = pPlayer->GetNanoSuit())
			{
				ENanoMode curMode = pSuit->GetMode();
				if (curMode == NANOMODE_STRENGTH)
				{
					IEntity *pThrowable = gEnv->pEntitySystem->GetEntity(m_throwableId);
					if(pThrowable)	//set sound intensity by item mass (sound request)
					{
						IPhysicalEntity *pEnt(pThrowable->GetPhysics());
						float mass(0);
						float massFactor = 0.3f;
						if (pEnt)
						{
							pe_status_dynamics dynStat;
							if (pEnt->GetStatus(&dynStat))
								mass = dynStat.mass;
							if(mass > pPlayer->GetActorParams()->maxGrabMass)
								massFactor = 1.0f;
							else if(mass > 30)
								massFactor = 0.6f;
						}
						if(!drop)
						{
							pSuit->PlaySound(STRENGTH_THROW_SOUND, massFactor);
							pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-(40.0f*massFactor));
						}
					}
					else if(!drop)
						pSuit->PlaySound(STRENGTH_THROW_SOUND, (pSuit->GetSlotValue(NANOSLOT_STRENGTH))*0.01f);
				}
			}
		}
	}

	m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(CItem::eIGS_FirstPerson), CSchedulerAction<ThrowAction>::Create(this), false);
	m_pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, g_pItemStrings->idle);
}

//--------------------------------------
void CThrow::DoDrop()
{

	m_pWeapon->HideItem(true);
	if (m_throwableId)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_throwableId);
		if (pEntity)
		{
			IPhysicalEntity *pPE=pEntity->GetPhysics();
			if (pPE&&(pPE->GetType()==PE_RIGID||pPE->GetType()==PE_PARTICLE))
			{
				Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
				Vec3 pos = GetFiringPos(hit);

				CActor *pActor = m_pWeapon->GetOwnerActor();
				IMovementController * pMC = pActor ? pActor->GetMovementController() : 0;
				if (pMC)
				{
					SMovementState info;
					pMC->GetMovementState(info);
					float speed=2.0f;

					//pe_params_pos ppos;
					//ppos.pos = pos+(info.eyeDirection*0.10f);
					//pPE->SetParams(&ppos);

					pe_action_set_velocity asv;
					asv.v = (info.eyeDirection*speed);
					pPE->Action(&asv);
				}
			}
		}

		if (m_throwableAction)
			m_throwableAction->execute(m_pWeapon);
	}
}

//---------------------------------------------------
void CThrow::PlayDropThrowSound()
{
	if(CActor* pOwner = m_pWeapon->GetOwnerActor())
	{
		COffHand * pOffHand=static_cast<COffHand*>(pOwner->GetWeaponByClass(CItem::sOffHandClass));
		if(pOffHand)
		{
			if(pOffHand->GetOffHandState()==eOHS_THROWING_OBJECT)
			{
				CPlayer *pPlayer = static_cast<CPlayer*>(pOwner);
				if(CNanoSuit* pSuit = pPlayer->GetNanoSuit())
				{
					pSuit->PlaySound(DROP_VS_THROW_SOUND);
				}
			}
		}
	}
}

void CThrow::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	CSingle::GetMemoryStatistics(s);
	m_throwactions.GetMemoryStatistics(s);
}
