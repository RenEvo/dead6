/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Item Implementation

-------------------------------------------------------------------------
History:
- 11:9:2004   15:00 : Created by Márcio Martins

*************************************************************************/
#ifndef __AUTOMATIC_H__
#define __AUTOMATIC_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"


class CAutomatic : public CSingle
{
public:
	CAutomatic();
	virtual ~CAutomatic();

	// CSingle
	virtual void Update(float frameTime, uint frameId);
	virtual void StopFire(EntityId shooterId);
	virtual const char *GetType() const;
	// no new members... don't override GetMemoryStatistics

	// ~CSingle
};

#endif //__AUTOMATIC_H__