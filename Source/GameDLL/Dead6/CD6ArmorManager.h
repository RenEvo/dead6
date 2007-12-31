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
	EntityArmorMap m_EntityArmors;

public:
	virtual ~CD6ArmorManager() {}

	virtual void LoadFromXML(XmlNodeRef& rootNode);
	virtual void Reset();
	virtual void GetMemoryStatistics(ICrySizer& s);
	virtual void RegisterEntityArmor(EntityId entity, SArmorDef const& armor);
	virtual void UnregisterEntityArmor(EntityId entity);
	virtual SArmorDef const* GetEntityArmorDef(EntityId entity) const;
	virtual SArmorDef const* GetArmorDef(char const* szName);
	virtual float GetMultiplier(char const* szArmorName, char const* szWarheadName);
};

#endif // _CD6ARMORMANAGER_H_