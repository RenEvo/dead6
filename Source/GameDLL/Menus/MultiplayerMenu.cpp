/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Multiplayer lobby

-------------------------------------------------------------------------
History:
- 12/2006: Created by Stas Spivakov

*************************************************************************/
#include "StdAfx.h"
#include "MultiplayerMenu.h"
#include "MPLobbyUI.h"
#include "INetwork.h"
#include "INetworkService.h"
#include "GameCVars.h"
#include "HUD/GameFlashLogic.h"
#include "CreateGame.h"
#include "Game.h"
#include "GameNetworkProfile.h"

enum EServerInfoKey
{
  eSIK_unknown,
  eSIK_hostname,
  eSIK_mapname,
  eSIK_numplayers,
  eSIK_maxplayers,
  eSIK_gametype,
  eSIK_dedicated,
  eSIK_official,
  eSIK_frinedlyFire,
  eSIK_gamever,
  eSIK_timelimit,
  eSIK_anticheat,
  eSIK_voicecomm,
  eSIK_private,

  //player,
  eSIK_playerName,
  eSIK_playerTeam,
  eSIK_playerRank,
  eSIK_playerKills,
  eSIK_playerDeaths
};


static TKeyValuePair<EServerInfoKey,const char*>
gServerKeyNames[] = { 
                      {eSIK_unknown,""},
                      {eSIK_hostname,"hostname"},
                      {eSIK_mapname,"mapname"},
                      {eSIK_numplayers,"numplayers"},
                      {eSIK_maxplayers,"maxplayers"},
                      {eSIK_gametype,"gametype"},
                      {eSIK_dedicated,"dedicated"},
                      {eSIK_official,"official"},
                      {eSIK_frinedlyFire,"friendlyfire"},
                      {eSIK_timelimit,"timelimit"},
                      {eSIK_gamever,"gamever"}, 
                      {eSIK_anticheat,"anticheat"},
                      {eSIK_voicecomm,"voicecomm"},
                      {eSIK_private,"password"},
                      {eSIK_playerName,"player"},
                      {eSIK_playerTeam,"team"},
                      {eSIK_playerRank,"rank"},
                      {eSIK_playerKills,"kills"},
                      {eSIK_playerDeaths,"deaths"}
                    };


class  CMultiPlayerMenu::CUI : public CMPLobbyUI
{
public:
  CUI(IFlashPlayer* plr):CMPLobbyUI(plr),m_menu(0),m_curTab(0)
  {
  }

  void SetMenu(CMultiPlayerMenu* m)
  {
    m_menu = m;
  }
  int GetCurTab()const
  {
    return m_curTab;
  }
protected:
  virtual void OnActivateTab(int tab);
  virtual void OnDeactivateTab(int tab);
  virtual bool OnHandleCommand(EGsUiCommand cmd, const char* pArgs);
  virtual void OnChatModeChange(bool global, int id);
  virtual void OnUserSelected(EChatCategory cat, int id);
  virtual void OnAddBuddy(const char* nick, const char* request);
  virtual void OnRemoveBuddy(const char* nick);
  virtual void OnShowUserInfo(int id, const char* nick);
  virtual void OnJoinWithPassword(int id);
  virtual void OnAddIgnore(const char* nick);
  virtual void OnStopIgnore(const char* nick);
  virtual bool CanAdd(EChatCategory cat, int id, const char* nick);
  virtual bool CanIgnore(EChatCategory cat, int id, const char* nick);

private:
  CMultiPlayerMenu* m_menu;
  int m_curTab;
};

struct CMultiPlayerMenu::SGSBrowser : public IServerListener
{
  
  SGSBrowser():
  m_menu(0),
  m_pendingUpdate(0)
  {

  }

  void  SetMenu(CMultiPlayerMenu* menu)
  {
    m_menu = menu;
  }

  virtual void UpdateServer(const int id,const SBasicServerInfo* info)
  {
    OnServer(id,info,true);
  }

  virtual void NewServer(const int id,const SBasicServerInfo* info)
  {
    OnServer(id,info,false);
  }

  void OnServer(const int id,const SBasicServerInfo* info, bool update)
  {
    CMPLobbyUI::SServerInfo si;
    si.m_numPlayers   = info->m_numPlayers;
    si.m_maxPlayers   = info->m_maxPlayers;
    si.m_private      = info->m_private;
    si.m_hostName     = info->m_hostName;
    si.m_publicIP     = info->m_publicIP;
    si.m_publicPort   = info->m_publicPort;
    si.m_hostPort     = info->m_hostPort;
    si.m_privateIP    = info->m_privateIP;
    si.m_hostName     = info->m_hostName;
    si.m_mapName      = info->m_mapName;
    si.m_gameVersion  = info->m_gameVersion;
    si.m_gameTypeName = GetGameType(info->m_gameType);
    si.m_gameType     = info->m_gameType;
    si.m_official     = info->m_official;
    si.m_anticheat    = info->m_anticheat;
    si.m_voicecomm    = info->m_voicecomm; 
    si.m_friendlyfire = info->m_friendlyfire;
    si.m_dx10         = info->m_dx10;
    si.m_dedicated    = info->m_dedicated;
    si.m_serverId     = id;
    si.m_ping         = 10000;
    for(int i=0;i<m_menu->m_favouriteServers.size();++i)
    {
      SStoredServer &srv = m_menu->m_favouriteServers[i];
      if(srv.ip == si.m_publicIP && srv.port == si.m_hostPort)
        si.m_favorite = true;
    }

    for(int i=0;i<m_menu->m_recentServers.size();++i)
    {
      SStoredServer &srv = m_menu->m_recentServers[i];
      if(srv.ip == si.m_publicIP && srv.port == si.m_hostPort)
        si.m_recent = true;
    }
    if(update)
      m_menu->m_ui->UpdateServer(si);
    else
      m_menu->m_ui->AddServer(si);
  }

  virtual void RemoveServer(const int id)
  {
    m_menu->m_ui->RemoveServer(id);
  }

  virtual void UpdatePing(const int id, const int ping)
  {
    m_menu->m_ui->UpdatePing(id,ping);
  }

  virtual void UpdateValue(const int id,const char* name,const char* value)
  {
    string val(name);
    EServerInfoKey key = KEY_BY_VALUE(val,gServerKeyNames);

    CMPLobbyUI::SServerInfo si;
    if(m_menu->m_ui->GetServer(id,si))
    {
      bool basic = true;
      switch(key)
      {
      case eSIK_hostname:
        si.m_hostName = value;
        break;
      case eSIK_mapname:
        si.m_mapName = value;
        break;
      case eSIK_numplayers:
        si.m_numPlayers = atoi(value);
        m_details.m_players.resize(si.m_numPlayers);
        break;
      case eSIK_maxplayers:
        si.m_maxPlayers = atoi(value);
        break;
      case eSIK_gametype:
        si.m_gameTypeName = GetGameType(value);
        si.m_gameType = value;
        break;
      case eSIK_official:
        si.m_official = atoi(value)!=0;
        break;
      case eSIK_anticheat:
        si.m_anticheat = atoi(value)!=0;
        break;
      case eSIK_private:
        si.m_private = atoi(value)!=0;
        break;
      default:
        basic = false;
      }

      if(basic)
      {
        m_menu->m_ui->UpdateServer(si);
      }
    }

    if(id != m_pendingUpdate)
      return;

    switch(key)
    {
    case eSIK_dedicated:
      m_details.m_dedicated = IsTrue(value);
      break;
    case eSIK_frinedlyFire:
      m_details.m_friendlyfire = IsTrue(value);
      break;
    case eSIK_timelimit:
      m_details.m_timelimit = atoi(value);
      break;
    case eSIK_gamever:
      m_details.m_gamever = value;
      break;
    case eSIK_gametype:
      m_details.m_gamemode = IsTrue(value);
      break;
    case eSIK_voicecomm:
      m_details.m_voicecomm = IsTrue(value);
      break;
    case eSIK_anticheat:
      m_details.m_anticheat = IsTrue(value);
      break;
    default:
      return;
    } 
  }

  virtual void UpdatePlayerValue(const int id,const int playerNum,const char* name,const char* value)
  {
    if(id != m_pendingUpdate)
      return;
    if(playerNum>=m_details.m_players.size())
      return;
    CMPLobbyUI::SServerDetails::SPlayerDetails& pl = m_details.m_players[playerNum];
    string val(name);
    EServerInfoKey key = KEY_BY_VALUE(val,gServerKeyNames);
    switch(key)
    {
    case eSIK_playerName:
      pl.m_name = value;
      break;
    case eSIK_playerTeam:
      pl.m_team = atoi(value);
      break;
    case eSIK_playerRank:
      {
        int rank = atoi(value);
        if(rank)
        {
          pl.m_rank.Format("@ui_short_rank_%d",rank);
        }
        else
        {
          pl.m_rank="";
        }
      }
      break;
    case eSIK_playerKills:
      pl.m_kills = atoi(value);
      break;
    case eSIK_playerDeaths:
      pl.m_deaths = atoi(value);
      break;
    }
  }

  virtual void UpdateTeamValue(const int id,const int teamNum,const char *name,const char* value)
  {
  }

  virtual void OnError(const EServerBrowserError)
  {
  }

  virtual void UpdateComplete(bool cancel)
  {
    m_menu->m_ui->SetUpdateProgress(m_menu->m_browser->GetPendingQueryCount(),m_menu->m_browser->GetServerCount());
		m_menu->m_ui->FinishUpdate();
  }

  virtual void ServerUpdateFailed(const int id)
  {
    m_menu->m_ui->UpdatePing(id,9999);
  }

  virtual void ServerUpdateComplete(const int id)
  {
    if(id == m_pendingUpdate)
    {
      m_menu->m_ui->SetServerDetails(m_details);
      m_pendingUpdate = -1;
    }
  }

  virtual void ServerDirectConnect(bool neednat, uint ip, ushort port)
  {
    string connect;
    if(neednat)
    {
      int cookie = rand() + (rand()<<16);
      connect.Format("connect <nat>%d",cookie);
      m_menu->m_browser->SendNatCookie(ip,port,cookie);
    }
    else
    {
      connect.Format("connect %d.%d.%d.%d:%d",ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF,port);
    }
    g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(connect.c_str());
  }

  const string& GetGameType(const char* gt)
  {
    static string gametype;
    gametype = gt;
    std::map<string,string>::const_iterator it = m_gameTypes.find(gametype);
    if(it==m_gameTypes.end())
    {
      static wstring wType;
      static string  cType;
      cType = "@ui_rules_";
      cType += gametype;
      if(gEnv->pSystem->GetLocalizationManager()->LocalizeLabel(cType,wType))
      {
        ConvertWString(wType,cType);
      }
      else
        cType = gametype;
      it = m_gameTypes.insert(std::make_pair(gametype,cType)).first;
    }
    return it->second;
  }

  std::map<string,string>     m_gameTypes;

  CMultiPlayerMenu*           m_menu;
  int                         m_pendingUpdate;
  CMPLobbyUI::SServerDetails  m_details;
};

struct CMultiPlayerMenu::SGSNetworkProfile : public INProfileUI
{
  SGSNetworkProfile():
  m_menu(0),
  m_selectedBuddy(-1)
  {

  }

  void  SetMenu(CMultiPlayerMenu* menu)
  {
    m_menu = menu;
  }

  virtual void AddBuddy(SChatUser usr)
  {
    m_nicks[usr.m_id] = usr.m_nick;
    m_menu->m_ui->AddChatUser(eCC_buddy, usr.m_id, usr.m_nick);
    m_menu->m_ui->ChatUserStatus(eCC_buddy, usr.m_id, usr.m_status, usr.m_location);
  }

  virtual void UpdateBuddy(SChatUser usr)
  {
//    m_menu->m_ui->ClearUserList();
    m_menu->m_ui->ChatUserStatus(eCC_buddy, usr.m_id, usr.m_status, usr.m_location);
  }
  
  virtual void RemoveBuddy(int id)
  {
    m_menu->m_ui->RemoveCharUser(eCC_buddy,id);
  }
  
  virtual void AddIgnore(SChatUser s)
  {
    m_menu->m_ui->AddChatUser(eCC_ignored, s.m_id, s.m_nick);
    m_menu->m_ui->UpdateUsers();
  }

  virtual void RemoveIgnore(int id)
  {
    m_menu->m_ui->RemoveCharUser(eCC_ignored,id);
  }

  virtual void OnMessage(int id, const char* message)
  {
    string msg = message;
    
    TNicksMap::iterator it = m_nicks.find(id);
    if(it!=m_nicks.end())
      msg = string("From [") + it->second + "] : " + msg;
    
    m_menu->m_ui->AddChatText(msg);
  }

  virtual void OnAuthRequest(int id, const char* nick, const char* reason)
  {
    m_menu->m_ui->ShowInvitation(id,nick,reason);
  }

  virtual void ProfileInfo(int id, SUserInfo ui)
  {
    m_menu->m_ui->SetInfoScreenId(id);
    m_menu->m_ui->EnableInfoScreenContorls(m_menu->m_profile->CanInvite(id),m_menu->m_profile->CanIgnore(id));
    m_menu->m_ui->SetProfileInfo(ui);    
  }
  
  virtual void ShowError()
  {

  }

  virtual void SearchResult(int id, const char* nick)
  {
    m_menu->m_ui->AddSearchResult(id,nick);
  }

  virtual void SearchCompleted()
  {
    m_menu->m_ui->EnableSearchButton(true);
  }


  void SelectUser( int id )
  {
    m_selectedBuddy = id;
  }

  void SendMessage(const char* message)
  {
    if(m_selectedBuddy==-1)
      return;

    string msg = message;
    
    TNicksMap::iterator it = m_nicks.find(m_selectedBuddy);
    if(it!=m_nicks.end())
    {
      msg = string("To [") + it->second + "] : " + message;
      m_menu->m_ui->AddChatText(msg);
    }
    m_menu->m_profile->SendBuddyMessage(m_selectedBuddy,message);
  }

  virtual void AddFavoriteServer(uint ip, ushort port)
  {
    SStoredServer s;
    s.ip = ip;
    s.port = port;
    m_menu->m_favouriteServers.push_back(s);
  }

  virtual void AddRecentServer(uint ip, ushort port)
  {
    SStoredServer s;
    s.ip = ip;
    s.port = port;
    m_menu->m_recentServers.push_back(s);
  }


  int                         m_selectedBuddy;
  typedef std::map<int,string> TNicksMap;
  TNicksMap                   m_nicks;
  CMultiPlayerMenu*           m_menu;
};

struct CMultiPlayerMenu::SChat : public IChatListener
{
  struct SChatUser
  {
    int    id;
    string nick;    
  };

  SChat(CMultiPlayerMenu* p):m_parent(p),m_userId(0)
  {
  }

  virtual void Joined(EChatJoinResult)
  {
    m_parent->m_profile->SetChattingStatus();
  }

  virtual void Message(const char* from, const char* message, ENetworkChatMessageType t)
  {
    switch(t)
    {
    case eNCMT_say:
      {
        string msg;
        msg.Format("[%s] : %s",from,message);
        m_parent->m_ui->AddChatText(msg);
      }
      break;
    case eNCMT_server:
      break;
    case eNCMT_data:
      break;
    }
  }

  virtual void ChatUser(const char* nick, EChatUserStatus st)
  {
    switch(st)
    {
    case eCUS_inchannel:
      {
        SChatUser u;
        u.id = m_userId++;
        u.nick = nick;
        m_userlist.push_back(u);
        m_parent->m_ui->AddChatUser(eCC_global, u.id, nick);
      }
      break;
    case eCUS_joined:
      {
        SChatUser u;
        u.id = m_userId++;
        u.nick = nick;
        m_userlist.push_back(u);
        m_parent->m_ui->AddChatUser(eCC_global, u.id, nick);
      }
      break;
    case eCUS_left:
      {
        for(int i=0;i<m_userlist.size();++i)
        {
          if(m_userlist[i].nick == nick)
          {
            m_parent->m_ui->RemoveCharUser(eCC_global,m_userlist[i].id);
            m_userlist.erase(m_userlist.begin()+i);
            break;
          }
        }
      }
      break;
    }
  }
  virtual void OnError(int code)
  {

  }
  
  std::vector<SChatUser>  m_userlist;
  int                     m_userId;
  CMultiPlayerMenu*       m_parent;
};

CMultiPlayerMenu::CMultiPlayerMenu(bool lan, IFlashPlayer* plr, CMPHub* hub):
m_browser(0),
m_profile(0),
m_serverlist(new SGSBrowser()),
m_buddylist(new SGSNetworkProfile()),
m_ui(new CUI(plr)),
m_creategame(new SCreateGame(plr)),
m_lan(lan),
m_hub(hub),
m_selectedCat(eCC_global),
m_selectedId(),
m_chat(0)
{
  m_buddylist->SetMenu(this);
  m_ui->SetMenu(this);
  m_ui->EnableTabs(!lan,!lan,!lan);

  m_profile = m_hub->GetProfile();
 
  INetworkService *serv=GetISystem()->GetINetwork()->GetService("GameSpy");
  if(serv)
  {
    m_browser = serv->GetServerBrowser();
    if(m_browser->IsAvailable())
    {
        m_browser->SetListener(m_serverlist.get());
        m_browser->Start(m_lan);
        m_browser->Update();
    }
    m_serverlist->SetMenu(this);
    if(!lan)
    {
      m_chat = serv->GetNetworkChat();
      m_chatlist.reset(new SChat(this));
      m_chat->SetListener(m_chatlist.get());
      m_chat->Join();
      m_hub->GetProfile()->InitUI(m_buddylist.get());
    }
  }

  m_ui->SetJoinButtonMode(m_hub->IsIngame()?eJBM_disconnect:eJBM_default);
  //read servers back

  //test
  /*SStoredServer srv;
  srv.ip = 1677264596;
  srv.port = 64087;
  m_recentServers.push_back(srv);
  m_favouriteServers.push_back(srv);
  m_favouriteServers.push_back(srv);
  srv.ip = 1677264555;
  m_favouriteServers.push_back(srv);*/
  //m_profile->ReadStats(new SServerListReader(this,false));
  //m_profile->ReadStats(new SServerListReader(this,true));
}

CMultiPlayerMenu::~CMultiPlayerMenu()
{
  m_ui->SetStatusString("");
  if(m_hub && m_hub->GetProfile())
    m_hub->GetProfile()->DestroyUI();

  if(m_chat)
  {
    m_chat->Leave();
    m_chat->SetListener(0);
  }

	if(m_browser)
  {
    m_browser->SetListener(0);
    m_browser->Stop();
  }
}

bool CMultiPlayerMenu::HandleFSCommand(EGsUiCommand cmd, const char* pArgs)
{
	if(!m_browser)
		return false;

  bool handled = true;

  handled = m_ui->HandleFSCommand(cmd,pArgs)||handled;

  if(m_creategame.get())
    handled = m_creategame->HandleFSCommand(cmd,pArgs) || handled;

  return handled;
}

void CMultiPlayerMenu::OnUIEvent(const SUIEvent& event)
{
  if(event.event == eUIE_connectFailed)
  {
    m_hub->CloseLoadingDlg();
    m_hub->ShowError(event.descrpition);
  }
}

void    CMultiPlayerMenu::UpdateServerList()
{
  switch(m_ui->GetCurTab())
  {
  case 0:
    m_ui->ClearServerList();
    m_ui->StartUpdate();
    m_ui->SetUpdateProgress(0,-1);
    m_browser->Update();
    break;
  case 1:
    for(int i=0;i<m_favouriteServers.size();++i)
      m_browser->BrowseForServer(m_favouriteServers[i].ip,m_favouriteServers[i].port);
    break;
  case 2:
    for(int i=0;i<m_recentServers.size();++i)
      m_browser->BrowseForServer(m_recentServers[i].ip,m_recentServers[i].port);
    break;
  }
}

void    CMultiPlayerMenu::StopServerListUpdate()
{
    m_browser->Stop();
    m_ui->FinishUpdate();
}
  
void CMultiPlayerMenu::SelectServer(int id)
{
	//do feed UI with players&teams info
  m_browser->UpdateServerInfo(id);
}

void CMultiPlayerMenu::JoinServer()
{
	CMPLobbyUI::SServerInfo serv;
  if(m_ui->GetSelectedServer(serv))
  {
    if(m_profile)
      m_profile->SetPlayingStatus(serv.m_publicIP,serv.m_hostPort,serv.m_publicPort,serv.m_gameType);
    if(m_lan)
    {
      uint ip = serv.m_publicIP;
      string connect;
      connect.Format("connect %d.%d.%d.%d:%d",ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF,serv.m_hostPort);
      g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(connect.c_str());
    } 
    else
      m_browser->CheckDirectConnect(serv.m_serverId,serv.m_hostPort);
    m_hub->ShowLoadingDlg(string().Format("Connecting to %s...",serv.m_hostName.c_str()));
  }
}

void    CMultiPlayerMenu::SaveServerLists()
{
/*  if(m_favouriteServers.size())
    m_profile->WriteStats(new SServerListWriter(this,false));
  if(m_recentServers.size())
    m_profile->WriteStats(new SServerListWriter(this,true));*/
}

bool CMultiPlayerMenu::CUI::OnHandleCommand(EGsUiCommand cmd, const char* pArgs)
{
  bool handled = true;
  switch(cmd)
  {
  case eGUC_update:
    m_menu->UpdateServerList();
    break;
  case eGUC_stop:
    m_menu->StopServerListUpdate();
    break;
  case eGUC_acceptBuddy:
    {
      int id = atoi(pArgs);
      m_menu->m_profile->AcceptBuddy(id,true);
      m_menu->m_profile->RequestBuddy(id, "I've added you as a buddy. Please authorize me.");
    }
    break;
  case eGUC_declineBuddy:
    {
      int id = atoi(pArgs);
      m_menu->m_profile->AcceptBuddy(id,false);
    }
    break;
  case eGUC_join:
    {
      SServerInfo si;
      if(GetSelectedServer(si))
      {
        SStoredServer srv;
        srv.ip = si.m_publicIP;
        srv.port = si.m_hostPort;
        m_menu->m_recentServers.push_back(srv);
        if(si.m_private)
          OpenPasswordDialog(si.m_serverId);
        else
          m_menu->JoinServer();
      }
    }
    break;
  case eGUC_joinIP:
    {
      string connect;
      connect.Format("connect %s",pArgs);
      if(connect[connect.size()-1]==':')
        connect = connect.substr(0,connect.size()-1);
      SetJoinPassword();
      m_menu->m_hub->ShowLoadingDlg(string().Format("Connecting to %s...",pArgs));
      g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(connect.c_str());
    }
    break;
  case eGUC_joinBuddy:
    break;
  case eGUC_disconnect:
    g_pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
    break;
  case eGUC_displayServerList:
    SetUpdateProgress(m_menu->m_browser->GetPendingQueryCount(),m_menu->m_browser->GetServerCount());
    break;
  case eGUC_chat:
    {
      if(m_menu->m_buddylist->m_selectedBuddy!=-1)
        m_menu->m_buddylist->SendMessage(pArgs);
      else
      {
        if(m_menu->m_chat)
          m_menu->m_chat->Say(pArgs);
      }
    }
    break;
  case eGUC_find:
    EnableSearchButton(false);
    ClearSearchResults();
    m_menu->m_profile->SearchUsers(pArgs);
    break;
  case eGUC_selectServer:

    //break;
  case eGUC_refreshServer:
    {
      ResetServerDetails();
      SServerInfo si;
      if(m_menu->m_serverlist->m_pendingUpdate = GetSelectedServer(si))
      {
        m_menu->m_serverlist->m_pendingUpdate = si.m_serverId;
        m_menu->m_serverlist->m_details = CMPLobbyUI::SServerDetails();
        m_menu->m_browser->UpdateServerInfo(si.m_serverId);
      }
    }
    break;
  case eGUC_addFavorite:
    {
      SServerInfo svr;
      if(GetSelectedServer(svr))
      {
        SStoredServer s;
        s.ip = svr.m_publicIP;
        s.port = svr.m_hostPort;
        m_menu->m_favouriteServers.push_back(s);
        m_menu->SaveServerLists();
      }
    }
    break;
  case eGUC_removeFavorite:
    {
      SServerInfo svr;
      if(GetSelectedServer(svr))
      {
        for(int i=0;i<m_menu->m_favouriteServers.size();++i)
        {
          if(m_menu->m_favouriteServers[i].ip == svr.m_publicIP && 
             m_menu->m_favouriteServers[i].port == svr.m_hostPort)
          {
            m_menu->m_favouriteServers[i].ip = 0;
            m_menu->m_favouriteServers[i].port = 0;
          }
        }
        m_menu->SaveServerLists();
      }
    }    
    break;
  default:
    handled = false;
  }
  return handled;
}

void CMultiPlayerMenu::CUI::OnActivateTab(int tab)
{
  switch(tab)
  {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    //m_menu->m_buddylist->m_nicks.clear();
    //ClearUserList();
    //m_menu->m_profile->UpdateBuddies();
    //m_menu->m_buddylist->AddIgnore(0,"asdasd");
    break;
  }
  m_curTab = tab;
}

void CMultiPlayerMenu::CUI::OnDeactivateTab(int tab)
{
  switch(tab)
  {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  }  
}

void CMultiPlayerMenu::CUI::OnChatModeChange(bool global, int id)
{
  m_menu->m_buddylist->m_selectedBuddy = global?-1:id;
}

void CMultiPlayerMenu::CUI::OnUserSelected(EChatCategory cat, int id)
{
  m_menu->m_selectedCat = cat;
  m_menu->m_selectedId = id;
}

void CMultiPlayerMenu::CUI::OnAddBuddy(const char* nick, const char* request)
{
  m_menu->m_profile->RequestBuddy(nick,request);  
}

void CMultiPlayerMenu::CUI::OnRemoveBuddy(const char* nick)
{
  m_menu->m_profile->RemoveBuddy(nick);
}

bool CMultiPlayerMenu::CUI::CanAdd(EChatCategory cat, int id, const char* nick)
{
  if(cat == eCC_buddy || cat == eCC_ignored)
    return m_menu->m_profile->CanInvite(id);
  else
    return m_menu->m_profile->CanInvite(nick);
}

bool CMultiPlayerMenu::CUI::CanIgnore(EChatCategory cat, int id, const char* nick)
{
  if(cat == eCC_buddy || cat == eCC_ignored)
    return m_menu->m_profile->CanIgnore(id);
  else
    return m_menu->m_profile->CanIgnore(nick);
}

void CMultiPlayerMenu::CUI::OnShowUserInfo(int id, const char* nick)
{
  if(nick)
  {
    m_menu->m_profile->GetUserInfo(nick);
  }  
  else//id is valid
  {
    m_menu->m_profile->GetUserInfo(id);
  }
}

void CMultiPlayerMenu::CUI::OnJoinWithPassword(int id)
{
  SetJoinPassword();
  m_menu->JoinServer();
}

void CMultiPlayerMenu::CUI::OnAddIgnore(const char* nick)
{
  m_menu->m_profile->AddIgnore(nick);
}

void CMultiPlayerMenu::CUI::OnStopIgnore(const char* nick)
{
  m_menu->m_profile->StopIgnore(nick);
}