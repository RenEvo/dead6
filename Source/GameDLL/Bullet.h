/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Bullet

-------------------------------------------------------------------------
History:
- 12:10:2005   11:15 : Created by Márcio Martins

*************************************************************************/
#ifndef __BULLET_H__
#define __BULLET_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Projectile.h"

//#define MAT_WATER_ID	194		//TODO: Get the material ID from the engine	

class CBullet : public CProjectile
{
public:
	CBullet();
	virtual ~CBullet();

	// CProjectile
	virtual void HandleEvent(const SGameObjectEvent &);
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	// ~CProjectile

	//For underwater trails (Called only from WeaponSystem.cpp)
	static void SetWaterMaterialId();
	static int  GetWaterMaterialId() { return m_waterMaterialId; }

	static IEntityClass*	EntityClass;

private:
	
	static int  m_waterMaterialId;
	bool m_underWater;

};


#endif // __BULLET_H__