/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 2:3:2005   16:06 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "C4.h"
#include "Plant.h"

#include "Game.h"
#include "Actor.h"
#include "WeaponSystem.h"


//------------------------------------------------------------------------
CC4::CC4()
{
}


//------------------------------------------------------------------------
CC4::~CC4()
{
};


//------------------------------------------------------------------------
void CC4::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	/*if (!strcmp(actionName, "zoom"))
	{
		IFireMode *pFireMode=GetFireMode(GetCurrentFireMode());

		if (pFireMode && !stricmp(pFireMode->GetType(), "Plant"))
		{
			if (activationMode == eAAM_OnPress)
			{
				CPlant *pPlant=static_cast<CPlant *>(pFireMode);
				pPlant->Time();
			}

			CWeapon::OnAction(actorId, "attack1", activationMode, value);

			return;
		}
	}*/

	CWeapon::OnAction(actorId, actionId, activationMode, value);
}

//------------------------------------------------------------------------
void CC4::PickUp(EntityId pickerId, bool sound, bool select/* =true */, bool keepHistory/* =true */)
{
	CActor *pActor=GetActor(pickerId);
	if (pActor)
	{
		IInventory *pInventory=GetActorInventory(pActor);
		if (pInventory)
		{
			if (!pInventory->GetItemByClass(CItem::sDetonatorClass))
			{
				if (IsServer())
					m_pItemSystem->GiveItem(pActor, "Detonator", false, false, false);
			}
		}
	}

	CWeapon::PickUp(pickerId, sound, select, keepHistory);
}

//------------------------------------------------------------------------
bool CC4::CanSelect() const
{
	CActor *pOwner=GetOwnerActor();
	if (!pOwner)
		return false;

	IFireMode *pFireMode=GetFireMode(GetCurrentFireMode());
	if (pFireMode && pFireMode->GetProjectileId() && g_pGame->GetWeaponSystem()->GetProjectile(pFireMode->GetProjectileId()))
	{
		if (pOwner->GetCurrentItem() && !strcmp(pOwner->GetCurrentItem()->GetEntity()->GetClass()->GetName(), "Detonator"))
			return false;

		return true;
	}
	else if (pFireMode)
		pFireMode->SetProjectileId(0);

	return CWeapon::CanSelect() && !OutOfAmmo(false);
};

//------------------------------------------------------------------------
void CC4::Select(bool select)
{
	if (select)
	{

		IFireMode *pFireMode=GetFireMode(GetCurrentFireMode());
		if (pFireMode && pFireMode->GetProjectileId() && g_pGame->GetWeaponSystem()->GetProjectile(pFireMode->GetProjectileId()))
		{
			CActor *pOwner=GetOwnerActor();
			if (!pOwner)
				return;

			EntityId detonatorId = pOwner->GetInventory()->GetItemByClass(CItem::sDetonatorClass);
			if (detonatorId)
				pOwner->SelectItemByName("Detonator", false);

			return;
		}
		else if (pFireMode)
		{
			if(OutOfAmmo(false))
			{
				Select(false);
				CActor *pOwner=GetOwnerActor();
				if (!pOwner)
					return;

				EntityId fistsId = pOwner->GetInventory()?pOwner->GetInventory()->GetItemByClass(CItem::sFistsClass):0;
				if (fistsId)
					pOwner->SelectItem(fistsId,true);

				return;
			}
			else
			{
				pFireMode->SetProjectileId(0);
			}
		}
	}

	CWeapon::Select(select);
}

//------------------------------------------------------------------------
void CC4::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	EntityId fistsId = 0;
	
	CActor *pOwner = GetOwnerActor();
	if(pOwner)
		fistsId = pOwner->GetInventory()?pOwner->GetInventory()->GetItemByClass(CItem::sFistsClass):0;

	CItem::Drop(impulseScale,selectNext,byDeath);

	if (fistsId)
		pOwner->SelectItem(fistsId,true);

}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CC4, ClSetProjectileId)
{
	IFireMode *pFireMode=GetFireMode(params.fmId);
	if (pFireMode)
		pFireMode->SetProjectileId(params.id);
	else
		return false;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CC4, SvRequestTime)
{
	IFireMode *pFireMode=GetFireMode(params.fmId);
	if (pFireMode && !stricmp(pFireMode->GetType(), "Plant"))
	{
		CPlant *pPlant=static_cast<CPlant *>(pFireMode);
		pPlant->SetTime(params.time);
	}
	else
		return false;

	return true;
}