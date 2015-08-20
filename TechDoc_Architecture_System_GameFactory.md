[Back](TechDoc_Architecture.md)

# Overview #

The Dead6 Game Factory module replaces the Crysis Game Factory and is used to force game entities such as the Player and Vehicles to use the derived Dead6 entity replacements.


# Additions/Replacements #

The following items have been added/replaced in comparison to the Crysis Game Factory:

  * "Player" - Player class (_Changed from **CPlayer** to **CD6Player**_)
  * "Harvester" - Vehicle Movement class (_Added for **CVehicleMovementHarvester**_)
  * "Gamerules" - Main Gamerules class (_Changed from **CGameRules** to **CD6GameRules**_)

[Back](TechDoc_Architecture.md)