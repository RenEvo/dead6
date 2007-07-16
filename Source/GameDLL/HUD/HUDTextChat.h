/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: HUD text chat - receives text input and broadcasts it to other players

-------------------------------------------------------------------------
History:
- 07:03:2006: Created by Jan Müller

*************************************************************************/
#ifndef __HUDTEXTCHAT_H__
#define __HUDTEXTCHAT_H__

//-----------------------------------------------------------------------------------------------------

#include "HUDObject.h"
#include <IInput.h>
#include "IFlashPlayer.h"
#include "IActionMapManager.h"

class CGameFlashAnimation;

static const int CHAT_LENGTH = 6;


class CHUDTextChat : public CHUDObject,public IInputEventListener, public IFSCommandHandler
{
public:
	CHUDTextChat();

	~CHUDTextChat();

	void Init(CGameFlashAnimation *pFlashChat);

	// IFSCommandHandler
	virtual void HandleFSCommand(const char* pCommand, const char* pArgs);
	// ~IFSCommandHandler

	virtual void OnUpdate(float deltaTime,float fadeValue);

	void GetMemoryStatistics(ICrySizer * s);

	virtual bool OnInputEvent(const SInputEvent &event );

	//add arriving multiplayer chat messages here
	virtual void AddChatMessage(EntityId sourceId, const char* msg, int teamFaction);
  virtual void AddChatMessage(const char* nick, const char* msg, int teamFaction);

	void ShowVirtualKeyboard(bool active);

	//open chat
	void OpenChat(int type);

private:
	virtual void Flush(bool close=true);
	virtual void ProcessInput(const SInputEvent &event);
	virtual void Delete();
	virtual void Backspace();
	virtual void Left();
	virtual void Right();
	virtual void Insert(const char *key);
	virtual void VirtualKeyboardInput(string direction);

	CGameFlashAnimation *m_flashChat;

	string				m_inputText;
	string				m_lastInputText;
	int						m_cursor;
	bool					m_isListening;
	bool					m_teamChat;

	string				m_chatStrings[CHAT_LENGTH];
	float					m_chatSpawnTime[CHAT_LENGTH];
	int						m_chatHead;

	bool					m_textInputActive;
	bool					m_showVirtualKeyboard;

	bool					m_anyCurrentText;
	/*bool					m_showing;
	float					m_lastUpdate;*/
	float					m_repeatTimer;
	SInputEvent		m_repeatEvent;
};

#endif