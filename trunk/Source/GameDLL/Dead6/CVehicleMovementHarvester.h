////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CVehicleMovementHarvester.h
//
// Purpose: Vehicle Movement class for the Harvester
//	Uses a "ghost" driver for dummy-AI control
//
// Note: Keep up-to-date with VehicleMovementStdWheeled!
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CVEHICLEMOVEMENTHARVESTER_H_
#define _D6C_CVEHICLEMOVEMENTHARVESTER_H_

#include "VehicleMovementStdWheeled.h"

class CVehicleMovementHarvester : public CVehicleMovementStdWheeled
{
	friend class CNetworkMovementStdWheeled;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CVehicleMovementHarvester(void);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CVehicleMovementHarvester(void);

	//Overrides from StdWheeled
	virtual bool Init(IVehicle* pVehicle, const SmartScriptTable &table);  
	virtual void PostInit(void);
	virtual void ProcessMovement(const float deltaTime);
	virtual bool RequestMovement(CMovementRequest& movementRequest);
	virtual void Update(const float deltaTime);
	virtual void UpdateSounds(const float deltaTime);
	virtual void GetMemoryStatistics(ICrySizer * s);

	////////////////////////////////////////////////////
	// StartEngine
	//
	// Purpose: Call to start the engine
	//
	// In:	driverId - Ignored
	//
	// Returns TRUE if engine started, FALSE otherwise
	//
	// Note: Overwritten to bypass needing a valid player
	//	to drive it!
	////////////////////////////////////////////////////
	virtual bool StartEngine(EntityId driverId);  
};

#endif //_D6C_CVEHICLEMOVEMENTHARVESTER_H_