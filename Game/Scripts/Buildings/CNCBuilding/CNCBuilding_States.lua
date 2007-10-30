------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- CNCBuilding_States.lua
--
-- Purpose: State logic for CNC building
--
-- History:
--      - 10/11/07 : File created - KAK
------------------------------------------------

-- Default states
CNCBuilding.States = { "Alive", "Dead", "NoPower" };

------------------------------------------------
-- Default state definition
------------------------------------------------
function _MakeDefState(state)
	local _DefaultStateDef =
	{
		OnBeginState = function(self)
			System.Log(self.Name .. ".BeginState(Alive)");
		end,
		OnEndState = function(self)
			System.Log(self.Name .. ".EndState(Alive)");
		end,
		OnUpdate = function(self,dt) end,
		OnUpdate_Server = function(self,dt) end,
		OnUpdate_Client = function(self,dt) end,
		OnDamaged = function(self,info,bIsExplosion) end,
		OnDamaged_Server = function(self,info,bIsExplosion) end,
		OnDamaged_Client = function(self,info,bIsExplosion) end,
	};

	CNCBuilding.States[state] = {};
	mergef(CNCBuilding.States[state], _DefaultStateDef, 1);
end
_MakeDefState("Alive");
_MakeDefState("Dead");
_MakeDefState("NoPower");

------------------------------------------------
-- CallStateFunc
--
-- Purpose: Call a function on the given state
--
-- In:	func - Name of function to call
--	... - Variable arguments
------------------------------------------------
function CNCBuilding:CallStateFunc(func, ...)
	if (not self._state) then return end

	-- Call it
	local call = self.States[self._state][func];
	if (call) then
		call(self,args);
	end
end

------------------------------------------------
-- SetStateFunc
--
-- Purpose: Set the callback function used for
--	a given state
--
-- In:	state - State to update
--	callback - Name of callback i.e. "OnUpdate"
--	func - New function to use
------------------------------------------------
function CNCBuilding:SetStateFunc(state, callback, func)
	-- validate
	if (not self or not self.States or not self.States[state]) then return end
	self.States[state][callback] = func;
end

------------------------------------------------
-- SetState
--
-- Purpose: Set the state of the building
--
-- In:	state - New state to go to
--
-- Returns '1' on success, nil on error
------------------------------------------------
function CNCBuilding:SetState(state)
	-- Must be a valid state
	if (not self.States[state]) then return end
	if (self._state == state) then return end

	-- Call OnEndState on last state
	self:CallStateFunc("OnEndState");

	-- Set new state
	self._state = state;
	self:CallStateFunc("OnBeginState");
	return 1;
end

------------------------------------------------
-- GetState
--
-- Purpose: Returns the current state
------------------------------------------------
function CNCBuilding:GetState()
	return self._state;
end

------------------------------------------------
-- GetStateTable
--
-- Purpose: Returns the current state table
------------------------------------------------
function CNCBuilding:GetStateTable()
	return self.States[self._state];
end