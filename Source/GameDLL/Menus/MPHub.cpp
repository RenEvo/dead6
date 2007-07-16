#include "StdAfx.h"

#include "INetwork.h"
#include "INetworkService.h"

#include "MPHub.h"
#include "MultiplayerMenu.h"
#include "QuickGame.h"
#include "GameNetworkProfile.h"


#include "Game.h"
#include "IGameFramework.h"
#include "IPlayerProfiles.h"
#include "OptionsManager.h"
#include "FlashMenuObject.h"
#include <IVideoPlayer.h>

static TKeyValuePair<EGsUiCommand,const char*>
gUiCommands[] = { 
  {eGUC_none,""},
  {eGUC_opened,"MenuOpened"},
  {eGUC_back,"Back"},
  {eGUC_cancel,"LoginCancelled"},
  {eGUC_login,"Login"},
  {eGUC_logoff,"ShowAcountInfo"},
  {eGUC_rememberPassword,"MPLogin_RememberPassword"},
  {eGUC_forgotPassword,"MPLogin_ForgotPassword"},
  {eGUC_autoLogin,"MPLogin_AutoLogin"},
  {eGUC_internetGame,"MP_InternetGame"},
  {eGUC_recordedGames,"MP_RecordedGames"},
  {eGUC_enterlobby,"LobbyEnter"},
  {eGUC_leavelobby,"LobbyLeave"},
  {eGUC_enterLANlobby,"LanLobbyEnter"},
  {eGUC_leaveLANlobby,"LanLobbyLeave"},
  {eGUC_update,"UpdateServerList"},
  {eGUC_stop,"StopUpdate"},
  {eGUC_setVisibleServers,"SetVisibleServers"},
  {eGUC_displayServerList,"GetServerList"},
  {eGUC_serverScrollBarPos,"ServerScrollbarPos"},
  {eGUC_serverScroll,"ScrollServerList"},
  {eGUC_refreshServer,"RefreshServerList"},
  {eGUC_selectServer,"SetSelectServer"},
  {eGUC_addFavorite,"AddToFavourites"},
  {eGUC_removeFavorite,"RemoveFromFavourites"},
  {eGUC_sortColumn,"SetSortColumn"},
  {eGUC_join,"JoinServer"},
  {eGUC_joinIP,"ConnectToServerIP"},
  {eGUC_joinPassword,"JoinServerWithPassword"},
  {eGUC_disconnect,"Disconnect"},
  {eGUC_tab,"TabEnter"},
  {eGUC_chatClick,"OnBuddyClick"},
  {eGUC_chatOpen,"OnBuddyOpen"},
  {eGUC_chat,"EnteredMessage"},
  {eGUC_find,"UserFind"},
  {eGUC_addBuddy,"AddToBuddies"},
  {eGUC_addIgnore,"AddToIgnore"},
  {eGUC_addBuddyFromInfo,"InfoAddToBuddies"},
  {eGUC_addIgnoreFromInfo,"InfoAddToIgnore"},
  {eGUC_removeBuddy,"RemoveFromBuddies"},
  {eGUC_inviteBuddy,"sendinvitation"},
  {eGUC_stopIgnore,"StopIgnoring"},
  {eGUC_acceptBuddy,"AddFriendAccepted"},
  {eGUC_declineBuddy,"AddFriendDeclined"},
  {eGUC_displayInfo,"ShowInfoOn"},
  {eGUC_displayInfoInList,"ShowBuddyListInfoOn"},
  {eGUC_joinBuddy,"JoinBuddy"},
  {eGUC_userScrollBarPos,"BuddyScrollbarPos"},
  {eGUC_userScroll,"ScrollBuddyList"},
  {eGUC_chatScrollBarPos,"ChatScrollbarPos"},
  {eGUC_chatScroll,"ScrollChatList"},
  {eGUC_register,"MPAccount_START"},
  {eGUC_registerNick,"MPAccount_LOGIN"},
  {eGUC_registerEmail,"MPAccount_EMAIL"},
  {eGUC_registerDateMM,"MPAccount_DATE_MM"},
  {eGUC_registerDateDD,"MPAccount_DATE_DD"},
  {eGUC_registerDateYY,"MPAccount_DATE_YY"},
  {eGUC_registerParentEmail,"MPAccount_PARENTEMAIL"},
  {eGUC_registerEnd,"MPAccount_END"},
  {eGUC_quickGame,"MP_QuickGame"},
  {eGUC_createServerStart,"StartServer"},
	{eGUC_createServerUpdateLevels,"CreateServer_GameMode"},
	{eGUC_createServerOpened,"CreateGameOpened"},
  {eGUC_createServerParams,"GetGlobalSettings"},
  {eGUC_dialogClosed,"LoadingCanceled"},
  {eGUC_dialogYes,"ErrorBoxYes"},
  {eGUC_dialogNo,"ErrorBoxNo"},
	{eGUC_filtersDisplay,"FiltersOpened"},
  {eGUC_filtersEnable,"MPFilter_Enabled"},
  {eGUC_filtersMode,"MPFilter_GameMode"},
  {eGUC_filtersMap,"MPFilter_Mapname"},
  {eGUC_filtersPing,"MPFilter_Ping"},
  {eGUC_filtersNotFull,"MPFilter_NotFull"},
  {eGUC_filtersNotEmpty,"MPFilter_NotEmpty"},
  {eGUC_filtersNoPassword,"MPFilter_NoPassword"},
  {eGUC_filtersAutoTeamBalance,"MPFilter_AutoTeam"},
  {eGUC_filtersAntiCheat,"MPFilter_AntiCheat"},
  {eGUC_filtersFriendlyFire,"MPFilter_FriendlyFire"},
  {eGUC_filtersGamepadsOnly,"MPFilter_GamepadsOnly"},
  {eGUC_filtersNoVoiceComms,"MPFilter_NoVoiceComms"},
  {eGUC_filtersDedicated,"MPFilter_DecicatedServer"},
  {eGUC_filtersDX10,"MPFilter_DirectX"}
};

CMPHub::CMPHub():
m_menu(0),
m_currentScreen(0),
m_loggingIn(false),
m_enteringLobby(false),
m_menuOpened(false),
m_lastMenu(0),
m_video(false)
{
  gEnv->pNetwork->GetService("GameSpy");
}

CMPHub::~CMPHub()
{
 // m_menu.reset(0);
 // m_profile.reset(0);
}

bool CMPHub::HandleFSCommand(const char* pCmd, const char* pArgs)
{
  EGsUiCommand cmd = KEY_BY_VALUE(string(pCmd),gUiCommands);

  for(int i=m_dialogs.size()-1;i>=0;--i)
  {
    if(m_dialogs[i]->OnCommand(cmd,pArgs))
    {
      return true;
    }
  }

  bool handled = true;

  switch(cmd)
  {
  case eGUC_opened:
    OnMenuOpened();
    break;
  case eGUC_register:
    m_reginfo = SRegisterInfo();
    break;
  case eGUC_registerNick:
    m_reginfo.nick = pArgs;
    break;
  case eGUC_registerEmail:
    m_reginfo.email = pArgs;
    break;
  case eGUC_registerDateDD:
    m_reginfo.day = atoi(pArgs);
    break;
  case eGUC_registerDateMM:
    m_reginfo.month = atoi(pArgs);
    break;
  case eGUC_registerDateYY:
    m_reginfo.year = atoi(pArgs);
    break;
  case eGUC_registerParentEmail:
    break;
  case eGUC_registerEnd:
    {
      {
        INetworkService *serv = gEnv->pNetwork->GetService("GameSpy");
        if(!serv || serv->GetState()!= eNSS_Ok)
          break;
      }
      SFlashVarValue val("");
      m_currentScreen->GetVariable("_root.MPAccount_Password",&val);
      string pass = val.GetConstStrPtr();
      m_profile.reset(new CGameNetworkProfile(this));
      m_profile->Register(m_reginfo.nick,m_reginfo.email,pass);
      m_loggingIn = true;
      m_enteringLobby = true;
      ShowLoadingDlg("@ui_menu_register");
    }
    break;
  case eGUC_rememberPassword:
    m_options.remeber = atoi(pArgs)!=0;
    break;
  case eGUC_autoLogin:
    m_options.autologin = atoi(pArgs)!=0; 
    break;
  case eGUC_login:
    {
      {
        INetworkService *serv = gEnv->pNetwork->GetService("GameSpy");
        if(!serv || serv->GetState()!= eNSS_Ok)
          break;
      }
      SFlashVarValue val("");
      m_currentScreen->GetVariable("_root.MPAccount_Password",&val);
      string pass = val.GetConstStrPtr();
      string login(pArgs);
      if(m_options.remeber || m_options.autologin)
      {
        m_options.login = login;
        m_options.password = pass;
      }
      m_enteringLobby = true;
      DoLogin(login,pass);
    }
    break;
  case eGUC_logoff:
    DoLogoff();
    m_options.autologin = false;
    SaveOptions();
    break;
  case eGUC_enterlobby:
    m_menu.reset(new CMultiPlayerMenu(false,m_currentScreen,this));
    m_lastMenu = 3;
    break;
  case eGUC_leavelobby:
  case eGUC_leaveLANlobby:
    m_lastMenu = 0;
    m_menu.reset(0);
    break;
  case eGUC_internetGame:
    {
      INetworkService *serv = gEnv->pNetwork->GetService("GameSpy");
      if(!serv || serv->GetState()!= eNSS_Ok)
        break;
    }
    if(!m_profile.get() || !m_profile->IsLoggedIn())
    {
      ReadOptions();
      if(m_options.autologin)
      {
        if(m_options.login.empty())
        {
          m_options.autologin = false;
          ShowLoginDlg();
        }
        else
        {
          DoLogin(m_options.login,m_options.password);
          m_enteringLobby = true;
        }
      }
      else
        ShowLoginDlg();
    }
    else
      SwitchToLobby();
    break;
  case eGUC_recordedGames:
    //ShowYesNoDialog("This will play Power Struggle Tutorial video in a new window. Continue?","tutorial");
    PlayVideo("Localized/Video/English/PS_Tutorial.sfd");
    break;
  case eGUC_enterLANlobby:
    {
      INetworkService *serv = gEnv->pNetwork->GetService("GameSpy");
      if(!serv || serv->GetState()!= eNSS_Ok)
        break;
    }
    m_menu.reset(new CMultiPlayerMenu(true,m_currentScreen,this));
    break;
  case eGUC_quickGame:
    OnQuickGame();
    break;
  case eGUC_back:
    if(m_quickGame.get())
    {
      m_quickGame->Cancel();
      m_quickGame.reset(0);
    }
    break;
  case eGUC_dialogClosed:
    gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
    break;
  case eGUC_dialogYes:
    if(!strcmp(pArgs,"tutorial"))
    {
      g_pGame->GetIGameFramework()->ShowPageInBrowser("Power Struggle Tutorial.wmv");
    }
    else if(pArgs,"patch")
    {
      INetworkService* gs = gEnv->pNetwork->GetService("GameSpy");
      if(gs)
      {
        IPatchCheck* pc = gs->GetPatchCheck();
        if(pc->IsUpdateAvailable())
        {
          g_pGame->GetIGameFramework()->ShowPageInBrowser(pc->GetPatchURL());
          g_pGame->GetIGameFramework()->ExecuteCommandNextFrame("quit");
        }
      }
    }
    break;
  default:
    handled = false;
  }

  if(m_menu.get() && !handled)
  {
    handled = m_menu->HandleFSCommand(cmd,pArgs);
  }
  return handled;
}

void CMPHub::OnUIEvent(const SUIEvent& event)
{
  switch(event.event)
  {
  case eUIE_disconnnect:
    if(int(eDC_NubDestroyed) == event.param)
    {
       if(!gEnv->bServer)
         ShowError("Server quit");
    }
    else if(int(eDC_UserRequested) != event.param)
      ShowError(event.descrpition);
    break;
  }

  if(m_menu.get())
    m_menu->OnUIEvent(event);

  for(int i=m_dialogs.size()-1;i>=0;--i)
  {
    m_dialogs[i]->OnUIEvent(event);
  }
}

void CMPHub::SetCurrentFlashScreen(IFlashPlayer* screen, bool ingame)
{
  if(m_currentScreen && !screen)
  {
    OnUIEvent(SUIEvent(eUIE_destroy,ingame?0:1));
    for(int i=m_dialogs.size()-1;i>=0;--i)
      m_dialogs[i]->Close();
    m_menu.reset(0);
  }

  m_menuOpened = false;
  m_currentScreen = screen;
   
  if(m_currentScreen)
  {
    if(gEnv->bMultiplayer)
    {
      OnShowIngameMenu();
    }
  }
}

void CMPHub::ConnectFailed(EConnectionFailureCause cause, const char * description)
{
  OnUIEvent(SUIEvent(eUIE_connectFailed,int(cause),description));
}

void CMPHub::OnLoginSuccess(const char* nick)
{
  m_loggingIn = false;
  CloseLoadingDlg();
  CloseLoginDlg();
  if(m_enteringLobby)
    SwitchToLobby();
  m_enteringLobby = false;
  m_login = nick;
  SetLoginInfo(nick);
  SaveOptions();
}

void CMPHub::OnLoginFailed(const char* reason)
{
  m_loggingIn = false;
  m_enteringLobby = false;
  CloseLoadingDlg();
  m_profile.reset(0);
  ShowError(string().Format("Error : \n%s",reason));
  m_options.autologin = false;
  m_options.remeber = false;
  //ShowLoginDlg();
}

void CMPHub::ShowLoginDlg()
{
  if(!m_currentScreen)
    return;
  SFlashVarValue params[] = {"1",m_options.remeber,m_options.autologin};
  m_currentScreen->Invoke("_root.Root.MainMenu.MultiPlayer.openLoginScreen",params,sizeof(params)/sizeof(params[0]));
  m_currentScreen->SetVariable("_root.Root.MainMenu.MultiPlayer.LoginScreen.Login_Controls.LoginStats.Colorset.Nickname.text",m_options.login.c_str());
  if(m_options.remeber)
    m_currentScreen->SetVariable("_root.Root.MainMenu.MultiPlayer.LoginScreen.Login_Controls.LoginStats.Colorset.Password.text",m_options.password.c_str());
	m_currentScreen->Invoke0("_root.Root.MainMenu.MultiPlayer.updateLoginScreen");
}

void CMPHub::CloseLoginDlg()
{
  if(m_currentScreen)
    m_currentScreen->Invoke1("_root.Root.MainMenu.MultiPlayer.openLoginScreen",false);
}

void CMPHub::SwitchToLobby()
{
  if(m_currentScreen)
  {
    m_currentScreen->Invoke1("_root.Root.MainMenu.MultiPlayer.MultiPlayer.gotoAndPlay","internetgame");
    m_lastMenu = 2;
  }
}

void CMPHub::ShowLoadingDlg(const char* message)
{
  if(m_currentScreen)
    m_currentScreen->Invoke1("showLOADING","1");
  //TODO : set header
  SetLoadingDlgText(message,true);
}

void CMPHub::SetLoadingDlgText(const char* text, bool localize)
{
  SFlashVarValue args[]={text,localize};
  if(m_currentScreen)
    m_currentScreen->Invoke("setLOADINGText",args,sizeof(args)/sizeof(args[0]));
}

void CMPHub::CloseLoadingDlg()
{
  if(m_currentScreen)
    m_currentScreen->Invoke1("showLOADING",false);
}

void CMPHub::OnQuickGame()
{
  INetworkService *serv = gEnv->pNetwork->GetService("GameSpy");
  if(!serv || serv->GetState()!= eNSS_Ok)
    return;

  m_quickGame.reset(new CQuickGame());
  m_quickGame->StartSearch(this);
}

void CMPHub::SetLoginInfo(const char* nick)
{
  if(!m_menuOpened || !m_currentScreen)
    return;
  if(nick)
  {
    m_currentScreen->Invoke0("GSConnect");
    m_currentScreen->Invoke1("setActiveProfile",nick);
  }
  else
  {
    m_currentScreen->Invoke0("GSDisconnect");
    IPlayerProfileManager *pProfileMan = g_pGame->GetOptions()->GetProfileManager();
    if(pProfileMan)
    {
      const char *userName = pProfileMan->GetCurrentUser();
      IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(userName);
      if(pProfile)
      {
        m_currentScreen->Invoke1("setActiveProfile",pProfile->GetName());
      }
    }
  }
}

void CMPHub::ShowError(const char* msg)
{
  if(!m_currentScreen)
  {
    m_errrorText = msg;//will be shown next time
    return;
  }
  m_currentScreen->Invoke1("setErrorTextNonLocalized",msg);
  m_currentScreen->Invoke1("showErrorMessage","Box1");
  m_errrorText.resize(0);
}

void CMPHub::DoLogin(const char* nick, const char* pwd)
{
  if(!nick || !*nick)
  {
    ShowError("@ui_Enter_Login");
    return;
  }
  m_profile.reset(new CGameNetworkProfile(this));
  m_profile->Login(nick,pwd);
  m_loggingIn = true;
  ShowLoadingDlg("@ui_menu_login");
}

void CMPHub::DoLogoff()
{
  if(!m_profile.get())
    return;
  m_login.resize(0);
  m_profile->Logoff();
  m_profile.reset(0);
  SetLoginInfo(0);
  if(m_menu.get())
    SwitchToMainScreen();
}

void CMPHub::SwitchToMainScreen()
{
  if(m_currentScreen)
    m_currentScreen->Invoke1("_root.Root.MainMenu.MultiPlayer.GameLobby_M.gotoAndPlay","close");
}

void CMPHub::ReadOptions()
{
  m_options.autologin = false;
  m_options.remeber = false;
  g_pGame->GetOptions()->GetProfileValue("Multiplayer.Login.AutoLogin",m_options.autologin);
  g_pGame->GetOptions()->GetProfileValue("Multiplayer.Login.RememberPassword",m_options.remeber);
  g_pGame->GetOptions()->GetProfileValue("Multiplayer.Login.Login",m_options.login);
  g_pGame->GetOptions()->GetProfileValue("Multiplayer.Login.Password",m_options.password);
}

void CMPHub::SaveOptions()
{
  g_pGame->GetOptions()->SaveValueToProfile("Multiplayer.Login.AutoLogin",m_options.autologin);
  g_pGame->GetOptions()->SaveValueToProfile("Multiplayer.Login.RememberPassword",m_options.remeber);
  g_pGame->GetOptions()->SaveValueToProfile("Multiplayer.Login.Login",m_options.login);
  g_pGame->GetOptions()->SaveValueToProfile("Multiplayer.Login.Password",m_options.password);
}

bool CMPHub::IsLoggingIn()const
{
  return m_loggingIn;
}

void CMPHub::OnMenuOpened()
{
  INetworkService* gs = gEnv->pNetwork->GetService("GameSpy");
  if(gs)
  {
    IPatchCheck* pc = gs->GetPatchCheck();
    if(pc->IsUpdateAvailable())
    {
      ShowYesNoDialog("Update available. This will open patch download page in your default Internet browser. Continue?","patch");
    }
  }
  m_menuOpened = true;
  if(!m_login.empty())
    SetLoginInfo(m_login);
  if(!m_errrorText.empty())
  {
    ShowError(m_errrorText);
    m_errrorText.empty();
  }
}

void CMPHub::OnShowIngameMenu()
{
  if(!m_currentScreen)
    return;
  m_currentScreen->SetVariable("MainWindow",2);
  if(m_lastMenu)
    m_currentScreen->SetVariable("SubWindow",m_lastMenu);
}


bool CMPHub::IsIngame()const
{
  return gEnv->pGame->GetIGameFramework()->IsGameStarted();
}

void CMPHub::ShowYesNoDialog(const char* str, const char* name)
{
  if(!m_currentScreen)
    return;
  SFlashVarValue args[]={"Box2",name};
  m_currentScreen->Invoke("showErrorMessage",args,sizeof(args)/sizeof(args[0]));
  m_currentScreen->Invoke1("setErrorText",str);
}

void CMPHub::PlayVideo(const char* name)
{
  if(g_pGame->GetMenu()->PlayVideo(name, false, IVideoPlayer::LOOP_PLAYBACK))
  {
    gEnv->pConsole->ExecuteString("sys_flash 0");
    m_video = true;
  }
}

void CMPHub::OnESC()
{
  ///WHAT AN AWFUL HACK!
  if(m_video)
  {
    g_pGame->GetMenu()->StopVideo();
    g_pGame->GetMenu()->PlayVideo("Languages/Video/bg.sfd", false, IVideoPlayer::LOOP_PLAYBACK);
    gEnv->pConsole->ExecuteString("sys_flash 1");
    m_video = false;
  }
}

CGameNetworkProfile* CMPHub::GetProfile()const
{
  return m_profile.get();
}

void CMPHub::AddGameModToList(const char* mod)
{
  if(m_currentScreen)
  {
    //_root.Root.MainMenu.MultiPlayer.ClearGameModeList() - clear
    m_currentScreen->Invoke1("_root.Root.MainMenu.MultiPlayer.AddGameMode",mod);
  }
}

CMPHub::CDialog::CDialog():
m_hub(0)
{
}

CMPHub::CDialog::~CDialog()
{
  Close();
}

void CMPHub::CDialog::Show(CMPHub* hub)
{
  m_hub = hub;
  m_hub->m_dialogs.push_back(this);
  //show in UI
  OnShow();
}

void CMPHub::CDialog::Close()
{
  if(!m_hub)//m_hub !=0 
    return;
  OnClose();
  //hide in UI
  assert(m_hub->m_dialogs.back() == this);
  m_hub->m_dialogs.pop_back();
  m_hub = 0;
}

bool CMPHub::CDialog::OnCommand(EGsUiCommand cmd, const char* pArgs)
{
  return false;
}

void CMPHub::CDialog::OnUIEvent(const SUIEvent& event)
{

}

void CMPHub::CDialog::OnClose()
{

}

void CMPHub::CDialog::OnShow()
{

}
