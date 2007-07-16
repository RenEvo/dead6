/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Repair Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 4:8:2006   10:21 : Created by Márcio Martins

*************************************************************************/
#ifndef __REPAIR_H__
#define __REPAIR_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Beam.h"


class CRepair :
	public CBeam
{
public:
	CRepair();
	virtual ~CRepair();

	// IFireMode
	virtual const char *GetType() const { return "Repair"; };
	//~IFireMode

	// no new members... don't override GetMemoryStatistics

	virtual void Hit(ray_hit &hit, const Vec3 &dir);
	virtual void Tick(ray_hit &hit, const Vec3 &dir);
};


#endif //__REPAIR_H__