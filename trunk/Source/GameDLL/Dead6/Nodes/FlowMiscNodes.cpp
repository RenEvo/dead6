////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowMiscNodes.cpp
//
// Purpose: Miscellaneous flow nodes
//
// File History:
//	- 8/14/07 : File created - Kevin Kirst
////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Nodes/FlowMiscNodes.h"

////////////////////////////////////////////////////
CFlowStopwatchNode::CFlowStopwatchNode(SActivationInfo * pActInfo)
{
	m_fStartTime = 0.0f;
}

////////////////////////////////////////////////////
CFlowStopwatchNode::~CFlowStopwatchNode()
{

}

////////////////////////////////////////////////////
void CFlowStopwatchNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_fStartTime", m_fStartTime);
}

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
	static const SOutputPortConfig outputs[] =
	{
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


////////////////////////////////////////////////////
CFlowEditorGameStartNode::CFlowEditorGameStartNode(SActivationInfo * pActInfo)
{

}

////////////////////////////////////////////////////
CFlowEditorGameStartNode::~CFlowEditorGameStartNode()
{

}

////////////////////////////////////////////////////
void CFlowEditorGameStartNode::Seralize(SActivationInfo* pActInfo, TSerialize ser)
{

}

////////////////////////////////////////////////////
void CFlowEditorGameStartNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input ports
	static const SInputPortConfig inputs[] =
	{
		{0},
	};

	// Output ports
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Start", _HELP("Triggered when editor game starts")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Used to detect when the game in the editor has started");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CFlowEditorGameStartNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		case eFE_Initialize:
		{
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
		}
		break;

		case eFE_Update:
		{
			// Game started?
			if (true == g_D6Core->pD6Game->IsEditorGameStarted())
			{
				// Trigger
				ActivateOutput(pActInfo, EOP_Start, true);
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
			}
		}
		break;
	}
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

// Register the flow node with the name "Stopwatch" in the "MyFlowNodes" section
// Use the Node class for the stopwatch
REGISTER_FLOW_NODE("Time:Stopwatch", CFlowStopwatchNode);
REGISTER_FLOW_NODE("Game:EditorGameStart", CFlowEditorGameStartNode);