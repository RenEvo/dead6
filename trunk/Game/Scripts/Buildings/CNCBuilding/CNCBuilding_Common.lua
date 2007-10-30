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
-- Init
--
-- Purpose: Initialize the building controller for
--	both server and client
------------------------------------------------
function CNCBuilding:Init()
	
end

------------------------------------------------
-- Shutdown
--
-- Purpose: Shutdown the building controller for
--	both server and client
------------------------------------------------
function CNCBuilding:Shutdown()
	
end

------------------------------------------------
-- Reset
--
-- Purpose: Reset the building controller for
--	both server and client
------------------------------------------------
function CNCBuilding:Reset()
	-- Reset state
	self:ResetStates();

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

------------------------------------------------
-- Destroyed
--
-- Purpose: Called when the building controller
--	is destroyed on both server and client
--
-- In:	info - damage info
--	bIsExplosion - TRUE if explosion
------------------------------------------------
function CNCBuilding:Destroyed(info, bIsExplosion)
	-- Call state
	self:CallStateFunc("OnDestroyed", info, bIsExplosion);
end

------------------------------------------------
-- Event
--
-- Purpose: Called when building event occurs
--	on both server and client
--
-- In:	event - Event that occured
--	params - Table of params following event
------------------------------------------------
function CNCBuilding:Event(event, params)
	-- Call state
	self:CallStateFunc("OnEvent", event, params);

	-- Check for state transitions
	if (event == CONTROLLER_EVENT_POWER) then
		-- Check health
		if (self.controller:GetHealth() <= 0) then
			self:SetState("Dead");
		else
			-- Check power status
			if (params[1]) then
				self:SetState("Alive");
			else
				self:SetState("NoPower");
			end
		end
	end
	if (event == CONTROLLER_EVENT_DESTROYED) then
		self:SetState("Dead");
	end
end