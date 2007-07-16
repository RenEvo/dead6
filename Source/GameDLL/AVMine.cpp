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

#include "StdAfx.h"
#include "AVMine.h"

#include "IEntityProxy.h"

float CAVMine::s_disarmTime = 0.0f;

//------------------------------------------------------------------------
CAVMine::CAVMine()
: m_currentWeight(0)
, m_triggerWeight(100)
{
}

//------------------------------------------------------------------------
CAVMine::~CAVMine()
{
}


//------------------------------------------------------------------------

void CAVMine::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CProjectile::Launch(pos, dir, velocity, speedScale);

	float boxDimension = 3;
	m_triggerWeight = GetParam("triggerweight", m_triggerWeight);
	boxDimension = GetParam("box_dimension", boxDimension);
	s_disarmTime = GetParam("disarm_time", s_disarmTime);

	IEntityTriggerProxy *pTriggerProxy = (IEntityTriggerProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_TRIGGER));
	
	if (!pTriggerProxy)
	{
		GetEntity()->CreateProxy(ENTITY_PROXY_TRIGGER);
		pTriggerProxy = (IEntityTriggerProxy*)GetEntity()->GetProxy(ENTITY_PROXY_TRIGGER);
	}

	if(pTriggerProxy)
	{
		AABB boundingBox = AABB(Vec3(-boxDimension,-boxDimension,-boxDimension), Vec3(boxDimension,boxDimension,boxDimension));
		pTriggerProxy->SetTriggerBounds(boundingBox);
	}
}

void CAVMine::ProcessEvent(SEntityEvent &event)
{
	switch(event.event)
	{
		case ENTITY_EVENT_ENTERAREA:
		{
			IEntity * pEntity = gEnv->pEntitySystem->GetEntity(event.nParam[0]);
			if(pEntity)
			{
				IPhysicalEntity *pPhysics = pEntity->GetPhysics();
				if(pPhysics)
				{
					pe_status_dynamics physStatus;
					if(0 != pPhysics->GetStatus(&physStatus))
					{
						// only count moving objects
						if(physStatus.v.GetLengthSquared() > 0.1f)
							m_currentWeight += physStatus.mass;

						if(m_currentWeight > m_triggerWeight)
							Explode(true);
					}
				}
			}
			break;
		}
		

		case ENTITY_EVENT_LEAVEAREA:
		{
			IEntity * pEntity = gEnv->pEntitySystem->GetEntity(event.nParam[0]);
			if(pEntity)
			{
				IPhysicalEntity *pPhysics = pEntity->GetPhysics();
				if(pPhysics)
				{
					pe_status_dynamics physStatus;
					if(0 != pPhysics->GetStatus(&physStatus))
					{
						m_currentWeight -= physStatus.mass;

						if(m_currentWeight < 0)
							m_currentWeight = 0;
					}
				}
			}
			break;
		}

		default:
			break;
	}
}
