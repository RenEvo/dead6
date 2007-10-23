////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowBuildingControllerNodes.h
//
// Purpose: Flow Nodes for Building Controllers
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_FLOWBUILDINGCONTROLLERNODES_H_
#define _D6C_FLOWBUILDINGCONTROLLERNODES_H_

#include "Nodes/G2FlowBaseNode.h"
#include "IBaseManager.h"
#include "IBuildingController.h"

// CBuildingControllerGeneralListenerNode
//	Simple node that lets you listen for basic controller events
class CBuildingControllerGeneralListenerNode : public CFlowBaseNode, public IBuildingControllerEventListener
{
protected:
	// Building GUID
	BuildingGUID m_nGUID;

	// Activation info
	SActivationInfo m_actInfo;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBuildingControllerGeneralListenerNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBuildingControllerGeneralListenerNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CBuildingControllerGeneralListenerNode(pActInfo);
	}

	////////////////////////////////////////////////////
	// Seralize
	//
	// Purpose: Used for saving/loading node's instance
	//
	// In:	pActInfo - Activation info
	//		ser - Serialization object
	////////////////////////////////////////////////////
	void Serialize(SActivationInfo* pActInfo, TSerialize ser);

	// Input Ports
	enum EInputPorts
	{
		EIP_Enable = 0,		// Bool: Turn it on/off
		EIP_Team,			// String: Team name
		EIP_Class,			// String: Class name
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Validated = 0,	// Triggered when controller has been validated
		EOP_Reset,			// Triggered when controller has been reset
	};

	////////////////////////////////////////////////////
	// GetConfiguration
	//
	// Purpose: Set up and return the configuration for
	//	this node for the Flow Graph
	//
	// Out:	config - The node's config
	////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config);

	////////////////////////////////////////////////////
	// ProcessEvent
	//
	// Purpose: Called when an event is to be processed
	//
	// In:	event - Flow event to process
	//		pActInfo - Activation info for the event
	////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo);

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s)
	{
		s->Add(*this);
	}

	////////////////////////////////////////////////////
	// AddListener
	//
	// Purpose: Add to controller's listener list
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	virtual void AddListener(SActivationInfo *pActInfo);
	
	////////////////////////////////////////////////////
	// OnBuildingControllerEvent
	//
	// Purpose: Called when event occurs on controller
	//
	// In:	pController - Building controller object
	//		nGUID - Controller's BuildingGUID
	//		event - Event that occured
	//
	// Note: See SControllerEvent
	////////////////////////////////////////////////////
	virtual void OnBuildingControllerEvent(IBuildingController *pController, BuildingGUID nGUID, SControllerEvent &event);
};

// CBuildingControllerDamageListenerNode
//	Simple node that lets you listen for damage-specific controller events
class CBuildingControllerDamageListenerNode : public CFlowBaseNode, public IBuildingControllerEventListener
{
protected:
	// Building GUID
	BuildingGUID m_nGUID;

	// Activation info
	SActivationInfo m_actInfo;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBuildingControllerDamageListenerNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBuildingControllerDamageListenerNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CBuildingControllerDamageListenerNode(pActInfo);
	}

	////////////////////////////////////////////////////
	// Seralize
	//
	// Purpose: Used for saving/loading node's instance
	//
	// In:	pActInfo - Activation info
	//		ser - Serialization object
	////////////////////////////////////////////////////
	void Serialize(SActivationInfo* pActInfo, TSerialize ser);

	// Input Ports
	enum EInputPorts
	{
		EIP_Enable = 0,		// Bool: Turn it on/off
		EIP_Team,			// String: Team name
		EIP_Class,			// String: Class name
		EIP_Hits,			// Bool: TRUE to listen for hits
		EIP_Explosions,		// Bool: TRUE to listen for explosions

		// Limiting input ports
		EIP_ShooterId,
		EIP_TargetId,
		EIP_Weapon,
		EIP_Ammo
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_OnDamage = 0,	// Triggered when damage has occured

		// Damage-specific items
		EOP_ShooterId,
		EOP_TargetId,
		EOP_WeaponId,
		EOP_ProjectileId,
		EOP_HitPos,
		EOP_HitDir,
		EOP_HitNormal,
		EOP_HitType,
		EOP_Damage,
		EOP_Material,
	};

	////////////////////////////////////////////////////
	// GetConfiguration
	//
	// Purpose: Set up and return the configuration for
	//	this node for the Flow Graph
	//
	// Out:	config - The node's config
	////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config);

	////////////////////////////////////////////////////
	// ProcessEvent
	//
	// Purpose: Called when an event is to be processed
	//
	// In:	event - Flow event to process
	//		pActInfo - Activation info for the event
	////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo);

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s)
	{
		s->Add(*this);
	}

	////////////////////////////////////////////////////
	// AddListener
	//
	// Purpose: Add to controller's listener list
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	virtual void AddListener(SActivationInfo *pActInfo);
	
	////////////////////////////////////////////////////
	// OnBuildingControllerEvent
	//
	// Purpose: Called when event occurs on controller
	//
	// In:	pController - Building controller object
	//		nGUID - Controller's BuildingGUID
	//		event - Event that occured
	//
	// Note: See SControllerEvent
	////////////////////////////////////////////////////
	virtual void OnBuildingControllerEvent(IBuildingController *pController, BuildingGUID nGUID, SControllerEvent &event);
};

// CBuildingControllerViewListenerNode
//	Simple node that lets you listen for view-specific controller events
class CBuildingControllerViewListenerNode : public CFlowBaseNode, public IBuildingControllerEventListener
{
protected:
	// Building GUID
	BuildingGUID m_nGUID;

	// Activation info
	SActivationInfo m_actInfo;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBuildingControllerViewListenerNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBuildingControllerViewListenerNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CBuildingControllerViewListenerNode(pActInfo);
	}

	////////////////////////////////////////////////////
	// Seralize
	//
	// Purpose: Used for saving/loading node's instance
	//
	// In:	pActInfo - Activation info
	//		ser - Serialization object
	////////////////////////////////////////////////////
	void Serialize(SActivationInfo* pActInfo, TSerialize ser);

	// Input Ports
	enum EInputPorts
	{
		EIP_Enable = 0,		// Bool: Turn it on/off
		EIP_Team,			// String: Team name
		EIP_Class,			// String: Class name
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_InView = 0,		// Triggered when controller is in view by local client
		EOP_OutOfView,		// Triggered when controller is no loner in view by local client
	};

	////////////////////////////////////////////////////
	// GetConfiguration
	//
	// Purpose: Set up and return the configuration for
	//	this node for the Flow Graph
	//
	// Out:	config - The node's config
	////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config);

	////////////////////////////////////////////////////
	// ProcessEvent
	//
	// Purpose: Called when an event is to be processed
	//
	// In:	event - Flow event to process
	//		pActInfo - Activation info for the event
	////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo);

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s)
	{
		s->Add(*this);
	}

	////////////////////////////////////////////////////
	// AddListener
	//
	// Purpose: Add to controller's listener list
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	virtual void AddListener(SActivationInfo *pActInfo);
	
	////////////////////////////////////////////////////
	// OnBuildingControllerEvent
	//
	// Purpose: Called when event occurs on controller
	//
	// In:	pController - Building controller object
	//		nGUID - Controller's BuildingGUID
	//		event - Event that occured
	//
	// Note: See SControllerEvent
	////////////////////////////////////////////////////
	virtual void OnBuildingControllerEvent(IBuildingController *pController, BuildingGUID nGUID, SControllerEvent &event);
};

#endif //_D6C_FLOWBUILDINGCONTROLLERNODES_H_