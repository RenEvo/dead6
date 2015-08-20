[Back](TechDoc_Architecture.md)

# Overview #

The Base Manager is responsible for aggregating all Building Controllers created during the lifetime of the Core. The Manager also listens to the entity system for entity creation/removing and damage reports, and delegates these to the Building Controllers as needed.

The Manager also holds on to a repository of building classes as they are defines, and creates unique IDs for them that are used in creating a building's GUID.

A Script Bind also exists that allows the Script System to communicate with the Base Manager.

# [Functionality](TechDoc_Architecture_System_BaseManager_Functionality.md) #

# Creating a Building Controller #
Based on the [CNCRules](TechDoc_Architecture_Game_CNCRules.md) XML file, each building listed in the Buildings section is created through the base manager. The system first looks for the XML file of the listed team in the map's Buildings\XML folder. If not found, the system looks for it in the Game\Scripts\Buildings\XML folder. Therefore, if listed:

```
<Buildings action="">
  <Building Team="GDI" Class="Refinery" ... />
</Buildings>
```

Then the system will look first for "

&lt;map&gt;

\Buildings\XML\Refinery.xml" and then "Game\Scripts\Buildings\XML\Refinery.xml". Additional attributes in the Building line are used to overwrite default values used when creating the building controller. These values are defined in the [CNCRules](TechDoc_Architecture_Game_CNCRules.md) XML file. To view the contents of this file, see [here](TechDoc_Architecture_Game_CNCBuildingControllers.md).

It is also possible to define a building inline by including what would be stored in the external Building's XML file directly into the building entry in the CNCRules.xml file.

Using **clear** as the _action_ attribute for the _Buildings_ tag will cause all existing building definitions to be cleared before parsing what is contained below. This can be useful if a map wishes to erase the default building definitions and load only the ones they define in their map's CNCRules.

# Base Power #
Each Building Controller retains its own power status. This enables some buildings to go underpowered while others remain fully powered. Some logic exists where all buildings are to go underpowered (such as when the team's [Power Plant](TechDoc_Architecture_Game_Buildings_PowerPlant.md) is destroyed) and therefore, functionality exists in the Base Manager to quickly toggle the power status of all controllers that belong to a given team.

[Back](TechDoc_Architecture.md)