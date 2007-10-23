////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowMiscNodes.h
//
// Purpose: Miscellaneous flow nodes
//
// File History:
//	- 8/14/07 : File created - Kevin Kirst
////////////////////////////////////////////////////

#ifndef _D6C_FLOWMISCNODES_H_
#define _D6C_FLOWMISCNODES_H_

#include "Nodes/G2FlowBaseNode.h"

// CFlowStopwatchNode
//	A handly little stop watch for our flow graphs!
class CFlowStopwatchNode : public CFlowBaseNode
{
	// Time when stopwatch was started
	float m_fStartTime;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowStopwatchNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowStopwatchNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowStopwatchNode(pActInfo);
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
		EIP_Start = 0,
		EIP_Stop,
		EIP_Timeout,
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Done = 0,
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


// CFlowEditorGameStartNode
//	Calls when the editor game starts
class CFlowEditorGameStartNode : public CFlowBaseNode
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowEditorGameStartNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowEditorGameStartNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowEditorGameStartNode(pActInfo);
	}

	////////////////////////////////////////////////////
	// Seralize
	//
	// Purpose: Used for saving/loading node's instance
	//
	// In:	pActInfo - Activation info
	//		ser - Serialization object
	////////////////////////////////////////////////////
	void Seralize(SActivationInfo* pActInfo, TSerialize ser);

	// Input Ports
	enum EInputPorts
	{
		
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Start = 0,		 // Triggered when editor game starts
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

#endif //_D6C_FLOWMISCNODES_H_