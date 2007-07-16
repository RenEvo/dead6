/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: View System interfaces.
  
 -------------------------------------------------------------------------
  History:
  - 17:9:2004 : Created by Filippo De Luca

*************************************************************************/
#ifndef __IVIEWSYSTEM_H__
#define __IVIEWSYSTEM_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <ISerialize.h>

//
#define VIEWID_NORMAL 0
#define VIEWID_FOLLOWHEAD 1
#define VIEWID_VEHICLE 2
#define VIEWID_RAGDOLL 3

struct SViewParams
{
	SViewParams()
	{
		memset(this,0,sizeof(SViewParams));

		rotation.SetIdentity();
    rotationLast.SetIdentity();
		currentShakeQuat.SetIdentity();
		blendRotOffset.SetIdentity();
		blend=true;
	}

	void SetViewID(uint8 id,bool blend=true)
	{
		viewID = id;
		if (!blend)
			viewIDLast = id;
	}

	void UpdateBlending(float frameTime,float rotSpeed = 3.5f,float posSpeed = 7.5f)
	{
		//if necessary blend the view
		if (blend)
		{
			if (viewIDLast != viewID)
			{
				blendPosOffset = positionLast - position;
				blendRotOffset = (rotationLast / rotation).GetNormalized();
			}
			else
			{
				blendPosOffset -= blendPosOffset * min(1.0f,posSpeed * frameTime);
				blendRotOffset = Quat::CreateSlerp(blendRotOffset, IDENTITY, frameTime * rotSpeed );
			}

			position += blendPosOffset;
			rotation *= blendRotOffset;
		}
		else
		{
			blendPosOffset.zero();
			blendRotOffset.SetIdentity();
		}

		viewIDLast = viewID;
	}

	void SaveLast()
	{
		positionLast = position;
		rotationLast = rotation;
	}

	void ResetBlending()
	{
		blendPosOffset.zero();
		blendRotOffset.SetIdentity();
	}

	//
	Vec3 position;//view position
	Quat rotation;//view orientation

	float nearplane;//custom near clipping plane, 0 means use engine defaults
	float	fov;

	uint8 viewID;
	
	//view shake status
	float shakingRatio;//whats the ammount of shake, from 0.0 to 1.0
	Quat currentShakeQuat;//what the current angular shake
	Vec3 currentShakeShift;//what is the current translational shake

  // For damping camera movement.
  EntityId idTarget;  // Who we're watching. 0 == nobody.
  Vec3 targetPos;     // Where the target was.
  float frameTime;    // current dt.
  float angleVel;     // previous rate of change of angle.
  float vel;          // previous rate of change of dist between target and camera.
  float dist;         // previous dist of cam from target

	bool blend;

	//blending
	Vec3 blendPosOffset;
	Quat blendRotOffset;

private:

	uint8 viewIDLast;

	Vec3 positionLast;//last view position
	Quat rotationLast;//last view orientation
};

struct IGameObject;
struct IEntity;
struct IAnimSequence;
struct SCameraParams;
struct ISound;

struct IView
{
	virtual void Update(float frameTime,bool isActive) = 0;
	virtual void LinkTo(IGameObject *follow) = 0;
	virtual void LinkTo(IEntity *follow) = 0;
	virtual unsigned int GetLinkedId() = 0;

	virtual void SetCurrentParams( SViewParams &params ) = 0;
	virtual const SViewParams * GetCurrentParams() = 0;
	virtual void SetViewShake(Ang3 shakeAngle,Vec3 shakeShift,float duration,float frequency,float randomness,int shakeID, bool do_flip = true, bool bUpdateOnly=false) = 0;
	virtual void ResetShaking() = 0;
};

struct IViewSystemListener
{
	virtual bool OnBeginCutScene(IAnimSequence* pSeq, bool bResetFX) = 0;
	virtual bool OnEndCutScene(IAnimSequence* pSeq) = 0;
	virtual void OnPlayCutSceneSound(IAnimSequence* pSeq, ISound* pSound) = 0;
	virtual bool OnCameraChange(const SCameraParams& cameraParams) = 0;
};

struct IViewSystem
{
	virtual IView *CreateView() = 0;

	virtual void SetActiveView(IView *pView) = 0;
	virtual void SetActiveView(unsigned int viewId) = 0;

	//utility functions
	virtual IView *GetView(unsigned int viewId) = 0;
	virtual IView *GetActiveView() = 0;

	virtual unsigned int GetViewId(IView *pView) = 0;
	virtual unsigned int GetActiveViewId() = 0;

	virtual IView *GetViewByEntityId(unsigned int id, bool forceCreate = false) = 0;

	virtual bool AddListener(IViewSystemListener* pListener) = 0;
	virtual bool RemoveListener(IViewSystemListener* pListener) = 0;

	virtual void Serialize(TSerialize ser) = 0;

	// Used by time demo playback.
	virtual void SetOverrideCameraRotation( bool bOverride,Quat rotation ) = 0;
};

#endif //__IVIEWSYSTEM_H__
