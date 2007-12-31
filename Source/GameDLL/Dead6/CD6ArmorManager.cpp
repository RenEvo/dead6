#include "StdAfx.h"
#include "CD6ArmorManager.h"


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

			m_ArmorDefs.push_back(armorDef);
		}
	}
}

void CD6ArmorManager::Reset()
{
	m_ArmorDefs.clear();
}

SArmorDef const* CD6ArmorManager::GetArmorDef(char const* szName)
{
	std::size_t size = m_ArmorDefs.size();
	for (std::size_t i = 0; i < size; ++i)
		if (szName == m_ArmorDefs[i].szArmorName)
			return &m_ArmorDefs[i];
	return NULL;
}

float CD6ArmorManager::GetMultiplier(const char *szArmorName, const char *szWarheadName)
{
	SArmorDef const* pArmor = GetArmorDef(szArmorName);
	if (NULL == pArmor)
		return 1.0f;

	for (std::vector<SArmorWarheadDef>::const_iterator iter = pArmor->warheads.begin();
		 iter != pArmor->warheads.end();
		 ++iter)
	{
		if (szWarheadName == iter->szWarheadName)
			return iter->fMultiplier;
	}

	return 1.0f;
}