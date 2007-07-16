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

#ifndef __GAMENETWORKPROFILE_H__
#define __GAMENETWORKPROFILE_H__

#pragma once

#include "INetworkService.h"

class CMPHub;

enum EChatUserType
{
  eCUT_buddy,
  eCUT_general,
  eCUT_ignore
};


struct SChatUser
{
  int         m_id;
  string      m_nick;
  EUserStatus m_status;
  string      m_location;
};
struct SUserInfo
{
  string  m_nick; 
  string  m_country;
  int     m_played;
  int     m_kills;
  int     m_deaths;
  int     m_accuracy;
  int     m_suitmode;
  int     m_favoriteWeapon;
  int     m_favoriteMap;
};

struct INProfileUI
{
  virtual void AddBuddy(SChatUser s)=0;
  virtual void UpdateBuddy(SChatUser s)=0;
  virtual void RemoveBuddy(int id)=0;
  virtual void AddIgnore(SChatUser s)=0;
  virtual void RemoveIgnore(int id)=0;
  virtual void ProfileInfo(int id, SUserInfo ui)=0;
  virtual void SearchResult(int id, const char* nick)=0;
  virtual void OnMessage(int id, const char* message)=0;
  virtual void SearchCompleted()=0;
  virtual void ShowError()=0;
  virtual void OnAuthRequest(int id, const char* nick, const char* message)=0;
  virtual void AddFavoriteServer(uint ip, ushort port)=0;
  virtual void AddRecentServer(uint ip, ushort port)=0;
};

struct SStoredServer
{
  SStoredServer():ip(0),port(0),id(-1){}
  uint    ip;
  ushort  port;
  int     id;//id in stored list
};

class CGameNetworkProfile
{
private:
  typedef std::map<int,SUserInfo> TUserInfoMap;

  struct SBuddies;
  struct SChat;
  struct SChatText;
  struct SStoredServerLists;
  struct SStorageQuery
  {
    SStorageQuery(CGameNetworkProfile* p):m_parent(p)
    {
      m_parent->m_queries.push_back(this);
    }
    virtual ~SStorageQuery()
    {
      if(m_parent)
        stl::find_and_erase(m_parent->m_queries,this);
    }
    CGameNetworkProfile *m_parent;
  };

  typedef std::map<int, SChatText> TChatTextMap;
public:
  CGameNetworkProfile(CMPHub* hub);
  ~CGameNetworkProfile();

  void Login(const char* login, const char* password);
  void Register(const char* login, const char* password, const char* email);
  void Logoff();
  
  void AcceptBuddy(int id, bool accept);
  void RequestBuddy(const char* nick, const char* reason);
  void RequestBuddy(int id, const char* reason);
  void RemoveBuddy(const char* nick);
  void AddIgnore(const char* nick);
  void StopIgnore(const char* nick);
  void GetUserInfo(const char* nick);
  void GetUserInfo(int id);
  void SendBuddyMessage(int id, const char* message);
  void SearchUsers(const char* nick);
  bool CanInvite(const char* nick);
  bool CanIgnore(const char* nick);
  bool CanInvite(int id);
  bool CanIgnore(int id);

  void SetPlayingStatus(uint ip, ushort port, ushort publicport, const char* game_type);
  void SetChattingStatus();
  
  void InitUI(INProfileUI* a);
  void DestroyUI();

  void AddFavoriteServer(uint ip, ushort port);
  void RemoveFavoriteServer(uint ip, ushort port);
  void AddRecentServer(uint ip, ushort port);

  bool IsLoggedIn()const;
  
private:
  void OnLoggedIn(int id, const char* nick);
  void CleanUpQueries();

private:
  CMPHub*                 m_hub;
  INetworkProfile*        m_profile;
  string                  m_login;
  int                     m_profileId;

  std::auto_ptr<SBuddies>           m_buddies;
  std::auto_ptr<SStoredServerLists> m_stroredServers;
  std::vector<SStorageQuery*>       m_queries;
};

#endif //__GAMENETWORKPROFILE_H__
