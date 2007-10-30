////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// FlowHarvesterNodes.h
//
// Purpose: Flow Nodes for Harvester logic control
//
// File History:
//	- 8/13/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_FLOWHARVESTERNODES_H_
#define _D6C_FLOWHARVESTERNODES_H_

#include "Nodes/G2FlowBaseNode.h"

// Index used in Graph Entity list for storing harvester ID between nodes in the same graph
// Please note: You should not put more than one controller in the same graph!
#define HARVESTER_GRAPH_INDEX (1)
#define PACK_TEAM_HARV_ID(teamid, harvid) ((((teamid)&0xFFFF)<<16)|((harvid)&0xFFFF))
#define UNPACK_TEAM_HARV_ID(recv, teamid, harvid) \
	(teamid) = (((recv)&0xFFFF0000)>>16); \
	(harvid) = (((recv)&0xFFFF));

enum ESignals
{
	SIGNAL_MOVETOFIELD = 0,	// Calls "ToField" Output Port on controller
	SIGNAL_MOVEFROMFIELD,	// Calls "FromField" Output Port on controller
	SIGNAL_LOAD,			// Causes Harvester to begin loading up on tiberium
	SIGNAL_UNLOAD,			// Causes Harvester to begin unloading its load
	SIGNAL_ABORT,			// Causes Harvester to stop loading/unloading and continue on with whatever it got
	SIGNAL_STARTENGINE,		// Causes Harvester to start its engine so it can move
	SIGNAL_STOPENGINE,		// Causes Harvester to stop its engine to it can't move
};

// CFlowHarvesterControllerNode
//	Controls the Harvester for a given team. Just create
//	one for each team and use the AI:AIGoto nodes to bring
//	it to the Tiberium Field and back.
class CFlowHarvesterControllerNode : public CFlowBaseNode
{
protected:
	bool m_bReportPurchase;
	float m_fLastUpdate;
	float m_fMakerTime;					// Set to time when harvester was marked for recreation or 0 if it doesn't need to be remade
	float m_fGiveAmount;				// How much to give to team when unloading is done
	TeamID m_nTeamID;
	STeamHarvesterDef *m_pHarvester;	// Harvester definition

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowHarvesterControllerNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowHarvesterControllerNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowHarvesterControllerNode(pActInfo);
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
		EIP_Team,			// String: Owning team
		EIP_CreateAt,		// Vector: Where to create Harvester at if Vehicle Factory is not used or not present
		EIP_CreateFace,		// Float: Direction to face when created 
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
	// RemakeHarvester
	//
	// Purpose: Removes the harvester from the world
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	bool RemakeHarvester(SActivationInfo *pActInfo);

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
	STeamHarvesterDef *m_pHarvester;	// Harvester definition

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowHarvesterSignalNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowHarvesterSignalNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowHarvesterSignalNode(pActInfo);
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
		EIP_Signal = 0,		// Call to make the signal
		EIP_SignalVal,		// Int: Signal to make (see ESignals)
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Done = 0,		// Called when the signal was made
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

// CFlowHarvesterGotoNode
//	Used to move a Harvester to a point in space
class CFlowHarvesterGotoNode : public CFlowBaseNode
{
protected:
	// Set to TRUE when active
	bool m_bIsActive;
	bool m_bHasPastPoint, m_bNeedsTurnAround;
	float m_fSpeedRat;
	float m_fStartTime;

	// Params
	bool m_bBreakPast;
	bool m_bReverse;
	bool m_bLockSteer;
	float m_fMaxSpeed, m_fMinSpeed;
	float m_fSlowDist;
	float m_fTimeOut;
	Vec3 m_vGotoPos;

	// Harvester info associated with graph
	STeamHarvesterDef *m_pHarvester;	// Harvester definition

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowHarvesterGotoNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowHarvesterGotoNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowHarvesterGotoNode(pActInfo);
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
		EIP_Move = 0,		// Call to move the harvester to the specified location
		EIP_GotoPos,		// Vec3: Where the harvester should move to
		EIP_MaxSpeed,		// Float: Top speed the harvester should move
		EIP_MinSpeed,		// Float: Low speed the harvester should move
		EIP_SlowDist,		// Float: How far away before slowing down
		EIP_BreakPast,		// Bool: TRUE if harvester should slam on the breaks once the point has been passed
		EIP_Reverse,		// Bool: TRUE if harvester should drive in reverse
		EIP_LockSteering,	// Bool: TRUE if harvester cannot steer while moving
		EIP_TimeOut,		// Float: How long before timeout is called
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Done = 0,		// Called when the harvester has made it to the location specified
		EOP_Start,			// Called when the harvester begins to move
		EOP_TimedOut,		// Called if node timed out and harvester did not make it to the location in time
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
	// Reset
	//
	// Purpose: Reset the node for the harvester to use
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	virtual void Reset(SActivationInfo *pActInfo);

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

// CFlowHarvesterTurnNode
//	Used to move a Harvester turn on the spot to face a new direction
class CFlowHarvesterTurnNode : public CFlowBaseNode
{
protected:
	// Set to TRUE when active
	bool m_bIsActive;
	bool m_bTurnNeg;
	float m_fLastUpdate;
	float m_fStartTime;

	// Params
	float m_fTurnAngle;
	float m_fTurnSpeed;
	float m_fTimeOut;

	// Harvester info associated with graph
	STeamHarvesterDef *m_pHarvester;	// Harvester definition

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CFlowHarvesterTurnNode(SActivationInfo * pActInfo);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CFlowHarvesterTurnNode();

	////////////////////////////////////////////////////
	// Clone
	//
	// Purpose: Used to clone the controller
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowHarvesterTurnNode(pActInfo);
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
		EIP_Turn = 0,		// Call to invoke the harvester to turn on the spot
		EIP_FaceDeg,		// Float: Degree of angle harvester should face, with 0 degrees being local forward
		EIP_Speed,			// Float: How fast the harvester should turn, degrees per second
		EIP_TimeOut,		// Float: How long before timeout is called
	};

	// Output Ports
	enum EOutputPorts
	{
		EOP_Done = 0,		// Called when the harvester has turned by the degree specified
		EOP_Start,			// Called when the harvester begins to move
		EOP_TimedOut,		// Called if node timed out
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
	// Reset
	//
	// Purpose: Reset the node for the harvester to use
	//
	// In:	pActInfo - Activation info
	////////////////////////////////////////////////////
	virtual void Reset(SActivationInfo *pActInfo);

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

#endif //_D6C_FLOWHARVESTERNODES_H_