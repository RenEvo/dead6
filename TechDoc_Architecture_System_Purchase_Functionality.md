[Back](TechDoc_Architecture_System_Purchase.md)

# Functionality #

## !SPurchaseDef ##
Below are the methods defined in the !SPurchaseDef object.

### **_Constructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


## !SPurchaseGroup ##
Below are the methods defined in the !SPurchaseGroup object.

## PurchaseType ##
Below are the methods defined in the PurchaseType interface.

### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_GetType_** ###
**Purpose**:
Returns purchase type

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


### **_Create_** ###
**Purpose**:
Create and initialize the entry

**Arguments**:
  * _nTeamID_ - `[In]` Owning team ID
  * _pData_ - `[In]` XML properties
  * _pDef_ - `[In]` Definition it belongs to

**Returns**:
void


### **_PreProcess_** ###
**Purpose**:
Confirm if player can buy this item

**Arguments**:
  * _pPlayer_ - `[In]` Player who wants to buy it

**Returns**:
void


### **_Process_** ###
**Purpose**:
Process the purchase for the given player

**Arguments**:
  * _pPlayer_ - `[In]` Player who bought it

**Returns**:
void


## PurchaseSystem ##
Below are the methods defined in the PurchaseSystem interface.

### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_CreatePurchaseType_** ###
**Purpose**:
Create the requested purchase type

**Arguments**:
  * _nType_ - `[In]` Type requested

**Returns**:
void


### **_Initialize_** ###
**Purpose**:
Prepare the system

**Arguments**:
void

**Returns**:
void


### **_Shutdown_** ###
**Purpose**:
Clean up

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


### **_Reset_** ###
**Purpose**:
Reset the system

**Arguments**:
void

**Returns**:
void

**Note**:
Should be called when a level is being loaded, before XML files are parsed

### **_ResetGame_** ###
**Purpose**:
Reset to prepare for a new instance of the game with the same purchase settings

**Arguments**:
  * _bStart_ - `[In]` TRUE if game is starting, FALSE if game is exiting

**Returns**:
void

**Note**:
Called by Editor when re-entering the game

### **_LoadTeamPurchaseSettings_** ###
**Purpose**:
Load and prepare a team's puchase settings from their XML file

**Arguments**:
  * _nTeamID_ - `[In]` Team ID
  * _pPurchases_ - `[In]` Purchases node that defines the purchase settings for this team

**Returns**:
TRUE if purchase settings were loaded


### **_GetTeamPurchaseGroups_** ###
**Purpose**:
Returns all groups belonging to a team

**Arguments**:
  * _nTeamID_ - `[In]` Team ID
  * _purchases_ - `[Out]` Team purchases

**Returns**:
TRUE if purchase groups were returned


### **_GetTeamPurchaseGroup_** ###
**Purpose**:
Returns a team's specified purchase group

**Arguments**:
  * _group_ - `[Out]` Team purchase group
  * _nTeamID_ - `[In]` TeamID
  * _szGroup_ - `[In]` Name of the group

**Returns**:
TRUE if purchase group was returned


# Sub Modules #

## !SPurchaseDef ##

Description: Describes a purchase definition, including its  name and cost, as well as its Type listings for fast iteration processing.

Variables:
  * nPurchaseType - Bit-mask for faster processing of what types are defined
  * PurchaseTypes - List of purchase type objects
  * nCost - Item's cost
  * nInflation - Item's inflation cost
  * szName - Name of item
  * szDescription - Description of item
  * szIcon - Texture icon used to represent the item
  * Prerequisites - List of building prerequisites

## !SPurchaseGroup ##

Description: Describes a purchase group with a name and listing of purchase definitions.

Variables:
  * szName - Name of the group
  * PurchaseDefs - List of purchase definitions under its banner

[Back](TechDoc_Architecture_System_Purchase.md)