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
	ArmorMap m_ArmorDefs;
	EntityArmorMap m_EntityArmors;
	ArmorId m_NextArmorId;

public:
	CD6ArmorManager();
	virtual ~CD6ArmorManager() {}

	virtual void LoadFromXML(XmlNodeRef& rootNode);
	virtual void Reset();
	virtual void GetMemoryStatistics(ICrySizer& s);
	virtual bool RegisterEntityArmor(EntityId entity, ArmorId armor);
	virtual void UnregisterEntityArmor(EntityId entity);
	virtual ArmorId GetEntityArmorId(EntityId entity) const;
	virtual ArmorId GetArmorId(char const* szName) const;
	virtual string const* GetArmorName(ArmorId armor) const ;
	virtual float GetArmorMultiplier(ArmorId armor, char const* szWarheadName) const;
	virtual float GetEntityMultiplier(EntityId entity, char const* szWarheadName) const;
};

#endif // _CD6ARMORMANAGER_H_