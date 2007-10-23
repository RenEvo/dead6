////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CBuildingController.h
//
// Purpose: Building controller which contains
//	a logical structure and is interfaced
//
// File History:
//	- 8/11/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_CBUILDINGCONTROLLER_H_
#define _D6C_CBUILDINGCONTROLLER_H_

#include "IBuildingController.h"
#include "IBaseManager.h"

class CBuildingController : public IBuildingController
{
	// Building's GUID
	BuildingGUID m_nGUID;

	// Building status flags
	unsigned int m_nFlags;

	// Building's health
	float m_fHealth, m_fInitHealth, m_fMaxHealth;

	// Script object
	IScriptSystem *m_pSS;
	SmartScriptTable m_pScriptTable;

	// Name of the building
	string m_szName;

	// Building's script
	string m_szScript;

	// Interface map
	typedef std::vector<IEntity*> InterfaceMap;
	InterfaceMap m_Interfaces;

	// Explosion queue (weaponId, explosion info with updated damage)
	typedef std::map<EntityId, ExplosionInfo> ExplosionQueue;
	ExplosionQueue m_ExplosionQueue;

	// Event listeners
	typedef std::list<IBuildingControllerEventListener*> EventListeners;
	EventListeners m_EventListeners;

public:
	////////////////////////////////////////////////////
	// Constructor
	////////////////////////////////////////////////////
	CBuildingController(void);
private:
	CBuildingController(CBuildingController const&) {}
	CBuildingController& operator =(CBuildingController const&) {return *this;}

public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~CBuildingController(void);

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
	// Purpose: Initialize the controller
	//
	// In:	nGUID - Building controller's GUID
	//		fHealth - Initial health
	////////////////////////////////////////////////////
	virtual void Initialize(BuildingGUID nGUID, float fHealth);

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: Shut the controller down
	////////////////////////////////////////////////////
	virtual void Shutdown(void);
	
	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update the controllers
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	//		nControllerUpdateFlags - Controller-specific
	//			update flags (see EControllerUpdateFlags)
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags, unsigned int nControllerUpdateFlags);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Reset the controller back to its
	//	initial state
	////////////////////////////////////////////////////
	virtual void Reset(void);

	////////////////////////////////////////////////////
	// BeforeValidate
	//
	// Purpose: Prepare the controller for validation
	//
	// Returns TRUE if validation should proceed, or
	//	FALSE if it should be skipped
	////////////////////////////////////////////////////
	virtual bool BeforeValidate(void);

	////////////////////////////////////////////////////
	// Validate
	//
	// Purpose: Validate the controller by checking for
	//	interfaces to it
	////////////////////////////////////////////////////
	virtual void Validate(void);

	////////////////////////////////////////////////////
	// IsValidated
	//
	// Purpose: Returns TRUE if building has been
	//	validated
	////////////////////////////////////////////////////
	virtual bool IsValidated(void) const;

	////////////////////////////////////////////////////
	// LoadFromXml
	//
	// Purpose: Define the building controller's attributes
	//	from the given XML file
	//
	// In:	szName - Class name of the building
	//
	// Returns TRUE if building controller is ready
	////////////////////////////////////////////////////
	virtual bool LoadFromXml(char const* szName);

	////////////////////////////////////////////////////
	// GetGUID
	//
	// Purpose: Return the controller's GUID
	////////////////////////////////////////////////////
	virtual BuildingGUID GetGUID(void) const;

	////////////////////////////////////////////////////
	// GetScriptTable
	//
	// Purpose: Return the script table attatched to
	//	the controller's script
	////////////////////////////////////////////////////
	virtual struct IScriptTable *GetScriptTable(void) const;

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Returns owning team's ID or TEAMID_NOTEAM
	//	if there is no team
	////////////////////////////////////////////////////
	virtual TeamID GetTeam(void) const;

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Returns owning team's name
	////////////////////////////////////////////////////
	virtual char const* GetTeamName(void) const;

	////////////////////////////////////////////////////
	// GetClass
	//
	// Purpose: Returns building class or BC_INVALID if
	//	there is no class
	////////////////////////////////////////////////////
	virtual BuildingClassID GetClass(void) const;

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Returns owning class' name
	////////////////////////////////////////////////////
	virtual char const* GetClassName(void) const;

	////////////////////////////////////////////////////
	// AddInterface
	//
	// Purpose: Mark an entity as an interface to this
	//	controller
	//
	// In:	pEntity - Entity to use
	//
	// Returns TRUE if entity is acting as an interface
	//	to it now or FALSE if not
	//
	// Note: Sets the ENTITY_FLAG_ISINTERFACE on the enemy
	//	so the same entity cannot interface more than one
	//	building at the same time!
	////////////////////////////////////////////////////
	virtual bool AddInterface(IEntity *pEntity);

	////////////////////////////////////////////////////
	// RemoveInterface
	//
	// Purpose: Remove an entity as an interface to this
	//	controller
	//
	// In:	pEntity - Entity to remove
	////////////////////////////////////////////////////
	virtual void RemoveInterface(IEntity *pEntity);

	////////////////////////////////////////////////////
	// HasInterface
	//
	// Purpose: Returns TRUE if the given entity is an
	//	interface for the controller
	//
	// In:	pEntity - Entity to check
	//		nEntityId - Entity to check
	////////////////////////////////////////////////////
	virtual bool HasInterface(IEntity *pEntity) const;
	virtual bool HasInterface(EntityId nEntityId) const;

	////////////////////////////////////////////////////
	// IsVisible
	//
	// Purpose: Returns TRUE if the controller is visible
	//	to the player (i.e. player is focused on an
	//	interface to it)
	////////////////////////////////////////////////////
	virtual bool IsVisible(void) const;

	////////////////////////////////////////////////////
	// SetMustBeDestroyed
	//
	// Purpose: Set if building wants to be destroyed
	//	for the team to lose
	//
	// In:	b - TRUE if it must be destroyed
	//
	// Note: Will be ignored if building is not interfaced
	////////////////////////////////////////////////////
	virtual void SetMustBeDestroyed(bool b = true);

	////////////////////////////////////////////////////
	// MustBeDestroyed
	//
	// Purpose: Returns TRUE if building must be destroyed
	//	for the team to lose
	////////////////////////////////////////////////////
	virtual bool MustBeDestroyed(void) const;

	////////////////////////////////////////////////////
	// AddEventListener
	//
	// Purpose: Add an event listener to controller
	//
	// In:	pListener - Listening object
	////////////////////////////////////////////////////
	virtual void AddEventListener(IBuildingControllerEventListener *pListener);

	////////////////////////////////////////////////////
	// RemoveEventListener
	//
	// Purpose: Remove an event listener from controller
	//
	// In:	pListener - Listening object
	////////////////////////////////////////////////////
	virtual void RemoveEventListener(IBuildingControllerEventListener *pListener);

	////////////////////////////////////////////////////
	// OnClientHit
	//
	// Purpose: Call when a hit occurs on the client
	//
	// In:	hitinfo - Hit information
	////////////////////////////////////////////////////
	virtual void OnClientHit(HitInfo const& hitInfo);

	////////////////////////////////////////////////////
	// OnServerHit
	//
	// Purpose: Call when a hit occurs on the server
	//
	// In:	hitinfo - Hit information
	////////////////////////////////////////////////////
	virtual void OnServerHit(HitInfo const& hitInfo);

	////////////////////////////////////////////////////
	// OnClientExplosion
	//
	// Purpose: Call when an explosion occurs on the client
	//
	// In:	explosionInfo - Explosion information
	//		nInterfaceId - ID of interface entity that
	//			was hit by this explosion
	//		fObstruction - Obstruction ratio (0 = fully
	//			obstructed, 1 = fully visible)
	////////////////////////////////////////////////////
	virtual void OnClientExplosion(ExplosionInfo const& explosionInfo, EntityId nInterfaceId, float fObstruction);

	////////////////////////////////////////////////////
	// OnServerExplosion
	//
	// Purpose: Call when an explosion occurs on the server
	//
	// In:	explosionInfo - Explosion information
	//		nInterfaceId - ID of interface entity that
	//			was hit by this explosion
	//		fObstruction - Obstruction ratio (0 = fully
	//			obstructed, 1 = fully visible)
	////////////////////////////////////////////////////
	virtual void OnServerExplosion(ExplosionInfo const& explosionInfo, EntityId nInterfaceId, float fObstruction);

protected:
	////////////////////////////////////////////////////
	// _SendListenerEvent
	//
	// Purpose: Send an event out to all listeners
	//
	// In:	event - Event to send out
	////////////////////////////////////////////////////
	virtual void _SendListenerEvent(SControllerEvent &event);
};

#endif //_D6C_CBUILDINGCONTROLLER_H_