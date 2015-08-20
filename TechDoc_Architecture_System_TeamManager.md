[Back](TechDoc_Architecture.md)

# Overview #
The Team Manager module is responsible for managing the defined teams. Teams are defined per level, with a Lua script and XML file that define the team's attributes and behavior. Each team is given a unique identifier ID that is used in conjunction with the Base Management code, as well as the game logic including damage rules and purchase rules.

The current team management code present in the Crysis codebase is stripped out and migrated into the Team Manager module. As such, some functionality present in the module is there for backwards compatibility.

The Team Manager is also responsible for managing the Harvesters created via the [Harvester Controllers](TechDoc_Architecture_Game_HarvesterFlow.md). Although the Team Manager does not handle any of the harvester logic, it does aggregate the Harvester Definitions for the controllers to use.

A Script Bind also exists that allows the Script System to communicate with the Team Manager.

# [Functionality](TechDoc_Architecture_System_TeamManager_Functionality.md) #


# Creating a Team #
Based on the [CNCRules](TechDoc_Architecture_Game_CNCRules.md) XML file, each team listed in the Teams section is created through the team manager. The system first looks for the XML file of the listed team in the map's Teams\XML folder. If not found, the system looks for it in the Game\Scripts\Teams\XML folder. Therefore, if listed:

```
<Teams action="">
  <Team>GDI</Team>
</Teams>
```

Then the system will look first for "

&lt;map&gt;

\Teams\XML\GDI.xml" and then "Game\Scripts\Teams\XML\GDI.xml". The attributes contained in this file are used to define the team, including the name, Lua script, and other properties. To view the contents of this file, see [here](TechDoc_Architecture_Game_CNCTeams.md).

It is also possible to define a team inline by including what would be stored in the external Team's XML file directly into the team entry in the CNCRules.xml file.

Using **clear** as the _action_ attribute for the _Teams_ tag will cause all existing teams to be cleared before parsing what is contained below. This can be useful if a map wishes to erase the default teams and load only the teams they define in their map's CNCRules.


# Team Harvesters #
[Harvester Controllers](TechDoc_Architecture_Game_HarvesterFlow.md) use the Team Manager to create and retain the Harvester Definitions created during the game. These definitions are used to define a Harvester in the world. The definitions are contained in a map inside each Team Definition.


# Vehicle Spawn Points #
Each team has its own list of valid [Vehicle Spawn Points](TechDoc_Architecture_Game_Entities_VehSpawnPoint.md) that are used to mark locations where a vehicle can be created when purchased/constructed through the team. Such vehicles include the [Harvester](TechDoc_Architecture_Game_HarvesterFlow.md) and vehicles purchased through the team's [Vehicle Factory](TechDoc_Architecture_Game_Buildings_VehicleFactory.md). When a vehicle is to be created, all spawn points have a weight factor calculated on them. This factor determines if a vehicle is able to spawn here (no static objects blocking its path) and how many actors and vehicles would have to be destroyed for it to go there. The first point with the lowest weight is used.


[Back](TechDoc_Architecture.md)