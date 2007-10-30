------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- Refinery.lua
--
-- Purpose: Refinery building
--	Increases team funds and owns a harvester
--
-- History:
--      - 10/11/07 : File created - KAK
------------------------------------------------

Script.ReloadScript("Scripts/Buildings/CNCBuilding/CNCBuilding.lua");

Refinery = 
{
	Name = "Refinery",

	-- Building properties
	Properties =
	{
		-- Credit dispursal rate
		TickRate_Power = 1.0,
		TickRate_NoPower = 2.0,

		-- Credits received amount
		TickValue_Power = 1.0,
		TickValue_NoPower = 1.0,
	},

	-- Last tick timer
	lastTick = 0,
};

Refinery = MakeBuilding(Refinery);

------------------------------------------------
------------------------------------------------

------------------------------------------------
-- UpdateTick
--
-- Purpose: Check to see if resource tick has
--	passed and reward team with credits
--
-- In:	power - Pass in if building has power
------------------------------------------------
function Refinery:UpdateTick(power)
	local tickOccured = nil;
	if (power and _time - self.lastTick >= self.Properties.TickRate_Power) then
		tickOccured = 1;
	elseif (not power and _time - self.lastTick >= self.Properties.TickRate_NoPower) then
		tickOccured = 1;
	end
	if (not tickOccured) then return end

	-- TODO Give team money
	System.Log("[TODO_TEAM TODO_CLASS] Ticked!");

	-- Prepare for next tick
	self.lastTick = _time;
end

------------------------------------------------
------------------------------------------------

------------------------------------------------
function Refinery:Reset()
	self.Base.Reset(self);

	-- Reset values back to start
	local props = self.Properties;
	props.TickRate_Power = 1.0;
	props.TickRate_NoPower = 2.0;
	props.TickValue_Power = 1.0;
	props.TickValue_NoPower = 1.0;

	-- Set last tick to current time
	lastTick = _time;
end

------------------------------------------------
function Refinery:OnUpdate_Alive(dt)
	-- Update tick
	self:UpdateTick(1);
end
Refinery:SetStateFunc("Alive", "OnUpdate", Refinery.OnUpdate_Alive);

------------------------------------------------
function Refinery:OnUpdate_NoPower(dt)
	-- Update tick
	self:UpdateTick(nil);
end
Refinery:SetStateFunc("NoPower", "OnUpdate", Refinery.OnUpdate_NoPower);