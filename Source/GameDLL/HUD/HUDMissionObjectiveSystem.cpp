#include "StdAfx.h"
#include "ISerialize.h"
#include "Game.h"
#include "HUDMissionObjectiveSystem.h"
#include "HUD.h"
#include "HUDRadar.h"

void CHUDMissionObjective::SetStatus(HUDMissionStatus status)
{
	if (status == m_eStatus)
		return;

	m_eStatus = status;

	assert (m_pMOS != 0);
	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->UpdateObjective(this);

	m_lastTimeChanged = gEnv->pTimer->GetFrameStartTime().GetSeconds();
}


const char* CHUDMissionObjectiveSystem::GetMissionObjectiveLongDescription(const char* id)
{
	const char* message = 0;
	CHUDMissionObjective* pObj = GetMissionObjective(id);
	if (pObj)
		message = pObj->GetMessage();
	return message;
}

const char* CHUDMissionObjectiveSystem::GetMissionObjectiveShortDescription(const char* id)
{
	const char* message = 0;
	CHUDMissionObjective* pObj = GetMissionObjective(id);
	if (pObj)
		message = pObj->GetShortDescription();
	return message;
}

void CHUDMissionObjectiveSystem::LoadLevelObjectives(bool forceReloading)
{
	if(!m_bLoadedObjectives || forceReloading)
	{
		m_currentMissionObjectives.clear();

		const char* filename = "Libs/UI/Objectives_new.xml";
		if(gEnv->bMultiplayer)
		{
			filename = "Libs/UI/MP_Objectives.xml";
		}
		XmlNodeRef missionObjectives = GetISystem()->LoadXmlFile(filename);
		if (missionObjectives == 0)
			return;

		for(int tag = 0; tag < missionObjectives->getChildCount(); ++tag)
		{
			XmlNodeRef mission = missionObjectives->getChild(tag);
			const char* attrib;
			const char* objective;
			const char* text;
			const char* secondary;
			for(int obj = 0; obj < mission->getChildCount(); ++obj)
			{
				XmlNodeRef objectiveNode = mission->getChild(obj);
				string id(mission->getTag());
				id.append(".");
				id.append(objectiveNode->getTag());
				if(objectiveNode->getAttributeByIndex(0, &attrib, &objective) && objectiveNode->getAttributeByIndex(1, &attrib, &text))
				{
					bool secondaryObjective = false;
					if(objectiveNode->getAttributeByIndex(2, &attrib, &secondary))
						if(!stricmp(secondary, "true"))
							secondaryObjective = true;
					m_currentMissionObjectives.push_back(CHUDMissionObjective(this, id.c_str(), objective, text, secondaryObjective));
				}
				else
					GameWarning("Error reading mission objectives.");
			}
		}

		m_bLoadedObjectives = true;
	}
	//else
	//	DeactivateObjectives(true);		//deactive old objectives on level change
}

void CHUDMissionObjectiveSystem::DeactivateObjectives(bool leaveOutRecentlyChanged)
{
	for(int i = 0; i < m_currentMissionObjectives.size(); ++i)
	{
		if(!leaveOutRecentlyChanged ||
			(m_currentMissionObjectives[i].m_lastTimeChanged - gEnv->pTimer->GetFrameStartTime().GetSeconds() > 0.2f))
		{
			m_currentMissionObjectives[i].SetSilent(true);
			m_currentMissionObjectives[i].SetStatus(CHUDMissionObjective::DEACTIVATED);
			m_currentMissionObjectives[i].SetSilent(false);
		}
	}
}

CHUDMissionObjective* CHUDMissionObjectiveSystem::GetMissionObjective(const char* id)
{
	std::vector<CHUDMissionObjective>::iterator it;
	for(it = m_currentMissionObjectives.begin(); it != m_currentMissionObjectives.end(); ++it)
	{
		if(!strcmp( (*it).GetID(), id))
			return &(*it);
	}

	return NULL;
}

void CHUDMissionObjectiveSystem::Serialize(TSerialize ser, unsigned aspects)	//not tested!!
{
	if(ser.GetSerializationTarget() != eST_Network)		
	{
		if (ser.IsReading())
			LoadLevelObjectives(true);

		float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();

		for(std::vector<CHUDMissionObjective>::iterator it = m_currentMissionObjectives.begin(); it != m_currentMissionObjectives.end(); ++it)
		{
			ser.BeginGroup("HUD_Objective");
			ser.EnumValue("ObjectiveStatus", (*it).m_eStatus, CHUDMissionObjective::FIRST, CHUDMissionObjective::LAST);
			ser.Value("trackedEntity", (*it).m_trackedEntity);
			ser.EndGroup();

			if(ser.IsReading() && (*it).m_eStatus != CHUDMissionObjective::DEACTIVATED)
			{				
				(*it).m_lastTimeChanged = now;
				g_pGame->GetHUD()->UpdateObjective(&(*it));
			}
		}
	}
}

void CHUDMissionObjectiveSystem::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_currentMissionObjectives);
	for (size_t i=0; i<m_currentMissionObjectives.size(); i++)
		m_currentMissionObjectives[i].GetMemoryStatistics(s);
}

void CHUDMissionObjective::SetTrackedEntity(EntityId entityID)
{	
	if(m_trackedEntity)
		if(g_pGame->GetHUD())
			g_pGame->GetHUD()->GetRadar()->UpdateMissionObjective(m_trackedEntity, false, 0);

	m_trackedEntity = entityID;
}