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
#include "Nodes/FlowHarvesterNodes.h"
#include "Interfaces/ITeamManager.h"
#include "IVehicleSystem.h"

////////////////////////////////////////////////////
CFlowHarvesterControllerNode::CFlowHarvesterControllerNode(SActivationInfo * pActInfo)
{
	m_fLastUpdate = 0.0f;
	m_fMakerTime = 0.0f;
	m_fGiveAmount = 0.0f;
	m_pHarvester = NULL;
	m_nTeamID = TEAMID_NOTEAM;
}

////////////////////////////////////////////////////
CFlowHarvesterControllerNode::~CFlowHarvesterControllerNode()
{

}

////////////////////////////////////////////////////
void CFlowHarvesterControllerNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_bReportPurchase", m_bReportPurchase);
	ser.Value("m_fLastUpdate", m_fLastUpdate);
	ser.Value("m_fMakerTime", m_fMakerTime);
	ser.Value("m_fGiveAmount", m_fGiveAmount);
	ser.Value("m_nTeamID", m_nTeamID);
}

////////////////////////////////////////////////////
void CFlowHarvesterControllerNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig<string>("Team", _HELP("Owning team")),
		InputPortConfig<Vec3>("CreateAt", _HELP("Where to create the Harvester at if not going through factory"), _HELP("Create At")),
		InputPortConfig<float>("CreateFace", 0.0f, _HELP("Direction (degrees) Harvester should face if not created through factory"), _HELP("Create Dir")),
		InputPortConfig<bool>("UseFactory", true, _HELP("TRUE Create Harvester through the Vechicle Factory, FALSE to use CreateAt point")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Purchased", _HELP("Triggered when the harvester is (re)purchased")),
		OutputPortConfig_Void("ToField", _HELP("Triggered when the harvester is to begin its route to the Tiberium Field")),
		OutputPortConfig_Void("FromField", _HELP("Triggered when the harvester is to begin its route back from the Tiberium Field")),
		OutputPortConfig_Void("Loading", _HELP("Triggered when the harvester is starting to load up")),
		OutputPortConfig_Void("Loaded", _HELP("Triggered when the harvester has loaded")),
		OutputPortConfig_Void("Unloading", _HELP("Triggered when the harvester is starting to unload")),
		OutputPortConfig_Void("Unloaded", _HELP("Triggered when the harvester has unloaded")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Main Harvester control point. Drop to create a harvester for a given team.");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
bool CFlowHarvesterControllerNode::RemakeHarvester(SActivationInfo *pActInfo)
{
	// Get factory and position settings
	bool bUseFactory = GetPortBool(pActInfo, EIP_UseFactory);
	Vec3 vPos(0,0,0);
	float fDir = 0.0f;
	if (false == bUseFactory)
	{
		vPos = GetPortVec3(pActInfo, EIP_CreateAt);
		fDir = GetPortFloat(pActInfo, EIP_CreateFace);
	}

	// Destroy old harvester if it is still in the world
	bool bNew = true;
	if (NULL != m_pHarvester)
	{
		// Remake it
		if (false == g_D6Core->pTeamManager->RemakeTeamHarvester(m_pHarvester, true))
		{
			// Warning...
			GameWarning("[Harvester Controller] Failed to recreate harvester %d for team \'%s\' (%u)",
				m_pHarvester->nID, g_D6Core->pTeamManager->GetTeamName(m_nTeamID), m_nTeamID);
			return false;
		}
	}
	else
	{
		// Create harvester
		HarvesterID nHarvesterID = g_D6Core->pTeamManager->CreateTeamHarvester(m_nTeamID, 
			bUseFactory, vPos, fDir, &m_pHarvester);
		if (HARVESTERID_INVALID == nHarvesterID || NULL == m_pHarvester)
		{
			// Warning...
			GameWarning("[Harvester Controller] Failed to create harvester for team \'%s\' (%u)",
				g_D6Core->pTeamManager->GetTeamName(m_nTeamID), m_nTeamID);
			return false;
		}
	}

	// Set graph entity
	pActInfo->pGraph->SetGraphEntity(PACK_TEAM_HARV_ID(m_nTeamID, m_pHarvester->nID), HARVESTER_GRAPH_INDEX);
	 
	m_fMakerTime = 0.0f;
	m_bReportPurchase = true;
	return true;
}

////////////////////////////////////////////////////
void CFlowHarvesterControllerNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			m_bReportPurchase = false;
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
			m_pHarvester = NULL;

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

				if (false == RemakeHarvester(pActInfo))
					break;

				// Need updating
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				m_fLastUpdate = gEnv->pTimer->GetCurrTime();
			}
			m_fMakerTime = 0.0f;
			m_fGiveAmount = 0.0f;
		}
		break;

		// eFE_Update - Flow node update
		case eFE_Update:
		{
			if (NULL == m_pHarvester) break;
			IVehicle *pVehicle = g_D6Core->pD6Game->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(m_pHarvester->nEntityID);
			IVehicleMovement *pVehicleMovement = (NULL == pVehicle ? NULL : pVehicle->GetMovement());
			if (NULL == pVehicle || NULL == pVehicleMovement) break;

			float fTime = gEnv->pTimer->GetCurrTime();
			float fDT = fTime - m_fLastUpdate;
			m_fLastUpdate = fTime;

			// Signal it was purchased
			// Note: We do this because the harvester is created in Initialize, which can't activate output ports
			if (true == m_bReportPurchase)
			{
				m_bReportPurchase = false;
				ActivateOutput(pActInfo, EOP_Purchased, true);
			}

			// Check on harvester if it is alive, recreate if not
			const SVehicleStatus status = pVehicle->GetStatus();
			if (status.health <= 0.0f /*|| status.flipped*/)
			{
				// Check repurchase timer
				if (m_fMakerTime == 0.0f)
				{
					m_fMakerTime = gEnv->pTimer->GetCurrTime();
				}
				else
				{
					// Has enough time passed to make a new one?
					if (gEnv->pTimer->GetCurrTime() - m_fMakerTime >= m_pHarvester->fBuildTime)
					{
						// Remake it
						RemakeHarvester(pActInfo);
					}
				}
				break;
			}

			// Check signals
			while (false == m_pHarvester->SignalQueue.empty())
			{
				// Dispatch signals
				int nSignal = m_pHarvester->SignalQueue.front();
				switch(nSignal)
				{
					// Move towards the field
					case SIGNAL_MOVETOFIELD:
					{
						ActivateOutput(pActInfo, EOP_ToField, true);
					}
					break;

					// Move away from field
					case SIGNAL_MOVEFROMFIELD:
					{
						ActivateOutput(pActInfo, EOP_FromField, true);
					}
					break;

					// Load tiberium
					case SIGNAL_LOAD:
					{
						// Cannot be unloading
						if (true == m_pHarvester->CheckFlag(HARVESTER_ISUNLOADING))
							break;

						// Set loading flag and begin the loading process
						m_pHarvester->SetFlag(HARVESTER_ISLOADING, true);
						m_pHarvester->fPayload = 0.0f;
						m_pHarvester->fPayloadTimer = gEnv->pTimer->GetCurrTime();

						// Signal out
						ActivateOutput(pActInfo, EOP_Loading, true);
					}
					break;

					// Unload tiberium
					case SIGNAL_UNLOAD:
					{
						// Cannot be loading and must have a load
						if (true == m_pHarvester->CheckFlag(HARVESTER_ISLOADING) ||
							false == m_pHarvester->CheckFlag(HARVESTER_HASLOAD))
							break;

						// Set unloading flag and begin the unloading process
						m_pHarvester->SetFlag(HARVESTER_ISUNLOADING, true);
						m_pHarvester->fPayloadTimer = gEnv->pTimer->GetCurrTime();

						// Signal out
						ActivateOutput(pActInfo, EOP_Unloading, true);
					}
					break;

					// Abort loading/unloading
					case SIGNAL_ABORT:
					{
						if (true == m_pHarvester->CheckFlag(HARVESTER_ISLOADING))
						{
							// Keep what we have and continue on
							m_pHarvester->SetFlag(HARVESTER_ISLOADING, false);
							m_pHarvester->SetFlag(HARVESTER_HASLOAD, true);
							m_pHarvester->fPayloadTimer = 0.0f;

							// Signal we are done
							ActivateOutput(pActInfo, EOP_Loaded, true);
						}
						else if (true == m_pHarvester->CheckFlag(HARVESTER_ISUNLOADING))
						{
							// Keep what we have and continue on
							m_pHarvester->SetFlag(HARVESTER_ISUNLOADING, false);
							m_pHarvester->SetFlag(HARVESTER_HASLOAD, true);
							m_pHarvester->fPayloadTimer = 0.0f;

							// Signal we are done
							ActivateOutput(pActInfo, EOP_Unloaded, true);
						}
					}
					break;

					// Start the engine
					case SIGNAL_STARTENGINE:
					{
						pVehicleMovement->StartEngine(0);
					}
					break;

					// Stop the engine
					case SIGNAL_STOPENGINE:
					{
						pVehicleMovement->StopEngine();
					}
					break;

					default:
						GameWarning("[Harvester Controller] Unknown signal given to harvester for team \'%s\' (%u) - Signal = %d",
							g_D6Core->pTeamManager->GetTeamName(m_pHarvester->nTeamID), m_pHarvester->nTeamID, nSignal);
				}
				m_pHarvester->SignalQueue.pop_front();
			}

			// Handle load/unload logic
			if (true == m_pHarvester->CheckFlag(HARVESTER_ISLOADING))
			{
				// Increase payload
				m_pHarvester->fPayload += m_pHarvester->fLoadRate*fDT;
#ifdef _DEBUG
				static int nLastPercent = 0;
				int nCurrPercent = int((m_pHarvester->fPayload/m_pHarvester->fCapacity)*10.0f);
				if (nCurrPercent > nLastPercent)
				{
					CryLogAlways("[Harvester] Debug: (Load) Payload at %d0%%", nCurrPercent);
					nLastPercent = nCurrPercent;
				}
#endif //_DEBUG
				if (m_pHarvester->fPayload >= m_pHarvester->fCapacity)
				{
					// At max
					m_pHarvester->fPayload = m_pHarvester->fCapacity;
					m_pHarvester->SetFlag(HARVESTER_ISLOADING, false);
					m_pHarvester->SetFlag(HARVESTER_HASLOAD, true);
					m_pHarvester->fPayloadTimer = 0.0f;

					// Signal we are done
					ActivateOutput(pActInfo, EOP_Loaded, true);
#ifdef _DEBUG
					nLastPercent = 0;
#endif //_DEBUG
				}
			}
			else if (true == m_pHarvester->CheckFlag(HARVESTER_ISUNLOADING))
			{
				// Decrease payload
				float fDecAmount = m_pHarvester->fUnloadRate*fDT;
				float fTransferAmount = fDecAmount;
				if (m_pHarvester->fPayload - fDecAmount < 0.0f)
					fTransferAmount = m_pHarvester->fPayload;

				// Check if we are done unloading
				m_pHarvester->fPayload -= fTransferAmount;
				m_fGiveAmount += fTransferAmount;
#ifdef _DEBUG
				static int nLastPercent = 10;
				int nCurrPercent = int((m_pHarvester->fPayload/m_pHarvester->fCapacity)*10.0f);
				if (nCurrPercent < nLastPercent)
				{
					CryLogAlways("[Harvester] Debug: (Unload) Payload at %d0%%", nCurrPercent);
					nLastPercent = nCurrPercent;
				}
#endif //_DEBUG
				if (m_pHarvester->fPayload <= 0.0f)
				{
					m_pHarvester->fPayload = 0.0f;
					m_pHarvester->SetFlag(HARVESTER_ISUNLOADING, false);
					m_pHarvester->SetFlag(HARVESTER_HASLOAD, false);
					m_pHarvester->fPayloadTimer = 0.0f;

					// Give players on team the amont
					g_D6Core->pTeamManager->GiveTeamCredits(m_pHarvester->nTeamID, m_fGiveAmount);
					m_fGiveAmount = 0.0f;

					// Signal we are done
					ActivateOutput(pActInfo, EOP_Unloaded, true);
#ifdef _DEBUG
					nLastPercent = 10;
#endif //_DEBUG
				}
			}
		}
		break;
	}
}


////////////////////////////////////////////////////
CFlowHarvesterSignalNode::CFlowHarvesterSignalNode(SActivationInfo * pActInfo)
{
	m_pHarvester = NULL;
}

////////////////////////////////////////////////////
CFlowHarvesterSignalNode::~CFlowHarvesterSignalNode()
{

}

////////////////////////////////////////////////////
void CFlowHarvesterSignalNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{

}

////////////////////////////////////////////////////
void CFlowHarvesterSignalNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Signal", _HELP("Make the signal")),
		InputPortConfig<int>("Value", 0, _HELP("Signal to send"), 0,
			_UICONFIG("enum_int:MoveToField=0,MoveFromField=1,Load=2,Unload=3,Abort=4,StartEngine=5,StopEngine=6") ),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Done", _HELP("Triggered when signal is made")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Use to send a signal to a Harvester.");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CFlowHarvesterSignalNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Activate - A port is active, handle it
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Signal))
			{
				// Get harvester associated with graph
				TeamID nTeamID = TEAMID_NOTEAM;
				HarvesterID nHarvesterID = HARVESTERID_INVALID;
				UNPACK_TEAM_HARV_ID(pActInfo->pGraph->GetGraphEntity(HARVESTER_GRAPH_INDEX), nTeamID, nHarvesterID);
				m_pHarvester = (NULL == g_D6Core->pTeamManager ? NULL : 
					g_D6Core->pTeamManager->GetTeamHarvester(nTeamID, nHarvesterID));
				if (NULL == m_pHarvester)
				{
					GameWarning("[Harvester Signal %d] Failed to get harvester associated with graph", pActInfo->myID);
					break;
				}

				// Send signal
				int nSignal = GetPortInt(pActInfo, EIP_SignalVal);
				m_pHarvester->SignalQueue.push_back(nSignal);

				// Activate done
				ActivateOutput(pActInfo, EOP_Done, true);
			}
		}
		break;
	}
}


////////////////////////////////////////////////////
CFlowHarvesterGotoNode::CFlowHarvesterGotoNode(SActivationInfo * pActInfo)
{
	m_bIsActive = m_bHasPastPoint = m_bNeedsTurnAround = false;
	m_fSpeedRat = 0.0f;
	m_fStartTime = 0.0f;

	m_bBreakPast = false;
	m_bReverse = false;
	m_bLockSteer = false;
	m_fMaxSpeed = m_fMinSpeed = m_fSlowDist = m_fTimeOut = 0.0f;
	m_pHarvester = NULL;
}

////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////
CFlowHarvesterGotoNode::~CFlowHarvesterGotoNode()
{

}

////////////////////////////////////////////////////
void CFlowHarvesterGotoNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_bIsActive", m_bIsActive);
	ser.Value("m_bHasPastPoint", m_bHasPastPoint);
	ser.Value("m_bNeedsTurnAround", m_bNeedsTurnAround);
	ser.Value("m_fSpeedRat", m_fSpeedRat);
	ser.Value("m_fStartTime", m_fStartTime);
	ser.Value("m_bBreakPast", m_bBreakPast);
	ser.Value("m_bReverse", m_bReverse);
	ser.Value("m_bLockSteer", m_bLockSteer);
	ser.Value("m_fMaxSpeed", m_fMaxSpeed);
	ser.Value("m_fMinSpeed", m_fMinSpeed);
	ser.Value("m_fSlowDist", m_fSlowDist);
	ser.Value("m_fTimeOut", m_fTimeOut);
	ser.Value("m_vGotoPos", m_vGotoPos);
}

////////////////////////////////////////////////////
void CFlowHarvesterGotoNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Move", _HELP("Move to the specified location")),
		InputPortConfig<Vec3>("GotoPos", _HELP("Where to move to. Note: 'Z' is ignored"), _HELP("Goto Position")),
		InputPortConfig<float>("MaxSpeed", 0.0f, _HELP("How fast the harvester can move (max)"), _HELP("Top Speed")),
		InputPortConfig<float>("MinSpeed", 0.0f, _HELP("How fast the harvester can move (min)"), _HELP("Low Speed")),
		InputPortConfig<float>("SlowDist", 0.0f, _HELP("How far away before harvester should start to slow down"), _HELP("Slow Distance")),
		InputPortConfig<bool>("BreakPast", false, _HELP("Set to signal harvester to break once it passes the location"), _HELP("Break Past")),
		InputPortConfig<bool>("Reverse", false, _HELP("Set to force harvester to drive in reverse")),
		InputPortConfig<bool>("LockSteering", false, _HELP("Set to prevent harvester from steering while moving"), _HELP("Lock Steering")),
		InputPortConfig<float>("TimeOut", 0.0f, _HELP("Set how long the harvester may attempt to move before timing out")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Done", _HELP("Triggered when harvester is at or passes the location")),
		OutputPortConfig_Void("Start", _HELP("Triggered when harvester begins to move to the location")),
		OutputPortConfig_Void("TimedOut", _HELP("Triggered when harvester times out and did not make it to the location"), _HELP("Timed Out")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Use to move a harvester to a new location.");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CFlowHarvesterGotoNode::Reset(SActivationInfo *pActInfo)
{
	m_bIsActive = m_bHasPastPoint = m_bNeedsTurnAround = false;
	m_fSpeedRat = 0.0f;

	m_bBreakPast = m_bReverse = m_bLockSteer = false;
	m_fMaxSpeed = m_fMinSpeed = m_fSlowDist = m_fTimeOut = 0.0f;
	pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
}

////////////////////////////////////////////////////
void CFlowHarvesterGotoNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			Reset(pActInfo);
		}
		break;

		// eFE_Activate - A port is active, handle it
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Move))
			{
				Reset(pActInfo);

				// Get params
				m_bBreakPast = GetPortBool(pActInfo, EIP_BreakPast);
				m_bReverse = GetPortBool(pActInfo, EIP_Reverse);
				m_bLockSteer = GetPortBool(pActInfo, EIP_LockSteering);
				m_fMaxSpeed = GetPortFloat(pActInfo, EIP_MaxSpeed);
				m_fMinSpeed = GetPortFloat(pActInfo, EIP_MinSpeed);
				m_fSlowDist = GetPortFloat(pActInfo, EIP_SlowDist);
				m_fTimeOut = GetPortFloat(pActInfo, EIP_TimeOut);
				m_vGotoPos = GetPortVec3(pActInfo, EIP_GotoPos);

				// Validate speeds
				if (m_fMinSpeed > m_fMaxSpeed) m_fMinSpeed = m_fMaxSpeed;
				if (m_fMaxSpeed < m_fMinSpeed) m_fMaxSpeed = m_fMinSpeed;

				// Square slow distance for faster math
				m_fSlowDist *= m_fSlowDist;
				if (m_fMaxSpeed == m_fMinSpeed) m_fSpeedRat = 1.0f;
				else m_fSpeedRat = m_fSlowDist/(m_fMaxSpeed-m_fMinSpeed);

				// Get harvester associated with graph
				TeamID nTeamID = TEAMID_NOTEAM;
				HarvesterID nHarvesterID = HARVESTERID_INVALID;
				UNPACK_TEAM_HARV_ID(pActInfo->pGraph->GetGraphEntity(HARVESTER_GRAPH_INDEX), nTeamID, nHarvesterID);
				m_pHarvester = (NULL == g_D6Core->pTeamManager ? NULL : 
					g_D6Core->pTeamManager->GetTeamHarvester(nTeamID, nHarvesterID));
				if (NULL == m_pHarvester)
				{
					GameWarning("[Harvester Goto %d] Failed to get harvester associated with graph", pActInfo->myID);
					break;
				}

				// Determine if vehicle needs to turn around first
				IEntity *pVehEntity = gEnv->pEntitySystem->GetEntity(m_pHarvester->nEntityID);
				Vec3 vVehPos(pVehEntity->GetPos());
				vVehPos.z = 0.0f;
				Quat orientation = pVehEntity->GetRotation();
				Vec3 vVehForward = orientation * Vec3Constants<float>::fVec3_OneY;
				if (true == m_bReverse) vVehForward *= -1.0f;
				float fDot = vVehForward.Dot((m_vGotoPos-vVehPos).GetNormalized());
				m_bNeedsTurnAround = (fDot < 0.0f);

				// Start a movin'!
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				m_bIsActive = true;
				m_fStartTime = gEnv->pTimer->GetCurrTime();
				ActivateOutput(pActInfo, EOP_Start, true);
			}
		}
		break;

		// eFE_Update - Flow node update
		case eFE_Update:
		{
			// Only update if active
			if (false == m_bIsActive) break;

			// Check timeout
			if (m_fTimeOut > 0.0f)
			{
				if (m_fTimeOut <= (gEnv->pTimer->GetCurrTime()-m_fStartTime))
				{
					ActivateOutput(pActInfo, EOP_TimedOut, true);
					m_bIsActive = false;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					break;
				}
			}

			// Manipulate harvester movement
			IEntity *pVehEntity = (NULL == m_pHarvester ? NULL : gEnv->pEntitySystem->GetEntity(m_pHarvester->nEntityID));
			IVehicle *pVehicle = (NULL == m_pHarvester ? NULL : g_D6Core->pD6Game->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(m_pHarvester->nEntityID));
			IVehicleMovement *pVehicleMovement = (NULL == pVehicle ? NULL : pVehicle->GetMovement());
			if (NULL != pVehEntity && NULL != pVehicleMovement)
			{
				float fRealSpeed = m_fMaxSpeed;
				Vec3 vVehPos(pVehEntity->GetPos());
				vVehPos.z = 0.0f;

				// See if it has passed the point
				if (false == m_bHasPastPoint)
				{
					Quat orientation = pVehEntity->GetRotation();
					Vec3 vVehForward = orientation * Vec3Constants<float>::fVec3_OneY;
					if (true == m_bReverse) vVehForward *= -1.0f;
					float fDot = vVehForward.Dot((m_vGotoPos-vVehPos).GetNormalized());
					if (false == m_bNeedsTurnAround && fDot <= 0.0f)
					{
						m_bHasPastPoint = true;
					}
					else if (true == m_bNeedsTurnAround && fDot > 0.0f)
					{
						m_bNeedsTurnAround = false; // We've turned around and are now heading towards it
					}
				}
				
				// Handle movement logic
				if (false == m_bHasPastPoint)
				{
					// Look to see if we should start slowing down
					if (m_fSlowDist > 0.0f)
					{
						float fDistSq = vVehPos.GetSquaredDistance2D(m_vGotoPos);
						if (fDistSq < m_fSlowDist)
						{
							// Calculate slowed speed
							fRealSpeed = (fDistSq/m_fSpeedRat) + m_fMinSpeed;
						}
					}
				}
				else
				{
					// Look if we need to apply the breaks
					if (true == m_bBreakPast)
						fRealSpeed = 0.0f;			// A speed of '0' activates the breaks
					else
						fRealSpeed = m_fMinSpeed;	// Use min speed to coast to a stop
				}

				// Request it to continue to move
				CMovementRequest request;
				request.SetDesiredSpeed((true==m_bReverse?-fRealSpeed:fRealSpeed)); // Will clamp to max speed based on vehicle settings
				request.SetMoveTarget(m_vGotoPos);
				request.SetLockedSteering(m_bLockSteer);
				pVehicleMovement->RequestMovement(request);

				// If we have arrived at the point, signal
				if (true == m_bHasPastPoint)
				{
					ActivateOutput(pActInfo, EOP_Done, true);
					m_bIsActive = false;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				}
			}
		}
		break;
	}
}



////////////////////////////////////////////////////
CFlowHarvesterTurnNode::CFlowHarvesterTurnNode(SActivationInfo * pActInfo)
{
	m_bIsActive = false;
	m_bTurnNeg = false;
	m_fLastUpdate = 0.0f;
	m_fTurnAngle = 0.0f;
	m_fTurnSpeed = 0.0f;
	m_fTimeOut = 0.0f;
	m_pHarvester = NULL;
}

////////////////////////////////////////////////////
CFlowHarvesterTurnNode::~CFlowHarvesterTurnNode()
{

}

////////////////////////////////////////////////////
void CFlowHarvesterTurnNode::Serialize(SActivationInfo* pActInfo, TSerialize ser)
{
	ser.Value("m_bIsActive", m_bIsActive);
	ser.Value("m_bTurnNeg", m_bTurnNeg);
	ser.Value("m_fLastUpdate", m_fLastUpdate);
	ser.Value("m_fStartTime", m_fStartTime);
	ser.Value("m_fTurnAngle", m_fTurnAngle);
	ser.Value("m_fTurnSpeed", m_fTurnSpeed);
	ser.Value("m_fTimeOut", m_fTimeOut);
}

////////////////////////////////////////////////////
void CFlowHarvesterTurnNode::GetConfiguration(SFlowNodeConfig& config)
{
	// Input
	static const SInputPortConfig inputs[] =
	{
		InputPortConfig_Void("Turn", _HELP("Turn to face the specified degree")),
		InputPortConfig<float>("FaceDeg", 0.0f, _HELP("Degree to face, with 0 being forward in vehicle's local space"), _HELP("Face Degree")),
		InputPortConfig<float>("Speed", 0.0f, _HELP("How fast the harvester can turn, degrees per second")),
		InputPortConfig<float>("TimeOut", 0.0f, _HELP("Set how long the harvester may attempt to turn before timing out")),
		{0},
	};

	// Output
	static const SOutputPortConfig outputs[] =
	{
		OutputPortConfig_Void("Done", _HELP("Triggered when harvester is at or passes the degree of facing")),
		OutputPortConfig_Void("Start", _HELP("Triggered when harvester begins to turn")),
		OutputPortConfig_Void("TimedOut", _HELP("Triggered when harvester times out and did not complete the turn"), _HELP("Timed Out")),
		{0},
	};

	// Set up config
	config.pInputPorts = inputs;
	config.pOutputPorts = outputs;
	config.sDescription = _HELP("Use to turn the harvester to a new facing on the spot.");
	config.SetCategory(EFLN_APPROVED);
}

////////////////////////////////////////////////////
void CFlowHarvesterTurnNode::Reset(SActivationInfo *pActInfo)
{
	m_bIsActive = false;
	m_bTurnNeg = false;
	m_fLastUpdate = 0.0f;
	m_fTurnAngle = 0.0f;
	m_fTurnSpeed = 0.0f;
	m_fTimeOut = 0.0f;
	m_pHarvester = NULL;
	pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
}

////////////////////////////////////////////////////
void CFlowHarvesterTurnNode::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	switch (event)
	{
		// eFE_Initialize - Flow node initialization
		case eFE_Initialize:
		{
			Reset(pActInfo);
		}
		break;

		// eFE_Activate - A port is active, handle it
		case eFE_Activate:
		{
			if (true == IsPortActive(pActInfo, EIP_Turn))
			{
				Reset(pActInfo);

				// Get params
				m_fTurnAngle = GetPortFloat(pActInfo, EIP_FaceDeg);
				m_fTurnSpeed = GetPortFloat(pActInfo, EIP_Speed);
				m_fTimeOut = GetPortFloat(pActInfo, EIP_TimeOut);

				// Get harvester associated with graph
				TeamID nTeamID = TEAMID_NOTEAM;
				HarvesterID nHarvesterID = HARVESTERID_INVALID;
				UNPACK_TEAM_HARV_ID(pActInfo->pGraph->GetGraphEntity(HARVESTER_GRAPH_INDEX), nTeamID, nHarvesterID);
				m_pHarvester = (NULL == g_D6Core->pTeamManager ? NULL : 
					g_D6Core->pTeamManager->GetTeamHarvester(nTeamID, nHarvesterID));
				if (NULL == m_pHarvester)
				{
					GameWarning("[Harvester Turn %d] Failed to get harvester associated with graph", pActInfo->myID);
					break;
				}

				// Turn off physics for this
				IEntity *pVehEntity = gEnv->pEntitySystem->GetEntity(m_pHarvester->nEntityID);
				IVehicle *pVehicle = g_D6Core->pD6Game->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(m_pHarvester->nEntityID);
				IVehicleMovement *pVehicleMovement = (NULL == pVehicle ? NULL : pVehicle->GetMovement());
				if (NULL != pVehEntity && NULL != pVehicleMovement)
				{
					CMovementRequest request;
					request.SetDesiredSpeed(0.0f);
					request.SetMoveTarget(pVehEntity->GetPos());
					pVehicleMovement->RequestMovement(request); // Clear whatever is in there right now
					pVehEntity->EnablePhysics(false);
				}

				// Clamp angle
				if (m_fTurnAngle < 0.0f) m_fTurnAngle += 360.0f;
				if (m_fTurnAngle >= 360.0f) m_fTurnAngle -= 360.0f;

				// Get current angle and corret it on vehicle
				Ang3 angles = Ang3(pVehEntity->GetRotation());
				float fAngle = RAD2DEG(angles.z);
				while (fAngle < 0.0f) fAngle += 360.0f;
				while (fAngle >= 360.0f) fAngle -= 360.0f;
				float fTestTurnAngle = m_fTurnAngle; // Don't lose clamped value
				pVehEntity->SetRotation(Quat::CreateRotationZ(DEG2RAD(fAngle)));

				// Test which way we need to rotate
				if (fabsf(fAngle-fTestTurnAngle) > 180.0f)
				{
					// Go the other way
					if (fAngle > 180.0f)
						fTestTurnAngle += 360.0f;
					else
						fTestTurnAngle -= 360.0f;
				}
				m_bTurnNeg = (fAngle > fTestTurnAngle);

				// Start a movin'!
				m_fLastUpdate = gEnv->pTimer->GetCurrTime();
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				m_bIsActive = true;
				m_fStartTime = gEnv->pTimer->GetCurrTime();
				ActivateOutput(pActInfo, EOP_Start, true);
			}
		}
		break;

		// eFE_Update - Flow node update
		case eFE_Update:
		{
			// Only update if active
			if (false == m_bIsActive) break;

			// Calculate update time
			float fCurrTime = gEnv->pTimer->GetCurrTime();
			float fDT = fCurrTime-m_fLastUpdate;
			m_fLastUpdate = fCurrTime;

			// Check timeout
			if (m_fTimeOut > 0.0f)
			{
				if (m_fTimeOut <= (fCurrTime-m_fStartTime))
				{
					ActivateOutput(pActInfo, EOP_TimedOut, true);
					m_bIsActive = false;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					break;
				}
			}

			// Manipulate harvester movement
			IEntity *pVehEntity = (NULL == m_pHarvester ? NULL : gEnv->pEntitySystem->GetEntity(m_pHarvester->nEntityID));
			if (NULL != pVehEntity)
			{
				// Get current facing degree
				Ang3 angles = Ang3(pVehEntity->GetRotation());
				float fAngle = RAD2DEG(angles.z);
				while (fAngle < 0.0f) fAngle += 360.0f;
				while (fAngle >= 360.0f) fAngle -= 360.0f;

				// Rotate a little
				if (false == m_bTurnNeg)
				{
					fAngle += m_fTurnSpeed*fDT;
					while (fAngle >= 360.0f) fAngle -= 360.0f;
					if (fAngle > m_fTurnAngle)
						fAngle = m_fTurnAngle;
				}
				else
				{
					fAngle -= m_fTurnSpeed*fDT;
					while (fAngle < 0.0f) fAngle += 360.0f;
					if (fAngle < m_fTurnAngle)
						fAngle = m_fTurnAngle;
				}

				// Set the angle
				pVehEntity->SetRotation(Quat::CreateRotationZ(DEG2RAD(fAngle)));

				// Are we done?
				if (fAngle == m_fTurnAngle)
				{
					pVehEntity->EnablePhysics(true);
					ActivateOutput(pActInfo, EOP_Done, true);
					m_bIsActive = false;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					break;
				}
			}
		}
		break;
	}
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Harvester:Controller", CFlowHarvesterControllerNode);
REGISTER_FLOW_NODE("Harvester:Signal", CFlowHarvesterSignalNode);
REGISTER_FLOW_NODE("Harvester:Goto", CFlowHarvesterGotoNode);
REGISTER_FLOW_NODE("Harvester:Turn", CFlowHarvesterTurnNode);