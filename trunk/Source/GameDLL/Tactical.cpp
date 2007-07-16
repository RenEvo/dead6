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
#include "Tactical.h"
#include "Game.h"
#include <ISound.h>
#include <ISound.h>
#include "Actor.h"
#include "TacticalAttachment.h"
#include "GameRules.h"


//------------------------------------------------------------------------
CTactical::CTactical()
: m_tacEffect(0)
{
}

//------------------------------------------------------------------------
CTactical::~CTactical()
{
	if (m_tacEffect)
		delete m_tacEffect;
}

//------------------------------------------------------------------------
void CTactical::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);
}

//------------------------------------------------------------------------
void CTactical::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CTactical::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *tac = params?params->GetChild("tactical"):0;
	m_tacparams.Reset(tac);

	if (m_tacEffect)
	{
		delete m_tacEffect;
		m_tacEffect = 0;
	}
	if (m_tacparams.tac_effect == "sleep")
		m_tacEffect = new SSleepEffect();
	else if (m_tacparams.tac_effect == "kill")
		m_tacEffect = new SKillEffect();
	else if (m_tacparams.tac_effect == "sound")
		m_tacEffect = new SSoundEffect();
}

//------------------------------------------------------------------------
void CTactical::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *tac = patch->GetChild("tactical");
	m_tacparams.Reset(tac, false);

	if (m_tacEffect)
	{
		delete m_tacEffect;
		m_tacEffect = 0;
	}
	if (m_tacparams.tac_effect == "sleep")
		m_tacEffect = new SSleepEffect();
	else if (m_tacparams.tac_effect == "kill")
		m_tacEffect = new SKillEffect();
	else if (m_tacparams.tac_effect == "sound")
		m_tacEffect = new SSoundEffect();
}

//------------------------------------------------------------------------
const char *CTactical::GetType() const
{
	return "Tactical";
}

//------------------------------------------------------------------------
void CTactical::Activate(bool activate)
{
}

//------------------------------------------------------------------------
void CTactical::StartFire(EntityId shooterId)
{
	if (!m_pWeapon->IsBusy())
	{
		m_pWeapon->RequestStartFire();
		if (m_pWeapon->IsServer())
			NetStartFire(shooterId);
	}
}

//------------------------------------------------------------------------
void CTactical::NetStartFire(EntityId shooterId)
{
	if (!m_pWeapon->IsServer())
		return;

	if (!m_tacEffect)
		return;

	//if (CTacticalAttachment *pTacticalAttachment = static_cast<CTacticalAttachment *>(m_pWeapon->GetAccessory("TacticalAttachment")))
		//pTacticalAttachment->RunEffectOnHumanTargets(m_tacEffect, shooterId);
	//if (CTacticalAttachment *pTacticalAttachment = static_cast<CTacticalAttachment *>(m_pWeapon->GetAccessory("TacticalAttachmentEMP")))
		//pTacticalAttachment->RunEffectOnAlienTargets(m_tacEffect, shooterId);
}

//------------------------------------------------------------------------
void CTactical::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(m_name);
	m_tacparams.GetMemoryStatistics(s);
}

//------------------------------------------------------------------------
void SSleepEffect::Activate(EntityId targetId, EntityId ownerId, EntityId weaponId, const char *effect, const char *defaultEffect)
{
	CActor *pActor = (CActor *)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetId);
	
	if (pActor)
	{
		IAISystem *pAISystem=gEnv->pAISystem;
		if (pAISystem)
		{
			if(IEntity* pEntity=pActor->GetEntity())
			{
				if(IAIObject* pAIObj=pEntity->GetAI())
				{
					IAISignalExtraData *pEData = pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SendAnonymousSignal
					// try to retrieve the shooter position
					if (IEntity* pOwnerEntity = gEnv->pEntitySystem->GetEntity(ownerId))
						pEData->point = pOwnerEntity->GetWorldPos();
					else
						pEData->point = pEntity->GetWorldPos();
					IAIActor* pAIActor = pAIObj->CastToIAIActor();
					if(pAIActor)
						pAIActor->SetSignal(1,"TRANQUILIZED",0,pEData);
				}
			}
		}

		pActor->CreateScriptEvent("sleep", 0);
		pActor->GetGameObject()->SetPhysicalizationProfile(eAP_Sleep);

		// no dropping weapons for AI
		if(pActor->IsPlayer())
			pActor->DropItem(pActor->GetCurrentItemId(), 1.0f, false);

		pActor->SetSleepTimer(12.5f);
	}
}

//------------------------------------------------------------------------
void SKillEffect::Activate(EntityId targetId, EntityId ownerId, EntityId weaponId, const char *effect, const char *defaultEffect)
{
	CActor *pActor = (CActor *)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(targetId);

	if (pActor)
	{
		//Execute particle effect and kill the actor
		GameWarning("Explosion effect: %s (Actor)", effect);

		// TODO: make work in MP...
		IParticleEffect *pEffect = gEnv->p3DEngine->FindParticleEffect(effect);
		if (pEffect)
		{

			int effectId = 0;

			effectId = pActor->GetEntity()->LoadParticleEmitter(-1,pEffect);
			AABB box;
			pActor->GetEntity()->GetLocalBounds(box);
			Matrix34 tm = IParticleEffect::ParticleLoc(box.GetCenter());
			pActor->GetEntity()->SetSlotLocalTM(effectId, tm);
		}	

		HitInfo info(ownerId, targetId, weaponId, 1000.0f, 0.0f, 0, 0, 0);
	
		g_pGame->GetGameRules()->ServerHit(info);
	}
	else if(!pActor && targetId)
	{
		// TODO: make work in MP...

		//No actor, but we have an entity
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(targetId);
		if(pEntity)
		{
			//Execute particle effect and kill the actor
			GameWarning("Explosion effect: %s (Entity)", defaultEffect);

			IParticleEffect *pEffect = gEnv->p3DEngine->FindParticleEffect(defaultEffect);
			if (pEffect)
			{
				int effectId = 0;

				effectId = pEntity->LoadParticleEmitter(-1,pEffect);
				AABB box;
				pEntity->GetLocalBounds(box);
				Matrix34 tm = IParticleEffect::ParticleLoc(box.GetCenter());
				pEntity->SetSlotLocalTM(effectId, tm);
			}	
		}

	}
}

//------------------------------------------------------------------------
void SSoundEffect::Activate(EntityId targetId, EntityId ownerId, EntityId weaponId, const char *effect, const char *defaultEffect)
{
	// TODO: make work in MP...

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(targetId);
	IEntity* pOwnerEntity = gEnv->pEntitySystem->GetEntity(ownerId);
	
	if(pEntity && gEnv->pSoundSystem && pOwnerEntity)
	{
		int		soundFlag = FLAG_SOUND_3D; 
		bool	repeating = false;
		bool	force3DSound = true;

		IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)pOwnerEntity->GetProxy(ENTITY_PROXY_SOUND);

		if(pSoundProxy)
		{
			//Execute sound at entity position
			GameWarning("Sound effect: %s (Entity)", defaultEffect);
			pSoundProxy->PlaySound(defaultEffect, pEntity->GetWorldPos()-pOwnerEntity->GetWorldPos(),FORWARD_DIRECTION, FLAG_SOUND_DEFAULT_3D);

			// Notify AI system (TODO: Parametrize radius)
			gEnv->pAISystem->SoundEvent(pEntity->GetWorldPos(), 100.0f, AISE_GENERIC, NULL);
			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(pEntity->GetWorldPos(),0.5f,ColorB(255,0,0));
		}
	}
}
