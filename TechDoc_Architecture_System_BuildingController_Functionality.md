[Back](TechDoc_Architecture_System_BuildingController.md)

# Functionality #

## !SControllerEvent ##
Below are the methods defined in the !SControllerEvent object.

## BuildingControllerEventListener ##
Below are the methods defined in the BuildingControllerEventListener interface.

### **_OnBuildingControllerEvent_** ###
**Purpose**:
Called when event occurs on controller

**Arguments**:
  * _event_ - `[In]` Event that occured
  * _nGUID_ - `[In]` Controller's BuildingGUID
  * _pController_ - `[In]` Building controller object

**Returns**:
void

**Note**:
See SControllerEvent

## BuildingController ##
Below are the methods defined in the BuildingController interface.

### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_GetMemoryStatistics_** ###
**Purpose**:
Used by memory management

**Arguments**:
  * _s_ - `[In]` Cry Sizer object

**Returns**:
void


### **_Initialize_** ###
**Purpose**:
Initialize the controller

**Arguments**:
  * _nGUID_ - `[In]` Building controller's GUID

**Returns**:
void


### **_Shutdown_** ###
**Purpose**:
Shut the controller down

**Arguments**:
void

**Returns**:
void


### **_Update_** ###
**Purpose**:
Update the controller

**Arguments**:
  * _bHaveFocus_ - `[In]` TRUE if game has focus
  * _nControllerUpdateFlags_ - `[In]` Controller-specific update flags (see EControllerUpdateFlags)
  * _nUpdateFlags_ - `[In]` Update flags

**Returns**:
void


### **_Reset_** ###
**Purpose**:
Reset the controller back to its

**Arguments**:
void

**Returns**:
void


### **_BeforeValidate_** ###
**Purpose**:
Prepare the controller for validation

**Arguments**:
void

**Returns**:
TRUE if validation should proceed, or FALSE if it should be skipped


### **_Validate_** ###
**Purpose**:
Validate the controller by checking for

**Arguments**:
void

**Returns**:
void


### **_IsValidated_** ###
**Purpose**:
Returns TRUE if building has been validated

**Arguments**:
void

**Returns**:
void


### **_LoadFromXml_** ###
**Purpose**:
Define the building controller's attributes from the given XML node

**Arguments**:
  * _pNode_ - `[In]` XML Node containing its attributes

**Returns**:
void


### **_!GetGUID_** ###
**Purpose**:
Return the controller's GUID

**Arguments**:
void

**Returns**:
void


### **_GetScriptTable_** ###
**Purpose**:
Return the script table attatched to the controller's script

**Arguments**:
void

**Returns**:
void


### **_GetTeam_** ###
**Purpose**:
Returns owning team's ID or TEAMID\_NOTEAM if there is no team

**Arguments**:
void

**Returns**:
void


### **_GetTeamName_** ###
**Purpose**:
Returns owning team's name

**Arguments**:
void

**Returns**:
void


### **_GetClass_** ###
**Purpose**:
Returns building class or BC\_INVALID if there is no class

**Arguments**:
void

**Returns**:
void


### **_GetClassName_** ###
**Purpose**:
Returns owning class' name

**Arguments**:
void

**Returns**:
void


### **_AddInterface_** ###
**Purpose**:
Mark an entity as an interface to this controller

**Arguments**:
  * _pEntity_ - `[In]` Entity to use

**Returns**:
TRUE if entity is acting as an interface to it now or FALSE if not

**Note**:
Sets the ENTITY\_FLAG\_ISINTERFACE on the enemy so the same entity cannot interface more than one building at the same time!

### **_RemoveInterface_** ###
**Purpose**:
Remove an entity as an interface to this controller

**Arguments**:
  * _pEntity_ - `[In]` Entity to remove

**Returns**:
void


### **_HasInterface_** ###
**Purpose**:
Returns TRUE if the given entity is an

**Arguments**:
  * _nEntityId_ - `[In]` Entity to check
  * _pEntity_ - `[In]` Entity to check

**Returns**:
void


### **_IsVisible_** ###
**Purpose**:
Returns TRUE if the controller is visible to the player (i.e. player is focused on an

**Arguments**:
void

**Returns**:
void


### **_SetMustBeDestroyed_** ###
**Purpose**:
Set if building wants to be destroyed for the team to lose

**Arguments**:
  * _b_ - `[In]` TRUE if it must be destroyed

**Returns**:
void

**Note**:
Will be ignored if building is not interfaced

### **_MustBeDestroyed_** ###
**Purpose**:
Returns TRUE if building must be destroyed for the team to lose

**Arguments**:
void

**Returns**:
void


### **_AddEventListener_** ###
**Purpose**:
Add an event listener to controller

**Arguments**:
  * _pListener_ - `[In]` Listening object

**Returns**:
void


### **_RemoveEventListener_** ###
**Purpose**:
Remove an event listener from controller

**Arguments**:
  * _pListener_ - `[In]` Listening object

**Returns**:
void


### **_AddScriptEventListener_** ###
**Purpose**:
Add an event listener to an entity's script

**Arguments**:
  * _nID_ - `[In]` Entity Id

**Returns**:
TRUE if added

**Note**:
Will call the entity's script's "OnBuildingEvent" when an event occurs

### **_RemoveScriptEventListener_** ###
**Purpose**:
Remove an event listener from an entity's script

**Arguments**:
  * _nID_ - `[In]` Entity Id

**Returns**:
void


### **_OnClientHit_** ###
**Purpose**:
Call when a hit occurs on the client

**Arguments**:
  * _hitinfo_ - `[In]` Hit information

**Returns**:
void


### **_OnServerHit_** ###
**Purpose**:
Call when a hit occurs on the server

**Arguments**:
  * _hitinfo_ - `[In]` Hit information

**Returns**:
void


### **_OnClientExplosion_** ###
**Purpose**:
Call when an explosion occurs on the client

**Arguments**:
  * _explosionInfo_ - `[In]` Explosion information
  * _fObstruction_ - `[In]` Obstruction ratio (0 = fully obstructed, 1 = fully visible)
  * _nInterfaceId_ - `[In]` ID of interface entity that was hit by this explosion

**Returns**:
void


### **_OnServerExplosion_** ###
**Purpose**:
Call when an explosion occurs on the server

**Arguments**:
  * _explosionInfo_ - `[In]` Explosion information
  * _fObstruction_ - `[In]` Obstruction ratio (0 = fully obstructed, 1 = fully visible)
  * _nInterfaceId_ - `[In]` ID of interface entity that was hit by this explosion

**Returns**:
void


### **_HasPower_** ###
**Purpose**:
Returns TRUE if the building has power

**Arguments**:
void

**Returns**:
void


### **_SetPower_** ###
**Purpose**:
Set the power status of the building

**Arguments**:
  * _bPowered_ - `[In]` TRUE to give it power, FALSE to take it away

**Returns**:
void


### **_GetHealth_** ###
**Purpose**:
Get the building's current health amount

**Arguments**:
void

**Returns**:
void


### **_IsAlive_** ###
**Purpose**:
Returns TRUE if the building is alive or FALSE if it is dead

**Arguments**:
void

**Returns**:
void


## ScriptBind\_BuildingController ##
Below are the methods defined in the ScriptBind\_BuildingController module.

### **_RegisterGlobals_** ###
**Purpose**:
Called to register global variables

**Arguments**:
void

**Returns**:
void


### **_RegisterMethods_** ###
**Purpose**:
Called to register function methods

**Arguments**:
void

**Returns**:
void


### **_GetController_** ###
**Purpose**:
Return the controller

**Arguments**:
  * _pH_ - `[In]` Function handler to get controller from

**Returns**:
void


### **_Constructor_** ###
**Purpose**:
Undefined.

**Arguments**:
  * _pGameFW_ - `[In]` Game frame work object
  * _pSystem_ - `[In]` System object

**Returns**:
void


### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_AttachTo_** ###
**Purpose**:
Bind to a controller

**Arguments**:
  * _pController_ - `[In]` Controller to be bound

**Returns**:
void


### **_GetHealth_** ###
**Purpose**:
Get the health of the building

**Arguments**:
void

**Returns**:
void


### **_IsAlive_** ###
**Purpose**:
Returns TRUE if building is alive

**Arguments**:
void

**Returns**:
void


### **_HasPower_** ###
**Purpose**:
Returns TRUE if building has power

**Arguments**:
void

**Returns**:
void


### **_SetPower_** ###
**Purpose**:
Set the power of the building

**Arguments**:
  * _bPower_ - `[In]` TRUE to give it power, FALSE to take it away

**Returns**:
void


### **_AddEventListener_** ###
**Purpose**:
Add the given entity to the entity event listener on the controller

**Arguments**:
  * _nEntityID_ - `[In]` Entity Id to add

**Returns**:
'1' if succeeded, nil if not


### **_RemoveEventListener_** ###
**Purpose**:
Remove the given entity from the entity event listener on the controller

**Arguments**:
  * _nEntityID_ - `[In]` Entity Id to add

**Returns**:
void


### **_!GetGUID_** ###
**Purpose**:
Return building's GUID

**Arguments**:
void

**Returns**:
void


### **_GetClass_** ###
**Purpose**:
Return building's class ID

**Arguments**:
void

**Returns**:
void


### **_GetClassName_** ###
**Purpose**:
Return building's class name

**Arguments**:
void

**Returns**:
void


### **_GetTeam_** ###
**Purpose**:
Return building's team ID

**Arguments**:
void

**Returns**:
void


### **_GetTeamName_** ###
**Purpose**:
Return building's team name

**Arguments**:
void

**Returns**:
void


[Back](TechDoc_Architecture_System_BuildingController.md)