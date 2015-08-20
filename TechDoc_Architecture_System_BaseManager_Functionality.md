[Back](TechDoc_Architecture_System_BaseManager.md)

# Functionality #

## BaseManager ##
Below are the methods defined in the BaseManager interface.

### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_Initialize_** ###
**Purpose**:
One-time initialization at the start

**Arguments**:
void

**Returns**:
void


### **_Shutdown_** ###
**Purpose**:
One-time clean up at the end

**Arguments**:
void

**Returns**:
void


### **_Update_** ###
**Purpose**:
Update the controllers

**Arguments**:
  * _bHaveFocus_ - `[In]` TRUE if game has focus
  * _nUpdateFlags_ - `[In]` Update flags

**Returns**:
void


### **_GetMemoryStatistics_** ###
**Purpose**:
Used by memory management

**Arguments**:
  * _s_ - `[In]` Cry Sizer object

**Returns**:
void


### **_LoadBuildingControllers_** ###
**Purpose**:
Load building definitions contained in an XML node

**Arguments**:
  * _pNode_ - `[In]` XML node to extract from

**Returns**:
TRUE on success or FALSE on error

**Note**:
Used in conjunction with CNCRules parsing

### **_CreateBuildingController_** ###
**Purpose**:
Create a building controller

**Arguments**:
  * _pAttr_ - `[In]` XML node containing attributes for it which overwrite the base attributes
  * _pBaseAttr_ - `[In]` XML node containing base attributes for it
  * _ppController_ - `[Out]` Optional controller catcher
  * _szName_ - `[In]` Class name to use for controller
  * _szTeam_ - `[In]` Team that owns the controller

**Returns**:
the building's GUID or GUID\_INVALID on error


### **_Reset_** ###
**Purpose**:
Clears all loaded BCs and prepares for new controller definitions

**Arguments**:
void

**Returns**:
void

**Note**:
Should be called at the start of a level load

### **_ResetGame_** ###
**Purpose**:
Called when the game is reset, such as when the editor game starts up

**Arguments**:
  * _bGameStart_ - `[In]` TRUE if game is starting, FALSE if game is stopping

**Returns**:
void


### **_Validate_** ###
**Purpose**:
Validates all controllers by (re)setting any interfaces to them and checks for errors

**Arguments**:
  * _nGUID_ - `[In]` Controller GUID to validate or GUID\_INVALID to validate all controlles

**Returns**:
void

**Note**:
Should be called at least once after level has loaded and when game is reset

### **_FindBuildingController_** ###
**Purpose**:
Find the building controller with the given GUID

**Arguments**:
  * _nGUID_ - `[In]` GUID to use

**Returns**:
building controller's object or NULL on error


### **_FindBuildingController_** ###
**Purpose**:
Find the building controller with the given team and class IDs

**Arguments**:
  * _nClassID_ - `[In]` Building class ID
  * _nTeamID_ - `[In]` Team ID

**Returns**:
building controller's object or NULL on error


### **_FindBuildingController_** ###
**Purpose**:
Find the building controller with the given team and class names

**Arguments**:
  * _szClass_ - `[In]` Building class name
  * _szTeam_ - `[In]` Team name

**Returns**:
building controller's object or NULL on error


### **_GetClassName_** ###
**Purpose**:
Returns the name of the given class

**Arguments**:
  * _nClassID_ - `[In]` ID of the class

**Returns**:
void


### **_GetClassId_** ###
**Purpose**:
Returns the ID of the class with the given ID

**Arguments**:
  * _szName_ - `[In]` Name to search for

**Returns**:
void


### **_IsValidClass_** ###
**Purpose**:
Returns TRUE if the specified class ID or name is valid

**Arguments**:
  * _nID_ - `[In]` Class ID
  * _szName_ - `[In]` Name of the class

**Returns**:
void

**Note**:
Using the name is slower than the ID!

### **_!GenerateGUID_** ###
**Purpose**:
Generates a Building GUID given the team and class names

**Arguments**:
  * _szClass_ - `[In]` Class name
  * _szTeam_ - `[In]` Team name

**Returns**:
building GUID or GUID\_INVALID on error


### **_GetBuildingFromInterface_** ###
**Purpose**:
Returns the Building GUID that is interfaced by the given entity

**Arguments**:
  * _nEntityId_ - `[In]` ID of entity that is the interface
  * _ppController_ - `[Out]` Pointer to controller object

**Returns**:
Building GUID or GUID\_INVALID on error


### **_AddBuildingControllerEventListener_** ###
**Purpose**:
Add a listener to receive callbacks on a building controller during certain events

**Arguments**:
  * _nGUID_ - `[In]` Building GUID
  * _pListener_ - `[In]` Listening object

**Returns**:
void

**Note**:
See EControllerEvent

### **_RemoveBuildingControllerEventListener_** ###
**Purpose**:
Remove a listener

**Arguments**:
  * _nGUID_ - `[In]` Building GUID
  * _pListener_ - `[In]` Listening object

**Returns**:
void


### **_ClientHit_** ###
**Purpose**:
Call when a hit occurs on the client

**Arguments**:
  * _hitinfo_ - `[In]` Hit information

**Returns**:
void


### **_ServerHit_** ###
**Purpose**:
Call when a hit occurs on the server

**Arguments**:
  * _hitinfo_ - `[In]` Hit information

**Returns**:
void


### **_ServerExplosion_** ###
**Purpose**:
Call when an explosion occurs on the server

**Arguments**:
  * _affectedEntities_ - `[In]` Map of affected entities
  * _explosionInfo_ - `[In]` Explosion information

**Returns**:
void


### **_ClientExplosion_** ###
**Purpose**:
Call when an explosion occurs on the client

**Arguments**:
  * _affectedEntities_ - `[In]` Map of affected entities
  * _explosionInfo_ - `[In]` Explosion information

**Returns**:
void


### **_SetBasePower_** ###
**Purpose**:
Set the power on all buildings

**Arguments**:
  * _bState_ - `[In]` TRUE to turn power on, FALSE to turn power off
  * _nTeamID_ - `[In]` Team that owns the base

**Returns**:
void


## ScriptBind\_BaseManager ##
Below are the methods defined in the ScriptBind\_BaseManager module.

### **_Constructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

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
Attaches binding to a base manager

**Arguments**:
void

**Returns**:
void


### **_FindBuilding_** ###
**Purpose**:
Return a controller's script table by using the team and class names

**Arguments**:
  * _szClass_ - `[In]` Building class name
  * _szTeam_ - `[In]` Owning team name

**Returns**:
controller's script table or nil on error


### **_SetBasePower_** ###
**Purpose**:
Set the power on all buildings

**Arguments**:
  * _bState_ - `[In]` TRUE to turn power on, FALSE to turn power off
  * _nTeamID_ - `[In]` Team that owns the base

**Returns**:
void


### **_RegisterGlobals_** ###
**Purpose**:
Registers any global values to the script system

**Arguments**:
void

**Returns**:
void


### **_RegisterMethods_** ###
**Purpose**:
Registers any binding methods to the script system

**Arguments**:
void

**Returns**:
void



# Sub Modules #

## CBaseManagerEntitySink ##

Description: This sub-module derives from the IEntitySystemSink and is used to listen for entity creation/removing and events such as hit reports. It works with the Base Manager to control Building Controller interface adding/removing and OnDamage reports.

[Back](TechDoc_Architecture_System_BaseManager.md)