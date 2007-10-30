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
function _MakeDefState(building, state)
	local _DefaultStateDef =
	{
		Name = state,
		OnBeginState = function(self)
			--System.Log(self.Name .. ".BeginState(" .. building.States[state].Name .. ")");
		end,
		OnEndState = function(self)
			--System.Log(self.Name .. ".EndState(" .. building.States[state].Name .. ")");
		end,
		OnInit = function(self) end,
		OnInit_Server = function(self) end,
		OnInit_Client = function(self) end,
		OnShutdown = function(self) end,
		OnShutdown_Server = function(self) end,
		OnShutdown_Client = function(self) end,
		OnUpdate = function(self,dt) end,
		OnUpdate_Server = function(self,dt) end,
		OnUpdate_Client = function(self,dt) end,
		OnReset = function(self) end,
		OnReset_Server = function(self) end,
		OnReset_Client = function(self) end,
		OnDamaged = function(self,info,bIsExplosion) end,
		OnDamaged_Server = function(self,info,bIsExplosion) end,
		OnDamaged_Client = function(self,info,bIsExplosion) end,
		OnDestroyed = function(self,info,bIsExplosion) end,
		OnDestroyed_Server = function(self,info,bIsExplosion) end,
		OnDestroyed_Client = function(self,info,bIsExplosion) end,
		OnEvent = function(self,event,params) end,
		OnEvent_Server = function(self,event,params) end,
		OnEvent_Client = function(self,event,params) end,
	};

	building.States[state] = {};
	mergef(building.States[state], _DefaultStateDef, 1);
end

------------------------------------------------
-- InitializeStates
--
-- Purpose: Initialize the states defined for
--	the building
------------------------------------------------
function CNCBuilding:InitializeStates()
	self._state = nil;
	for i,v in ipairs(self.States) do
		_MakeDefState(self, v);
	end
end

------------------------------------------------
-- ResetStates
--
-- Purpose: Reset state machine
------------------------------------------------
function CNCBuilding:ResetStates()
	self:CallStateFunc("OnEndState");
	self._state = nil;
end

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