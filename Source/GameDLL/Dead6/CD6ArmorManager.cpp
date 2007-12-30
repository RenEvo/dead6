#include "CD6ArmorManager.h"

void CD6ArmorManager::LoadFromFile(char const* szFileName)
{
	// TODO: Write this function when XML format has been approved
}

void CD6ArmorManager::Reset()
{
	m_ArmorDefs.clear();
}

SArmorDef const* CD6ArmorManager::GetArmorDef(char const* szName)
{
	std::size_t size = m_ArmorDefs.size();
	for (std::size_t i = 0; i < size; ++i)
		if (szName == m_ArmorDefs[i].szName)
			return &m_ArmorDefs[i];
	return NULL;
}