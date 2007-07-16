/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:1:2007   15:17 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Scan.h"
#include "Game.h"
#include "Actor.h"
#include "GameRules.h"
#include "IEntityProxy.h"
#include "IMaterial.h"
#include "HUD/HUD.h"
#include <IFlashPlayer.h>

#include <ISound.h>


//------------------------------------------------------------------------
CScan::CScan()
:	m_scanning(false),
	m_delayTimer(0.0f),
	m_durationTimer(0.0f),
	m_scanLoopId(INVALID_SOUNDID)
{
}

//------------------------------------------------------------------------
CScan::~CScan()
{
}

//------------------------------------------------------------------------
void CScan::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	if (params)
		ResetParams(params);
}

//------------------------------------------------------------------------
void CScan::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CScan::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *scan = params?params->GetChild("scan"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;
	m_scanparams.Reset(scan);
	m_scanactions.Reset(actions);
}

//------------------------------------------------------------------------
void CScan::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *scan = patch->GetChild("scan");
	const IItemParamsNode *actions = patch->GetChild("actions");
	m_scanparams.Reset(scan, false);
	m_scanactions.Reset(actions, false);
}

//------------------------------------------------------------------------
const char *CScan::GetType() const
{
	return "Scan";
}

//------------------------------------------------------------------------
void CScan::Activate(bool activate)
{
	m_scanning=false;
	m_delayTimer=m_durationTimer=0.0f;

	if (m_scanLoopId != INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_scanLoopId);
		m_scanLoopId = INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
void CScan::Update(float frameTime, uint frameId)
{
	if (m_scanning && m_pWeapon->IsClient())
	{
		if (m_delayTimer>0.0f)
		{
			m_delayTimer -= frameTime;
			if (m_delayTimer>0.0f)
				return;

			m_delayTimer = 0.0f;

			int slot = m_pWeapon->GetStats().fp ? CItem::eIGS_FirstPerson : CItem::eIGS_ThirdPerson;
			int id = m_pWeapon->GetStats().fp ? 0 : 1;

			m_scanLoopId=m_pWeapon->PlayAction(m_scanactions.scan, 0, true, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending);
			ISound *pSound = m_pWeapon->GetSoundProxy()->GetSound(m_scanLoopId);
			if (pSound)
				pSound->SetLoopMode(true);
		}

		if(m_delayTimer==0.0f)
		{
			if (m_durationTimer>0.0f)
			{
				m_durationTimer-=frameTime;
				if (m_durationTimer<=0.0f)
				{
					m_durationTimer=0.0f;
					
					StopFire(0);
					m_pWeapon->RequestShoot(0, ZERO, ZERO, ZERO, ZERO, 0, false);
				}
			}
		}

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CScan::StartFire(EntityId shooterId)
{
	if (!m_pWeapon->IsBusy())
	{
		if(m_pWeapon->GetOwnerActor())
		{
			// add the flash animation part here
			IEntity *pEntity = m_pWeapon->GetEntity();
			if(pEntity)
			{
				IEntityRenderProxy* pRenderProxy((IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER));
				if (pRenderProxy)
				{
					IMaterial* pMtl(pRenderProxy->GetRenderMaterial(0));
					if (pMtl)
					{
						pMtl = pMtl->GetSafeSubMtl(2);
						if (pMtl)
						{
							const SShaderItem& shaderItem(pMtl->GetShaderItem());
							if (shaderItem.m_pShaderResources && shaderItem.m_pShaderResources->m_Textures[0])
							{
								SEfResTexture* pTex(shaderItem.m_pShaderResources->m_Textures[0]);
								if (pTex->m_Sampler.m_pDynTexSource)
								{
									IFlashPlayer* pFlashPlayer(0);
									IDynTextureSource::EDynTextureSource type(IDynTextureSource::DTS_I_FLASHPLAYER);

									pTex->m_Sampler.m_pDynTexSource->GetDynTextureSource((void*&)pFlashPlayer, type);
									if (pFlashPlayer && type == IDynTextureSource::DTS_I_FLASHPLAYER)
									{
										pFlashPlayer->Invoke0("startScan");
									}
								}
							}
						}
					}
				}
			}

			CHUD *pHUD = g_pGame->GetHUD();
			if(pHUD)
				pHUD->SetRadarScanningEffect(true);

			m_scanning=true;
			m_delayTimer=m_scanparams.delay;
			m_durationTimer=m_scanparams.duration;
			m_pWeapon->SetBusy(true);

			//request shooting for the local client
			if(gEnv->pGame->GetIGameFramework()->GetClientActor() == m_pWeapon->GetOwnerActor())
			{
				Vec3 hit;
				NetShoot(hit, 0);
			}
		}

		m_pWeapon->PlayAction(m_scanactions.spin_up, 0, false, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending);
		m_pWeapon->RequestStartFire();
		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CScan::StopFire(EntityId shooterId)
{
	if (!m_scanning)
		return;

	IEntity *pEntity = m_pWeapon->GetEntity();
	if(pEntity)
	{
		IEntityRenderProxy* pRenderProxy((IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER));
		if (pRenderProxy)
		{
			IMaterial* pMtl(pRenderProxy->GetRenderMaterial(0));
			if (pMtl)
			{
				pMtl = pMtl->GetSafeSubMtl(2);
				if (pMtl)
				{
					const SShaderItem& shaderItem(pMtl->GetShaderItem());
					if (shaderItem.m_pShaderResources && shaderItem.m_pShaderResources->m_Textures[0])
					{
						SEfResTexture* pTex(shaderItem.m_pShaderResources->m_Textures[0]);
						if (pTex->m_Sampler.m_pDynTexSource)
						{
							IFlashPlayer* pFlashPlayer(0);
							IDynTextureSource::EDynTextureSource type(IDynTextureSource::DTS_I_FLASHPLAYER);

							pTex->m_Sampler.m_pDynTexSource->GetDynTextureSource((void*&)pFlashPlayer, type);
							if (pFlashPlayer && type == IDynTextureSource::DTS_I_FLASHPLAYER)
							{
								pFlashPlayer->Invoke0("cancelScan");
							}
						}
					}
				}
			}
		}
	}

	CHUD *pHUD = g_pGame->GetHUD();
	if(pHUD)
		pHUD->SetRadarScanningEffect(false);

	m_pWeapon->PlayAction(m_scanactions.spin_down, 0, false, CItem::eIPAF_Default|CItem::eIPAF_CleanBlending);

	m_scanning=false;
	m_pWeapon->SetBusy(false);
	m_pWeapon->RequestStopFire();

	if (m_scanLoopId != INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_scanLoopId);
		m_scanLoopId = INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
void CScan::NetStartFire(EntityId shooterId)
{
	if (!m_pWeapon->IsClient())
		return;

	m_pWeapon->PlayAction(m_scanactions.scan);
}

//------------------------------------------------------------------------
void CScan::NetStopFire(EntityId shooterId)
{
	if (!m_pWeapon->IsClient())
		return;

	if (m_scanLoopId != INVALID_SOUNDID)
	{
		m_pWeapon->StopSound(m_scanLoopId);
		m_scanLoopId = INVALID_SOUNDID;
	}
}

//------------------------------------------------------------------------
void CScan::NetShoot(const Vec3 &hit, int ph)
{
	if (m_pWeapon->IsServer())
	{
		SEntityProximityQuery query;

		float radius=m_scanparams.range;
		Vec3 pos=m_pWeapon->GetEntity()->GetWorldPos();
		query.box = AABB(Vec3(pos.x-radius,pos.y-radius,pos.z-radius), Vec3(pos.x+radius,pos.y+radius,pos.z+radius));
		query.nEntityFlags = ENTITY_FLAG_ON_RADAR; // Filter by entity flag.
		gEnv->pEntitySystem->QueryProximity( query );

		IEntity *pOwner=m_pWeapon->GetOwner();
		EntityId ownerId=pOwner->GetId();

		for(int i=0; i<query.nCount; i++)
		{
			IEntity *pEntity = query.pEntities[i];
			if(pEntity && pEntity != pOwner && !pEntity->IsHidden())
			{
				CActor *pActor=m_pWeapon->GetActor(pEntity->GetId());
				if (pActor && pActor->GetActorClass()==CPlayer::GetActorClassType())
				{
					CPlayer *pPlayer=static_cast<CPlayer *>(pActor);
					CNanoSuit *pSuit=pPlayer->GetNanoSuit();
					if (pSuit && pSuit->GetMode()==NANOMODE_CLOAK && pSuit->GetCloak()->GetType()==CLOAKMODE_REFRACTION)
						continue;
				}

				g_pGame->GetGameRules()->AddTaggedEntity(ownerId, pEntity->GetId(), true);
			}
		}
	}
}

//------------------------------------------------------------------------
void CScan::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(m_name);
	m_scanparams.GetMemoryStatistics(s);
	m_scanactions.GetMemoryStatistics(s);
}