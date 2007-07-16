/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Game-side part of Palyer's network profile

-------------------------------------------------------------------------
History:
- 03/2007: Created by Stas Spivakov

*************************************************************************/
#include "StdAfx.h"
#include "INetwork.h"
#include "GameNetworkProfile.h"
#include "MPHub.h"

#include "Game.h"
#include "GameCVars.h"
#include "HUD/HUD.h"
#include "HUD/HUDRadar.h"
#include "HUD/HUDTextChat.h"


enum EUserInfoKey
{
  eUIK_none,
  eUIK_nick,
  eUIK_country,
  eUIK_played,
  eUIK_kills,
  eUIK_deaths,
  eUIK_accuracy,
  eUIK_suitMode,
  eUIK_favouriteWep,
  eUIK_favouriteMap
};

static TKeyValuePair<EUserInfoKey,const char*>
gUserInfoKeys[] = {
  {eUIK_none,""},
  {eUIK_nick,"nick"},
  {eUIK_country,"country"},
  {eUIK_played,"played"},
  {eUIK_kills,"kills"},
  {eUIK_deaths,"deaths"},
  {eUIK_accuracy,"accuracy"},
  {eUIK_suitMode,"suitMode"},
  {eUIK_favouriteWep,"fav_weapon"},
  {eUIK_favouriteMap,"fav_map"}
};

struct CGameNetworkProfile::SChatText
{
  struct SChatLine 
  {
    string  text;
    int     id;
  };
  SChatText():m_viewed(false),m_size(1000)
  {}
  void AddText(const char* str, int text_id)
  {
    m_viewed = false;
    m_text.push_back(SChatLine());
    m_text.back().text = str;
    m_text.back().id = text_id;
    while(m_size && m_text.size()>m_size)
    {
      int id = m_text.front().id;
      do
      {
        m_text.pop_front();
      }while(m_text.front().id == id);
    }
  }
  int                     m_size;
  bool                    m_viewed;
  std::list<SChatLine>    m_text;
  std::vector<SChatUser>  m_chatList;
};

struct CGameNetworkProfile::SBuddies  : public INetworkProfileListener
{
  enum EQueryReason
  {
    eQR_addBuddy,
    eQR_showInfo,
    eQR_addIgnore,
    eQR_stopIgnore,
    eQR_getStats,
    eQR_buddyRequest,
  };

  struct SPendingOperation
  {
    SPendingOperation(){}
    SPendingOperation(EQueryReason t):m_id(0),m_type(t)
    {}
    string        m_nick;
    int           m_id;
    string        m_param;
    EQueryReason  m_type;
  };


  struct SBuddyRequest
  {
    string nick;
    string message;
  };

  struct SIgnoredProfile
  {
    int recordId;
    int id;
    string nick;
  };

  typedef std::map<int, SBuddyRequest> TBuddyRequestsMap;

  class CIgnoreListReader : public SStorageQuery, public IStatsReader
  {
  public:
    CIgnoreListReader(CGameNetworkProfile* parent):
    SStorageQuery(parent)
    {
      m_ignore.id = -1;
      m_ignore.recordId = -1;
    }

    //input
    virtual const char* GetTableName()
    {
      return "ignorelist";
    }

    virtual int         GetFieldsNum()
    {
      return 2;
    }

    virtual const char* GetFieldName(int i)
    {
      if(i==0)
        return "profile";
      else
        return "nick";
    }
    //output
    virtual void        NextRecord(int id)
    {
      AddIgnore();
      m_ignore.recordId = id;
    }

    virtual void        OnValue(int field, const char* val)
    {
      assert(field == 1);
      m_ignore.nick = val;
    }

    virtual void        OnValue(int field, int val)
    {
      assert(field == 0);
      m_ignore.id = val;
    }

    virtual void        OnValue(int field, float val)
    {
      assert(false);    
    }

    virtual void        End(bool error)
    {
      if(!error)
      {
        AddIgnore();
      }
      //end
      delete this;
    }

    void AddIgnore()
    {
      if(m_ignore.recordId == -1 || !m_parent)
        return;
      m_parent->m_buddies->AddIgnore(m_ignore.id,m_ignore.nick);
    }

    SIgnoredProfile m_ignore;
  };

  class CIgnoreListWriter : public SStorageQuery, public IStatsWriter
  {
  public:
    CIgnoreListWriter(CGameNetworkProfile* parent):
    SStorageQuery(parent),
    m_idx(-1),
    m_results(0)
    {}

    virtual const char* GetTableName()
    {
      return "ignoredlist";
    }

    virtual int GetFieldsNum()
    {
      return 2;
    }

    virtual const char* GetFieldName(int i)
    {
      if(i==0)
        return "profile";
      else
        return "nick";
    }

    //output
    virtual int   GetRecordsNum()
    {
      return m_parent->m_buddies->m_ignoreList.size();
    }
    virtual int   NextRecord()
    {
      m_idx++;
      return m_parent->m_buddies->m_ignoreList[m_idx].recordId;
    }

    virtual bool  GetValue(int field, const char*& val)
    {
      if(field == 1)
      {
        if(m_parent->m_buddies->m_ignoreList[m_idx].nick.empty())
          val = "";
        else
          val = m_parent->m_buddies->m_ignoreList[m_idx].nick;
      }
      else
        return false;
      return true;
    }

    virtual bool  GetValue(int field, int& val)
    {
      if(field == 0)
        val = m_parent->m_buddies->m_ignoreList[m_idx].id;
      else
        return false;
      return true;
    }
    virtual bool  GetValue(int field, float& val){return false;}
    //output
    virtual void  OnResult(int idx, int id, bool success)
    {
      if(success && m_parent)
      {
          m_parent->m_buddies->m_ignoreList[idx].recordId = id;
      }

      m_results++;
      if(m_results == GetRecordsNum())
      {
        delete this;
      }
    }

    int               m_idx;
    int               m_results;
  };

  SBuddies(CGameNetworkProfile* p):m_parent(p),m_textId(0),m_ui(0)
  {}

  void AddNick(const char* nick){}

  void UpdateFriend(int id, const char* nick, EUserStatus s, const char* location)
  {
    SChatUser *u = 0;
    for(int i=0;i<m_buddyList.size();++i)
    {
      if(m_buddyList[i].m_id == id)
      {
        u = &m_buddyList[i];
        break;
      }
    }
    bool added = false;
    if(!u)
    {
      m_buddyList.push_back(SChatUser());
      u = &m_buddyList.back();
      added = true;
    }
    u->m_nick = nick;
    u->m_status = s;
    u->m_id = id;
    u->m_location = location;
    //updated user with id
    if(m_ui)
    {
      if(added)
        m_ui->AddBuddy(*u);
      else
        m_ui->UpdateBuddy(*u);
    }
  }

  void RemoveFriend(int id)
  {
    for(int i=0;i<m_buddyList.size();++i)
    {
      if(m_buddyList[i].m_id == id)
      {
        //remove
        if(m_ui)
          m_ui->RemoveBuddy(id);
        m_buddyList.erase(m_buddyList.begin()+i);
        break;
      }
    }
  }

  void AddIgnore(int id, const char* nick)
  {
    bool found = false;
    for(int i=0;i<m_ignoreList.size();++i)
    {
      if(m_ignoreList[i].id == -1)
      {
        m_ignoreList[i].id = id;
        m_ignoreList[i].nick = nick;
        found = true;
        break;
      }
    }

    if(!found)
    {
      SIgnoredProfile ip;
      ip.recordId=-1;
      ip.id = id;
      ip.nick = nick;
      m_ignoreList.push_back(ip);
    }

    SaveIgnoreList();
    if(m_ui)
    {
      SChatUser u;
      u.m_id = id;
      u.m_nick = nick;
      m_ui->AddIgnore(u);
    }
  }

  void RemoveIgnore(int id)
  {
    for(int i=0;i<m_ignoreList.size();++i)
    {
      if(m_ignoreList[i].id == id)
      {
        m_ignoreList[i].id = -1;
        m_ignoreList[i].nick="";
        if(m_ui)
          m_ui->RemoveIgnore(id);
        SaveIgnoreList();
        break;
      }
    }
  }

  void OnFriendRequest(int id, const char* message)
  {
    if(IsIgnoring(id))
      m_parent->m_profile->AuthFriend(id,false);
    SBuddyRequest br;
    br.message = message;
    
    TUserInfoMap::iterator it = m_infoCache.find(id);
    if(it!=m_infoCache.end())
    {
      br.nick = it->second.m_nick;
      if(m_ui)
        m_ui->OnAuthRequest(id,br.nick,message);
    }
    else
    {
      m_parent->m_profile->GetProfileInfo(id);
    }

    m_requests.insert(std::make_pair(id,br));
  }

  void OnMessage(int id, const char* message)
  {
    if(IsIgnoring(id))
      return;
      
    TChatTextMap::iterator it = m_buddyChats.find(id);
    if(it==m_buddyChats.end())
      it = m_buddyChats.insert(std::make_pair(id,SChatText())).first;
    
    m_textId++;
    it->second.AddText(message,m_textId);
    if(m_ui)
      m_ui->OnMessage(id,message);

    if(m_parent->m_hub->IsIngame() && g_pGame->GetCVars()->g_buddyMessagesIngame)
    {
      if(CHUDTextChat *pChat = g_pGame->GetHUD()->GetMPChat())
      {
        const char* name = "";
        for(int i=0;i<m_buddyList.size();++i)
        {
          if(m_buddyList[i].m_id == id)
          {
            name = m_buddyList[i].m_nick.c_str();
            break;
          }
        }
        pChat->AddChatMessage(string().Format("From [%s] :",name), message, 0);
      }
    }
  }

  void LoginResult(ENetworkProfileError res, const char* descr, int id, const char* nick)
  {
    if(res == eNPE_ok)
    {
      m_parent->OnLoggedIn(id, nick);
    }
    else
    {
      m_parent->m_hub->OnLoginFailed(descr);
    }
  }

  void OnError(ENetworkProfileError res, const char* descr)
  {
    if(m_ui)
      m_ui->ShowError();
  }

  void OnProfileInfo(int id, const char* key, const char* value)
  {
    TUserInfoMap::iterator it = m_infoCache.find(id);
    if(it == m_infoCache.end())
    {
      it = m_infoCache.insert(std::make_pair(id,SUserInfo())).first;
    }
    SUserInfo &u = it->second;
    EUserInfoKey k = KEY_BY_VALUE(string(key),gUserInfoKeys);
    switch(k)
    {
    case eUIK_nick:
      u.m_nick = value;
      break;
    case eUIK_country:
      u.m_country = value;
      break;
    case eUIK_played:
      u.m_played = atoi(value);
      break;
    case eUIK_kills:
      u.m_kills = atoi(value);
      break;
    case eUIK_deaths:
      u.m_deaths = atoi(value);
      break;
    case eUIK_accuracy:
      u.m_accuracy = atoi(value);
      break;
    case eUIK_suitMode:
      u.m_suitmode = atoi(value);
      break;
    case eUIK_favouriteWep:
      u.m_favoriteWeapon = atoi(value);
      break;
    case eUIK_favouriteMap:
      u.m_favoriteMap = atoi(value);
      break;
    default:;
    }
  }

  virtual void OnProfileComplete(int id)
  {
    TUserInfoMap::iterator iit = m_infoCache.find(id);
    if(iit == m_infoCache.end())
      return;
    if(m_ui)
      m_ui->ProfileInfo(id,iit->second);
  }

  virtual void OnSearchResult(int id, const char* nick)
  {
    if(m_ui)
      m_ui->SearchResult(id,nick);
  }

  virtual void OnSearchComplete()
  {
    if(m_ui && !m_uisearch.empty())
    {
      m_uisearch.resize(0);
      m_ui->SearchCompleted();
    }
  }

  virtual void OnUserId(const char* nick, int id)
  {
    for(int i=0;i<m_operations.size();++i)
    {
      if(m_operations[i].m_nick == nick)
      {
        m_operations[i].m_id = id;
        ExecuteOperation(i);
        break;
      }
    }
  }
  
  virtual void OnUserNick(int id, const char* nick)
  {
    for(int i=0;i<m_operations.size();++i)
    {
      if(m_operations[i].m_id == id && m_operations[i].m_nick.empty())
      {
        m_operations[i].m_nick = nick;
        ExecuteOperation(i);
        break;
      }
    }
  }

  void ExecuteOperation(int idx)
  {
    assert(0 <= idx && idx < m_operations.size());
    SPendingOperation &o = m_operations[idx];
    assert(!o.m_nick.empty() && o.m_id);

    switch(o.m_type)
    {
    case eQR_addBuddy:
      m_parent->m_profile->AddFriend(o.m_id,o.m_param);
      break;
    case eQR_showInfo:
      m_parent->m_profile->GetProfileInfo(o.m_id);
      break;
    case eQR_addIgnore:
      AddIgnore(o.m_id,o.m_nick);
      break;
    case eQR_stopIgnore:
      RemoveIgnore(o.m_id);
      break;
    case eQR_getStats:
      m_parent->m_profile->GetProfileInfo(o.m_id);
      break;
    case eQR_buddyRequest:
      if(m_ui)
        m_ui->OnAuthRequest(o.m_id,o.m_nick,o.m_param);
      break;
    }
    m_operations.erase(m_operations.begin()+idx);
  }

  void Accept(int id, bool accept)
  {
    TBuddyRequestsMap::iterator it = m_requests.find(id);
    if(it!= m_requests.end())  
    {
      m_parent->m_profile->AuthFriend(it->first,accept);
      m_requests.erase(it);
    }
  }

  void Request(const char* nick, const char* reason)
  {
    SPendingOperation op(eQR_addBuddy);
    op.m_nick = nick;
    op.m_param = reason;
    m_operations.push_back(op);
    if(!CheckNick(nick))
      m_parent->m_profile->GetUserId(nick);
  }

  void Request(int id, const char* reason)
  {
    m_parent->m_profile->AddFriend(id,reason);
  }

  bool CheckNick(const char* nick)
  {
    for(TUserInfoMap::iterator it = m_infoCache.begin();it!=m_infoCache.end();++it)
    {
      if(it->second.m_nick == nick)
      {
        OnUserId(nick,it->first);
        return true;
      }
    }
    return false;
  }

  bool CheckId(int id)
  {
    TUserInfoMap::iterator it = m_infoCache.find(id);
    if(it!=m_infoCache.end())
    {
      OnUserNick(id,it->second.m_nick);
      return true;
    }
    return false;
  }

  void Remove(const char* nick)
  {
    for(int i=0;i<m_buddyList.size();++i)
    {
      if(m_buddyList[i].m_nick == nick)
      {
        m_parent->m_profile->RemoveFriend(m_buddyList[i].m_id,false);
        break;
      }
    }
  }

  void Ignore(const char* nick)
  {
    Remove(nick);

    SPendingOperation op(eQR_addIgnore);
    op.m_nick = nick;
    m_operations.push_back(op);
    if(!CheckNick(nick))
      m_parent->m_profile->GetUserId(nick);
  }
  
  void GetUserInfo(const char* nick)
  {
    SPendingOperation op(eQR_getStats);
    op.m_nick = nick;
    m_operations.push_back(op);
    if(!CheckNick(nick))
      m_parent->m_profile->GetUserId(nick);
  }

  void GetUserInfo(int id)
  {
    SPendingOperation op(eQR_getStats);
    op.m_id = id;
    m_operations.push_back(op);
    if(!CheckId(id))
      m_parent->m_profile->GetUserNick(id);
  }

  void StopIgnore(const char* nick)
  {
    SPendingOperation op(eQR_stopIgnore);
    op.m_nick = nick;
    m_operations.push_back(op);
    if(!CheckNick(nick))
      m_parent->m_profile->GetUserId(nick);
  }

  bool IsIgnoring(int id)
  {
    for(int i=0;i<m_ignoreList.size();++i)
    {
      if(m_ignoreList[i].id == id)
        return true;
    }
    return false;
  }

  bool IsIgnoring(const char* nick)
  {
    for(int i=0;i<m_ignoreList.size();++i)
    {
      if(m_ignoreList[i].nick == nick)
        return true;
    }
    return false;
  }

  void SendUserMessage(int id, const char* message)
  {
    m_parent->m_profile->SendFriendMessage(id,message);
  }

  void ReadIgnoreList()
  {
    m_parent->m_profile->ReadStats(new CIgnoreListReader(m_parent));
  }

  void SaveIgnoreList()
  {
    m_parent->m_profile->WriteStats(new CIgnoreListWriter(m_parent));
  }

  bool CanInvite(const char* nick)
  {
    if(!strcmp(nick,"ChatMonitor"))
      return false;
    for(TUserInfoMap::iterator it = m_infoCache.begin();it!=m_infoCache.end();++it)
    {
      if(it->second.m_nick == nick)
      {
        return CanInvite(it->first);
      }
    }
    return true;
  }

  bool CanInvite(int id)
  {
    for(int i=0;i<m_buddyList.size();++i)
      if(m_buddyList[i].m_id == id)
        return false;

    return true;
  }

  bool CanIgnore(const char* nick)
  {
    for(int i=0;i<m_ignoreList.size();++i)
    {
      if(m_ignoreList[i].nick == nick)
      {
        return false;
      }
    }
    return true;
  }

  bool CanIgnore(int id)
  {
    for(int i=0;i<m_ignoreList.size();++i)
    {
      if(m_ignoreList[i].id == id)
      {
        return false;
      }
    }
    return true;
  }

  void UIActivated(INProfileUI* ui)
  {
    m_ui = ui;
    if(!m_ui)
      return;

    for(int i=0;i<m_buddyList.size();++i)
    {
      m_ui->AddBuddy(m_buddyList[i]);
    }
    for(int i=0;i<m_ignoreList.size();++i)
    {
      SChatUser u;
      u.m_id = m_ignoreList[i].id;
      u.m_nick = m_ignoreList[i].nick;
      m_ui->AddIgnore(u);
    }
  }

  std::vector<SChatUser>        m_buddyList;
  std::vector<SIgnoredProfile>  m_ignoreList;
  TUserInfoMap                  m_infoCache;

  TBuddyRequestsMap       m_requests;

  TChatTextMap              m_buddyChats;
  std::auto_ptr<SChatText>  m_globalChat;

  std::vector<SPendingOperation>  m_operations;

  int                   m_textId;
  CGameNetworkProfile*  m_parent;
  INProfileUI*          m_ui;

  string                m_uisearch;
};

struct CGameNetworkProfile::SStoredServerLists
{
  struct SServerListReader : public SStorageQuery, public IStatsReader
  {
    SServerListReader(CGameNetworkProfile* parent, bool rec):
    SStorageQuery(parent),
    m_recent(rec)
    {
      m_server.id = -1;
    }
  
    //input
    virtual const char* GetTableName()
    {
      return m_recent?"recent":"favorites";
    }

    virtual int         GetFieldsNum()
    {
      return 2;
    }

    virtual const char* GetFieldName(int i)
    {
      if(i==0)
        return "ip";
      else
        return "port";
    }
    //output
    virtual void        NextRecord(int id)
    {
      AddServer();
      m_server.id = id;
    }

    virtual void        OnValue(int field, const char* val)
    {
      assert(false);
    }

    virtual void        OnValue(int field, int val)
    {
      if(field == 0)
        m_server.ip = val;
      else
        m_server.port = val;
    }

    virtual void        OnValue(int field, float val)
    {
      assert(false);    
    }

    virtual void        End(bool error)
    {
      if(!error)
      {
        AddServer();
      }
      delete this;
    }

    void AddServer()
    {
      if(m_server.id ==-1 || !m_parent)
        return;
      if(m_recent)
        m_parent->m_stroredServers->m_recent.push_back(m_server);
      else
        m_parent->m_stroredServers->m_favorites.push_back(m_server);
    }

    SStoredServer     m_server;
    bool m_recent;
  };

  struct SServerListWriter : public SStorageQuery, public IStatsWriter 
  {
    SServerListWriter(CGameNetworkProfile* parent, bool rec):
    SStorageQuery(parent),
    m_recent(rec),
    m_idx(-1),
    m_results(0)
    {
    }

    //input
    virtual const char* GetTableName()
    {
      return m_recent?"recent":"favorites";
    }

    virtual int GetFieldsNum()
    {
      return 2;
    }

    virtual const char* GetFieldName(int i)
    {
      if(i==0)
        return "ip";
      else
        return "port";
    }

    //output
    virtual int   GetRecordsNum()
    {
      if(m_recent)
        return m_parent->m_stroredServers->m_recent.size();
      else
        return m_parent->m_stroredServers->m_favorites.size();
    }
    virtual int   NextRecord()
    {
      m_idx++;
      return GetServer().id;
    }
    virtual bool  GetValue(int field, const char*& val){return false;}
    virtual bool  GetValue(int field, int& val)
    {
      if(field == 0)
        val = GetServer().ip;
      else if(field == 1)
        val = GetServer().port;
      else
        return false;
      return true;
    }
    virtual bool  GetValue(int field, float& val){return false;}
    //output
    virtual void  OnResult(int idx, int id, bool success)
    {
      if(success && m_parent)
      {
        if(m_recent)
          m_parent->m_stroredServers->m_recent[idx].id = id;
        else
          m_parent->m_stroredServers->m_favorites[idx].id = id;
      }

      m_results++;
      if(m_results == GetRecordsNum())
      {
        delete this;
      }
    }

    const SStoredServer& GetServer()
    {
      assert(m_parent);
      if(m_recent)
        return m_parent->m_stroredServers->m_recent[m_idx];
      else
        return m_parent->m_stroredServers->m_favorites[m_idx];
    }

    int               m_idx;
    bool              m_recent;
    int               m_results;
  };

  SStoredServerLists(CGameNetworkProfile* p):
  m_parent(p)
  {

  }

  ~SStoredServerLists()
  {
  }

  void ReadLists()
  {
    m_parent->m_profile->ReadStats(new SServerListReader(m_parent,false));
    m_parent->m_profile->ReadStats(new SServerListReader(m_parent,true));    
  }

  void UIActivated(INProfileUI* ui)
  {
    m_ui = ui;
    if(!m_ui)
      return;
    for(int i=0;i<m_favorites.size();++i)
      m_ui->AddFavoriteServer(m_favorites[i].ip,m_favorites[i].port);
    for(int i=0;i<m_recent.size();++i)
      m_ui->AddRecentServer(m_recent[i].ip,m_recent[i].port);
  }

  void AddFavoriteServer(uint ip, ushort port)
  {
    SStoredServer s;
    s.id = -1;
    s.ip = ip;
    s.port = port;
    m_favorites.push_back(s);
  }

  void RemoveFavoriteServer(uint ip, ushort port)
  {
    for(int i=0;i<m_favorites.size();++i)
    {
      if(m_favorites[i].ip==ip && m_favorites[i].port==port)
      {
        m_favorites.erase(m_favorites.begin()+i);
        break;
      }
    }
  }

  void AddRecentServer(uint ip, ushort port)
  {
    SStoredServer s;
    s.id = -1;
    s.ip = ip;
    s.port = port;
    m_recent.push_back(s);
  }

  std::vector<SStoredServer>      m_favorites;
  std::vector<SStoredServer>      m_recent;
  INProfileUI*                    m_ui;

  CGameNetworkProfile*            m_parent;
};

CGameNetworkProfile::CGameNetworkProfile(CMPHub* hub):
m_hub(hub)
{
  INetworkService *serv = gEnv->pNetwork->GetService("GameSpy");
  if(serv)
  {
    m_profile = serv->GetNetworkProfile();
  }
  else
  {
    m_profile = 0;
  }
}

CGameNetworkProfile::~CGameNetworkProfile()
{
  if(m_profile)
     m_profile->RemoveListener(m_buddies.get());
}

void CGameNetworkProfile::Login(const char* login, const char* password)
{
  m_login = login;
  m_buddies.reset(new SBuddies(this));
  m_stroredServers.reset(new SStoredServerLists(this));
  m_profile->AddListener(m_buddies.get());
  m_profile->Login(login,password);
}

void CGameNetworkProfile::Register(const char* login, const char* password, const char* email)
{
  assert(m_profile);
  m_login = login;
  m_buddies.reset(new SBuddies(this));
  m_stroredServers.reset(new SStoredServerLists(this));
  m_profile->AddListener(m_buddies.get());
  m_profile->Register(login,password,email);
}

void CGameNetworkProfile::Logoff()
{
  assert(m_profile);

  //cancel pending storage queries..
  //TODO: wait for important queries to finish.
  CleanUpQueries();

  m_profile->Logoff();
  m_profile->RemoveListener(m_buddies.get());
  m_buddies.reset(0);
  m_stroredServers.reset(0);
}

void CGameNetworkProfile::AcceptBuddy(int id, bool accept)
{
  m_buddies->Accept(id,accept);
}

void CGameNetworkProfile::RequestBuddy(const char* nick, const char* reason)
{
  m_buddies->Request(nick, reason);
}

void CGameNetworkProfile::RequestBuddy(int id, const char* reason)
{
  m_buddies->Request(id,reason);
}

void CGameNetworkProfile::RemoveBuddy(const char* nick)
{
  m_buddies->Remove(nick);
}

void CGameNetworkProfile::AddIgnore(const char* nick)
{
  m_buddies->Ignore(nick);
}

void CGameNetworkProfile::StopIgnore(const char* nick)
{
  m_buddies->StopIgnore(nick);
}

void CGameNetworkProfile::GetUserInfo(const char* nick)
{
  m_buddies->GetUserInfo(nick);
}

void CGameNetworkProfile::GetUserInfo(int id)
{
  m_buddies->GetUserInfo(id);
}

void CGameNetworkProfile::SendBuddyMessage(int id, const char* message)
{
  m_buddies->SendUserMessage(id,message);
}

void CGameNetworkProfile::InitUI(INProfileUI* a)
{
  if(m_buddies.get())
    m_buddies->UIActivated(a);
  if(m_stroredServers.get())
    m_stroredServers->UIActivated(a);
}

void CGameNetworkProfile::DestroyUI()
{
  if(m_buddies.get())
    m_buddies->m_ui = 0;
}

void CGameNetworkProfile::AddFavoriteServer(uint ip, ushort port)
{
  m_stroredServers->AddFavoriteServer(ip,port);
}

void CGameNetworkProfile::RemoveFavoriteServer(uint ip, ushort port)
{
  m_stroredServers->RemoveFavoriteServer(ip,port);
}

void CGameNetworkProfile::AddRecentServer(uint ip, ushort port)
{
  m_stroredServers->AddRecentServer(ip,port);
}

bool CGameNetworkProfile::IsLoggedIn()const
{
  return m_profile->IsLoggedIn();
}

void CGameNetworkProfile::SearchUsers(const char* nick)
{
  m_buddies->m_uisearch=nick;
  m_profile->SearchFriends(nick);
}

bool CGameNetworkProfile::CanInvite(const char* nick)
{
  if(!stricmp(m_login.c_str(),nick))
    return false;
  return m_buddies->CanInvite(nick);
}

bool CGameNetworkProfile::CanIgnore(const char* nick)
{
  if(!stricmp(m_login.c_str(),nick))
    return false;
  return m_buddies->CanIgnore(nick);
}

bool CGameNetworkProfile::CanInvite(int id)
{
  if(m_profileId == id)
    return false;
  return m_buddies->CanInvite(id);
}

bool CGameNetworkProfile::CanIgnore(int id)
{
  if(m_profileId == id)
    return false;
  return m_buddies->CanIgnore(id);
}

void CGameNetworkProfile::SetPlayingStatus(uint ip, ushort port, ushort publicport, const char* game_type)
{
  string loc;
  loc.Format("%d.%d.%d.%d:%d/?type=game&queryport=%d&game=%s",ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF,port,publicport,game_type);
  m_profile->SetStatus(eUS_playing,loc);
}

void CGameNetworkProfile::SetChattingStatus()
{
  m_profile->SetStatus(eUS_chatting,"/?chatting");
}

void CGameNetworkProfile::OnLoggedIn(int id, const char* nick)
{
  m_profileId = id;
  m_login = nick;

  m_stroredServers->ReadLists();
  m_hub->OnLoginSuccess(nick);
  m_profile->SetStatus(eUS_online,"");
}

void CGameNetworkProfile::CleanUpQueries()
{
  for(int i=0;i<m_queries.size();++i)
    m_queries[i]->m_parent = 0;
}