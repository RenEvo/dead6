#include "StdAfx.h"
#include "CD6ArmorManager.h"

CD6ArmorManager::CD6ArmorManager() : m_NextArmorId(1)
{}

void CD6ArmorManager::LoadFromXML(XmlNodeRef& rootNode)
{
	int childCount = rootNode->getChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef node = rootNode->getChild(i);
		if (NULL == node)
			continue;

		if (0 == strcmp(node->getTag(), "Armor"))
		{
			XmlString armorName;
			SArmorDef armorDef;

			// Read the Armor tag's name attribute
			if (!node->getAttr("name", armorName))
				continue;
			armorDef.szArmorName = armorName;

			// Read warheads
			int childCount = node->getChildCount();
			for (int i = 0; i < childCount; ++i)
			{
				XmlNodeRef warhead = node->getChild(i);
				if (0 == strcmp(warhead->getTag(), "Warhead"))
				{
					SArmorWarheadDef warheadDef;
					XmlString name;

					if (!warhead->getAttr("name", name))
						break;
					warheadDef.szWarheadName = name;
					if (!warhead->getAttr("multiplier", warheadDef.fMultiplier))
						warheadDef.fMultiplier = 1.0f;

					armorDef.warheads.push_back(warheadDef);
				}
			}

			m_ArmorDefs.insert(std::make_pair(m_NextArmorId++, armorDef));
		}
	}
}

void CD6ArmorManager::Reset()
{
	m_ArmorDefs.clear();
	m_EntityArmors.clear();
	m_NextArmorId = 1;
}

void CD6ArmorManager::GetMemoryStatistics(ICrySizer& s)
{
	s.AddContainer<ArmorMap>(m_ArmorDefs);
	s.AddContainer<EntityArmorMap>(m_EntityArmors);
	s.Add(m_NextArmorId);
}

bool CD6ArmorManager::RegisterEntityArmor(EntityId entity, ArmorId armor)
{
	// Ensure the armor exists
	if (m_ArmorDefs.end() == m_ArmorDefs.find(armor))
		return false;

	m_EntityArmors.insert(std::make_pair(entity, armor));
	return true;
}

void CD6ArmorManager::UnregisterEntityArmor(EntityId entity)
{
	EntityArmorMap::iterator iter = m_EntityArmors.find(entity);
	if (m_EntityArmors.end() != iter)
		m_EntityArmors.erase(iter);
}

ArmorId CD6ArmorManager::GetEntityArmorId(EntityId entity) const
{
	EntityArmorMap::const_iterator iter = m_EntityArmors.find(entity);
	return m_EntityArmors.end() == iter ? NULL : iter->second;
}

ArmorId CD6ArmorManager::GetArmorId(char const* szName) const
{
	// Just perform a linear search
	for (ArmorMap::const_iterator iter = m_ArmorDefs.begin();
		 iter != m_ArmorDefs.end();
		 ++iter)
	{
		if (iter->second.szArmorName == szName)
			return iter->first;
	}
	return NULL;
}

string const* CD6ArmorManager::GetArmorName(ArmorId armor) const
{
	ArmorMap::const_iterator iter = m_ArmorDefs.find(armor);
	return m_ArmorDefs.end() == iter ? NULL : &iter->second.szArmorName;
}

float CD6ArmorManager::GetMultiplier(ArmorId armor, const char *szWarheadName) const
{
	// Ensure armor exists
	ArmorMap::const_iterator iter = m_ArmorDefs.find(armor);
	if (m_ArmorDefs.end() == iter)
		return 1.0f;

	// Search for the warhead with the specified name in the warhead list
	for (std::vector<SArmorWarheadDef>::const_iterator wIter = iter->second.warheads.begin();
		 wIter != iter->second.warheads.end();
		 ++wIter)
	{
		if (wIter->szWarheadName == szWarheadName)
			return wIter->fMultiplier;
	}
	
	// Warhead not found
	return 1.0f;
}