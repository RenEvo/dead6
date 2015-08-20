[Back](TechDoc_Architecture.md)

# Overview #

The Vehicle Spawn Point entity is used to describe a new spawn point for a team's vehicles. The owning team can be listed, along with special attributes that control if a Harvester can spawn here when purchased, and enabling/disabling of spawning.

# [Functionality](TechDoc_Architecture_Game_Entities_VehSpawnPoint_Functionality.md) #


# Properties #

  * Enabled - (Boolean) Set to TRUE if spawning at this point is initially allowed.
  * CanSpawnHarvester - (Boolean) Set to TRUE if the Harvester can spawn here. It is good practice to only allow the Harvester to spawn at points that are kind to its OnPurchased and/or ToField paths.
  * Team - (String) Name of team that owns the spawn point.
  * Class - (String) Name of building class that can create vehicles here.
  * BuildArea - (Vector) Dimensions of what describes the "Build Area". Any living item in this field will be killed if a vehicle spawns here. Furthermore, any static geometry in this area will prevent a vehicle from spawning here.


[Back](TechDoc_Architecture.md)