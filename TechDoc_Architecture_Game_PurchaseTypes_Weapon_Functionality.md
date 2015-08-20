[Back](TechDoc_Architecture_Game_PurchaseTypes_Weapon.md)

# Functionality #

## WeaponPurchaseType ##
Below are the methods defined in the WeaponPurchaseType module.

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

[Back](TechDoc_Architecture_Game_PurchaseTypes_Weapon.md)