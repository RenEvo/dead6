/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Crosshair HUD object (refactored from old HUD code)

-------------------------------------------------------------------------
History:
- 15:05:2007  11:00 : Created by Jan Müller

*************************************************************************/

#include "HUDObject.h"
#include "IFlashPlayer.h"
#include "IVehicleSystem.h"
#include "GameFlashAnimation.h"

class CHUD;

class CHUDCrosshair : public CHUDObject
{

public:
	CHUDCrosshair(CHUD* pHUD);
	~CHUDCrosshair();

	void OnUpdate(float fDeltaTime,float fFadeValue);

	//use-icon
	void SetUsability(int usable, const char* actionLabel = NULL, const char* param = NULL);
	bool GetUsability() const;
	//show enemy hit in crosshair
	void CrosshairHit();
	//choose crosshair design (usually from menu)
	void SetCrosshair(int iCrosshair);
	//get crosshair flash movie
	CGameFlashAnimation *GetFlashAnim() {return &m_animCrossHair;}

private:

	//update function
	void UpdateCrosshair();

	//the main HUD
	CHUD	*g_pHUD;
	//the crosshair flash asset
	CGameFlashAnimation	m_animCrossHair;
	//usability flag (can use lookat object)
	bool m_bUsable;
	// targetted friendly unit
	int m_iFriendlyTarget;
};