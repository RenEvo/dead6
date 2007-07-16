#include "StdAfx.h"
#include "Radio.h"
#include "GameRules.h"
#include "Game.h"
#include "HUD/HUD.h"

static const int RADIO_GROUPS=4;
static const int RADIO_GROUP_SIZE=5;
const int CRadio::RADIO_MESSAGE_NUM=RADIO_GROUPS*RADIO_GROUP_SIZE;//sizeof(RadioMessages)/sizeof(RadioMessage_s);

//------------------------------------------------------------------------

CRadio::CRadio(CGameRules* gr):m_currentGroup(-1),m_pGameRules(gr)
{
//	gEnv->pInput->AddEventListener(this);
}

CRadio::~CRadio()
{
	gEnv->pInput->RemoveEventListener(this);
	CancelRadio();
}

static void PlaySound(const char* name)
{
	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(name,0);
	if (pSound)
		pSound->Play();
}

static void PlayVoice(const char* name)
{
	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(name,FLAG_SOUND_3D|FLAG_SOUND_VOICE);
	if (pSound)
	{
		pSound->SetPosition(g_pGame->GetIGameFramework()->GetClientActor()->GetEntity()->GetWorldPos());
		pSound->Play();
	}
}

void CRadio::CancelRadio()
{
	if(m_currentGroup!=-1)
	{
		m_currentGroup=-1;
		gEnv->pInput->SetExclusiveListener(0);
	}
}

static bool GetTeamRadioTable(CGameRules *gr,const string& team_name,SmartScriptTable& out_table)
{
	if(!gr)
		return false;

	IScriptTable *pTable=gr->GetEntity()->GetScriptTable();

	if(!pTable)
		return false;

	SmartScriptTable pTeamRadio;
	if(!pTable->GetValue("teamRadio",pTeamRadio))
		return false;

	if(!pTeamRadio->GetValue(team_name,out_table))
		return false;

	return true;
}

static bool GetRadioSoundName(CGameRules *gr,const string &teamName,const int groupId,const int keyId,char **ppSoundName=0,char **ppSoundText=0, int* pVariations = 0)
{
	SmartScriptTable radioTable;
	if(!GetTeamRadioTable(gr,teamName,radioTable))
		return false;

	SmartScriptTable groupTable;
	if(!radioTable->GetAt(groupId,groupTable))
		return false;


	SmartScriptTable soundTable;
	if(!groupTable->GetAt(keyId,soundTable))
		return false;

	ScriptAnyValue soundName;
	ScriptAnyValue soundText;
	ScriptAnyValue soundVariations = 1;
	
	if(!soundTable->GetAtAny(1,soundName) || !soundTable->GetAtAny(2,soundText))
		return false;

	soundTable->GetAtAny(3, soundVariations);
	if(pVariations)
		soundVariations.CopyTo(*pVariations);

	if(ppSoundName)
	{
		if(!soundName.CopyTo(*ppSoundName))
			return false;
	}

	if(ppSoundText)
	{
		if(!soundText.CopyTo(*ppSoundText))
			return false;
	}

	return true;
}

bool CRadio::OnAction(const ActionId& actionId, int activationMode, float value)
{
	if(!gEnv->bMultiplayer)
		return false;

	if(gEnv->pInput->GetExclusiveListener())
		return false;

	if(!g_pGame->GetIGameFramework()->GetClientActor() || g_pGame->GetIGameFramework()->GetClientActor()->GetHealth()<=0)
		return false;

	if(!m_TeamName.length())
		return false;

	const SGameActions& actions = g_pGame->Actions();

	int group=-1;

	if(actions.radio_group_0 == actionId)
		group=0;
	else if(actions.radio_group_1 == actionId)
		group=1;
	else if(actions.radio_group_2 == actionId)
		group=2;
	else if(actions.radio_group_3 == actionId)
		group=3;

	if(group==-1)
		return false;

	if(group==m_currentGroup)
	{
		PlaySound("Sounds/interface:menu:close");
		CancelRadio();
		if(g_pGame->GetHUD())
			g_pGame->GetHUD()->SetRadioButtons(false);
		gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("mp_radio",false);
		gEnv->pInput->RemoveEventListener(this);
		return true;
	}

	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->SetRadioButtons(true, group+1);

	m_currentGroup=group;

	PlaySound("Sounds/interface:menu:pop_up");

	gEnv->pInput->AddEventListener(this);
	gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("mp_radio",true);

	SmartScriptTable radioTable;
	if(!GetTeamRadioTable(m_pGameRules,m_TeamName,radioTable))
		return false;

	SmartScriptTable groupTable;
	if(!radioTable->GetAt(group+1,groupTable))
		return false;

	return true;
}	

bool CRadio::OnInputEvent( const SInputEvent &event )
{
	if(m_currentGroup==-1)
		return false;

	if (event.deviceId!=eDI_Keyboard)
		return false;

	if (event.state != eIS_Released)
		return false;

	if(gEnv->pConsole->GetStatus())
		return false;

	if (!gEnv->bMultiplayer)
		return false;

	const char* sKey = event.keyName.c_str();
	// nasty check, but fastest early out
	int iKey = -1;

	if(sKey && sKey[0] && !sKey[1])
	{
		iKey = atoi(sKey);
		if(iKey == 0 && sKey[0] != '0')
			iKey = -1;
	}

	if(iKey==-1)
		return false;

	if(!GetRadioSoundName(m_pGameRules,m_TeamName,m_currentGroup+1,iKey))
		return false;

	//PlayVoice(pSoundName);

	int id=(m_currentGroup*RADIO_GROUP_SIZE+iKey)-1;
	m_pGameRules->SendRadioMessage(gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId(),id);

	CancelRadio();

	if(g_pGame->GetHUD())
		g_pGame->GetHUD()->SetRadioButtons(false);

	gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->EnableFilter("mp_radio",false);
	gEnv->pInput->RemoveEventListener(this);

	return true;
}

void CRadio::OnRadioMessage(int id,const char* from)
{
	int groupId=id/RADIO_GROUP_SIZE;
	int keyId=id%RADIO_GROUP_SIZE;

	char *pSoundName,*pSoundText;
	int variations = 1;
	bool result=GetRadioSoundName(m_pGameRules,m_TeamName,groupId+1,keyId+1,&pSoundName,&pSoundText, &variations);

	assert(result);

	if(g_pGame->GetHUD())
	{
		string completeMsg;
		completeMsg=string(from)+" :radio: "+pSoundText;
		SGameObjectEvent evt(eCGE_MultiplayerChatMessage,eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void*)completeMsg.c_str());
		g_pGame->GetHUD()->HandleEvent(evt);
		//PlaySound("Sounds/interface:suit:generic_beep");

		string sound = pSoundName;
		if(variations > 1)
		{
			int rand = Random(3);
			sound += "_0%1d";
			sound.Format(sound.c_str(), rand+1);
		}

		PlayVoice(sound.c_str());
	}
}

void CRadio::SetTeam(const string& name)
{
	m_TeamName=name;
}

void CRadio::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->Add(m_TeamName);
}