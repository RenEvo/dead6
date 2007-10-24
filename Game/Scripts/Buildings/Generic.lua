------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- Generic.lua
--
-- Purpose: Generic building
--	Visual only
--
-- History:
--      - 10/23/2007 : File created - TDA
------------------------------------------------

Generic = 
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
function Generic.Server:OnInit()
	System.Log("Generic.Server:OnInit()");
end

------------------------------------------------
-- Client.OnInit
--
-- Purpose: Called when the building controller
--	is created on the client
------------------------------------------------
function Generic.Client:OnInit()
	System.Log("Generic.Client:OnInit()");
end

------------------------------------------------
-- Server.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the server
------------------------------------------------
function Generic.Server:OnShutdown()
	System.Log("Generic.Server:OnShutdown()");
end

------------------------------------------------
-- Client.OnShutdown
--
-- Purpose: Called when the building controller
--	is destroyed on the client
------------------------------------------------
function Generic.Client:OnShutdown()
	System.Log("Generic.Client:OnShutdown()");
end

------------------------------------------------
-- Server.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the server
------------------------------------------------
function Generic.Server:OnReset()
	System.Log("Generic.Server:OnReset()");
end

------------------------------------------------
-- Client.OnReset
--
-- Purpose: Called when the building controller
--	is reset on the client
------------------------------------------------
function Generic.Client:OnReset()
	System.Log("Generic.Client:OnReset()");
end