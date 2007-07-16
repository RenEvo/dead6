/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 19:12:2005   12:10 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "FreezingBeam.h"
#include "Game.h"
#include "GameRules.h"



//------------------------------------------------------------------------
CFreezingBeam::CFreezingBeam()
{
}

//------------------------------------------------------------------------
CFreezingBeam::~CFreezingBeam()
{
}

//------------------------------------------------------------------------
void CFreezingBeam::ResetParams(const struct IItemParamsNode *params)
{
  CBeam::ResetParams(params);

  const IItemParamsNode *freeze = params?params->GetChild("freeze"):0;  
  m_freezeparams.Reset(freeze);  
}

//------------------------------------------------------------------------
void CFreezingBeam::PatchParams(const struct IItemParamsNode *patch)
{
  CBeam::PatchParams(patch);

  const IItemParamsNode *freeze = patch?patch->GetChild("freeze"):0;  
  m_freezeparams.Reset(freeze, false);
}


//------------------------------------------------------------------------
void CFreezingBeam::Hit(ray_hit &hit, const Vec3 &dir)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);

	if (pEntity && (hit.pCollider->GetType()==PE_RIGID || hit.pCollider->GetType()==PE_ARTICULATED || hit.pCollider->GetType()==PE_LIVING || hit.pCollider->GetType()==PE_WHEELEDVEHICLE))
  {
		g_pGame->GetGameRules()->ClientSimpleHit(SimpleHitInfo(m_shooterId, pEntity->GetId(), m_pWeapon->GetEntityId(), 0xe));
  }
}

//------------------------------------------------------------------------
void CFreezingBeam::Tick(ray_hit &hit, const Vec3 &dir)
{
}

//------------------------------------------------------------------------
void CFreezingBeam::TickDamage(ray_hit &hit, const Vec3 &dir)
{ 
  IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
  
  if (pEntity)
  {
    CGameRules *pGameRules = g_pGame->GetGameRules();

    HitInfo info(m_pWeapon->GetOwnerId(), pEntity->GetId(), m_pWeapon->GetEntityId(), m_fireparams.damage, 0.25f,
      pGameRules->GetHitMaterialIdFromSurfaceId(hit.surface_idx), hit.partid, pGameRules->GetHitTypeId(m_fireparams.hit_type.c_str()),
      hit.pt, dir, hit.n);

    info.frost = m_freezeparams.freeze_speed>0.f ? m_freezeparams.freeze_speed*m_beamparams.tick : 1.f;

    //gEnv->pRenderer->DrawLabel(hit.pt, 1.8f, "Frost: %.2f", info.frost);

    if (m_pWeapon->GetForcedHitMaterial() != -1)
      info.material=pGameRules->GetHitMaterialIdFromSurfaceId(m_pWeapon->GetForcedHitMaterial());

    pGameRules->ClientHit(info);
  }
}


void CFreezingBeam::GetMemoryStatistics(ICrySizer * s)
{
  s->Add(*this);
  CBeam::GetMemoryStatistics(s);
  m_freezeparams.GetMemoryStatistics(s);  
}
