////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// IPortalManager.h
//
// Purpose: Interface object
//	Describes a portal manager that can dynamically
//	update a texture with "the world" viewed from a
//	custom camera
//
// File History:
//	- 8/20/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_IPORTALMANAGER_H_
#define _D6C_IPORTALMANAGER_H_

// Portal definition
struct SPortalDef
{
	// Entity IDs
	EntityId nEntityID;
	EntityId nCameraEntityID;

	// Portal properties
	string szPortalTexture;
	int nFrameSkip;

	// Helpers
	int nLastFrameID;	// Last frame ID updated
	int nPortalTextureID;	// ID of portal texture
	int nWidth;			// Texture width
	int nHeight;		// Texture height

	// Temp buffer used for transfering from buffers
	static unsigned char *pBuffer;

	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	SPortalDef(void);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~SPortalDef(void);

	////////////////////////////////////////////////////
	// UpdateTextureInfo
	//
	// Purpose: Update the texture info
	//
	// Note: Should be called when a new texture is to
	//	be used
	////////////////////////////////////////////////////
	virtual void UpdateTextureInfo(void);

	////////////////////////////////////////////////////
	// RenderPortal
	//
	// Purpose: Render the portal and update the texture
	//
	// In:	pRenderer - Renderer to use
	////////////////////////////////////////////////////
	virtual void RenderPortal(struct IRenderer *pRenderer);
};
typedef std::map<EntityId, SPortalDef*> PortalMap;

struct IPortalManager
{
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~IPortalManager(void) {}

	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: Perform one-time initialization
	////////////////////////////////////////////////////
	virtual void Initialize(void) = 0;

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: Clean up
	////////////////////////////////////////////////////
	virtual void Shutdown(void) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Reset the manager
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update portals
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags) = 0;

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s) = 0;

	////////////////////////////////////////////////////
	// MakeEntityPortal
	//
	// Purpose: Make an entity a portal by updating
	//	a texture on it with what another entity sees
	//
	// In:	nEntityID - ID of entity to make a portal
	//		szCameraEntity - Name of entity to use as the
	//			camera
	//		szTexture - Name of texture on entity to
	//			update dynamically
	//		nFrameSkip - Number of frames to skip in-between
	//			updates to the portal
	//
	// Returns TRUE on success, FALSE otherwise
	////////////////////////////////////////////////////
	virtual bool MakeEntityPortal(EntityId nEntityID, char const* szCameraEntity,
		char const* szTexture, int nFrameSkip = 5) = 0;

	////////////////////////////////////////////////////
	// RemoveEntityPortal
	//
	// Purpose: Remove a portal from an entity
	//
	// In:	nEntityID - ID of entity
	////////////////////////////////////////////////////
	virtual void RemoveEntityPortal(EntityId nEntityID) = 0;
};

#endif //_D6C_IPORTALMANAGER_H_