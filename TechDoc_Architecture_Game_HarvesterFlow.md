[Back](TechDoc_Architecture.md)

# Overview #

The Harvester Flog logic is the series of Flow Nodes contained in a unique Flow Graph on the level that defines the behavior of a Harvester in the world. Each Harvester is created through the existence of a Harvester Controller node. This node has output ports that can be used to define the behavior for when the Harvester is purchased, moving to/from the Tiberium Field, and when the Harvester is loading/unloading its tank.

A team can have an infinite amount of Harvesters. (Actually, it can have 65536 Harvesters at one time.) Each Controller node represents one Harvester for the specified team. **It is important to note that each Controller must exist in its OWN FLOW GRAPH!** The Controller utilizes the secondary default entity slot of the graph to transfer the necessary information needed by the other Harvester nodes to function. Putting more than one Controller on the same graph will cause conflict between the two separate flows.

Further down, each Node will be explained. The Functionality present here is for the Node's C++ classes.

# [Functionality](TechDoc_Architecture_Game_HarvesterFlow_Functionality.md) #

# Flow Nodes #

There are four separate Flow Nodes utilized by the Harvester Flow to achieve specific results for controlling the Harvester. Below, each of these Nodes are explained, as well as their port's purposes.

## Harvester:Controller ##
Purpose: This Node acts as the main controller for the Harvester. It creates the Harvester and handles the main logic behind it. Specific Output ports allow you to define the behavior of the Harvester when specific events occur throughout its life.

Input Ports:
  * Team - (String) The team that owns this Harvester.
  * Create At - (Vector) Where to create the Harvester at if it doesn't go through the Vehicle Factory.
  * Create Dir - (Float) Degree of facing of Harvester once created if it doesn't go through the Vehicle Factory. A degree of 0 is the forward facing of the entity in its local space.
  * Use Factory - (Bool) TRUE to have the Harvester be created through the Vehicle Factory or FALSE to use the properties listed above. Defaults to FALSE if no Vehicle Factory for the team exists.

Output Ports:
  * Purchased - Triggered when the harvester is (re)purchased
  * ToField - Triggered when the harvester is to begin its route to the Tiberium Field
  * FromField - Triggered when the harvester is to begin its route back from the Tiberium Field
  * Loading - Triggered when the harvester is starting to load up
  * Loaded - Triggered when the harvester has loaded
  * Unloading - Triggered when the harvester is starting to unload
  * Unloaded - Triggered when the harvester has unloaded


## Harvester:Signal ##
Purpose: This Node acts as a signaler to the controller and is used to tell the Harvester when to perform certain actions.

Input Ports:
  * Signal - Call to make the signal to the Harvester.
  * Value - (Enum) Signal to send. See _Signals_ section below for available signals and what they mean/do.

Output Ports:
  * Done - Triggered when the signal has been made. This does not mean the signal has been processed by the controller, but that the signal has been queued up for the controller to process!

Signals:
  * MoveToField - Calls "ToField" Output Port on controller
  * MoveFromField - Calls "FromField" Output Port on controller
  * Load - Causes Harvester to begin loading up on tiberium
  * Unload - Causes Harvester to begin unloading its tank
  * Abort - Causes Harvester to stop loading/unloading and continue on with whatever it got. The Loaded and Unloaded signals on the Harvester are subsequently called.
  * StartEngine - Causes Harvester to start its engine so it can move
  * StopEngine - Causes Harvester to stop its engine so it can't move


## Harvester:Goto ##
Purpose: This Node is used to move the Harvester to a new location in the world. The Harvester moves using the vehicle physics engine in CryENGINE 2 with the movement properties laid out in the vehicle's definition.

Input Ports:
  * Move - Call to have the Harvester begin to move to the designated location.
  * Goto Position - (Vector) Where the Harvester should move to. Note that the 'Z' component is not used.
  * Top Speed - (Float) Maximum speed the Harvester can use when moving to the point.
  * Low Speed - (Float) Minimum speed the Harvester can use when slowing down near the point.
  * Slow Distance - (Float) How far from the Goto Position before the Harvester can begin to slow down. Its speed is based on an linear interpretation between its distance to the point and the ratio of Low Speed to Top Speed. Keep at '0' if Harvester should maintain Top Speed all the way to the point.
  * Break Past - (Bool) TRUE if Harvester should apply breaks once it reaches the Goto Position. FALSE will cause the Harvester to keep its current speed.
  * Reverse - (Bool) TRUE if Harvester should drive in reverse to reach the point. Note that steering is not as good in reverse, so it may be more difficult for the Harvester to complete the Goto in reverse.
  * Lock Steering - (Bool) TRUE if Harvester cannot use steering while moving to the Goto Position. Note that this may prevent the Harvester from successfully moving to the Goto Position! Used to ensure Harvester maintains its direction when driving in reverse.
  * Timeout - (Float) How long the Harvester has to make it to its point before timing out. Keep at 0 to ignore the Timeout logic. Can be used to ensure Harvester maintains its course in case it loses its way or something blocks it.

Output Ports:
  * Done - Triggered when harvester is at or passes the location
  * Start - Triggered when harvester begins to move to the location
  * TimedOut - Triggered when harvester times out and did not make it to the location


## Harvester:Turn ##
Purpose: Used to turn the Harvester to a new facing on the dime. Vehicle physics are disabled and not used while this is occurring to ensure a smooth, accurate facing is achieved.

Input Ports:
  * Turn - Call to begin turning the Harvester.
  * Face Degree - (Float) Degree of facing Harvester should move towards. A degree of 0 is the forward facing of the entity in its local space.
  * Speed - (Float) How fast the Harvester turns in degrees per second.
  * Timeout - (Float) How long the Harvester has to make it to its point before timing out. Keep at 0 to ignore the Timeout logic.

Output Ports:
  * Done - Triggered when harvester is at or passes the degree of facing
  * Start - Triggered when harvester begins to turn
  * TimedOut - Triggered when harvester times out and did not complete the turn


## Example Graph ##
To see an example graph, please view the Harvester\_Test map provided in the latest build.

![![](http://dead6.googlecode.com/svn/trunk/Media/Harvester_Flow_01.thumb.png)](http://dead6.googlecode.com/svn/trunk/Media/Harvester_Flow_01.png)

[Back](TechDoc_Architecture.md)