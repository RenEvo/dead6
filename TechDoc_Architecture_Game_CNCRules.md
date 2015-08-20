[Back](TechDoc_Architecture.md)

# Overview #
The Command & Conquer Mode Game Rules are divided up into two separate entities: the GameRules script which defines the behaviors and game logic of CNC Mode, and the CNC Rules XML feed which defines the contents of the game including the teams and building classes.

# [Script Functionality](TechDoc_Architecture_Game_CNCRules_Functionality.md) #

# CNC Rules XML #
The CNC Rules XML file (CNCRules.xml) is a **per-map defined** file. It defines the content to be created for the current CNC match. This content includes:

  * Team definitions
  * Building class definitions
  * Environment settings

The default rules file ~~loaded when not defined by a map~~ is located in the GameRules\XML folder. (Game\Scripts\GameRules\XML) This file contains the generic CNC rule's contents inherited by all maps. ~~that do not define their own.~~

A CNC-mode map is free to create their own CNCRules.xml file for use when that map is played. The file must be placed in the level's root folder. An entry must be added to the map's filelist.xml to ensure it is not tampered with on the client's end via the md5 checksum. If a map chooses to do this, their contents will compliment or overwrite the existing content in the default CNCRules.xml, based on the content itself. This allows maps to include their own, custom Teams and Building classes without losing or having to redefine the default ones.

## Sample XML ##

This is an example of a typical CNCRules.xml file:
```
<?xml version="1.0" encoding="utf-8"?>
<CNCRules>
  <Teams>
    <Team>GDI</Team>
    <Team Name="Nod" LongName="Brotherhood of Nod" Script="CNCTeam.lua" IsPlayerTeam="1">
      <Harvester Entity="Harvester" Capacity="300.0" BuildTime="10.0" LoadRate="10.0" UnloadRate="20.0" />
    </Team>
  </Teams>
  <Buildings>
    <Building Team="GDI" Class="Refinery" Script="" InitHealth="500" MustBeDestroyed="1"/>
    <Building Team="Nod" Class="Refinery" Script="" InitHealth="500" MaxHealth="500" MustBeDestroyed="1"/>
  </Buildings>
  <General>
    <TimeOfDay Hour="9" Minute="45" EnableLoop="0" />
  </General>
</CNCRules>
```

### Teams ###
The Teams section specifies what teams are present on this map. Each team is contained within its own _Team_ tag. If used as an element, the name specified must match the filename of the team's XML file, located in the Teams/XML folder. (Game\Scripts\Teams\XML). The team's definition file must then be defined in this XML file (as is the case with team GDI). Otherwise, the team's definition can be included directly in the CNCRules.xml file (as is the case with team Nod).

### Buildings ###
Similar to how the Teams section works, this area defines what Building Controllers should be created when the map is loading. Each building controller is contained within its own _Building_ tag. **The _Team_ and _Class_ attributes MUST BE DEFINED!** These are used to not only determine what team owns the controller, but also what XML file should be used to define the controller's default attributes. They are also used to construct the building's GUID. For more information on this, see [here](TechDoc_Architecture_System_BuildingController.md).

Below are the attributes that can be defined alongside the _Building_ tag:
  * Team - The owning team. This name must match the name of the Team specified in the team's XML file.
  * Name - Building class. This serves two purposes: 1. The name is used in the Class Repository of the [Base Manager](TechDoc_Architecture_System_BaseManager.md) to create a unique ID for it, which is in turn used to create the building's GUID. 2. The name is used to determine which Building XML file should be used to define the controller's default attributes. See [here](TechDoc_Architecture_System_BaseManager.md) for more information on how this works.
  * Please view the [CNC Building Classes](TechDoc_Architecture_Game_CNCBuildingClasses.md) for the definitions of the remaining attributes.

These values must be defined in the Building Controller's XML file, but not necessarily in the CNCRules.xml line. Any values present in CNCRules.xml will overwrite the values defined in the Building Controller's XML file.

### General ###
The General section defines general items for the map, such as environment settings. The following items can be defined here to give you more control over your map:

  * TimeOfDay - Specifies the starting time of day to be played out for this level. The **Hour** and **Minute** attributes may be defined to specify the hour and minute of the day. Furthermore, **EnableLoop** may be defined as "1" to turn day/night looping on.

# CNC Rules Script #
The CNC GameRules script handles CNC-Mode game logic. It adheres to the standards laid out for a GameRules script by Crysis's implementation for its many game types.

[Back](TechDoc_Architecture.md)