/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 12:04:2006   17:22 : Created by Márcio Martins
- 18:02:2007	 13:30 : Refactored Offhand by Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "OffHand.h"
#include "Actor.h"
#include "Throw.h"
#include "GameRules.h"
#include <IWorldQuery.h>
#include "Fists.h"
#include "GameActions.h"
#include "Melee.h"

#include "HUD/HUD.h"
#include "HUD/HUDCrosshair.h"

#define KILL_NPC_TIMEOUT	7.25f
#define TIME_TO_UPDATE_CH 0.25f

#define MAX_CHOKE_SOUNDS	5

#define MAX_GRENADE_TYPES 4

//Sounds tables
namespace 
{
	const char gChokeSoundsTable[MAX_CHOKE_SOUNDS][64] =
	{
		"Languages/dialog/ai_korean_soldier_1/choke_01.mp3",
		"Languages/dialog/ai_korean_soldier_2/choke_02.mp3",
		"Languages/dialog/ai_korean_soldier_1/choke_03.mp3",
		"Languages/dialog/ai_korean_soldier_3/choke_04.mp3",
		"Languages/dialog/ai_korean_soldier_1/choke_05.mp3"
	};
	const char gDeathSoundsTable[MAX_CHOKE_SOUNDS][64] =
	{
		"Languages/dialog/ai_korean_soldier_1/choke_grab_00.mp3",
		"Languages/dialog/ai_korean_soldier_1/choke_grab_01.mp3",
		"Languages/dialog/ai_korean_soldier_1/choke_grab_02.mp3",
		"Languages/dialog/ai_korean_soldier_1/choke_grab_03.mp3",
		"Languages/dialog/ai_korean_soldier_1/choke_grab_04.mp3"
	};
}

//========================Scheduled offhand actions =======================//
namespace
{
	//This class help us to select the correct action
	class FinishOffHandAction
	{
	public:
		FinishOffHandAction(EOffHandActions _eOHA,COffHand *_pOffHand)
		{
			eOHA = _eOHA;
			pOffHand = _pOffHand;
		}
		void execute(CItem *cItem)
		{
			pOffHand->FinishAction(eOHA);
		}

	private:

		EOffHandActions eOHA;
		COffHand				*pOffHand;
	};

	//End finish grenade action (switch/throw)
	class FinishGrenadeAction
	{
	public:
		FinishGrenadeAction(COffHand *_pOffHand, CItem *_pMainHand)
		{
			pOffHand  = _pOffHand;
			pMainHand = _pMainHand;
		}
		void execute(CItem *cItem)
		{
			pOffHand->HideItem(true);
			float timeDelay = 0.1f;	//ms

			if(pMainHand && !pMainHand->IsDualWield())
			{
				pMainHand->ResetDualWield();		//I can reset, because if DualWield it's not possible to switch grenades (see PreExecuteAction())
				pMainHand->PlayAction(g_pItemStrings->offhand_off, 0, false, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
				timeDelay = (pMainHand->GetCurrentAnimationTime(CItem::eIGS_FirstPerson) + 50)*0.001f;
			}
			else
			{
				pOffHand->GetOwnerActor()->HolsterItem(false);
			}
			pOffHand->SetOffHandState(eOHS_TRANSITIONING);

			//Offhand goes back to initial state
			pOffHand->SetResetTimer(timeDelay);
			pOffHand->RequireUpdate(eIUS_General);
		}

	private:
		COffHand *pOffHand;
		CItem    *pMainHand;
	};
}

//=====================~Scheduled offhand actions======================//

COffHand::COffHand():
m_currentState(eOHS_INIT_STATE),
m_mainHand(NULL),
m_mainHandWeapon(NULL),
m_nextGrenadeThrow(-1.0f),
m_lastFireModeId(0),
m_range(2.15f),
m_usable(false),
m_heldEntityId(0),
m_pickingTimer(-1.0f),
m_resetTimer(-1.0f),
m_preHeldEntityId(0),
m_grabbedNPCSpecies(eGCT_UNKNOWN),
m_killTimeOut(-1.0f),
m_killNPC(false),
m_effectRunning(false),
m_grabbedNPCInitialHealth(0),
m_npcWasDead(false),
m_lastCHUpdate(0.0f),
m_heldEntityMass(0.0f)
{
}

//=============================================================
COffHand::~COffHand()
{
}

//=============================================================
void COffHand::Reset()
{
	CWeapon::Reset();

	if(m_heldEntityId)
	{
		//Prevent editor-reset issues
		if(m_currentState&(eOHS_GRABBING_NPC|eOHS_HOLDING_NPC|eOHS_THROWING_NPC))
		{
			ThrowNPC(false);
		}
		else if(m_currentState&(eOHS_PICKING|eOHS_HOLDING_OBJECT|eOHS_THROWING_OBJECT|eOHS_MELEE))
		{
			IgnoreCollisions(false,m_heldEntityId);
			DrawNear(false,m_heldEntityId);
		}
	}

	m_nextGrenadeThrow = -1.0f;
	m_lastFireModeId   = 0;
	m_pickingTimer     = -1.0f;
	m_heldEntityId		 = 0;
	m_preHeldEntityId  = 0;
	m_constraintId     = 0;
	m_resetTimer			 = -1.0f;
	m_killTimeOut			 = -1.0f;
	m_killNPC					 = false;
	m_effectRunning    = false;
	m_npcWasDead       = false;
	m_grabbedNPCSpecies = eGCT_UNKNOWN;
	m_lastCHUpdate			= 0.0f;
	m_heldEntityMass		= 0.0f;
		
	SetOffHandState(eOHS_INIT_STATE);
}

//=============================================================
void COffHand::PostInit(IGameObject *pGameObject)
{
	CWeapon::PostInit(pGameObject);

	m_lastFireModeId = 0;
	SetCurrentFireMode(0);
	//EnableUpdate(true,eIUS_General);
}

//============================================================
bool COffHand::ReadItemParams(const IItemParamsNode *root)
{
	if (!CWeapon::ReadItemParams(root))
		return false;

	m_grabTypes.clear();

	//Read offHand grab types
	if (const IItemParamsNode* pickabletypes = root->GetChild("pickabletypes"))
	{
		int n = pickabletypes->GetChildCount();
		for (int i = 0; i<n; ++i)
		{
			const IItemParamsNode* pt = pickabletypes->GetChild(i);

			SGrabType grabType;
			grabType.helper = pt->GetAttribute("helper");
			grabType.pickup = pt->GetAttribute("pickup");
			grabType.idle = pt->GetAttribute("idle");
			grabType.throwFM = pt->GetAttribute("throwFM");

			if (strcmp(pt->GetName(), "onehanded") == 0)
			{
				grabType.twoHanded = false;
				m_grabTypes.push_back(grabType);
			}
			else if(strcmp(pt->GetName(), "twohanded") == 0)
			{
				grabType.twoHanded = true;
				m_grabTypes.push_back(grabType);
			}
		}
	}


	return true;
}
//============================================================
void COffHand::Serialize(TSerialize ser, unsigned aspects)
{
	CWeapon::Serialize(ser, aspects);

	if(ser.GetSerializationTarget() != eST_Network)
	{
		EntityId oldHeldId = m_heldEntityId;

		ser.Value("m_lastFireModeId", m_lastFireModeId);
		ser.Value("m_usable", m_usable);
		ser.Value("m_currentState", m_currentState);
		ser.Value("m_heldEntityId", m_heldEntityId);
		ser.Value("m_constraintId", m_constraintId);
		ser.Value("m_grabType", m_grabType);
		ser.Value("m_grabbedNPCSpecies", m_grabbedNPCSpecies);
		ser.Value("m_killTimeOut", m_killTimeOut);
		ser.Value("m_effectRunning",m_effectRunning);
		ser.Value("m_grabbedNPCInitialHealth",m_grabbedNPCInitialHealth);

		if(ser.IsReading() && m_heldEntityId != oldHeldId)
		{
			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(oldHeldId);
			if(pActor)
			{
				pActor->GetEntity()->GetCharacter(0)->SetFlags(pActor->GetEntity()->GetCharacter(0)->GetFlags()&(~ENTITY_SLOT_RENDER_NEAREST));
			}
			else
				DrawNear(false, oldHeldId);
		}
	}
}

//============================================================
void COffHand::PostSerialize()
{
	SetCurrentFireMode(m_lastFireModeId);

	if(m_currentState==eOHS_PICKING_ITEM)
	{
		//If picking an item...
		if(m_heldEntityId)
		{
			CActor* pPlayer = GetOwnerActor();
			if(pPlayer)
			{
				m_currentState = eOHS_PICKING_ITEM2;
				pPlayer->PickUpItem(m_heldEntityId,true);
				IgnoreCollisions(false,m_heldEntityId);
			}
		}
		SetOffHandState(eOHS_INIT_STATE);		
	}
	else if(m_heldEntityId && m_currentState!=eOHS_TRANSITIONING)
	{
		CItem			*pMain = GetActorItem(GetOwnerActor());
		if(pMain && pMain->IsBusy())
			pMain->SetBusy(false);

		//If holding an object or NPC
		if(m_currentState&(eOHS_HOLDING_OBJECT|eOHS_PICKING|eOHS_THROWING_OBJECT|eOHS_MELEE))
		{
			IgnoreCollisions(false, m_heldEntityId);
			DrawNear(false);

			//Do grabbing again
			m_currentState = eOHS_INIT_STATE;
			m_preHeldEntityId = m_heldEntityId;
			PreExecuteAction(eOHA_USE,eAAM_OnPress);
			PickUpObject();
		}
		else if(m_currentState&(eOHS_HOLDING_NPC|eOHS_GRABBING_NPC|eOHS_THROWING_NPC))
		{
			//Do grabbing again
			m_currentState = eOHS_INIT_STATE;
			m_preHeldEntityId = m_heldEntityId;
			PreExecuteAction(eOHA_USE,eAAM_OnPress);
			PickUpObject(true);
		}
	}
	else if(m_currentState!=eOHS_INIT_STATE)
	{
		SetOffHandState(eOHS_INIT_STATE);	
	}	
}

//=============================================================
bool COffHand::CanSelect() const
{
	return false;
}

//=============================================================
void COffHand::Select(bool select)
{
	CWeapon::Select(select);
}

//=============================================================
void COffHand::Update(SEntityUpdateContext &ctx, int slot)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	bool keepUpdating = false;

	CWeapon::Update(ctx, slot);

	if (slot==eIUS_General)
	{
		if(m_resetTimer>=0.0f)
		{
			m_resetTimer -= ctx.fFrameTime;
			if(m_resetTimer<0.0f)
			{
				SetOffHandState(eOHS_INIT_STATE);
				m_resetTimer = -1.0f;
			}
			else
			{
				keepUpdating = true;
			}
		}
		if (m_nextGrenadeThrow>=0.0f)
		{
			//Grenade throw fire rate
			m_nextGrenadeThrow -= ctx.fFrameTime;
			keepUpdating = true;
		}
		if(m_pickingTimer>=0.0f)
		{
			m_pickingTimer -= ctx.fFrameTime;

			if(m_pickingTimer<0.0f)
			{
				PerformPickUp();
			}
			else
			{
				keepUpdating = true;
			}
		}
		if(m_killTimeOut>=0.0f)
		{
			m_killTimeOut -= ctx.fFrameTime;
			if(m_killTimeOut<0.0f)
			{
				m_killTimeOut = -1.0f;
				m_killNPC = true;
			}
			else
			{
				keepUpdating = true;
			}
		}

		if(keepUpdating)
			RequireUpdate(eIUS_General);
		else
			EnableUpdate(false,eIUS_General);
	}
}

//=============================================================

void COffHand::UpdateFPView(float frameTime)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_GAME );

	CWeapon::UpdateFPView(frameTime);

	CheckViewChange();

	m_lastCHUpdate += frameTime;

	if(m_stats.fp && m_currentState==eOHS_INIT_STATE)
	{
		UpdateCrosshairUsability();
		UpdateMainWeaponRaising();
		
		//Fix offhand floating on spawn (not really nice fix...)
		if(m_stats.hand==0)
		{
 			SetHand(eIH_Left);					//This will be only done once after loading
			Select(true);Select(false);
		}
		//=========================================================
	}
	else if(m_heldEntityId && m_currentState&(eOHS_HOLDING_OBJECT|eOHS_PICKING|eOHS_THROWING_OBJECT|eOHS_PICKING_ITEM|eOHS_MELEE))
	{
		UpdateHeldObject();
	}
}

//============================================================

void COffHand::UpdateCrosshairUsability()
{
	//Only update a few times per second
	if(m_lastCHUpdate>TIME_TO_UPDATE_CH)
		m_lastCHUpdate = 0.0f;
	else
		return;

	CActor *pActor=GetOwnerActor();
	if (pActor && pActor->IsPlayer())
	{
		IPhysicalEntity *pPhysicalEntity=0;
		IWorldQuery *pWorldQuery=pActor->GetGameObject()->GetWorldQuery();
		assert(pWorldQuery);

		EntityId hitEntityId = pWorldQuery->GetLookAtEntityId();
		if (hitEntityId)
		{
			IEntity *pEntity = m_pEntitySystem->GetEntity(hitEntityId);
			if (pEntity)
				pPhysicalEntity = pEntity->GetPhysics();
		}
		else
		{
			const ray_hit *pRay = pWorldQuery->GetLookAtPoint(m_range);
			if (pRay)
				pPhysicalEntity = pRay->pCollider;
		}

		CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
		bool isLadder = pPlayer->IsLadderUsable();
		if (pPhysicalEntity && (CanPerformPickUp(pActor, pPhysicalEntity) || isLadder))
		{
			if (g_pGame->GetHUD())
			{
				if(IItem *pItem = m_pItemSystem->GetItem(hitEntityId))
				{
					if(pItem->GetIWeapon())
						g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@pick_weapon", pItem->GetEntity()->GetClass()->GetName());
					else
						g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@pick_item", pItem->GetEntity()->GetClass()->GetName());
				}
				else if(IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitEntityId))
					g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@grab_enemy");
				else if(isLadder)
					g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@use_ladder");
				else
					g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@grab_object");
			}
			m_usable=true;
		}
		else if(pPhysicalEntity && gEnv->bMultiplayer)
		{
			//Offhand pick ups are disabled in MP (check only for items)
			if(g_pGame->GetHUD())
			{
				if(IItem *pItem = m_pItemSystem->GetItem(hitEntityId))
				{
					if(pItem->GetIWeapon())
						g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@pick_weapon", pItem->GetEntity()->GetClass()->GetName());
					else
						g_pGame->GetHUD()->GetCrosshair()->SetUsability(true, "@pick_item", pItem->GetEntity()->GetClass()->GetName());
				}
			}
			m_usable=true;
		}
		else if (m_usable)
		{
			if (g_pGame->GetHUD())
				g_pGame->GetHUD()->GetCrosshair()->SetUsability(false, "");
			m_usable=false;
		}
	}
}

//=============================================================
void COffHand::UpdateHeldObject()
{
	IEntity *pEntity=m_pEntitySystem->GetEntity(m_heldEntityId);
	if(pEntity)
	{
		if(pEntity->GetPhysics() && m_constraintId)
		{
			pe_status_constraint state;
			state.id = m_constraintId;

			//The constraint was removed (the object was broken/destroyed)
			if(!pEntity->GetPhysics()->GetStatus(&state) && (m_currentState!=eOHS_THROWING_OBJECT))
			{
				if(m_mainHand && m_mainHand->IsBusy())
					m_mainHand->SetBusy(false);
			
				m_currentState = eOHS_HOLDING_OBJECT;
				OnAction(GetOwnerId(),"use",eAAM_OnPress,0.0f);
				OnAction(GetOwnerId(),"use",eAAM_OnRelease,0.0f);
				return;
			}

		}

		//Update entity WorldTM 
		int id=m_stats.fp?eIGS_FirstPerson:eIGS_ThirdPerson;

		Matrix34 finalMatrix(Matrix34(GetSlotHelperRotation(id, "item_attachment", true)));
		finalMatrix.SetTranslation(GetSlotHelperPos(id, "item_attachment", true));
		finalMatrix = finalMatrix * m_holdOffset;

		//This is need it for breakable/joint-constraints stuff
		Vec3 diff = finalMatrix.GetTranslation()-pEntity->GetWorldPos();
		diff.Normalize();
		if(pEntity->GetPhysics())
		{
			pe_action_set_velocity v;
			v.v = Vec3(0.01f,0.01f,0.01f);
			pEntity->GetPhysics()->Action(&v);
		}
		//====================================
		pEntity->SetWorldTM(finalMatrix);

	}
}

//===========================================================
void COffHand::UpdateGrabbedNPCState()
{
	//Get actor
	CActor  *pActor  = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_heldEntityId));

	if(pActor)
	{
		RunEffectOnGrabbedNPC(pActor);
		pActor->SetAnimationInput( "Action", "grabStruggleFP" );

		//Actor died while grabbed
		if((pActor->GetHealth()<=0 && !m_npcWasDead)||pActor->IsFallen())
		{
			if(m_mainHand && m_mainHand->IsBusy())
				m_mainHand->SetBusy(false);
			//Already throwing (do nothing)
			if(m_currentState&(eOHS_GRABBING_NPC|eOHS_HOLDING_NPC))
			{
				//Drop NPC
				m_currentState = eOHS_HOLDING_NPC;
				OnAction(GetOwnerId(),"use",eAAM_OnPress,0.0f);
				OnAction(GetOwnerId(),"use",eAAM_OnRelease,0.0f);
			}
			else if(m_currentState&eOHS_THROWING_NPC)
			{
				OnAction(GetOwnerId(),"use",eAAM_OnRelease,0.0f);
			}
			m_npcWasDead = true;

		}
	}
}

//============================================================
void COffHand::PostFilterView(struct SViewParams &viewParams)
{
	//This should be only be called when grabbing/holding/throwing an NPC from CPlayer::PostUpdateView()
	IEntity *pEntity = m_pEntitySystem->GetEntity(m_heldEntityId);

	UpdateGrabbedNPCState();

	if(pEntity)
	{
		SPlayerStats *stats = static_cast<SPlayerStats*>(GetOwnerActor()->GetActorStats());
		Quat wQuat(viewParams.rotation*Quat::CreateRotationXYZ(stats->FPWeaponAnglesOffset * gf_PI/180.0f));
		wQuat *= Quat::CreateSlerp(viewParams.currentShakeQuat,IDENTITY,0.5f);
		wQuat.Normalize();

		Matrix34 neckFinal = Matrix34::CreateIdentity();

		Vec3 itemAttachmentPos  = GetSlotHelperPos(0,"item_attachment",false);
		itemAttachmentPos = stats->FPWeaponPos + wQuat * itemAttachmentPos;

		neckFinal.SetRotation33(Matrix33(viewParams.rotation*Quat::CreateRotationZ(gf_PI)));
		neckFinal.SetTranslation(itemAttachmentPos);

		ICharacterInstance *pCharacter=pEntity->GetCharacter(0);
		assert(pCharacter && "COffHand::PostFilterView --> Actor entity has no character!!");
		if(!pCharacter)
			return;

		ISkeleton *pSkeleton=pCharacter->GetISkeleton();
		int neckId = 0;
		Vec3 specialOffset(0.0f,-0.07f,-0.09f);

		switch(m_grabbedNPCSpecies)
		{
			case eGCT_HUMAN:  neckId = pSkeleton->GetIDByName("Bip01 Neck");
												specialOffset.Set(0.0f,0.0f,0.0f);
												break;

			case eGCT_ALIEN:  neckId = pSkeleton->GetIDByName("Bip01 Neck");
												specialOffset.Set(0.0f,0.0f,-0.09f);
												break;

			case eGCT_TROOPER: neckId = pSkeleton->GetIDByName("Bip01 Head");
												break;
		}

		Vec3 neckLOffset(pSkeleton->GetAbsJQuatByID(neckId).t);
		Vec3 charOffset(pEntity->GetSlotLocalTM(0,false).GetTranslation());
		if(m_grabbedNPCSpecies==eGCT_TROOPER)
			charOffset.Set(0.0f,0.0f,0.0f);
		//Vec3 charOffset(0.0f,0.0f,0.0f);		//For some reason the above line didn't work with the trooper...

		//float white[4] = {1,1,1,1};
		//gEnv->pRenderer->Draw2dLabel( 100, 50, 2, white, false, "neck: %f %f %f", neckLOffset.x,neckLOffset.y,neckLOffset.z );
		//gEnv->pRenderer->Draw2dLabel( 100, 70, 2, white, false, "char: %f %f %f", charOffset.x,charOffset.y,charOffset.z );

		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(neckFinal.GetTranslation(),0.08f,ColorB(255,0,0));

		neckFinal.AddTranslation(Quat(neckFinal)*-(neckLOffset+charOffset+specialOffset));
		pEntity->SetWorldTM(neckFinal);

		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(neckFinal.GetTranslation(),0.08f,ColorB(0,255,0));
	}
}

//=============================================================
void COffHand::UpdateMainWeaponRaising()
{
	CActor* pActor = GetOwnerActor();
	if(pActor)
	{
		CItem *pMain=GetActorItem(pActor);
		CWeapon *pMainWeapon = static_cast<CWeapon *>(pMain?pMain->GetIWeapon():0);
		bool weaponRaised = false;

		if(pMainWeapon && pMainWeapon->CanBeRaised())
		{
			IMovementController* pMC = pActor->GetMovementController();
			SPlayerStats *stats = static_cast<SPlayerStats*>(pActor->GetActorStats());

			if(pMC && stats)
			{
				SMovementState info;
				pMC->GetMovementState(info);

				Vec3 pos = info.weaponPosition;
				Vec3 dir = info.aimDirection;


				ray_hit hit;

				//If it's dual wield we need some more tests
				if(pMainWeapon->IsDualWield())
				{
					//Cross product to get the "right" direction
					Vec3 rightDir = -info.upDirection.Cross(dir);
					rightDir.Normalize();
					pos = pos - rightDir*0.13f;

					//Raytrace for the left SOCOM
					gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir*pMainWeapon->GetRaiseDistance(), ent_static|ent_terrain,
						rwi_stop_at_pierceable|rwi_ignore_back_faces|14,&hit, 1, pActor->GetEntity()->GetPhysics());

					// SNH: only raise the weapon when ray hits a nearly-vertical surface
					//Left SOCOM up/down
					if(hit.pCollider && abs(hit.n.z) < 0.1f)
					{
						CWeapon *pSlave = static_cast<CWeapon*>(pMainWeapon->GetDualWieldSlave());
						if(pSlave)
							pSlave->RaiseWeapon(true);
						weaponRaised = true;
					}
					else
					{
						CWeapon *pSlave = static_cast<CWeapon*>(pMainWeapon->GetDualWieldSlave());
						if(pSlave)
							pSlave->RaiseWeapon(false);
					}

					//Raytrace for the right SOCOM
					pos = pos + rightDir*0.26f;
					gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir*pMainWeapon->GetRaiseDistance(), ent_static|ent_terrain,
						rwi_stop_at_pierceable|rwi_ignore_back_faces|14,&hit, 1, pActor->GetEntity()->GetPhysics());

					//Right SOCOM up/down
					if(hit.pCollider && abs(hit.n.z) < 0.1f)
					{
						pMainWeapon->RaiseWeapon(true);
						weaponRaised = true;
					}
					else
						pMainWeapon->RaiseWeapon(false);
					
				}
				else
				{
					//If it's not dualWield, just trace a ray using the position and aiming direction
					gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir*pMainWeapon->GetRaiseDistance(), ent_static|ent_terrain,
						rwi_stop_at_pierceable|rwi_ignore_back_faces|14,&hit, 1, pActor->GetEntity()->GetPhysics());

					if(hit.pCollider && abs(hit.n.z) < 0.1f)
					{
						pMainWeapon->RaiseWeapon(true);
						weaponRaised = true;
					}
					else
						pMainWeapon->RaiseWeapon(false);
				}

				//Lower weapon in front of friendly AI
				if(!weaponRaised && m_lastCHUpdate<=0.0f && !gEnv->bMultiplayer)
				{
					if(!pMainWeapon->IsModifying() && !pMainWeapon->IsBusy())
					{
						CWeapon *pSlave = NULL;
						if(pMainWeapon->IsDualWield())
							pSlave = static_cast<CWeapon*>(pMainWeapon->GetDualWieldSlave());
						
						stats->bLookingAtFriendlyAI = false;
						pMainWeapon->LowerWeapon(false);
						if(pSlave)
							pSlave->LowerWeapon(false);

						EntityId entityId = pActor->GetGameObject()->GetWorldQuery()->GetLookAtEntityId();
						IEntity* pLookAtEntity = m_pEntitySystem->GetEntity(entityId);
						CActor *pActorAI = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId));
						
						//Only for actors (not vehicles)
						if(pActorAI && pLookAtEntity && pLookAtEntity->GetAI() && pActor->GetEntity())
						{
							if(!pLookAtEntity->GetAI()->IsHostile(pActor->GetEntity()->GetAI(),false))
							{
								pMainWeapon->LowerWeapon(true);
								pMainWeapon->StopFire(pActor->GetEntityId());//Just in case
								if(pSlave)
								{
									pSlave->LowerWeapon(true);
									pSlave->StopFire(pActor->GetEntityId());
								}
								Vec3 dis = pLookAtEntity->GetWorldPos()-pActor->GetEntity()->GetWorldPos();
								if(dis.len2()<4.0f)
									stats->bLookingAtFriendlyAI = true;
							}
						}

					}
				}
				//============================================
			}
		}
	}
}

//=============================================================
void COffHand::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	EOffHandActions eOHA = eOHA_NO_ACTION;
	const SGameActions& actions = g_pGame->Actions();

	if(actionId == actions.use)
	{
		eOHA = eOHA_USE;
	}
	else if(actionId == actions.xi_use)
	{
		eOHA = eOHA_XI_USE;
	}
	else if(actionId == actions.grenade)
	{
		eOHA = eOHA_THROW_GRENADE;
	}
	else if(actionId == actions.handgrenade)
	{
		eOHA = eOHA_SWITCH_GRENADE;
	}
	else if(actionId == actions.special)
	{
		eOHA = eOHA_MELEE_ATTACK;
	}

	//Return if there was no OffHand action requested
	if(eOHA == eOHA_NO_ACTION)
		return;

	//Test if action is possible
	if(!EvalutateStateTransition(eOHA,activationMode))
		return;

	if(!PreExecuteAction(eOHA,activationMode))
		return;

	if(eOHA==eOHA_SWITCH_GRENADE)
	{
		StartSwitchGrenade();
		if(m_mainHandWeapon && m_mainHandWeapon->IsWeaponRaised())
			m_mainHandWeapon->RaiseWeapon(false,true);
	}
	else if(eOHA==eOHA_THROW_GRENADE)
	{
		if (!m_fm->OutOfAmmo() && GetFireModeIdx(m_fm->GetName())<MAX_GRENADE_TYPES)
		{
			PerformThrow(activationMode,0,GetCurrentFireMode());
			if(m_mainHandWeapon && m_mainHandWeapon->IsWeaponRaised())
				m_mainHandWeapon->RaiseWeapon(false,true);
		}
	}
	else if(eOHA==eOHA_MELEE_ATTACK)
	{
		MeleeAttack();
	}
	else if(eOHA==eOHA_USE || eOHA==eOHA_XI_USE)
	{
		if(m_currentState&eOHS_INIT_STATE)
		{
			int typ = CanPerformPickUp(GetOwnerActor(),NULL,true);
			if(typ==OH_GRAB_ITEM)
			{
				StartPickUpItem();
			}
			else if(typ==OH_GRAB_OBJECT)
			{
				PickUpObject();
			}
			else if(typ==OH_GRAB_NPC)
			{
				PickUpObject(true);
			}
			else if(typ==OH_NO_GRAB)
			{
				CancelAction();
			}

		}
		else if(m_currentState&(eOHS_HOLDING_OBJECT|eOHS_THROWING_OBJECT))
		{
			ThrowObject(activationMode);
		}
		else if(m_currentState&(eOHS_HOLDING_NPC|eOHS_THROWING_NPC))
		{
			ThrowObject(activationMode,true);
		}
	}
	
}

//==================================================================
bool COffHand::EvalutateStateTransition(int requestedAction, int activationMode)
{
	switch(requestedAction)
	{
		case eOHA_SWITCH_GRENADE: if(activationMode==eAAM_OnPress && m_currentState==eOHS_INIT_STATE)
															{
																return true;
															}
															break;

		case eOHA_THROW_GRENADE:	if(activationMode==eAAM_OnPress && m_currentState==eOHS_INIT_STATE)
															{
																//Don't throw if there's no ammo (or not fm)
																if(m_fm && !m_fm->OutOfAmmo() && m_nextGrenadeThrow<=0.0f)
																	return true;
															}
															else if(activationMode==eAAM_OnRelease && m_currentState==eOHS_HOLDING_GRENADE)
															{
																return true;
															}
															break;
		case eOHA_USE:						
		case eOHA_XI_USE:					if(activationMode==eAAM_OnPress && (m_currentState&(eOHS_INIT_STATE|eOHS_HOLDING_OBJECT|eOHS_HOLDING_NPC)))
															{
																return true;
															}
															else if(activationMode==eAAM_OnRelease && (m_currentState&(eOHS_THROWING_OBJECT|eOHS_THROWING_NPC)))
															{
																return true;
															}
		case eOHA_MELEE_ATTACK:		if(activationMode==eAAM_OnPress && m_currentState==eOHS_HOLDING_OBJECT && m_grabType==0)
															{
																return true;
															}
	}

	return false;
}

//==================================================================
bool COffHand::PreExecuteAction(int requestedAction, int activationMode)
{
	if(m_currentState!=eOHS_INIT_STATE)
		return true;

	//We have to test the main weapon/item state, in some cases the offhand could not perform the action
	CActor *pActor = GetOwnerActor();
	if(!pActor || pActor->GetHealth()<=0 || pActor->IsSwimming())
		return false;

	CItem			*pMain = GetActorItem(pActor);
	CWeapon		*pMainWeapon = static_cast<CWeapon *>(pMain?pMain->GetIWeapon():NULL);

	bool exec = true;

	if(pMain)
		exec &= !pMain->IsBusy();


	if(pMainWeapon)
		exec &= !pMainWeapon->IsZoomed() && !pMainWeapon->IsZooming() && !pMainWeapon->IsModifying();

	if(exec)
	{
		SetHand(eIH_Left);		//Here??

		if (GetEntity()->IsHidden() && activationMode==eAAM_OnPress)
		{
			m_stats.fp=!m_stats.fp;	

			GetScheduler()->Lock(true);
			Select(true);
			GetScheduler()->Lock(false);
			SetBusy(false);
		}

		m_mainHand = pMain;
		m_mainHandWeapon = pMainWeapon;

		if(requestedAction==eOHA_THROW_GRENADE)
		{
			if(m_mainHand && (m_mainHand->TwoHandMode()==1 || m_mainHand->IsDualWield()))
			{
				GetOwnerActor()->HolsterItem(true);
				m_mainHand = m_mainHandWeapon = NULL;
			}
		}
			
	}

	return exec;
}

//=============================================================================
//This function seems redundant...
void COffHand::CancelAction()
{
	SetOffHandState(eOHS_INIT_STATE);
}

//=============================================================================
void COffHand::FinishAction(EOffHandActions eOHA)
{
	switch(eOHA)
	{
		case eOHA_SWITCH_GRENADE: EndSwitchGrenade();
															break;

		case eOHA_PICK_ITEM:			EndPickUpItem();
															break;

		case eOHA_GRAB_NPC:				m_currentState = eOHS_HOLDING_NPC;
															break;

		case eOHA_THROW_NPC:			GetScheduler()->TimerAction(300, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_RESET,this)), true);
															ThrowNPC();
															m_currentState = eOHS_TRANSITIONING;
															break;
		
		case eOHA_PICK_OBJECT:		m_currentState = eOHS_HOLDING_OBJECT;
															break;

		case eOHA_THROW_OBJECT:		{
															// after it's thrown, wait 300ms to enable collisions again
															GetScheduler()->TimerAction(300, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_RESET,this)), true);
															DrawNear(false);

															IEntity *pEntity=m_pEntitySystem->GetEntity(m_heldEntityId);
															if (pEntity)
															{
																SEntityEvent entityEvent;
																entityEvent.event = ENTITY_EVENT_PICKUP;
																entityEvent.nParam[0] = 0;
																pEntity->SendEvent( entityEvent );
															}												
															m_currentState = eOHS_TRANSITIONING;
															}
															break;
		
		case eOHA_RESET:					//Reset main weapon status and reset offhand	
															SetCurrentFireMode(m_lastFireModeId);
															{
																float timeDelay = 0.1f; 
																if(!m_mainHand)
																{
																	GetOwnerActor()->HolsterItem(false);
																}
																else if(!m_mainHand->IsDualWield())
																{
																	m_mainHand->ResetDualWield();
																	m_mainHand->PlayAction(g_pItemStrings->offhand_off, 0, false, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
																	timeDelay = (m_mainHand->GetCurrentAnimationTime(CItem::eIGS_FirstPerson)+50)*0.001f;
																}
																SetResetTimer(timeDelay);
																RequireUpdate(eIUS_General);
															}

															//turn off collision with thrown objects
															if(m_heldEntityId)
																IgnoreCollisions(false,m_heldEntityId);

															break;

		case	eOHA_FINISH_MELEE:	if(m_heldEntityId)
															{
																m_currentState = eOHS_HOLDING_OBJECT;
															}
															break;

		case eOHA_FINISH_AI_THROW_GRENADE:
															{
																// Reset the main weapon after a grenade throw.
																CActor *pActor = GetOwnerActor();
																if (pActor)
																{
																	CItem			*pMain = GetActorItem(pActor);
																	if (pMain)
																		pMain->PlayAction(g_pItemStrings->idle, 0, false, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
																}
															}
															break;
	}
}

//==============================================================================
void COffHand::SetOffHandState(EOffHandStates eOHS)
{
	m_currentState = eOHS;

	if(eOHS == eOHS_INIT_STATE)
	{
		m_mainHand = m_mainHandWeapon = NULL;
		m_heldEntityId = m_preHeldEntityId = 0;
		Select(false);
	}
}

//==============================================================================
void COffHand::StartSwitchGrenade()
{
	//Iterate different firemodes 
	int firstMode = GetCurrentFireMode();
	int newMode = GetNextFireMode(GetCurrentFireMode());

	while (newMode != firstMode)
	{
		//Fire mode idx>2 means its a throw object/npc firemode
		if (GetFireMode(newMode)->OutOfAmmo() || newMode>=MAX_GRENADE_TYPES )
			newMode = GetNextFireMode(newMode);
		else
		{
			m_lastFireModeId = newMode;
			break;
		}
	}

	//We didn't find a fire mode with ammo
	if (newMode == firstMode)
	{
		CancelAction();
		return;
	}
	else
		RequestFireMode(newMode);

	//No animation in multiplayer
	if(gEnv->bMultiplayer)
	{
		CancelAction();
		return;
	}

	//A new grenade type/fire mode was chosen
	if(m_mainHand && (m_mainHand->TwoHandMode()!=1) && !m_mainHand->IsDualWield())
	{
		//Play deselect left hand on main item
		m_mainHand->PlayAction(g_pItemStrings->offhand_on);
		m_mainHand->SetActionSuffix("akimbo_");

		m_mainHand->GetScheduler()->TimerAction(m_mainHand->GetCurrentAnimationTime(eIGS_FirstPerson),
			CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_SWITCH_GRENADE, this)), false);

	}
	else
	{
		//No main item or holstered (wait 100ms)
		GetOwnerActor()->HolsterItem(true);
		m_mainHand = m_mainHandWeapon = NULL;
		GetScheduler()->TimerAction(100, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_SWITCH_GRENADE,this)),false);
	}

	//Change offhand state
	m_currentState = eOHS_SWITCHING_GRENADE;
}

//==============================================================================
void COffHand::EndSwitchGrenade()
{
	//Play select grenade animation (and un-hide grenade geometry)
	PlayAction(g_pItemStrings->select_grenade);
	HideItem(false);
	GetScheduler()->TimerAction(GetCurrentAnimationTime(CItem::eIGS_FirstPerson),
		CSchedulerAction<FinishGrenadeAction>::Create(FinishGrenadeAction(this,m_mainHand)), false);
}

//==============================================================================
void COffHand::PerformThrow(EntityId shooterId, float speedScale)
{
	if (!m_fm)
		return;

	m_fm->StartFire(shooterId);

	CThrow *pThrow = static_cast<CThrow *>(m_fm);
	pThrow->SetSpeedScale(speedScale);

	m_fm->StopFire(shooterId);
	pThrow->ThrowingGrenade(true);

	// Schedule to revert back to main weapon.
	GetScheduler()->TimerAction(2000,
		CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_FINISH_AI_THROW_GRENADE, this)), false);
}

//===============================================================================
void COffHand::PerformThrow(int activationMode, EntityId throwableId, int oldFMId /* = 0 */, bool isLivingEnt /*=false*/)
{
	if (!m_fm)
		return;

	if(activationMode==eAAM_OnPress)
		m_currentState = eOHS_HOLDING_GRENADE;

	//Throw objects...
	if (throwableId)
	{
		if(!isLivingEnt)
		{
			m_currentState = eOHS_THROWING_OBJECT;
			CThrow *pThrow = static_cast<CThrow *>(m_fm);
			pThrow->SetThrowable(throwableId, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_THROW_OBJECT,this)));
		}
		else
		{
			m_currentState = eOHS_THROWING_NPC;
			CThrow *pThrow = static_cast<CThrow *>(m_fm);
			pThrow->SetThrowable(throwableId, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_THROW_NPC,this)));

		}
	}
	//--------------------------

	if (activationMode==eAAM_OnPress)
	{
		if (!m_fm->IsFiring() && m_nextGrenadeThrow < 0.0f)
		{
			m_fm->StartFire(GetOwnerId());
			if (m_mainHand && m_fm->IsFiring())
			{
				if(!(m_currentState&(eOHS_THROWING_NPC|eOHS_THROWING_OBJECT)))
				{
					m_mainHand->PlayAction(g_pItemStrings->offhand_on);
					m_mainHand->SetActionSuffix("akimbo_");
				}
			}
		}

	}
	else if (activationMode==eAAM_OnRelease && m_nextGrenadeThrow <= 0.0f)
	{
		CThrow *pThrow = static_cast<CThrow *>(m_fm);
		if(m_currentState!=eOHS_HOLDING_GRENADE)
		{
			pThrow->ThrowingGrenade(false);
		}
		else
			m_currentState = eOHS_THROWING_GRENADE;

		m_nextGrenadeThrow = 60.0f/m_fm->GetFireRate();
		m_fm->StopFire(GetOwnerId());
		pThrow->ThrowingGrenade(true);

		if (m_fm->IsFiring())
		{
			GetScheduler()->ScheduleAction(CSchedulerAction<FinishGrenadeAction>::Create(FinishGrenadeAction(this,m_mainHand)),false);
		}
	}
}

//=====================================================================================
int COffHand::CanPerformPickUp(CActor *pActor, IPhysicalEntity *pPhysicalEntity /*=NULL*/, bool getEntityInfo /*= false*/)
{
	if (!pActor || !pActor->IsClient() || pActor->IsSwimming() || gEnv->bMultiplayer)
		return OH_NO_GRAB;

	if (!pPhysicalEntity)
	{
		const ray_hit *pRay = pActor->GetGameObject()->GetWorldQuery()->GetLookAtPoint(m_range);
		if (pRay)
			pPhysicalEntity = pRay->pCollider;
		else
			return OH_NO_GRAB;
	}

	IEntity *pEntity = m_pEntitySystem->GetEntityFromPhysics(pPhysicalEntity);

	if(!pEntity)
		return OH_NO_GRAB;

	// check if we have to pickup with two hands or just on hand
	SelectGrabType(pEntity);

	IMovementController * pMC = pActor?pActor->GetMovementController() : 0;
	if (pMC)
	{
		SMovementState info;
		pMC->GetMovementState(info);
		Vec3 pos = info.eyePosition;
		float lenSqr = 0.0f;
		bool  breakable = false;
		pe_params_part pPart;

		//Check if entity is in range
		if (pEntity)
		{
			lenSqr=(pos-pEntity->GetWorldPos()).len2();
			if(pPhysicalEntity->GetType()==PE_RIGID && !strcmp(pEntity->GetClass()->GetName(), "Default"))
			{
				//Procedurally breakable object (most likely...)
				//I need to adjust the distance, since the pivot of the entity could be anywhere
				pPart.ipart = 0;
				if(pPhysicalEntity->GetParams(&pPart) && pPart.pPhysGeom)
				{
					lenSqr-=pPart.pPhysGeom->origin.len2();
					breakable = true;
				}
			}

		}

		if (lenSqr<m_range*m_range)
		{

			{
				if(getEntityInfo)
					m_preHeldEntityId = pEntity->GetId();

				//1.- Player can grab some NPCs
				//Let the actor decide if it can be grabbed
				if(CActor *pActorAI = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId())))
				{
					if(pEntity->GetAI() && pActor->GetEntity() && !pActor->GetLinkedVehicle()) 
					{
						//Check script table (maybe is not possible to grab)
						SmartScriptTable props;
						pEntity->GetScriptTable()->GetValue("Properties", props);
						SmartScriptTable propsDamage;
						props->GetValue("Damage", propsDamage);

						int noGrab = 0;
						if(propsDamage->GetValue("bNoGrab", noGrab) && noGrab!=0)
							return OH_NO_GRAB;

						if(pActorAI->GetActorSpecies()!=eGCT_UNKNOWN && pActorAI->GetHealth()>0 && !pActorAI->IsFallen() && pEntity->GetAI()->IsHostile(pActor->GetEntity()->GetAI(),false))
							return OH_GRAB_NPC;
						else
							return OH_NO_GRAB;
					}
					return OH_NO_GRAB;
				}

				//2. -if it's an item, let the item decide if it can be picked up or not
				if (CItem *pItem=static_cast<CItem *>(m_pItemSystem->GetItem(pEntity->GetId())))
				{
					if(pItem->CanPickUp(pActor->GetEntityId()))
						return OH_GRAB_ITEM;
					else 
						return OH_NO_GRAB;
				}

				//3. -If we found a helper, it has to be pickable
				if(m_hasHelper && pPhysicalEntity->GetType()==PE_RIGID)
				{
					SmartScriptTable props;
					IScriptTable *pEntityScript=pEntity->GetScriptTable();
					if (pEntityScript && pEntityScript->GetValue("Properties", props))
					{
						//If it's not pickable, ignore helper
						int pickable=0;
						if (props->GetValue("bPickable", pickable) && !pickable)
							return false;
					}
					return OH_GRAB_OBJECT;
				}

				// Pick boid object.
				if (pPhysicalEntity->GetType()==PE_PARTICLE && strcmp(pEntity->GetClass()->GetName(), "Boid")==0)
				{
					//m_holdOffset = Matrix34::CreateRotationXYZ( Ang3(0.5f,0.2f,0.6f),Vec3(-0.2f,0.5f,0.1f) );
					m_holdOffset = Matrix34::CreateRotationXYZ( Ang3(0.0f,0,0.0f),Vec3(0.0f,0.0f,-0.2f) );
					m_grabType = 0;
					return OH_GRAB_OBJECT;
				}

				//4. -Procedurally breakable object (most likely...)
				if(breakable)
				{
					//Set "hold" matrix
					if(pPart.pPhysGeom->V<0.35f && pPart.pPhysGeom->Ibody.len()<0.1)
					{
						m_holdOffset.SetTranslation(pPart.pPhysGeom->origin+Vec3(0.0f,-0.15f,0.0f));
						m_holdOffset.InvertFast();
						return OH_GRAB_OBJECT;
					}

				}
				
				//5.- here's some code for a video game (legacy code)
				SmartScriptTable props;
				IScriptTable *pEntityScript=pEntity->GetScriptTable();
				if (pPhysicalEntity->GetType()==PE_RIGID && pEntityScript && pEntityScript->GetValue("Properties", props))
				{
					int pickable=0;
					int usable = 0;
					if (props->GetValue("bPickable", pickable) && !pickable)
						return false;
					else if (pickable)
						if(props->GetValue("bUsable", usable) && !usable)
							return OH_GRAB_OBJECT;

					return false;
				}

				if(getEntityInfo)
					m_preHeldEntityId = 0;

				return OH_NO_GRAB;
			}
		}
	}
	return OH_NO_GRAB;
}

//==========================================================================================
bool COffHand::PerformPickUp()
{
	//If we are here, we must have the entity ID
	m_heldEntityId = m_preHeldEntityId;
	m_preHeldEntityId =  0;
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_heldEntityId);

	if(pEntity)
	{
		// Send event to entity.
		SEntityEvent entityEvent;
		entityEvent.event = ENTITY_EVENT_PICKUP;
		entityEvent.nParam[0] = 1;
		pEntity->SendEvent( entityEvent );

		IgnoreCollisions(true,m_heldEntityId);
		DrawNear(true);

		CActor* pActor = GetOwnerActor();
		if(pActor && pActor->IsPlayer())
		{
			m_heldEntityMass = 1.0f;
			if(IPhysicalEntity* pPhy = pEntity->GetPhysics())
			{
				pe_status_dynamics dynStat;
				if (pPhy->GetStatus(&dynStat))
					m_heldEntityMass = dynStat.mass;
			}
		}

		SetDefaultIdleAnimation(eIGS_FirstPerson, m_grabTypes[m_grabType].idle);

		return true;
	}
	return false;
}

//===========================================================================================
void COffHand::IgnoreCollisions(bool ignore, EntityId entityId /*=0*/)
{
	if (!m_heldEntityId && !entityId)
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(m_heldEntityId?m_heldEntityId:entityId);
	if (!pEntity)
		return;

	IPhysicalEntity *pPE=pEntity->GetPhysics();
	if (!pPE)
		return;

	if (ignore)
	{
		//The constraint doesn't work with Items physicalized as static
		if(pPE->GetType()==PE_STATIC)
		{
			IItem *pItem = m_pItemSystem->GetItem(pEntity->GetId());
			if(pItem)
			{
				pItem->Physicalize(true,true);
				pPE = pEntity->GetPhysics();
				assert(pPE);
			}
		}

		CActor *pActor = GetOwnerActor();
		if (!pActor)
			return;

		pe_action_add_constraint ic;
		ic.flags=constraint_inactive|constraint_ignore_buddy;
		ic.pBuddy=pActor->GetEntity()->GetPhysics();
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

//==========================================================================================
void COffHand::DrawNear(bool drawNear, EntityId entityId /*=0*/)
{
	if (!m_heldEntityId && !entityId)
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(m_heldEntityId?m_heldEntityId:entityId);
	if (!pEntity)
		return;

	int nslots=pEntity->GetSlotCount();
	for (int i=0;i<nslots;i++)
	{
		if (pEntity->GetSlotFlags(i)&ENTITY_SLOT_RENDER)
		{
			if (drawNear)
				pEntity->SetSlotFlags(i, pEntity->GetSlotFlags(i)|ENTITY_SLOT_RENDER_NEAREST);
			else
				pEntity->SetSlotFlags(i, pEntity->GetSlotFlags(i)&(~ENTITY_SLOT_RENDER_NEAREST));
		}
	}
}

//=========================================================================================
void COffHand::SelectGrabType(IEntity* pEntity)
{
	CActor *pActor=GetOwnerActor();

	assert(pActor && "COffHand::SelectGrabType: No OwnerActor, probably something bad happened");
	if (!pActor)
		return;

	if (pEntity)
	{
		// iterate over the grab types and see if this object supports one
		m_grabType = 0;
		const TGrabTypes::const_iterator end = m_grabTypes.end();
		for (TGrabTypes::const_iterator i = m_grabTypes.begin(); i != end; ++i, ++m_grabType)
		{
			SEntitySlotInfo slotInfo;
			if (pEntity->GetSlotInfo(0, slotInfo) && slotInfo.pStatObj)
			{
				//It is already a subobject, we have to search in the parent
				if(slotInfo.pStatObj->GetParentObject())
				{
					IStatObj::SSubObject* pSubObj = slotInfo.pStatObj->GetParentObject()->FindSubObject((*i).helper.c_str());
					if (pSubObj)
					{
						m_holdOffset = pSubObj->tm;
						m_holdOffset.OrthonormalizeFast();
						m_holdOffset.InvertFast();
						m_hasHelper = true;
						return;						
					}
				}
				else
				{
					IStatObj::SSubObject* pSubObj = slotInfo.pStatObj->FindSubObject((*i).helper.c_str());
					if (pSubObj)
					{
						m_holdOffset = pSubObj->tm;
						m_holdOffset.OrthonormalizeFast();
						m_holdOffset.InvertFast();
						m_hasHelper = true;
						return;						
					}
				}
			}
		}

		// when we come here, we haven't matched any of the predefined helpers ... so try to make a 
		// smart decision based on how large the object is
		//float volume(0),heavyness(0);
		//pActor->CanPickUpObject(pEntity, heavyness, volume);

		// grabtype 0 is onehanded and 1 is twohanded
		//m_grabType = (volume>0.08f) ? 1 : 0;
		m_holdOffset.SetIdentity();
		m_hasHelper = false;
		m_grabType = 1;
	}
}

//========================================================================================================
void COffHand::StartPickUpItem()
{	
	CPlayer *pPlayer = static_cast<CPlayer*>(GetOwnerActor());

	bool drop_success=false;

	if(!pPlayer)
	{
		assert(pPlayer && "COffHand::StartPickUpItem -> No player found!!");
		CancelAction();
		return;
	}
	else if(gEnv->bMultiplayer)
	{
		CancelAction();		//Temp work-around for multiplayer (don't pick up items with offhand)
		return;
	}
	else if(!pPlayer->CheckInventoryRestrictions(m_pEntitySystem->GetEntity(m_preHeldEntityId)->GetClass()->GetName()))
	{
		//Can not carry more heavy/medium weapons
		//Drop existing weapon and pick up the other
		IItem* pItem = pPlayer->GetCurrentItem();
		if(pItem)
		{
			const char *itemCategory = m_pItemSystem->GetItemCategory(pItem->GetEntity()->GetClass()->GetName());
			if (!strcmp(itemCategory,"medium") || !strcmp(itemCategory,"heavy"))
			{
				//can replace medium or heavy weapon
				if(pPlayer->DropItem(pItem->GetEntityId(), 30.f, true))
					drop_success=true;
			}
		}
		if(!drop_success)
		{
			g_pGame->GetGameRules()->OnTextMessage(eTextMessageError, "@mp_CannotCarryMore");
			CancelAction();
			return;
		}
	}
	else if(IItem* pItem =m_pItemSystem->GetItem(m_preHeldEntityId))
	{
		if(!pItem->CheckAmmoRestrictions(pPlayer->GetEntityId()))
		{
			if(g_pGame->GetHUD())
				g_pGame->GetHUD()->DisplayFlashMessage("@ammo_maxed_out", 2, ColorF(1.0f, 0,0), true, pItem->GetEntity()->GetClass()->GetName());
			CancelAction();
			return;
		}
	}



	//Everything seems ok, start the action...
	m_currentState = eOHS_PICKING_ITEM;
	m_pickingTimer = 0.3f;
	pPlayer->NeedToCrouch();

	if (m_mainHand && (m_mainHand->TwoHandMode() >= 1 || m_mainHand->IsDualWield() || drop_success))
	{
		GetOwnerActor()->HolsterItem(true);
		m_mainHand = m_mainHandWeapon = NULL;
	}
	else
	{
		if (m_mainHand)
		{
			m_mainHand->PlayAction(g_pItemStrings->offhand_on);
			m_mainHand->SetActionSuffix("akimbo_");
		}
	}

	PlayAction(g_pItemStrings->pickup_weapon_left,0,false,eIPAF_Default|eIPAF_RepeatLastFrame);
	GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson)+100, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_PICK_ITEM,this)), false);
	RequireUpdate(eIUS_General);
}

//=========================================================================================================
void COffHand::EndPickUpItem()
{
	//Restore main weapon
	if (m_mainHand)
	{
		m_mainHand->ResetDualWield();
		m_mainHand->PlayAction(g_pItemStrings->offhand_off, 0, false, eIPAF_Default|eIPAF_NoBlend);
	}
	else
	{
		GetOwnerActor()->HolsterItem(false);
	}

	//Pick-up the item, and reset offhand
	m_currentState = eOHS_PICKING_ITEM2;

	GetOwnerActor()->PickUpItem(m_heldEntityId, true);
	IgnoreCollisions(false,m_heldEntityId);

	SetOffHandState(eOHS_INIT_STATE);
}

//=======================================================================================
void COffHand::PickUpObject(bool isLivingEnt /* = false */)
{	
	//Grab NPCs-----------------
	if(isLivingEnt)
		if(!GrabNPC())
			CancelAction();
	//-----------------------

	//Don't pick up in prone
	CPlayer *pPlayer = static_cast<CPlayer*>(GetOwnerActor());
	if(pPlayer && pPlayer->GetStance()==STANCE_PRONE)
	{
		CancelAction();
		return;
	}

	if (m_mainHand && (m_grabTypes[m_grabType].twoHanded || m_mainHand->TwoHandMode() >= 1 || m_mainHand->IsDualWield()))
	{
		GetOwnerActor()->HolsterItem(true);
		m_mainHand = m_mainHandWeapon = NULL;
	}
	else
	{
		if (m_mainHand)
		{
			m_mainHand->PlayAction(g_pItemStrings->offhand_on);
			m_mainHand->SetActionSuffix("akimbo_");

			if(m_mainHandWeapon)
			{
				IFireMode *pFireMode = m_mainHandWeapon->GetFireMode(m_mainHandWeapon->GetCurrentFireMode());
				if(pFireMode)
				{
					pFireMode->SetRecoilMultiplier(1.5f);		//Increase recoil for the weapon
				}
			}
		}
	}
	if(!isLivingEnt)
	{
		m_currentState	= eOHS_PICKING;
		m_pickingTimer	= 0.3f;
		SetDefaultIdleAnimation(CItem::eIGS_FirstPerson,m_grabTypes[m_grabType].idle);
		PlayAction(m_grabTypes[m_grabType].pickup);
		GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson)+100, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_PICK_OBJECT,this)), false);
	}
	else
	{
		m_currentState	= eOHS_GRABBING_NPC;
		m_grabType			= 2;
		SetDefaultIdleAnimation(CItem::eIGS_FirstPerson,m_grabTypes[m_grabType].idle);
		PlayAction(m_grabTypes[m_grabType].pickup);
		GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson)+100, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_GRAB_NPC,this)), false);
	}
	RequireUpdate(eIUS_General);
}

//=========================================================================================
void COffHand::ThrowObject(int activationMode, bool isLivingEnt /*= false*/)
{
	if (activationMode == eAAM_OnPress)
		m_lastFireModeId = GetCurrentFireMode();

	if (m_heldEntityId)
		SetCurrentFireMode(GetFireModeIdx(m_grabTypes[m_grabType].throwFM));

	PerformThrow(activationMode, m_heldEntityId, m_lastFireModeId, isLivingEnt);

	if(m_mainHandWeapon)
	{
		IFireMode *pFireMode = m_mainHandWeapon->GetFireMode(m_mainHandWeapon->GetCurrentFireMode());
		if(pFireMode)
		{
			pFireMode->SetRecoilMultiplier(1.0f);		//Restore normal recoil for the weapon
		}
	}

}

//==========================================================================================
bool COffHand::GrabNPC()
{

	CActor  *pPlayer = GetOwnerActor();

	assert(pPlayer && "COffHand::GrabNPC --> Offhand has no owner actor (player)!");
	if(!pPlayer)
		return false;

	//Do not grab in prone
	if(pPlayer->GetStance()==STANCE_PRONE)
		return false;
	
	//Get actor
	CActor  *pActor  = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_preHeldEntityId));
	
	assert(pActor && "COffHand::GrabNPC -> No actor found!");
	if(!pActor)
		return false;

	IEntity *pEntity = pActor->GetEntity();
	assert(pEntity && "COffHand::GrabNPC -> Actor has no Entity");
	if(!pEntity)
		return false;

	//The NPC holster his weapon
	bool mounted = false;
	CItem *currentItem = static_cast<CItem*>(pActor->GetCurrentItem());
	if(currentItem)
	{
		if(currentItem->IsMounted() && currentItem->IsUsed())
		{
			currentItem->StopUse(pActor->GetEntityId());
			mounted = true;
		}
	}

	if(!mounted)
	{
		pActor->HolsterItem(false); //AI sometimes has holstered a weapon and selected a different one...
		pActor->HolsterItem(true);
	}


	if(IAISystem *pAISystem=gEnv->pAISystem)
	{
		IAISignalExtraData *pEData = pAISystem->CreateSignalExtraData();	

		pEData->point = Vec3(0,0,0);
		IAIActor* pAIActor = CastToIAIActorSafe(pEntity->GetAI());
		if(pAIActor)
			pAIActor->SetSignal(1,"TRANQUILIZED",0,pEData);
	}

	pActor->SetAnimationInput( "Action", "grabStruggleFP" );

	//Modify Enemy Render Flags
	pEntity->GetCharacter(0)->SetFlags(pEntity->GetCharacter(0)->GetFlags()|ENTITY_SLOT_RENDER_NEAREST);

	//Disable IK
	SPlayerStats *stats = static_cast<SPlayerStats*>(pActor->GetActorStats());
	stats->isGrabbed = true;

	m_heldEntityId = m_preHeldEntityId;
	m_preHeldEntityId = 0;
	m_grabbedNPCSpecies = pActor->GetActorSpecies();
	m_grabType = 2;
	m_killTimeOut = KILL_NPC_TIMEOUT;
	m_killNPC = m_effectRunning = m_npcWasDead =false;
	m_grabbedNPCInitialHealth = pActor->GetHealth();

	RequireUpdate(eIUS_General);

	return true;
}

//=============================================================================================
void COffHand::ThrowNPC(bool kill /*= true*/)
{
	//Get actor
	CActor  *pActor  = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_heldEntityId));

	assert(pActor && "COffHand::Throw -> No actor found!");
	if(!pActor)
		return;

	IEntity *pEntity = pActor->GetEntity();
	assert(pEntity && "COffHand::Throw -> Actor has no Entity");
	if(!pEntity)
		return;

	SPlayerStats *stats = static_cast<SPlayerStats*>(pActor->GetActorStats());
	stats->isGrabbed = false;

	if(kill)
	{
		pActor->HolsterItem(false);
		IItem *currentItem = pActor->GetCurrentItem();

		//For the tutorial enemies...
		SmartScriptTable props;
		pEntity->GetScriptTable()->GetValue("Properties", props);
		SmartScriptTable propsDamage;
		props->GetValue("Damage", propsDamage);

		int noDeath = 0;
		if(propsDamage->GetValue("bNoDeath", noDeath) && noDeath!=0)
		{
			if(pEntity->GetAI())
			{
				//pEntity->GetAI()->Event(AIEVENT_ENABLE,0);
				pActor->SetAnimationInput( "Action", "idle" );
				pActor->Fall();
				PlaySound(eOHSound_Kill_Human, true);
			}
		}
		else
		{
			int prevHealth = pActor->GetHealth();
			int health = prevHealth-100;
			if(health<=0 || (m_grabbedNPCSpecies!=eGCT_HUMAN))
			{
				if(currentItem)
					pActor->DropItem(currentItem->GetEntityId(),0.5f,false, true);
				pActor->SetHealth(0);
				//Don't kill if it was already dead
				if(prevHealth>0)
					pActor->CreateScriptEvent("kill",0);
			}
			else
			{
				if(pEntity->GetAI())
				{
					pActor->SetHealth(health);
					pActor->SetAnimationInput( "Action", "idle" );
					//pEntity->GetAI()->Event(AIEVENT_ENABLE,0);
					pActor->Fall();
					PlaySound(eOHSound_Kill_Human, true);
				}
			}
		}
	}
	else
	{
		pActor->SetAnimationInput( "Action", "idle" );
	}

	if(m_grabbedNPCSpecies==eGCT_TROOPER)
	{
		PlaySound(eOHSound_Choking_Trooper,false);
	}
	else if(m_grabbedNPCSpecies==eGCT_HUMAN)
	{
		PlaySound(eOHSound_Choking_Human, false);
	}

	m_killTimeOut = -1.0f;
	m_killNPC = m_effectRunning = m_npcWasDead = false;
	m_grabbedNPCInitialHealth = 0;

	//Restore Enemy RenderFlags
	pEntity->GetCharacter(0)->SetFlags(pEntity->GetCharacter(0)->GetFlags()&(~ENTITY_SLOT_RENDER_NEAREST));
}

//==============================================================================
void COffHand::RunEffectOnGrabbedNPC(CActor* pNPC)
{

	//Under certain conditions, different things could happen to the grabbed NPC (die, auto-destruct...)
	if(m_grabbedNPCSpecies==eGCT_HUMAN)
	{
		PlaySound(eOHSound_Choking_Human,true);
		//if(m_killNPC)
		//pNPC->SetHealth(0);	//Kill him automatically (this will switch automatic drop/throw)
	}
	else if(m_grabbedNPCSpecies==eGCT_TROOPER)
	{
		if(m_killNPC && m_effectRunning)
		{
			pNPC->SetHealth(0);
			PlaySound(eOHSound_Choking_Trooper,false);
			m_killNPC = false;
		}
		else if(m_killNPC || (pNPC->GetHealth()<m_grabbedNPCInitialHealth && !m_effectRunning))
		{
			SGameObjectEvent event(eCGE_InitiateAutoDestruction, eGOEF_ToScriptSystem);
			pNPC->GetGameObject()->SendEvent(event);
			m_effectRunning = true;
			m_killTimeOut = KILL_NPC_TIMEOUT-0.75f;
			m_killNPC = false;
		}
		else
		{
			PlaySound(eOHSound_Choking_Trooper,true);
		}
	}

}

//========================================================================================
void COffHand::PlaySound(EOffHandSounds sound, bool play)
{
	if(!gEnv->pSoundSystem)
		return;

	bool repeating = false;
	uint idx = 0;
	const char* soundName = NULL;

	switch(sound)
	{
		case eOHSound_Choking_Trooper:
			soundName = "Sounds/alien:trooper:choke";
			repeating = true;
			break;

		case eOHSound_Choking_Human:
			idx = Random(MAX_CHOKE_SOUNDS);
			idx = CLAMP(idx,0,MAX_CHOKE_SOUNDS-1);
			if(idx>=0 && idx<MAX_CHOKE_SOUNDS)
				soundName = gChokeSoundsTable[idx];
			repeating = true;
			break;

		case eOHSound_Kill_Human:
			idx = Random(MAX_CHOKE_SOUNDS);
			idx = CLAMP(idx,0,MAX_CHOKE_SOUNDS-1);
			if(idx>=0 && idx<MAX_CHOKE_SOUNDS)
				soundName = gDeathSoundsTable[idx];
			break;

		default:
			break;
	}

	if(!soundName)
		return;

	if(play)
	{
		ISound *pSound = NULL;
		if(repeating && m_sounds[sound])
			if(pSound = gEnv->pSoundSystem->GetSound(m_sounds[sound]))
				if(pSound->IsPlaying())
					return;

		if(!pSound)
			pSound = gEnv->pSoundSystem->CreateSound(soundName,FLAG_SOUND_2D);
		if(pSound)
		{
			if(repeating)
				m_sounds[sound] = pSound->GetId();
			pSound->Play();
		}
	}
	else if(repeating && m_sounds[sound])
	{
		ISound *pSound = gEnv->pSoundSystem->GetSound(m_sounds[sound]);
		if(pSound)
			pSound->Stop();
		m_sounds[sound] = 0;
	}


}
//==========================================================================================
void COffHand::MeleeAttack()
{
	if(m_melee)
	{
		CMelee* melee = static_cast<CMelee*>(m_melee);

		m_currentState = eOHS_MELEE;
		m_melee->Activate(true);
		if(melee)
		{
			melee->IgnoreEntity(m_heldEntityId);
			//Most 1-handed objects have a mass between 1.0f and 10.0f
			//Scale melee damage/impulse based on mass of the held object
			melee->MeleeScale(min(m_heldEntityMass*0.1f,1.0f)); 
		}
		m_melee->StartFire(GetOwnerId());
		m_melee->StopFire(GetOwnerId());

		GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_FirstPerson)+100, CSchedulerAction<FinishOffHandAction>::Create(FinishOffHandAction(eOHA_FINISH_MELEE,this)), false);
	}
}

//=========================================================================================
bool COffHand::IsHoldingEntity()
{
	bool ret = false;
	if(m_currentState&(eOHS_GRABBING_NPC|eOHS_HOLDING_NPC|eOHS_THROWING_NPC|eOHS_PICKING|eOHS_HOLDING_OBJECT|eOHS_THROWING_OBJECT))
		ret = true;

	return ret;
}
