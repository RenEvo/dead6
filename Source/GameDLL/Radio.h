#ifndef __RADIO_H__
#define __RADIO_H__

#pragma once

#include "GameActions.h"
#include "IInput.h"

class CGameRules;

class CRadio:public IInputEventListener
{
public:
	CRadio(CGameRules*);
	~CRadio();
	bool OnAction(const ActionId& actionId, int activationMode, float value);

	static const int RADIO_MESSAGE_NUM;

	//from IInputEventListener
	virtual bool	OnInputEvent( const SInputEvent &event );
	void			OnRadioMessage(int id,const char* from);
	void			CancelRadio();
	void			SetTeam(const string& name);
	void			GetMemoryStatistics(ICrySizer * s);
private:
	CGameRules	*m_pGameRules;
	int			m_currentGroup;
	string		m_TeamName;

	void		CloseRadio();
};

#endif