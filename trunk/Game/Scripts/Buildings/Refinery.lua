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

Refinery = 
{
	Server = {},
	Client = {},
};

------------------------------------------------
-- Server.OnInit
--
-- Purpose: Called when the building controller
--	is created on the server
------------------------------------------------
function Refinery.Server:OnInit()
	System.Log("Refinery.Server:OnInit()");
end

------------------------------------------------
-- Client.OnInit
--
-- Purpose: Called when the building controller
--	is created on the client
------------------------------------------------
function Refinery.Client:OnInit()
	System.Log("Refinery.Client:OnInit()");
	self.controller:Test(5);
end

------------------------------------------------
-- Server.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the server
------------------------------------------------
function Refinery.Server:OnShutdown()
	System.Log("Refinery.Server:OnShutdown()");
end

------------------------------------------------
-- Client.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the client
------------------------------------------------
function Refinery.Client:OnShutdown()
	System.Log("Refinery.Client:OnShutdown()");
end

------------------------------------------------
-- Server.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the server
------------------------------------------------
function Refinery.Server:OnReset()
	System.Log("Refinery.Server:OnReset()");
end

------------------------------------------------
-- Client.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the client
------------------------------------------------
function Refinery.Client:OnReset()
	System.Log("Refinery.Client:OnReset()");
end