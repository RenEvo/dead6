/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 9:12:2005   10:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "Weapon.h"
#include "Player.h"
#include "HUD/HUD.h"
#include "GameRules.h"

#include <IActorSystem.h>
#include <IAISystem.h>
#include <IAgent.h>

CWeapon::TEventListenerVector * CWeapon::m_listenerCache = 0;
bool CWeapon::m_listenerCacheInUse = false;

#define BROADCAST_WEAPON_EVENT(event, params)	\
	if (!m_listenerCacheInUse) \
	{ \
		m_listenerCacheInUse = true; \
		if (!m_listenerCache) m_listenerCache = new TEventListenerVector; \
		*m_listenerCache = m_listeners; \
		for (TEventListenerVector::const_iterator it=m_listenerCache->begin(); it!=m_listenerCache->end(); ++it)	\
			it->pListener->event params; \
		m_listenerCacheInUse = false; \
	} \
	else \
	{ \
		TEventListenerVector listeners(m_listeners); \
		for (TEventListenerVector::const_iterator it=listeners.begin(); it!=listeners.end(); ++it)	\
			it->pListener->event params; \
	}

//------------------------------------------------------------------------
void CWeapon::OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3&vel)
{
	BROADCAST_WEAPON_EVENT(OnShoot, (this, shooterId, ammoId, pAmmoType, pos, dir, vel));

	//FIXME:quick temporary solution
	CActor *pActor = static_cast<CActor*> (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId));
	if (pActor)
		pActor->HandleEvent(SGameObjectEvent(eCGE_OnShoot,eGOEF_ToExtensions));

	IActor *pClientActor=m_pGameFramework->GetClientActor();

	if (pActor && pActor->GetActorClass() == CPlayer::GetActorClassType() && IsServer())
	{
		if (pActor == pClientActor)
		{
			if (IAIObject *pAIObject=pActor->GetEntity()->GetAI())
				gEnv->pAISystem->SendSignal(SIGNALFILTER_LEADER, 1, "OnEnableFire",	pAIObject, 0);
		}

		CPlayer *pPlayer=static_cast<CPlayer *>(pActor);
		CNanoSuit *pSuit=pPlayer->GetNanoSuit();
		if(pSuit)
		{
			if (pSuit->GetMode() == NANOMODE_STRENGTH && !IsMounted())
				pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-3.0f);
			else if(pSuit->GetMode() == NANOMODE_CLOAK)
				pSuit->SetSuitEnergy(pSuit->GetSuitEnergy()-25.0f);
		}

	}
	
	if (pClientActor && m_fm && strcmp(m_fm->GetType(), "Thrown"))	
	{
		CHUD *pHUD = g_pGame->GetHUD(); 
		// inform the HUDRadar about the sound event
		if (pHUD)
		{
			Vec3 vPlayerPos=pClientActor->GetEntity()->GetWorldPos();
			float fDist2=(vPlayerPos-pos).len2();
			if (fDist2<250.0f*250.0f)
			{			
				//if (pClientActor->GetEntityId() != shooterId) 
					//	pHUD->ShowSoundOnRadar(pos);
					
				if(gEnv->bMultiplayer)
				{
					CGameRules *pGameRules = g_pGame->GetGameRules();
					if(pGameRules->GetTeamCount() < 2 || (pGameRules->GetTeam(shooterId) != pGameRules->GetTeam(pClientActor->GetEntityId())))
						pHUD->GetRadar()->AddEntityTemporarily(shooterId, 5.0f);
				}

				if ((!IsSilencerAttached()) && fDist2<sqr(pHUD->GetBattleRange())) 				
					pHUD->TickBattleStatus(1.0f); 									
			}
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::OnStartFire(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnStartFire, (this, shooterId));
}

//------------------------------------------------------------------------
void CWeapon::OnStopFire(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnStopFire, (this, shooterId));
}

//------------------------------------------------------------------------
void CWeapon::OnStartReload(EntityId shooterId, IEntityClass* pAmmoType)
{
	BROADCAST_WEAPON_EVENT(OnStartReload, (this, shooterId, pAmmoType));

	if (CActor *pActor = GetOwnerActor())
	{
		if (IAIObject *pAIObject=pActor->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal( SIGNALFILTER_SENDER, 1, "OnReload", pAIObject);
	}
}

//------------------------------------------------------------------------
void CWeapon::OnEndReload(EntityId shooterId, IEntityClass* pAmmoType)
{
	BROADCAST_WEAPON_EVENT(OnEndReload, (this, shooterId, pAmmoType));

	if (CActor *pActor = GetOwnerActor())
	{
		if (IAIObject *pAIObject=pActor->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal( SIGNALFILTER_SENDER, 1, "OnReloadDone", pAIObject);
	}
}

//------------------------------------------------------------------------
void CWeapon::OnOutOfAmmo(IEntityClass* pAmmoType)
{
	BROADCAST_WEAPON_EVENT(OnOutOfAmmo, (this, pAmmoType));

/*	- no need to send signal here - puppet will check ammo when fires
	if (CActor *pActor = GetOwnerActor())
	{
		if (IAIObject *pAIObject=Actor->GetEntity()->GetAI())
			gEnv->pAISystem->SendSignal( SIGNALFILTER_SENDER, 1, "OnOutOfAmmo", pAIObject);
	}
*/
}

//------------------------------------------------------------------------
void CWeapon::OnReadyToFire()
{
	BROADCAST_WEAPON_EVENT(OnReadyToFire, (this));
}

//------------------------------------------------------------------------
void CWeapon::OnPickedUp(EntityId actorId, bool destroyed)
{
	BROADCAST_WEAPON_EVENT(OnPickedUp, (this, actorId, destroyed));

	CItem::OnPickedUp(actorId, destroyed);

	if(GetISystem()->IsSerializingFile() == 1)
		return;

	if (!IsServer())
		return;

	CActor *pActor=GetActor(actorId);
	if (!pActor)
		return;

	// bonus ammo is always put in the actor's inv
	if (!m_bonusammo.empty())
	{
		for (TAmmoMap::iterator it=m_bonusammo.begin(); it!=m_bonusammo.end(); ++it)
		{
			int count=it->second;

			SetInventoryAmmoCount(it->first, GetInventoryAmmoCount(it->first)+count);
		}

		m_bonusammo.clear();
	}
	
	// current ammo is only added to actor's inv, if we already have this weapon
	if (destroyed && m_params.unique)
	{
		for (TAmmoMap::iterator it=m_ammo.begin(); it!=m_ammo.end(); ++it)
		{
			//Only add ammo to inventory, if not accessory ammo (accessories give ammo already)
			if(m_accessoryAmmo.find(it->first)==m_accessoryAmmo.end())
			{
				int count=it->second;

				SetInventoryAmmoCount(it->first, GetInventoryAmmoCount(it->first)+count);
			}
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::OnDropped(EntityId actorId)
{
	BROADCAST_WEAPON_EVENT(OnDropped, (this, actorId));

	CItem::OnDropped(actorId);
}


//------------------------------------------------------------------------
void CWeapon::OnMelee(EntityId shooterId)
{
	BROADCAST_WEAPON_EVENT(OnMelee, (this, shooterId));
}

//------------------------------------------------------------------------
void CWeapon::OnStartTargetting(IWeapon *pWeapon)
{
	BROADCAST_WEAPON_EVENT(OnStartTargetting,(this));
}

//------------------------------------------------------------------------
void CWeapon::OnStopTargetting(IWeapon *pWeapon)
{
	BROADCAST_WEAPON_EVENT(OnStopTargetting,(this));
}