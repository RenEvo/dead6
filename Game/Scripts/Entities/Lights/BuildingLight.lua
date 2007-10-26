------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- BuildingLight.lua
--
-- Purpose: Used as a multi-purpose light based
--	on a building's status
--
-- History:
--      - 10/24/07 : File created - KAK
------------------------------------------------

Script.ReloadScript("Scripts/Entities/Lights/Light.lua" );

-- Light properties
local BuildingLight_LightProp = new(Light.Properties);
BuildingLight_LightProp.bActive = nil;

BuildingLight =
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

		-- Light profiles
		Profile_Alive_Power = {},
		Profile_Alive_NoPower = {},
		Profile_Dead_Power = {},
		Profile_Dead_NoPower = {},
	};
	Editor = {},

	-- Used to hold current light settings
	_LightTable = {},

	-- Active profile
	ActiveProfile = nil,
};
BuildingLight.Editor = merge(BuildingLight.Editor, Light.Editor, 1);
-- Profile copies
BuildingLight.Properties.Profile_Alive_Power = merge(BuildingLight.Properties.Profile_Alive_Power, BuildingLight_LightProp, 1);
BuildingLight.Properties.Profile_Alive_NoPower = merge(BuildingLight.Properties.Profile_Alive_NoPower, BuildingLight_LightProp, 1);
BuildingLight.Properties.Profile_Dead_Power = merge(BuildingLight.Properties.Profile_Dead_Power, BuildingLight_LightProp, 1);
BuildingLight.Properties.Profile_Dead_NoPower = merge(BuildingLight.Properties.Profile_Dead_NoPower, BuildingLight_LightProp, 1);

------------------------------------------------
-- OnInit
--
-- Purpose: Called when light is initialized
------------------------------------------------
function BuildingLight:OnInit()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	self:OnReset();
end

------------------------------------------------
-- OnShutdown
--
-- Purpose: Called when light is destroyed
------------------------------------------------
function BuildingLight:OnShutDown()
	self:FreeSlot(1);

	-- Remove me from listening
	if (self.Building) then
		self.Building.controller:RemoveEventListener(self.id);
	end
end

------------------------------------------------
-- OnLoad
--
-- Purpose: Called when light is loaded
--
-- In:	props - Serialization object
------------------------------------------------
function BuildingLight:OnLoad(props)
	self:OnReset();
	self:ActivateLight(props.bEnabled);
end

------------------------------------------------
-- OnSave
--
-- Purpose: Called when light is saved
--
-- In:	props - Serialization object
------------------------------------------------
function BuildingLight:OnSave(props)
	props.bEnabled = self.bEnabled;
end

------------------------------------------------
-- OnPropertyChange
--
-- Purpose: Called when a property changes on
--	the light fixture
------------------------------------------------
function BuildingLight:OnPropertyChange()
	self:OnReset();
	self:ActivateLight(self.bEnabled);
end

------------------------------------------------
-- OnReset
--
-- Purpose: Reset the light to initial state
------------------------------------------------
function BuildingLight:OnReset()
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
		System.Log("[BuildingLight] Could not find building Team=" .. self.Properties.CNCBuilding.Team .. " Class=" .. self.Properties.CNCBuilding.Class);
		self:ActivateLight(0);
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
		self:ResetLight();
	else
		self:GetBuildingStatus();
	end
end

------------------------------------------------
-- ResetLight
--
-- Purpose: Reset the active light
------------------------------------------------
function BuildingLight:ResetLight()
	-- Load profile
	if (self.ActiveProfile.Options.object_RenderGeometry ~= "") then
		self:LoadGeometry(0, self.ActiveProfile.Options.object_RenderGeometry);
		self:DrawSlot(0, 1);
	end
	self:ActivateLight(self.bEnabled);
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
function BuildingLight:SetBuildingStatus(status)
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
function BuildingLight:GetBuildingStatus()
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
		self:ResetLight();
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
function BuildingLight:OnBuildingEvent(building, event, params)
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
		self:ResetLight();
	end
end

------------------------------------------------
-- ActivateLight
--
-- Purpose: Set the light on/off
--
-- In:	enable - '1' to turn it on, nil or '0'
--		 to turn it off
------------------------------------------------
function BuildingLight:ActivateLight(enable)
	if (self.Building and enable and enable ~= 0) then
		self.bEnabled = 1;
		self:LoadLightToSlot(1);
		self:ActivateOutput("Active", true);
	else
		self.bEnabled = 0;
		self:FreeSlot(1);
		self:ActivateOutput("Active", false);
	end
end

------------------------------------------------
-- LoadLightToSlot
--
-- Purpose: Load a light into the specified slot
--	for the entity
------------------------------------------------
function BuildingLight:LoadLightToSlot(nSlot)
	local props = self.ActiveProfile;
	local Style = props.Style;
	local Projector = props.Projector;
	local Color = props.Color;
	local Options = props.Options;

	local diffuse_mul = Color.fDiffuseMultiplier;
	local specular_mul = Color.fSpecularMultiplier;
	
	local lt = self._LightTable;
	lt.style = Style.nLightStyle;
	lt.corona_scale = Style.fCoronaScale;
	lt.corona_dist_size_factor = Style.fCoronaDistSizeFactor;
	lt.corona_dist_intensity_factor = Style.fCoronaDistIntensityFactor;
	lt.radius = props.Radius;
	lt.diffuse_color = { x=Color.clrDiffuse.x*diffuse_mul, y=Color.clrDiffuse.y*diffuse_mul, z=Color.clrDiffuse.z*diffuse_mul };
	if (diffuse_mul ~= 0) then
		lt.specular_multiplier = specular_mul / diffuse_mul;
	else
		lt.specular_multiplier = 1;
	end
	
	lt.hdrdyn = Color.fHDRDynamic;
	lt.projector_texture = Projector.texture_Texture;
	lt.proj_fov = Projector.fProjectorFov;
	lt.proj_nearplane = Projector.fProjectorNearPlane;
	lt.cubemap = Projector.bProjectInAllDirs;
	lt.this_area_only = Options.bAffectsThisAreaOnly;
	lt.realtime = Options.bUsedInRealTime;
	lt.heatsource = 0;
	lt.fake = Options.bFakeLight;
	lt.fill_light = props.Test.bFillLight;
	lt.negative_light = props.Test.bNegativeLight;
	lt.indoor_only = 0;
	lt.has_cbuffer = 0;
	lt.cast_shadow = Options.bCastShadow;

	lt.lightmap_linear_attenuation = 1;
	lt.is_rectangle_light = 0;
	lt.is_sphere_light = 0;
	lt.area_sample_number = 1;
	
	lt.RAE_AmbientColor = { x = 0, y = 0, z = 0 };
	lt.RAE_MaxShadow = 1;
	lt.RAE_DistMul = 1;
	lt.RAE_DivShadow = 1;
	lt.RAE_ShadowHeight = 1;
	lt.RAE_FallOff = 2;
	lt.RAE_VisareaNumber = 0;

	self:LoadLight( nSlot,lt );
end

------------------------------------------------
-- Event_Enable
--
-- Purpose: Event called to enable the light
------------------------------------------------
function BuildingLight:Event_Enable(sender)
	if (self.bEnabled == 0) then
		self:ActivateLight(1);
	end
end

------------------------------------------------
-- Event_Disable
--
-- Purpose: Event called to disable the light
------------------------------------------------
function BuildingLight:Event_Disable(sender)
	if (self.bEnabled == 1) then
		self:ActivateLight(0);
	end
end

------------------------------------------------
-- Event_Active
--
-- Purpose: Event called to activate the light
------------------------------------------------
function BuildingLight:Event_Active(sender, bEnabled)
	self:ActivateLight(self.bEnabled);
end

------------------------------------------------
------------------------------------------------

-- Flow events
BuildingLight.FlowEvents =
{
	Inputs =
	{
		Active = { BuildingLight.Event_Active, "bool" },
		Enable = { BuildingLight.Event_Enable, "bool" },
		Disable = { BuildingLight.Event_Disable, "bool" },
	},
	Outputs =
	{
		Active = "bool",
	},
}