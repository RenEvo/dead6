/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 28:10:2005   16:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "IronSight.h"
#include "Player.h"
#include "GameCVars.h"
#include "Single.h"

#define PHYS_FOREIGN_ID_DOF_QUERY PHYS_FOREIGN_ID_USER+3

#define REFLEX_DOT_ON_TIMER 0.75f
//------------------------------------------------------------------------
CIronSight::CIronSight()
: m_pWeapon(0),
	m_savedFoVScale(0.0f),
	m_zoomed(false),
	m_zoomingIn(false),
	m_zoomTimer(0.0f),
	m_zoomTime(0.0f),
	m_focus(1.0f),
	m_startFoV(0.0f),
	m_endFoV(0.0f),
	m_smooth(0.0f),
	m_enabled(true),
	m_currentStep(0),
	m_FarCry_style(false),
	m_initialNearFov(60.0f),
	m_reflexDotEntity(0),
	m_maxDoF(100.0f),
	m_minDoF(0.0f),
	m_averageDoF(50.0f)
{
}

//------------------------------------------------------------------------
CIronSight::~CIronSight()
{
	DestroyReflexDot();
}

//------------------------------------------------------------------------
void CIronSight::Init(IWeapon *pWeapon, const struct IItemParamsNode *params)
{
	m_pWeapon = static_cast<CWeapon *>(pWeapon);

	ResetParams(params);
}

//------------------------------------------------------------------------
void CIronSight::Update(float frameTime, uint frameId)
{
	bool keepUpdating=false;

	float doft = 1.0f;
	if (!m_zoomed)
		doft = 0.0f;

	if (m_zoomTime > 0.0f)	// zoomTime is set to 0.0 when zooming ends
	{
		keepUpdating=true;
		float t = CLAMP(1.0f-m_zoomTimer/m_zoomTime, 0.0f, 1.0f);
		float fovScale;

		if (m_smooth)
		{
			if (m_startFoV > m_endFoV)
				doft = t;
			else
				doft = 1.0f-t;

			fovScale = m_startFoV+t*(m_endFoV-m_startFoV);
		}
		else
		{
			fovScale = m_startFoV;
			if (t>=1.0f)
				fovScale = m_endFoV;
		}

		OnZoomStep(m_startFoV>m_endFoV, t);

		SetActorFoVScale(fovScale, true, true, true);

		if(m_zoomparams.scope_mode)
		{
			AdjustScopePosition(t,m_startFoV>m_endFoV);
			AdjustNearFov(t,m_startFoV>m_endFoV);
		}

		// marcok: please don't touch
		if (g_pGameCVars->cl_tryme)
		{
			const static float scale_start = 1.0f;
			const static float scale_end = 0.2f;
			static ICVar* time_scale = gEnv->pConsole->GetCVar("time_scale");

			g_pGameCVars->cl_tryme_targety = LERP((-2.5f), (-1.5f), doft*doft);
			float scale = LERP(scale_start, scale_end, doft*doft);
			if (g_pGameCVars->cl_tryme_bt_ironsight && !g_pGameCVars->cl_tryme_bt_speed)
			{
				time_scale->Set(scale);
			}
		}

		if (t>=1.0f)
		{
			if (m_zoomingIn)
			{
				m_zoomed = true;
				m_pWeapon->RequestZoom(fovScale);
			}
			else
			{
				m_zoomed = false;
				m_pWeapon->RequestZoom(1.0f);
			}

			m_zoomTime = 0.0f;
		}
	}

	if (g_pGameCVars->g_dof_ironsight != 0 && g_pGameCVars->cl_tryme==0)
		UpdateDepthOfField(frameTime, doft);

	if (m_zoomTimer>0.0f || m_zoomed)
	{
		m_zoomTimer -= frameTime;
		if (m_zoomTimer<0.0f)
			m_zoomTimer=0.0f;		

		if (m_focus < 1.0f)
		{
			m_focus += frameTime*1.5f;
		}
		else if (m_focus > 1.0f)
		{
			m_focus = 1.0f;
		}

		//float t=m_zoomTimer/m_zoomTime;
		if (m_zoomTime > 0.0f)
		{
			//t=1.0f-max(t, 0.0f);
			gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 1.0f);
		}

		gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMin", g_pGameCVars->g_dofset_minScale * m_minDoF);
		gEnv->p3DEngine->SetPostEffectParam("Dof_FocusMax", g_pGameCVars->g_dofset_maxScale * m_averageDoF);
		gEnv->p3DEngine->SetPostEffectParam("Dof_FocusLimit", g_pGameCVars->g_dofset_limitScale * m_averageDoF);

		keepUpdating=true;
	}

	if (keepUpdating)
		m_pWeapon->RequireUpdate(eIUS_Zooming);
}

//------------------------------------------------------------------------
void CIronSight::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CIronSight::ResetParams(const struct IItemParamsNode *params)
{
	const IItemParamsNode *zoom = params?params->GetChild("zoom"):0;
	const IItemParamsNode *actions = params?params->GetChild("actions"):0;
	const IItemParamsNode *spreadMod = params?params->GetChild("spreadMod"):0;
	const IItemParamsNode *recoilMod = params?params->GetChild("recoilMod"):0; 

	m_zoomparams.Reset(zoom);
	m_actions.Reset(actions);
	m_spreadModParams.Reset(spreadMod);
	m_recoilModParams.Reset(recoilMod);
}

//------------------------------------------------------------------------
void CIronSight::PatchParams(const struct IItemParamsNode *patch)
{
	const IItemParamsNode *zoom = patch->GetChild("zoom");
	const IItemParamsNode *actions = patch->GetChild("actions");
	const IItemParamsNode *spreadMod = patch->GetChild("spreadMod");
	const IItemParamsNode *recoilMod = patch->GetChild("recoilMod");

	m_zoomparams.Reset(zoom, false);
	m_actions.Reset(actions, false);
	m_spreadModParams.Reset(spreadMod, false);
	m_recoilModParams.Reset(recoilMod, false);
}

//------------------------------------------------------------------------
void CIronSight::Activate(bool activate)
{
	if (!activate && m_zoomed && m_zoomingIn && !m_zoomparams.dof_mask.empty())
		ClearDoF();

	if (!activate && !m_zoomparams.suffix.empty())
		m_pWeapon->SetActionSuffix("");

	m_zoomed = false;
	m_zoomingIn = false;

	m_currentStep = 0;

	SetRecoilScale(1.0f);
	SetActorFoVScale(1.0f, true, true, true);
	SetActorSpeedScale(1.0f);

	ResetTurnOff();

	if(activate && m_zoomparams.reflex_aimDot)
	{
		if(!m_reflexDotEntity)
			CreateReflexDot();
		HideReflexDot(true);
		m_reflexDotOnTimer = REFLEX_DOT_ON_TIMER;
	}
	if(!activate && m_zoomparams.scope_mode)
	{
		ResetFovAndPosition();
	}

	m_FarCry_style = false;
}

//------------------------------------------------------------------------
bool CIronSight::CanZoom() const
{
	return true;
}

//------------------------------------------------------------------------
bool CIronSight::StartZoom(bool stayZoomed, bool fullZoomout, int zoomStep)
{
	if (m_pWeapon->IsBusy())
		return false;
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (pActor && pActor->GetScreenEffects() != 0)
	{
		pActor->GetScreenEffects()->EnableBlends(false, pActor->m_autoZoomInID);
		pActor->GetScreenEffects()->EnableBlends(false, pActor->m_autoZoomOutID);
		pActor->GetScreenEffects()->EnableBlends(false, pActor->m_hitReactionID);
	}
	if (!m_zoomed || stayZoomed)
	{
		CheckFarCryIronSight();
		EnterZoom(m_zoomparams.zoom_in_time, m_zoomparams.layer.c_str(), true, zoomStep);
		m_currentStep = zoomStep;

		m_pWeapon->AssistAiming(m_zoomparams.stages[m_currentStep-1], true);
	}
	else
	{
		int currentStep = m_currentStep;
		int nextStep = currentStep+1;

		if (nextStep > m_zoomparams.stages.size())
		{
			if (!stayZoomed)
			{
				if (fullZoomout)
				{
					StopZoom();
				}
				else
				{
					float oFoV = GetZoomFoVScale(currentStep);
					m_currentStep = 0;
					float tFoV = GetZoomFoVScale(m_currentStep);
					ZoomIn(m_zoomparams.stage_time, oFoV, tFoV, true);
					return true;
				}
			}
		}
		else
		{
			float oFoV = GetZoomFoVScale(currentStep);
			float tFoV = GetZoomFoVScale(nextStep);

			ZoomIn(m_zoomparams.stage_time, oFoV, tFoV, true);

			m_currentStep = nextStep;

			m_pWeapon->AssistAiming(m_zoomparams.stages[m_currentStep-1], true);
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
void CIronSight::StopZoom()
{
	LeaveZoom(m_zoomparams.zoom_out_time, true);
	m_currentStep = 0;
}


//------------------------------------------------------------------------
void CIronSight::ZoomIn()
{
	if (m_pWeapon->IsBusy())
		return;

	if (!m_zoomed)
	{
		EnterZoom(m_zoomparams.zoom_in_time, m_zoomparams.layer.c_str(), true);
		m_currentStep = 1;
	}
	else
	{
		int currentStep = m_currentStep;
		int nextStep = currentStep+1;

		if (nextStep > m_zoomparams.stages.size())
			return;
		else
		{
			float oFoV = GetZoomFoVScale(currentStep);
			float tFoV = GetZoomFoVScale(nextStep);

			ZoomIn(m_zoomparams.stage_time, oFoV, tFoV, true);

			m_currentStep = nextStep;
		}
	}
}

//------------------------------------------------------------------------
bool CIronSight::ZoomOut()
{
	if (m_pWeapon->IsBusy())
		return false;

	if (!m_zoomed)
	{
		EnterZoom(m_zoomparams.zoom_in_time, m_zoomparams.layer.c_str(), true);
		m_currentStep = 1;
	}
	else
	{
		int currentStep = m_currentStep;
		int nextStep = currentStep-1;

		if (nextStep < 1)
			return false;
		else
		{
			float oFoV = GetZoomFoVScale(currentStep);
			float tFoV = GetZoomFoVScale(nextStep);

			ZoomIn(m_zoomparams.stage_time, oFoV, tFoV, true);

			m_currentStep = nextStep;
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------
bool CIronSight::IsZoomed() const
{
	return m_zoomed;
}

//------------------------------------------------------------------------
bool CIronSight::IsZooming() const
{
	return m_zoomTimer>0.0f;
}

//------------------------------------------------------------------------
void CIronSight::Enable(bool enable)
{
	m_enabled = enable;
}

//------------------------------------------------------------------------
bool CIronSight::IsEnabled() const
{
	return m_enabled;
}

//------------------------------------------------------------------------
struct CIronSight::EnterZoomAction
{
	EnterZoomAction(CIronSight *_ironSight): ironSight(_ironSight) {};
	CIronSight *ironSight;

	void execute(CItem *pWeapon)
	{
		pWeapon->SetBusy(false);
		pWeapon->PlayLayer(ironSight->m_zoomparams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
		pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, ironSight->m_actions.idle);
		
		ironSight->m_zoomed = true;
		ironSight->OnZoomedIn();
	}
};

void CIronSight::EnterZoom(float time, const char *zoom_layer, bool smooth, int zoomStep)
{
	ResetTurnOff();
	OnEnterZoom();
	SetActorSpeedScale(0.35f);

	// marcok: please leave cl_tryme alone
	if(!m_FarCry_style && !g_pGameCVars->cl_tryme)
		m_pWeapon->FadeCrosshair(1.0f, 0.0f, WEAPON_FADECROSSHAIR_ZOOM);

	float oFoV = GetZoomFoVScale(0);
	float tFoV = GetZoomFoVScale(zoomStep);

	ZoomIn(time, oFoV, tFoV, smooth);

	m_pWeapon->SetBusy(true);
	if(m_FarCry_style)
		m_pWeapon->SetActionSuffix(m_zoomparams.suffix_FC.c_str());
	else
		m_pWeapon->SetActionSuffix(m_zoomparams.suffix.c_str());
	m_pWeapon->PlayAction(m_actions.zoom_in, 0, false, CItem::eIPAF_Default);

	m_pWeapon->GetScheduler()->TimerAction((uint)(m_zoomparams.zoom_in_time*1000), CSchedulerAction<EnterZoomAction>::Create(this), false);
}

//------------------------------------------------------------------------
struct CIronSight::LeaveZoomAction
{
	LeaveZoomAction(CIronSight *_ironSight): ironSight(_ironSight) {};
	CIronSight *ironSight;

	void execute(CItem *pWeapon)
	{
		pWeapon->SetBusy(false);

		ironSight->m_zoomed = false;
		ironSight->OnZoomedOut();
	}
};

void CIronSight::LeaveZoom(float time, bool smooth)
{
	ResetTurnOff();
	OnLeaveZoom();
	SetActorSpeedScale(1.0f);

	// marcok: please leave cl_tryme alone
	if(!m_FarCry_style && !g_pGameCVars->cl_tryme)
		m_pWeapon->FadeCrosshair(0.0f, 1.0f, WEAPON_FADECROSSHAIR_ZOOM);

	float oFoV = GetZoomFoVScale(0);
	float tFoV = GetZoomFoVScale(1);

	ZoomOut(time, tFoV, oFoV, smooth);

	m_pWeapon->SetBusy(true);
	
	m_pWeapon->StopLayer(m_zoomparams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	m_pWeapon->PlayAction(m_actions.zoom_out, 0, false, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	
	m_pWeapon->SetActionSuffix("");
	m_pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, g_pItemStrings->idle);
	m_currentStep = 0;

	m_pWeapon->GetScheduler()->TimerAction((uint)(m_zoomparams.zoom_in_time*1000), CSchedulerAction<LeaveZoomAction>::Create(this), false);
}

//------------------------------------------------------------------------
void CIronSight::ResetTurnOff()
{
	static ItemString idle = "idle";
	m_savedFoVScale = 0.0f;
	m_pWeapon->StopLayer(m_zoomparams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	m_pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, idle);
}

//------------------------------------------------------------------------
struct CIronSight::DisableTurnOffAction
{
	DisableTurnOffAction(CIronSight *_ironSight): ironSight(_ironSight) {};
	CIronSight *ironSight;

	void execute(CItem *pWeapon)
	{
		pWeapon->PlayLayer(ironSight->m_zoomparams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
		if(ironSight->m_FarCry_style)
			pWeapon->SetActionSuffix(ironSight->m_zoomparams.suffix_FC.c_str());
		else
			pWeapon->SetActionSuffix(ironSight->m_zoomparams.suffix.c_str());
		pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, ironSight->m_actions.idle);
		
		ironSight->m_zoomed = true;
		ironSight->OnZoomedIn();

		pWeapon->SetBusy(false);
	}
};

struct CIronSight::EnableTurnOffAction
{
	EnableTurnOffAction(CIronSight *_ironSight): ironSight(_ironSight) {};
	CIronSight *ironSight;

	void execute(CItem *pWeapon)
	{
		ironSight->m_zoomed = false;
		ironSight->OnZoomedOut();
	}
};

void CIronSight::TurnOff(bool enable, bool smooth, bool anim)
{
	if (!enable && (m_savedFoVScale > 0.0f))
	{
		OnEnterZoom();
	
		float oFoV = GetZoomFoVScale(0);
		float tFoV = m_savedFoVScale;

		SetActorSpeedScale(0.35f);

		ZoomIn(m_zoomparams.zoom_out_time, oFoV, tFoV, smooth);

		if (anim)
		{
			if(m_FarCry_style)
				m_pWeapon->SetActionSuffix(m_zoomparams.suffix_FC.c_str());
			else
				m_pWeapon->SetActionSuffix(m_zoomparams.suffix.c_str());
			m_pWeapon->PlayAction(m_actions.zoom_in);
		}

		m_pWeapon->GetScheduler()->TimerAction((uint)(m_zoomparams.zoom_out_time*1000), CSchedulerAction<DisableTurnOffAction>::Create(this), false);
		m_savedFoVScale = 0.0f;
	}
	else if (m_zoomed && enable)
	{
		m_pWeapon->SetBusy(true);
		m_savedFoVScale = GetActorFoVScale();

		OnLeaveZoom();

		float oFoV = GetZoomFoVScale(0);
		float tFoV = m_savedFoVScale;

		SetActorSpeedScale(1);
		ZoomOut(m_zoomparams.zoom_out_time, tFoV, oFoV, smooth);

		m_pWeapon->StopLayer(m_zoomparams.layer, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
		m_pWeapon->SetDefaultIdleAnimation(CItem::eIGS_FirstPerson, g_pItemStrings->idle);

		if (anim)
		{
			m_pWeapon->PlayAction(m_actions.zoom_out);
			m_pWeapon->SetActionSuffix("");
		}

		m_pWeapon->GetScheduler()->TimerAction((uint)(m_zoomparams.zoom_out_time*1000), CSchedulerAction<EnableTurnOffAction>::Create(this), false);
	}
}

//------------------------------------------------------------------------
void CIronSight::ZoomIn(float time, float from, float to, bool smooth)
{
	m_zoomTime = time;
	m_zoomTimer = m_zoomTime;
	m_startFoV = from;
	m_endFoV = to;
	m_smooth = smooth;

	float totalFoV = abs(m_endFoV-m_startFoV);
	float ownerFoV = GetActorFoVScale();

	m_startFoV = ownerFoV;
	
	float deltaFoV = abs(m_endFoV-m_startFoV)/totalFoV;
	
	if (deltaFoV < totalFoV)
	{
		m_zoomTime = (deltaFoV/totalFoV)*time;
		m_zoomTimer = m_zoomTime;
	}

	m_zoomingIn = true;

	m_initialNearFov = *(float*)gEnv->pRenderer->EF_Query(EFQ_DrawNearFov);

	m_pWeapon->RequireUpdate(eIUS_Zooming);
}

//------------------------------------------------------------------------
void CIronSight::ZoomOut(float time, float from, float to, bool smooth)
{

	m_zoomTimer = time;
	m_zoomTime = m_zoomTimer;

	m_startFoV = from;
	m_endFoV = to;
	m_smooth = smooth;


	float totalFoV = abs(m_endFoV-m_startFoV);
	float ownerFoV = GetActorFoVScale();

	m_startFoV = ownerFoV;

	float deltaFoV = abs(m_endFoV-m_startFoV);

	if (deltaFoV < totalFoV)
	{
		m_zoomTime = (deltaFoV/totalFoV)*time;
		m_zoomTimer = m_zoomTime;
	}

	m_zoomingIn = false;

	if(m_zoomparams.reflex_aimDot)
	{
		HideReflexDot(true);
		m_reflexDotOnTimer = REFLEX_DOT_ON_TIMER;
	}

	m_pWeapon->RequireUpdate(eIUS_Zooming);
}

//------------------------------------------------------------------------
void CIronSight::OnEnterZoom()
{
	if (g_pGameCVars->g_dof_ironsight != 0)
	{
		if (!m_zoomparams.dof_mask.empty())
		{
			gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 1);
			gEnv->p3DEngine->SetPostEffectParamString("Dof_MaskTexName", m_zoomparams.dof_mask);
		}
		else
		{
			gEnv->p3DEngine->SetPostEffectParam("Dof_UseMask", 0);
		}
		gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1);
		gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1);
	}
}

//------------------------------------------------------------------------
void CIronSight::OnZoomedIn()
{
	if (!m_zoomparams.dof_mask.empty())
	{
		m_focus = 1.0f;

		gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", -1.0f);
		gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", 1.0f);
		gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1);
	}

	if(m_zoomparams.reflex_aimDot)
		HideReflexDot(false);

	//Patch spread and recoil modifications
	IFireMode* pFireMode = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode());

	if(pFireMode)
	{
		pFireMode->PatchSpreadMod(m_spreadModParams);
		pFireMode->PatchRecoilMod(m_recoilModParams);
	}

}

//------------------------------------------------------------------------
void CIronSight::OnLeaveZoom()
{
}

//------------------------------------------------------------------------
void CIronSight::OnZoomedOut()
{
	ClearDoF();
	m_pWeapon->ExitViewmodes();
	CActor *pActor = m_pWeapon->GetOwnerActor();
	if (pActor && pActor->GetScreenEffects() != 0)
	{
		pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomOutID);
		pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_autoZoomInID);
		pActor->GetScreenEffects()->ClearBlendGroup(pActor->m_hitReactionID);
		pActor->GetScreenEffects()->EnableBlends(true, pActor->m_autoZoomInID);
		pActor->GetScreenEffects()->EnableBlends(true, pActor->m_autoZoomOutID);
		pActor->GetScreenEffects()->EnableBlends(true, pActor->m_hitReactionID);
	}

	//Reset spread and recoil modifications
	IFireMode* pFireMode = m_pWeapon->GetFireMode(m_pWeapon->GetCurrentFireMode());

	if(pFireMode)
	{
		pFireMode->ResetSpreadMod();
		pFireMode->ResetRecoilMod();
	}

	if(m_zoomparams.scope_mode)
		ResetFovAndPosition();

}

//------------------------------------------------------------------------
void CIronSight::OnZoomStep(bool zoomingIn, float t)
{
	m_focus = 0.0f;
}

//------------------------------------------------------------------------
void CIronSight::UpdateDepthOfField(float frameTime, float t)
{
	CActor* pActor = m_pWeapon->GetOwnerActor();
	if (pActor && pActor->IsClient() && pActor->GetActorClass() == CPlayer::GetActorClassType())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
		if (IMovementController *pMV = pActor->GetMovementController())
		{
			SMovementState ms;
			pMV->GetMovementState(ms);
			Vec3 start = ms.eyePosition;
			Vec3 dir = ms.eyeDirection;
			static ray_hit hit;	

			IPhysicalEntity* pSkipEntities[10];
			int nSkip = CSingle::GetSkipEntities(m_pWeapon, pSkipEntities, 10);
			// jitter the direction (non-uniform disk sampling ... we want to bias the center in this case)
			f32 cosTheta, sinTheta;
			f32 theta = Random() * gf_PI2;
			f32 spreadAngle = DEG2RAD(g_pGameCVars->g_dof_sampleAngle)/2.0f;
			f32 scale = tan_tpl(spreadAngle);
			f32 radiusSqrt = scale * Random();
			sincos_tpl(theta, &cosTheta, &sinTheta);
			f32 x = radiusSqrt * cosTheta;
			f32 y = radiusSqrt * sinTheta;
			
			Matrix33 viewRotation(pPlayer->GetViewQuatFinal());

			Vec3 xOff = x * viewRotation.GetColumn0();
			Vec3 yOff = y * viewRotation.GetColumn2();
			
			// jitter
			if (true)
			{
				dir += xOff + yOff;
				dir.Normalize();
			}
			const static float minRelaxSpeed = 10.0f;
			const static float maxRelaxSpeed = 1.0f;

			f32 delta;

			if (gEnv->pPhysicalWorld->RayWorldIntersection(start, 1000.0f*dir, ent_all,
				rwi_stop_at_pierceable|rwi_ignore_back_faces, &hit, 1, pSkipEntities, nSkip))
			{
				delta = g_pGameCVars->g_dof_minHitScale*hit.dist - m_minDoF;
				Limit(delta, -g_pGameCVars->g_dof_minAdjustSpeed, g_pGameCVars->g_dof_minAdjustSpeed);
				//delta *= fabs(delta/minAdjustSpeed);
				m_minDoF += delta * frameTime;
				
				delta = g_pGameCVars->g_dof_maxHitScale*hit.dist - m_maxDoF;
				Limit(delta, -g_pGameCVars->g_dof_maxAdjustSpeed, g_pGameCVars->g_dof_maxAdjustSpeed);
				//delta *= fabs(delta/maxAdjustSpeed);
				m_maxDoF += delta * frameTime;
			}
			if (m_maxDoF - g_pGameCVars->g_dof_distAppart < m_minDoF)
			{
				m_maxDoF = m_minDoF + g_pGameCVars->g_dof_distAppart;
			}
			else
			{
				// relax max to min
				delta = m_minDoF - m_maxDoF;
				Limit(delta, -maxRelaxSpeed, maxRelaxSpeed);
				//delta *= fabs(delta/maxRelaxSpeed);
				m_maxDoF += delta * frameTime;
			}
			// the average is relaxed to the center between min and max
			m_averageDoF = (m_maxDoF - m_minDoF)/2.0f;
			Limit(delta, -g_pGameCVars->g_dof_averageAdjustSpeed, g_pGameCVars->g_dof_averageAdjustSpeed);
			//delta *= fabs(delta/averageAdjustSpeed);
			m_averageDoF += delta * frameTime;
		}
	}
}
//------------------------------------------------------------------------
void CIronSight::Serialize(TSerialize ser)
{
	if(ser.GetSerializationTarget() != eST_Network)
	{
		ser.Value("m_savedFoVScale", m_savedFoVScale);
		ser.Value("m_zoomed", m_zoomed);
		ser.Value("m_zoomingIn", m_zoomingIn);
		ser.Value("m_zoomTimer", m_zoomTimer);
		ser.Value("m_zoomTime", m_zoomTime);
		ser.Value("m_focus", m_focus);
		ser.Value("m_recoilScale", m_recoilScale);
		ser.Value("m_startFoV", m_startFoV);
		ser.Value("m_endFoV", m_endFoV);
		ser.Value("m_smooth", m_smooth);
		ser.Value("m_currentStep", m_currentStep);
		ser.Value("m_enabled", m_enabled);
		ser.Value("m_minDoF", m_minDoF);
		ser.Value("m_maxDoF", m_maxDoF);
		ser.Value("m_averageDoF", m_averageDoF);

		if(ser.IsReading() && m_zoomed && m_enabled)
			SetActorFoVScale(m_endFoV, true, true, true);
	}
}

//------------------------------------------------------------------------
void CIronSight::SetActorFoVScale(float fovScale, bool sens,bool recoil, bool hbob)
{
	if (!m_pWeapon->GetOwnerActor())
		return;

	SActorParams *pActorParams = m_pWeapon->GetOwnerActor()->GetActorParams();
	if (!pActorParams)
		return;

	pActorParams->viewFoVScale = fovScale;

	if (sens)
		pActorParams->viewSensitivity = GetSensitivityFromFoVScale(fovScale);

	if (hbob)
	{
		float mult = GetHBobFromFoVScale(fovScale);
		pActorParams->weaponInertiaMultiplier = mult;
		pActorParams->weaponBobbingMultiplier = mult;
		pActorParams->headBobbingMultiplier = mult;
	}
	

}

//------------------------------------------------------------------------
float CIronSight::GetActorFoVScale() const
{
	if (!m_pWeapon->GetOwnerActor())
		return 1.0f;

	SActorParams *pActorParams = m_pWeapon->GetOwnerActor()->GetActorParams();
	if (!pActorParams)
		return 1.0f;

	return pActorParams->viewFoVScale;
}


//------------------------------------------------------------------------
void CIronSight::SetActorSpeedScale(float scale)
{
	if (!m_pWeapon->GetOwnerActor())
		return;

	SPlayerParams *pActorParams = static_cast<SPlayerParams *>(m_pWeapon->GetOwnerActor()->GetActorParams());
	if (!pActorParams)
		return;

	pActorParams->speedMultiplier = scale;
}

//------------------------------------------------------------------------
float CIronSight::GetActorSpeedScale() const
{
	if (!m_pWeapon->GetOwnerActor())
		return 1.0f;

	SPlayerParams *pActorParams = static_cast<SPlayerParams *>(m_pWeapon->GetOwnerActor()->GetActorParams());
	if (!pActorParams)
		return 1.0f;

	return pActorParams->speedMultiplier;
}

//------------------------------------------------------------------------
float CIronSight::GetSensitivityFromFoVScale(float scale) const
{
	float mag = GetMagFromFoVScale(scale);
	if (mag<=0.99f)
		return 1.0f;

	return 1.0f/(mag*m_zoomparams.sensitivity_ratio);
}

//------------------------------------------------------------------------
float CIronSight::GetHBobFromFoVScale(float scale) const
{
	float mag = GetMagFromFoVScale(scale);
	if (mag<=1.001f)
		return 1.0f;

	return 1.0f/(mag*m_zoomparams.hbob_ratio);
}

//------------------------------------------------------------------------
float CIronSight::GetRecoilFromFoVScale(float scale) const
{
	float mag = GetMagFromFoVScale(scale);
	if (mag<=1.001f)
		return 1.0f;

	return 1.0f/(mag*m_zoomparams.recoil_ratio);
}

//------------------------------------------------------------------------
float CIronSight::GetMagFromFoVScale(float scale) const
{
	assert(scale>0.0f);
	return 1.0f/scale;
}

//------------------------------------------------------------------------
float CIronSight::GetFoVScaleFromMag(float mag) const
{
	assert(mag>0.0f);
	if (mag >= 1.0f)
		return 1.0f/mag;
	else
		return 1.0f;
}

//------------------------------------------------------------------------
float CIronSight::GetZoomFoVScale(int step) const
{
	if (!step)
		return 1.0f;

	return 1.0f/m_zoomparams.stages[step-1];
}

//------------------------------------------------------------------------
void CIronSight::ClearDoF()
{
	gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0.0f);
}

//------------------------------------------------------------------------
void CIronSight::ExitZoom()
{
	if (m_zoomed || m_zoomTime>0.0f)
	{
		LeaveZoom(m_zoomparams.zoom_out_time, true);
		m_currentStep = 0;
	}
}

//-------------------------------------------------------------------------
void CIronSight::CheckFarCryIronSight()
{
	m_FarCry_style = false;
	if(m_zoomparams.support_FC_IronSight && (g_pGameCVars->g_enableAlternateIronSight!=0))
		m_FarCry_style = true;
}

//-------------------------------------------------------------------------
void CIronSight::AdjustScopePosition(float time, bool zoomIn)
{
	Vec3 offset;

	if(zoomIn)
	{
		offset = m_zoomparams.scope_offset * time;
	}
	else
	{
		offset = m_zoomparams.scope_offset - m_zoomparams.scope_offset*time;
		if(time>=1.0f)
			offset.zero();
	}

	g_pGameCVars->i_offset_right = offset.x;
	g_pGameCVars->i_offset_front = offset.y;
	g_pGameCVars->i_offset_up = offset.z;
}

//-------------------------------------------------------------------------
void CIronSight::AdjustNearFov(float time, bool zoomIn)
{
	float newFov;
	if(zoomIn)
	{
		newFov = (m_initialNearFov*(1.0f-time))+(m_zoomparams.scope_nearFov*time);
	}
	else
	{
		newFov = (m_initialNearFov*time)+(m_zoomparams.scope_nearFov*(1.0f-time));
		if(time>1.0f)
			newFov = m_initialNearFov;
	}
	gEnv->pRenderer->EF_Query(EFQ_DrawNearFov,(INT_PTR)&newFov);
}

//------------------------------------------------------------------------
void CIronSight::ResetFovAndPosition()
{
	if(m_pWeapon->GetOwnerActor() && m_pWeapon->GetOwnerActor()->IsClient())
	{
		AdjustScopePosition(1.1f,false);
		AdjustNearFov(1.1f,false);
	}
}
//-------------------------------------------------------------------------
void CIronSight::CreateReflexDot()
{

	if (!m_reflexDotEntity)
	{
		SEntitySpawnParams spawnParams;
		spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
		spawnParams.sName = "";
		spawnParams.nFlags |= ENTITY_FLAG_CLIENT_ONLY;

		IEntity *pNewEntity =gEnv->pEntitySystem->SpawnEntity(spawnParams);
		assert(pNewEntity);

		pNewEntity->FreeSlot(0);
		pNewEntity->FreeSlot(1);

		pNewEntity->LoadGeometry(-1, m_zoomparams.reflex_dotEffect.c_str());

		int nslots=pNewEntity->GetSlotCount();
		for (int i=0;i<nslots;i++)
		{
			if (pNewEntity->GetSlotFlags(i)&ENTITY_SLOT_RENDER)
			{
					pNewEntity->SetSlotFlags(i, pNewEntity->GetSlotFlags(i)|ENTITY_SLOT_RENDER_NEAREST);
			}
		}

		m_reflexDotEntity = pNewEntity->GetId();
	}
}

//-------------------------------------------------------------------------
void CIronSight::DestroyReflexDot()
{
	if (m_reflexDotEntity)
		gEnv->pEntitySystem->RemoveEntity(m_reflexDotEntity);
	m_reflexDotEntity = 0;
}

//-----------------------------------------------------------------------
void CIronSight::HideReflexDot(bool hide)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_reflexDotEntity);
	if(pEntity)
	{
		pEntity->Hide(hide);
	}
}

//------------------------------------------------------------------------
void CIronSight::UpdateReflexDot(float frameTime)
{

	//Delay a little bit the update when zooming in
	if(m_reflexDotOnTimer>0.0f)
		m_reflexDotOnTimer-=frameTime;

	if(m_reflexDotEntity)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_reflexDotEntity);

		if(pEntity)
		{

			Vec3 finalPos;
			finalPos.zero();

			CActor *pActor = m_pWeapon->GetOwnerActor();
			IMovementController * pMC = pActor ? pActor->GetMovementController() : NULL;
			if (pMC)
			{ 
				SMovementState info;
				pMC->GetMovementState(info);

				Vec3 pos = m_pWeapon->GetSlotHelperPos(CItem::eIGS_FirstPerson,"attachment_top",true);
				Vec3 dir = -m_pWeapon->GetSlotHelperRotation(CItem::eIGS_FirstPerson,"attachment_top",true).GetColumn0();
			
				float dotProduct = info.fireDirection.Dot(dir);

				if(m_reflexDotOnTimer>0.0f && dotProduct<0.9999f)
				{
					HideReflexDot(true);
					return;
				}

				finalPos = pos  + (dir*7.0f);
				if(dotProduct<0.999827f)
					HideReflexDot(true);					
			}	

			Matrix34 tm;
			tm.SetIdentity();
			tm.SetTranslation(finalPos);
			pEntity->SetWorldTM(tm);

		}
	}
}

//-------------------------------------------------------------------------
void CIronSight::UpdateFPView(float frameTime)
{
	if(m_zoomparams.reflex_aimDot && m_reflexDotEntity && IsZoomed() && !IsZooming())
	{
		HideReflexDot(false);
		UpdateReflexDot(frameTime);
	}
}

//-------------------------------------------------------------------------
void CIronSight::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	m_zoomparams.GetMemoryStatistics(s);
	m_actions.GetMemoryStatistics(s);
	m_spreadModParams.GetMemoryStatistics(s);
	m_recoilModParams.GetMemoryStatistics(s);
}

