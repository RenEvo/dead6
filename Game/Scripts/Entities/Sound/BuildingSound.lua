------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- BuildingSound.lua
--
-- Purpose: Used as a multi-purpose sound based
--	on a building's status
--
-- History:
--      - 10/24/07 : File created - KAK
------------------------------------------------

Script.ReloadScript("Scripts/Entities/Sound/SoundSpot.lua" );

-- Sound properties
local BuildingSound_SoundProp = new(SoundSpot.Properties);
BuildingSound_SoundProp.bEnabled = nil;
BuildingSound_SoundProp.bPlay = 1;

BuildingSound =
{
	Properties =
	{
		-- Initially enabled
		bEnabled = 1,
		InnerRadius = 0,
		OuterRadius = 0,

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

		-- Sound profiles
		Profile_Alive_Power = {},
		Profile_Alive_NoPower = {},
		Profile_Dead_Power = {},
		Profile_Dead_NoPower = {},
	};
	Editor = {},

	started = 0,

	-- Active profile
	ActiveProfile = nil,
};
BuildingSound.Editor = merge(BuildingSound.Editor, SoundSpot.Editor, 1);
-- Profile copies
BuildingSound.Properties.Profile_Alive_Power = merge(BuildingSound.Properties.Profile_Alive_Power, BuildingSound_SoundProp, 1);
BuildingSound.Properties.Profile_Alive_NoPower = merge(BuildingSound.Properties.Profile_Alive_NoPower, BuildingSound_SoundProp, 1);
BuildingSound.Properties.Profile_Dead_Power = merge(BuildingSound.Properties.Profile_Dead_Power, BuildingSound_SoundProp, 1);
BuildingSound.Properties.Profile_Dead_NoPower = merge(BuildingSound.Properties.Profile_Dead_NoPower, BuildingSound_SoundProp, 1);

------------------------------------------------
-- OnSpawn
--
-- Purpose: Called when sound object spawns
------------------------------------------------
function BuildingSound:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

------------------------------------------------
-- OnInit
--
-- Purpose: Called when sound is initialized
------------------------------------------------
function BuildingSound:OnInit()
	self:OnReset();
end

------------------------------------------------
-- OnShutdown
--
-- Purpose: Called when sound is destroyed
------------------------------------------------
function BuildingSound:OnShutDown()
	self:Stop();

	-- Remove me from listening
	if (self.Building) then
		self.Building.controller:RemoveEventListener(self.id);
	end
end

------------------------------------------------
-- OnLoad
--
-- Purpose: Called when sound is loaded
--
-- In:	props - Serialization object
------------------------------------------------
function BuildingSound:OnLoad(props)
	self.started = props.started;
	self.bEnabled = props.bEnabled;
	self:OnReset();
end

------------------------------------------------
-- OnSave
--
-- Purpose: Called when sound is saved
--
-- In:	props - Serialization object
------------------------------------------------
function BuildingSound:OnSave(props)
	props.started = self.started;
	props.bEnabled = self.bEnabled;
end

------------------------------------------------
-- OnPropertyChange
--
-- Purpose: Called when a property changes on
--	the sound
------------------------------------------------
function BuildingSound:OnPropertyChange()
	self:OnReset();
end

------------------------------------------------
-- OnReset
--
-- Purpose: Reset the sound to initial state
------------------------------------------------
function BuildingSound:OnReset()
	self:NetPresent(0);
	self:Stop();
	self.soundid = nil;
	self.bEnabled = self.Properties.bEnabled;
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
		System.Log("[BuildingSound] Could not find building Team=" .. self.Properties.CNCBuilding.Team .. " Class=" .. self.Properties.CNCBuilding.Class);
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
			else
				self.ActiveProfile = self.Properties.Profile_Alive_Power;
			end
		else
			if (0 == self.Properties.CNCBuilding.Debug.bHasPower) then
				self.ActiveProfile = self.Properties.Profile_Dead_NoPower;
			else
				self.ActiveProfile = self.Properties.Profile_Dead_Power;
			end
		end
		self:ResetSound();
	else
		self:GetBuildingStatus();
	end
end

------------------------------------------------
-- ResetSound
--
-- Purpose: Reset the active sound
------------------------------------------------
function BuildingSound:ResetSound()
	-- Stop old sound
	self:Stop();

	-- If play immediatly, play it
	if (self.ActiveProfile.bPlay == 1) then
		self:Play();
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
function BuildingSound:SetBuildingStatus(status)
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
function BuildingSound:GetBuildingStatus()
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
		self:ResetSound();
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
function BuildingSound:OnBuildingEvent(building, event, params)
	local changed = nil;

	if (event == CONTROLLER_EVENT_POWER) then
		local bHasPower = params[1];

		-- Switch to correct power profile
		if (bHasPower) then
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
		self:ResetSound();
	end
end

------------------------------------------------
-- OnSoundDone
--
-- Purpose: Called when sound is done playing
------------------------------------------------
function BuildingSound:OnSoundDone()
	self:ActivateOutput("DonePlaying", true);
end

------------------------------------------------
-- Play
--
-- Purpose: Play the selected sound
------------------------------------------------
function BuildingSound:Play()
	if (self.bEnabled == 0 or not self.ActiveProfile) then return end

	-- Stop current sound
	if (self.soundid ~= nil) then
		self:Stop(); -- entity proxy
	end

	-- Set the flags
	local sndFlags = bor(SOUND_DEFAULT_3D,SOUND_RADIUS);
	if (self.ActiveProfile.bLoop ~=0) then
		sndFlags = bor(sndFlags, SOUND_LOOP);
	end;

	-- Play it
	self.soundid = self:PlaySoundEventEx(self.ActiveProfile.sndSource, sndFlags, self.ActiveProfile.fVolume, g_Vectors.v000, self.ActiveProfile.InnerRadius, self.ActiveProfile.OuterRadius, SOUND_SEMANTIC_SOUNDSPOT);
	if (self.soundid) then
		--Sound.SetSoundVolume(self.soundid, self.ActiveProfile.fVolume);
		--Sound.SetMinMaxDistance(self.soundid, self.ActiveProfile.InnerRadius, self.ActiveProfile.OuterRadius);
		self.started = 1;
	end
end

------------------------------------------------
-- Stop
--
-- Purpose: Stop the current playing sound
------------------------------------------------
function BuildingSound:Stop()
	if (self.soundid ~= nil) then
		self:StopSound(self.soundid); -- stopping through entity proxy
		self.soundid = nil;
	end
	self.started = 0;
end

------------------------------------------------
-- Event_Play
--
-- Purpose: Play the current sound
------------------------------------------------
function SoundSpot:Event_Play(sender)
	-- Stop current sond
	if (self.soundid ~= nil) then
		self:Stop();
	end

	-- If we only play once, don't go on
	if (self.ActiveProfile and self.ActiveProfile.bOnce ~= 0 and self.started ~= 0) then return end

	-- Play it
	self:Play();
end

------------------------------------------------
-- Event_Stop
--
-- Purpose: Stop the current playing sound
------------------------------------------------
function SoundSpot:Event_Stop(sender, bStop)
	if (bStop == true and self.soundid ~= nil) then
		self:Stop();
	end
end

------------------------------------------------
-- Event_Enable
--
-- Purpose: Enable the system to play sounds
------------------------------------------------
function SoundSpot:Event_Enable(sender)
	self.Properties.bEnabled = 1;
	OnReset();
end

------------------------------------------------
-- Event_Disable
--
-- Purpose: Disable the system from playing sounds
------------------------------------------------
function BuildingSound:Event_Disable(sender)
	self.Properties.bEnabled = 0;
	OnReset();
end

------------------------------------------------
-- Event_Active
--
-- Purpose: Event called to activate the light
------------------------------------------------
function BuildingSound:Event_Active(sender, bEnabled)
	self.Properties.bEnabled = bEnabled;
	OnReset();
end

------------------------------------------------
------------------------------------------------

-- Flow events
BuildingSound.FlowEvents =
{
	Inputs =
	{
		Play = { BuildingSound.Event_Play, "bool" },
		Stop = { BuildingSound.Event_Stop, "bool" },
		Active = { BuildingSound.Event_Active, "bool" },
		Enable = { BuildingSound.Event_Enable, "bool" },
		Disable = { BuildingSound.Event_Disable, "bool" },
	},
	Outputs =
	{
		Active = "bool",
		DonePlaying = "bool",
	},	
}