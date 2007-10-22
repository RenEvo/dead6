////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CPortalManager.h
//
// Purpose: Dynamically updates a texture with 
//	"the world" viewed from a custom camera
//
// File History:
//	- 8/20/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CPORTALMANAGER_H_
#define _D6C_CPORTALMANAGER_H_

#include "Interfaces/IPortalManager.h"
#include "IEntitySystem.h"

// Max dimentions for portal images
#define PORTAL_MAX_WIDTH	(256)
#define PORTAL_MAX_HEIGHT	(256)

class CPortalManager : public IPortalManager
{
	// Last updated frame ID
	int nLastFrameID;

	// Portal map
	PortalMap m_Portals;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CPortalManager(void);
private:
	CPortalManager(CPortalManager const&) {}
	CPortalManager& operator =(CPortalManager const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CPortalManager(void);

	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: Perform one-time initialization
	////////////////////////////////////////////////////
	virtual void Initialize(void);

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: Clean up
	////////////////////////////////////////////////////
	virtual void Shutdown(void);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Reset the manager
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update portals
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags);

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s);

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
		char const* szTexture, int nFrameSkip = 5);

	////////////////////////////////////////////////////
	// RemoveEntityPortal
	//
	// Purpose: Remove a portal from an entity
	//
	// In:	nEntityID - ID of entity
	////////////////////////////////////////////////////
	virtual void RemoveEntityPortal(EntityId nEntityID);
};

#endif //_D6C_CPORTALMANAGER_H_