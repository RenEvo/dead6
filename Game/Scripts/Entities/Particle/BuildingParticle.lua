------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- BuildingParticle.lua
--
-- Purpose: Used as a multi-purpose particle
--	emitter based on a building's status
--
-- History:
--      - 10/25/07 : File created - KAK
------------------------------------------------

Script.ReloadScript("Scripts/Entities/Particle/ParticleEffect.lua" );

-- Particle properties
local BuildingParticle_ParticleProp = new(ParticleEffect.Properties);
BuildingParticle_ParticleProp.bActive = nil;

BuildingParticle =
{
	Properties =
	{
		-- Initially enabled
		bEnabled = 1,

		-- CNC Building items
		CNCBuilding =
		{
			Team = "",
			Class = "",

			-- Debug table (for editor use)
			Debug =
			{
				bIsDead = 0,	-- Set to '1' to make it die
				bHasPower = 1,	-- Set to '0' to make it lose power
			},
		},

		-- Particle profiles
		Profile_Alive_Power = {},
		Profile_Alive_NoPower = {},
		Profile_Dead_Power = {},
		Profile_Dead_NoPower = {},
	};
	Editor = {},

	-- Active profile
	ActiveProfile = nil,
	ActiveProfileStr = "",
};
BuildingParticle.Editor = merge(BuildingParticle.Editor, ParticleEffect.Editor, 1);
-- Profile copies
BuildingParticle.Properties.Profile_Alive_Power = merge(BuildingParticle.Properties.Profile_Alive_Power, BuildingParticle_ParticleProp, 1);
BuildingParticle.Properties.Profile_Alive_NoPower = merge(BuildingParticle.Properties.Profile_Alive_NoPower, BuildingParticle_ParticleProp, 1);
BuildingParticle.Properties.Profile_Dead_Power = merge(BuildingParticle.Properties.Profile_Dead_Power, BuildingParticle_ParticleProp, 1);
BuildingParticle.Properties.Profile_Dead_NoPower = merge(BuildingParticle.Properties.Profile_Dead_NoPower, BuildingParticle_ParticleProp, 1);

------------------------------------------------
-- OnInit
--
-- Purpose: Called when effect is initialized
------------------------------------------------
function BuildingParticle:OnInit()
	self.nParticleSlot = -1;
	self.spawnTimer = 0;
	
	self:SetRegisterInSectors(1);
	
	self:SetUpdatePolicy(ENTITY_UPDATE_POT_VISIBLE);
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);

	self:OnReset();

	-- Preload all the effects
	self:PreLoadParticleEffect(self.Properties.Profile_Alive_Power.ParticleEffect);
	self:PreLoadParticleEffect(self.Properties.Profile_Alive_NoPower.ParticleEffect);
	self:PreLoadParticleEffect(self.Properties.Profile_Dead_Power.ParticleEffect);
	self:PreLoadParticleEffect(self.Properties.Profile_Dead_NoPower.ParticleEffect);
end

------------------------------------------------
-- OnShutdown
--
-- Purpose: Called when effect is destroyed
------------------------------------------------
function BuildingParticle:OnShutDown()
	-- Remove me from listening
	if (self.Building) then
		self.Building.controller:RemoveEventListener(self.id);
	end
end

------------------------------------------------
-- OnLoad
--
-- Purpose: Called when effect is loaded
--
-- In:	props - Serialization object
------------------------------------------------
function BuildingParticle:OnLoad(props)
	if (not props.nParticleSlot) then
		self:Reset();
	elseif (not self.nParticleSlot or self.nParticleSlot ~= props.nParticleSlot) then
		self:Reset();
		if (props.nParticleSlot >= 0 and self.ActiveProfile) then
			self.nParticleSlot = self:LoadParticleEffect(props.nParticleSlot, self.ActiveProfile.ParticleEffect, self.ActiveProfile);
		end
	end
end

------------------------------------------------
-- OnSave
--
-- Purpose: Called when effect is saved
--
-- In:	props - Serialization object
------------------------------------------------
function BuildingParticle:OnSave(props)
	props.nParticleSlot = self.nParticleSlot;
end

------------------------------------------------
-- OnPropertyChange
--
-- Purpose: Called when a property changes on
--	the effect emitter
------------------------------------------------
function BuildingParticle:OnPropertyChange()
	self:OnReset();
end

------------------------------------------------
-- OnReset
--
-- Purpose: Reset the effect to initial state
------------------------------------------------
function BuildingParticle:OnReset()
	self.bEnabled = self.Properties.bEnabled;
	self:Disable();
	self.nParticleSlot = nil;
	self.ActiveProfile = nil;
	self.ActiveProfileStr = "";

	-- Remove me from listening
	if (self.Building) then
		self.Building.controller:RemoveEventListener(self.id);
	end
	self.Building = nil;

	-- Get owning building's controller
	self.Building = Base.FindBuilding(self.Properties.CNCBuilding.Team, self.Properties.CNCBuilding.Class);
	if (not self.Building) then
		System.Log("[BuildingParticle] Could not find building Team=" .. self.Properties.CNCBuilding.Team .. " Class=" .. self.Properties.CNCBuilding.Class);
		return;
	end

	-- Add me to listener
	self.Building.controller:AddEventListener(self.id);

	-- If in editor, use the debug values
	if (g_gameRules.game:IsEditor() and not g_gameRules.game:IsInEditorGame()) then
		-- Set profile
		if (0 == self.Properties.CNCBuilding.Debug.bIsDead) then
			if (0 == self.Properties.CNCBuilding.Debug.bHasPower) then
				self.ActiveProfile = self.Properties.Profile_Alive_NoPower;
				self.ActiveProfileStr = "AliveNoPower";
			else
				self.ActiveProfile = self.Properties.Profile_Alive_Power;
				self.ActiveProfileStr = "AlivePower";
			end
		else
			if (0 == self.Properties.CNCBuilding.Debug.bHasPower) then
				self.ActiveProfile = self.Properties.Profile_Dead_NoPower;
				self.ActiveProfileStr = "DeadNoPower";
			else
				self.ActiveProfile = self.Properties.Profile_Dead_Power;
				self.ActiveProfileStr = "DeadPower";
			end
		end
		self:ResetEffect();
	else
		self:GetBuildingStatus();
	end
end

------------------------------------------------
-- ResetEffect
--
-- Purpose: Reset the active effect
------------------------------------------------
function BuildingParticle:ResetEffect()
	-- Destroy old effect
	self:Disable();
	self.nParticleSlot = nil;

	-- Go to correct state
	if (self.bEnabled ~= 0) then
		self:Enable();
	end
end

------------------------------------------------
-- SetBuildingStatus
--
-- Purpose: Set the status to use
--
-- In:	status - Status string
--
-- Returns '1' if status was changed
------------------------------------------------
function BuildingParticle:SetBuildingStatus(status)
	if (status == self.ActiveProfileStr) then return end
	if (status == "AlivePower") then
		self.ActiveProfile = self.Properties.Profile_Alive_Power;
		self.ActiveProfileStr = "AlivePower";
	elseif (status == "AliveNoPower") then
		self.ActiveProfile = self.Properties.Profile_Alive_NoPower;
		self.ActiveProfileStr = "AliveNoPower";
	elseif (status == "DeadPower") then
		self.ActiveProfile = self.Properties.Profile_Dead_Power;
		self.ActiveProfileStr = "DeadPower";
	elseif (status == "DeadNoPower") then
		self.ActiveProfile = self.Properties.Profile_Dead_NoPower;
		self.ActiveProfileStr = "DeadNoPower";
	else
		self.ActiveProfile = nil;
		self.ActiveProfileStr = "";
	end
	return 1;
end

------------------------------------------------
-- GetBuildingStatus
--
-- Purpose: Get the latest building status
--
-- Note: Should update ActiveProfile table
------------------------------------------------
function BuildingParticle:GetBuildingStatus()
	if (not self.Building) then return end

	-- Look at the status of the building
	local changed = nil;
	if (true == self.Building.controller:IsAlive()) then
		if (true == self.Building.controller:HasPower()) then
			changed = self:SetBuildingStatus("AlivePower");
		else
			changed = self:SetBuildingStatus("AliveNoPower");
		end
	else
		if (true == self.Building.controller:HasPower()) then
			changed = self:SetBuildingStatus("DeadPower");
		else
			changed = self:SetBuildingStatus("DeadNoPower");
		end
	end

	-- Detect change
	if (changed and self.ActiveProfile) then
		self:ResetEffect();
	end
end

------------------------------------------------
-- OnBuildingEvent
--
-- Purpose: Called when building event occurs
--
-- In:	building - Building controller
--	event - Event that occured
--	params - Table of params following event
------------------------------------------------
function BuildingParticle:OnBuildingEvent(building, event, params)
	local changed = nil;

	if (event == CONTROLLER_EVENT_POWER) then
		local bHasPower = params[0];

		-- Switch to correct power profile
		if (true == bHasPower) then
			if (self.ActiveProfileStr == "AlivePower" or self.ActiveProfileStr == "AliveNoPower") then
				changed = self:SetBuildingStatus("AlivePower");
			elseif (self.ActiveProfileStr == "DeadPower" or self.ActiveProfileStr == "DeadNoPower") then
				changed = self:SetBuildingStatus("DeadPower");
			end
		else
			if (self.ActiveProfileStr == "AlivePower" or self.ActiveProfileStr == "AliveNoPower") then
				changed = self:SetBuildingStatus("AliveNoPower");
			elseif (self.ActiveProfileStr == "DeadPower" or self.ActiveProfileStr == "DeadNoPower") then
				changed = self:SetBuildingStatus("DeadNoPower");
			end
		end
	elseif (event == CONTROLLER_EVENT_DESTROYED) then
		-- Switch to correct power profile
		if (self.ActiveProfileStr == "AlivePower" or self.ActiveProfileStr == "DeadPower") then
			changed = self:SetBuildingStatus("DeadPower");
		elseif (self.ActiveProfileStr == "AliveNoPower" or self.ActiveProfileStr == "DeadNoPower") then
			changed = self:SetBuildingStatus("DeadNoPower");
		end
	end

	-- Detect change
	if (changed and self.ActiveProfile) then
		self:ResetEffect();
	end
end

------------------------------------------------
-- Enable
--
-- Purpose: Create the effect
------------------------------------------------
function BuildingParticle:Enable()
	if (not self.nParticleSlot or self.nParticleSlot < 0) then
		self.nParticleSlot = self:LoadParticleEffect(-1, self.ActiveProfile.ParticleEffect, self.ActiveProfile);
	end
end

------------------------------------------------
-- Disable
--
-- Purpose: Destroy the effect
------------------------------------------------
function BuildingParticle:Disable()
	if (self.nParticleSlot and self.nParticleSlot >= 0) then
		self:FreeSlot(self.nParticleSlot);
		self.nParticleSlot = -1;
	end
end

------------------------------------------------
-- Event_Enable
--
-- Purpose: Event called to enable the effect
------------------------------------------------
function BuildingParticle:Event_Enable(sender)
	self:Enable();
	self:ActivateOutput("Active", true);
end

------------------------------------------------
-- Event_Disable
--
-- Purpose: Event called to disable the effect
------------------------------------------------
function BuildingParticle:Event_Disable(sender)
	self:Disable();
	self:ActivateOutput("Active", false);
end

------------------------------------------------
-- Event_Active
--
-- Purpose: Event called to activate the effect
------------------------------------------------
function BuildingParticle:Event_Active(sender, bEnabled)
	if (bEnabled ~= 0) then
		self:Enable();
	else
		self:Disable();
	end
	self:ActivateOutput("Active", bEnabled);
end

------------------------------------------------
-- Event_Restart
--
-- Purpose: Event called to restart the effect
------------------------------------------------
function BuildingParticle:Event_Restart()
	self:Disable();
	self:Enable();
	self:ActivateOutput("Restarted", true);
end

------------------------------------------------
------------------------------------------------

-- Flow events
BuildingParticle.FlowEvents =
{
	Inputs =
	{
		Active = { BuildingParticle.Event_Active, "bool" },
		Enable = { BuildingParticle.Event_Enable, "bool" },
		Disable = { BuildingParticle.Event_Disable, "bool" },
		Restart = { BuildingParticle.Event_Restart, "bool" },
	},
	Outputs =
	{
		Active = "bool",
		Restarted = "bool",
	},
}