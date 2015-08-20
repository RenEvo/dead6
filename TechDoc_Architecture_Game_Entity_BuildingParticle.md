[Back](TechDoc_Architecture.md)

# Overview #

The Building Particle entity acts as a specialty particle emitter that is bound to a given Building Controller. Based on the status of the controller, one of the specified Profiles (which describe a particular emitter) will be used by the entity. This allows you define an emitter for when the building is functioning, has no power, or is destroyed.

# [Functionality](TechDoc_Architecture_Game_Entity_BuildingParticle_Functionality.md) #

# Properties #
The particle emitter profiles inherit from the ParticleEffect entity. Please review its documentation for a description on the properties contained in the profile.

  * Enabled - (Boolean) Set to TRUE to enable the active effect, FALSE to disable it.
  * CNCBuilding.Team - (String) Name of controller's owning team. (Used to build GUID)
  * CNCBuilding.Class - (String) Name of controller's building class. (Used to build GUID)
  * CNCBuilding.Debug.IsDead - (Boolean) Used to debug in the Editor when not in game mode. Set to TRUE to make the building dead.
  * CNCBuilding.Debug.HasPower - (Boolean) Used to debug in the Editor when not in game mode. Set to TRUE to give the building power.

[Back](TechDoc_Architecture.md)