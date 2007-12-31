////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// IArmorManager.h
//
// Purpose: Interface object
//	Describes the Armor Manager for handling armor
//  types
//
// File History:
//	- 12/30/2007 : File created - Dan
//
// TODO:
//  - Change manager to give out armordef ID's, instead
//    of pointers.
////////////////////////////////////////////////////
#ifndef _ARMORMANAGER_H_
#define _ARMORMANAGER_H_
#include <vector>
#include "IXml.h"
#include "IEntity.h"

class ICrySizer;

////////////////////////////////////////////////////
// Defines a warhead type
struct SArmorWarheadDef
{
	string szWarheadName;
	float fMultiplier;
};

////////////////////////////////////////////////////
// Defines an armor type
struct SArmorDef
{
	string szArmorName;
	std::vector<SArmorWarheadDef> warheads;
};

typedef std::map<EntityId, SArmorDef const*> EntityArmorMap;

class IArmorManager
{
public:
	virtual ~IArmorManager(){}

	////////////////////////////////////////////////////
	// LoadFromXML
	//
	// Purpose: Loads armor definitions from an XML node
	//
	// In: rootNode the node whose child nodes are Armor
	//     definitions
	////////////////////////////////////////////////////
	virtual void LoadFromXML(XmlNodeRef& rootNode) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Resets the Armor Manager back to empty
	////////////////////////////////////////////////////
	virtual void Reset() = 0;

	////////////////////////////////////////////////////
	// GetMemoryStatistics
	//
	// Purpose: Calculates the memory statistics of the
	//          armor manager
	//
	// In: s - The sizer to use
	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer& s) = 0;

	////////////////////////////////////////////////////
	// RegisterEntityArmor
	//
	// Purpose: Binds an armor type to a specific entity
	//
	// In: entity - The ID of the entity to bind
	//     armor - The armor to bind to the entity
	////////////////////////////////////////////////////
	virtual void RegisterEntityArmor(EntityId entity, SArmorDef const& armor) = 0;

	////////////////////////////////////////////////////
	// UnregisterEntityArmor
	//
	// Purpose: Removes an entity's armor from the manager
	//
	// In: entity - The ID of the entity to unbind
	////////////////////////////////////////////////////
	virtual void UnregisterEntityArmor(EntityId entity) = 0;

	////////////////////////////////////////////////////
	// GetEntityArmorDef
	//
	// Purpose: Retrieves the armor type of an entity
	//
	// In: entity - The ID of the entity whose armor to
	//              retrieve
	//
	// Returns a pointer to an SArmorDef object or NULL
	//         if the entity does not have any armor
	//         registered
	////////////////////////////////////////////////////
	virtual SArmorDef const* GetEntityArmorDef(EntityId entity) const = 0;

	////////////////////////////////////////////////////
	// GetArmorDef
	//
	// Purpose: Retrieves the Armor Definition which has
	//          the specified name.
	//
	// In: szName - The name of the armor definition to
	//              retrieve.
	//
	// Returns a pointer to an SArmorDef object with the
	//         specified name, or NULL if no such armor
	//         definition exists.
	////////////////////////////////////////////////////
	virtual SArmorDef const* GetArmorDef(char const* szName) = 0;

	////////////////////////////////////////////////////
	// GetMultiplier
	//
	// Purpose: Retrieves the multiplier for a warhead
	//          from an armor definition.
	//
	// In: szArmorName - The name of the armor
	//     szWarheadName - The name of the warhead to get
	//					   the multiplier of
	//
	// Returns the multiplier of the warhead if it exists
	//         in the armor definition, or 1.0 if no such
	//         warhead or armor definition exists
	////////////////////////////////////////////////////
	virtual float GetMultiplier(char const* szArmorName, char const* szWarheadName) = 0;
};

#endif // _ARMORMANAGER_H_