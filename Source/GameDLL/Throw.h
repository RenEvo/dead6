/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Throw Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 261:10:2005   15:45 : Created by Márcio Martins

*************************************************************************/
#ifndef __THROW_H__
#define __THROW_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"


class CThrow : public CSingle
{
	struct StartThrowAction;
	struct ThrowAction;
		
protected:
	struct SThrowActions
	{
		SThrowActions() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValueEx("throw", throwit,		"throw");
			ResetValue(hold,		"hold");
			ResetValue(pull,		"pull");
			ResetValue(next,		"next");
		}

		ItemString throwit;
		ItemString hold;
		ItemString pull;
		ItemString next;

		void GetMemoryStatistics(ICrySizer*s)
		{
			s->Add(throwit);
			s->Add(hold);
			s->Add(pull);
			s->Add(next);
		}
	};

	struct SThrowParams
	{
		SThrowParams() { Reset(); };
		void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
		{
			CItemParamReader reader(params);
			ResetValue(delay, 				0.15f);
			ResetValue(hide_ammo,			true);
			ResetValue(auto_select_last,true);

			ResetValue(hold_duration, 1.0f);
			ResetValue(hold_min_scale,1.0f);
			ResetValue(hold_max_scale,1.0f);
		}

		float hold_duration;
		float hold_min_scale;
		float hold_max_scale;

		float	delay;
		bool	hide_ammo;
		bool	auto_select_last;

	};
public:
	CThrow();
	virtual ~CThrow();

	virtual void Update(float frameTime, uint frameId);
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void ResetParams(const struct IItemParamsNode *params);
	virtual void PatchParams(const struct IItemParamsNode *patch);

	virtual void Activate(bool activate);

	virtual bool CanFire(bool considerAmmo) const;
	virtual bool CanReload() const;

	virtual const char* GetType() const
	{
		return "Thrown";
	}

	virtual bool IsReadyToFire() const;
	virtual void StartFire(EntityId shooterId);
	virtual void StopFire(EntityId shooterId);

	virtual void SetThrowable(EntityId entityId, ISchedulerAction *action);
	virtual EntityId GetThrowable() const;

	void SetSpeedScale(float speedScale) { m_speed_scale = speedScale; }
	void ThrowingGrenade(bool throwing) { m_usingGrenade = throwing; }

protected:

	virtual void CheckAmmo();
	virtual void DoThrow(bool drop);
	virtual void DoDrop();

private:

	void   PlayDropThrowSound();

	bool  m_usingGrenade;
	bool	m_thrown;
	bool	m_pulling;
	bool	m_throwing;
	bool	m_auto_throw;
	float	m_throw_time;

	float	m_hold_timer;


	EntityId					m_throwableId;
	ISchedulerAction	*m_throwableAction;

	SThrowActions m_throwactions;
	SThrowParams	m_throwparams;
};

#endif 