------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- PowerPlant.lua
--
-- Purpose: Power Plant building
--	Gives team's building's power
--
-- History:
--      - 10/11/07 : File created - KAK
------------------------------------------------

Script.ReloadScript("Scripts/Buildings/CNCBuilding/CNCBuilding.lua");

PowerPlant = 
{
	Name = "PowerPlant",

	-- Building properties
	Properties =
	{
		
	},
};

PowerPlant = MakeBuilding(PowerPlant);

------------------------------------------------
------------------------------------------------

------------------------------------------------
function PowerPlant:BeginState_Alive()
	-- Set team power
	Base.SetBasePower(self.controller:GetTeam(), true);
end
PowerPlant:SetStateFunc("Alive", "OnBeginState", PowerPlant.BeginState_Alive);

------------------------------------------------
function PowerPlant:BeginState_NoPower()
	-- Set team power
	Base.SetBasePower(self.controller:GetTeam(), false);
end
PowerPlant:SetStateFunc("NoPower", "OnBeginState", PowerPlant.BeginState_NoPower);

------------------------------------------------
function PowerPlant:BeginState_Dead()
	-- Set team power
	Base.SetBasePower(self.controller:GetTeam(), false);
end
PowerPlant:SetStateFunc("Dead", "OnBeginState", PowerPlant.BeginState_Dead);