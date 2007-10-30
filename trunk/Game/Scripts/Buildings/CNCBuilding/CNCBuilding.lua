------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- CNCBuilding.lua
--
-- Purpose: Basic building that all other buildings
--	inherit from
--
-- History:
--      - 10/11/07 : File created - KAK
------------------------------------------------

CNCBuilding =
{
	Server = {},
	Client = {},
	isServer = CryAction.IsServer(),
	isClient = CryAction.IsClient(),

	-- Building's name
	Name = "CNCBuilding",
};

-- Load helper scripts
Script.ReloadScript("Scripts/Buildings/CNCBuilding/CNCBuilding_States.lua");
Script.ReloadScript("Scripts/Buildings/CNCBuilding/CNCBuilding_Common.lua");

------------------------------------------------
-- MakeBuilding
--
-- Purpose: Make a building (inherit this one)
--
-- In:	building - Building to make
--
-- Returns new building table
--
-- Example: Refinery = MakeBuilding(Refinery);
------------------------------------------------
function MakeBuilding(building)
	local res = mergef(building, CNCBuilding, 1);
	res.Base = CNCBuilding;

	-- Create defined states
	CNCBuilding.InitializeStates(building);

	return res;
end

------------------------------------------------
------------------------------------------------

------------------------------------------------
-- GetFullName
--
-- Purpose: Return building's full name
--
-- Example: "[GDI Refiner]"
------------------------------------------------
function CNCBuilding:GetFullName()
	return "[" .. self.controller:GetTeamName() .. " " .. self.controller:GetClassName() .. "]";
end

------------------------------------------------
-- LogMessage
--
-- Purpose: Log a message from the controller
--
-- In:	szMsg - Message to log
------------------------------------------------
function CNCBuilding:LogMessage(szMsg)
	System.Log(self:GetFullName() .. " " .. szMsg);
end

------------------------------------------------
------------------------------------------------

------------------------------------------------
-- Server.OnInit
--
-- Purpose: Called when the building controller
--	is created on the server
------------------------------------------------
function CNCBuilding.Server:OnInit()
	--System.Log(self.Name .. ".Server:OnInit()");

	-- Common call
	self:Init();
end

------------------------------------------------
-- Client.OnInit
--
-- Purpose: Called when the building controller
--	is created on the client
------------------------------------------------
function CNCBuilding.Client:OnInit()
	--System.Log(self.Name .. ".Client:OnInit()");

	-- Common call
	if (not self.isServer) then
		self:Init();
	end
end

------------------------------------------------
-- Server.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the server
------------------------------------------------
function CNCBuilding.Server:OnShutdown()
	--System.Log(self.Name .. ".Server:OnShutdown()");

	-- Common call
	self:Shutdown();
end

------------------------------------------------
-- Client.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the client
------------------------------------------------
function CNCBuilding.Client:OnShutdown()
	--System.Log(self.Name .. ".Client:OnShutdown()");

	-- Common call
	if (not self.isServer) then
		self:Shutdown();
	end
end

------------------------------------------------
-- Server.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the server
------------------------------------------------
function CNCBuilding.Server:OnReset()
	--System.Log(self.Name .. ".Server:OnReset()");

	-- Common call
	self:Reset();
end

------------------------------------------------
-- Client.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the client
------------------------------------------------
function CNCBuilding.Client:OnReset()
	--System.Log(self.Name .. ".Client:OnReset()");

	-- Common call
	if (not self.isServer) then
		self:Reset();
	end
end

------------------------------------------------
-- Server.OnUpdate
--
-- Purpose: Called when the building controller
--	is updated on the server
--
-- In:	dt - Change in time since last update
------------------------------------------------
function CNCBuilding.Server:OnUpdate(dt)
	--System.Log(self.Name .. ".Server:OnUpdate(" .. dt .. ")");

	-- Call state
	self:CallStateFunc("OnUpdate_Server", dt);

	-- Common call
	self:Update();
end

------------------------------------------------
-- Client.OnUpdate
--
-- Purpose: Called when the building controller
--	is updated on the client
--
-- In:	dt - Change in time since last update
------------------------------------------------
function CNCBuilding.Client:OnUpdate(dt)
	--System.Log(self.Name .. ".Client:OnUpdate(" .. dt .. ")");

	-- Call state
	self:CallStateFunc("OnUpdate_Client", dt);

	-- Common call
	if (not self.isServer) then
		self:Update();
	end
end

------------------------------------------------
-- Server.OnDamaged
--
-- Purpose: Called when the building controller
--	is damaged on the server
--
-- In:	info - damage info
--	bIsExplosion - TRUE if explosion
------------------------------------------------
function CNCBuilding.Server:OnDamaged(info, bIsExplosion)
	--System.Log(self.Name .. ".Server:OnDamaged(" .. info.damage .. ")");

	-- Call state
	self:CallStateFunc("OnDamaged_Server", info, bIsExplosion);

	-- Common call
	self:Damaged(info, bIsExplosion);
end

------------------------------------------------
-- Client.OnDamaged
--
-- Purpose: Called when the building controller
--	is reset on the client
--
-- In:	info - damage info
--	bIsExplosion - TRUE if explosion
------------------------------------------------
function CNCBuilding.Client:OnDamaged(info, bIsExplosion)
	--System.Log(self.Name .. ".Client:OnDamaged(" .. info.damage .. ")");

	-- Call state
	self:CallStateFunc("OnDamaged_Client", info, bIsExplosion);

	-- Common call
	if (not self.isServer) then
		self:Damaged(info, bIsExplosion);
	end
end

------------------------------------------------
-- Server.OnDestroyed
--
-- Purpose: Called when the building controller
--	is reset on the server
--
-- In:	info - damage info that destroyed it
--	bIsExplosion - TRUE if explosion
------------------------------------------------
function CNCBuilding.Server:OnDestroyed(info, bIsExplosion)
	--System.Log(self.Name .. ".Server:OnDestroyed()");

	-- Call state
	self:CallStateFunc("OnDestroyed_Server", info, bIsExplosion);

	-- Common call
	self:Destroyed(info, bIsExplosion);
end

------------------------------------------------
-- Client.OnDestroyed
--
-- Purpose: Called when the building controller
--	is reset on the client
--
-- In:	info - damage info that destroyed it
--	bIsExplosion - TRUE if explosion
------------------------------------------------
function CNCBuilding.Client:OnDestroyed(info, bIsExplosion)
	--System.Log(self.Name .. ".Client:OnDestroyed()");

	-- Call state
	self:CallStateFunc("OnDestroyed_Client", info, bIsExplosion);

	-- Common call
	if (not self.isServer) then
		self:Destroyed(info, bIsExplosion);
	end
end

------------------------------------------------
-- Server.OnEvent
--
-- Purpose: Called when building event occurs
--	on the server
--
-- In:	event - Event that occured
--	params - Table of params following event
------------------------------------------------
function CNCBuilding.Server:OnEvent(event, params)
	--System.Log(self.Name .. ".Server:OnEvent(" .. event .. ")");

	-- Call state
	self:CallStateFunc("OnEvent_Server", event, params);

	-- Common call
	self:Event(event, params);
end

------------------------------------------------
-- Client.OnEvent
--
-- Purpose: Called when building event occurs
--	on the client
--
-- In:	event - Event that occured
--	params - Table of params following event
------------------------------------------------
function CNCBuilding.Client:OnEvent(event, params)
	--System.Log(self.Name .. ".Client:OnEvent(" .. event .. ")");

	-- Call state
	self:CallStateFunc("OnEvent_Client", event, params);

	-- Common call
	if (not self.isServer) then
		self:Event(event, params);
	end
end