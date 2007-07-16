/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Anti-Vehicle mine implementation
-------------------------------------------------------------------------
History:
- 22:1:2007   14:39 : Created by Steve Humphreys

*************************************************************************/

#ifndef __AVMINE_H__
#define __AVMINE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Projectile.h"


class CAVMine : public CProjectile
{
public:
	CAVMine();
	virtual ~CAVMine();

	virtual void ProcessEvent(SEntityEvent &event);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);

	static float GetDisarmTime() { return s_disarmTime; }

protected:
	float m_triggerWeight;
	float m_currentWeight;
	static float s_disarmTime;
};


#endif // __AVMINE_H__