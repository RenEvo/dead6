# Introduction #

The Dead 6 Core Technology is a collection of code and guidelines that add on to the existing content present in the CryENGINE2 public SDK.  The Core brings in multiple system and game features that create the rich gameplay experiences present in Command & Conquer Mode, the first-person perspective of the C&C battlefield.

The Dead 6 Core exists for two reasons:
  1. To satisfy the technological needs of Command & Crysis: The Dead 6 .
  1. To offer a stable platform for other development teams to create their own C&C Mode-based mods on the CryENGINE2 public SDK.

The Dead 6 Core achieves these tasks by utilizing multiple systems which manage such logic ranging from team and base management to building controls and functionality. It executes all the necessary components needed to make a successful C&C Mode-based mod in a speedy and secure manner while promising robustness and maintaining a strict "modtastic" code. The Core's architecture is designed in such a way to promise development teams **complete and total customization controls without the need of re-engineering**. This is achieved by exposing the functionality and interfacing of all the modules to external scripting patterns and XML-based feeds. This is not to say one cannot develop their own in-house technology on top of the Dead 6 Core. Mod teams will find it unnecessary to do so given the amount of control that is already accessible.

# Sections #

The following sections will outline the system and game features present in the Dead 6 Core. To illustrate the differences between these two sections:
  * A **system feature** is a functionality that exists internally and aid the system by managing and controlling items that enable the game to function correctly. Examples include: Team and Base Management, Building Controllers, and the Purchase System.
  * A **game feature** is a functionality that exists externally and adds on to the gameplay experience demonstrated by the system. Examples include: CNC Mode rules, player spawning, buildings and vehicles.

## System Features ##

### Low Level ###
  * [Dead6 Main Game Module](TechDoc_Architecture_System_Game.md)
  * [Dead6 GameRules](TechDoc_Architecture_System_GameRules.md)
  * [Dead6 Game Factory](TechDoc_Architecture_System_GameFactory.md)
  * [Dead6 Player Entity](TechDoc_Architecture_System_D6Player.md)
  * [Vehicle Movement - Harvester](TechDoc_Architecture_System_VMHarvester.md)
  * [Portal Manager](TechDoc_Architecture_System_PortalManager.md)

### CNC Logic ###
  * [Team Manager](TechDoc_Architecture_System_TeamManager.md)
  * [Base Manager](TechDoc_Architecture_System_BaseManager.md)
  * [Building Controller](TechDoc_Architecture_System_BuildingController.md)
  * [Purchase System](TechDoc_Architecture_System_Purchase.md)

## Game Features ##

### CNC Game Features ###
  * [CNC Game Rules (XML/Lua)](TechDoc_Architecture_Game_CNCRules.md)
  * [CNC Teams (XML/Lua)](TechDoc_Architecture_Game_CNCTeams.md)
  * [CNC Building Classes (XML/Lua)](TechDoc_Architecture_Game_CNCBuildingClasses.md)
  * [Harvester Flow](TechDoc_Architecture_Game_HarvesterFlow.md)
  * [Purchase Types](TechDoc_Architecture_Game_PurchaseTypes.md)

### Buildings ###
  * [CNC Building](TechDoc_Architecture_Game_Buildings_CNCBuilding.md)
  * [Vehicle Factory](TechDoc_Architecture_Game_Buildings_VehicleFactory.md)
  * [Refinery](TechDoc_Architecture_Game_Buildings_Refinery.md)
  * [Power Plant](TechDoc_Architecture_Game_Buildings_PowerPlant.md)

### Entities ###
  * [Building Light](TechDoc_Architecture_Game_Entity_BuildingLight.md)
  * [Building Sound](TechDoc_Architecture_Game_Entity_BuildingSound.md)
  * [Building Particle](TechDoc_Architecture_Game_Entity_BuildingParticle.md)
  * [Vehicle Spawn Point](TechDoc_Architecture_Game_Entities_VehSpawnPoint.md)

### Nodes ###
  * [General Nodes](TechDoc_Architecture_Game_Node_General.md)
  * [Harvester Nodes (See Harvester Flow)](TechDoc_Architecture_Game_HarvesterFlow.md)
  * [Building Controller Nodes](TechDoc_Architecture_Game_Node_BuildingController.md)
  * [Dead6 Player Nodes](TechDoc_Architecture_Game_Node_D6Player.md)

# Appendix #

[Crysis Codebase Changes](TechDoc_Architecture_CrysisCodeChange.md) - All changes to the Crysis SDK codebase are logged at this location.

[Return to Front Page](TechDoc.md)


