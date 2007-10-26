////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowBuildingControllerNodes.cpp
//
// Purpose: Flow Nodes for Building Controllers
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Nodes/FlowBuildingControllerNodes.h"

////////////////////////////////////////////////////
CBuildingControllerGeneralListenerNode::CBuildingControllerGeneralListenerNode(SActivationInfo * pActInfo)
{
	m_nGUID = GUID_INVALID;
}

////////////////////////////////////////////////////
CBuildingControllerGeneralListenerNode::~CBuildingControllerGeneralListenerNode()
{
	IBaseManager *pBaseManager = g_D6Core->pBaseManager;
	if (NULL != pBaseManager)
		pBaseManager->RemoveBuildingControllerEventListener(m_nGUID, this);
}

////////////////////////////////////////////////////
void CBuildingControllerGeneralListenerNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_nGUID", m_nGUID);
	if (ser.IsReading())
	{
		AddListener(pActInfo);
	}
}

////////////////////////////////////////////////////
void CBuildingControllerGeneralListenerNode::AddListener(SActivationInfo *pActInfo)
{
	// Get base manager
	IBaseManager *pBaseManager = g_D6Core->pBaseManager;
	if (NULL == pBaseManager)
	{
		GameWarning("[Flow] CBuildingControllerGeneralListenerNode::AddListener: No Base Manager!");
		return;
	}

	// Get GUID info if not set
	if (GUID_INVALID == m_nGUID)
	{
		const string& szTeam = GetPortString(pActInfo, EIP_Team);
		const string& szClass = GetPortString(pActInfo, EIP_Class);
		m_nGUID = pBaseManager->GenerateGUID(szTeam.c_str(), szClass.c_str());
	}

	// Add listener
	bool bEnable = GetPortBool(pActInfo, EIP_Enable);
	if (bEnable)
		pBaseManager->AddBuildingControllerEventListener(m_nGUID, this);
	else
		pBaseManager->RemoveBuildingControllerEventListener(m_nGUID, this);
}

////////////////////////////////////////////////////
void CBuildingControllerGeneralListenerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig<bool>("Enable", true, _HELP("Turn listening on/off")),
		InputPortConfig<string>("Team", _HELP("Team name")),
		InputPortConfig<string>("Class", _HELP("Class name")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Validated", _HELP("Triggered when controller has been validated")),
		OutputPortConfig_Void("Reset", _HELP("Triggered when controller has been reset")),
		OutputPortConfig<bool>("PowerChanged", _HELP("Triggered when controller's power status changed. Sends TRUE if power is online, FALSE if offline."), _HELP("Power Changed")),
		OutputPortConfig_Void("Destroyed", _HELP("Triggered when controller is destroyed")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Used to listen for general events from a Building Controller");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CBuildingControllerGeneralListenerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			m_actInfo = *pActInfo;
			AddListener(pActInfo);
		};
		break;

		case eFE_Activate:
		{
			if (IsPortActive(pActInfo, EIP_Enable))
			{
				AddListener(pActInfo);
			}
		}
		break;
	};
}

////////////////////////////////////////////////////
void CBuildingControllerGeneralListenerNode::OnBuildingControllerEvent(IBuildingController *pController,
																BuildingGUID nGUID, SControllerEvent &event)
{
	// Handle events
	switch (event.event)
	{
		case CONTROLLER_EVENT_VALIDATED:
		{
			ActivateOutput(&m_actInfo, EOP_Validated, true);
		}
		break;
		case CONTROLLER_EVENT_RESET:
		{
			ActivateOutput(&m_actInfo, EOP_Reset, true);
		}
		break;
		case CONTROLLER_EVENT_POWER:
		{
			ActivateOutput(&m_actInfo, EOP_PowerChanged, (bool)(event.nParam[0] ? true : false));
		}
		break;
		case CONTROLLER_EVENT_DESTROYED:
		{
			ActivateOutput(&m_actInfo, EOP_Destroyed, true);
		}
		break;
	}
}



////////////////////////////////////////////////////
CBuildingControllerDamageListenerNode::CBuildingControllerDamageListenerNode(SActivationInfo * pActInfo)
{
	m_nGUID = GUID_INVALID;
}

////////////////////////////////////////////////////
CBuildingControllerDamageListenerNode::~CBuildingControllerDamageListenerNode()
{
	IBaseManager *pBaseManager = g_D6Core->pBaseManager;
	if (NULL != pBaseManager)
		pBaseManager->RemoveBuildingControllerEventListener(m_nGUID, this);
}

////////////////////////////////////////////////////
void CBuildingControllerDamageListenerNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_nGUID", m_nGUID);
	if (ser.IsReading())
	{
		AddListener(pActInfo);
	}
}

////////////////////////////////////////////////////
void CBuildingControllerDamageListenerNode::AddListener(SActivationInfo *pActInfo)
{
	// Get base manager
	IBaseManager *pBaseManager = g_D6Core->pBaseManager;
	if (NULL == pBaseManager)
	{
		GameWarning("[Flow] CBuildingControllerDamageListenerNode::AddListener: No Base Manager!");
		return;
	}

	// Get GUID info if not set
	if (GUID_INVALID == m_nGUID)
	{
		const string& szTeam = GetPortString(pActInfo, EIP_Team);
		const string& szClass = GetPortString(pActInfo, EIP_Class);
		m_nGUID = pBaseManager->GenerateGUID(szTeam.c_str(), szClass.c_str());
	}

	// Add listener
	bool bEnable = GetPortBool(pActInfo, EIP_Enable);
	if (bEnable)
		pBaseManager->AddBuildingControllerEventListener(m_nGUID, this);
	else
		pBaseManager->RemoveBuildingControllerEventListener(m_nGUID, this);
}

////////////////////////////////////////////////////
void CBuildingControllerDamageListenerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig<bool>("Enable", true, _HELP("Turn listening on/off")),
		InputPortConfig<string>("Team", _HELP("Team name")),
		InputPortConfig<string>("Class", _HELP("Class name")),
		InputPortConfig<bool>("Hits", true, _HELP("TRUE to listen for weapon hits (non-explosions)")),
		InputPortConfig<bool>("Explosions", true, _HELP("TRUE to listen for explosions")),
		InputPortConfig<EntityId>("ShooterId", _HELP("When connected, limit HitInfo to this shooter")),
		InputPortConfig<EntityId>("TargetId", _HELP("When connected, limit HitInfo to this target")),
		InputPortConfig<string>("Weapon", _HELP("When set, limit HitInfo to this weapon"), 0, _UICONFIG("enum_global:weapon")),
		InputPortConfig<string>("Ammo", _HELP("When set, limit HitInfo to this ammo"), 0, _UICONFIG("enum_global:ammos")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("OnDamage", _HELP("Triggered when damage has occured")),
		OutputPortConfig<EntityId>("ShooterId", _HELP("EntityID of the Shooter")),
		OutputPortConfig<EntityId>("TargetId", _HELP("EntityID of the Target")),
		OutputPortConfig<EntityId>("WeaponId", _HELP("EntityID of the Weapon")),
		OutputPortConfig<EntityId>("ProjectileId",  _HELP("EntityID of the Projectile if it was a bullet hit")),
		OutputPortConfig<Vec3>("HitPos", _HELP("Position of the Hit")),
		OutputPortConfig<Vec3>("HitDir", _HELP("Direction of the Hit")),
		OutputPortConfig<Vec3>("HitNormal", _HELP("Normal of the Hit Impact")),
		OutputPortConfig<string>("HitType", _HELP("Name of the HitType")),
		OutputPortConfig<float>("Damage", _HELP("Damage amout which was caused")),
		OutputPortConfig<string>("Material", _HELP("Name of the Material")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Used to listen for damage-specific events from a Building Controller");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CBuildingControllerDamageListenerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			m_actInfo = *pActInfo;
			AddListener(pActInfo);
		};
		break;

		case eFE_Activate:
		{
			if (IsPortActive(pActInfo, EIP_Enable))
			{
				AddListener(pActInfo);
			}
		}
		break;
	};
}

////////////////////////////////////////////////////
void CBuildingControllerDamageListenerNode::OnBuildingControllerEvent(IBuildingController *pController,
																BuildingGUID nGUID, SControllerEvent &event)
{
	// Handle events
	switch (event.event)
	{
		case CONTROLLER_EVENT_ONHIT:
		{
			// Can we process hits?
			if (false == GetPortBool(&m_actInfo, EIP_Hits)) break;
			
			// Get hit info
			HitInfo *pHitInfo = (HitInfo*)event.nParam[0];
			if (NULL == pHitInfo) break;

			// Check limits
			IEntity* pTempEntity;
			EntityId nLimitShooterID = GetPortEntityId(&m_actInfo, EIP_ShooterId);
			EntityId nLimitTargetID = GetPortEntityId(&m_actInfo, EIP_TargetId);
			const string& weapon = GetPortString(&m_actInfo, EIP_Weapon);
			const string& ammo = GetPortString(&m_actInfo, EIP_Ammo);
			if (0 != nLimitShooterID && nLimitShooterID != pHitInfo->shooterId) break;
			if (0 != nLimitTargetID && nLimitTargetID != pHitInfo->targetId) break;
			if (false == weapon.empty())
			{
				pTempEntity = gEnv->pEntitySystem->GetEntity(pHitInfo->weaponId);
				if (NULL == pTempEntity || 0 != weapon.compare(pTempEntity->GetClass()->GetName()))
					break;
			}
			if (false == ammo.empty())
			{
				pTempEntity = gEnv->pEntitySystem->GetEntity(pHitInfo->projectileId);
				if (NULL == pTempEntity || 0 != ammo.compare(pTempEntity->GetClass()->GetName()))
					break;
			}

			// Get values
			ISurfaceType* pSurface = g_pGame->GetGameRules()->GetHitMaterial(pHitInfo->material);
			char const* szHitType = "";
			if (CGameRules* pGR = g_pGame->GetGameRules())
				szHitType = pGR->GetHitType(pHitInfo->type);
			
			// Activate the outputs
			ActivateOutput(&m_actInfo, EOP_OnDamage, true);
			ActivateOutput(&m_actInfo, EOP_ShooterId, pHitInfo->shooterId);
			ActivateOutput(&m_actInfo, EOP_TargetId, pHitInfo->targetId);
			ActivateOutput(&m_actInfo, EOP_WeaponId, pHitInfo->weaponId);
			ActivateOutput(&m_actInfo, EOP_ProjectileId, pHitInfo->projectileId);
			ActivateOutput(&m_actInfo, EOP_HitPos, pHitInfo->pos);
			ActivateOutput(&m_actInfo, EOP_HitDir, pHitInfo->dir);
			ActivateOutput(&m_actInfo, EOP_HitNormal, pHitInfo->normal);
			ActivateOutput(&m_actInfo, EOP_Damage, pHitInfo->damage);
			ActivateOutput(&m_actInfo, EOP_Material, string(pSurface ? pSurface->GetName() : ""));
			ActivateOutput(&m_actInfo, EOP_HitType, string(szHitType));
		}
		break;
		case CONTROLLER_EVENT_ONEXPLOSION:
		{
			// Can we process explosions?
			if (false == GetPortBool(&m_actInfo, EIP_Explosions)) break;

			// Get hit info
			ExplosionInfo *pHitInfo = (ExplosionInfo*)event.nParam[0];
			if (NULL == pHitInfo) break;

			// Check limits
			IEntity* pTempEntity;
			EntityId nLimitShooterID = GetPortEntityId(&m_actInfo, EIP_ShooterId);
			const string& weapon = GetPortString(&m_actInfo, EIP_Weapon);
			if (0 != nLimitShooterID && nLimitShooterID != pHitInfo->shooterId) break;
			if (false == weapon.empty())
			{
				pTempEntity = gEnv->pEntitySystem->GetEntity(pHitInfo->weaponId);
				if (NULL == pTempEntity || 0 != weapon.compare(pTempEntity->GetClass()->GetName()))
					break;
			}
			
			// Activate the outputs
			ActivateOutput(&m_actInfo, EOP_OnDamage, true);
			ActivateOutput(&m_actInfo, EOP_ShooterId, pHitInfo->shooterId);
			ActivateOutput(&m_actInfo, EOP_WeaponId, pHitInfo->weaponId);
			ActivateOutput(&m_actInfo, EOP_HitPos, pHitInfo->pos);
			ActivateOutput(&m_actInfo, EOP_HitDir, pHitInfo->dir);
			ActivateOutput(&m_actInfo, EOP_HitNormal, pHitInfo->impact_normal);
			ActivateOutput(&m_actInfo, EOP_Damage, pHitInfo->damage);
			ActivateOutput(&m_actInfo, EOP_HitType, string("Explosion"));
		}
		break;
	}
}


////////////////////////////////////////////////////
CBuildingControllerViewListenerNode::CBuildingControllerViewListenerNode(SActivationInfo * pActInfo)
{
	m_nGUID = GUID_INVALID;
}

////////////////////////////////////////////////////
CBuildingControllerViewListenerNode::~CBuildingControllerViewListenerNode()
{
	IBaseManager *pBaseManager = g_D6Core->pBaseManager;
	if (NULL != pBaseManager)
		pBaseManager->RemoveBuildingControllerEventListener(m_nGUID, this);
}

////////////////////////////////////////////////////
void CBuildingControllerViewListenerNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_nGUID", m_nGUID);
	if (ser.IsReading())
	{
		AddListener(pActInfo);
	}
}

////////////////////////////////////////////////////
void CBuildingControllerViewListenerNode::AddListener(SActivationInfo *pActInfo)
{
	// Get base manager
	IBaseManager *pBaseManager = g_D6Core->pBaseManager;
	if (NULL == pBaseManager)
	{
		GameWarning("[Flow] CBuildingControllerViewListenerNode::AddListener: No Base Manager!");
		return;
	}

	// Get GUID info if not set
	if (GUID_INVALID == m_nGUID)
	{
		const string& szTeam = GetPortString(pActInfo, EIP_Team);
		const string& szClass = GetPortString(pActInfo, EIP_Class);
		m_nGUID = pBaseManager->GenerateGUID(szTeam.c_str(), szClass.c_str());
	}

	// Add listener
	bool bEnable = GetPortBool(pActInfo, EIP_Enable);
	if (bEnable)
		pBaseManager->AddBuildingControllerEventListener(m_nGUID, this);
	else
		pBaseManager->RemoveBuildingControllerEventListener(m_nGUID, this);
}

////////////////////////////////////////////////////
void CBuildingControllerViewListenerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig<bool>("Enable", true, _HELP("Turn listening on/off")),
		InputPortConfig<string>("Team", _HELP("Team name")),
		InputPortConfig<string>("Class", _HELP("Class name")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("InView", _HELP("Triggered when controller is in view by local client")),
		OutputPortConfig_Void("OutOfView", _HELP("Triggered when controller is no loner in view by local client")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Used to listen for view-specific events from a Building Controller");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CBuildingControllerViewListenerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			m_actInfo = *pActInfo;
			AddListener(pActInfo);
		};
		break;

		case eFE_Activate:
		{
			if (IsPortActive(pActInfo, EIP_Enable))
			{
				AddListener(pActInfo);
			}
		}
		break;
	};
}

////////////////////////////////////////////////////
void CBuildingControllerViewListenerNode::OnBuildingControllerEvent(IBuildingController *pController,
																BuildingGUID nGUID, SControllerEvent &event)
{
	// Handle events
	switch (event.event)
	{
		case CONTROLLER_EVENT_INVIEW:
		{
			ActivateOutput(&m_actInfo, EOP_InView, (int)event.nParam[0]);
		}
		break;
		case CONTROLLER_EVENT_OUTOFVIEW:
		{
			ActivateOutput(&m_actInfo, EOP_OutOfView, true);
		}
		break;
	}
}


////////////////////////////////////////////////////
CBuildingControllerHasPowerNode::CBuildingControllerHasPowerNode(SActivationInfo * pActInfo)
{

}

////////////////////////////////////////////////////
CBuildingControllerHasPowerNode::~CBuildingControllerHasPowerNode()
{

}

////////////////////////////////////////////////////
void CBuildingControllerHasPowerNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{

}

////////////////////////////////////////////////////
void CBuildingControllerHasPowerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Check",_HELP("Check the power status")),
		InputPortConfig<string>("Team", _HELP("Team name")),
		InputPortConfig<string>("Class", _HELP("Class name")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig<bool>("Status", _HELP("Triggered when check occurs. Returns TRUE if controller has power, FALSE if it doesn't.")),
		OutputPortConfig_Void("Power", _HELP("Triggered if controller has power")),
		OutputPortConfig_Void("Power", _HELP("Triggered if controller does not have power")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Used to check the power status of a building");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CBuildingControllerHasPowerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		case eFE_Activate:
		{
			if (IsPortActive(pActInfo, EIP_Check))
			{
				// Get base manager
				IBaseManager *pBaseManager = g_D6Core->pBaseManager;
				if (NULL == pBaseManager)
				{
					GameWarning("[Flow] CBuildingControllerHasPowerNode::ProcessEvent: No Base Manager!");
					return;
				}

				// Get GUID info if not set
				const string& szTeam = GetPortString(pActInfo, EIP_Team);
				const string& szClass = GetPortString(pActInfo, EIP_Class);
				BuildingGUID GUID = pBaseManager->GenerateGUID(szTeam.c_str(), szClass.c_str());

				// Get controller
				IBuildingController *pController = pBaseManager->FindBuildingController(GUID);
				if (NULL == pController)
				{
					GameWarning("[Flow] CBuildingControllerHasPowerNode::ProcessEvent: Could not find controller!");
					return;
				}

				// Check its status
				bool bStatus = pController->HasPower();
				ActivateOutput(pActInfo, EOP_Status, bStatus);
				if (true == bStatus) ActivateOutput(pActInfo, EOP_Power, true);
				else ActivateOutput(pActInfo, EOP_NoPower, true);
			}
		}
		break;
	};
}


////////////////////////////////////////////////////
CBuildingControllerSetPowerNode::CBuildingControllerSetPowerNode(SActivationInfo * pActInfo)
{

}

////////////////////////////////////////////////////
CBuildingControllerSetPowerNode::~CBuildingControllerSetPowerNode()
{

}

////////////////////////////////////////////////////
void CBuildingControllerSetPowerNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{

}

////////////////////////////////////////////////////
void CBuildingControllerSetPowerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig<bool>("Set",_HELP("Set the power status. Pass in TRUE to turn power on, FALSE to turn power off.")),
		InputPortConfig_Void("SetWithValue",_HELP("Set the power status using what is specified in Value")),
		InputPortConfig<string>("Team", _HELP("Team name")),
		InputPortConfig<string>("Class", _HELP("Class name")),
		InputPortConfig<bool>("Value", true, _HELP("Power status to use")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Done", _HELP("Triggered when status is set. Returns building's new power status.")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Used to set the power status of a building");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CBuildingControllerSetPowerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Set) || 
				true == IsPortActive(pActInfo, EIP_SetWithValue))
			{
				// Get base manager
				IBaseManager *pBaseManager = g_D6Core->pBaseManager;
				if (NULL == pBaseManager)
				{
					GameWarning("[Flow] CBuildingControllerSetPowerNode::ProcessEvent: No Base Manager!");
					return;
				}

				// Get GUID info if not set
				const string& szTeam = GetPortString(pActInfo, EIP_Team);
				const string& szClass = GetPortString(pActInfo, EIP_Class);
				BuildingGUID GUID = pBaseManager->GenerateGUID(szTeam.c_str(), szClass.c_str());

				// Get controller
				IBuildingController *pController = pBaseManager->FindBuildingController(GUID);
				if (NULL == pController)
				{
					GameWarning("[Flow] CBuildingControllerSetPowerNode::ProcessEvent: Could not find controller!");
					return;
				}

				// Set its status
				bool bStatus =  GetPortBool(pActInfo, (true == IsPortActive(pActInfo, EIP_Set) ? EIP_Set : EIP_Value));
				pController->SetPower(bStatus);
				ActivateOutput(pActInfo, EOP_Done, pController->HasPower());
			}
		}
		break;
	};
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("BuildingController:GeneralListener", CBuildingControllerGeneralListenerNode);
REGISTER_FLOW_NODE("BuildingController:DamageListener", CBuildingControllerDamageListenerNode);
REGISTER_FLOW_NODE("BuildingController:ViewListener", CBuildingControllerViewListenerNode);
REGISTER_FLOW_NODE("BuildingController:HasPower", CBuildingControllerHasPowerNode);
REGISTER_FLOW_NODE("BuildingController:SetPower", CBuildingControllerSetPowerNode);