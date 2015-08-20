[Back](TechDoc_Architecture.md)

# Overview #

The Refinery derives from [CNC Building](TechDoc_Architecture_Game_Buildings_CNCBuilding.md) and has the functionality to reward all players on its team with credits every X seconds while it is alive. The amounts can change when the building loses power.

# [Functionality](TechDoc_Architecture_Game_Buildings_Refinery_Functionality.md) #

# Properties #
The Refinery makes use of the following properties:
  * TickRate\_Power - How often the credit tick occurs when powered, in seconds
  * TickRate\_NoPower - How often the credit tick occurs when underpowered, in seconds
  * TickValue\_Power - How many credits the players get per tick when powered
  * TickValue\_NoPower - How many credits the players get per tick when underpowered

[Back](TechDoc_Architecture.md)