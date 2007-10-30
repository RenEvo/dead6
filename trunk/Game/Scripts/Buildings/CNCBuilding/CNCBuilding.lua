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
	return res;
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
	System.Log(self.Name .. ".Server:OnInit()");

	self._state = nil;
end

------------------------------------------------
-- Client.OnInit
--
-- Purpose: Called when the building controller
--	is created on the client
------------------------------------------------
function CNCBuilding.Client:OnInit()
	System.Log(self.Name .. ".Client:OnInit()");

	self._state = nil;
end

------------------------------------------------
-- Server.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the server
------------------------------------------------
function CNCBuilding.Server:OnShutdown()
	System.Log(self.Name .. ".Server:OnShutdown()");
end

------------------------------------------------
-- Client.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the client
------------------------------------------------
function CNCBuilding.Client:OnShutdown()
	System.Log(self.Name .. ".Client:OnShutdown()");
end

------------------------------------------------
-- Server.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the server
------------------------------------------------
function CNCBuilding.Server:OnReset()
	System.Log(self.Name .. ".Server:OnReset()");

	-- Common reset
	self:Reset();
end

------------------------------------------------
-- Client.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the client
------------------------------------------------
function CNCBuilding.Client:OnReset()
	System.Log(self.Name .. ".Client:OnReset()");

	-- Common reset
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

	-- Common reset
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

	-- Common reset
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
	System.Log(self.Name .. ".Server:OnDamaged(" .. info.damage .. ")");

	-- Call state
	self:CallStateFunc("OnDamaged_Server", info, bIsExplosion);

	-- Common damage
	self:Damage(info, bIsExplosion);
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
	System.Log(self.Name .. ".Client:OnDamaged(" .. info.damage .. ")");

	-- Call state
	self:CallStateFunc("OnDamaged_Client", info, bIsExplosion);

	-- Common reset
	if (not self.isServer) then
		self:Damage(info, bIsExplosion);
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
	System.Log(self.Name .. ".Server:OnDestroyed()");
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
	System.Log(self.Name .. ".Client:OnDestroyed()");
end