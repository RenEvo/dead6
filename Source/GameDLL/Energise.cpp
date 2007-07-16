/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 01:11:2006:		Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Energise.h"
#include "Game.h"
#include "Item.h"
#include "Weapon.h"
#include "GameRules.h"
#include <IEntitySystem.h>
#include <IWorldQuery.h>


//------------------------------------------------------------------------
CEnergise::CEnergise() :
	m_energising(false),
	m_firing(false)
{
}

//------------------------------------------------------------------------
CEnergise::~CEnergise()
{
}

//------------------------------------------------------------------------
void CEnergise::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);

	m_energy = 0.0f;
	m_energising = false;
	m_firing = false;
	m_lastEnergisingId = 0;
	m_effectEntityId = 0;	
	m_effectSlot = 0;
	m_soundId = 0;
}

//------------------------------------------------------------------------
void CEnergise::Update(float frameTime, uint frameId)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	if (!m_firing)
		return;

	bool requireUpdate=false;

	CActor *pActor=m_pWeapon->GetOwnerActor();

	if (m_delayTimer>0.0f)
	{
		m_energising=false;
		m_delayTimer -= frameTime;

		if (m_delayTimer<=0.0f)
		{
			m_delayTimer=0.0f;
			m_energising=true;

			if (m_pWeapon->IsClient())
			{
				m_pWeapon->PlayAction(m_energiseactions.prefire.c_str());

				if (m_soundId!=INVALID_SOUNDID)
				{
					if (ISound *pSound=m_pWeapon->GetISound(m_soundId))
					{
						pSound->SetLoopMode(true);
						pSound->SetPaused(false);
					}
				}
			}
		}
	}

	if (m_delayTimer<=0.0f && m_energising && m_pWeapon->IsClient())
	{
		if (m_effectEntityId)
			UpdateCollectEffect(gEnv->pEntitySystem->GetEntity(m_effectEntityId));
	}

	if (m_delayTimer<=0.0f && m_energising && m_pWeapon->IsServer())
	{
		bool keepEnergising=true;

		if (IEntity	*pEntity=CanEnergise())
		{
			if (pEntity->GetId() == m_lastEnergisingId)
			{
				float energy=CallEnergise(pEntity, frameTime);

				m_energy=m_energy+energy;
				m_energy=CLAMP(m_energy, 0.0f, 1.0f);

				//CryLogAlways("energy: %.3f", m_energy);

				m_pWeapon->GetGameObject()->ChangedNetworkState(CWeapon::ASPECT_FIREMODE);

				if ((m_energy==0.0f) || (m_energy==1.0f) || (energy==0.0f))
					keepEnergising=false;
			}
			else
			{
				if (IEntity *pLast=gEnv->pEntitySystem->GetEntity(m_lastEnergisingId))
					CallStopEnergise(pLast);

				if (CallCanEnergise(pEntity, m_energy) && CallStartEnergise(pEntity))
				{
					m_lastEnergisingId=pEntity->GetId();

					m_pWeapon->GetGameObject()->InvokeRMI(CWeapon::ClStartFire(), CWeapon::EmptyParams(), eRMI_ToAllClients|eRMI_NoLocalCalls);
			
					if (m_pWeapon->IsClient())
						EnergyEffect(true);
				}
				else
					keepEnergising=false;
			}
		}
		else
			keepEnergising=false;

		if (!keepEnergising)
		{
			m_pWeapon->GetGameObject()->InvokeRMI(CWeapon::ClStopFire(), CWeapon::EmptyParams(), eRMI_ToAllClients|eRMI_NoLocalCalls);

			Energise(false);

			if (m_pWeapon->IsClient())
			{
				EnergyEffect(false);

				StopEnergise();
			}
		}

		requireUpdate=true;
	}

	if (requireUpdate)
		m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CEnergise::UpdateFPView(float frameTime)
{
	float col[4]={1.0f-m_energy,m_energy,0,1};
	static char energyText[256];
	sprintf(energyText, "%d%%", (int)(m_energy*100.0f));

	Vec3 wp=gEnv->pSystem->GetViewCamera().GetPosition()+gEnv->pSystem->GetViewCamera().GetViewdir();
	//wp-=gEnv->pSystem->GetViewCamera().GetViewMatrix().GetColumn2().normalized()*0.15f;
	gEnv->pRenderer->DrawLabelEx(wp,2.2f,col,true,true,energyText);
}


//------------------------------------------------------------------------
void CEnergise::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CEnergise::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *energise = params?params->GetChild("energise"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;

	m_energiseparams.Reset(energise);
	m_energiseactions.Reset(actions);
}

//------------------------------------------------------------------------
void CEnergise::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *energise = patch->GetChild("energise");
	const IItemParamsNode *actions = patch->GetChild("actions");

	m_energiseparams.Reset(energise, false);
	m_energiseactions.Reset(actions, false);
}

//------------------------------------------------------------------------
void CEnergise::Activate(bool activate)
{
	m_energising = false;
	m_delayTimer = 0.0f;
	m_lastEnergisingId = 0;

	EnergyEffect(false);

	if (m_soundId!=INVALID_SOUNDID)
		m_pWeapon->StopSound(m_soundId);
	m_soundId=INVALID_SOUNDID;
}

//------------------------------------------------------------------------
bool CEnergise::CanFire(bool considerAmmo) const
{
	return !m_energising && m_delayTimer<=0.0f;
}

//------------------------------------------------------------------------
void CEnergise::StartFire(EntityId shooterId)
{
	if (m_pWeapon->IsBusy() && CanFire(false))
		return;

	m_firing=true;
	m_pWeapon->SetBusy(true);
	m_soundId=m_pWeapon->PlayAction(m_energiseactions.energise.c_str(), 0, true, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending|CItem::eIPAF_SoundStartPaused);

	m_pWeapon->EnableUpdate(true, eIUS_FireMode);	

	m_delayTimer=m_energiseparams.delay;

	if (!m_pWeapon->IsServer())
		m_pWeapon->RequestStartFire();
}

//------------------------------------------------------------------------
void CEnergise::StopFire(EntityId shooterId)
{
	if (m_firing)
	{
		StopEnergise();
		m_pWeapon->EnableUpdate(false, eIUS_FireMode);

		if (!m_pWeapon->IsServer())
			m_pWeapon->RequestStopFire();
		else
			Energise(false);

		EnergyEffect(false);
	}
	m_firing=false;
}

//------------------------------------------------------------------------
void CEnergise::StopEnergise()
{
	m_pWeapon->SetBusy(false);
	if (m_energising)
		m_pWeapon->PlayAction(m_energiseactions.postfire.c_str());
	m_pWeapon->PlayAction(m_energiseactions.idle.c_str(), 0, true, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending);

	if (m_soundId!=INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_soundId);
		m_soundId=INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
void CEnergise::NetShoot(const Vec3 &hit, int ph)
{
	NetShootEx(ZERO, ZERO, ZERO, hit, ph);
}

//------------------------------------------------------------------------
void CEnergise::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, int ph)
{
	Shoot(pos, dir, vel);
}

//------------------------------------------------------------------------
void CEnergise::Shoot(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel)
{
}

//------------------------------------------------------------------------
void CEnergise::NetStartFire(EntityId shooterId)
{
	m_pWeapon->EnableUpdate(true, eIUS_FireMode);

	if (m_pWeapon->IsClient() && m_energising)
	{
		EnergyEffect(true);
	}
	else if (m_pWeapon->IsServer())
		m_delayTimer=m_energiseparams.delay;
	m_firing=true;
}

//------------------------------------------------------------------------
void CEnergise::NetStopFire(EntityId shooterId)
{
	m_pWeapon->EnableUpdate(false, eIUS_FireMode);

	if (m_pWeapon->IsClient() && m_energising)
	{
		EnergyEffect(false);

		StopEnergise();
	}

	Energise(false);

	m_firing=false;
}

//------------------------------------------------------------------------
const char *CEnergise::GetType() const
{
	return "Energise";
}

//------------------------------------------------------------------------
int CEnergise::GetDamage() const
{
	return 0;
}

//------------------------------------------------------------------------
void CEnergise::Energise(bool enable)
{
	if (enable && !m_energising)
	{
		m_energising = true;
	}
	else if (!enable && m_energising)
	{
		m_energising = false;

		if (m_pWeapon->IsServer())
		{
			if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_lastEnergisingId))
				CallStopEnergise(pEntity);
		}

		m_lastEnergisingId = 0;
	}
}

//------------------------------------------------------------------------
void CEnergise::UpdateCollectEffect(IEntity *pEntity)
{
	if (!pEntity)
		return;

	if (IParticleEmitter *pEmitter=pEntity->GetParticleEmitter(m_effectSlot))
	{
		int slot = m_pWeapon->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
		int id = m_pWeapon->GetStats().fp ? 0 : 1;

		ParticleTarget target;
		target.vTarget=m_pWeapon->GetSlotHelperPos(slot, m_energiseparams.collect_effect.helper[id], true);
		target.bPriority=true;
		target.bExtendSpeed=true;
		target.bStretch=true;
		target.bTarget=true;

		pEmitter->SetTarget(target);
	}
}

//------------------------------------------------------------------------
void CEnergise::EnergyEffect(bool enable)
{
	if (enable)
	{
		Vec3 center;
		float radius;
		bool charged=true;

		if (IEntity *pEntity=CanEnergise(&center, &radius, &charged))
		{
			int slot = m_pWeapon->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
			int id = m_pWeapon->GetStats().fp ? 0 : 1;

			if (charged)
			{
				// the collect effect is a bit more complex... it needs to live on the target entity
				// and have the collector's tip as target... we create the emitter here and then keep updating the target...
				if (IParticleEffect *pEffect = gEnv->p3DEngine->FindParticleEffect(m_energiseparams.collect_effect.effect[id]))
				{
					m_effectSlot=pEntity->LoadParticleEmitter(-1, pEffect);
					m_effectEntityId=pEntity->GetId();

					Matrix34 worldMatrix = pEntity->GetWorldTM();
					Matrix34 localMatrix = worldMatrix.GetInverted()*Matrix34::CreateTranslationMat(center);

					pEntity->SetSlotLocalTM(m_effectSlot, localMatrix);

					UpdateCollectEffect(pEntity);
				}
			}
			else
			{
				m_effectId = m_pWeapon->AttachEffect(slot, 0, true, m_energiseparams.energise_effect.effect[id].c_str(), 
					m_energiseparams.energise_effect.helper[id].c_str());

				if (IParticleEmitter *pEmitter=m_pWeapon->GetEffectEmitter(m_effectId))
				{
					ParticleTarget target;
					target.vTarget=center;
					target.bPriority=true;
					target.bExtendSpeed=true;
					target.bStretch=true;
					target.bTarget=true;

					pEmitter->SetTarget(target);
				}
			}
		}
	}
	else
	{
		if (m_effectId)
			m_pWeapon->AttachEffect(0, m_effectId, false);

		if (m_effectEntityId)
		{
			if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_effectEntityId))
				pEntity->FreeSlot(m_effectSlot);

			m_effectEntityId=0;
			m_effectSlot=0;
		}
	}
}

//------------------------------------------------------------------------
IEntity *CEnergise::CanEnergise(Vec3 *pCenter, float *pRadius, bool *pCharged) const
{
	CActor *pActor=m_pWeapon->GetOwnerActor();
	IMovementController * pMC = pActor?pActor->GetMovementController():0;
	if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);

		Vec3 pos=info.eyePosition;
		Vec3 dir=info.eyeDirection;

		SEntityProximityQuery query;
		Vec3	c(pos);
		float l(dir.len()+2.0f*m_energiseparams.range);

		query.box = AABB(Vec3(c.x-l,c.y-l,c.z-l), Vec3(c.x+l,c.y+l,c.z+l));
		query.nEntityFlags = -1;
		query.pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AlienEnergyPoint");
		gEnv->pEntitySystem->QueryProximity(query);

		for(int i=0; i<query.nCount; i++)
		{
			IEntity *pEntity = query.pEntities[i];
			IScriptTable *pScriptTable=pEntity->GetScriptTable();			
			if (!pScriptTable)
				continue;

			float radius=0.0f;
			Vec3 center;

			HSCRIPTFUNCTION getRadius=0;
			if (!pScriptTable->GetValue("GetRadius", getRadius))
				continue;

			Script::CallReturn(gEnv->pScriptSystem, getRadius, pScriptTable, radius);
			gEnv->pScriptSystem->ReleaseFunc(getRadius);

			HSCRIPTFUNCTION getCenter=0;
			if (!pScriptTable->GetValue("GetCenter", getCenter))
				continue;

			Script::CallReturn(gEnv->pScriptSystem, getCenter, pScriptTable, center);
			gEnv->pScriptSystem->ReleaseFunc(getCenter);

			if (Overlap::Lineseg_Sphere(Lineseg(pos-dir*0.1f, pos+dir*(m_energiseparams.range+0.1f)), Sphere(center, radius)))
			{
				if (pCenter)
					*pCenter=center;
				if (pRadius)
					*pRadius=radius;
				if (pCharged)
					pScriptTable->GetValue("charged", *pCharged);

				return pEntity;
			}
		}
	}

	return 0;
}

//------------------------------------------------------------------------
int CEnergise::CallStartEnergise(IEntity *pEntity)
{
	int result=0;

	if (IScriptTable *pScriptTable=pEntity->GetScriptTable())
	{	
		HSCRIPTFUNCTION pfnStartEnergise=0;
		if (pScriptTable->GetValue("StartEnergise", pfnStartEnergise))
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnStartEnergise, pScriptTable,
				ScriptHandle(m_pWeapon->GetOwnerId()), result);
			gEnv->pScriptSystem->ReleaseFunc(pfnStartEnergise);
		}
	}
	return result;
}

//------------------------------------------------------------------------
float CEnergise::CallEnergise(IEntity *pEntity, float frameTime)
{
	float result=0;

	if (IScriptTable *pScriptTable=pEntity->GetScriptTable())
	{	
		HSCRIPTFUNCTION pfnEnergise=0;
		if (pScriptTable->GetValue("Energise", pfnEnergise))
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnEnergise, pScriptTable, frameTime,
				ScriptHandle(m_pWeapon->GetOwnerId()), ScriptHandle(m_pWeapon->GetEntityId()), m_energy, result);
			gEnv->pScriptSystem->ReleaseFunc(pfnEnergise);
		}
	}

	return result;
}


//------------------------------------------------------------------------
float CEnergise::CallGetEnergy(IEntity *pEntity) const
{
	float result=0;

	if (IScriptTable *pScriptTable=pEntity->GetScriptTable())
	{	
		HSCRIPTFUNCTION pfnGetEnergy=0;
		if (pScriptTable->GetValue("GetEnergy", pfnGetEnergy))
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnGetEnergy, pScriptTable, result);
			gEnv->pScriptSystem->ReleaseFunc(pfnGetEnergy);
		}
	}

	return result;
}

//------------------------------------------------------------------------
bool CEnergise::CallCanEnergise(IEntity *pEntity, float energy) const
{
	bool result=false;

	if (IScriptTable *pScriptTable=pEntity->GetScriptTable())
	{	
		HSCRIPTFUNCTION pfnCanEnergise=0;
		if (pScriptTable->GetValue("CanEnergise", pfnCanEnergise))
		{
			Script::CallReturn(gEnv->pScriptSystem, pfnCanEnergise, pScriptTable, energy, 
				ScriptHandle(m_pWeapon->GetOwnerId()), result);
			gEnv->pScriptSystem->ReleaseFunc(pfnCanEnergise);
		}
	}

	return result;
}


//------------------------------------------------------------------------
void CEnergise::CallStopEnergise(IEntity *pEntity)
{
	if (IScriptTable *pScriptTable=pEntity->GetScriptTable())
	{
		HSCRIPTFUNCTION pfnStopEnergise=0;
		if (pScriptTable->GetValue("StopEnergise", pfnStopEnergise))
		{
			Script::Call(gEnv->pScriptSystem, pfnStopEnergise, pScriptTable, ScriptHandle(m_pWeapon->GetOwnerId()));
			gEnv->pScriptSystem->ReleaseFunc(pfnStopEnergise);
		}
	}
}

//------------------------------------------------------------------------
void CEnergise::Serialize(TSerialize ser)
{
	if (ser.GetSerializationTarget()==eST_Network)
	{
		ser.Value("energy", m_energy, 'unit');
		
		if (ser.IsReading())
		{
			//CryLogAlways("energy: %.3f", m_energy);
			// update the client effects here
		}
	}
}

//------------------------------------------------------------------------
void CEnergise::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_name);
	m_energiseparams.GetMemoryStatistics(s);
	m_energiseactions.GetMemoryStatistics(s);
}
