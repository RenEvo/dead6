------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- CNCBuilding_Common.lua
--
-- Purpose: Common logic for CNC building that
--	executes on both client and server
--
-- History:
--      - 10/11/07 : File created - KAK
------------------------------------------------

------------------------------------------------
-- Reset
--
-- Purpose: Reset the building controller for
--	both server and client
------------------------------------------------
function CNCBuilding:Reset()
	-- Set state
	if (self.controller:GetHealth() <= 0) then
		self:SetState("Dead");
	elseif (self.controller:HasPower() == false) then
		self:SetState("NoPower");
	else
		self:SetState("Alive");
	end
end

------------------------------------------------
-- Update
--
-- Purpose: Update the building controller for
--	both server and client
--
-- In:	dt - Change in time since last update
------------------------------------------------
function CNCBuilding:Update(dt)
	-- Call state
	self:CallStateFunc("OnUpdate", dt);
end

------------------------------------------------
-- Damaged
--
-- Purpose: Called when the building controller
--	is damaged on both server and client
--
-- In:	info - damage info
--	bIsExplosion - TRUE if explosion
------------------------------------------------
function CNCBuilding:Damaged(info, bIsExplosion)
	-- Call state
	self:CallStateFunc("OnDamaged", info, bIsExplosion);
end