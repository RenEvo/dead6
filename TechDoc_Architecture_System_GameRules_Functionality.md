[Back](TechDoc_Architecture_System_GameRules.md)

# Functionality #

## D6GameRules ##
Below are the methods defined in the D6GameRules module.

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


### **_ClearAllTeams_** ###
**Purpose**:
Clears all teams that are currently loaded in the gamerules and removes all

**Arguments**:
void

**Returns**:
void


### **_GetServerStateScript_** ###
**Purpose**:
Returns server state script object

**Arguments**:
void

**Returns**:
void


### **_GetClientStateScript_** ###
**Purpose**:
Returns client state script object

**Arguments**:
void

**Returns**:
void


### **_GetHitTable_** ###
**Purpose**:
Returns hit script object

**Arguments**:
void

**Returns**:
void


### **_GetExplosionTable_** ###
**Purpose**:
Returns explosion script object

**Arguments**:
void

**Returns**:
void


### **_CreateScriptHitInfo_** ###
**Purpose**:
Create a script table that contains the given hit info

**Arguments**:
  * _hitInfo_ - `[In]` Hit info to build from
  * _scriptHitInfo_ - `[Out]` Script table containing hit fo

**Returns**:
void


### **_CreateScriptExplosionInfo_** ###
**Purpose**:
Create a script table that contains the given explosion info

**Arguments**:
  * _explosionInfo_ - `[In]` Explosion info to build from
  * _scriptExplosionInfo_ - `[Out]` Script table containing  explosion info

**Returns**:
void

[Back](TechDoc_Architecture_System_GameRules.md)