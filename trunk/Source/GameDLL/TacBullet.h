#pragma once
#include "Projectile.h"


class CTacBullet :
	public CProjectile
{
public:
	CTacBullet(void);
	~CTacBullet(void);
	virtual void HandleEvent(const SGameObjectEvent &);
};
