[Back](TechDoc_Architecture.md)

# Overview #

The Portal Manager retains and updates Portal Profiles which are created by the system at run-time based on game events. Portals act as recursively-rendered scenes "painted" onto a texture and drawn again in the active seen on top of static objects or directly on the HUD. They act as another set of eyes, letting the player see the world through a different camera. Such applications include a monitor located in the Refinery displaying what the Harvester sees, or a screen on the player's HUD showing what another player is seeing when they issue an "Enemy Spotted!" alert.

Each Portal Definition is bound to a specific texture ID which it updated dynamically based on a update time.

# [Functionality](TechDoc_Architecture_System_PortalManager_Functionality.md) #


# Creating a Portal #

You can create a portal to behave in one of two ways:

  * Bind it to a Static Object through a given texture
  * Bind it to the HUD through a given widget

TODO

[Back](TechDoc_Architecture.md)