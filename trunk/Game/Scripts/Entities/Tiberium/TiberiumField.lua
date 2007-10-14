------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- TiberiumField.lua
--
-- Purpose: Tiberium Field controller for harvesting
--	tiberium
--
-- History:
--      - 10/13/07 : File created - KAK
------------------------------------------------

TiberiumField =
{
	type = "Trigger",
	trigger = true,

	Properties =
	{
		DimX = 5,
		DimY = 5,
		DimZ = 5,
		LoadTime = 30.0,
	},
	States = {"Empty","Occupied"},
	
	Editor = 
	{
		Model = "Editor/Objects/T.cgf",
		Icon = "Trigger.bmp",
		ShowBounds = 1,
	},
}

------------------------------------------------
-- OnPropertyChange
--
-- Purpose: Called when user changes a property
--	in the Editor
------------------------------------------------
function TiberiumField:OnPropertyChange()
	self:OnReset();
end

------------------------------------------------
-- OnInit
--
-- Purpose: Called when field is first created
------------------------------------------------
function TiberiumField:OnInit()
	self:SetUpdatePolicy(ENTITY_UPDATE_PHYSICS);
	self:OnReset();
end

------------------------------------------------
-- OnShutDown
--
-- Purpose: Called when field is destroyed
------------------------------------------------
function TiberiumField:OnShutDown()

end

------------------------------------------------
-- OnSave
--
-- Purpose: Called when game is being saved
------------------------------------------------
function TiberiumField:OnSave(tbl)

end

------------------------------------------------
-- OnLoad
--
-- Purpose: Called when game is being loaded
------------------------------------------------
function TiberiumField:OnLoad(tbl)

end

------------------------------------------------
-- OnReset
--
-- Purpose: Called when field needs to be reset
--	back to its initial state
------------------------------------------------
function TiberiumField:OnReset()
	-- Set dimensions
	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetTriggerBBox(Min, Max);

	-- No harvester is in me
	self:GotoState("Empty");
end

------------------------------------------------
-- Area_Entered
--
-- Purpose: Called when something enters the area
--
-- In:	entity - Who entered the area
--		areaId - ID of the area
------------------------------------------------
function TiberiumField:Area_Entered(entity, areaId)
	
end

------------------------------------------------
-- Area_Left
--
-- Purpose: Called when something leaves the area
--
-- In:	entity - Who entered the area
--		areaId - ID of the area
------------------------------------------------
function TiberiumField:Area_Left(entity, areaId)
	
end
	
------------------------------------------------
------------------------------------------------

-- Empty state (No harvester is in it)
TiberiumField.Empty =
{
	------------------------------------------------
	-- OnBeginState
	--
	-- Purpose: Called when transitioning into state
	------------------------------------------------
	OnBeginState = function(self)
		
	end,
}

-- Occupied state (Harvester is in it)
TiberiumField.Occupied =
{
	------------------------------------------------
	-- OnBeginState
	--
	-- Purpose: Called when transitioning into state
	------------------------------------------------
	OnBeginState = function(self)
		
	end,
}

------------------------------------------------
------------------------------------------------

------------------------------------------------
-- Event: Load
--
-- Purpose: Call from a harvester to have it
--	load from this field
--
-- In:	sender - Harvester that is to load from
--	here
------------------------------------------------
function TiberiumField:Event_Load(sender)
	-- TODO
end

------------------------------------------------
-- FlowEvents
------------------------------------------------
TiberiumField.FlowEvents =
{
	Inputs =
	{
		Load = { TiberiumField.Event_Load, "void"},		-- Call to have the harvester load here
	},
	Outputs =
	{
		Loaded = "void",								-- Called when harvester is done loading here
	},
}
