/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: HUD score board for multiplayer

-------------------------------------------------------------------------
History:
- 27:03:2006: Created by Jan Müller

*************************************************************************/
#ifndef __HUDSCORE_H__
#define __HUDSCORE_H__

#include "HUDObject.h"

class CGameFlashAnimation;

class CHUDScore : public CHUDObject
{
	friend class CHUD;

	struct ScoreEntry
	{
		int					m_kills;
		int					m_deaths;
		int					m_ping;
		int					m_team;
		EntityId		m_entityId;

		ScoreEntry(EntityId id, int kills, int deaths, int ping);
		bool operator<(const ScoreEntry& entry) const;
	};

private:

	int				m_lastUpdate;
	int				m_currentClientTeam;
	bool			m_bShow;
	float			m_lastShowSwitch;
	CGameFlashAnimation *m_pFlashBoard;
	
	void Render();

public:

	CHUDScore();

	~CHUDScore()
	{
	}

	virtual void OnUpdate(float fDeltaTime,float fFadeValue);

	virtual bool OnInputEvent(const SInputEvent &event )
	{
		return false;
	}

	std::vector<ScoreEntry> m_scoreBoard;

	void Reset();
	void AddEntry(EntityId player, int kills, int deaths, int ping);

	void GetMemoryStatistics(ICrySizer * s);

	ILINE void SetVisible(bool visible, CGameFlashAnimation *board = NULL)
	{
		m_bShow = visible;
		m_pFlashBoard = board;
		m_lastShowSwitch = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	}

};

#endif