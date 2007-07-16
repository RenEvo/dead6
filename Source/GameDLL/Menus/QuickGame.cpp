/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Quick game screen

-------------------------------------------------------------------------
History:
- 03/12/2006: Created by Stas Spivakov

*************************************************************************/
#include "StdAfx.h"
#include "INetwork.h"
#include "INetworkService.h"
#include "QuickGame.h"

#include "Game.h"
#include "GameCVars.h"
#include "MPHub.h"

class CQuickGameDlg : public CMPHub::CDialog
{
public:
  CQuickGameDlg(CQuickGame* qg):m_qg(qg)
  {

  }

  virtual bool OnCommand(EGsUiCommand cmd, const char* pArgs)
  {
    if(cmd==eGUC_dialogClosed)
    {
      if(m_qg->IsSearching())
      {
        m_qg->Cancel();
      }
      else//connecting
      {
        gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
      }
      Close();
      return true;
    }
    return false;
  }
  virtual void OnUIEvent(const SUIEvent& event)
  {
    switch(event.event)
    {
    case eUIE_quickGame:
      if(event.param == 1)
        m_connectingTo = event.descrpition;
      m_hub->SetLoadingDlgText(string().Format("Connecting to %s ...",m_connectingTo.c_str()),false);
      break;
    case eUIE_connectFailed:
      //
      m_hub->ShowError(string().Format("Error : %s",event.descrpition));
      Close();
      break;
    case eUIE_disconnnect:
      m_hub->ShowError(string().Format("Error : %s",event.descrpition));
      Close();
      break;
    case eUIE_connect:
      m_hub->SetLoadingDlgText(string().Format("Connected to %s. Transferring data...",m_connectingTo.c_str()),false);
      break;
    }
  }
  virtual void OnClose()
  {
    m_hub->CloseLoadingDlg();
  }
  virtual void OnShow()
  {
    m_hub->ShowLoadingDlg("Searching for quick game");
  }
  void OnFinished()
  {
    m_hub->ShowError(string().Format("Game matching your preferences not found."));
    Close();
  }
private:
  string        m_connectingTo;
  CQuickGame*   m_qg;
};


struct CQuickGame::SQGServerList : public IServerListener
{
  struct SRatedServer
  {
    int id;
    uint port;

    uint ping;
    uint players;
    uint maxplayers;
    string name;
    string map;
    string mode;
    bool   mapmatch;

    int score;
    bool operator<(const SRatedServer& r)const
    {
      return score < r.score;
    }
  };

  SQGServerList(CQuickGame* qg):m_qg(qg)
  {}

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
    SRatedServer srv;
    srv.id = id;
    srv.ping = 99999;
    srv.score = 0;
    //drop full servers
    if(info->m_maxPlayers == info->m_numPlayers)
      return;
    //drop password protected servers
    if(info->m_private)
      return;
    //drop non-matching game type
    if(!m_gameMode.empty())
    {
      if(m_gameMode != info->m_gameType)
        return;
    }
    //drop players
    if(m_minPlayers)
      if(info->m_numPlayers<m_minPlayers)
        return;

    srv.ping = -1;
    srv.players = info->m_numPlayers;
    srv.maxplayers = info->m_maxPlayers;
    srv.map = info->m_mapName;
    srv.mode = info->m_gameType;
    srv.name = info->m_hostName;

    if(!m_mapName.empty())
    {
      srv.mapmatch = m_mapName == info->m_mapName;
    }
    else 
      srv.mapmatch = true;

    srv.port = info->m_hostPort;
    if(update)
    {
      int idx = Find(id);
      if(idx != -1)
        m_servers[idx] = srv;
    }
    else
      m_servers.push_back(srv);
  }

  int Find(int id)
  {
    for(int i=0;i<m_servers.size();++i)
    {
      if(m_servers[i].id == id)
      {
        return i;
      }
    }
    return -1;
  }

  static void ComputeScore(SRatedServer& svr)
  {
    svr.score = ((128-min(127u,svr.players))<<10) + min(1023u, svr.ping);
    if(svr.ping > m_ping1)
      svr.score += 1<<20;

    if(svr.players < (svr.maxplayers/2))
      svr.score += 1<<21;

    if(svr.ping > m_ping2)
      svr.score += 1<<22;
    if(!svr.mapmatch)
      svr.score += 1<<23;
  }

  virtual void RemoveServer(const int id)
  {
    int idx = Find(id);
    if(idx!=-1)
      m_servers.erase(m_servers.begin()+idx);
  }

  virtual void UpdatePing(const int id,const int ping)
  {
    int idx = Find(id);
    if(idx!=-1)
    {
      m_servers[idx].ping = ping;
    }
  }

  virtual void UpdateValue(const int id,const char* name,const char* value)
  {

  }

  virtual void UpdatePlayerValue(const int id,const int playerNum,const char* name,const char* value)
  {

  }

  virtual void UpdateTeamValue(const int id,const int teamNum,const char *name,const char* value)
  {
  } 

  virtual void OnError(const EServerBrowserError)
  {

  }

  virtual void UpdateComplete(bool cancel)
  {
    if(cancel)
      return;
    if(m_servers.empty())
    {
      m_qg->NextStage();
      return;
    }
    if(m_servers.size()>=1)
    {
      std::for_each(m_servers.begin(),m_servers.end(),&SQGServerList::ComputeScore);

      std::sort(m_servers.begin(),m_servers.end());
    }

    m_qg->m_browser->Stop();

    if(g_pGameCVars->g_quickGame_debug!=0)
    {
      CryLog("Quick Game debug output. Phase %d\n", m_qg->GetStage());
      int num = min(g_pGameCVars->g_quickGame_debug,int(m_servers.size()));
      for(int i=0;i<num;++i)
      {
        SRatedServer &svr = m_servers[i];
        CryLog("\tscore:0x%X plrs:%d ping:%d map:%s mode:%s", svr.score, svr.players, svr.ping, svr.map.c_str(), svr.mode.c_str());
      }
      CryLog("Total %d servers\n", m_servers.size());
      m_qg->NextStage();
    }
    else
    {
      m_qg->m_ui->OnUIEvent(SUIEvent(eUIE_quickGame,1,m_servers[0].name));
      m_qg->m_browser->CheckDirectConnect(m_servers[0].id,m_servers[0].port);
    }
    m_qg->m_searching = false;
  }

  virtual void ServerUpdateFailed(const int id)
  {
    RemoveServer(id);
  }

  virtual void ServerUpdateComplete(const int id)
  {

  }

  virtual void ServerDirectConnect(bool neednat, uint ip, ushort port)
  {
    string connect;
    if(neednat)
    {
      int cookie = rand() + (rand()<<16);
      connect.Format("connect <nat>%d",cookie);
      m_qg->m_browser->SendNatCookie(ip,port,cookie);
    }
    else
    {
      connect.Format("connect %d.%d.%d.%d:%d",ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF,port);
    }
    g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(connect.c_str()); 
  }
  void Reset()
  {
    m_servers.resize(0);
  }
  std::vector<SRatedServer> m_servers;
  string                    m_gameMode;
  string                    m_mapName;
  int                       m_minPlayers;
  bool                      m_preferLan;
  bool                      m_preferFav;
  bool                      m_preferCountry;
  static  uint              m_ping1;
  static  uint              m_ping2;
  CQuickGame*               m_qg;
};

uint CQuickGame::SQGServerList::m_ping1 = 0;
uint CQuickGame::SQGServerList::m_ping2 = 0;


CQuickGame::CQuickGame():
m_stage(0),
m_browser(0),
m_searching(false)
{
  m_list.reset(new SQGServerList(this));
}

CQuickGame::~CQuickGame()
{
}

void CQuickGame::StartSearch(CMPHub* hub)
{
  Cancel();

  if(hub)//if we want UI
  {
    if(!m_ui.get())
      m_ui.reset(new CQuickGameDlg(this));
    m_ui->Show(hub);
  }

  m_list->m_gameMode = g_pGameCVars->g_quickGame_mode->GetString();
  m_list->m_mapName = g_pGameCVars->g_quickGame_map->GetString();
  m_list->m_minPlayers = g_pGameCVars->g_quickGame_min_players;
  m_list->m_preferLan = g_pGameCVars->g_quickGame_prefer_lan!=0;
  m_list->m_preferFav = g_pGameCVars->g_quickGame_prefer_favorites!=0;
  m_list->m_preferCountry = g_pGameCVars->g_quickGame_prefer_my_country!=0;
  SQGServerList::m_ping1 = g_pGameCVars->g_quickGame_ping1_level;
  SQGServerList::m_ping2 = g_pGameCVars->g_quickGame_ping2_level;

  m_stage = 0;
  m_searching = true;
  NextStage();
}

void CQuickGame::Cancel()
{
  if(m_searching)
    m_searching = false;
  if(m_browser)
    m_browser->Stop();
  if(m_ui.get())
    m_ui->Close();
  m_stage = 0;
}

void CQuickGame::NextStage()
{
  m_stage++;
  switch(m_stage)
  {
  case 1://Init and LAN check
    {
      INetworkService* serv = GetISystem()->GetINetwork()->GetService("GameSpy");
      if(serv)
      {
        //
        m_browser = serv->GetServerBrowser();
        m_browser->SetListener(m_list.get());
        m_list->Reset();
        m_browser->Start(true);
        m_browser->Update();
      }
    }
    break;
  case 2://check internet
    m_list->Reset();
    m_browser->Start(false);
    m_browser->Update();
    break;
  case 3:
    m_browser->SetListener(0);
    m_browser = 0;
    m_stage = 0;
    m_searching = false;
    if(m_ui.get())
      m_ui->OnFinished();
    break;
  }
}

int CQuickGame::GetStage()const
{
  return m_stage;
}

bool CQuickGame::IsSearching()const
{
  return m_searching;

}