[Back](TechDoc_Architecture.md)

# Overview #

The Building Sound entity acts as a specialty sound that is bound to a given Building Controller. Based on the status of the controller, one of the specified Profiles (which describe a particular sound) will be used by the entity. This allows you define a sound for when the building is functioning, has no power, or is destroyed.

# [Functionality](TechDoc_Architecture_Game_Entity_BuildingSound_Functionality.md) #

# Properties #
The sound profiles inherit from the SoundSpot entity. Please review its documentation for a description on the properties contained in the profile.

  * Enabled - (Boolean) Set to TRUE to enable the active sound, FALSE to disable it.
  * InnerRadius - (Int) Used to debug the InnerRadius flag of a sound profile by letting the Editor draw the radius in the world. Serves no functionality in game mode.
  * OuterRadius - (Int) Used to debug the OuterRadius flag of a sound profile by letting the Editor draw the radius in the world. Serves no functionality in game mode.
  * CNCBuilding.Team - (String) Name of controller's owning team. (Used to build GUID)
  * CNCBuilding.Class - (String) Name of controller's building class. (Used to build GUID)
  * CNCBuilding.Debug.IsDead - (Boolean) Used to debug in the Editor when not in game mode. Set to TRUE to make the building dead.
  * CNCBuilding.Debug.HasPower - (Boolean) Used to debug in the Editor when not in game mode. Set to TRUE to give the building power.

[Back](TechDoc_Architecture.md)