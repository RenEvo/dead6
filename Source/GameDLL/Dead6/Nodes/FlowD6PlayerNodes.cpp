////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowD6PlayerNodes.cpp
//
// Purpose: Flow Nodes for D6 player
//
// File History:
//	- 8/23/07 : File created - KAK
////////////////////////////////////////////////////

#include "Stdafx.h"
#include "FlowD6PlayerNodes.h"
#include "CD6Player.h"

////////////////////////////////////////////////////
// GetD6Player
//
// Purpose: Get the D6 Player from ID
//
// In:	nID - Entity ID
//
// Returns D6Player or NULL on error
////////////////////////////////////////////////////
CD6Player* GetD6Player(EntityId nID)
{
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(nID);
	if (NULL == pActor || false == pActor->IsPlayer()) return NULL;
	return (CD6Player*)pActor;
}

////////////////////////////////////////////////////
CD6PlayerCreditBankNode::CD6PlayerCreditBankNode(SActivationInfo * pActInfo)
{

}

////////////////////////////////////////////////////
CD6PlayerCreditBankNode::~CD6PlayerCreditBankNode()
{

}

////////////////////////////////////////////////////
void CD6PlayerCreditBankNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{

}

////////////////////////////////////////////////////
void CD6PlayerCreditBankNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input ports
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("GetCredits", _HELP("Just get the amount of credits"), _HELP("Get Credits")),
		InputPortConfig_Void("SetCredits", _HELP("Set the credits to the amount specified"), _HELP("Set Credits")),
		InputPortConfig_Void("GiveCredits", _HELP("Give (+) amount of credits specified"), _HELP("Give Credits")),
		InputPortConfig_Void("TakeCredits", _HELP("Take (-) amount of credits specified"), _HELP("Take Credits")),
		InputPortConfig<int>("Amount", 0, _HELP("Amount of credits")),
		{0},
	};

	// Output ports
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig<int>("Credits", _HELP("Resulting amount of credits based on action")),
		{0},
	};

	// Set up config
	config.nFlags |= EFLN_TARGET_ENTITY;
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Access or control a D6 player's credits");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CD6PlayerCreditBankNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		case eFE_Activate:
		{
			// Get D6 Player
			CD6Player *pPlayer = GetD6Player((NULL != pActInfo->pEntity ? pActInfo->pEntity->GetId() : 0));
			if (NULL == pPlayer) break;

			// Get amount
			unsigned int nAmount = GetPortInt(pActInfo, EIP_Amount);

			// Check which test to perform
			if (true == IsPortActive(pActInfo, EIP_GetCredits))
			{
				// Just return amount
				ActivateOutput(pActInfo, EOP_Credits, pPlayer->GetCredits());
			}
			else if (true == IsPortActive(pActInfo, EIP_SetCredits))
			{
				// Set amount and return
				pPlayer->SetCredits(nAmount);
				ActivateOutput(pActInfo, EOP_Credits, pPlayer->GetCredits());
			}
			else if (true == IsPortActive(pActInfo, EIP_GiveCredits))
			{
				// Give amount and return
				pPlayer->GiveCredits(nAmount);
				ActivateOutput(pActInfo, EOP_Credits, pPlayer->GetCredits());
			}
			else if (true == IsPortActive(pActInfo, EIP_TakeCredits))
			{
				// Take amount and return
				pPlayer->TakeCredits(nAmount);
				ActivateOutput(pActInfo, EOP_Credits, pPlayer->GetCredits());
			}
		}
		break;
	}
}


////////////////////////////////////////////////////
CD6PlayerCreditRangeNode::CD6PlayerCreditRangeNode(SActivationInfo * pActInfo)
{

}

////////////////////////////////////////////////////
CD6PlayerCreditRangeNode::~CD6PlayerCreditRangeNode()
{

}

////////////////////////////////////////////////////
void CD6PlayerCreditRangeNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{

}

////////////////////////////////////////////////////
void CD6PlayerCreditRangeNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input ports
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Trigger", _HELP("Run the test")),
		InputPortConfig<unsigned int>("MinAmount", 0.0f, _HELP("Minimum amount to check for"), _HELP("Minimum Amount")),
		InputPortConfig<unsigned int>("MaxAmount", 0.0f, _HELP("Maximum amount to check for"), _HELP("Maximum Amount")),
		{0},
	};

	// Output ports
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Passed", _HELP("Triggered if test passes (in range)")),
		OutputPortConfig_Void("Failed", _HELP("Triggered if test fails (not in range)")),
		{0},
	};

	// Set up config
	config.nFlags |= EFLN_TARGET_ENTITY;
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Determines if player's credits are in given range");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CD6PlayerCreditRangeNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Trigger))
			{
				// Get D6 Player
				CD6Player *pPlayer = GetD6Player((NULL != pActInfo->pEntity ? pActInfo->pEntity->GetId() : 0));
				if (NULL == pPlayer) break;

				// Get range
				unsigned int nMinRange = GetPortFloat(pActInfo, EIP_MinAmount);
				unsigned int nMaxRange = GetPortFloat(pActInfo, EIP_MaxAmount);

				// Get credits and test
				unsigned int nCredits = pPlayer->GetCredits();
				if (nCredits >= nMinRange && nCredits <= nMaxRange)
					ActivateOutput(pActInfo, EOP_Pass, true);
				else
					ActivateOutput(pActInfo, EOP_Fail, true);
			}
		}
		break;
	}
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("D6Player:CreditBank", CD6PlayerCreditBankNode);
REGISTER_FLOW_NODE("D6Player:CreditRange", CD6PlayerCreditRangeNode);