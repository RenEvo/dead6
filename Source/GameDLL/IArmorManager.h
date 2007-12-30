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

////////////////////////////////////////////////////
// Defines an armor type
struct SArmorDef
{
	string szName;
	float fMultiplier;
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
	virtual void LoadFromFile(char const* szFileName) = 0;

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
};

#endif // _ARMORMANAGER_H_