/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:12:2005   14:01 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "AmmoPickup.h"
#include "Weapon.h"
#include "Actor.h"
#include "IItemSystem.h"
#include "ItemParamReader.h"
#include "GameCVars.h"
#include "OffHand.h"
#include <ISound.h>

#define EXPLOSIVE_GRENADE 0
#define SMOKE_GRENADE			1
#define FLASHBANG_GRENADE	2
#define EMP_GRENADE				3

//------------------------------------------------------------------------
void CAmmoPickup::PostInit( IGameObject * pGameObject )
{
	if (m_modelName.empty())
	{
		const char *model=0;
		if (GetEntityProperty("objModel", model) && model && model[0])
			m_modelName=model;
	}

	if (!m_modelName.empty())
		SetGeometry(eIGS_ThirdPerson, m_modelName.c_str());

	CWeapon::PostInit(pGameObject);
}

//------------------------------------------------------------------------
bool CAmmoPickup::CanUse(EntityId userId) const
{
	return false;
}

//------------------------------------------------------------------------
bool CAmmoPickup::CanPickUp(EntityId pickerId) const
{
	return true;
}

//------------------------------------------------------------------------
void CAmmoPickup::SerializeSpawnInfo( TSerialize ser )
{
	string modelName;
	ser.Value("modelName", modelName);
	m_modelName=modelName;
}

//------------------------------------------------------------------------
ISerializableInfoPtr CAmmoPickup::GetSpawnInfo()
{
	struct SInfo : public ISerializableInfo
	{
		string modelName;
		void SerializeWith( TSerialize ser )
		{
			ser.Value("modelName", modelName);
		}
	};

	SInfo *p=new SInfo();
	p->modelName=m_modelName;

	return p;
}

//------------------------------------------------------------------------
bool CAmmoPickup::ReadItemParams(const IItemParamsNode *root)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (!CWeapon::ReadItemParams(root))
		return false;
	
	const IItemParamsNode *params = root->GetChild("params");
	{
		CItemParamReader reader(params);
		reader.Read("ammo_pickup_sound", m_pickup_sound);
	}

	return true;
}

//------------------------------------------------------------------------
void CAmmoPickup::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory)
{
	SetOwnerId(pickerId);

	CActor *pActor=GetActor(pickerId);

	if (!pActor)
		return;

	IInventory *pInventory = GetActorInventory(pActor);
	if (!pInventory)
		return;

	if (IsServer())
	{
		// bonus ammo is always put in the actor's inv
		if (!m_bonusammo.empty())
		{
			for (TAmmoMap::iterator it=m_bonusammo.begin(); it!=m_bonusammo.end(); ++it)
			{
				int count=it->second;

				SetInventoryAmmoCount(it->first, GetInventoryAmmoCount(it->first)+count);

				if(pActor->IsPlayer())
					ShouldSwitchGrenade(it->first);
			}

			m_bonusammo.clear();
		}

		for (TAmmoMap::iterator it=m_ammo.begin(); it!=m_ammo.end(); ++it)
		{
			int count=it->second;

			SetInventoryAmmoCount(it->first, GetInventoryAmmoCount(it->first)+count);

			if(pActor->IsPlayer())
				ShouldSwitchGrenade(it->first);
		}

		const char *ammoName=0;
		if (GetEntityProperty("AmmoName", ammoName) && ammoName && ammoName[0])
		{
			int count=0;
			GetEntityProperty("Count", count);
			if (count)
			{
				IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammoName);
				SetInventoryAmmoCount(pClass, GetInventoryAmmoCount(pClass)+count);

				if(pActor->IsPlayer())
					ShouldSwitchGrenade(pClass);
			}
		}

		TriggerRespawn();
	}

	//Play sound
	if(!m_pickup_sound.empty())
	{
		IEntity *pPicker = m_pEntitySystem->GetEntity(pickerId);
		if(pPicker)
		{
			IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*)pPicker->GetProxy(ENTITY_PROXY_SOUND);

			if(pSoundProxy)
			{
				//Execute sound at picker position
				pSoundProxy->PlaySound(m_pickup_sound, pPicker->GetWorldPos(),FORWARD_DIRECTION, FLAG_SOUND_DEFAULT_3D);
			}
		}
	}

	RemoveEntity();
}

//--------------------------------------------
bool CAmmoPickup::CheckAmmoRestrictions(EntityId pickerId)
{
	if(g_pGameCVars->i_unlimitedammo != 0)
		return true;

	IActor* pPicker = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pickerId);
	if(pPicker)
	{
		IInventory *pInventory = pPicker->GetInventory();
		if(pInventory)
		{
			for (TAmmoMap::const_iterator it=m_ammo.begin(); it!=m_ammo.end(); ++it)
			{
				int invAmmo  = pInventory->GetAmmoCount(it->first);
				int invLimit = pInventory->GetAmmoCapacity(it->first);

				if(invAmmo>=invLimit && (!gEnv->pSystem->IsEditor()))
					return false;
			}

			const char *ammoName=0;
			if (GetEntityProperty("AmmoName", ammoName) && ammoName && ammoName[0])
			{
				int count=0;
				GetEntityProperty("Count", count);
				if (count)
				{
					IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammoName);
					if(pClass)
					{
						int invAmmo  = pInventory->GetAmmoCount(pClass);
						int invLimit = pInventory->GetAmmoCapacity(pClass);
						if(invAmmo>=invLimit && (!gEnv->pSystem->IsEditor()))
							return false;
					}
				}
			}
		}
	}

	return true;
}

//---------------------------------------------------------------
void CAmmoPickup::ShouldSwitchGrenade(IEntityClass* pClass)
{
	bool flashbang = (pClass==CItem::sFlashbangGrenade);
	bool smoke     = (pClass==CItem::sSmokeGrenade);
	bool emp       = (pClass==CItem::sEMPGrenade);
	bool explosive = (pClass==CItem::sExplosiveGrenade);

	if(!flashbang && !smoke && !emp && !explosive)
		return;

	CActor* pPlayer = GetOwnerActor();
	
	if(!pPlayer)
		return;

	COffHand* pOffHand = static_cast<COffHand*>(pPlayer->GetWeaponByClass(CItem::sOffHandClass));

	if(pOffHand)
	{
		if(IFireMode* fm = pOffHand->GetFireMode(pOffHand->GetCurrentFireMode()))
		{
			if(fm->OutOfAmmo())
			{
				if(explosive)
					pOffHand->RequestFireMode(EXPLOSIVE_GRENADE);
				else if(smoke)
					pOffHand->RequestFireMode(SMOKE_GRENADE);
				else if(flashbang)
					pOffHand->RequestFireMode(FLASHBANG_GRENADE);
				else if(emp)
					pOffHand->RequestFireMode(EMP_GRENADE);
			}
		}
	}
}