#include "StdAfx.h"
#include "HUDTextChat.h"
#include "IGameRulesSystem.h"
#include "IActorSystem.h"
#include "IUIDraw.h"
#include "HUD.h"
#include "GameActions.h"

#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"

CHUDTextChat::CHUDTextChat() : m_isListening(false), m_repeatTimer(0.0f), m_chatHead(0), m_cursor(0),
		m_anyCurrentText(false), m_teamChat(false), m_showVirtualKeyboard(false), m_textInputActive(false)
{
	assert(GetISystem()->GetIInput());
	GetISystem()->GetIInput()->AddEventListener(this);
	m_flashChat = NULL;
}

CHUDTextChat::~CHUDTextChat()
{
	GetISystem()->GetIInput()->RemoveEventListener(this);
	if(m_isListening)
		GetISystem()->GetIInput()->SetExclusiveListener(NULL);
}

void CHUDTextChat::Init(CGameFlashAnimation *pFlashChat)
{
	m_flashChat = pFlashChat;
	if(m_flashChat)
		m_flashChat->GetFlashPlayer()->SetFSCommandHandler(this);
	//m_showing = false;
	//m_lastUpdate = 0;
}

void CHUDTextChat::OnUpdate(float fDeltaTime, float fFadeValue)
{
	if(!m_flashChat)
		return;

	float now = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();

	//insert some fancy text-box
	
	//render current chat messages over it
	/*if(m_anyCurrentText)
	{
		bool current = false;
		int msgNr = m_chatHead;
		for(int i = 0; i < CHAT_LENGTH; i++)
		{
			float age = now - m_chatSpawnTime[msgNr];
			if(age < 10000.0f)
			{
				float alpha=1.0f;
				if (age>4000.0f)
					alpha=1.0f-(age-4000.0f)/6000.0f;
				gEnv->pGame->GetIGameFramework()->GetIUIDraw()->DrawText(30, 320+i*20, 0, 0, m_chatStrings[msgNr].c_str(), alpha, 0.1f, 1.0f, 0.3f, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP);
				current = true;
			}
			msgNr++;
			if(msgNr > CHAT_LENGTH-1)
				msgNr = 0;
		}

		if(!current)
			m_anyCurrentText = false;
	}*/

	//render input text and cursor
	if(m_isListening)
	{
		if (m_repeatEvent.keyId!=eKI_Unknown)
		{
			float repeatSpeed = 40.0;
			float nextTimer = (1000.0f/repeatSpeed); // repeat speed

			if (now - m_repeatTimer > nextTimer)
			{
				ProcessInput(m_repeatEvent);
				m_repeatTimer = now + nextTimer;
			}
		}

		//gEnv->pGame->GetIGameFramework()->GetIUIDraw()->DrawText(30, 450, 16.0f, 16.0f, m_inputText.c_str(), 1.0f, 0.1f, 1.0f, 0.3f, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP);
		if(stricmp(m_lastInputText.c_str(), m_inputText.c_str()))
		{
			m_flashChat->Invoke("setInputText", m_inputText.c_str());
			m_lastInputText = m_inputText;
			//m_lastUpdate = now;
			//m_showing = true;
		}

		//render cursor
		/*if(int(now) % 200 < 100)
		{
			string sub = m_inputText.substr(0, m_cursor);
			float width=0.0f;
			gEnv->pGame->GetIGameFramework()->GetIUIDraw()->GetTextDim(&width, 0, 16.0f, 16.0f, sub.c_str());
			gEnv->pGame->GetIGameFramework()->GetIUIDraw()->DrawText(30+width, 450, 16.0f, 17.0f, "_", 1.0f, 0.1f, 1.0f, 0.3f, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP, UIDRAWHORIZONTAL_LEFT, UIDRAWVERTICAL_TOP);
		}*/
	}

	/*if(m_showing && now - m_lastUpdate >= 3000)
	{
		m_flashChat->Invoke("hideChat", "");
		m_showing = false;
	}*/
}

bool CHUDTextChat::OnInputEvent(const SInputEvent &event )
{
	if(!m_flashChat)
		return false;

	//X-gamepad virtual keyboard input
	if(event.deviceId==eDI_XI && event.state == 1)
	{
		if(event.keyId == eKI_XI_DPadUp)
			VirtualKeyboardInput("up");
		else if(event.keyId == eKI_XI_DPadDown)
			VirtualKeyboardInput("down");
		else if(event.keyId == eKI_XI_DPadLeft)
			VirtualKeyboardInput("left");
		else if(event.keyId == eKI_XI_DPadRight)
			VirtualKeyboardInput("right");
		else if(event.keyId == eKI_XI_A)
			VirtualKeyboardInput("press");
	}

	if (event.deviceId!=eDI_Keyboard)
		return false;

	if (event.state==eIS_Released && m_isListening)
		m_repeatEvent.keyId = eKI_Unknown;

	if (event.state != eIS_Pressed)
		return false;

	if(gEnv->pConsole->GetStatus())
		return false;

	if (!gEnv->bMultiplayer)
		return false;

	m_repeatEvent = event;

	float repeatDelay = 200.0f;
	float now = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();

	m_repeatTimer = now+repeatDelay;

	bool isEnter = (event.keyId == eKI_Y || event.keyId == eKI_U);
	bool isClose = (event.keyId == eKI_Enter || event.keyId==eKI_NP_Enter);

	if (isClose && event.state==eIS_Pressed)		//end text chat
	{
		GetISystem()->GetIInput()->ClearKeyState();
		Flush();

		return true;
	}
	else if (m_isListening && event.keyId==eKI_Escape)
	{
		m_inputText.resize(0);
		GetISystem()->GetIInput()->ClearKeyState();
		Flush();
	}

	if (m_isListening)
		ProcessInput(event);

	return true;
}

void CHUDTextChat::Delete()
{
	if (!m_inputText.empty())
	{
		if(m_cursor < (int)m_inputText.length())
			m_inputText.erase(m_cursor, 1);
	}
}

void CHUDTextChat::Backspace()
{
	if(!m_inputText.empty())
	{
		if (m_cursor>0)
		{
			m_inputText.erase(m_cursor-1,1);
			m_cursor--;
		}
	}
}

void CHUDTextChat::Left()
{
	if(m_cursor)
		m_cursor--;
}

void CHUDTextChat::Right()
{
	if(m_cursor < (int)m_inputText.length())
		m_cursor++;
}

void CHUDTextChat::Insert(const char *key)
{
	if (key)
	{
		if(strlen(key)!=1)
			return;

		if(m_cursor < (int)m_inputText.length())
			m_inputText.insert(m_cursor, 1, key[0]);
		else
			m_inputText=m_inputText+key[0];
		m_cursor++;
	}
}

void CHUDTextChat::Flush(bool close)
{
	if(m_inputText.size() > 0)
	{
	//broadcast text / send to receivers ...
		EChatMessageType type = eChatToAll;
		if(m_teamChat)
			type = eChatToTeam;

		gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules()->SendChatMessage(
			type, gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId(), 0, m_inputText.c_str());

		m_inputText.clear();
		m_cursor = 0;
	}

	if (close)
	{
		m_isListening = false;
		GetISystem()->GetIInput()->SetExclusiveListener(NULL);
		//((CHUD*)m_parent)->ShowTextField(false);
		m_flashChat->Invoke("setVisibleChatBox", 0);
		m_flashChat->Invoke("GamepadAvailable", false);
		m_textInputActive = false;
	}
}

void CHUDTextChat::ProcessInput(const SInputEvent &event)
{
	if(gEnv->pConsole->GetStatus())
		return;

	if (event.keyId == eKI_Backspace)
		Backspace();
	else if (event.keyId == eKI_Delete)
		Delete();
	else if (event.keyId == eKI_Left)
		Left();
	else if (event.keyId == eKI_Right)
		Right();
	else if (event.keyId == eKI_Home)
	{
		m_cursor=0;
	}
	else if (event.keyId == eKI_End)
	{
		m_cursor=(int)m_inputText.length();
	}
	else if(event.keyId == eKI_Enter)
	{
		//this shoudn't happen (probably enter got processed repeatedly)
	}
	else
	{
		const char *key = GetISystem()->GetIInput()->GetKeyName(event, 1);
		Insert(key);
	}

	if (m_inputText.length()>60)
		Flush(false);
}

void CHUDTextChat::AddChatMessage(EntityId sourceId, const char* msg, int teamFaction)
{
	string sourceName;
	if(IEntity *pSource = gEnv->pEntitySystem->GetEntity(sourceId))
		sourceName = string(pSource->GetName());
  AddChatMessage(sourceName,msg,teamFaction);
}

void CHUDTextChat::AddChatMessage(const char* nick, const char* msg, int teamFaction)
{
  if(!m_flashChat)
    return;

  m_chatStrings[m_chatHead] = string(msg);
  m_chatSpawnTime[m_chatHead] = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();

  // flash stuff
  SFlashVarValue args[3] = {nick, msg, teamFaction};
  m_flashChat->Invoke("setChatText", args, 3);
  //m_showing = true;

  m_chatHead++;
  if(m_chatHead > CHAT_LENGTH-1)
    m_chatHead = 0;
  m_anyCurrentText = true;  
}

void CHUDTextChat::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_inputText);
	s->Add(m_lastInputText);
	for (int i=0; i<CHAT_LENGTH; i++)
		s->Add(m_chatStrings[i]);
}


void CHUDTextChat::ShowVirtualKeyboard(bool active)
{
	m_showVirtualKeyboard = active;
	if(m_flashChat)
		m_flashChat->Invoke("GamepadAvailable", active);
}

void CHUDTextChat::HandleFSCommand(const char *pCommand, const char *pArgs)
{
	if(!stricmp(pCommand, "sendChatText"))
	{
		string data(pArgs);
		for(int i = 0; i < data.length(); ++i)
		{
			string key(data[i]);
			Insert(key.c_str());
		}

		GetISystem()->GetIInput()->ClearKeyState();
		Flush();
	}
}

void CHUDTextChat::VirtualKeyboardInput(string direction)
{
	if(m_flashChat && m_textInputActive)
		m_flashChat->Invoke("moveCursor", direction.c_str());
}

void CHUDTextChat::OpenChat(int type)
{
	if(!m_isListening)
	{
		GetISystem()->GetIInput()->ClearKeyState();
		GetISystem()->GetIInput()->SetExclusiveListener(this);
		m_isListening = true;
		//((CHUD*)m_parent)->ShowTextField(true);
		m_flashChat->Invoke("setVisibleChatBox", 1);
		m_flashChat->Invoke("GamepadAvailable", m_showVirtualKeyboard);
		m_textInputActive = true;

		if(type == 2)
		{
			m_teamChat = true;
			m_flashChat->Invoke("setShowTeamChat");
		}
		else
		{
			m_teamChat = false;
			m_flashChat->Invoke("setShowGlobalChat");
		}

		//m_lastUpdate = now;
		//m_showing = true;

		m_repeatEvent = SInputEvent();
	}
}