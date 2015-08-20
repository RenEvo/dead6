[Back](TechDoc_Architecture_System_Game.md)

# Functionality #

## EditorGameListener ##
Below are the methods defined in the EditorGameListener interface.

### **_OnEditorGameStart_** ###
**Purpose**:
Called when the game has started

**Arguments**:
  * _pLocalPlayer_ - `[In]` Local player object

**Returns**:
void


### **_OnEditorGameEnd_** ###
**Purpose**:
Called when the game has ended

**Arguments**:
  * _pLocalPlayer_ - `[In]` Local player object

**Returns**:
void


## D6Game ##
Below are the methods defined in the D6Game module.

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


### **_GetD6GameRules_** ###
**Purpose**:
Returns OUR game rules

**Arguments**:
void

**Returns**:
void


### **_IsEditorGameStarted_** ###
**Purpose**:
Returns TRUE if the game in the editor has started

**Arguments**:
void

**Returns**:
void


### **_GetBaseManagerScriptBind_** ###
**Purpose**:
Returns the script bind for the base manager

**Arguments**:
void

**Returns**:
void


### **_GetTeamManagerScriptBind_** ###
**Purpose**:
Returns the script bind for the team manager

**Arguments**:
void

**Returns**:
void


### **_GetBuildingControllerScriptBind_** ###
**Purpose**:
Returns the script bind for the building controller

**Arguments**:
void

**Returns**:
void


### **_GetPortalManagerScriptBind_** ###
**Purpose**:
Returns the script bind for the portal manager

**Arguments**:
void

**Returns**:
void


### **_GetActorScriptBind_** ###
**Purpose**:
Returns the D6 Player script bind overload

**Arguments**:
void

**Returns**:
void


### **_GetGameRulesScriptBind_** ###
**Purpose**:
Returns the D6 GameRules script bind overload

**Arguments**:
void

**Returns**:
void


### **_AddEditorGameListener_** ###
**Purpose**:
Add a listener for the editor game

**Arguments**:
  * _pListener_ - `[In]` Listening object

**Returns**:
void


### **_RemoveEditorGameListener_** ###
**Purpose**:
Remove a listener for the editor game

**Arguments**:
  * _pListener_ - `[In]` Listening object

**Returns**:
void


### **_!ParseCNCRules_** ###
**Purpose**:
Parses the specified CNCRules file

**Arguments**:
  * _szXMLFile_ - `[In]` XML File to load

**Returns**:
TRUE on success or FALSE if an error occured

[Back](TechDoc_Architecture_System_Game.md)