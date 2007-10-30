////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CBaseManager.h
//
// Purpose: Monitors building logic on the field
//
// File History:
//	- 7/21/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CBASEMANAGER_H_
#define _D6C_CBASEMANAGER_H_

#include "IBaseManager.h"
#include "IBuildingController.h"
#include "IEntitySystem.h"

class CBaseManagerEntitySink;
class CBaseManager : public IBaseManager
{
protected:
	friend class CBaseManagerEntitySink;
	CBaseManagerEntitySink *m_pSink;

	// Class name repository
	typedef std::map<BuildingClassID, string> ClassRepository;
	BuildingClassID m_nClassNameIDGen;
	ClassRepository m_ClassNameList;

	// Building controllers
	typedef std::map<BuildingGUID, IBuildingController*> ControllerList;
	ControllerList m_ControllerList;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBaseManager(void);
private:
	CBaseManager(CBaseManager const&) {}
	CBaseManager& operator =(CBaseManager const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBaseManager(void);

public:
	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(class ICrySizer *s);

	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: One-time initialization at the start
	////////////////////////////////////////////////////
	virtual void Initialize(void);

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: One-time clean up at the end
	////////////////////////////////////////////////////
	virtual void Shutdown(void);
	
	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update the controllers
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags);

	////////////////////////////////////////////////////
	// LoadBuildingControllers
	//
	// Purpose: Load building definitions contained in an
	//	XML node
	//
	// In:	pNode - XML node to extract from
	//
	// Returns TRUE on success or FALSE on error
	//
	// Note: Used in conjunction with CNCRules parsing
	////////////////////////////////////////////////////
	virtual bool LoadBuildingControllers(XmlNodeRef pNode);

	////////////////////////////////////////////////////
	// CreateBuildingController
	//
	// Purpose: Create a building controller
	//
	// In:	szTeam - Team that owns the controller
	//		szName - Class name to use for controller
	//		pBaseAttr - XML node containing base attributes for it
	//		pAttr - XML node containing attributes for it
	//			which overwrite the base attributes
	//
	// Out:	ppController - Optional controller catcher
	//
	// Returns the building's GUID or GUID_INVALID on error
	////////////////////////////////////////////////////
	virtual BuildingGUID CreateBuildingController(char const* szTeam, char const* szName, XmlNodeRef pBaseAttr,
		XmlNodeRef pAttr, struct IBuildingController **ppController = NULL);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Clears all loaded BCs and prepares
	//	for new controller definitions
	//
	// Note: Should be called at the start of a level
	//	load
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// ResetGame
	//
	// Purpose: Called when the game is reset, such as
	//	when the editor game starts up
	//
	// In:	bGameStart - TRUE if game is starting,
	//	FALSE if game is stopping
	////////////////////////////////////////////////////
	virtual void ResetGame(bool bGameStart);

	////////////////////////////////////////////////////
	// Validate
	//
	// Purpose: Validates all controllers by (re)setting
	//	any interfaces to them and checks for errors
	//
	// In:	nGUID - Controller GUID to validate or GUID_INVALID
	//	to validate all controlles
	//
	// Note: Should be called at least once after level
	//	has loaded and when game is reset
	////////////////////////////////////////////////////
	virtual void Validate(BuildingGUID nGUID = GUID_INVALID);

	////////////////////////////////////////////////////
	// FindBuildingController
	//
	// Purpose: Find the building controller with the
	//	given GUID
	//
	// In:	nGUID - GUID to use
	//
	// Returns building controller's object or NULL on
	//	error
	////////////////////////////////////////////////////
	virtual IBuildingController *FindBuildingController(BuildingGUID nGUID) const;

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Returns the name of the given class
	//
	// In:	nClassID - ID of the class
	////////////////////////////////////////////////////
	virtual const char *GetClassName(BuildingClassID nClassID) const;

	////////////////////////////////////////////////////
	// GetClassId
	//
	// Purpose: Returns the ID of the class with the
	//	given ID
	//
	// In:	szName - Name to search for
	////////////////////////////////////////////////////
	virtual BuildingClassID GetClassId(char const* szName) const;

	////////////////////////////////////////////////////
	// IsValidClass
	//
	// Purpose: Returns TRUE if the specified class ID or
	//	name is valid
	//
	// In:	nID - Class ID
	//		szName - Name of the class
	//
	// Note: Using the name is slower than the ID!
	////////////////////////////////////////////////////
	virtual bool IsValidClass(BuildingClassID nID) const;
	virtual bool IsValidClass(char const* szName) const;

	////////////////////////////////////////////////////
	// GenerateGUID
	//
	// Purpose: Generates a Building GUID given the
	//	team and class names
	//
	// In:	szTeam - Team name
	//		szClass - Class name
	//
	// Returns building GUID or GUID_INVALID on error
	////////////////////////////////////////////////////
	virtual BuildingGUID GenerateGUID(char const* szTeam, char const* szClass) const;

	////////////////////////////////////////////////////
	// GetBuildingFromInterface
	//
	// Purpose: Returns the Building GUID that is interfaced
	//	by the given entity
	//
	// In:	nEntityId - ID of entity that is the interface
	//
	// Out:	ppController - Pointer to controller object
	//
	// Returns Building GUID or GUID_INVALID on error
	////////////////////////////////////////////////////
	virtual BuildingGUID GetBuildingFromInterface(EntityId nEntityId, IBuildingController **ppController = NULL) const;

	////////////////////////////////////////////////////
	// AddBuildingControllerEventListener
	//
	// Purpose:	Add a listener to receive callbacks on
	//	a building controller during certain events
	//
	// In:	nGUID - Building GUID
	//		pListener - Listening object
	//
	// Note: See EControllerEvent
	////////////////////////////////////////////////////
	virtual void AddBuildingControllerEventListener(BuildingGUID nGUID, IBuildingControllerEventListener *pListener);

	////////////////////////////////////////////////////
	// RemoveBuildingControllerEventListener
	//
	// Purpose:	Remove a listener
	//
	// In:	nGUID - Building GUID
	//		pListener - Listening object
	////////////////////////////////////////////////////
	virtual void RemoveBuildingControllerEventListener(BuildingGUID nGUID, IBuildingControllerEventListener *pListener);

	////////////////////////////////////////////////////
	// ClientHit
	//
	// Purpose: Call when a hit occurs on the client
	//
	// In:	hitinfo - Hit information
	////////////////////////////////////////////////////
	virtual void ClientHit(HitInfo const& hitInfo);

	////////////////////////////////////////////////////
	// ServerHit
	//
	// Purpose: Call when a hit occurs on the server
	//
	// In:	hitinfo - Hit information
	////////////////////////////////////////////////////
	virtual void ServerHit(HitInfo const& hitInfo);

	////////////////////////////////////////////////////
	// ServerExplosion
	//
	// Purpose: Call when an explosion occurs on the server
	//
	// In:	explosionInfo - Explosion information
	//		affectedEntities - Map of affected entities
	////////////////////////////////////////////////////
	virtual void ServerExplosion(ExplosionInfo const& explosionInfo, GRTExplosionAffectedEntities const& affectedEntities);

	////////////////////////////////////////////////////
	// ClientExplosion
	//
	// Purpose: Call when an explosion occurs on the client
	//
	// In:	explosionInfo - Explosion information
	//		affectedEntities - Map of affected entities
	////////////////////////////////////////////////////
	virtual void ClientExplosion(ExplosionInfo const& explosionInfo, GRTExplosionAffectedEntities const& affectedEntities);

	////////////////////////////////////////////////////
	// SetBasePower
	//
	// Purpose: Set the power on all buildings
	//
	// In:	nTeamID - Team that owns the base
	//		bState - TRUE to turn power on, FALSE to turn
	//			power off
	////////////////////////////////////////////////////
	virtual void SetBasePower(TeamID nTeamID, bool bState) const;
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////

class CBaseManagerEntitySink : public IEntitySystemSink
{
	CBaseManager *m_pManager;

public:
	////////////////////////////////////////////////////
	// Constructor
	//
	// In:	pManager - Base manager
	////////////////////////////////////////////////////
	CBaseManagerEntitySink(CBaseManager *pManager);

	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBaseManagerEntitySink(void);

	////////////////////////////////////////////////////
	// Callbacks
	////////////////////////////////////////////////////
	virtual bool OnBeforeSpawn(SEntitySpawnParams &params);
	virtual void OnSpawn(IEntity *pEntity, SEntitySpawnParams &params);
	virtual bool OnRemove(IEntity *pEntity);
	virtual void OnEvent(IEntity *pEntity, SEntityEvent &event);
};

#endif //_D6C_CBASEMANAGER_H_