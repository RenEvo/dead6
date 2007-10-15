////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowHarvesterNodes.cpp
//
// Purpose: Flow Nodes for Harvester logic control
//
// File History:
//	- 8/13/07 : File created - KAK
////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Nodes/G2FlowBaseNode.h"
#include "Interfaces/ITeamManager.h"

// CFlowHarvesterControllerNode
//	Controls the Harvester for a given team. Just create
//	one for each team and use the AI:AIGoto nodes to bring
//	it to the Tiberium Field and back.
class CFlowHarvesterControllerNode : public CFlowBaseNode
{
protected:
	float m_fMakerTime;			// Set to time when harvester was marked for recreation or 0 if it doesn't need to be remade
	HarvesterID m_nHarvester;	// Harvester ID
	TeamID m_nTeamID;			// Team ID

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowHarvesterControllerNode(SActivationInfo * pActInfo)
	{
		m_fMakerTime = 0.0f;
		m_nHarvester = HARVESTERID_INVALID;
		m_nTeamID = TEAMID_NOTEAM;
	}

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowHarvesterControllerNode()
	{

	}

	// Input Ports
	enum EInputPorts
	{
		EIP_Team,			// String: Owning team
		EIP_UnloadTime,		// Float: How long it takes to unload
		EIP_CreateAt,		// Vector: Where to create Harvester at if Vehicle Factory is not used or not present
		EIP_UseFactory,		// Bool: TRUE to use the factory, FALSE to use the CreateAt point. If no factory present, always false.
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Purchased = 0,	// Called when the harvester is purchased
		EOP_ToField,		// Called when harvester is to begin its travel to the field
		EOP_FromField,		// Called when harvester is to begin its travel back from 
		EOP_Loading,		// Called when the harvester starts to load
		EOP_Loaded,			// Called when the harvester has finished loading
		EOP_Unloading,		// Called when the harvester starts to unload
		EOP_Unloaded,		// Called when the harvester has finished unloading
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
};

// CFlowHarvesterSignalNode
//	Used to signal to a Harvester to perform a new step
//	such as traveling to/from the field, etc
class CFlowHarvesterSignalNode : public CFlowBaseNode
{
protected:
	

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowHarvesterSignalNode(SActivationInfo * pActInfo)
	{
		
	}

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowHarvesterSignalNode()
	{

	}

	// Input Ports
	enum EInputPorts
	{
		EIP_Signal,			// Call to make the signal
		EIP_SignalVal,		// Int: Signal to make (0 = "MoveToField", 1 = "MoveFromField", 2 = "Unload")
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Done = 0,	// Called when the signal was made, always
		EOP_Success,	// Called when the signal was made successfully
		EOP_Fail,		// Called when the signal was not made successfully
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
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
void CFlowHarvesterControllerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig<string>("Team", _HELP("Owning team")),
		InputPortConfig<float>("UnloadTime", 10.0f, _HELP("How long it should take to unload")),
		InputPortConfig<Vec3>("CreateAt", _HELP("Where to create the Harvester at if not going through factory")),
		InputPortConfig<bool>("UseFactory", true, _HELP("TRUE Create Harvester through the Vechicle Factory, FALSE to use CreateAt point")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] = {
		OutputPortConfig_Void("Purchased", _HELP("Trigger when the harvester is (re)purchased")),
		OutputPortConfig_Void("ToField", _HELP("Trigger when the harvester is to begin its route to the Tiberium Field.")),
		OutputPortConfig_Void("FromField", _HELP("Trigger when the harvester is to begin its route back from the Tiberium Field.")),
		OutputPortConfig_Void("Loading", _HELP("Trigger when the harvester is starting to load up")),
		OutputPortConfig_Void("Loaded", _HELP("Trigger when the harvester has loaded")),
		OutputPortConfig_Void("Unloading", _HELP("Trigger when the harvester is starting to unload")),
		OutputPortConfig_Void("Unloaded", _HELP("Trigger when the harvester has unloaded")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Main Harvester control point. Define the path using AI:AIGoto nodes and bring the harvester to/from the Tiberium Field!");
	config.SetCategory(EFLN_WIP);
}

////////////////////////////////////////////////////
void CFlowHarvesterControllerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			// If we're in the editor, only go through if game has started
			if (true == g_D6Core->pSystem->IsEditor() &&
				false == g_D6Core->pD6Game->IsEditorGameStarted())
				break;

			assert(g_D6Core->pTeamManager);
			if (NULL != g_D6Core->pTeamManager)
			{
				// Get team ID
				string szTeamName = GetPortString(pActInfo, EIP_Team);
				m_nTeamID = g_D6Core->pTeamManager->GetTeamId(szTeamName.c_str());

				// Get factory and position settings
				bool bUseFactory = GetPortBool(pActInfo, EIP_UseFactory);
				Vec3 vPos(0,0,0);
				if (false == bUseFactory)
				{
					vPos = GetPortVec3(pActInfo, EIP_CreateAt);
				}

				// Create harvester for first time
				m_nHarvester = g_D6Core->pTeamManager->CreateTeamHarvester(m_nTeamID, bUseFactory, vPos);
				if (HARVESTERID_INVALID == m_nHarvester)
				{
					// Warning...
					GameWarning("[Harvester Controller] Failed to create harvester for team \'%s\' (%u)",
						szTeamName.c_str(), m_nTeamID);
				}
			}
			m_fMakerTime = 0.0f;
		}
		break;

		// eFE_Activate - A port is active, handle it
		case eFE_Activate:
		{
			
		}
		break;

		// eFE_Update - Flow node update
		case eFE_Update:
		{
			// TODO Check on harvester if it is alive, recreate if not

			// TODO Check recreation timer
		}
		break;
	}
}

////////////////////////////////////////////////////
void CFlowHarvesterSignalNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Signal", _HELP("Make the signal")),
		InputPortConfig<int>("Value", 0, _HELP("Signal to send"), 0, _UICONFIG("enum_int:MoveToField=0,MoveFromField=1,Unload=2") ),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] = {
		OutputPortConfig_Void("Done", _HELP("Trigger when signal is made, always")),
		OutputPortConfig_Void("Success", _HELP("Trigger when signal is made successfully")),
		OutputPortConfig_Void("Fail", _HELP("Trigger when signal is not made successfully")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Use to send a signal to a Harvester.");
	config.SetCategory(EFLN_WIP);
}

////////////////////////////////////////////////////
void CFlowHarvesterSignalNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			
		}
		break;

		// eFE_Activate - A port is active, handle it
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Signal))
			{
				int a = 2;
			}
		}
		break;

		// eFE_Update - Flow node update
		case eFE_Update:
		{
			
		}
		break;
	}
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Harvester:Controller", CFlowHarvesterControllerNode);
REGISTER_FLOW_NODE_SINGLETON("Harvester:Signal", CFlowHarvesterSignalNode);