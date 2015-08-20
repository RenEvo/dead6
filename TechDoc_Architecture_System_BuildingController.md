[Back](TechDoc_Architecture.md)

# Overview #

A Building Controller is a logical entity that defines a building in the game. A controller has other entities that act as "interfaces" to it. An entity that interfaces a controller can be used to not only damage the building, but also get information about the building to the player via the HUD.

The Controller is also responsible for handling the building's game logic and its Lua script instance.

A Script Bind also exists that allows the Script System to communicate with the instance of this Building Controller. It takes the form of the "controller" table in the Building Controller's script table.

# [Functionality](TechDoc_Architecture_System_BuildingController_Functionality.md) #

# Adding an Interface #
Interfaces are added to a Building Controller in one of two ways: either A.) Automatically if the entity's script table has the **CNCBuilding** table defined in its _Properties_ table, or B.) If manually added.

## CNCBuilding Table ##
An example of where this table is defined is the GeomEntity entity.

This table contains two properties:
  * Team = Name of the team that owns the building
  * Class = Class type of the building to interface

These two values are used to construct the GUID of the Building Controller that should be interfaced. If valid, this entity is automatically added as an interface to the controller after the level has loaded and each time the level is reset.


[Back](TechDoc_Architecture.md)