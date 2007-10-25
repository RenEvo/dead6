////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// IBuildingController.h
//
// Purpose: Interface object
//	Describes a building controller which contains
//	a logical structure and is interfaced
//
// File History:
//	- 8/11/07 : File created - KAK
////////////////////////////////////////////////////

#ifndef _D6C_IBUILDINGCONTROLLER_H_
#define _D6C_IBUILDINGCONTROLLER_H_

#include "IBaseManager.h"

struct HitInfo;
struct ExplosionInfo;

// Controller update flags - EControllerUpdateFlags
enum EControllerUpdateFlags
{
	CUF_CHECKVISIBILITY		= 0x01,		// Check if player is looking at controller
	CUF_PARSEEXPLOSIONQUEUE	= 0x02,		// Parse the explosion queues

	// Update all
	CUF_ALL = (CUF_CHECKVISIBILITY|CUF_PARSEEXPLOSIONQUEUE),
};

// Building state flags - EControllerStateFlags
enum EControllerStateFlags
{
	CSF_MUSTBEDESTROYED		= 0x01,		// Set if building must be destroyed for team to lose
	CSF_WANTSTOBEDESTROYED	= 0x02,		// Set if building wants to be destroyed
	CSF_ISVISIBLE			= 0x04,		// Set if building is visible by the player
	CSF_ISVALIDATED			= 0x08,		// Set if building has been validated
};

// Controller events
enum EControllerEvent
{
	// Sent when the controller has been validated
	CONTROLLER_EVENT_VALIDATED,

	// Sent when the controller has been reset
	CONTROLLER_EVENT_RESET,

	// Sent when the controller has been damaged
	// nParam[0] = HitInfo
	CONTROLLER_EVENT_ONHIT,

	// Sent when the controller has been damaged via explosion
	// nParam[0] = ExplosionInfo
	CONTROLLER_EVENT_ONEXPLOSION,

	// Sent when the controller is being looked at by the local client
	// nParam[0] = ID of interface entity focused on
	CONTROLLER_EVENT_INVIEW,

	// Sent when the controller is no longer being looked at by the local client
	CONTROLLER_EVENT_OUTOFVIEW,

	// Last event in list.
	CONTROLLER_EVENT_LAST,
};

struct SControllerEvent
{
	SControllerEvent() {event=CONTROLLER_EVENT_LAST; nParam[0]=0;nParam[1]=0;nParam[2]=0;nParam[3]=0;};
	SControllerEvent(EControllerEvent _event) : event(_event) {nParam[0]=0;nParam[1]=0;nParam[2]=0;nParam[3]=0;};

	// Any event from EControllerEvent enum
	EControllerEvent event;

	// Event parameters
	INT_PTR nParam[4];
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////

struct IBuildingController;
struct IBuildingControllerEventListener
{
	////////////////////////////////////////////////////
	// OnBuildingControllerEvent
	//
	// Purpose: Called when event occurs on controller
	//
	// In:	pController - Building controller object
	//		nGUID - Controller's BuildingGUID
	//		event - Event that occured
	//
	// Note: See SControllerEvent
	////////////////////////////////////////////////////
	virtual void OnBuildingControllerEvent(IBuildingController *pController, BuildingGUID nGUID, SControllerEvent &event) = 0;
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////

struct IBuildingController
{
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~IBuildingController(void) {}

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Used by memory management
	//
	// In:	s - Cry Sizer object
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(class ICrySizer *s) = 0;

	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: Initialize the controller
	//
	// In:	nGUID - Building controller's GUID
	////////////////////////////////////////////////////
	virtual void Initialize(BuildingGUID nGUID) = 0;

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: Shut the controller down
	////////////////////////////////////////////////////
	virtual void Shutdown(void) = 0;
	
	////////////////////////////////////////////////////
	// Update
	//
	// Purpose: Update the controller
	//
	// In:	bHaveFocus - TRUE if game has focus
	//		nUpdateFlags - Update flags
	//		nControllerUpdateFlags - Controller-specific
	//			update flags (see EControllerUpdateFlags)
	////////////////////////////////////////////////////
	virtual void Update(bool bHaveFocus, unsigned int nUpdateFlags, unsigned int nControllerUpdateFlags) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Reset the controller back to its
	//	initial state
	////////////////////////////////////////////////////
	virtual void Reset(void) = 0;

	////////////////////////////////////////////////////
	// BeforeValidate
	//
	// Purpose: Prepare the controller for validation
	//
	// Returns TRUE if validation should proceed, or
	//	FALSE if it should be skipped
	////////////////////////////////////////////////////
	virtual bool BeforeValidate(void) = 0;

	////////////////////////////////////////////////////
	// Validate
	//
	// Purpose: Validate the controller by checking for
	//	interfaces to it
	////////////////////////////////////////////////////
	virtual void Validate(void) = 0;

	////////////////////////////////////////////////////
	// IsValidated
	//
	// Purpose: Returns TRUE if building has been
	//	validated
	////////////////////////////////////////////////////
	virtual bool IsValidated(void) const = 0;

	////////////////////////////////////////////////////
	// LoadFromXml
	//
	// Purpose: Define the building controller's attributes
	//	from the given XML node
	//
	// In:	pNode - XML Node containing its attributes
	////////////////////////////////////////////////////
	virtual void LoadFromXml(XmlNodeRef pNode) = 0;

	////////////////////////////////////////////////////
	// GetGUID
	//
	// Purpose: Return the controller's GUID
	////////////////////////////////////////////////////
	virtual BuildingGUID GetGUID(void) const = 0;

	////////////////////////////////////////////////////
	// GetScriptTable
	//
	// Purpose: Return the script table attatched to
	//	the controller's script
	////////////////////////////////////////////////////
	virtual struct IScriptTable *GetScriptTable(void) const = 0;

	////////////////////////////////////////////////////
	// GetTeam
	//
	// Purpose: Returns owning team's ID or TEAMID_NOTEAM
	//	if there is no team
	////////////////////////////////////////////////////
	virtual unsigned short GetTeam(void) const = 0;

	////////////////////////////////////////////////////
	// GetTeamName
	//
	// Purpose: Returns owning team's name
	////////////////////////////////////////////////////
	virtual char const* GetTeamName(void) const = 0;

	////////////////////////////////////////////////////
	// GetClass
	//
	// Purpose: Returns building class or BC_INVALID if
	//	there is no class
	////////////////////////////////////////////////////
	virtual unsigned short GetClass(void) const = 0;

	////////////////////////////////////////////////////
	// GetClassName
	//
	// Purpose: Returns owning class' name
	////////////////////////////////////////////////////
	virtual char const* GetClassName(void) const = 0;

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
	virtual bool AddInterface(struct IEntity *pEntity) = 0;

	////////////////////////////////////////////////////
	// RemoveInterface
	//
	// Purpose: Remove an entity as an interface to this
	//	controller
	//
	// In:	pEntity - Entity to remove
	////////////////////////////////////////////////////
	virtual void RemoveInterface(struct IEntity *pEntity) = 0;

	////////////////////////////////////////////////////
	// HasInterface
	//
	// Purpose: Returns TRUE if the given entity is an
	//	interface for the controller
	//
	// In:	pEntity - Entity to check
	//		nEntityId - Entity to check
	////////////////////////////////////////////////////
	virtual bool HasInterface(struct IEntity *pEntity) const = 0;
	virtual bool HasInterface(unsigned int nEntityId) const = 0;

	////////////////////////////////////////////////////
	// IsVisible
	//
	// Purpose: Returns TRUE if the controller is visible
	//	to the player (i.e. player is focused on an
	//	interface to it)
	////////////////////////////////////////////////////
	virtual bool IsVisible(void) const = 0;

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
	virtual void SetMustBeDestroyed(bool b = true) = 0;

	////////////////////////////////////////////////////
	// MustBeDestroyed
	//
	// Purpose: Returns TRUE if building must be destroyed
	//	for the team to lose
	////////////////////////////////////////////////////
	virtual bool MustBeDestroyed(void) const = 0;

	////////////////////////////////////////////////////
	// AddEventListener
	//
	// Purpose: Add an event listener to controller
	//
	// In:	pListener - Listening object
	////////////////////////////////////////////////////
	virtual void AddEventListener(IBuildingControllerEventListener *pListener) = 0;

	////////////////////////////////////////////////////
	// RemoveEventListener
	//
	// Purpose: Remove an event listener from controller
	//
	// In:	pListener - Listening object
	////////////////////////////////////////////////////
	virtual void RemoveEventListener(IBuildingControllerEventListener *pListener) = 0;

	////////////////////////////////////////////////////
	// OnClientHit
	//
	// Purpose: Call when a hit occurs on the client
	//
	// In:	hitinfo - Hit information
	////////////////////////////////////////////////////
	virtual void OnClientHit(HitInfo const& hitInfo) = 0;

	////////////////////////////////////////////////////
	// OnServerHit
	//
	// Purpose: Call when a hit occurs on the server
	//
	// In:	hitinfo - Hit information
	////////////////////////////////////////////////////
	virtual void OnServerHit(HitInfo const& hitInfo) = 0;

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
	virtual void OnClientExplosion(ExplosionInfo const& explosionInfo, unsigned int nInterfaceId, float fObstruction) = 0;

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
	virtual void OnServerExplosion(ExplosionInfo const& explosionInfo, unsigned int nInterfaceId, float fObstruction) = 0;
};

#endif //_D6C_IBUILDINGCONTROLLER_H_