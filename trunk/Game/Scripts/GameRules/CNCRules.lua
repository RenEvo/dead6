------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- CNCRules.lua
--
-- Purpose: CNC Mode Game Rules
--
-- History:
--      - 9/28/07 : File created - KAK
------------------------------------------------

-- Inherit from Team Instant Action
Script.LoadScript("Scripts/GameRules/TeamInstantAction.lua", 1, 1);
CNCRules = new(TeamInstantAction);
CNCRules.Base = TeamInstantAction;

-- Drop teams from it, as they are taken out of the Rules.xml file
CNCRules.teamName = {};
CNCRules.teamModel = {};
CNCRules.teamRadio = {};

------------------------------------------------
-- Net table
Net.Expose {
	Class = CNCRules,
	ClientMethods = {
		ClSetSpawnGroup		= { RELIABLE_UNORDERED, POST_ATTACH, ENTITYID, },
		ClVictory		= { RELIABLE_ORDERED, POST_ATTACH, INT8, INT8 },

		ClClientConnect		= { RELIABLE_UNORDERED, POST_ATTACH, STRING, BOOL },
		ClClientDisconnect	= { RELIABLE_UNORDERED, POST_ATTACH, STRING, },
		ClClientEnteredGame	= { RELIABLE_UNORDERED, POST_ATTACH, STRING, },	
	},
	ServerMethods = {
		RequestRevive		= { RELIABLE_UNORDERED, POST_ATTACH, ENTITYID, },
		RequestSpawnGroup	= { RELIABLE_UNORDERED, POST_ATTACH, ENTITYID, ENTITYID },
	},
	ServerProperties = {
	},
};
------------------------------------------------

------------------------------------------------
-- Server.OnInit
--
-- Purpose: Called when the game rules are
--	first initialized on the server
------------------------------------------------
function CNCRules.Server:OnInit()
	-- TODO Fill teamId table with IDs of current teams
	self.teamId = {};

	-- Let base init
	if (self.Base) then
		self.Base.Server.OnInit(self);
	end
end

------------------------------------------------
-- Client.OnInit
--
-- Purpose: Called when the game rules are
--	first initialized on the client
------------------------------------------------
function CNCRules.Client:OnInit()
	-- Let base init
	if (self.Base) then
		self.Base.Client.OnInit(self);
	end

	-- TODO Fill teamId table with IDs of current teams
	if (not self.isServer) then
		self.teamId = {};
	end
end