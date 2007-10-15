////////////////////////////////////////////////////
// FlowMiscNodes.cpp
//
// Purpose: Miscellaneous flow nodes
//
// File History:
//	- 8/14/07 : File created - Kevin Kirst
////////////////////////////////////////////////////

#include "StdAfx.h"
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
	CFlowStopwatchNode(SActivationInfo * pActInfo)
	{
		m_fStartTime = 0.0f;
	}

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowStopwatchNode() { }

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

////////////////////////////////////////////////////
void CFlowStopwatchNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input ports
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Start", _HELP("Start the stopwatch")),
		InputPortConfig_Void("Stop", _HELP("Stop the stopwatch and calculate how much time has passed")),
		InputPortConfig<float>("Timeout", 0.0f, _HELP("How long the stopwatch should go before automatically stopping. Keep at \"0\" for infinite."), _HELP("Timeout Value")),
		{0},
	};

	// Output ports
	static const SOutputPortConfig outputs[] = {
		OutputPortConfig<float>("Done", _HELP("Triggered when the stopwatch stops")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("A helpful stopwatch for figuring out how long it takes to do something!");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CFlowStopwatchNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	// Handle the event...
	switch (event)
	{
		// eFE_Activate - A port is active
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Start))
			{
				// Start port is active, start the stopwatch!
				m_fStartTime = gEnv->pTimer->GetCurrTime();

				// We need to update now for the timeout!
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
			}
			else if (true == IsPortActive(pActInfo, EIP_Stop))
			{
				// Stop port is active, calculate how much time has passed.
				float fDuration = 0.0f;
				if (m_fStartTime > 0.0f)
				{
					fDuration = gEnv->pTimer->GetCurrTime() - m_fStartTime;
				}

				// Signal we are done
				ActivateOutput(pActInfo, EOP_Done, fDuration);

				// Reset start time
				m_fStartTime = 0.0f;
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
			}
		}
		break;

		// eFE_Update - The node is being updated
		case eFE_Update:
		{
			// If the stopwatch has started, check the timeout value
			if (m_fStartTime > 0.0f)
			{
				// Get timeout value
				float fTimeout = GetPortFloat(pActInfo, EIP_Timeout);
				if (fTimeout > 0.0f)
				{
					// They want it to timeout, so check to see if it has
					float fDuration = gEnv->pTimer->GetCurrTime() - m_fStartTime;
					if (fDuration > fTimeout)
					{
						// Signal we are done using the timeout value
						ActivateOutput(pActInfo, EOP_Done, fTimeout);

						// Reset start time
						m_fStartTime = 0.0f;
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					}
				}
			}
		}
		break;
	}
}

// Register the flow node with the name "Stopwatch" in the "MyFlowNodes" section
// Use the Node class for the stopwatch
REGISTER_FLOW_NODE("Timer:Stopwatch", CFlowStopwatchNode);