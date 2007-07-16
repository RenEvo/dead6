/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 17:1:2006   11:18 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "TracerManager.h"
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"
#include <I3DEngine.h>


//------------------------------------------------------------------------
CTracer::CTracer(const Vec3 &pos)
: m_length(1.0f),
	m_pos(0,0,0),
	m_dest(0,0,0),
	m_startingpos(pos),
	m_age(0.0f),
	m_lifeTime(1.5f),
	m_entityId(0)
{
	CreateEntity();
}

//------------------------------------------------------------------------
CTracer::~CTracer()
{
	if (m_entityId)
		gEnv->pEntitySystem->RemoveEntity(m_entityId);
	m_entityId=0;
}

//------------------------------------------------------------------------
void CTracer::Reset(const Vec3 &pos)
{
	m_length=1.0f;
	m_pos=Vec3(0,0,0);
	m_dest=Vec3(0,0,0);
	m_startingpos=pos;
	m_age=0.0f;
	m_lifeTime=1.5f;

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		pEntity->FreeSlot(0);
		pEntity->FreeSlot(1);
	}
}

//------------------------------------------------------------------------
void CTracer::CreateEntity()
{
	if (!m_entityId)
	{
		SEntitySpawnParams spawnParams;
		spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
		spawnParams.sName = "_tracer";
		spawnParams.nFlags = ENTITY_FLAG_NO_PROXIMITY | ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_SAVE;

		if (IEntity *pEntity=gEnv->pEntitySystem->SpawnEntity(spawnParams))
			m_entityId=pEntity->GetId();
	}
}

//------------------------------------------------------------------------
void CTracer::GetMemoryStatistics(ICrySizer * s) const
{
	s->Add(*this);
}

//------------------------------------------------------------------------
void CTracer::SetGeometry(const char *name, float scale)
{
	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		int slot=pEntity->LoadGeometry(-1, name);

		if (scale!=1.0f)
		{
			Matrix34 tm=Matrix34::CreateIdentity();
			tm.Scale(Vec3(scale, scale, scale));
			pEntity->SetSlotLocalTM(slot, tm);
		}
	}
}

//------------------------------------------------------------------------
void CTracer::SetEffect(const char *name, float scale)
{
	IParticleEffect *pEffect = gEnv->p3DEngine->FindParticleEffect(name);
	if (!pEffect)
		return;

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		int slot=pEntity->LoadParticleEmitter(-1, pEffect);
		if (scale!=1.0f)
		{
			Matrix34 tm=Matrix34::CreateIdentity();
			tm.Scale(Vec3(scale, scale, scale));
			pEntity->SetSlotLocalTM(slot, tm);
		}
	}
}

//------------------------------------------------------------------------
bool CTracer::Update(float frameTime, const Vec3 &camera)
{
	m_age += frameTime;

	if (m_age >= m_lifeTime)
		return false;

	Vec3 end(m_dest);

	if ((m_pos-end).len2() <= 0.5f*0.5f)
		return false;

	Vec3 dp = end-m_pos;
	float dist = dp.len();
	Vec3 dir = dp/dist;

	float scaleMult=1.0f;

	float minDistance = g_pGameCVars->tracer_min_distance;
	float maxDistance = g_pGameCVars->tracer_max_distance;
	float minLength = g_pGameCVars->tracer_min_length;
	float maxLength = g_pGameCVars->tracer_max_length;
	float minScale = g_pGameCVars->tracer_min_scale;
	float maxScale = g_pGameCVars->tracer_max_scale;
	float sqrRadius = g_pGameCVars->tracer_player_radiusSqr;

	float cameraDistance = (m_pos-camera).len2();
	float speed = m_speed;

	//Slow down tracer when near the player
	if(cameraDistance<=sqrRadius)
	{
		speed *= (0.35f + (cameraDistance/(sqrRadius*2)));
	}

	m_pos = m_pos+dir*MIN(speed*frameTime, dist);
	cameraDistance = (m_pos-camera).len2();

	if (cameraDistance<=minDistance*minDistance)
	{
		m_length=minLength;
		scaleMult=minScale;
	}
	else if (cameraDistance>=maxDistance*maxDistance)
	{
		m_length=maxLength;
		scaleMult=maxScale;
	}
	else
	{
		float t=(sqrtf(cameraDistance)-minDistance)/(maxDistance-minDistance);
		m_length=minLength+t*(maxLength-minLength);
		scaleMult=minScale+t*(maxScale-minScale);
	}

	float cosine = dir.Dot((m_pos-camera).normalized());
	m_length = minLength+(m_length-minLength)*fabsf(cosine);

	if ((m_pos-end).len2() > 0.5f*0.5f)
	{
		if ((m_pos-m_startingpos).len2()<m_length*m_length)
			m_pos = m_startingpos+dir*m_length;
	}

	UpdateVisual(m_pos, dir, scaleMult, m_length);

	return true;
}

//------------------------------------------------------------------------
void CTracer::UpdateVisual(const Vec3 &pos, const Vec3 &dir, float scale, float length)
{
	Matrix34 tm(Matrix33::CreateRotationVDir(dir));
	tm.AddTranslation(pos);
	tm.Scale(Vec3(scale, scale, scale));

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
		pEntity->SetWorldTM(tm);
}

//------------------------------------------------------------------------
void CTracer::SetLifeTime(float lifeTime)
{
	m_lifeTime = lifeTime;
}

//------------------------------------------------------------------------
CTracerManager::CTracerManager()
: m_lastFree(0)
{
}

//------------------------------------------------------------------------
CTracerManager::~CTracerManager()
{
}

//------------------------------------------------------------------------
void CTracerManager::EmitTracer(const STracerParams &params)
{
	if(!g_pGameCVars->g_enableTracers)
		return;

	int idx=0;
	if (m_pool.empty() && g_pGameCVars->tracer_max_count>0)
	{
		m_pool.push_back(new CTracer(params.position));
		idx=m_pool.size()-1;
	}
	else
	{
		int s=m_pool.size();
		int i=m_lastFree+1;

		while (true)
		{
			IEntity *pEntity=0;
			if (i<(int)m_pool.size())
				pEntity=gEnv->pEntitySystem->GetEntity(m_pool[i]->m_entityId);

			if (i==s)
				i=0;
			else if (i==m_lastFree)
			{
				m_pool.push_back(new CTracer(params.position));
				idx=m_pool.size()-1;
				break;
			}
			else if (pEntity && pEntity->IsHidden())
			{
				m_pool[i]->Reset(params.position);
				idx=i;
				break;
			}
			else
				++i;
		}
	}

	m_lastFree=idx;

	CTracer *tracer = m_pool[idx];

	if (params.geometry && params.geometry[0])
		tracer->SetGeometry(params.geometry, params.geometryScale);
	if (params.effect && params.effect[0])
		tracer->SetEffect(params.effect, params.effectScale);
	tracer->SetLifeTime(params.lifetime);

	tracer->m_speed = params.speed * g_pGameCVars->tracer_speed_scale;
	tracer->m_pos = params.position;
	tracer->m_dest = params.destination;

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(tracer->m_entityId))
		pEntity->Hide(0);

	m_actives.push_back(idx);
}

//------------------------------------------------------------------------
void CTracerManager::Update(float frameTime)
{
	IActor *pActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pActor)
		return;

	SMovementState state;
	if (!pActor->GetMovementController())
		return;
	
	pActor->GetMovementController()->GetMovementState(state);

	m_actives.swap(m_updating);
	for (TTracerIdVector::iterator it = m_updating.begin(); it!=m_updating.end(); ++it)
	{
		CTracer *tracer = m_pool[*it];

		if (tracer->Update(frameTime, state.eyePosition))
			m_actives.push_back(*it);
		else
		{
			if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(tracer->m_entityId))
			{
				pEntity->Hide(1);
				pEntity->SetWorldTM(Matrix34::CreateIdentity());
			}
		}
	}

	m_updating.resize(0);
}

//------------------------------------------------------------------------
void CTracerManager::Reset()
{
	for (TTracerPool::iterator it = m_pool.begin(); it!=m_pool.end(); ++it)
		delete *it;

	m_updating.resize(0);
	m_actives.resize(0);
	m_pool.resize(0);
}

void CTracerManager::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "TracerManager");
	s->Add(*this);
	s->AddContainer(m_updating);
	s->AddContainer(m_actives);
	s->AddContainer(m_pool);

	for (size_t i=0; i<m_pool.size(); i++)
		m_pool[i]->GetMemoryStatistics(s);
}