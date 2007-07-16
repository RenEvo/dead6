// Flashlight Attachment
// John Newfield

#include "StdAfx.h"
#include "Flashlight.h"
#include "Actor.h"



bool CFlashlight::Init(IGameObject * pGameObject )
{
	if (!CItem::Init(pGameObject))
		return false;

	m_lightID = 0;
	m_lightID_tp = 0;
	m_activated = false;

	return true;
}

void CFlashlight::PostInit(IGameObject * pGameObject )
{
	CItem::PostInit(pGameObject);

	m_lightID = 0;
	m_lightID_tp = 0;
	m_activated = false;
}

void CFlashlight::OnReset()
{
	CItem::OnReset();

	m_lightID = 0;
	m_lightID_tp = 0;
	m_activated = false;
}

void CFlashlight::Update(SEntityUpdateContext& ctx, int slot)
{
	CItem::Update(ctx, slot);
	
	if (slot!=eIUS_General)
		return;

	if (!m_activated)
		return;
	
	IEntity *parent = gEnv->pEntitySystem->GetEntity(GetParentId());
	//if (parent)
		//CryLogAlways("parent: %s", parent->GetName());

	IItem *owner = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetParentId());
	IWeapon *wep = 0;
	if (owner)
		wep = owner->GetIWeapon();
	EntityId playerID = ~0;
	if (wep)
		playerID = owner->GetOwnerId();



	IActor *pOwner = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerID);
	if (!pOwner)
		return;
/*	else
	{
		if(!pOwner->IsPlayer())		//For AI is deactivated for now - Beni
			return;
	}
*/
	Vec3 color = Vec3(1,1,1);
	float specular = 1.0f/6.0f;
	color *= 6;
	Vec3 angles_offset = Vec3(0, 0, g_PI);
	Vec3 pos_offset = Vec3(-.15f, 0, 0);
	CItem *clight = (CItem *)owner;

	//We need to use the params->attach_helper or the Shotgun would not work - Beni
	if (m_lightID_tp == 0 && pOwner->IsThirdPerson())
	{
		SAccessoryParams *params=clight->GetAccessoryParams(GetEntity()->GetClass()->GetName());
		if (!params)
			return;

		string helper(params->attach_helper.c_str());
		helper.append("_light");

		m_lightID_tp = clight->AttachLight(eIGS_ThirdPerson, 0, true, false, 50.0f, color, specular, "textures/lights/flashlight1.dds", 40.0f, params->attach_helper.c_str(), Vec3(0,0,0), Vec3(-1,0,0));
		clight->AttachLight(eIGS_FirstPerson, m_lightID, false, false, 50.0f, color, specular, "textures/lights/flashlight1.dds", 40.0f, helper.c_str());
		m_lightID = 0;
	}
	if (m_lightID == 0 && !pOwner->IsThirdPerson())
	{
		SAccessoryParams *params=clight->GetAccessoryParams(GetEntity()->GetClass()->GetName());
		if (!params)
			return;

		string helper(params->attach_helper.c_str());
		helper.append("_light");

		m_lightID = clight->AttachLight(eIGS_FirstPerson, 0, true,false, 50.0f, color, specular, "textures/lights/flashlight1.dds", 40.0f, helper.c_str(), Vec3(0,0,0), Vec3(0,-1,0));
		clight->AttachLight(eIGS_ThirdPerson, m_lightID_tp, false,false, 50.0f, color, specular, "textures/lights/flashlight1.dds", 40.0f, params->attach_helper.c_str());
		m_lightID_tp = 0;
	}

	RequireUpdate(eIUS_General);
}

void CFlashlight::ActivateLight(bool activate)
{
	IItem *owner = m_pItemSystem->GetItem(GetParentId());
	CItem *clight = (CItem *)owner;

	m_activated = activate;

	if (!m_activated)
	{
		SAccessoryParams *params=clight->GetAccessoryParams(GetEntity()->GetClass()->GetName());
		if (!params)
			return;

		string helper(params->attach_helper.c_str());
		helper.append("_light");

		if (m_lightID != 0)
			clight->AttachLight(eIGS_FirstPerson, m_lightID, false, false, 50.0f, Vec3(0,0,0), 0, "textures/lights/flashlight1.dds", 40.0f, helper.c_str(), Vec3(0,0,0), Vec3(0,-1,0));
		if (m_lightID_tp != 0)
			clight->AttachLight(eIGS_ThirdPerson, m_lightID_tp, false, false, 50.0f, Vec3(0,0,0), 0, "textures/lights/flashlight1.dds", 40.0f, helper.c_str());
		m_lightID = 0;
		m_lightID_tp = 0;

		EnableUpdate(false, eIUS_General);
	}
	else
		RequireUpdate(eIUS_General);
}

void CFlashlight::OnAttach(bool attach)
{
	ActivateLight(attach);
}

void CFlashlight::OnParentSelect(bool select)
{
	ActivateLight(select);
}