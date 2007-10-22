////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// ScriptBind_PortalManager.h
//
// Purpose: Script binding for the portal manager
//
// File History:
//	- 8/20/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_SCRIPTBIND_PORTALMANAGER_H_
#define _D6C_SCRIPTBIND_PORTALMANAGER_H_

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IPortalManager;
struct ISystem;

class CScriptBind_PortalManager : public CScriptableBase
{
public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CScriptBind_PortalManager(ISystem *pSystem);
private:
	CScriptBind_PortalManager(CScriptBind_PortalManager const&) {}
	CScriptBind_PortalManager& operator =(CScriptBind_PortalManager const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CScriptBind_PortalManager(void);

	////////////////////////////////////////////////////
	// AttachTo
	//
	// Purpose: Attaches binding to a portal manager
	////////////////////////////////////////////////////
	void AttachTo(IPortalManager *pPortalManager);

public:
	////////////////////////////////////////////////////
	// MakeEntityPortal
	//
	// Purpose: Make an entity a portal entity
	//
	// In:	nEntityID - ID of the entity
	//		szCameraEntity - Name of the entity to use as
	//			the camera
	//		szTexture - Name of texture on entity to update
	//		nFrameSkip - How many frames to skip in-between
	//			updates to the portal
	//
	// Returns nil on error
	//
	// Note: See Portal Manager's MakeEntityPortal
	////////////////////////////////////////////////////
	int MakeEntityPortal(IFunctionHandler *pH, ScriptHandle nEntityID, char const* szCameraEntity,
		char const* szTexture, int nFrameSkip);

	////////////////////////////////////////////////////
	// RemoveEntityPortal
	//
	// Purpose: Remove a portal from an entity
	//
	// In:	nEntityID - ID of the entity
	//
	// Note: See Portal Manager's RemoveEntityPortal
	////////////////////////////////////////////////////
	int RemoveEntityPortal(IFunctionHandler *pH, ScriptHandle nEntityID);

private:
	////////////////////////////////////////////////////
	// RegisterGlobals
	//
	// Purpose: Registers any global values to the script
	//	system
	////////////////////////////////////////////////////
	void RegisterGlobals(void);

	////////////////////////////////////////////////////
	// RegisterMethods
	//
	// Purpose: Registers any binding methods to the script
	//	system
	////////////////////////////////////////////////////
	void RegisterMethods(void);

	IPortalManager *m_pPortalManager;
	ISystem *m_pSystem;
	IScriptSystem *m_pSS;
};

#endif //_D6C_SCRIPTBIND_PORTALMANAGER_H_
