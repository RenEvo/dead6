////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   MusicLogic.h
//  Version:     v1.00
//  Created:     24/08/2006 by Tomas.
//  Compilers:   Visual Studio.NET
//  Description: decleration of the MusicLogic class.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MUSICLOGIC_H__
#define __MUSICLOGIC_H__

#include "ISound.h"
#include <ITimer.h>
#include "IGameRulesSystem.h"
#include "IAnimationGraph.h"

#include <list>

#pragma once

struct IMusicSystem;
struct IAnimationGraphState;


#define UPDATE_MUSICLOGIC_IN_MS 200

enum EMusicEvents
{
	MUSICEVENT_SET_MULTIPLIER = 0,
	MUSICEVENT_SET_AI,
	MUSICEVENT_ADD_AI,
	MUSICEVENT_SET_PLAYER,
	MUSICEVENT_ADD_PLAYER,
	MUSICEVENT_SET_GAME,
	MUSICEVENT_ADD_GAME,
	MUSICEVENT_SET_CHANGE,
	MUSICEVENT_ADD_CHANGE,
	MUSICEVENT_ENTER_VEHICLE,
	MUSICEVENT_LEAVE_VEHICLE,
	MUSICEVENT_MOUNT_WEAPON,
	MUSICEVENT_UNMOUNT_WEAPON,
	MUSICEVENT_ENEMY_SPOTTED,
	MUSICEVENT_ENEMY_KILLED,
	MUSICEVENT_ENEMY_HEADSHOT,
	MUSICEVENT_ENEMY_OVERRUN,
	MUSICEVENT_PLAYER_WOUNDED,
	MUSICEVENT_EXPLOSION,
	MUSICEVENT_MAX
};

class CMusicLogic : public IHitListener
{
public:
	//struct SGameStateInfo
	//{
	//	SGameStateInfo()
	//	{
	//		fAction			= 0;
	//		fStealth		= 0;
	//		fVictory		= 0;
	//		fBoredom		= 0;
	//	};
	//	// all values from 0 to 1
	//	float fAction;			// abstract value how much action is going on like fighting, jumping, driving
	//	float fStealth;			// abstract value how much stealth game play is going on being in shadow, ducking, hiding, laying
	//	float fVictory;			// abstract value how close/far the victory of a battle is (dead_bodies/near_enemies) ?
	//	float fBoredom;			// abstract value how boring the music gets
	//};

	struct SMusicStateInfo
	{
		SMusicStateInfo()
		{
			fIntensity = 0;
			fBoredom = 0;
		};
		// all values from 0 to 1
		float fIntensity;		// abstract value how intense the music is
		float fBoredom;			// abstract value how boring the music gets
	};


	CMusicLogic(void);
	~CMusicLogic(void);

	//////////////////////////////////////////////////////////////////////////
	// Initialization
	//////////////////////////////////////////////////////////////////////////

	bool Init();

	bool Start();
	bool Stop();

	void Update();

	// Game State Information
	//void							SetGameStateInfo(SGameStateInfo* pGameStateInfo) { m_GameState = *pGameStateInfo; }
	//SGameStateInfo*		GetGameStateInfo() { return &m_GameState; }

	// Music State Information
	void							SetMusicStateInfo(SMusicStateInfo* pMusicStateInfo) { m_MusticState = *pMusicStateInfo; }
	SMusicStateInfo*	GetMusicStateInfo() { return &m_MusticState; }

	// incoming events
	void SetMusicEvent(EMusicEvents MusicEvent, const float fValue);
	//void SetMusicEvent(EMusicEvents MusicEvent, const char *sText);

	//IHitListener
	virtual void OnHit(const HitInfo&);
	virtual void OnExplosion(const ExplosionInfo&);
	virtual void OnServerExplosion(const ExplosionInfo&);

	void GetMemoryStatistics( ICrySizer * );
	void Serialize(TSerialize ser,unsigned aspects);

private:

	IAnimationGraphState *m_pMusicState;

	//SGameStateInfo	m_GameState;
	SMusicStateInfo	m_MusticState;

	IMusicSystem		*m_pMusicSystem;

	float						m_fMultiplier;

	CTimeValue			m_MusicTime;
	CTimeValue			m_SilenceTime;
	CTimeValue			m_tLastUpdate;

	IAnimationGraph::InputID m_AI_Intensity_ID;
	IAnimationGraph::InputID m_Player_Intensity_ID;
	IAnimationGraph::InputID m_Allow_Change_ID;
	IAnimationGraph::InputID m_Game_Intensity_ID;
	IAnimationGraph::InputID m_MusicTime_ID;
	IAnimationGraph::InputID m_MoodTime_ID;
	IAnimationGraph::InputID m_SilenceTime_ID;

	typedef std::list<CTimeValue> tExplosionList;
	tExplosionList m_listExplosions;

	bool						m_bActive;
	bool						m_bHitListener;

};
#endif