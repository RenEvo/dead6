[Back](TechDoc_Architecture.md)

# Overview #

The Dead6 Player Entity derives from the Crysis Player Entity and is used to expand on the player for CNC-specific logic, including player credits. The Dead6 Game Factory ensures this module is used to represent in the player in place of the one used in Crysis.

The Dead6 Player also takes advantage of an overloaded Actor Script Bind, known as the Dead6 Player Script Bind. It inherits all functionality of the Actor Script Bind, but adds accessibility for altering a player's credits from the Script System.

# [Functionality](TechDoc_Architecture_System_D6Player_Functionality.md) #

[Back](TechDoc_Architecture.md)