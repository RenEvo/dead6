/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 30:8:2005   12:52 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Item.h"
#include "Actor.h"
#include "Player.h"
#include "GameCVars.h"
#include <IViewSystem.h>


//------------------------------------------------------------------------
void CItem::UpdateFPView(float frameTime)
{ 
	if (!m_stats.selected)
		return;

	CheckViewChange();

	if (!m_stats.fp && !m_stats.mounted)
		return;

	if (m_camerastats.animating)
	{
		if (m_camerastats.position)
			m_camerastats.pos=GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper.c_str(), false, true);

		if (m_camerastats.rotation)
			m_camerastats.rot=Quat(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper.c_str(), false, true)); //*Quat::CreateRotationZ(-gf_PI);
	}
	
	if (!m_stats.mounted)
	{
		UpdateFPPosition(frameTime);
		UpdateFPCharacter(frameTime);
	}

	if (IItem *pSlave = GetDualWieldSlave())
		pSlave->UpdateFPView(frameTime);

	// vehicle weapons should update here, since they don't get the PostFilterView call
	if (m_stats.mounted && GetOwnerActor() && GetOwnerActor()->IsClient() && GetOwnerActor()->GetLinkedVehicle())
		UpdateMounted(frameTime);
}

//------------------------------------------------------------------------
void CItem::UpdateFPPosition(float frameTime)
{
	CActor* pActor = GetOwnerActor();
	if (!pActor)
		return;

	SPlayerStats *pStats = static_cast<SPlayerStats *>(pActor->GetActorStats());
	if (!pStats)
		return;

	Matrix34 tm = Matrix33::CreateRotationXYZ(pStats->FPWeaponAngles);

	Vec3 offset(0.0f,0.0f,0.0f);

	float right(g_pGameCVars->i_offset_right);
	float front(g_pGameCVars->i_offset_front);
	float up(g_pGameCVars->i_offset_up);

	if (front!=0.0f || up!=0.0f || right!=0.0f)
	{
		offset += tm.GetColumn(0).GetNormalized() * right;
		offset += tm.GetColumn(1).GetNormalized() * front;
		offset += tm.GetColumn(2).GetNormalized() * up;
	}

	tm.SetTranslation(pStats->FPWeaponPos + offset);
	GetEntity()->SetWorldTM(tm);

	//CryLogAlways("weaponpos: %.3f,%.3f,%.3f // weaponrot: %.3f,%.3f,%.3f", tm.GetTranslation().x,tm.GetTranslation().y,tm.GetTranslation().z, pStats->FPWeaponAngles.x, pStats->FPWeaponAngles.y, pStats->FPWeaponAngles.z);
}

//------------------------------------------------------------------------
void CItem::UpdateFPCharacter(float frameTime)
{
	if (IsClient())
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);

		if (pCharacter && !m_idleAnimation[eIGS_FirstPerson].empty() && pCharacter->GetISkeleton()->GetNumAnimsInFIFO(0)<1)
			PlayAction(m_idleAnimation[eIGS_FirstPerson], 0, true);
	}

	// need to explicitly update characters at this point
	// cause the entity system update occered earlier, with the last position
	for (int i=0; i<eIGS_Last; i++)
	{
		if (GetEntity()->GetSlotFlags(i)&ENTITY_SLOT_RENDER)
		{
			ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
			if (pCharacter)
			{
				Matrix34 mloc = GetEntity()->GetSlotLocalTM(i,false);
				Matrix34 m34=GetEntity()->GetWorldTM()*mloc;
				QuatT renderLocation = QuatT(m34);
				pCharacter->GetISkeleton()->SetForceSkeletonUpdate(8);
				pCharacter->SkeletonPreProcess(renderLocation, renderLocation, GetISystem()->GetViewCamera() );
				pCharacter->SkeletonPostProcess(renderLocation, renderLocation, 0 );
			}
		}
	}

	IEntityRenderProxy *pProxy=GetRenderProxy();
	if (pProxy)
		pProxy->InvalidateLocalBounds();
}

//------------------------------------------------------------------------
bool CItem::FilterView(struct SViewParams &viewParams)
{
	if (m_camerastats.animating && m_camerastats.follow)
	{
		const Matrix34 tm = GetEntity()->GetSlotWorldTM(eIGS_FirstPerson);
		Vec3 offset(0.0f,0.0f,0.0f);

		offset += tm.GetColumn(0).GetNormalized()*m_camerastats.pos.x;
		offset += tm.GetColumn(1).GetNormalized()*m_camerastats.pos.y;
		offset += tm.GetColumn(2).GetNormalized()*m_camerastats.pos.z;

		viewParams.position+=offset;
		viewParams.rotation*=m_camerastats.rot;
		viewParams.blend=true;
		viewParams.viewID=5;
	}

	return m_camerastats.reorient;
}

//------------------------------------------------------------------------
void CItem::PostFilterView(struct SViewParams &viewParams)
{
	if (m_camerastats.animating && !m_camerastats.follow)
	{
		const Matrix34 tm = GetEntity()->GetSlotWorldTM(eIGS_FirstPerson);
		Vec3 offset(0.0f,0.0f,0.0f);

		offset += tm.GetColumn(0).GetNormalized()*m_camerastats.pos.x;
		offset += tm.GetColumn(1).GetNormalized()*m_camerastats.pos.y;
		offset += tm.GetColumn(2).GetNormalized()*m_camerastats.pos.z;

		viewParams.position+=offset;
		viewParams.rotation*=m_camerastats.rot;
		viewParams.blend=true;
		viewParams.viewID=5;
	}

	if (m_camerastats.animating && m_stats.mounted && !m_camerastats.helper.empty() && IsOwnerFP())
	{
		//Mounted Shi-Ten camera helper stuff
		//I need to Update here, because I need the final entity position/orientation 
		//before I get the camera helper data
		UpdateMounted(0.0f);
		viewParams.position = GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper, true);
		viewParams.rotation = Quat(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper, true));    
		viewParams.blend = true;
		viewParams.viewID=5;

		viewParams.nearplane = 0.1f;
	}
}

//------------------------------------------------------------------------
bool CItem::IsOwnerFP()
{
	CActor *pOwner = GetOwnerActor();
	if (!pOwner)
		return false;

	if (m_pGameFramework->GetClientActor() != pOwner)
		return false;

	return !pOwner->IsThirdPerson();
}

//------------------------------------------------------------------------
bool CItem::IsCurrentItem()
{
	CActor *pOwner = GetOwnerActor();
	if (!pOwner)
		return false;

	if (pOwner->GetCurrentItem() == this)
		return true;

	return false;
}

//------------------------------------------------------------------------
void CItem::UpdateMounted(float frameTime)
{
	if (!m_ownerId || !m_stats.mounted)
		return;

	CActor *pActor = GetOwnerActor();
	if (!pActor)
		return;

	CheckViewChange();

  if (true)
  {  
	  if (IsClient())
	  {
		  ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);

		  if (pCharacter && !m_idleAnimation[eIGS_FirstPerson].empty() && pCharacter->GetISkeleton()->GetNumAnimsInFIFO(0)<1)
			  PlayAction(m_idleAnimation[eIGS_FirstPerson], 0, true);
	  }

		// need to explicitly update characters at this point
		// cause the entity system update occered earlier, with the last position
		for (int i=0; i<eIGS_Last; i++)
		{
			if (GetEntity()->GetSlotFlags(i)&ENTITY_SLOT_RENDER)
			{
				ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
				if (pCharacter)
				{
					Matrix34 mloc = GetEntity()->GetSlotLocalTM(i,false);
					Matrix34 m34 = GetEntity()->GetWorldTM()*mloc;
					QuatT renderLocation = QuatT(m34);
					pCharacter->GetISkeleton()->SetForceSkeletonUpdate(9);
					pCharacter->SkeletonPreProcess(renderLocation, renderLocation, GetISystem()->GetViewCamera() );
					pCharacter->SkeletonPostProcess(renderLocation, renderLocation, 0 );		
				}
			}
		}

		if (!pActor->GetLinkedVehicle())
		{
			if (IMovementController * pMC = pActor->GetMovementController())
			{
				SMovementState info;
				pMC->GetMovementState(info);

				Vec3 dir = info.aimDirection.GetNormalized();
				if (!pActor->IsPlayer())
					dir=pActor->GetEntity()->GetWorldRotation().GetColumn1();

				Matrix34 tm = Matrix34(Matrix33::CreateRotationVDir(dir));
				tm.SetTranslation(GetEntity()->GetWorldPos());

				GetEntity()->SetWorldTM(tm);
	  		
				float	dist = m_mountparams.eye_distance;
				if (pActor->IsThirdPerson())
					dist = m_mountparams.body_distance;

				Vec3 oldp = pActor->GetEntity()->GetWorldPos();
				Vec3 newp = GetEntity()->GetWorldPos()-dir*dist;
				newp.z = oldp.z;

			//	Matrix34 actortm(Matrix33::CreateRotationVDir(dir).GetTransposed());
				Matrix34 actortm(pActor->GetEntity()->GetWorldTM());
				actortm.SetTranslation(newp);
				pActor->GetEntity()->SetWorldTM(actortm, ENTITY_XFORM_USER);
				pActor->GetAnimationGraphState()->SetInput("Action","gunnerMounted");

				if (ICharacterInstance *pCharacter = pActor->GetEntity()->GetCharacter(0))
				{
					ISkeleton *pSkeleton = pCharacter->GetISkeleton();
					assert(pSkeleton);

					if (uint32 numAnimsLayer = pSkeleton->GetNumAnimsInFIFO(0))
					{
						CAnimation &animation = pSkeleton->GetAnimFromFIFO(0, 0);
						if (animation.m_AnimParams.m_nFlags & CA_MANUAL_UPDATE)
						{
							Vec2 d(dir.x, dir.y);
							d.NormalizeSafe();
							float dx=d.x;
							float dy=d.y;
							float value=0.0f;
	  					
							value=atan2(dy, dx);

							value=value/(gf_PI*0.5f);
							value = fabs_tpl(value);  //Just in case it was negative...

							while(value>1.0f)
								value-=1.0f;
							animation.m_fAnimTime=1.0f-value;

							//added ivo
							assert(animation.m_fAnimTime>=0.0f && animation.m_fAnimTime<=1.0f);
							if (animation.m_fAnimTime<0.0f || animation.m_fAnimTime>1.0f)
							{
								GetISystem()->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,0,	"not normalized animation time: %f",animation.m_fAnimTime);
								if (animation.m_fAnimTime<0.0f)
									animation.m_fAnimTime=0.0f;
								if (animation.m_fAnimTime>1.0f)
									animation.m_fAnimTime=1.0f;
							}

							//if (dir.Dot(m_stats.mount_last_aimdir)<0.999f)
							//	animation.m_fAnimTime += (Random()-0.5f)*0.01f;
						}
					}
				}

				m_stats.mount_last_aimdir = dir;
				
			}
		}
    
    if (ICharacterInstance* pCharInstance = pActor->GetEntity()->GetCharacter(0))
		{
       if (ISkeleton* pSkeleton = pCharInstance->GetISkeleton())
      {   
        OldBlendSpace ap;				
        if (GetAimBlending(ap))
        {
					pSkeleton->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, 0.5f - 0.5f * ap.m_turn, true);
        }        
      }        
    } 

    UpdateIKMounted(pActor);
   
		RequireUpdate(eIUS_General);
  }
}

//------------------------------------------------------------------------
void CItem::UpdateIKMounted(IActor* pActor)
{
  if (!m_mountparams.left_hand_helper.empty() || !m_mountparams.right_hand_helper.empty())
  { 
    Vec3 lhpos=GetSlotHelperPos(eIGS_FirstPerson, m_mountparams.left_hand_helper.c_str(), true);
    Vec3 rhpos=GetSlotHelperPos(eIGS_FirstPerson, m_mountparams.right_hand_helper.c_str(), true);
    pActor->SetIKPos("leftArm", lhpos, 1);
    pActor->SetIKPos("rightArm", rhpos, 1);

    //gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(lhpos, 0.125f, ColorB(255, 255, 255, 255));
    //gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(rhpos, 0.125f, ColorB(128, 128, 128, 255));
  }
}

//------------------------------------------------------------------------
bool CItem::GetAimBlending(OldBlendSpace& params)
{ 
  // unused here so far 
  return false;
}


//------------------------------------------------------------------------
void CItem::CheckViewChange()
{
  CActor *pOwner = GetOwnerActor();
  
  if (m_stats.mounted)
	{
    bool fp = pOwner?!pOwner->IsThirdPerson():false;

    if (fp!=m_stats.fp)
    {
      if (fp || !(m_stats.viewmode&eIVM_FirstPerson))
        OnEnterFirstPerson();
      else if (!fp)
        AttachArms(false, false);
    }
    
    m_stats.fp = fp;
 
		return;
	}

  if (!pOwner)
    return;

	if (!pOwner->IsThirdPerson())
	{
		if (!m_stats.fp || !(m_stats.viewmode&eIVM_FirstPerson))
			OnEnterFirstPerson();
		m_stats.fp = true;
	}
	else
	{
		if (m_stats.fp || !(m_stats.viewmode&eIVM_ThirdPerson))
			OnEnterThirdPerson();
		m_stats.fp = false;
	}
}

//------------------------------------------------------------------------
void CItem::SetViewMode(int mode)
{
	m_stats.viewmode = mode;

	if (mode & eIVM_FirstPerson)
	{
		SetHand(m_stats.hand);

		if (!m_parentId)
		{
			uint flags = GetEntity()->GetFlags();
			if (!m_stats.mounted)
				flags &= ~ENTITY_FLAG_CASTSHADOW;
			else
				flags |= ENTITY_FLAG_CASTSHADOW;

			//GetEntity()->SetFlags(flags|ENTITY_FLAG_RECVSHADOW);
			DrawSlot(eIGS_FirstPerson, true, !m_stats.mounted);
		}
		else
			DrawSlot(eIGS_FirstPerson, false, false);
	}
	else
	{
		SetGeometry(eIGS_FirstPerson, 0);
	}

	if (mode & eIVM_ThirdPerson)
	{
		DrawSlot(eIGS_ThirdPerson, true);
		if (!m_stats.mounted)
			CopyRenderFlags(GetOwner());
	}
	else
		DrawSlot(eIGS_ThirdPerson, false);

	for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
	{
		IItem *pItem = m_pGameFramework->GetIItemSystem()->GetItem(it->second);
		if (pItem)
		{
			CItem *pCItem = static_cast<CItem *>(pItem);
			if (pCItem)
				pCItem->SetViewMode(mode);
		}
	}
}

//------------------------------------------------------------------------
void CItem::ResetRenderFlags()
{
	if (!GetRenderProxy())
		return;

	IRenderNode *pRenderNode = GetRenderProxy()->GetRenderNode();
	if (pRenderNode)
	{
		pRenderNode->SetViewDistRatio(127);
		pRenderNode->SetLodRatio(127);
		GetEntity()->SetFlags(GetEntity()->GetFlags()|ENTITY_FLAG_CASTSHADOW);	
	}
}

//------------------------------------------------------------------------
void CItem::CopyRenderFlags(IEntity *pOwner)
{
	if (!pOwner || !GetRenderProxy())
		return;

	IRenderNode *pRenderNode = GetRenderProxy()->GetRenderNode();
	if (pRenderNode)
	{
		IEntityRenderProxy *pOwnerRenderProxy = (IEntityRenderProxy *)pOwner->GetProxy(ENTITY_PROXY_RENDER);
		IRenderNode *pOwnerRenderNode = pOwnerRenderProxy?pOwnerRenderProxy->GetRenderNode():NULL;
		if (pOwnerRenderNode)
		{
			pRenderNode->SetViewDistRatio(pOwnerRenderNode->GetViewDistRatio());
			pRenderNode->SetLodRatio(pOwnerRenderNode->GetLodRatio());

			uint flags = pOwner->GetFlags()&(ENTITY_FLAG_CASTSHADOW);
			uint mflags = GetEntity()->GetFlags()&(~(ENTITY_FLAG_CASTSHADOW));
			GetEntity()->SetFlags(mflags|flags);
		}
	}
}
