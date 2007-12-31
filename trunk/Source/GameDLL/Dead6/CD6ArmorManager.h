////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CD6ArmorManager.h
//
// Purpose: Dead6 Armor Manager Implementation
//	The Dead 6 implementation of the Armor Manager
//
// File History:
//	- 12/30/2007 : File created - Dan
////////////////////////////////////////////////////
#ifndef _CD6ARMORMANAGER_H_
#define _CD6ARMORMANAGER_H_
#include "IArmorManager.h"
#include <vector>

class CD6ArmorManager : public IArmorManager
{
private:
	std::vector<SArmorDef> m_ArmorDefs;

public:
	virtual ~CD6ArmorManager() {}

	////////////////////////////////////////////////////
	// LoadFromFile
	//
	// Purpose: Loads all armor definitions from the
	//          specified XML file.
	//
	// In: szFileName - The name of the XML file to read
	////////////////////////////////////////////////////
	virtual void LoadFromXML(XmlNodeRef& rootNode);

	////////////////////////////////////////////////////
	// Reset
	//
	// Purpose: Empties the manager of all Armor 
	//			Definitions.
	////////////////////////////////////////////////////
	virtual void Reset();

	////////////////////////////////////////////////////
	// GetArmorDef
	//
	// Purpose: Retrieves the Armor Definition which has
	//			the specified name.
	//
	// In: szName - The name of the Armor Definition to 
	//				retrieve.
	//
	// Returns a pointer to an SArmorDef object with the
	//		   specified name or NULL if no such definition
	//		   exists
	////////////////////////////////////////////////////
	virtual SArmorDef const* GetArmorDef(char const* szName);

	virtual float GetMultiplier(char const* szArmorName, char const* szWarheadName);
};

#endif // _CD6ARMORMANAGER_H_