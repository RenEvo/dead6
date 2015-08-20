[Back](TechDoc_Architecture.md)

# Overview #

The CNC Building is the base building definition that all other building scripts inherit from. It defines all the common callback routines used by the Building Controller and includes useful features such as a State Machine and common callbacks shared by the client and server executions.


# [Functionality](TechDoc_Architecture_Game_Buildings_CNCBuilding_Functionality.md) #


# State Machine #

The CNC Building includes a State Machine. A building implementation can include its own states, but by default, the following are created:

  * Alive - Building is alive and with power
  * NoPower - Building is alive but without power
  * Dead - Building is not alive. Power does not affect this state.

Automatic state transitions to these types are handled by the CNC Building's implementation. Each state has a special callback defined for all forms of callbacks (for server, client and common) as well as a Begin and End state.

Common = 

&lt;Name&gt;



Server = 

&lt;Name&gt;

 

&lt;underscore&gt;

 Server

Client = 

&lt;Name&gt;

 

&lt;underscore&gt;

 Client

where 

&lt;Name&gt;

 is:
  * OnBeginState (no Server or Client version) - Transitioned to this state
  * OnEndState (no Server or Client version) - Transitioned from this state
  * OnInit - Building has initialized
  * OnShutdown - Building has been shutdown
  * OnUpdate - Game loop tick
  * OnReset - Building has reset to its initial state
  * OnDamaged - Building has been damaged
  * OnDestroyed - Building has been destroyed due to 0 health
  * OnEvent - Building event has occured

The following common functions can be used to interface with the State Machine:
  * SetState(state) - Transition to the _state_ State.
  * SetStateFunc(state, callback, func) - Set the _callback_ function (i.e. "OnInit") for the given _state_ State to the given _func_ Function.
  * CallStateFunc(func, ...) - Call the _func_ callback function (i.e. "OnInit\_Server") for the active state, passing in the variable arguments.


[Back](TechDoc_Architecture.md)