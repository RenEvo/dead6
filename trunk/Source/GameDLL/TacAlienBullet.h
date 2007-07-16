/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Tactical Sleep Alien Bullet Implementation

-------------------------------------------------------------------------
History:
- 22:11:2006   19:55 : Created by Benito Gangoso Rodriguez

-  5:02:2007   11:55 : This won't be in game (Removed from weapon system)
*************************************************************************/

#pragma once
#include "TacBullet.h"

class CTacAlienBullet :	public CTacBullet
{
	public:
		CTacAlienBullet(void);
		~CTacAlienBullet(void);
	
		virtual void HandleEvent(const SGameObjectEvent &);
};