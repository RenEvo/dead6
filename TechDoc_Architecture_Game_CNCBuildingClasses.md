[Back](TechDoc_Architecture.md)

# Overview #
A Building Class represents a unique building in CNC rules. Combined with an owning team, a Building Class becomes a living Building Controller which in turn has its own interface entities that allow players to communicate with the building. A Building Class has its own XML file that is used to define its attributes and a Lua Script that is used to define its behaviors.

# [Script Functionality](TechDoc_Architecture_Game_CNCBuildingClasses_Functionality.md) #

# Building Class XML File #
The Building Class XML file is the blueprint for a class. It defines the class' attributes and is the lookup book used by the Dead 6 Core to create the a Building Controller. When a Building Controller is created, the class' XML file is loaded from the Buildings\XML folder. (Game\Scripts\Buildings\XML)

This file is used as the default definition for the Building Class. Attributes may be defined in the CNCRules.xml file that will overwrite the values contained here.

## Sample XML File ##
This is an example of a typical building class XML file:
```
<?xml version="1.0" encoding="utf-8"?>
<Buildings>
  <Building Name="Refinery" Script="Refinery.lua" InitHealth="500" MaxHealth="500" MustBeDestroyed="1" />
    <Properties>
      <add key="someProp" value="1.0" />
    </Properties>
</Buildings>
```

The Building Class' tag's attributes are as such:
  * Name - The name of the building class. This must match the script table defined in its Lua script.
  * Script - The Lua script inherited and used by the Building Controller for building behavior.
  * InitHealth - Initial health of the building. If not specified or invalid, the building's health will be set to its Max value. An invalid value would be negative or greater than the building's Max health. The health of the building is also set to this value whenever it is "Reset".
  * MaxHealth - The controller's max health. The building can be repaired up to this value.
  * MustBeDestroyed - A boolean flag that determines if the building must be destroyed for its owning team to lose. If a building has an invalid max health on load or has no interfaces linking to it, this will be forced to false.

Each Building can also have its own Properties listings. These listings directly alter the Properties table defined in the Building's Lua script file. This is described further down.


# Building Class LUA File #
Each Building Controller has the ability to run a Lua script along side it to handle building behaviors. Special callbacks are made to it to control when the building gains/losses an interface, when the building is damaged, and when special events take place on the building such as power loss.


# Building Properties #
Also contained within the controller's Lua script file is a Properties table. This table can define variables used to control the building's functionality. These values can easily be set through the controller's XML file using its **Properties** child listings.

Each entry is headed by an **add** tag. The **Name** attribute states the name of the variable in the building's Properties table that is to be altered. The **Value** attribute contains the value to set the variable to.

The following demonstrates how this works. The property _TickRate_ is defined with an initial value of 1.0 in the Lua script file. However, the Properties entry in the building's XML file _overwrites_ this value with 0.5.

Lua:
```
MyBuilding =
{
	Name = "MyBuilding",

	-- Properties
	Properties =
	{
		TickRate = 1.0, -- Default value
	},
};
```

XML:
```
<?xml version="1.0" encoding="utf-8"?>
<Buildings>
  <Building Name="MyBuilding" Script="MyBuilding.lua" InitHealth="500" MaxHealth="500" MustBeDestroyed="1" />
    <Properties>
      <add key="TickRate" value="0.5" />
    </Properties>
</Buildings>
```

[Back](TechDoc_Architecture.md)