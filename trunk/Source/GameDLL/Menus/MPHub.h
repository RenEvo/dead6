#ifndef __MPHUB_H__
#define __MPHUB_H__

#pragma once

#include "INetwork.h"

template<class T, class U>
struct TKeyValuePair
{
  T key;
  U value;
};

template<class T, class U, class V>
T GetKeyByValue(V value, TKeyValuePair<T,U> list[], size_t list_size)
{
  for(int i=0;i<list_size;++i)
    if(value == list[i].value)
      return list[i].key;
  return list[0].key;
}

template<class T, class U, class V>
U GetValueByKey(V key, TKeyValuePair<T,U> list[], size_t list_size)
{
  for(int i=0;i<list_size;++i)
    if(key == list[i].key)
      return list[i].value;
  return list[0].value;
}

//first element of array is default values
#define KEY_BY_VALUE(v,g) GetKeyByValue(v,g,sizeof(g)/sizeof(g[0]))
#define VALUE_BY_KEY(v,g) GetValueByKey(v,g,sizeof(g)/sizeof(g[0]))

// commands coming from UI to logic
enum EGsUiCommand
{
  eGUC_none,
  eGUC_opened,
  eGUC_back,
  eGUC_cancel,
  eGUC_internetGame,
  eGUC_recordedGames,
  eGUC_login,
  eGUC_logoff,
  eGUC_rememberPassword,
  eGUC_forgotPassword,
  eGUC_autoLogin,
  eGUC_enterlobby,
  eGUC_leavelobby,
  eGUC_enterLANlobby,
  eGUC_leaveLANlobby,
  eGUC_update,
  eGUC_stop,
  eGUC_setVisibleServers,
  eGUC_displayServerList,
  eGUC_serverScrollBarPos,
  eGUC_serverScroll,
  eGUC_selectServer,
  eGUC_refreshServer,
  eGUC_addFavorite,
  eGUC_removeFavorite,
  eGUC_sortColumn,
  eGUC_join,
  eGUC_joinIP,
  eGUC_joinPassword,
  eGUC_disconnect,
  eGUC_tab,
  eGUC_chatClick,
  eGUC_chatOpen,
  eGUC_chat,
  eGUC_find,
  eGUC_addBuddy,
  eGUC_addIgnore,
  eGUC_addBuddyFromInfo,
  eGUC_addIgnoreFromInfo,
  eGUC_inviteBuddy,
  eGUC_removeBuddy,
  eGUC_stopIgnore,
  eGUC_acceptBuddy,
  eGUC_declineBuddy,
  eGUC_displayInfo,
  eGUC_displayInfoInList,
  eGUC_joinBuddy,
  eGUC_userScrollBarPos,
  eGUC_userScroll,
  eGUC_chatScrollBarPos,
  eGUC_chatScroll,
  eGUC_register,
  eGUC_registerNick,
  eGUC_registerEmail,
  eGUC_registerDateMM,
  eGUC_registerDateDD,
  eGUC_registerDateYY,
  eGUC_registerParentEmail,
  eGUC_registerEnd,
  eGUC_quickGame,
  eGUC_createServerStart,
  eGUC_createServerUpdateLevels,
	eGUC_createServerOpened,
  eGUC_createServerParams,
  eGUC_dialogClosed,
  eGUC_dialogYes,
  eGUC_dialogNo,

	eGUC_filtersDisplay,
  eGUC_filtersEnable,
  eGUC_filtersMode,
  eGUC_filtersMap,
  eGUC_filtersPing,
  eGUC_filtersNotFull,
  eGUC_filtersNotEmpty,
  eGUC_filtersNoPassword,
  eGUC_filtersAutoTeamBalance,
  eGUC_filtersAntiCheat,
  eGUC_filtersFriendlyFire,
  eGUC_filtersGamepadsOnly,
  eGUC_filtersNoVoiceComms,
  eGUC_filtersDedicated,
  eGUC_filtersDX10,

};

//events coming from logic to UI
enum EUIEvent
{
  eUIE_none,
  eUIE_destroy,//when we are destroying, will be closed afterwards
  eUIE_connectFailed,//connecting failed
  eUIE_disconnnect,

  eUIE_login,
  eUIE_quickGame,
  eUIE_connect,
};

//inherit this for UI event params to make purpose obvious
struct SUIEventParam
{};

struct SUIEvent
{
  SUIEvent():event(eUIE_none),param(0),descrpition(0),data(0){}
  SUIEvent(EUIEvent e, int p = 0, const char* s = 0, const SUIEventParam *d = 0):event(e),param(p),descrpition(s),data(d){}
  EUIEvent        event;
  int             param;
  const char*     descrpition;
  const SUIEventParam*  data;
};

class   CMultiPlayerMenu;
class   CQuickGame;
class   CGameNetworkProfile;

//class handles all MP UI things
class CMPHub
{
private:
  struct SRegisterInfo
  {
    string nick;
    string password;
    string email;
    int    month;
    int    day;
    int    year;
  };
  struct SMPOptions
  {
    SMPOptions():remeber(0),autologin(0){}
    string  login;
    string  password;
    int    remeber;
    int    autologin;
  };
public:
  class CDialog
  {
  public:
    CDialog();
    virtual ~CDialog();
    void Show(CMPHub* hub);
    void Close();
    virtual bool OnCommand(EGsUiCommand cmd, const char* pArgs);
    virtual void OnUIEvent(const SUIEvent& event);
    virtual void OnClose();
    virtual void OnShow();
  protected:
    CMPHub*     m_hub;
  };
public:
  CMPHub();
  ~CMPHub();
  bool HandleFSCommand(const char* pCmd, const char* pArgs);//Flash does it
  void OnUIEvent(const SUIEvent& event);//game does it
  void SetCurrentFlashScreen(IFlashPlayer* current_screen, bool ingame);
  void ConnectFailed(EConnectionFailureCause cause, const char * description);
  void OnLoginSuccess(const char* nick);
  void OnLoginFailed(const char* reason);
  void ShowLoginDlg();
  void CloseLoginDlg();
  
  void ShowLoadingDlg(const char* message);//will be localized
  void SetLoadingDlgText(const char* text, bool localize);
  void CloseLoadingDlg();

  void OnQuickGame();
  
  void SwitchToLobby();
  void SetLoginInfo(const char* nick);
  void ShowError(const char* msg);
  void DoLogin(const char* nick, const char* pwd);
  void DoLogoff();
  void AddGameModToList(const char* mod);
  void SwitchToMainScreen();
  void ReadOptions();
  void SaveOptions();
  bool IsLoggingIn()const;
  CGameNetworkProfile* GetProfile()const;
  void OnMenuOpened();
  void OnShowIngameMenu();
  bool IsIngame()const;
  void ShowYesNoDialog(const char* str, const char* name);
  void PlayVideo(const char* name);
  void OnESC();
private:
  IFlashPlayer*                   m_currentScreen;
  std::auto_ptr<CGameNetworkProfile> m_profile;
  std::auto_ptr<CMultiPlayerMenu> m_menu;
  std::auto_ptr<CQuickGame>       m_quickGame;
  SRegisterInfo                   m_reginfo;
  SMPOptions                      m_options;
  std::vector<CDialog*>           m_dialogs;
  bool                            m_loggingIn;
  bool                            m_enteringLobby;
  bool                            m_menuOpened;
  string                          m_login;
  int                             m_lastMenu;
  bool                            m_video;
  string                          m_errrorText;
};


#endif __MPHUB_H__