////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowD6PlayerNodes.h
//
// Purpose: Flow Nodes for D6 player
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_FLOWD6PLAYERNODES_H_
#define _D6C_FLOWD6PLAYERNODES_H_

#include "Nodes/G2FlowBaseNode.h"

// CD6PlayerCreditBankNode
//	Access and control a player's credits
class CD6PlayerCreditBankNode : public CFlowBaseNode
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6PlayerCreditBankNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6PlayerCreditBankNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CD6PlayerCreditBankNode(pActInfo);
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
		EIP_GetCredits = 0,		// Return the player's credits without altering them
		EIP_SetCredits,			// Set the player's credits to the specified amount
		EIP_GiveCredits,		// Give the player the specified amount of credits (+)
		EIP_TakeCredits,		// Take the specified amount of credits from the player (-)
		EIP_Amount,				// Float: How many credits to take
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Credits = 0,		// Triggered after action is carried out. Passes many credits player has.
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


// CD6PlayerCreditRangeNode
//	Determine if player's credits are in specified range
class CD6PlayerCreditRangeNode : public CFlowBaseNode
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CD6PlayerCreditRangeNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CD6PlayerCreditRangeNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CD6PlayerCreditRangeNode(pActInfo);
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
		EIP_Trigger = 0,		// Call to check the credit amount
		EIP_MinAmount,			// Float: Minimum amount to check for
		EIP_MaxAmount,			// Float: Maximum amount to check for
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Pass = 0,			// Triggered if test passes (in range)
		EOP_Fail,				// Triggered if test fails (not in range)
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

#endif //_D6C_FLOWD6PLAYERNODES_H_