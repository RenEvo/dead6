/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 27:10:2004   11:26 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Item.h"
#include "ItemSharedParams.h"
#include "Game.h"
#include "GameActions.h"
#include "IGameObject.h"
#include "ISerialize.h"
#include <IEntitySystem.h>

#include "Actor.h"
#include "Player.h"
#include "IActorSystem.h"
#include "IItemSystem.h"
#include "ActionMapManager.h"
#include "ScriptBind_Item.h"
#include "GameRules.h"
#include "HUD/HUD.h"
#include "GameCVars.h"
#include "Binocular.h"


#pragma warning(disable: 4355)	// ´this´ used in base member initializer list

#if defined(USER_alexl)
#define ITEM_DEBUG_MEMALLOC
#endif

#ifdef ITEM_DEBUG_MEMALLOC
int gInstanceCount = 0;
#endif


IEntitySystem *CItem::m_pEntitySystem=0;
IItemSystem *CItem::m_pItemSystem=0;
IGameFramework *CItem::m_pGameFramework=0;
IGameplayRecorder*CItem::m_pGameplayRecorder=0;

IEntityClass* CItem::sOffHandClass = 0;
IEntityClass* CItem::sFistsClass = 0;
IEntityClass* CItem::sSOCOMClass = 0;
IEntityClass* CItem::sDetonatorClass = 0;
IEntityClass* CItem::sC4Class = 0;
IEntityClass* CItem::sBinocularsClass = 0;
IEntityClass* CItem::sGaussRifleClass = 0;
IEntityClass* CItem::sDebugGunClass = 0;
IEntityClass* CItem::sRefWeaponClass = 0;
IEntityClass* CItem::sClaymoreExplosiveClass = 0;
IEntityClass* CItem::sAVExplosiveClass = 0;
IEntityClass* CItem::sDSG1Class = 0;
IEntityClass* CItem::sLAMFlashLight = 0;
IEntityClass* CItem::sLAMRifleFlashLight = 0;

IEntityClass* CItem::sFlashbangGrenade = 0;
IEntityClass* CItem::sExplosiveGrenade = 0;
IEntityClass* CItem::sEMPGrenade = 0;
IEntityClass* CItem::sSmokeGrenade = 0;

//------------------------------------------------------------------------
CItem::CItem()
: m_scheduler(this),	// just to store the pointer.
	m_dualWieldMasterId(0),
	m_dualWieldSlaveId(0),
	m_ownerId(0),
	m_postSerializeMountedOwner(0),
	m_parentId(0),
	m_effectGenId(0),
	m_pForcedArms(0),
  m_hostId(0),
	m_pEntityScript(0),
	m_modifying(false),
	m_transitioning(false),
	m_frozen(false),
	m_cloaked(false),
	m_enableAnimations(true),
	m_noDrop(false),
	m_constraintId(0)
{
#ifdef ITEM_DEBUG_MEMALLOC
	++gInstanceCount;
#endif
	memset(m_animationTime, 0, sizeof(m_animationTime));
	memset(m_animationEnd, 0, sizeof(m_animationTime));
	memset(m_animationSpeed, 0, sizeof(m_animationSpeed));
}

//------------------------------------------------------------------------
CItem::~CItem()
{
	AttachArms(false, false);

	//Auto-detach from the parent
	if(m_parentId)
	{
		CItem *pParent= static_cast<CItem*>(m_pItemSystem->GetItem(m_parentId));
		if(pParent)
		{
			//When destroyed the item parent should not be busy
			pParent->SetBusy(false);

			for (TAccessoryMap::iterator it=pParent->m_accessories.begin(); it!=pParent->m_accessories.end(); ++it)
			{
				if(GetEntityId()==it->second)
				{
					pParent->AttachAccessory(it->first.c_str(),false,true);
					break;
				}
			}
		}
	}

	GetGameObject()->ReleasePhysics(this);

	if (GetOwnerActor() && GetOwnerActor()->GetInventory())
		GetOwnerActor()->GetInventory()->RemoveItem(GetEntityId());

	if(!(GetISystem()->IsSerializingFile() && GetGameObject()->IsJustExchanging()))
		for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
			gEnv->pEntitySystem->RemoveEntity(it->second);

	if(m_pItemSystem)
		m_pItemSystem->RemoveItem(GetEntityId());

	 Quiet();

#ifdef ITEM_DEBUG_MEMALLOC
	 --gInstanceCount;
#endif
}

//------------------------------------------------------------------------
bool CItem::Init( IGameObject *pGameObject )
{
	SetGameObject(pGameObject);

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("CItem::Init Instance=%d %p Id=%d Class=%s", gInstanceCount, GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	m_pEntityScript = GetEntity()->GetScriptTable();

	if (!m_pGameFramework)
	{
		m_pEntitySystem = gEnv->pEntitySystem;
		m_pGameFramework= gEnv->pGame->GetIGameFramework();
		m_pGameplayRecorder = m_pGameFramework->GetIGameplayRecorder();
		m_pItemSystem = m_pGameFramework->GetIItemSystem();
		sOffHandClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("OffHand");
		sFistsClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Fists");
		sSOCOMClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("SOCOM");
		sDetonatorClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Detonator");
		sC4Class = gEnv->pEntitySystem->GetClassRegistry()->FindClass("C4");
		sBinocularsClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Binoculars");
		sGaussRifleClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("GaussRifle");
		sDebugGunClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DebugGun");
		sRefWeaponClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("RefWeapon");
		sClaymoreExplosiveClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("claymoreexplosive");
		sAVExplosiveClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("avexplosive");
		sDSG1Class = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DSG1");
		sLAMFlashLight			= gEnv->pEntitySystem->GetClassRegistry()->FindClass("LAMFlashLight");
		sLAMRifleFlashLight	= gEnv->pEntitySystem->GetClassRegistry()->FindClass("LAMRifleFlashLight");

		sFlashbangGrenade = gEnv->pEntitySystem->GetClassRegistry()->FindClass("flashbang");
		sEMPGrenade       = gEnv->pEntitySystem->GetClassRegistry()->FindClass("empgrenade");
		sSmokeGrenade     = gEnv->pEntitySystem->GetClassRegistry()->FindClass("smokegrenade");
		sExplosiveGrenade = gEnv->pEntitySystem->GetClassRegistry()->FindClass("explosivegrenade");
	}

	if (!GetGameObject()->CapturePhysics(this))
		return false;

	// bind to network
	if (0 == (GetEntity()->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY)))
	{
		if (!GetGameObject()->BindToNetwork())
		{
			GetGameObject()->ReleasePhysics(this);
			return false;
		}
	}

	// register with item system
	m_pItemSystem->AddItem(GetEntityId(), this);
	m_pItemSystem->CacheItemGeometry(GetEntity()->GetClass()->GetName());

	// attach script bind
	g_pGame->GetItemScriptBind()->AttachTo(this);

	m_sharedparams=g_pGame->GetItemSharedParamsList()->GetSharedParams(GetEntity()->GetClass()->GetName(), true);

	m_noDrop = false;

	if (GetEntity()->GetScriptTable())
	{
		SmartScriptTable props;
		GetEntity()->GetScriptTable()->GetValue("Properties", props);
		ReadProperties(props);
	}

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("CItem::Init End %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	return true;
}

//------------------------------------------------------------------------
void CItem::Reset()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("CItem::Reset Start %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	if (IsModifying())
		ResetAccessoriesScreen();

	ResetOwner();  
  m_scheduler.Reset();
  
	m_params=SParams();
//	m_mountparams=SMountParams();
	m_enableAnimations = true;
	// detach any effects
	TEffectInfoMap temp = m_effects;
	TEffectInfoMap::iterator end = temp.end();
	for (TEffectInfoMap::iterator it=temp.begin(); it!=end;++it)
		AttachEffect(it->second.slot, it->first, false);
	m_effectGenId=0;

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("    CItem::Read ItemParams Start %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	// read params
	m_sharedparams=0; // decrease refcount to force a deletion of old parameters in case we are reloading item scripts
	m_sharedparams=g_pGame->GetItemSharedParamsList()->GetSharedParams(GetEntity()->GetClass()->GetName(), true);
	const IItemParamsNode *root = m_pItemSystem->GetItemParams(GetEntity()->GetClass()->GetName());
	ReadItemParams(root);

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("    CItem::Read ItemParams End %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
#endif

	m_stateTable[0] = GetEntity()->GetScriptTable();
	if (!!m_stateTable[0])
	{
		m_stateTable[0]->GetValue("Server", m_stateTable[1]);
		m_stateTable[0]->GetValue("Client", m_stateTable[2]);
	}

	Quiet();

	//compute item volume
	AABB bounds;
	GetEntity()->GetLocalBounds(bounds);
	Vec3 delta(bounds.max - bounds.min);
	m_fVolume = fabs(delta.x * delta.y * delta.z);

	ReAttachAccessories();
	AccessoriesChanged();

	for (TAccessoryMap::iterator it=m_accessories.begin(); it!=m_accessories.end(); ++it)
		FixAccessories(GetAccessoryParams(it->first), true);

	InitRespawn();
	if (IsCloaked())
		Cloak(false);

	m_noDrop = false;

	if(m_params.has_first_select)
		m_stats.first_selection = true; //Reset (just in case)

	OnReset();

#ifdef ITEM_DEBUG_MEMALLOC
	CGame::DumpMemInfo("  CItem::Reset End %p Id=%d Class=%s", GetGameObject(), GetEntityId(), gEnv->pEntitySystem->GetEntity(GetEntityId())->GetClass()->GetName());
	CryLogAlways(" ");
#endif

}

//------------------------------------------------------------------------
void CItem::ResetOwner()
{
  if (m_ownerId)
  {
    if (m_stats.used)
      StopUse(m_ownerId);

    CActor *pOwner=GetOwnerActor();

    if (!pOwner || pOwner->GetInventory()->FindItem(GetEntityId())<0)
      SetOwnerId(0);
  }
}

//------------------------------------------------------------------------
void CItem::PostInit( IGameObject * pGameObject )
{
	// prevent ai from automatically disabling weapons
	for (int i=0; i<4;i++)
		pGameObject->SetUpdateSlotEnableCondition(this, i, eUEC_WithoutAI);

	Reset();
	//InitialSetup();
	PatchInitialSetup();	
	InitialSetup();		//Must be called after Patch
}

//------------------------------------------------------------------------
void CItem::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CItem::Update( SEntityUpdateContext& ctx, int slot )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (m_frozen || IsDestroyed())
		return;

	switch (slot)
	{
	case eIUS_Scheduler:
		m_scheduler.Update(ctx.fFrameTime);
		break;
	}

	// drop the item if too heavy
	/*if (IsSelected())		//current design: just slow down the player movement
	{
		CActor *pActor = GetOwnerActor();

		if(pActor)
		{
			bool zeroG = false;
			if(pActor->IsPlayer())
			{
				CPlayer *pPlayer = (CPlayer*)pActor;
				zeroG = pPlayer->IsZeroG();
			}

			if (!zeroG && !pActor->CanPickUpObject(m_params.mass, m_fVolume))
			{
				const char *str="You need more strength to hold this item!";
				SGameObjectEvent evt("HUD_TextMessage",eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void*)str);
				SendHUDEvent(evt);

				pActor->DropItem(GetEntityId());
			}
		}
	}*/

	// update mounted
	if (slot==eIUS_General)
	{
		if (m_stats.mounted)
		{
			CActor *pActor=GetOwnerActor();
			if (pActor && !pActor->IsClient())
  			UpdateMounted(ctx.fFrameTime);
		}
	}
}

//------------------------------------------------------------------------
bool CItem::SetProfile( uint8 profile )
{
	//CryLogAlways("%s::SetProfile(%d: %s)", GetEntity()->GetName(), profile, profile==eIPhys_Physicalized?"Physicalized":"NotPhysicalized");

	switch (profile)
	{
	case eIPhys_PhysicalizedStatic:
		{
			SEntityPhysicalizeParams params;
			params.type = PE_STATIC;
			params.nSlot = eIGS_ThirdPerson;

			GetEntity()->Physicalize(params);

			return true;
		}
		break;
	case eIPhys_PhysicalizedRigid:
		{
			SEntityPhysicalizeParams params;
			params.type = PE_RIGID;
			params.nSlot = eIGS_ThirdPerson;
			params.mass = m_params.mass;

			pe_params_buoyancy buoyancy;
			buoyancy.waterDamping = 1.5;
			buoyancy.waterResistance = 1000;
			buoyancy.waterDensity = 0;
			params.pBuoyancy = &buoyancy;

			GetEntity()->Physicalize(params);

			IPhysicalEntity *pPhysics = GetEntity()->GetPhysics();
			if (pPhysics)
			{
				pe_action_awake action;
				action.bAwake = m_ownerId!=0;
				pPhysics->Action(&action);
			}
		}
		return true;
	case eIPhys_NotPhysicalized:
		{
			IEntityPhysicalProxy *pPhysicsProxy = GetPhysicalProxy();
			if (pPhysicsProxy)
			{
				SEntityPhysicalizeParams params;
				params.type = PE_NONE;
				params.nSlot = eIGS_ThirdPerson;
				pPhysicsProxy->Physicalize(params);
			}
		}
		return true;
	default:
		//assert(!"Unknown physicalization profile!");
		return false;
	}
}

uint8 CItem::GetDefaultProfile()
{
	if (m_properties.pickable)
	{
		if (m_properties.physics)
			return eIPhys_PhysicalizedRigid;
		return eIPhys_PhysicalizedStatic;
	}
	return eIPhys_NotPhysicalized;
}

bool CItem::SerializeProfile( TSerialize ser, uint8 profile, int pflags )
{
	//if(profile == eIPhys_DemoRecording)
	//{
	//	// ??? - worse hack ever? - whomever wrote this talk to me... love Craig
	//	uint8 currentProfile = 255;
	//	if(ser.IsWriting())
	//		currentProfile = (pe_type)(GetGameObject()->GetPhysicalizationProfile());
	//	ser.Value("PhysicalizationProfile", currentProfile);
	//	return SerializeProfile(ser, currentProfile, pflags);
	//}

	pe_type type = PE_NONE;
	switch (profile)
	{
	case eIPhys_PhysicalizedStatic:
		type = PE_STATIC;
		break;
	case eIPhys_PhysicalizedRigid:
		type = PE_RIGID;
		break;
	case eIPhys_NotPhysicalized:
		type = PE_NONE;
		break;
	default:
		return false;
	}

	if (type == PE_NONE)
		return true;

	IEntityPhysicalProxy * pEPP = (IEntityPhysicalProxy *) GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS);
	if (ser.IsWriting())
	{
		if (!pEPP || !pEPP->GetPhysicalEntity() || pEPP->GetPhysicalEntity()->GetType() != type)
		{
			gEnv->pPhysicalWorld->SerializeGarbageTypedSnapshot( ser, type, 0 );
			return true;
		}
	}
	else if (!pEPP)
	{
		return false;
	}

	pEPP->SerializeTyped( ser, type, pflags );
	return true;
}

//------------------------------------------------------------------------
void CItem::HandleEvent( const SGameObjectEvent &evt )
{
	if (evt.event == eCGE_PostFreeze)
		Freeze(evt.param!=0);
}

//------------------------------------------------------------------------
void CItem::ProcessEvent(SEntityEvent &event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	switch (event.event)
	{
	case ENTITY_EVENT_TIMER:
		{
			switch (event.nParam[0])
			{
			case eIT_Flying:
				m_stats.flying = false;
				if (IsServer())
					IgnoreCollision(false);
				break;
			}
      break;
		}
  case ENTITY_EVENT_RESET:
		Reset();

		if (gEnv->pSystem->IsEditor() && !m_stats.mounted)
		{
			IInventory *pInventory=GetActorInventory(GetOwnerActor());

			if (event.nParam[0]) // entering game mode in editor
				m_editorstats=SEditorStats(GetOwnerId(), pInventory?pInventory->GetCurrentItem()==GetEntityId():0);
			else // leaving game mode
			{

				if (m_editorstats.ownerId)
				{
					m_noDrop=true;
					if(IsDualWieldMaster())
						ResetDualWield();

					int iValue = gEnv->pConsole->GetCVar("i_noweaponlimit")->GetIVal();
					gEnv->pConsole->GetCVar("i_noweaponlimit")->Set(1);

					PickUp(m_editorstats.ownerId, false, false, false);

					gEnv->pConsole->GetCVar("i_noweaponlimit")->Set(iValue);

					IItemSystem *pItemSystem=g_pGame->GetIGameFramework()->GetIItemSystem();

					if (m_editorstats.current && pInventory && pInventory->GetCurrentItem()==GetEntityId())
					{
						//if (pInventory)
						pInventory->SetCurrentItem(0);
						pItemSystem->SetActorItem(GetActor(m_editorstats.ownerId), GetEntityId(), false);
					}
					else if (pInventory && pInventory->GetCurrentItem()==GetEntityId())
						pItemSystem->SetActorItem(GetActor(m_editorstats.ownerId), (EntityId)0, false);
				}
				else
				{
					if(GetIWeapon() && !GetParentId())
						Drop(0,false,false);

					SetOwnerId(0);
				}
			}
		}
    break;
	}
}

//------------------------------------------------------------------------
void CItem::Serialize( TSerialize ser, unsigned aspects )
{
	if (ser.GetSerializationTarget() != eST_Network)
	{
		m_stats.Serialize(ser);

		EntityId ownerId = m_ownerId;
		EntityId parentId = m_parentId;
		EntityId hostId = m_hostId;

		ser.Value("ownerId", ownerId);
		ser.Value("parentId", parentId);
		ser.Value("hostId", hostId);

		if (m_stats.mounted)
		{
			//Vec3 mountpos = GetEntity()->GetWorldPos();
			//ser.Value("mountpos", mountpos);

			if(ser.IsReading())
			{
				if (!m_hostId)
					MountAt(GetEntity()->GetWorldPos());
				//else
				//	MountAtEntity(hostId, mountpos, Ang3(0,0,0));
			}
		}

		if (ser.IsReading() && m_stats.mounted && m_params.usable)
		{
			if(m_ownerId)
			{
				IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_ownerId);
				CPlayer *pPlayer = static_cast<CPlayer*> (pActor);
				if(pPlayer)
				{
					if(m_stats.used)
						pPlayer->UseItem(GetEntityId());
					else
						StopUse(m_ownerId);
				}
				m_stats.used = false;
			}

			m_postSerializeMountedOwner = ownerId;
		}
		else
			m_ownerId = ownerId;

		//serialize attachments
		int attachmentAmount = m_accessories.size();
		ser.Value("attachmentAmount", attachmentAmount);
		if(ser.IsWriting())
		{
			TAccessoryMap::iterator it = m_accessories.begin();
			for(; it != m_accessories.end(); ++it)
			{
				string name((it->first).c_str());
				EntityId id = it->second;
				ser.BeginGroup("Accessory");
				ser.Value("Name", name);
				ser.Value("Id", id);
				ser.EndGroup();
			}
		}
		else if(ser.IsReading())
		{
			m_accessories.clear();
			string name;
			for(int i = 0; i < attachmentAmount; ++i)
			{
				EntityId id = 0;
				ser.BeginGroup("Accessory");
				ser.Value("Name", name);
				ser.Value("Id", id);
#ifndef ITEM_USE_SHAREDSTRING
				m_accessories[name] = id;
#else
				m_accessories[ItemString(name)] = id;
#endif
				ser.EndGroup();
			}
		}

		if(ser.IsReading())
		{
			SetViewMode(m_stats.viewmode);

			/*if(m_parentId)
			{
				CItem * pItem = (CItem*)g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_parentId);
				if(pItem)
					pItem->ReAttachAccessory(GetEntityId());
			}*/
		}

		//Extra ammo given by some accessories
		{
			ser.BeginGroup("AccessoryAmmo");
			if(ser.IsReading())
				m_bonusAccessoryAmmo.clear();
			TAccessoryAmmoMap::iterator it = m_bonusAccessoryAmmo.begin();
			int ammoTypeAmount = m_bonusAccessoryAmmo.size();
			ser.Value("AccessoryAmmoAmount", ammoTypeAmount);
			for(int i = 0; i < ammoTypeAmount; ++i, ++it)
			{
				string name;
				int amount = 0;
				if(ser.IsWriting())
				{
					name = it->first->GetName();
					amount = it->second;
				}
				ser.BeginGroup("Ammo");
				ser.Value("AmmoName", name);
				ser.Value("Bullets", amount);
				ser.EndGroup();
				if(ser.IsReading())
				{
					IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
					assert(pClass);
					if(pClass)
						m_bonusAccessoryAmmo[pClass] = amount;
				}
			}
			ser.EndGroup();
		}
	}
	else
	{
		// network ser here
	}
}

//------------------------------------------------------------------------
void CItem::PostSerialize()
{
	if(m_postSerializeMountedOwner)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_postSerializeMountedOwner);
		CPlayer *pPlayer = static_cast<CPlayer*> (pActor);
		if(pPlayer && m_params.usable)
		{
			m_stats.used = false;
			pPlayer->UseItem(GetEntityId());
      assert(m_ownerId);
		}
		m_postSerializeMountedOwner = 0;		
	}

	ReAttachAccessories();

	if(GetOwnerActor() && this == GetOwnerActor()->GetCurrentItem() && !GetOwnerActor()->GetLinkedVehicle())
	{
		GetOwnerActor()->HolsterItem(true);	//this "fixes" old attachments that are not replaced still showing up in the model ..
		GetOwnerActor()->HolsterItem(false);
	}

	//Fix incorrect view mode (in same cases) and not physicalized item after dropping/picking (in same cases too)
	if(!GetOwnerActor() && m_stats.dropped)
	{
		SetViewMode(eIVM_ThirdPerson);
		AttachToHand(false);

		Hide(false);
		Pickalize(true,true);
		GetEntity()->EnablePhysics(true);

		Physicalize(true, m_properties.physics);
	}
}

//------------------------------------------------------------------------
void CItem::SetOwnerId(EntityId ownerId)
{
	m_ownerId = ownerId;

	GetGameObject()->ChangedNetworkState(ASPECT_OWNER_ID);
}

//------------------------------------------------------------------------
EntityId CItem::GetOwnerId() const
{
	return m_ownerId;
}

//------------------------------------------------------------------------
void CItem::SetParentId(EntityId parentId)
{
	m_parentId = parentId;

	GetGameObject()->ChangedNetworkState(ASPECT_OWNER_ID);
}

//------------------------------------------------------------------------
EntityId CItem::GetParentId() const
{
	return m_parentId;
}

//------------------------------------------------------------------------
void CItem::SetHand(int hand)
{
	m_stats.hand = hand;

	int idx = 0;
	if (hand == eIH_Right)
		idx = 1;
	else if (hand == eIH_Left)
		idx = 2;

	if (m_fpgeometry[idx].name.empty())
		idx = 0;

	bool result=false;
	bool ok=true;

	if (m_parentId)
	{
		if (CItem *pParent=static_cast<CItem*>(m_pItemSystem->GetItem(m_parentId)))
			ok = pParent->IsSelected();
	}

	if (m_stats.mounted)
		ok=true;

	if (m_stats.viewmode&eIVM_FirstPerson && ok)
	{
		SGeometry &geometry = m_fpgeometry[idx];
		result=SetGeometry(eIGS_FirstPerson, geometry.name, geometry.position, geometry.angles, geometry.scale);
	}

	if (idx == 0)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);
		if (!pCharacter)
			return;
		
		if (hand == eIH_Left)
			pCharacter->SetScale(Vec3(-1,1,1));
		else
			pCharacter->SetScale(Vec3(1,1,1));
	}

	if (result)
		PlayAction(m_idleAnimation[eIGS_FirstPerson], 0, true, (eIPAF_Default|eIPAF_NoBlend)&~eIPAF_Owner);
}

//------------------------------------------------------------------------
void CItem::Use(EntityId userId)
{
	if (m_params.usable && m_stats.mounted)
	{
		if (!m_ownerId)
			StartUse(userId);
		else if (m_ownerId == userId)
			StopUse(userId);
	}
}

//------------------------------------------------------------------------
struct SelectAction
{
	void execute(CItem *_item)
	{
		_item->SetBusy(false);
	}
};

void CItem::Select(bool select)
{
	m_stats.selected=select;

	CheckViewChange();

	for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
	{
		EntityId cur = (EntityId)it->second;
		IItem *attachment = m_pItemSystem->GetItem(cur);
		if (attachment)
		{
			attachment->OnParentSelect(select);
		}
	}

  IAISystem* pAISystem = gEnv->pAISystem;

	if (select)
	{
		if (IsDualWield())
			SetActionSuffix(m_params.dual_wield_suffix.c_str());

		Hide(false);

		if (!m_stats.mounted && GetOwner())
		  GetEntity()->SetWorldTM(GetOwner()->GetWorldTM());	// move somewhere near the owner so the sound can play
		float speedOverride = -1.0f;
		CActor *owner = GetOwnerActor();
		if (owner && owner->GetActorClass() == CPlayer::GetActorClassType())
		{
			CPlayer *plr = (CPlayer *)owner;
			if(plr->GetNanoSuit())
			{
				ENanoMode curMode = plr->GetNanoSuit()->GetMode();
				if (curMode == NANOMODE_SPEED)
					speedOverride = 2.5f;
			}
		}

		const char* select_animation;
		
		if(m_params.has_first_select && m_stats.first_selection)
		{
			//Only the LAW has 2 different select animations
			select_animation = g_pItemStrings->first_select;
			m_stats.first_selection = false;
		}
		else
		{
			select_animation = g_pItemStrings->select;
		}

		if (speedOverride > 0.0f)
			PlayAction(select_animation, 0, false, eIPAF_Default|eIPAF_NoBlend, speedOverride);
		else
			PlayAction(select_animation, 0, false, eIPAF_Default|eIPAF_NoBlend);

		//ForceSkinning(true);
		uint selectBusyTimer = 0;
		if (m_params.select_override == 0.0f)
			selectBusyTimer = MAX(250, GetCurrentAnimationTime(eIGS_FirstPerson)) - 250;
		else
			selectBusyTimer = (uint)m_params.select_override*1000;
		SetBusy(true);
		GetScheduler()->TimerAction(selectBusyTimer, CSchedulerAction<SelectAction>::Create(), false);

		if (m_stats.fp)
			AttachArms(true, true);
		else
			AttachToHand(true);

    if (owner)
    {
			// update smart objects states
      if (pAISystem)
        pAISystem->ModifySmartObjectStates( owner->GetEntity(), GetEntity()->GetClass()->GetName() );

      //[kirill] make sure AI gets passed the new weapon properties
      if(GetIWeapon() && owner->GetEntity() && owner->GetEntity()->GetAI())
        owner->GetEntity()->GetAI()->SetWeaponDescriptor(GetIWeapon()->GetAIWeaponDescriptor());
    }    
	}
	else
	{
		GetScheduler()->Reset(true);

		if (!m_stats.mounted)
		{
			SetViewMode(0);
			Hide(true);
		}

		// set no-weapon pose on actor
		CActor *pOwner = GetOwnerActor();
		if (pOwner)
			pOwner->PlayAction(g_pItemStrings->idle, ITEM_DESELECT_POSE);

		EnableUpdate(false);

		ReleaseStaticSounds();
		ResetAccessoriesScreen();

		AttachToHand(false);
		AttachArms(false, false);

		if (m_stats.mounted)
			m_stats.fp=false; // so that OnEnterFirstPerson is called next select

		// update smart objects states
		if ( pAISystem && pOwner )
		{
			CryFixedStringT<256> tmpString ('-');
			tmpString+=GetEntity()->GetClass()->GetName();
			pAISystem->ModifySmartObjectStates( pOwner->GetEntity(), tmpString.c_str() );
		}
	}

	if (IItem *pSlave=GetDualWieldSlave())
		pSlave->Select(select);

	// ensure attachments get cloaked
	if (select && IsCloaked())
	{
		IMaterial *oldMat = Cloak(false);
		Cloak(true, oldMat);
	}

	if(CActor *pOwner = GetOwnerActor())
	{
		if(g_pGame->GetHUD() && pOwner == g_pGame->GetIGameFramework()->GetClientActor())
			g_pGame->GetHUD()->UpdateHUDElements();	//crosshair might change
	}
}

//------------------------------------------------------------------------
void CItem::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	bool isDWSlave=IsDualWieldSlave();
	bool isDWMaster=IsDualWieldMaster();
		
	CActor *pOwner = GetOwnerActor();
	
	//CryLogAlways("%s::Drop()", GetEntity()->GetName());
	
	if (isDWMaster)
	{
		IItem *pSlave=GetDualWieldSlave();
		if (pSlave)
		{
			pSlave->Drop(impulseScale, false, byDeath);
			ResetDualWield();
			Select(true);  

			if (IsServer() && pOwner)
			{
				GetGameObject()->SetNetworkParent(0);
				if ((GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)) == 0)
					pOwner->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
				//pOwner->GetGameObject()->InvokeRMI(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale), eRMI_ToRemoteClients);
			}
			m_pItemSystem->DropActorAccessory(pOwner, GetEntity()->GetId());
			return;
		}
	}

	ResetDualWield();

	if (pOwner)
	{
		IInventory *pInventory = GetActorInventory(pOwner);
		if (pInventory && pInventory->GetCurrentItem() == GetEntity()->GetId())
			pInventory->SetCurrentItem(0);

		if (IsServer() && !isDWSlave) // don't send slave drops over network, the master is sent instead
		{
			GetGameObject()->SetNetworkParent(0);
			if ((GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)) == 0)
				pOwner->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale, selectNext), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
				//pOwner->GetGameObject()->InvokeRMI(CActor::ClDrop(), CActor::DropItemParams(GetEntityId(), impulseScale), eRMI_ToRemoteClients);
		}
	}

	Select(false);
	SetViewMode(eIVM_ThirdPerson);
	AttachToHand(false);

	Hide(false);
	GetEntity()->EnablePhysics(true);
	Physicalize(true, true);

	if (IsServer())
		IgnoreCollision(true);

	EntityId ownerId = GetOwnerId();

	if (pOwner)
	{
		if (IMovementController * pMC = pOwner->GetMovementController())
		{
			SMovementState moveState;
			pMC->GetMovementState(moveState);

			float offset = 1.05f;
			if(pOwner->IsPlayer())
				offset = 0.25f;		//Weapons could go through the terrain/walls with higher offsets

			Vec3 dir = moveState.eyeDirection;
			Vec3 pos = moveState.eyePosition+moveState.eyeDirection*offset;
			pos-= moveState.upDirection*0.175f;

			Matrix34 tm = Matrix34(Matrix33::CreateRotationVDir(dir)*Matrix33::CreateRotationXYZ(DEG2RAD(m_params.drop_angles)));
			tm.SetTranslation(pos);
			GetEntity()->SetWorldTM(tm);

			IEntityPhysicalProxy *pPhysics = GetPhysicalProxy();
			if (pPhysics)
				pPhysics->AddImpulse(-1, m_params.drop_impulse_pos, dir*m_params.drop_impulse*impulseScale, true, 1.0f);
		}

		// remove from inventory
		GetActorInventory(GetOwnerActor())->RemoveItem(GetEntity()->GetId());
		SetOwnerId(0);
	}

	Pickalize(true, true);
	EnableUpdate(false);

	if (IsServer())
		g_pGame->GetGameRules()->OnItemDropped(GetEntityId(), pOwner?pOwner->GetEntityId():0);

	if (pOwner && pOwner->IsClient())
	{
		ResetAccessoriesScreen();

		if (CanSelect() && selectNext && pOwner->GetHealth()>0 && !isDWSlave)
		{
			CBinocular *pBinoculars = static_cast<CBinocular*>(pOwner->GetItemByClass(CItem::sBinocularsClass));

			if (pOwner->GetInventory() && pOwner->GetInventory()->GetLastItem()
				  && (!pBinoculars || pBinoculars->GetEntityId()!=pOwner->GetInventory()->GetLastItem()))
				pOwner->SelectLastItem(false);
			else
				pOwner->SelectNextItem(1, false);
		}
		if (CanSelect())
			m_pItemSystem->DropActorItem(pOwner, GetEntity()->GetId());
		else
			m_pItemSystem->DropActorAccessory(pOwner, GetEntity()->GetId());
	}

	Quiet();

	OnDropped(ownerId);
}

//------------------------------------------------------------------------
void CItem::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory)
{
	CActor *pActor=GetActor(pickerId);
	if (!pActor)
		return;

	if (!pActor->CheckInventoryRestrictions(GetEntity()->GetClass()->GetName()))
	{
		if (IsServer())
			g_pGame->GetGameRules()->SendTextMessage(eTextMessageCenter, "You can't carry anymore heavy/medium items! Drop one of those first...", eRMI_ToClientChannel, pActor->GetChannelId());

		return;
	}
	else if(!CheckAmmoRestrictions(pickerId))
	{
		if (IsServer())
			g_pGame->GetGameRules()->SendTextMessage(eTextMessageCenter, "Can not pick up more ammo", eRMI_ToClientChannel, pActor->GetChannelId());

		return;
	}

	TriggerRespawn();

	GetEntity()->EnablePhysics(false);
	Physicalize(false, false);

	bool soundEnabled = IsSoundEnabled();
	EnableSound(sound);

	SetViewMode(0);		
	SetOwnerId(pickerId);

	CopyRenderFlags(GetOwner());

	Hide(true);
	m_stats.dropped = false;
	m_stats.brandnew = false;

	CActor *pOwner = GetOwnerActor();
	if (pOwner)
	{
		// move the entity to picker position
		Matrix34 tm(pOwner->GetEntity()->GetWorldTM());
		tm.AddTranslation(Vec3(0,0,2));
		GetEntity()->SetWorldTM(tm);

		if (IsServer())
		{
			GetGameObject()->SetNetworkParent(pickerId);
			if ((GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)) == 0)
			{
				//CryLogAlways("%s::PickUp(%08x:%s) Invoking RMI! **", GetEntity()->GetName(), pickerId, GetActor(pickerId)?GetActor(pickerId)->GetEntity()->GetName():"null");
				pOwner->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClPickUp(), CActor::PickItemParams(GetEntityId(), select, sound), eRMI_ToAllClients|eRMI_NoLocalCalls, GetEntityId());
				//pOwner->GetGameObject()->InvokeRMI(CActor::ClPickUp(), CActor::PickItemParams(GetEntityId(), select, sound), eRMI_ToRemoteClients);
			}
		}
	}

	bool alone = true;
	bool slave = false;
	IInventory *pInventory = GetActorInventory(pOwner);
	if (!pInventory)
	{
		GameWarning("Actor '%s' has no inventory, when trying to pickup '%s'!",
			GetActor(pickerId)?GetActor(pickerId)->GetEntity()->GetName():"null",
			GetEntity()->GetName());
		return;
	}

	if (IsServer() && pOwner->IsPlayer())
	{
		// go through through accessories map and give a copy of each to the player
		for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
		{
			const char *name=it->first.c_str();
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			if(!pInventory->GetItemByClass(pClass))
				g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, name, false, false, true);
		}

		for(TInitialSetup::iterator it = m_initialSetup.begin(); it != m_initialSetup.end(); it++)
		{
			const char *name=it->c_str();
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			if(!pInventory->GetItemByClass(pClass))
				g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, name, false, false ,true);
		}
	}

	CItem *pBuddy=0;
	int n = pInventory->GetCount();
	for (int i=0; i<n; i++)
	{
		EntityId itemId = pInventory->GetItem(i);
		IItem *pItem = m_pItemSystem->GetItem(itemId);
		if (!pItem)
			continue;

		if (pItem != this)
		{
			if (!pItem->IsDualWield() && pItem->SupportsDualWield(GetEntity()->GetClass()->GetName()))
			{
				EnableUpdate(true, eIUS_General);

				SetDualWieldMaster(pItem->GetEntity()->GetId());
				pItem->SetDualWieldSlave(GetEntity()->GetId());

				slave = true;
				if (pItem->GetEntity()->GetId() == pInventory->GetCurrentItem())
					pItem->Select(true);

				//Set the same fire mode for both weapons
				IWeapon *pMasterWeapon = pItem->GetIWeapon();
				IWeapon *pSlaveWeapon = GetIWeapon();
				if(pMasterWeapon && pSlaveWeapon)
				{
					if(pMasterWeapon->GetCurrentFireMode()!=pSlaveWeapon->GetCurrentFireMode())
					{
						pSlaveWeapon->ChangeFireMode();
					}
				}

				break;
			}
			else if (pItem->GetEntity()->GetClass() == GetEntity()->GetClass())
			{
				pBuddy=static_cast<CItem *>(pItem);
				alone=false;
			}
		}
	}

	if (slave || alone || !m_params.unique)
	{
		// add to inventory
		pInventory->AddItem(GetEntity()->GetId());

		if (select && !pOwner->GetLinkedVehicle())
		{
			if(CanSelect() && !slave)
				m_pItemSystem->SetActorItem(GetOwnerActor(), GetEntity()->GetId(), keepHistory);
			else
				m_pItemSystem->SetActorAccessory(GetOwnerActor(), GetEntity()->GetId(), keepHistory);

			pOwner->GetGameObject()->ChangedNetworkState(CPlayer::ASPECT_CURRENT_ITEM);
		}

		EnableSound(soundEnabled);

		if (IsServer() && g_pGame->GetGameRules())
			g_pGame->GetGameRules()->OnItemPickedUp(GetEntityId(), pickerId);

		PlayAction(g_pItemStrings->pickedup);
	}
	else if (!slave && m_params.unique && !alone)
	{
		if (IsServer() && !(GetISystem()->IsDemoMode() == 2))
			RemoveEntity();

		if (pBuddy)
			pBuddy->PlayAction(g_pItemStrings->pickedup);
	}

	if (pOwner)
	{
		IMaterial *ownerMat = pOwner->GetReplacementMaterial();
		if (ownerMat)
		{
			Cloak(false);
			Cloak(true, ownerMat);
		}
	}

	OnPickedUp(pickerId, m_params.unique && !alone);
}

//------------------------------------------------------------------------
void CItem::Physicalize(bool enable, bool rigid)
{
	int profile=eIPhys_NotPhysicalized;
	if (enable)
		profile=rigid?eIPhys_PhysicalizedRigid:eIPhys_PhysicalizedStatic;

	if (IsServer())
		GetGameObject()->SetPhysicalizationProfile(profile);
}

//------------------------------------------------------------------------
void CItem::Pickalize(bool enable, bool dropped)
{
	if (enable)
	{
		m_stats.flying = dropped;
		m_stats.dropped = true;
		m_stats.pickable = true;

		GetEntity()->KillTimer(eIT_Flying);
		GetEntity()->SetTimer(eIT_Flying, m_params.fly_timer);
	}
	else
	{
		m_stats.flying = false;
		m_stats.pickable = false;
	}
}

//------------------------------------------------------------------------
void CItem::IgnoreCollision(bool ignore)
{
	IPhysicalEntity *pPE=GetEntity()->GetPhysics();
	if (!pPE)
		return;

	if (ignore)
	{
		CActor *pActor=GetOwnerActor();
		if (!pActor)
			return;

		IPhysicalEntity *pActorPE=pActor->GetEntity()->GetPhysics();
		if (!pActorPE)
			return;

		pe_action_add_constraint ic;
		ic.flags=constraint_inactive|constraint_ignore_buddy;
		ic.pBuddy=pActorPE;
		ic.pt[0].Set(0,0,0);
		m_constraintId=pPE->Action(&ic);
	}
	else
	{
		pe_action_update_constraint up;
		up.bRemove=true;
		up.idConstraint = m_constraintId;
		m_constraintId=0;
		pPE->Action(&up);
	}
}

//------------------------------------------------------------------------
void CItem::AttachArms(bool attach, bool shadow)
{
	if (!m_params.arms)
		return;

	CActor *pOwnerActor = static_cast<CActor *>(GetOwnerActor());
	if (!pOwnerActor)
		return;

	if (attach)
		SetGeometry(eIGS_Arms, 0);
	else
		ResetCharacterAttachment(eIGS_FirstPerson, ITEM_ARMS_ATTACHMENT_NAME);

	if (shadow && !m_stats.mounted)
	{
		ICharacterInstance *pOwnerCharacter = pOwnerActor->GetEntity()->GetCharacter(0);
		if (!pOwnerCharacter)
			return;

		IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_params.attachment[m_stats.hand].c_str());

		if (!pAttachment)
		{
			GameWarning("Item owner '%s' doesn't have third-person item attachment point '%s'!", pOwnerActor->GetEntity()->GetName(), m_params.attachment[m_stats.hand].c_str());
			return;
		}

		if (!shadow)
			pAttachment->ClearBinding();
		else if (IStatObj *pStatObj=GetEntity()->GetStatObj(eIGS_ThirdPerson))
		{
			CCGFAttachment *pCGFAttachment = new CCGFAttachment();
			pCGFAttachment->pObj = pStatObj;

			pAttachment->AddBinding(pCGFAttachment);
			pAttachment->HideAttachment(1);
			pAttachment->HideInShadow(0);
		}
	}
}

//------------------------------------------------------------------------
void CItem::Impulse(const Vec3 &position, const Vec3 &direction, float impulse)
{
	if (direction.len2() <= 0.001f)
		return;

	IEntityPhysicalProxy *pPhysicsProxy = GetPhysicalProxy();
	if (pPhysicsProxy)
		pPhysicsProxy->AddImpulse(-1, position, direction.GetNormalized()*impulse, true, 1);
}

//------------------------------------------------------------------------
bool CItem::CanPickUp(EntityId userId) const
{
	if (m_params.pickable && m_stats.pickable && !m_stats.flying && !m_frozen &&(!m_ownerId || m_ownerId==userId) && !m_stats.selected && !m_stats.used)
	{
		IInventory *pInventory=GetActorInventory(GetActor(userId));
		
		//LAW stuff
		if(m_params.auto_droppable)
		{
			//Can not pick up a LAW while I have one already
			if(pInventory && pInventory->GetCountOfClass("LAW")>0)
			{
				g_pGame->GetGameRules()->OnTextMessage(eTextMessageCenter, "You can not carry more than one rocket launcher");
				return false;
			}
		}
		if (pInventory && pInventory->FindItem(GetEntityId())==-1)
			return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool CItem::CanDrop() const
{
	if (m_params.droppable)
		return true;

	return false;
}

//------------------------------------------------------------------------
bool CItem::CanUse(EntityId userId) const
{
	return m_params.usable && m_properties.usable && IsMounted() && (!m_stats.used || (m_ownerId == userId));
}

//------------------------------------------------------------------------
bool CItem::IsMounted() const
{
	return m_stats.mounted;
}


//------------------------------------------------------------------------
void CItem::SetMountedAngleLimits(float min_pitch, float max_pitch, float yaw_range)
{
	m_mountparams.min_pitch = min_pitch;
	m_mountparams.max_pitch = max_pitch;
	m_mountparams.yaw_range = yaw_range;
}


//------------------------------------------------------------------------
Vec3 CItem::GetMountedAngleLimits() const
{
	if(m_stats.mounted)
		return Vec3(m_mountparams.min_pitch, m_mountparams.max_pitch, m_mountparams.yaw_range);
	else 
		return ZERO;
}

//------------------------------------------------------------------------
bool CItem::IsUsed() const
{
	return m_stats.used;
}

//------------------------------------------------------------------------
bool CItem::InitRespawn()
{
	if (IsServer() && m_respawnprops.respawn)
	{
		CGameRules *pGameRules=g_pGame->GetGameRules();
		assert(pGameRules);
		if (pGameRules)
			pGameRules->CreateEntityRespawnData(GetEntityId());

		return true;
	}

	return false;
};

//------------------------------------------------------------------------
void CItem::TriggerRespawn()
{
	if (!m_stats.brandnew || !IsServer())
		return;
	
	if (m_respawnprops.respawn)
	{

		CGameRules *pGameRules=g_pGame->GetGameRules();
		assert(pGameRules);
		if (pGameRules)
			pGameRules->ScheduleEntityRespawn(GetEntityId(), m_respawnprops.unique, m_respawnprops.timer);
	}
}

//------------------------------------------------------------------------
void CItem::EnableSelect(bool enable)
{
	m_stats.selectable = enable;
}

//------------------------------------------------------------------------
bool CItem::CanSelect() const
{
	if(g_pGameCVars->g_proneNotUsableWeapon_FixType == 1)
		if(GetOwnerActor() && GetOwnerActor()->GetStance() == STANCE_PRONE && GetParams().prone_not_usable)
			return false;

	return m_params.selectable && m_stats.selectable;
}

//------------------------------------------------------------------------
bool CItem::IsSelected() const
{
	return m_stats.selected;
}

//------------------------------------------------------------------------
void CItem::EnableSound(bool enable)
{
	m_stats.sound_enabled = enable;
}

//------------------------------------------------------------------------
bool CItem::IsSoundEnabled() const
{
	return m_stats.sound_enabled;
}

//------------------------------------------------------------------------
void CItem::MountAt(const Vec3 &pos)
{
	if (!m_params.mountable)
		return;

	m_stats.mounted = true;

	SetViewMode(eIVM_FirstPerson);
	
	Matrix34 tm(GetEntity()->GetWorldTM());
	tm.SetTranslation(pos);
	GetEntity()->SetWorldTM(tm);

	m_stats.mount_dir = GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION);
}

//------------------------------------------------------------------------
void CItem::MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles)
{
	if (!m_params.mountable)
		return;

	IEntity *pHost = m_pEntitySystem->GetEntity(entityId);
	if (!pHost)
		return;

	m_hostId = entityId;
	m_stats.mounted = true;

	SetViewMode(eIVM_FirstPerson);

	pHost->AttachChild(GetEntity(), 0);

	Matrix34 tm = Matrix33(Quat::CreateRotationXYZ(angles));
	tm.SetTranslation(pos);
	GetEntity()->SetLocalTM(tm);

	m_stats.mount_dir = GetEntity()->GetWorldTM().TransformVector(FORWARD_DIRECTION);
}


//------------------------------------------------------------------------
IEntity *CItem::GetOwner() const
{
	if (!m_ownerId)
		return 0;

	return m_pEntitySystem->GetEntity(m_ownerId);
}

//------------------------------------------------------------------------
CActor *CItem::GetOwnerActor() const
{
	if(!m_pGameFramework)
		return NULL;
	return static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActor(m_ownerId));
}

//------------------------------------------------------------------------
CActor *CItem::GetActor(EntityId actorId) const
{
	return static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActor(actorId));
}

//------------------------------------------------------------------------
IInventory *CItem::GetActorInventory(IActor *pActor) const
{
	if (!pActor)
		return 0;

	return pActor->GetInventory();
}

//------------------------------------------------------------------------
CItem *CItem::GetActorItem(IActor *pActor) const
{
	if (!pActor)
		return 0;

	IInventory *pInventory=pActor->GetInventory();
	if (!pInventory)
		return 0;

	EntityId id = pInventory->GetCurrentItem();
	if (!id)
		return 0;

	return static_cast<CItem *>(m_pItemSystem->GetItem(id));
}

//------------------------------------------------------------------------
EntityId CItem::GetActorItemId(IActor *pActor) const
{
	if (!pActor)
		return 0;

	IInventory *pInventory=pActor->GetInventory();
	if (!pInventory)
		return 0;

	EntityId id = pInventory->GetCurrentItem();
	if (!id)
		return 0;

	return id;
}

//------------------------------------------------------------------------
CActor *CItem::GetActorByNetChannel(INetChannel *pNetChannel) const
{
	return static_cast<CActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel)));
}

//------------------------------------------------------------------------
void CItem::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	const char* temp = actionId.c_str();
	CallScriptEvent(eISET_Client, "OnAction", actorId, temp, activationMode, value, 0, 0, 0);
}

//------------------------------------------------------------------------
void CItem::StartUse(EntityId userId)
{
	if (!m_params.usable || m_ownerId)
		return;

	ScriptHandle scriptHandle(userId);
	CallScriptEvent(eISET_All, "OnStartUse", scriptHandle, 0, 0, 0);

	// holster user item here
	SetOwnerId(userId);
	m_pItemSystem->SetActorItem(GetOwnerActor(), GetEntityId(), true);

	m_stats.used = true;

	CActor *pActor = GetOwnerActor();
	if (!pActor)
		return;

	SActorParams *pParams = pActor->GetActorParams();

	pParams->viewPivot = GetEntity()->GetWorldPos();
	pParams->viewDistance = -m_mountparams.eye_distance;
	pParams->viewHeightOffset = m_mountparams.eye_height;
	pParams->vLimitDir = m_stats.mount_dir;
	pParams->vLimitRangeH = DEG2RAD(m_mountparams.yaw_range);
	pParams->vLimitRangeV = DEG2RAD((m_mountparams.max_pitch-m_mountparams.min_pitch)*0.5f);
	pParams->speedMultiplier = 0.0f;

	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	// TODO: precreate this table
	SmartScriptTable locker(gEnv->pScriptSystem);
	locker->SetValue("locker", ScriptHandle(GetEntityId()));
	locker->SetValue("lockId", ScriptHandle(GetEntityId()));
	locker->SetValue("lockIdx", 1);
	pActor->GetGameObject()->SetExtensionParams("Interactor", locker);

	pActor->LinkToMountedWeapon(GetEntityId());

	if (IsServer())
		pActor->GetGameObject()->InvokeRMI(CActor::ClStartUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);
}

//------------------------------------------------------------------------
void CItem::StopUse(EntityId userId)
{
	if (userId != m_ownerId)
		return;

	ScriptHandle scriptHandle(userId);
	CallScriptEvent(eISET_All, "OnStopUse", scriptHandle, 0, 0, 0);

	CActor *pActor = GetOwnerActor();
	if (!pActor)
		return;

	if (pActor->GetHealth()>0)
		pActor->SelectLastItem(true);

	pActor->GetAnimationGraphState()->SetInput("Action","idle");

  SActorParams *pParams = pActor->GetActorParams();
	pParams->viewPivot.zero();
	pParams->viewDistance = 0.0f;
	pParams->viewHeightOffset = 0.0f;
	pParams->vLimitDir.zero();
	pParams->vLimitRangeH = 0.0f;
	pParams->vLimitRangeV = 0.0f;
	pParams->speedMultiplier = 1.0f;

  if (m_stats.mounted)
  {
    AttachArms(false, false);
  }
		
	EnableUpdate(false);

	m_stats.used = false;

	SetOwnerId(0);

	// TODO: precreate this table
	SmartScriptTable locker(gEnv->pScriptSystem);
	locker->SetValue("locker", ScriptHandle(GetEntityId()));
	locker->SetValue("lockId", ScriptHandle(0));
	locker->SetValue("lockIdx", 0);
	pActor->GetGameObject()->SetExtensionParams("Interactor", locker);

	pActor->LinkToMountedWeapon(0);

	if (IsServer())
		pActor->GetGameObject()->InvokeRMI(CActor::ClStopUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToAllClients|eRMI_NoLocalCalls);
}

//------------------------------------------------------------------------
void CItem::UseManualBlending(bool enable)
{
  IActor* pActor = GetOwnerActor();
  if (!pActor)
    return;

  if (ICharacterInstance* pCharInstance = pActor->GetEntity()->GetCharacter(0))
  {
    if (ISkeleton* pSkeleton = pCharInstance->GetISkeleton())
    { 
      pSkeleton->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, 0.f, enable);
    }        
  } 
}

//------------------------------------------------------------------------
void CItem::AttachToHand(bool attach)
{
  if (m_stats.mounted)
    return;

	IEntity *pOwner = GetOwner();
	if (!pOwner)
		return;

	ICharacterInstance *pOwnerCharacter = pOwner->GetCharacter(0);
	if (!pOwnerCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_params.attachment[m_stats.hand].c_str());

	if (!pAttachment)
	{
		GameWarning("Item owner '%s' doesn't have third-person item attachment point '%s'!", pOwner->GetName(), m_params.attachment[m_stats.hand].c_str());
		return;
	}

	if (!attach)
	{
		pAttachment->ClearBinding();
	}
	else
	{
		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());

		pAttachment->AddBinding(pEntityAttachment);
		pAttachment->HideAttachment(0);
	}
}

//------------------------------------------------------------------------
void CItem::RequireUpdate(int slot)
{
	if (slot==-1)
		for (int i=0;i<4;i++)
			GetGameObject()->ForceUpdateExtension(this, i);	
	else
		GetGameObject()->ForceUpdateExtension(this, slot);
}

//------------------------------------------------------------------------
void CItem::EnableUpdate(bool enable, int slot)
{
	if (enable)
	{
		if (slot==-1)
			for (int i=0;i<4;i++)
				GetGameObject()->EnableUpdateSlot(this, i);
		else
			GetGameObject()->EnableUpdateSlot(this, slot);

	}
	else
	{
		if (slot==-1)
		{
			for (int i=0;i<4;i++)
				GetGameObject()->DisableUpdateSlot(this, i);
		}
		else
			GetGameObject()->DisableUpdateSlot(this, slot);
	}
}

//------------------------------------------------------------------------
void CItem::Hide(bool hide)
{
	GetEntity()->SetFlags(GetEntity()->GetFlags()&~ENTITY_FLAG_UPDATE_HIDDEN);

	if ((hide && m_stats.fp) || IsServer())
		GetEntity()->SetFlags(GetEntity()->GetFlags()|ENTITY_FLAG_UPDATE_HIDDEN);	

	GetEntity()->Hide(hide);
}

//------------------------------------------------------------------------
void CItem::HideArms(bool hide)
{
	HideCharacterAttachment(eIGS_FirstPerson, ITEM_ARMS_ATTACHMENT_NAME, hide);
}

//------------------------------------------------------------------------
void CItem::HideItem(bool hide)
{
	HideCharacterAttachmentMaster(eIGS_FirstPerson, ITEM_ARMS_ATTACHMENT_NAME, hide);

	IEntity *pOwner = GetOwner();
	if (!pOwner)
		return;

	ICharacterInstance *pOwnerCharacter = pOwner->GetCharacter(0);
	if (!pOwnerCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_params.attachment[m_stats.hand].c_str());
	if (pAttachment)
		pAttachment->HideAttachment(hide?1:0);
}

//------------------------------------------------------------------------
void CItem::Freeze(bool freeze)
{
	if (freeze)
	{
		m_frozen = true;
		for (int i=0; i<eIGS_Last; i++)
		{
			ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
			if (pCharacter)
				pCharacter->SetAnimationSpeed(0);
		}

		Quiet();
	}
	else
	{
		m_frozen = false;
		for (int i=0; i<eIGS_Last; i++)
		{
			ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
			if (pCharacter)
				pCharacter->SetAnimationSpeed(m_animationSpeed[i]);
		}
	}
}

IMaterial *CItem::Cloak(bool cloak, IMaterial *cloakMat)
{
	if (cloak == m_cloaked)
		return cloakMat;

	if(cloak )	//when switching view there are some errors without this check
	{
    CActor* pActor = GetOwnerActor();
    if (pActor && pActor->GetActorClass() == CPlayer::GetActorClassType())
    {
      CPlayer *plr = (CPlayer *)pActor;
      if(plr->GetNanoSuit() && plr->GetNanoSuit()->GetMode() != NANOMODE_CLOAK)
        return cloakMat;
    }    
	}

	SEntitySlotInfo slotInfo;

	
	if (GetEntity()->GetSlotInfo(CItem::eIGS_FirstPerson, slotInfo))
	{
		if (slotInfo.pCharacter)
		{
			SetMaterialRecursive(slotInfo.pCharacter, !cloak, cloakMat);
		}
	}


	GetEntity()->SetMaterial(cloakMat);
	for (int i = 0; i < GetEntity()->GetChildCount(); i++)
	{
		IEntity *child = GetEntity()->GetChild(i);
		if (child)
		{
			child->SetMaterial(cloakMat);
		}
	}
	IMaterial *ret = 0;
	if (cloak)
	{
		m_pLastCloakMat = cloakMat;
		ret = cloakMat;
	}
	else
	{
		m_testOldMats.clear();
		m_attchObjMats.clear();
		ret = m_pLastCloakMat;
	}
	m_cloaked = cloak;
	return ret;
}

void CItem::SetMaterialRecursive(ICharacterInstance *charInst, bool undo, IMaterial *newMat)
{
	if (!charInst || (!undo && !newMat))
		return;
	if ((!undo && m_testOldMats.find(charInst) != m_testOldMats.end()) || (undo && m_testOldMats.find(charInst) == m_testOldMats.end()))
		return;

	if (undo)
		charInst->SetMaterial(m_testOldMats[charInst]);
	else
	{
		IMaterial *oldMat = charInst->GetMaterial();
		if (newMat != oldMat)
		{
			m_testOldMats[charInst] = oldMat;
			charInst->SetMaterial(newMat);
		}
	}
	for (int i = 0; i < charInst->GetIAttachmentManager()->GetAttachmentCount(); i++)
	{
		IAttachment *attch = charInst->GetIAttachmentManager()->GetInterfaceByIndex(i);
		if (attch)
		{
			IAttachmentObject *obj = attch->GetIAttachmentObject();
			if (obj)
			{
				SetMaterialRecursive(obj->GetICharacterInstance(), undo, newMat);
				if (!obj->GetICharacterInstance() && ((!undo && m_attchObjMats.find(obj) == m_attchObjMats.end()) || undo && m_attchObjMats.find(obj) != m_attchObjMats.end()))
				{

					if (undo)
						obj->SetMaterial(m_attchObjMats[obj]);
					else
					{
						IMaterial *oldMat = obj->GetMaterial();
						if (oldMat != newMat)
						{
							m_attchObjMats[obj] = obj->GetMaterial();
							obj->SetMaterial(newMat);
						}
					}
				}
			}
		}
	}
}

void CItem::TakeAccessories(EntityId receiver)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(receiver);
	IInventory *pInventory = GetActorInventory(pActor);

	if (pInventory)
	{
		for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
		{
			const EntityId id = it->second;
			const char* name = it->first;
			if (!pInventory->GetCountOfClass(name))
				g_pGame->GetIGameFramework()->GetIItemSystem()->GiveItem(pActor, name, false, false, false);
		}
	}
}

void CItem::AddAccessoryAmmoToInventory(IEntityClass* pAmmoType, int count)
{
	IInventory *pInventory=GetActorInventory(GetOwnerActor());
	if (!pInventory)
		return;

	int capacity = pInventory->GetAmmoCapacity(pAmmoType);
	int current = pInventory->GetAmmoCount(pAmmoType);
	if((!gEnv->pSystem->IsEditor()) && ((current+count) > capacity))
	{
		if(g_pGame->GetHUD())
			g_pGame->GetHUD()->DisplayFlashMessage("@ammo_maxed_out", 2, ColorF(1.0f, 0,0), true, pAmmoType->GetName());

		//If still there's some place, full inventory to maximum...
		if(current<capacity)
		{
			pInventory->SetAmmoCount(pAmmoType,capacity);
			if (IsServer())
				GetOwnerActor()->GetGameObject()->InvokeRMI(CActor::ClSetAmmo(), CActor::AmmoParams(pAmmoType->GetName(), capacity), eRMI_ToRemoteClients);

		}
	}
	else
	{
		int newCount = current+count;
		pInventory->SetAmmoCount(pAmmoType, newCount);

		if (IsServer())
			GetOwnerActor()->GetGameObject()->InvokeRMI(CActor::ClSetAmmo(), CActor::AmmoParams(pAmmoType->GetName(), newCount), eRMI_ToRemoteClients);
	}
}

int CItem::GetAccessoryAmmoCount(IEntityClass* pAmmoType)
{
	IInventory *pInventory=GetActorInventory(GetOwnerActor());
	if (!pInventory)
		return 0;

	return pInventory->GetAmmoCount(pAmmoType);
}

bool CItem::CheckAmmoRestrictions(EntityId pickerId)
{
	if(g_pGameCVars->i_unlimitedammo != 0)
		return true;

	IActor* pPicker = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pickerId);
	if(pPicker)
	{

		IInventory *pInventory = pPicker->GetInventory();
		if(pInventory)
		{
			if(pInventory->GetCountOfClass(GetEntity()->GetClass()->GetName()) == 0)
				return true;

			if(!m_bonusAccessoryAmmo.empty())
			{
				for (TAccessoryAmmoMap::const_iterator it=m_bonusAccessoryAmmo.begin(); it!=m_bonusAccessoryAmmo.end(); ++it)
				{
					int invAmmo  = pInventory->GetAmmoCount(it->first);
					int invLimit = pInventory->GetAmmoCapacity(it->first);

					if(invAmmo>=invLimit && (!gEnv->pSystem->IsEditor()))
						return false;
				}
			}
		}
	}

	return true;
}

void CItem::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	m_params.GetMemoryStatistics(s);
	m_mountparams.GetMemoryStatistics(s);
	m_camerastats.GetMemoryStatistics(s);
	m_properties.GetMemoryStatistics(s);
	s->AddContainer(m_damageLevels);
	s->AddContainer(m_effects);
	s->AddContainer(m_activelayers);
	s->AddContainer(m_accessories);
	s->AddContainer(m_initialSetup);
	s->AddContainer(m_instanceActions);
	m_scheduler.GetMemoryStatistics(s);
	for (int i=0; i<eIGS_Last; i++)
		s->Add(m_idleAnimation[i]);
	s->Add(m_actionSuffix);
	for (int i=0; i<3; i++)
		m_fpgeometry[i].GetMemoryStatistics(s);
	s->AddContainer(m_testOldMats);
	s->AddContainer(m_attchObjMats);
	for (int i=0; i<eIGS_Last; i++)
		s->Add(m_geometry[i]);

	for (TInitialSetup::iterator iter = m_initialSetup.begin(); iter != m_initialSetup.end(); ++iter)
		s->Add(*iter);
	for (TEffectInfoMap::iterator iter = m_effects.begin(); iter != m_effects.end(); ++iter)
		iter->second.GetMemoryStatistics(s);
	for (TInstanceActionMap::iterator iter = m_instanceActions.begin(); iter != m_instanceActions.end(); ++iter)
		iter->second.GetMemoryStatistics(s);
}


SItemStrings* g_pItemStrings = 0;

SItemStrings::~SItemStrings()
{
	g_pItemStrings = 0;
}

SItemStrings::SItemStrings()
{
	g_pItemStrings = this;

	activate = "activate";
	begin_reload = "begin_reload";
	cannon = "cannon";
	change_firemode = "change_firemode";
	crawl = "crawl";
	deactivate = "deactivate";
	deselect = "deselect";
	destroy = "destroy";
	enter_modify = "enter_modify";
	exit_reload_nopump = "exit_reload_nopump";
	exit_reload_pump = "exit_reload_pump";
	fire = "fire";
	idle = "idle";
	idle_relaxed = "idle_relaxed";
	idle_raised = "idle_raised";
	jump_end = "jump_end";
	jump_idle = "jump_idle";
	jump_start = "jump_start";
	leave_modify = "leave_modify";
	left_item_attachment = "left_item_attachment";
	lock = "lock";
	lower = "lower";
	modify_layer = "modify_layer";
	nw = "nw";
	offhand_on = "offhand_on";
	offhand_off = "offhand_off";
	pickedup = "pickedup";
	pickup_weapon_left = "pickup_weapon_left";
	raise = "raise";
	reload_shell = "reload_shell";
	right_item_attachment = "right_item_attachment";
	run_forward = "run_forward";
	SCARSleepAmmo = "SCARSleepAmmo";
	SCARTagAmmo = "SCARTagAmmo";
	select = "select";
	select_grenade = "select_grenade";
	swim_idle = "swim_idle";
	swim_forward = "swim_forward";
	swim_forward_2 = "swim_forward_2";
	swim_backward = "swim_backward";
	speed_swim = "speed_swim";
	turret = "turret";  
  enable_light = "enable_light";
  disable_light = "disable_light";
  use_light = "use_light";
	first_select = "first_select";
	LAM = "LAM";
	LAMRifle = "LAMRifle";
	Silencer = "Silencer";
	SOCOMSilencer = "SOCOMSilencer";
	lever_layer_1 = "lever_layer_1";
	lever_layer_2 = "lever_layer_2";
};

