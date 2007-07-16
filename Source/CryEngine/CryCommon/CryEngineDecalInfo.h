//////////////////////////////////////////////////////////////////////
//
//	Crytek Common Source code
//	
//	File:CryEngineDecalInfo.h
//	Description: declaration of struct CryEngineDecalInfo.
//
//	History:
//	-Sep 23, 2002: Created by Ivo Herzeg
//
//	Note:
//    3D Engine and Character Animation subsystems (as well as perhaps
//    some others) transfer data about the decals that need to be spawned
//    via this structure. This is to avoid passing many parameters through
//    each function call, and to save on copying these parameters when just
//    simply passing the structure from one function to another.
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRY_ENGINE_DECAL_INFO_HDR_
#define _CRY_ENGINE_DECAL_INFO_HDR_

// Summary:
//     Structure containing common parameters that describe a decal

struct SDecalOwnerInfo
{
	SDecalOwnerInfo() { memset(this,0,sizeof(*this)); nRenderNodeSlotId = nRenderNodeSlotSubObjectId = -1; }
	struct IStatObj * GetOwner(Matrix34 & objMat);

	struct IRenderNode *	pRenderNode;							// Owner (decal will be rendered on this entity)
	PodArray<struct SRNInfo>*pDecalReceivers;
	int										nRenderNodeSlotId;  // set by 3dengine
	int										nRenderNodeSlotSubObjectId; // set by 3dengine
};

struct CryEngineDecalInfo
{
	SDecalOwnerInfo				ownerInfo;
	Vec3                  vPos;											// Decal position (world coordinates)
	Vec3                  vNormal;									// Decal/face normal
	float									fSize;										// Decal size
	float									fLifeTime;								// Decal life time (in seconds)
	char									szTextureName[_MAX_PATH]; // texture name
	float									fAngle;										// Angle of rotation
	struct IStatObj *			pIStatObj;									// Decal geometry
	Vec3                  vHitDirection;						// Direction from weapon/player position to decal position (bullet direction)
	float									m_fGrowTime;							// Used for blood pools
	bool									bAdjustPos;								// Place decal on some visible surface
	uint8									sortPrio;									
	
	char szMaterialName[_MAX_PATH]; // name of material used for rendering the decal (in favor of szTextureName/nTid and the default decal shader)	
	
	bool preventDecalOnGround;				// mainly for decal placement support
	const Matrix33* pExplicitRightUpFront;	// mainly for decal placement support

	// the constructor fills in some non-obligatory fields; the other fields must be filled in by the client
	CryEngineDecalInfo ()
	{
		memset(this,0,sizeof(*this));
		bAdjustPos = true;
		sortPrio = 255;
	}
};

#endif