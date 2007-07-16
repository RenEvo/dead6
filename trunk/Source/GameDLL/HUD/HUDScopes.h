/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Binocular/Scopes HUD object (refactored from old HUD code)

-------------------------------------------------------------------------
History:
- 17:04:2007  17:30 : Created by Jan Müller

*************************************************************************/

#include "HUDObject.h"
#include "IFlashPlayer.h"
#include "GameFlashAnimation.h"

class CHUD;
class CPlayer;

class CHUDScopes : public CHUDObject
{
	friend class CHUD;
public:

	enum EScopeMode 
	{
		ESCOPE_NONE,
		ESCOPE_ASSAULT,
		ESCOPE_SNIPER
	};

	CHUDScopes(CHUD *pHUD);
	~CHUDScopes();

	virtual void OnUpdate(float fDeltaTime,float fFadeValue);
	virtual void LoadFlashFiles(bool force=false);
	void ShowBinoculars(bool bVisible, bool bShowIfNoHUD=false);
	void SetBinocularsDistance(float fDistance);
	void SetBinocularsZoomMode(int iZoomMode);
	void ShowScope(int iVisible);
	void SetScopeZoomMode(int iZoomMode, string &scopeType);
	void OnToggleThirdPerson(bool thirdPerson);
	ILINE bool IsBinocularsShown() const { return m_bShowBinoculars; }
	ILINE EScopeMode GetCurrentScope() const { return m_eShowScope; }

private:

	void DisplayBinoculars(CPlayer* pPlayerActor);
	void DisplayScope(CPlayer* pPlayerActor);

	//the main HUD
	CHUD			*g_pHUD;
	//the scope flash movies
	CGameFlashAnimation m_animBinoculars;
	CGameFlashAnimation m_animBinocularsEnemyIndicator;
//	CGameFlashAnimation m_animAssaultScope;
	CGameFlashAnimation m_animSniperScope;
	//binoculars visible
	bool m_bShowBinoculars;
	//scope visible
	EScopeMode m_eShowScope;
	// show binoculars without HUD being visible, e.g. cutscenes
	bool m_bShowBinocularsNoHUD; 
	// distance of binocs view
	float m_fBinocularDistance;
	// saving zoom level
	int m_oldScopeZoomLevel;
	// currently in third person ?
	bool m_bThirdPerson;
};