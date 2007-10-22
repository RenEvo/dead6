------------------------------------------------
-- C&C: The Dead 6 - Core File
-- Copyright (C), RenEvo Software & Designs, 2007
--
-- PortalEntity.lua
--
-- Purpose: An entity that has a portal attached
--	to it
--
-- History:
--      - 10/13/07 : File created - KAK
------------------------------------------------

Script.ReloadScript( "Scripts/Default/Entities/Others/BreakableObject.lua" );

PortalEntity =
{
	-- Properties
	Properties =
	{
		Portal =
		{
			CameraEntity = "",
			Texture = "",
			nFrameSkip = 5,
		},
	},
};

------------------------------------------------
------------------------------------------------

------------------------------------------------
-- OnReset
--
-- Purpose: Called when entity is reset
------------------------------------------------
function PortalEntity:OnReset()
	-- Base call
	BreakableObject.OnReset(self);

	-- Set up portal stuff
	local portalprop = self.Properties.Portal;
	Portal.MakeEntityPortal(self.id, portalprop.CameraEntity, portalprop.Texture, portalprop.nFrameSkip);
end

------------------------------------------------
-- OnShutDown
--
-- Purpose: Called when entity is destroyed
------------------------------------------------
function PortalEntity:OnShutDown()
	-- Base init
	BreakableObject.OnShutDown(self);

	-- Remove portal
	Portal.RemoveEntityPortal(self.id);
end

------------------------------------------------
------------------------------------------------

PortalEntity = mergef(PortalEntity, BreakableObject, 1);