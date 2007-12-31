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

class IArmorManager
{
public:
	virtual ~IArmorManager(){}

	////////////////////////////////////////////////////
	// LoadFromFile
	//
	// Purpose: Loads armor definitions from an XML file
	//
	// In: szFileName - The name of the XML file to load
	//                  the armor definitions from.
	////////////////////////////////////////////////////
	virtual void LoadFromXML(XmlNodeRef& rootNode) = 0;

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Resets the Armor Manager back to empty
	////////////////////////////////////////////////////
	virtual void Reset() = 0;

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