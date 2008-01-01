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

typedef unsigned int ArmorId;
typedef std::map<ArmorId, SArmorDef> ArmorMap;
typedef std::map<EntityId, ArmorId> EntityArmorMap;

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
	//     armor - The armor id to bind to the entity
	//
	// Returns true if the armor was successfully bound
	//         to the entity, false if the armor with 
	//         the specified ArmorId does not exist.
	////////////////////////////////////////////////////
	virtual bool RegisterEntityArmor(EntityId entity, ArmorId armor) = 0;

	////////////////////////////////////////////////////
	// UnregisterEntityArmor
	//
	// Purpose: Removes an entity's armor from the manager
	//
	// In: entity - The ID of the entity to unbind
	////////////////////////////////////////////////////
	virtual void UnregisterEntityArmor(EntityId entity) = 0;

	////////////////////////////////////////////////////
	// GetEntityArmorId
	//
	// Purpose: Retrieves the armor id of an entity
	//
	// In: entity - The ID of the entity whose armor to
	//              retrieve
	//
	// Returns a pointer to an SArmorDef object or NULL
	//         if the entity does not have any armor
	//         registered
	////////////////////////////////////////////////////
	virtual ArmorId GetEntityArmorId(EntityId entity) const = 0;

	////////////////////////////////////////////////////
	// GetArmorId
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
	virtual ArmorId GetArmorId(char const* szName) const = 0;

	////////////////////////////////////////////////////
	// GetArmorName
	//
	// Purpose: Retrieves the name of the armor with the
	//			specified armor ID. Returns a pointer
	//			to avoid the cost of copying the string.
	//
	// In: armor - The ID of the Armor to find the name 
	//			   of
	//
	// Returns a pointer to the name of the armor, or
	//         NULL if no armor with the specified ID
	//		   exists
	////////////////////////////////////////////////////
	virtual string const* GetArmorName(ArmorId armor) const = 0;

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
	virtual float GetMultiplier(ArmorId armor, char const* szWarheadName) const = 0;
};

#endif // _ARMORMANAGER_H_