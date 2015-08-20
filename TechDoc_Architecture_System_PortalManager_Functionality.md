[Back](TechDoc_Architecture_System_PortalManager.md)

# Functionality #

## !SPortalDef ##
Below are the methods defined in the !SPortalDef object.

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


### **_UpdateTextureInfo_** ###
**Purpose**:
Update the texture info

**Arguments**:
void

**Returns**:
void

**Note**:
Should be called when a new texture is to be used

### **_RenderPortal_** ###
**Purpose**:
Render the portal and update the texture

**Arguments**:
  * _pRenderer_ - `[In]` Renderer to use

**Returns**:
void


## PortalManager ##
Below are the methods defined in the PortalManager interface.

### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_Initialize_** ###
**Purpose**:
Perform one-time initialization

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


### **_Reset_** ###
**Purpose**:
Reset the manager

**Arguments**:
void

**Returns**:
void


### **_Update_** ###
**Purpose**:
Update portals

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


### **_MakeEntityPortal_** ###
**Purpose**:
Make an entity a portal by updating a texture on it with what another entity sees

**Arguments**:
  * _nEntityID_ - `[In]` ID of entity to make a portal
  * _nFrameSkip_ - `[In]` Number of frames to skip in-between updates to the portal
  * _szCameraEntity_ - `[In]` Name of entity to use as the camera
  * _szTexture_ - `[In]` Name of texture on entity to update dynamically

**Returns**:
TRUE on success, FALSE otherwise


### **_RemoveEntityPortal_** ###
**Purpose**:
Remove a portal from an entity

**Arguments**:
  * _nEntityID_ - `[In]` ID of entity

**Returns**:
void


# Sub Modules #

## !SPortalDef ##
Description: This structure defines a Portal Definition and holds on to all information needed to describe and update it. It also holds on to helper functions to initialize and update the portal.

Variables:
  * nEntityID - ID of entity that owns the portal
  * nCameraEntityID - ID of entity that acts as the camera for the portal
  * szPortalTexture - Name of texture that is being updated by the portal, owned by the owning entity's materials
  * nFrameSkip - Number of frames to skip in-between updates for the portal

Helper Variables:
  * nLastFrameID - Last frame ID updated
  * nPortalTextureID - ID of portal texture
  * nWidth - Texture width
  * nHeight - Texture height

[Back](TechDoc_Architecture_System_PortalManager.md)