/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Claymore mine implementation
-------------------------------------------------------------------------
History:
- 07:2:2007   12:34 : Created by Steve Humphreys

*************************************************************************/

#include "StdAfx.h"
#include "Claymore.h"

#include "IEntityProxy.h"

#include "IRenderAuxGeom.h"

float CClaymore::s_disarmTime = 0.0f;

//------------------------------------------------------------------------
CClaymore::CClaymore()
: m_triggerDirection(0, 0, 0)
, m_triggerAngle(0.0f)
, m_triggerRadius(3.0f)
, m_timeToArm(0.0f)
, m_armed(false)
{
}

//------------------------------------------------------------------------
CClaymore::~CClaymore()
{
}


//------------------------------------------------------------------------

void CClaymore::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	Vec3 newDir = dir;
	m_triggerRadius = GetParam("trigger_radius", m_triggerRadius);
	m_triggerAngle = DEG2RAD(GetParam("trigger_angle", m_triggerAngle));
	m_timeToArm = GetParam("arm_delay", m_timeToArm);	
	s_disarmTime = GetParam("disarm_time", s_disarmTime);
	newDir.z = 0.0f;
	m_triggerDirection = newDir;
	m_triggerDirection.Normalize();
	m_armed = false;

	CProjectile::Launch(pos, newDir, velocity, speedScale);
}

void CClaymore::ProcessEvent(SEntityEvent &event)
{
	switch(event.event)
	{
		case ENTITY_EVENT_ENTERAREA:
		{
			IEntity * pEntity = gEnv->pEntitySystem->GetEntity(event.nParam[0]);
			if(pEntity)
			{
				m_targetList.push_back(pEntity->GetId());
			}
			break;
		}

		case ENTITY_EVENT_LEAVEAREA:
		{
			IEntity * pEntity = gEnv->pEntitySystem->GetEntity(event.nParam[0]);
			if(pEntity)
			{
				std::list<EntityId>::iterator it = std::find(m_targetList.begin(), m_targetList.end(), pEntity->GetId());
				if(it != m_targetList.end())
					m_targetList.erase(it);
			}
			break;
		}

		default:
			break;
	}
}

void CClaymore::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	CProjectile::Update(ctx, updateSlot);

	if(gEnv->bServer)
	{
		if(m_armed)
		{
			for(std::list<EntityId>::iterator it = m_targetList.begin(); it != m_targetList.end(); ++it)
			{
				IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*it);
				if(!pEntity) continue;

				IPhysicalEntity *pPhysics = pEntity->GetPhysics();
				if(pPhysics)
				{
					pe_status_dynamics physStatus;
					if(0 != pPhysics->GetStatus(&physStatus) && physStatus.v.GetLengthSquared() > 0.01f)
					{
						// now check angle between this claymore and approaching object
						//	to see if it is within the angular range m_triggerAngle.
						//	If it is, then check distance is less than m_triggerRange,
						//	and also check line-of-sight between the two entities.
//						IRenderAuxGeom * pRAG = gEnv->pRenderer->GetIRenderAuxGeom();
//						pRAG->SetRenderFlags( e_Mode3D | e_AlphaBlended | e_DrawInFrontOff | e_FillModeSolid | e_CullModeNone );

						AABB entityBBox;
						pEntity->GetWorldBounds(entityBBox);
//						pRAG->DrawAABB( entityBBox, true, ColorF(1,0,0,0.4f), eBBD_Faceted );

						Vec3 enemyDir = entityBBox.GetCenter() - GetEntity()->GetPos();
						Vec3 checkDir = enemyDir; 
						checkDir.z = 0;
						float distanceSq = enemyDir.GetLengthSquared();

						if(distanceSq < (m_triggerRadius * m_triggerRadius))
						{
							enemyDir.NormalizeSafe();
							checkDir.NormalizeSafe();
							float dotProd = checkDir.Dot(m_triggerDirection);
#if 0
							pRAG->DrawLine(GetEntity()->GetPos(), ColorF(1,0,0,1), GetEntity()->GetPos() + Matrix33::CreateRotationZ(m_triggerAngle/2.0f)*m_triggerDirection*m_triggerRadius, ColorF(1,0,0,1), 5.0f);
							pRAG->DrawLine(GetEntity()->GetPos(), ColorF(1,0,0,1), GetEntity()->GetPos() + Matrix33::CreateRotationZ(-m_triggerAngle/2.0f)*m_triggerDirection*m_triggerRadius, ColorF(1,0,0,1), 5.0f);

							ColorF clr;
							clr.a = 0.3f;
							clr.b = 0.4f;
							clr.g = 0.1f;
							clr.r = 1.0f;
							pRAG->DrawLine(GetEntity()->GetPos(), clr, GetEntity()->GetPos() + (enemyDir * m_triggerRadius), clr, 5.0f);
#endif
							if(dotProd > cry_cosf(m_triggerAngle/2.0f))
							{
								static const int objTypes = ent_all&(~ent_terrain);   
								static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
								ray_hit hit;
								int col = gEnv->pPhysicalWorld->RayWorldIntersection(GetEntity()->GetPos(), (enemyDir * m_triggerRadius * 1.5f), objTypes, flags, &hit, 1, GetEntity()->GetPhysics());

								bool bang = false;
								if (!col)
									bang = true;
								else if (entityBBox.IsContainPoint(hit.pt))
									bang = true;
								else if (hit.pt.GetSquaredDistance(GetEntity()->GetWorldPos()) >= distanceSq)
									bang = true;
								if (bang)
								{
									Explode(true, false, Vec3(0,0,0), m_triggerDirection);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			m_timeToArm -= gEnv->pTimer->GetFrameTime();
			if(m_timeToArm <= 0.0f)
			{
				m_armed = true;

				IEntityTriggerProxy *pTriggerProxy = (IEntityTriggerProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_TRIGGER));

				if (!pTriggerProxy)
				{
					GetEntity()->CreateProxy(ENTITY_PROXY_TRIGGER);
					pTriggerProxy = (IEntityTriggerProxy*)GetEntity()->GetProxy(ENTITY_PROXY_TRIGGER);
				}

				if(pTriggerProxy)
				{
					AABB boundingBox = AABB(Vec3(-m_triggerRadius,-m_triggerRadius,-m_triggerRadius), Vec3(m_triggerRadius,m_triggerRadius,m_triggerRadius));
					pTriggerProxy->SetTriggerBounds(boundingBox);
				}
			}
		}
	}

#if 0
	if(m_armed)
	{
		IRenderAuxGeom * pRAG = gEnv->pRenderer->GetIRenderAuxGeom();
		ColorF clr;
		clr.a = 0.3f;
		clr.b = 0.4f;
		clr.g = 0.1f;
		clr.r = 1.0f;
		pRAG->SetRenderFlags( e_Mode3D | e_AlphaBlended | e_DrawInFrontOff | e_FillModeSolid | e_CullModeNone );
		pRAG->DrawCylinder(GetEntity()->GetPos(), Vec3(0, 0, 1), m_triggerRadius, m_triggerRadius * 2.0f, clr);
		/*
		// draw line from centre in direction claymore is facing
		Quat rot = GetEntity()->GetRotation();
		rot.Normalize();

		Matrix33 rotMatrix(rot);
		Vec3 dir = rot * Vec3(0,1,0);*/
		pRAG->DrawLine(GetEntity()->GetPos(), clr, GetEntity()->GetPos() + m_triggerDirection, clr, 5.0f);
	}
#endif
}
