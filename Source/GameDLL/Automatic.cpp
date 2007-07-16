/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Automatic.h"
#include "Actor.h"


//------------------------------------------------------------------------
CAutomatic::CAutomatic()
{
}

//------------------------------------------------------------------------
CAutomatic::~CAutomatic()
{
}

//------------------------------------------------------------------------
void CAutomatic::Update(float frameTime, uint frameId)
{
	CSingle::Update(frameTime, frameId);

	if (m_firing && CanFire(false))
		m_firing = Shoot(true);
}

//------------------------------------------------------------------------
void CAutomatic::StopFire(EntityId shooterId)
{
	if (m_zoomtimeout > 0.0f)
	{
		CActor *pActor = m_pWeapon->GetOwnerActor();
		if (pActor && pActor->IsClient() && pActor->GetScreenEffects() != 0)
		{
			pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomInID);

			// this is so we will zoom out always at the right speed
			//float speed = (1.0f/.1f) * (1.0f - pActor->GetScreenEffects()->GetCurrentFOV())/(1.0f - .75f);
			//speed = fabs(speed);
			float speed = 1.0f/.1f;
			//if (pActor->GetScreenEffects()->HasJobs(pActor->m_autoZoomOutID))
			//	speed = pActor->GetScreenEffects()->GetAdjustedSpeed(pActor->m_autoZoomOutID);

			pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomOutID);
			CFOVEffect *fov = new CFOVEffect(pActor->GetEntityId(), 1.0f);
			CLinearBlend *blend = new CLinearBlend(1);
			pActor->GetScreenEffects()->StartBlend(fov, blend, speed, pActor->m_autoZoomOutID);
		}
		m_zoomtimeout = 0.0f;
	}
	m_firing = false;
}

//------------------------------------------------------------------------
const char *CAutomatic::GetType() const
{
	return "Automatic";
}