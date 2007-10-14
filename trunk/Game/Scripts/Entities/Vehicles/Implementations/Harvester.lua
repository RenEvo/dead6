------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- Harvester.lua
--
-- Purpose: Basic CNC Harvester vehicle script
--
-- History:
--      - 10/13/07 : File created - KAK
------------------------------------------------

Harvester =
{
	Properties = 
	{	
		bDisableEngine = 0,
		material = "",
		bFrozen = 0,
		Modification = "",
		FrozenModel = "",
		soclasses_SmartObjectClass = "",		
	},
	
	Client = {},
	Server = {},
}

Harvester.AIProperties = 
{
	-- AI attributes
	AIType = AIOBJECT_CAR,
	hidesUser = 1, 
	
	PropertiesInstance = 
	{
		aibehavior_behaviour = "CarIdle",
	},
	
	Properties = 
	{
		aicharacter_character = "Car",
		aiSpeedMult = 1.0,
	},
	
	AIMovementAbility = 
	{
		walkSpeed = 7.0,
		runSpeed = 15.0,
		sprintSpeed = 25.0,
		maneuverSpeed = 7.0,
		minTurnRadius = 4,
		maxTurnRadius = 15,    
		pathType = AIPATH_CAR,
		pathLookAhead = 8,
		pathRadius = 2,
		pathSpeedLookAheadPerSpeed = 3.0,
		cornerSlowDown = 0.75,
		pathFindPrediction = 1.0,
		velDecay = 130,
		resolveStickingInTrace = 0,
		pathRegenIntervalDuringTrace = 4.0,
		avoidanceRadius = 10.0,
	},   
}

