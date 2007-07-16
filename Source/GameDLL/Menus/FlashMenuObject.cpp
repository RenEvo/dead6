/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Flash menu screens manager

-------------------------------------------------------------------------
History:
- 07:18:2006: Created by Julien Darre

*************************************************************************/
#include "StdAfx.h"
#include <StlUtils.h>

#include <IVideoPlayer.h>

#include "FlashMenuObject.h"
#include "FlashMenuScreen.h"
#include "IGameFramework.h"
#include "IPlayerProfiles.h"
#include "IUIDraw.h"
#include "IMusicSystem.h"
#include "ISound.h"
#include "Game.h"
#include "GameCVars.h"
#include <CryPath.h>
#include <ISaveGame.h>
#include "MPHub.h"
#include "HUD/HUD.h"
#include "HUD/HUDTextChat.h"
#include "OptionsManager.h"
#include "IAVI_Reader.h"

//-----------------------------------------------------------------------------------------------------

#define CRYSIS_PROFILE_COLOR_AMBER	"12612932"
#define CRYSIS_PROFILE_COLOR_BLUE		"5079987"
#define CRYSIS_PROFILE_COLOR_GREEN	"4481854"
#define CRYSIS_PROFILE_COLOR_RED		"7474188"
#define CRYSIS_PROFILE_COLOR_WHITE	"13553087"

//-----------------------------------------------------------------------------------------------------

#define SAFE_HARDWARE_MOUSE_FUNC(func)\
	if(gEnv->pHardwareMouse)\
		gEnv->pHardwareMouse->func

//-----------------------------------------------------------------------------------------------------

#define SAFE_HUD_FUNC(func)\
	if(g_pGame && g_pGame->GetHUD()) (g_pGame->GetHUD()->func)

#define SAFE_HUD_FUNC_RET(func)\
	((g_pGame && g_pGame->GetHUD()) ? g_pGame->GetHUD()->func : NULL)

//-----------------------------------------------------------------------------------------------------

static const int BLACK_FRAMES = 4;

static TKeyValuePair<CFlashMenuObject::EEntryMovieState,const char*>
gMovies[] = {
	{CFlashMenuObject::eEMS_Start,""},
	{CFlashMenuObject::eEMS_EA,"Localized/Video/Trailer_EA.sfd"},
	{CFlashMenuObject::eEMS_Crytek,"Localized/Video/Trailer_Crytek.sfd"},
	{CFlashMenuObject::eEMS_EAGames,"Localized/Video/Trailer_EA.sfd"},
	{CFlashMenuObject::eEMS_NVidia,"Localized/Video/Trailer_NVidia.sfd"},
	{CFlashMenuObject::eEMS_Intel,"Localized/Video/Trailer_Intel.sfd"},
	{CFlashMenuObject::eEMS_Dell,"Localized/Video/Trailer_Dell.sfd"},
	{CFlashMenuObject::eEMS_SofDec,"Localized/Video/Trailer_SofDec.sfd"},
	{CFlashMenuObject::eEMS_Stop,""},
	{CFlashMenuObject::eEMS_Done,"Localized/Video/bg.sfd"},
	{CFlashMenuObject::eEMS_GameStart,""},
	{CFlashMenuObject::eEMS_GameIntro,"Localized/Video/English/Intro.sfd"},
	{CFlashMenuObject::eEMS_GameStop,""},
	{CFlashMenuObject::eEMS_GameDone,""}
};

//-----------------------------------------------------------------------------------------------------

CFlashMenuObject *CFlashMenuObject::s_pFlashMenuObject = NULL;

//-----------------------------------------------------------------------------------------------------

CFlashMenuObject::CFlashMenuObject()
: m_pFlashPlayer(0)
, m_pVideoPlayer(0)
, m_multiplayerMenu(0)
{
	s_pFlashMenuObject = this;

	for(int i=0; i<MENUSCREEN_COUNT; i++)
	{
		m_apFlashMenuScreens[i] = NULL;
	}

	m_iGamepadsConnected = 0;

	m_pCurrentFlashMenuScreen	= NULL;
	m_pPlayerProfileManager = NULL;

	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return;

	m_eCrysisProfileColor = CrysisProfileColor_Default;
	m_iMaxProgress = 100;

	m_bNoMoveEnabled = false;
	m_bNoMouseEnabled	= false;
	m_bLanQuery = false;
	m_bIgnoreEsc = true;
	m_bUpdate = true;
	m_bVirtualKeyboardFocus = false;
	m_nBlackGraceFrames = 0;
	m_nLastFrameUpdateID = 0;
	m_bColorChanged = true;

	m_fLastUnloadFrame = 0.0f;
	m_fMusicFirstTime = -1.0f;

	m_bCatchNextInput = false;
	m_bTakeScreenshot = false;
	m_bControllerConnected = false;

	m_bDestroyStartMenuPending = false;
	m_bDestroyInGameMenuPending = false;

	m_stateEntryMovies = eEMS_Start;

	SAFE_HARDWARE_MOUSE_FUNC(AddListener(this));

	gEnv->pInput->AddEventListener(this);

	gEnv->pGame->GetIGameFramework()->RegisterListener(this, "flashmenu", FRAMEWORKLISTENERPRIORITY_MENU);
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);

	m_pMusicSystem = gEnv->pSystem->GetIMusicSystem();
	if(m_pMusicSystem)
	{
		m_pMusicSystem->LoadFromXML("music/common_music.xml",true);
		m_pMusicSystem->SetTheme("menu", false, false);
		m_pMusicSystem->SetDefaultMood("menu_music_1st_time");
		m_fMusicFirstTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
	}	

	if(gEnv->pCryPak->GetLvlResStatus())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING] = new CFlashMenuScreen;
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Load("Libs/UI/Menus_Loading_MP.gfx");
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART] = new CFlashMenuScreen;
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Load("Libs/UI/Menus_StartMenu.gfx");
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] = new CFlashMenuScreen;
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Load("Libs/UI/Menus_IngameMenu.gfx");
	}

  m_multiplayerMenu = new CMPHub();

	// create the avi reader; 
	//m_pAVIReader = g_pISystem->CreateAVIReader();
	//m_pAVIReader->OpenFile("Crysis_main_menu_background.avi");
}

//-----------------------------------------------------------------------------------------------------

CFlashMenuObject::~CFlashMenuObject()
{
  SAFE_DELETE(m_multiplayerMenu);

	SAFE_RELEASE(m_pFlashPlayer);
	SAFE_RELEASE(m_pVideoPlayer);

	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return;

	gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
	gEnv->pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);

	gEnv->pInput->RemoveEventListener(this);

	SAFE_HARDWARE_MOUSE_FUNC(RemoveListener(this));

	DestroyStartMenu();
	DestroyIngameMenu();

	for(int i=0; i<MENUSCREEN_COUNT; i++)
	{
		SAFE_DELETE(m_apFlashMenuScreens[i]);
	}
  
	m_pCurrentFlashMenuScreen	= NULL;
	
	s_pFlashMenuObject = NULL;

	// g_pISystem->ReleaseAVIReader(m_pAVIReader);
}

//-----------------------------------------------------------------------------------------------------

CFlashMenuObject *CFlashMenuObject::GetFlashMenuObject()
{
	return s_pFlashMenuObject;
}

//-----------------------------------------------------------------------------------------------------

ECrysisProfileColor CFlashMenuObject::GetCrysisProfileColor()
{
	return m_eCrysisProfileColor;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::UpdateRatio()
{
	for(int i=0; i<MENUSCREEN_COUNT; i++)
	{
		if(m_apFlashMenuScreens[i])
		{
			m_apFlashMenuScreens[i]->UpdateRatio();
		}
	}

	StopVideo();

	const char* movie = VALUE_BY_KEY(m_stateEntryMovies, gMovies);
	if(movie)
	{
		if(m_stateEntryMovies == eEMS_Done)
			PlayVideo(movie, false, IVideoPlayer::LOOP_PLAYBACK);
		else
			PlayVideo(movie, true, IVideoPlayer::DELAY_START);
	}

	m_iWidth = gEnv->pRenderer->GetWidth();
	m_iHeight = gEnv->pRenderer->GetHeight();
}

//-----------------------------------------------------------------------------------------------------

bool CFlashMenuObject::ColorChanged()
{
	if(m_bColorChanged)
	{
		m_bColorChanged = false;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SetColorChanged()
{
	m_bColorChanged = true;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::PlaySound(ESound eSound)
{
	if(!gEnv->pSoundSystem) return;

	const char *strSound = NULL;

	switch(eSound)
	{
	case ESound_RollOver:
		strSound = "Sounds/interface:menu:rollover";
		break;
	case ESound_Click1:
		strSound = "Sounds/interface:menu:click1";
		break;
	case ESound_Click2:
		strSound = "Sounds/interface:menu:click2";
		break;
	case ESound_ScreenChange:
		strSound = "Sounds/interface:menu:screen_change";
		break;
	case ESound_MenuHighlight:
		strSound = "sounds/interface:menu:rollover";
		break;
	case ESound_MenuSelect:
		strSound = "sounds/interface:menu:click1";
		break;
	case ESound_MenuClose:
		strSound = "sounds/interface:menu:close";
		break;
	case ESound_MenuOpen:
		strSound = "sounds/interface:menu:open";
		break;
	default:
		assert(0);
		return;
	}

	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(strSound,FLAG_SOUND_2D);

	if(pSound)
	{
		pSound->Play();
	}
}

//-----------------------------------------------------------------------------------------------------

int CFlashMenuObject::KeyNameToKeyCode(const SInputEvent &rInputEvent, uint32 keyState, bool &specialKey)
{
	EKeyId keyId = rInputEvent.keyId;
	specialKey = true;
	if(keyId == eKI_Backspace)			return 8;
	else if(keyId == eKI_Tab)				return 9;
	else if(keyId == eKI_Enter)			return 13;
	else if(keyId == eKI_LShift)		return 16;
	else if(keyId == eKI_RShift)		return 16;
	else if(keyId == eKI_RCtrl)			return 17;
	else if(keyId == eKI_LCtrl)			return 17;
	else if(keyId == eKI_RAlt)			return 18;
	else if(keyId == eKI_LAlt)			return 18;
	else if(keyId == eKI_Escape)				return 27;
	else if(keyId == eKI_PgUp)			return 33;
	else if(keyId == eKI_PgDn)			return 34;
	else if(keyId == eKI_End)				return 35;
	else if(keyId == eKI_Home)			return 36;
	else if(keyId == eKI_Left)			return 37;
	else if(keyId == eKI_Up)				return 38;
	else if(keyId == eKI_Right)			return 39;
	else if(keyId == eKI_Down)			return 40;
	else if(keyId == eKI_Insert)		return 45;
	else if(keyId == eKI_Delete)		return 46;
	else
	{
		specialKey = false;
		const char *key = GetISystem()->GetIInput()->GetKeyName(rInputEvent, true);
		return (int)key[0];
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------

bool CFlashMenuObject::OnInputEvent(const SInputEvent &rInputEvent)
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated() || rInputEvent.keyId == eKI_SYS_Commit)
		return false;

  if(eIS_Released == rInputEvent.state && rInputEvent.keyId == eKI_Escape)
  {
		if(m_stateEntryMovies!=eEMS_Done && m_stateEntryMovies!=eEMS_Stop)
		{
			StopVideo();
		}
		else if(m_multiplayerMenu)
      m_multiplayerMenu->OnESC();
  }

  if(m_bCatchNextInput && !gEnv->pConsole->GetStatus())
	{
		if(eIS_Pressed == rInputEvent.state)
		{
			const char* key = rInputEvent.keyName.c_str();

			const bool bGamePad = false;
			if (rInputEvent.deviceId == eDI_XI)
				bGamePad == true;
			const bool bSave = !m_catchGamePad || bGamePad;
			if(bSave)
			{
				ChangeFlashKey(m_sActionMapToCatch, m_sActionToCatch, key, true, bGamePad);
				m_sActionMapToCatch.clear();
				m_sActionToCatch.clear();
				m_bCatchNextInput = false;
			}
			return false;
		}
	}
		
	if(eDI_XI == rInputEvent.deviceId)
	{
		if(gEnv->pConsole->GetStatus())
			return false;

		if(m_bUpdate && m_pCurrentFlashMenuScreen)
		{
			if(rInputEvent.keyId == eKI_XI_Back && rInputEvent.state == eIS_Released)
			{
				m_pCurrentFlashMenuScreen->CheckedInvoke("onBack");
			}
		}
	}

	if(eDI_Keyboard == rInputEvent.deviceId)
	{
		if(gEnv->pConsole->GetStatus())
			return false;

		if(m_bUpdate && m_pCurrentFlashMenuScreen)
		{
			bool specialKey;
			int keyCode = KeyNameToKeyCode(rInputEvent, (uint32)rInputEvent.state, specialKey);
			unsigned char asciiCode = 0;
			if(!specialKey)
				asciiCode = (unsigned char)keyCode;

			if(eIS_Pressed == rInputEvent.state)
			{
        if(m_pCurrentFlashMenuScreen->GetFlashPlayer())
        {
				  m_pCurrentFlashMenuScreen->CheckedInvoke("onPressedKey", rInputEvent.keyName.c_str());
				  SFlashKeyEvent keyEvent(SFlashKeyEvent::eKeyDown, static_cast<SFlashKeyEvent::EKeyCode>(keyCode), asciiCode);
				  m_pCurrentFlashMenuScreen->GetFlashPlayer()->SendKeyEvent(keyEvent);
				  if (m_pFlashPlayer)
					  m_pFlashPlayer->SendKeyEvent(keyEvent);
        }
			}
			else if(eIS_Released == rInputEvent.state)
			{
				SFlashKeyEvent keyEvent(SFlashKeyEvent::eKeyUp, static_cast<SFlashKeyEvent::EKeyCode>(keyCode), asciiCode);
				if(m_pCurrentFlashMenuScreen->IsLoaded())
					m_pCurrentFlashMenuScreen->GetFlashPlayer()->SendKeyEvent(keyEvent);
				if (m_pFlashPlayer)
					m_pFlashPlayer->SendKeyEvent(keyEvent);
			}
		}
	}
	else if(eDI_XI == rInputEvent.deviceId)		//x-gamepad controls
	{
		int oldGamepads = m_iGamepadsConnected;
		if(rInputEvent.keyId == eKI_XI_Connect)
			(m_iGamepadsConnected>=0)?m_iGamepadsConnected++:(m_iGamepadsConnected=1);
		else if (rInputEvent.keyId == eKI_XI_Disconnect)
			(m_iGamepadsConnected>0)?m_iGamepadsConnected--:(m_iGamepadsConnected=0);
		
		if(m_iGamepadsConnected != oldGamepads)
		{
			bool connected = (m_iGamepadsConnected > 0)?true:false;
			if(connected != m_bControllerConnected)
			{
				if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
					m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("GamepadAvailable", m_iGamepadsConnected?true:false);
				if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->IsLoaded())
					m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("GamepadAvailable", m_iGamepadsConnected?true:false);

				SAFE_HUD_FUNC(ShowVirtualKeyboard(connected));
			}

			m_bControllerConnected = connected;
		}

		//X-gamepad virtual keyboard input
		bool move			= false;
		bool genericA	= false;
		bool commit		= rInputEvent.state == eIS_Pressed;
		const char* direction = "";
		Vec2 dirvec(0,0);
		if(rInputEvent.keyId == eKI_XI_DPadUp)
		{
			move = true;
			direction = "up";
			dirvec = Vec2(0,-1);
		}
		else if(rInputEvent.keyId == eKI_XI_DPadDown)
		{
			move = true;
			direction = "down";
			dirvec = Vec2(0,1);
		}
		else if(rInputEvent.keyId == eKI_XI_DPadLeft)
		{
			move = true;
			direction = "left";
			dirvec = Vec2(-1,0);
		}
		else if(rInputEvent.keyId == eKI_XI_DPadRight)
		{
			move = true;
			direction = "right";
			dirvec = Vec2(1,0);
		}
		else if(rInputEvent.keyId == eKI_XI_A)
		{
			move = true;
			// Note: commit on release so no release events come after closing virtual keyboard!
			commit = rInputEvent.state == eIS_Released;
			direction = "press";

			genericA=FindButton("xi_a") == m_avButtonPosition.end();
		}
		
		if (m_bVirtualKeyboardFocus)
		{
			// Virtual keyboard navigation
			if(move && commit)
			{			
				if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
					m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("moveCursor", direction);
				if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->IsLoaded())
					m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("moveCursor", direction);
			}
		}
		else if (m_pCurrentFlashMenuScreen->IsLoaded())
		{
			// Controller menu navigation:
			//	Select menu item
			if (genericA)
			{
				// Better simply emulate a mouse button push in this case so push works for non-tracked UI elements too!
				//PushButton(m_avButtonPosition.find(m_sCurrentButton), rInputEvent.state == eIS_Pressed, false);
				float x, y;
				SAFE_HARDWARE_MOUSE_FUNC(GetHardwareMouseClientPosition(&x, &y));
				SAFE_HARDWARE_MOUSE_FUNC(Event((int)x, (int)y, rInputEvent.state == eIS_Pressed ? HARDWAREMOUSEEVENT_LBUTTONDOWN : HARDWAREMOUSEEVENT_LBUTTONUP));
			}
			//	Navigate
			else if(dirvec.GetLength()>0.0 && commit)
			{
				SnapToNextButton(dirvec);
			}
			//	Try find button for shortcut action and press it
			else
			{
				PushButton(FindButton(rInputEvent.keyName), rInputEvent.state == eIS_Pressed, true);
			}
		}		
	}

	if(eIS_Pressed == rInputEvent.state)
	{
		if(eDI_Keyboard == rInputEvent.deviceId || eDI_XI == rInputEvent.deviceId)
		{
			if(!m_bIgnoreEsc && rInputEvent.keyId == eKI_Escape && gEnv->pGame->GetIGameFramework()->IsGameStarted())
      {
        if(m_bUpdate)
          HideInGameMenuNextFrame();
        else
          ShowInGameMenu(true);
      }
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::OnLoadingStart(ILevelInfo *pLevel)
{
	SAFE_HUD_FUNC(OnLoadingStart(pLevel));

	StopVideo();	//stop background video

	if(m_stateEntryMovies!=eEMS_Done) m_stateEntryMovies=eEMS_Stop;

	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return;

	if(m_bLanQuery)
	{
		gEnv->pGame->GetIGameFramework()->EndCurrentQuery();
		m_bLanQuery = false;
	}
	if(pLevel)
	{
		m_iMaxProgress = pLevel->GetDefaultGameType()->cgfCount;
	}
	//DestroyStartMenu();
	if(!m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->IsLoaded())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Load("Libs/UI/Menus_Loading_MP.gfx");
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->GetFlashPlayer()->SetFSCommandHandler(this);
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("SetProgress", 0.0f);
		
		UpdateMenuColor();
	}

	//now load the actual map
	string mapName = (pLevel->GetPath());
	int slashPos = mapName.rfind('\\');
	if(slashPos == -1)
		slashPos = mapName.rfind('/');
	mapName = mapName.substr(slashPos+1, mapName.length()-slashPos);

	string sXml(pLevel->GetPath());
	sXml.append("/");
	sXml.append(mapName);
	sXml.append(".xml");
	XmlNodeRef mapInfo = GetISystem()->LoadXmlFile(sXml.c_str());
	std::vector<string> screenArray;

	const char* header = NULL;
	const char* description = NULL;

	if(mapInfo == 0)
	{
		GameWarning("Did not find a map info file %s in %s.", sXml.c_str(), mapName.c_str());
	}
	else
	{
		//retrieve the coordinates of the map
		if(mapInfo)
		{
			for(int n = 0; n < mapInfo->getChildCount(); ++n)
			{
				XmlNodeRef mapNode = mapInfo->getChild(n);
				const char* name = mapNode->getTag();
				if(!stricmp(name, "LoadingScreens"))
				{
					int attribs = mapNode->getNumAttributes();
					const char* key;
					const char* value;
					for(int i = 0; i < attribs; ++i)
					{
						mapNode->getAttributeByIndex(i, &key, &value);
						screenArray.push_back(value);
					}
				}
				else if(!stricmp(name, "HeaderText"))
				{
					int attribs = mapNode->getNumAttributes();
					const char* key;
					for(int i = 0; i < attribs; ++i)
					{
						mapNode->getAttributeByIndex(i, &key, &header);
						if(!stricmp(key,"text"))
						{
							break;
						}
					}
				}
				else if(!stricmp(name, "DescriptionText"))
				{
					int attribs = mapNode->getNumAttributes();
					const char* key;
					for(int i = 0; i < attribs; ++i)
					{
						mapNode->getAttributeByIndex(i, &key, &description);
						if(!stricmp(key,"text"))
						{
							break;
						}
					}
				}

			}
		}
	}

	int size = screenArray.size();
	if(size<=0)
	{
		screenArray.push_back("loading.dds");
		size = 1;
	}

	if(!header)
	{
		header = "";
	}
	if(!description)
	{
		description = "";
	}

	uint iUse = cry_rand()%size;
	string sImg(pLevel->GetPath());
	sImg.append("/");
	sImg.append(screenArray[iUse]);

	SFlashVarValue arg[2] = {header,description};
	m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("setText",arg,2);
	m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("setMapBackground",SFlashVarValue(sImg));
	m_pCurrentFlashMenuScreen = m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING];

	m_bUpdate = true;
	m_nBlackGraceFrames = 0; 

	if(m_pMusicSystem && m_fMusicFirstTime == -1.0f)
	{
		m_pMusicSystem->SetMood("multiplayer_high");
	}

	SetDifficulty(); //set difficulty setting at game/level start
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::OnLoadingComplete(ILevel *pLevel)
{
	SAFE_HUD_FUNC(OnLoadingComplete(pLevel));

	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) 
	{
		return;
	}

	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
	{
		if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->IsLoaded())
			m_bDestroyStartMenuPending = true;
	}

/*	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME])
	{
		if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
			m_bDestroyInGameMenuPending = true;
	}*/

	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->IsLoaded())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Unload();
		m_fLastUnloadFrame = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	}

	m_bUpdate = false;
	m_nBlackGraceFrames = gEnv->pRenderer->GetFrameID(false) + BLACK_FRAMES;
	// multiplayer music is stopped later on (continues until players spawn in)
	if(m_pMusicSystem && !gEnv->bMultiplayer)
		m_pMusicSystem->SetMood("menu_silence");
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::OnLoadingError(ILevelInfo *pLevel, const char *error)
{
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->IsLoaded())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Unload();
		m_fLastUnloadFrame = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	}

	ShowMainMenu();
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::OnLoadingProgress(ILevelInfo *pLevel, int progressAmount)
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated() || gEnv->pGame->GetIGameFramework()->IsGameStarted() ) 
	{
		return;
	}

	// TODO: seems that OnLoadingProgress can be called *after* OnLoadingComplete ...
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->GetFlashPlayer())
	{
		float fProgress = progressAmount / (float) m_iMaxProgress * 100.0f;
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("SetProgress", fProgress);
	}

	const bool bStandAlone = gEnv->pRenderer->EF_Query(EFQ_RecurseLevel) <= 0;
	if (bStandAlone)
		gEnv->pSystem->RenderBegin();
	OnPostUpdate(0.1f);
	if (bStandAlone)
		gEnv->pSystem->RenderEnd();
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::ShowMainMenu()
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated())
    return;
  m_bUpdate = true;
	SetColorChanged();
	InitStartMenu();
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::LockPlayerInputs(bool bLock)
{
	// In multiplayer, we can't pause the game, so we have to disable the player inputs
	IActionMapManager *pActionMapManager = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
	if(!pActionMapManager)
		return;

	pActionMapManager->Enable(!bLock);
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::ShowInGameMenu(bool bShow)
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return;

	if(m_bUpdate != bShow)
	{
		if(bShow)
		{
			// In the case we press ESC very quickly three times in a row while playing:
			// The menu is created, set in destroy pending event and hidden, then recreated, then destroyed by the pending event at the next OnPostUpdate
			// To avoid that, simply cancel the destroy pending event when creating
			m_bDestroyInGameMenuPending = false;
			if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME])
				m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->SetVisible(true);
			//

			InitIngameMenu();
		}
		else
		{
			DestroyIngameMenu();
		}
	}

	gEnv->pInput->ClearKeyState();

	m_bUpdate = bShow;

	m_pCurrentFlashMenuScreen = m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME];
	if(m_pCurrentFlashMenuScreen && !bShow)
	{
		m_pCurrentFlashMenuScreen->Invoke("Root.MainMenu.StartMenu.gotoAndPlay", "on");
	} 

	if(bShow) //this causes some stalling, but reduces flash memory pool peak by ~ 10MB
	{
		UnloadHUDMovies();
		//stuff is reloaded after "m_DestroyInGameMenuPending" ...
	}

	LockPlayerInputs(m_bUpdate);

  if(!gEnv->bMultiplayer)
	  g_pGame->GetIGameFramework()->PauseGame(m_bUpdate,false);

	SAFE_HUD_FUNC(SetInMenu(m_bUpdate));
	if(bShow)
	{
		SAFE_HUD_FUNC(UpdateHUDElements());
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::HideInGameMenuNextFrame()
{
  if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
	{
		gEnv->pInput->ClearKeyState();

		m_bDestroyInGameMenuPending = true;
		m_bUpdate = false;
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->SetVisible(false);
	  
		LockPlayerInputs(m_bUpdate);

		if(!gEnv->bMultiplayer)
			g_pGame->GetIGameFramework()->PauseGame(m_bUpdate,false);

		SAFE_HUD_FUNC(SetInMenu(m_bUpdate));
		SAFE_HUD_FUNC(UpdateHUDElements());
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::UnloadHUDMovies()
{
  SAFE_HUD_FUNC(UnloadVehicleHUD(true));	//removes vehicle hud to save memory (pool spike)
  SAFE_HUD_FUNC(UnloadSimpleHUDElements(true)); //removes hud elements to save memory (pool spike)
  SAFE_HUD_FUNC(GetMapAnim()->Unload());
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::ReloadHUDMovies()
{
  SAFE_HUD_FUNC(UnloadSimpleHUDElements(false)); //removes hud elements to save memory (pool spike)
  CGameFlashAnimation *mapAnim = SAFE_HUD_FUNC_RET(GetMapAnim());
  if(mapAnim)
  {
    mapAnim->Reload();
    SAFE_HUD_FUNC(GetRadar()->ReloadMiniMap());
    if(mapAnim == SAFE_HUD_FUNC_RET(GetModalHUD()))
    {
      mapAnim->SetVisible(true);
      mapAnim->Invoke("showPDA", gEnv->bMultiplayer);
    }
  }
  SAFE_HUD_FUNC(UnloadVehicleHUD(false));	//removes vehicle hud to save memory (pool spike)
}

//-----------------------------------------------------------------------------------------------------

bool CFlashMenuObject::PlayFlashAnim(const char* pFlashAnim)
{
	SAFE_RELEASE(m_pFlashPlayer);
	if (pFlashAnim && pFlashAnim[0])
	{
		m_pFlashPlayer = gEnv->pSystem->CreateFlashPlayerInstance();
		if (m_pFlashPlayer && m_pFlashPlayer->Load(pFlashAnim))
		{
			// TODO: SetViewport should also be called when we resize the app window to scale flash animation accordingly
			int flashWidth(m_pFlashPlayer->GetWidth());
			int flashHeight(m_pFlashPlayer->GetHeight());

			int screenWidth(gEnv->pRenderer->GetWidth());
			int screenHeight(gEnv->pRenderer->GetHeight());

			float scaleX((float)screenWidth / (float)flashWidth);
			float scaleY((float)screenHeight / (float)flashHeight);

			float scale(scaleY);
			if (scaleY * flashWidth > screenWidth)
				scale = scaleX;

			int w((int)(flashWidth * scale));
			int h((int)(flashHeight * scale));
			int x((screenWidth - w) / 2);
			int y((screenHeight - h) / 2);

			m_pFlashPlayer->SetViewport(x, y, w, h);
			m_pFlashPlayer->SetScissorRect(x, y, w, h);
			m_pFlashPlayer->SetBackgroundAlpha(0);
			// flash player is now initialized, frames are rendered in PostUpdate( ... )
		}
		else
		{
			SAFE_RELEASE(m_pFlashPlayer);
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool CFlashMenuObject::PlayVideo(const char* pVideoFile, bool origUpscaleMode, unsigned int videoOptions)
{
	SAFE_RELEASE(m_pVideoPlayer);
	if (pVideoFile && pVideoFile[0])
	{
		m_pVideoPlayer = gEnv->pRenderer->CreateVideoPlayerInstance();
		if (m_pVideoPlayer && m_pVideoPlayer->Load(pVideoFile, videoOptions))	
		{
			// TODO: SetViewport should also be called when we resize the app window to scale flash animation accordingly
			int videoWidth(m_pVideoPlayer->GetWidth());
			int videoHeight(m_pVideoPlayer->GetHeight());

			int screenWidth(gEnv->pRenderer->GetWidth());
			int screenHeight(gEnv->pRenderer->GetHeight());

			float scaleX((float)screenWidth / (float)videoWidth);
			float scaleY((float)screenHeight / (float)videoHeight);

			float scale(scaleY);

			if (origUpscaleMode)
			{
				if (scaleY * videoWidth > screenWidth)
					scale = scaleX;
			}
			else
			{
				float videoRatio((float)videoWidth / (float)videoHeight);
				float screenRatio((float)screenWidth / (float)screenHeight);

				if (videoRatio < screenRatio)
					scale = scaleX;
			}

			int w((int)(videoWidth * scale));
			int h((int)(videoHeight * scale));
			int x((screenWidth - w) / 2);
			int y((screenHeight - h) / 2);

			m_pVideoPlayer->SetViewport(x, y, w, h);
		}
		else
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::StopVideo()
{
  SAFE_RELEASE(m_pVideoPlayer);
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::NextIntroVideo()
{
	m_stateEntryMovies = (EEntryMovieState)(((int)m_stateEntryMovies) + 1);
	if(m_stateEntryMovies!=eEMS_Stop)
	{
		const char* movie = VALUE_BY_KEY(m_stateEntryMovies, gMovies);
		if(movie)
			PlayVideo(movie, true, IVideoPlayer::DELAY_START);
	}
}


//-----------------------------------------------------------------------------------------------------

bool CFlashMenuObject::IsOnScreen(EMENUSCREEN screen)
{
  if(m_apFlashMenuScreens[screen] && m_pCurrentFlashMenuScreen == m_apFlashMenuScreens[screen])
    return m_bUpdate;
  return false;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent)
{
	if(HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK == eHardwareMouseEvent)
	{
		if(m_pCurrentFlashMenuScreen && m_pCurrentFlashMenuScreen->GetFlashPlayer())
		{
			int x(iX), y(iY);
			m_pCurrentFlashMenuScreen->GetFlashPlayer()->ScreenToClient(x,y);
			SFlashVarValue args[2] = {x,y};
			m_pCurrentFlashMenuScreen->CheckedInvoke("_root.Root.MainMenu.MultiPlayer.DoubleClick",args,2);
			m_pCurrentFlashMenuScreen->CheckedInvoke("DoubleClick",args,2);
		}
	}
	else
	{
		SFlashCursorEvent::ECursorState eCursorState = SFlashCursorEvent::eCursorMoved;
		if(HARDWAREMOUSEEVENT_LBUTTONDOWN == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorPressed;
		}
		else if(HARDWAREMOUSEEVENT_LBUTTONUP == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorReleased;
		}

		if(m_pCurrentFlashMenuScreen && m_pCurrentFlashMenuScreen->GetFlashPlayer())
		{
			int x(iX), y(iY);
			m_pCurrentFlashMenuScreen->GetFlashPlayer()->ScreenToClient(x,y);
			m_pCurrentFlashMenuScreen->GetFlashPlayer()->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
			UpdateButtonSnap(Vec2(x,y));
		}

		if(m_pFlashPlayer)
		{
			int x(iX), y(iY);
			m_pFlashPlayer->ScreenToClient(x,y);
			m_pFlashPlayer->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::UpdateButtonSnap(const Vec2 mouse)
{
	if(!m_avButtonPosition.size()) return;
	ButtonPosMap::iterator bestEst = m_avButtonPosition.end();
	float fBestDist = -1.0f;
	for(ButtonPosMap::iterator it = m_avButtonPosition.begin(); it != m_avButtonPosition.end(); ++it)
	{
		Vec2 pos = it->second;
		float dist = (mouse-pos).GetLength();
		if(dist<200.0f && (fBestDist<0.0f || dist<fBestDist))
		{
			fBestDist = dist;
			bestEst = it;
		}
	}
	if(bestEst != m_avButtonPosition.end())
		m_sCurrentButton = bestEst->first;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SnapToNextButton(const Vec2 dir)
{
	if(!m_bUpdate) return;
	if(!m_avButtonPosition.size()) return;
	ButtonPosMap::iterator current = m_avButtonPosition.find(m_sCurrentButton);
	if(current == m_avButtonPosition.end())
	{
		current = m_avButtonPosition.begin();
	}

	// Used only for 2D navigation mode
	/*if (dir.y > 0.0)
	{
		++current;

		if (current == m_avButtonPosition.end())
			current = m_avButtonPosition.begin();		
	}
	else
	{
		if (current == m_avButtonPosition.begin())
			current = m_avButtonPosition.end();

		--current;
	}

	if (current == m_avButtonPosition.end())
	{
		m_sCurrentButton="";
	}
	else
	{
		m_sCurrentButton=current->first;
		HighlightButton(current);
	}
	*/

	// The original implementation with four directions
	Vec2 curPos = current->second;

	ButtonPosMap::iterator bestEst = m_avButtonPosition.end();
	float fBestValue = -1.0f;
	for(ButtonPosMap::iterator it = m_avButtonPosition.begin(); it != m_avButtonPosition.end(); ++it)
	{
		if(it == current) continue;
		Vec2 btndir = it->second - curPos;
		float dist = btndir.GetLength();
		btndir = btndir.GetNormalizedSafe();
		float arc = dir.Dot(btndir);
		if(arc<=0.01) continue;
		if(fBestValue<0 || dist<fBestValue)
		{
			fBestValue = dist;
			bestEst = it;
			m_sCurrentButton = it->first;
		}
	}

	// Wrap around
	if(bestEst==m_avButtonPosition.end())
	{
		Vec2 round=dir*-1.0f;
		fBestValue = -1.0f;

		for(ButtonPosMap::iterator it = m_avButtonPosition.begin(); it != m_avButtonPosition.end(); ++it)
		{
			if(it == current) continue;
			Vec2 btndir = it->second - curPos;
			float dist = btndir.GetLength();
			btndir = btndir.GetNormalizedSafe();
			float arc = round.Dot(btndir);
			if(arc<=0.01) continue;
			if(dist>fBestValue)
			{
				fBestValue = dist;
				bestEst = it;
				m_sCurrentButton = it->first;
			}
		}
	}

/*	if(bestEst==m_avButtonPosition.end())
	{
		fBestValue = -1.0f;
		for(ButtonPosMap::iterator it = m_avButtonPosition.begin(); it != m_avButtonPosition.end(); ++it)
		{
			if(it == current) continue;
			Vec2 btndir = it->second - curPos;
			float dist = btndir.GetLength();
			btndir = btndir.GetNormalizedSafe();
			float arc = dir.Dot(btndir);
			if(arc<=0.0) continue;
			if(fBestValue<0 || dist<fBestValue)
			{
				fBestValue = dist;
				bestEst = it;
				m_sCurrentButton = it->first;
			}
		}
	}
*/
	if(bestEst!=m_avButtonPosition.end())
		HighlightButton(bestEst);
	else if(current!=m_avButtonPosition.end())
		HighlightButton(current);
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::GetButtonClientPos(ButtonPosMap::iterator button, Vec2 &pos)
{
	IRenderer *pRenderer = gEnv->pRenderer;

	
	float fMovieRatio		=	((float)m_pCurrentFlashMenuScreen->GetFlashPlayer()->GetWidth()) / ((float)m_pCurrentFlashMenuScreen->GetFlashPlayer()->GetHeight());
	float fRenderRatio	=	pRenderer->GetWidth() / pRenderer->GetHeight();

	float fWidth				=	pRenderer->GetWidth();
	float fHeight				=	pRenderer->GetHeight();
	float fXPos = 0.0f;

	if(fRenderRatio != fMovieRatio)
	{
		fXPos = ( fWidth - (fHeight * fMovieRatio) ) * 0.5;
		fWidth = fHeight * fMovieRatio;
	}

	pos = button->second;
	pos.x+=fXPos;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::HighlightButton(ButtonPosMap::iterator button)
{
	Vec2 pos;
	GetButtonClientPos(button, pos);
	
	SAFE_HARDWARE_MOUSE_FUNC(SetHardwareMouseClientPosition(pos.x, pos.y));
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::PushButton(ButtonPosMap::iterator button, bool press, bool force)
{
	if (button == m_avButtonPosition.end())
		return;

	Vec2 pos;
	GetButtonClientPos(button, pos);
	
	if (!force)
	{
		SAFE_HARDWARE_MOUSE_FUNC(Event((int)pos.x, (int)pos.y, press ? HARDWAREMOUSEEVENT_LBUTTONDOWN : HARDWAREMOUSEEVENT_LBUTTONUP));
	}
	else if (!press)
	{
		string method=button->first;
		method.append(".pressButton");
		m_pCurrentFlashMenuScreen->GetFlashPlayer()->Invoke0(method);
	}
}

//-----------------------------------------------------------------------------------------------------

CFlashMenuObject::ButtonPosMap::iterator CFlashMenuObject::FindButton(const TKeyName &shortcut)
{
	if(m_avButtonPosition.empty())
		return m_avButtonPosition.end();

	// FIXME: Try to find a more elgant way to identify shortcuts
	string sc;
	if (shortcut == "xi_a")
		sc="_a";
	else if (shortcut == "xi_b")
		sc="_b";
	else if (shortcut == "xi_x")
		sc="_x";
	else if (shortcut == "xi_y")
		sc="_y";
	else
		return m_avButtonPosition.end();


	for(ButtonPosMap::iterator it = m_avButtonPosition.begin(); it != m_avButtonPosition.end(); ++it)
	{
		if (it->first.substr(it->first.length()-sc.length(), sc.length()) == sc)
			return it;
	}

	return m_avButtonPosition.end();
}

//-----------------------------------------------------------------------------------------------------
//ALPHA HACK
void CFlashMenuObject::UpdateLevels(const char* gamemode)
{
	m_pCurrentFlashMenuScreen->Invoke("resetMultiplayerLevel");
	ILevelSystem *pLevelSystem = gEnv->pGame->GetIGameFramework()->GetILevelSystem();
	if(pLevelSystem)
	{
    SFlashVarValue args[2] = {"","Any"};
    m_pCurrentFlashMenuScreen->Invoke("addMultiplayerLevel", args, 2);
		for(int l = 0; l < pLevelSystem->GetLevelCount(); ++l)
		{
			ILevelInfo *pLevelInfo = pLevelSystem->GetLevelInfo(l);
			if(pLevelInfo && pLevelInfo->SupportsGameType(gamemode))
			{
        string disp(pLevelInfo->GetDisplayName());
        SFlashVarValue args[2] = {disp.empty()?pLevelInfo->GetName():disp,disp.empty()?pLevelInfo->GetName():disp};
				m_pCurrentFlashMenuScreen->Invoke("addMultiplayerLevel", args, 2);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::HandleFSCommand(const char *strCommand,const char *strArgs)
{
  if(g_pGameCVars->g_debug_fscommand)
    CryLog("HandleFSCommand : %s %s\n", strCommand, strArgs);

	if(g_pGame->GetOptions()->HandleFSCommand(strCommand, strArgs))
		return;

	if(!stricmp(strCommand, "menu_highlight"))
	{
		PlaySound(ESound_MenuHighlight);
	}
	else if(!stricmp(strCommand, "menu_select"))
	{
		PlaySound(ESound_MenuSelect);
	}
	else if(!stricmp(strCommand, "menu_winopen"))
	{
		PlaySound(ESound_MenuOpen);
	}
	else if(!stricmp(strCommand, "menu_winclose"))
	{
		PlaySound(ESound_MenuClose);
	}
	else if(!strcmp(strCommand, "UpdateLevels"))
	{
		string mode = strArgs;
		if(mode.length()>0)
			UpdateLevels(strArgs);
		else
		{
			const char* defaultMode = g_pGameCVars->g_quickGame_mode->GetString();
			UpdateLevels(defaultMode);
		}
	}
	else if(!strcmp(strCommand,"LayerCallBack"))
	{
		if(!strcmp(strArgs,"Clear"))
		{
			m_avButtonPosition.clear();
		}
		else if(!strcmp(strArgs,"AddOnTop"))
		{
			m_avLastButtonPosition = m_avButtonPosition;
			m_avButtonPosition.clear();
		}
		else if(!strcmp(strArgs,"RemoveFromTop"))
		{
			m_avButtonPosition = m_avLastButtonPosition;
			m_avLastButtonPosition.clear();
		}
	}
	else if(!strcmp(strCommand,"BtnCallBack"))
	{
		string sTemp(strArgs);
		int iSep = sTemp.find("|");
		string sName = sTemp.substr(0,iSep);
		sTemp = sTemp.substr(iSep+1,sTemp.length());
		iSep = sTemp.find("|");
		string sX = sTemp.substr(0,iSep);
		string sY = sTemp.substr(iSep+1,sTemp.length());
		m_avButtonPosition.insert(ButtonPosMap::iterator::value_type(sName, Vec2(atoi(sX), atoi(sY))));

		//if (m_iGamepadsConnected && sName.substr(sName.length()-1, 1) == "1")
			//HighlightButton(m_avButtonPosition.find(sName));
	}
	else if(!strcmp(strCommand,"VirtualKeyboard"))
	{
		m_bVirtualKeyboardFocus = !(strcmp(strArgs,"On") && strcmp(strArgs,"on"));
	}
	else if(!strcmp(strCommand,"Back"))
	{
    /*if(m_QuickGame)
    {
      gEnv->pConsole->ExecuteString("g_quickGameStop");
      m_QuickGame = false;
    }*/
    if(m_multiplayerMenu)
      m_multiplayerMenu->HandleFSCommand(strCommand,strArgs);
    
    m_pCurrentFlashMenuScreen = m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART];
		if(m_pMusicSystem)
		{
			m_pMusicSystem->SetMood("menu_music");
			m_fMusicFirstTime = -1.0f;
		}
	}
	else if(!strcmp(strCommand,"Resume"))
	{
		m_bUpdate = !m_bUpdate;
		if(m_bUpdate)
      ShowInGameMenu(m_bUpdate);
    else
      HideInGameMenuNextFrame();

		if(IActor *pPlayer = g_pGame->GetIGameFramework()->GetClientActor())
		{
			string lastSaveGame = g_pGame->GetLastSaveGame();
			if(pPlayer->GetHealth() <= 0 && lastSaveGame.size())
			{
				g_pGame->GetHUD()->DisplayFlashMessage("", 2);	//removing warning / loading text
				m_sLoadSave.save = false;
				m_sLoadSave.name = lastSaveGame;
			}
		}
	}
	else if(!strcmp(strCommand,"Restart"))
	{
		gEnv->pGame->InitMapReloading();
		HideInGameMenuNextFrame();
	}
	else if(!strcmp(strCommand,"Return"))
	{
		if(INetChannel* pCh = g_pGame->GetIGameFramework()->GetClientChannel())
      pCh->Disconnect(eDC_UserRequested,"User left the game");
    g_pGame->GetIGameFramework()->EndGameContext();
    HideInGameMenuNextFrame();
	}
	else if(!strcmp(strCommand,"Quit"))
	{
		gEnv->pSystem->Quit();
	}
	else if(!strcmp(strCommand,"RollOver"))
	{
		PlaySound(ESound_RollOver);
	}
	else if(!strcmp(strCommand,"Click"))
	{
		PlaySound(ESound_Click1);
	}
	else if(!strcmp(strCommand,"ScreenChange"))
	{
		PlaySound(ESound_ScreenChange);
	}
	else if(!strcmp(strCommand,"AddProfile"))
	{
		AddProfile(strArgs);
	}
	else if(!strcmp(strCommand,"SelectProfile"))
	{
		SelectProfile(strArgs);
	}
	else if(!strcmp(strCommand,"DeleteProfile"))
	{
		if(strArgs)
			DeleteProfile(strArgs);
	}
	else if(!strcmp(strCommand, "SetDifficulty"))
	{
		if(strArgs)
			SetDifficulty(atoi(strArgs));
	}
	else if(!strcmp(strCommand, "EnableVoiceChat"))
	{
		if(!stricmp(strArgs, "on"))
			gEnv->pConsole->ExecuteString("net_enable_voice_chat 1");
		else
			gEnv->pConsole->ExecuteString("net_enable_voice_chat 0");
	}
	else if(!strcmp(strCommand, "Shoulder"))
	{
		if(!strcmp(strArgs, "On"))
			gEnv->pConsole->ExecuteString("g_enableAlternateIronSight 1");
		else
			gEnv->pConsole->ExecuteString("g_enableAlternateIronSight 0");
	}
	else if(!strcmp(strCommand,"UpdateSingleplayerDifficulties"))
	{
		UpdateSingleplayerDifficulties();
	}
	else if(!strcmp(strCommand,"StartSingleplayerGame"))
	{
		StartSingleplayerGame(strArgs);
	}
	else if(!strcmp(strCommand,"LoadGame"))
	{
		m_sLoadSave.save = false;
		m_sLoadSave.name = strArgs;
    HideInGameMenuNextFrame();
	}
	else if(!strcmp(strCommand,"DeleteSaveGame"))
	{
		if(strArgs)
			DeleteSaveGame(strArgs);
	}
	else if(!strcmp(strCommand,"SaveGame"))
	{
		m_sLoadSave.save = true;
		m_sLoadSave.name = strArgs;
	}
	else if(!strcmp(strCommand,"UpdateHUD"))
	{
		SetColorChanged();
		UpdateMenuColor();
	}
	/*else if(!stricmp(strCommand, "StartServer"))
	{
		string command("map ");
		command.append(strArgs);
		command.append(" s");
		gEnv->pConsole->ExecuteString(command.c_str());
	}*/
	else if(!strcmp(strCommand,"CatchNextInput"))
	{
		string sTemp(strArgs);
		int iSep = sTemp.find("//");
		string s1 = sTemp.substr(0,iSep);
		string s2 = sTemp.substr(iSep+2,sTemp.length());
		m_bCatchNextInput = true;
		m_sActionToCatch = s2;
		m_sActionMapToCatch = s1;
	}
	else if(!strcmp(strCommand,"UpdateActionMap"))
	{
		m_catchGamePad = !strcmp(strArgs,"Pad");
		UpdateKeyMenu();
	}
	else if(!strcmp(strCommand,"UpdateSaveGames"))
	{
		UpdateSaveGames();
	}
	else if(!strcmp(strCommand,"SetCVar"))
	{
		string sTemp(strArgs);
		int iSep = sTemp.find("//");
		string s1 = sTemp.substr(0,iSep);
		string s2 = sTemp.substr(iSep+2,sTemp.length());
		SetCVar(s1, s2);
	}
	else if(!strcmp(strCommand,"TabStopPutMouse"))
	{
/*		string sTemp(strArgs);
		int iSep = sTemp.find("//");
		string s1 = sTemp.substr(0,iSep);
		string s2 = sTemp.substr(iSep+2,sTemp.length());

		float x;
		float y;

		TFlowInputData data = (TFlowInputData)s1;
		data.GetValueWithConversion(x);
		data = (TFlowInputData)s2;
		data.GetValueWithConversion(y);

		CryLogAlways("%f %f",x,y);
		CryLogAlways("%f %f",m_fCursorX,m_fCursorY);

		UpdateMousePosition();
*/
	}
	else if(!strcmp(strCommand,"SetKey"))
	{
		string sTmp(strArgs);
		int iSep = sTmp.find("//");
		string s1 = sTmp.substr(0,iSep);
		sTmp = sTmp.substr(iSep+2,sTmp.length());
		iSep = sTmp.find("//");
		string s2 = sTmp.substr(0,iSep);
		string s3 = sTmp.substr(iSep+2,sTmp.length());
		SaveActionToMap(s1, s2, s3);
	}
	else if(!strcmp(strCommand,"SetProfileValue"))
	{
		string sTemp(strArgs);
		int iSep = sTemp.find("//");
		string s1 = sTemp.substr(0,iSep);
		string s2 = sTemp.substr(iSep+2,sTemp.length());
		SetProfileValue(s1, s2);
	
		if(!strcmp(s1,"ColorLine"))
		{
						if(!strcmp(s2,CRYSIS_PROFILE_COLOR_AMBER))	m_eCrysisProfileColor = CrysisProfileColor_Amber;
			else	if(!strcmp(s2,CRYSIS_PROFILE_COLOR_BLUE))		m_eCrysisProfileColor = CrysisProfileColor_Blue;
			else	if(!strcmp(s2,CRYSIS_PROFILE_COLOR_GREEN))	m_eCrysisProfileColor = CrysisProfileColor_Green;
			else	if(!strcmp(s2,CRYSIS_PROFILE_COLOR_RED))		m_eCrysisProfileColor = CrysisProfileColor_Red;
			else	if(!strcmp(s2,CRYSIS_PROFILE_COLOR_WHITE))	m_eCrysisProfileColor = CrysisProfileColor_White;
			else CRY_ASSERT(0);
		}
	}
	else if(!strcmp(strCommand,"RestoreDefaults"))
	{
		RestoreDefaults();
	}
  else if(m_multiplayerMenu && m_multiplayerMenu->HandleFSCommand(strCommand,strArgs))
  {
    //handled by Multiplayer menu
  }
  else 
	{
		// Dev mode: we clicked on a button which should execute something like "map MapName"
		gEnv->pConsole->ExecuteString(strCommand);
	}
}

//-----------------------------------------------------------------------------------------------------

bool CFlashMenuObject::Load()
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return true;

	m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING] = new CFlashMenuScreen;

	if(g_pGameCVars->g_skipIntro==1)
	{
		m_stateEntryMovies = eEMS_Stop;
	}
	InitStartMenu();

	return true;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::InitStartMenu()
{
	if(!m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART])
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART] = new CFlashMenuScreen;

	if(m_stateEntryMovies==eEMS_Done)
		m_stateEntryMovies = eEMS_Stop;

	if(!m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->IsLoaded())
	{
		SAFE_HARDWARE_MOUSE_FUNC(IncrementCounter());

		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Load("Libs/UI/Menus_StartMenu.gfx");
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->GetFlashPlayer()->SetFSCommandHandler(this);
		// not working yet, gets reset on loadMovie within .swf/.gfx
		// m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->GetFlashPlayer()->SetLoadMovieHandler(this);

		if(g_pGameCVars->g_debugDirectMPMenu)
		{
			m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->SetVariable("MainWindow",2);
			m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->SetVariable("SubWindow",2);
		}

		char strProductVersion[256];
		gEnv->pSystem->GetProductVersion().ToString(strProductVersion);
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("setGameVersion", strProductVersion);
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->SetVariable("GamepadAvailableVar",m_iGamepadsConnected?1:0);

		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("Directx10", (gEnv->pRenderer->GetRenderType() == eRT_DX10)?true:false);
		
		if(!gEnv->pSystem->IsEditor())
			UpdateMenuColor();
	}

//  m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->SetVariable("Authorized","1");
  /*m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->SetVariable("MainWindow","2");
	m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->SetVariable("SubWindow","2");*/

	m_pCurrentFlashMenuScreen = m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART];
  if(m_multiplayerMenu)
  {
    m_multiplayerMenu->SetCurrentFlashScreen(m_pCurrentFlashMenuScreen->GetFlashPlayer(),false);
  }
	m_bIgnoreEsc = true;

	if(g_pGame->GetOptions())
		g_pGame->GetOptions()->UpdateFlashOptions();
	SetDisplayFormats();
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::DestroyStartMenu()
{
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->IsLoaded())
	{
    if(m_multiplayerMenu)
      m_multiplayerMenu->SetCurrentFlashScreen(0,false);
		SAFE_HARDWARE_MOUSE_FUNC(DecrementCounter());
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Unload();
		m_fLastUnloadFrame = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	}

	m_bIgnoreEsc = false;
	m_bDestroyStartMenuPending = false;
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::InitIngameMenu()
{
	if(!m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME])
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] = new CFlashMenuScreen;

	if(!m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
	{
		SAFE_HARDWARE_MOUSE_FUNC(IncrementCounter());

		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Load("Libs/UI/Menus_IngameMenu.gfx");
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->GetFlashPlayer()->SetFSCommandHandler(this);
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("setLineColor", SFlashVarValue(g_pGameCVars->hud_colorLine));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("setOverColor", SFlashVarValue(g_pGameCVars->hud_colorOver));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("setTextColor", SFlashVarValue(g_pGameCVars->hud_colorText));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->SetVariable("GamepadAvailableVar",m_iGamepadsConnected?true:false);
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("setGameMode", gEnv->bMultiplayer?"MP":"SP");

		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("Directx10", (gEnv->pRenderer->GetRenderType() == eRT_DX10)?true:false);
	}
	m_pCurrentFlashMenuScreen = m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME];
	if(m_multiplayerMenu)
	{
		m_multiplayerMenu->SetCurrentFlashScreen(m_pCurrentFlashMenuScreen->GetFlashPlayer(),true);
	}

	g_pGame->GetOptions()->UpdateFlashOptions();
	SetDisplayFormats();
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::DestroyIngameMenu()
{
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
	{
    if(m_multiplayerMenu)
      m_multiplayerMenu->SetCurrentFlashScreen(0,true);
		SAFE_HARDWARE_MOUSE_FUNC(DecrementCounter());
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Unload();
		m_fLastUnloadFrame = gEnv->pTimer->GetFrameStartTime().GetSeconds();
	}
  if(g_pGame->GetIGameFramework()->IsGameStarted())
    ReloadHUDMovies();
}

//-----------------------------------------------------------------------------------------------------

CMPHub* CFlashMenuObject::GetMPHub()const
{
  return m_multiplayerMenu;
}

//-----------------------------------------------------------------------------------------------------

CFlashMenuScreen *CFlashMenuObject::GetMenuScreen(EMENUSCREEN screen) const 
{ 
	return m_apFlashMenuScreens[screen]; 
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::UpdateMenuColor()
{
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->IsLoaded())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->CheckedInvoke("setLineColor", SFlashVarValue(g_pGameCVars->hud_colorLine));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("setOverColor", SFlashVarValue(g_pGameCVars->hud_colorOver));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDSTART]->Invoke("setTextColor", SFlashVarValue(g_pGameCVars->hud_colorText));
	}
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->IsLoaded())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->CheckedInvoke("setLineColor", SFlashVarValue(g_pGameCVars->hud_colorLine));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("setOverColor", SFlashVarValue(g_pGameCVars->hud_colorOver));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("setTextColor", SFlashVarValue(g_pGameCVars->hud_colorText));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->Invoke("resetColor");
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDLOADING]->GetFlashPlayer()->Advance(0.2f);
	}
	if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
	{
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->CheckedInvoke("setLineColor", SFlashVarValue(g_pGameCVars->hud_colorLine));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("setOverColor", SFlashVarValue(g_pGameCVars->hud_colorOver));
		m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("setTextColor", SFlashVarValue(g_pGameCVars->hud_colorText));
	}
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::OnPostUpdate(float fDeltaTime)
{
	if(gEnv->pSystem->IsEditor() || gEnv->pSystem->IsDedicated()) return;

	if(m_bDestroyInGameMenuPending)
	{
		m_bUpdate = false; 
		DestroyIngameMenu();
		m_bDestroyInGameMenuPending = false;
	}

	if(m_bDestroyStartMenuPending)
		DestroyStartMenu();

	int width = gEnv->pRenderer->GetWidth();
	int height = gEnv->pRenderer->GetHeight() ;
	if(m_iWidth!=width || m_iHeight!=height)
	{
		UpdateRatio();
	}

	if(m_stateEntryMovies!=eEMS_Done)
	{
		if(m_stateEntryMovies!=eEMS_Stop && m_stateEntryMovies!=eEMS_GameStop && m_stateEntryMovies!=eEMS_GameDone)
		{
			float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			if(m_pVideoPlayer==NULL)
			{
				NextIntroVideo();
				return;
			}
			IVideoPlayer::EPlaybackStatus status = m_pVideoPlayer->GetStatus();
			if(status == IVideoPlayer::PBS_ERROR || status == IVideoPlayer::PBS_FINISHED)
			{
				StopVideo();
				ColorF cBlack(Col_Black);
				gEnv->pRenderer->ClearBuffer(FRT_CLEAR | FRT_CLEAR_IMMEDIATE,&cBlack);
				return;
			}
			if(m_pVideoPlayer)
			{
				m_pVideoPlayer->Render();
				return;
			}
			else
			{
				m_stateEntryMovies = eEMS_Stop;
			}
		}
	}
		
	if(m_stateEntryMovies==eEMS_Stop)
	{
		m_stateEntryMovies = eEMS_Done;
		const char* movie = VALUE_BY_KEY(m_stateEntryMovies, gMovies);
		if(movie)
			PlayVideo(movie, false, IVideoPlayer::LOOP_PLAYBACK);
	}

	if(m_stateEntryMovies==eEMS_GameStop)
	{
		// map load
		m_stateEntryMovies = eEMS_GameDone;
		m_bUpdate = false;

		gEnv->pConsole->ExecuteString("map island nonblocking");

		return;
	}

	if(m_stateEntryMovies==eEMS_GameDone)
	{
		//game intro video was shown and laoding is pending
		return;
	}

  if(!IsOnScreen(MENUSCREEN_FRONTENDINGAME) && !IsOnScreen(MENUSCREEN_FRONTENDSTART))
  {
    if(!g_pGame->GetIGameFramework()->IsGameStarted() && !g_pGame->GetIGameFramework()->GetClientChannel())
    {
      g_pGame->DestroyHUD();
      ShowMainMenu();
    }
  }

	if(m_sLoadSave.name.size())
	{
		string temp = m_sLoadSave.name;
		m_sLoadSave.name.resize(0);
		if(m_sLoadSave.save)
			SaveGame(temp.c_str());
			if(m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME] && m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->IsLoaded())
			{
				m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME]->Invoke("showStatusMessage", SFlashVarValue("@ui_GAMESAVEDSUCCESSFUL"));
			}

			
		else
			LoadGame(temp.c_str());
	}

	int curFrameID = gEnv->pRenderer->GetFrameID(false);
	// if (curFrameID == m_nLastFrameUpdateID)
	// 	return;
	m_nLastFrameUpdateID = curFrameID;

	if (m_nBlackGraceFrames > 0)
	{
/*		IUIDraw *pUIDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();

		pUIDraw->PreRender();
		//const uchar *pImage=m_pAVIReader->QueryFrame();
		pUIDraw->DrawImage(*pImage,0,0,800,600,0,1,1,1,1);
		pUIDraw->PostRender();
*/
		ColorF cBlack(Col_Black);
		gEnv->pRenderer->ClearBuffer(FRT_CLEAR | FRT_CLEAR_IMMEDIATE,&cBlack);

		if (curFrameID >= m_nBlackGraceFrames)
			m_nBlackGraceFrames = 0;
	}

	if(NULL == m_pCurrentFlashMenuScreen || !m_pCurrentFlashMenuScreen->IsLoaded())
		return;

	if(m_fLastUnloadFrame == gEnv->pTimer->GetFrameStartTime().GetSeconds())
		return;

	if(m_pMusicSystem && m_fMusicFirstTime != -1.0f)
	{
		if(gEnv->pTimer->GetAsyncTime().GetSeconds() >= m_fMusicFirstTime+78.0f)
		{
			m_pMusicSystem->SetMood("menu_music");
			m_fMusicFirstTime = -1.0f;
		}
	}
		
	if(!m_sScreenshot.empty())
	{
		if(m_bTakeScreenshot)
		{
			gEnv->pRenderer->ScreenShot(m_sScreenshot,200);
			m_sScreenshot.erase(0);
			m_bTakeScreenshot = false;
		}
		else
		{
			m_bTakeScreenshot = true;
		}
		return;
	}

	if(m_bUpdate)
	{
		if(m_pCurrentFlashMenuScreen != m_apFlashMenuScreens[MENUSCREEN_FRONTENDINGAME])
		{
			if(m_pVideoPlayer)
			{
				m_pVideoPlayer->Render();
			}
		}

		if(m_pCurrentFlashMenuScreen && m_pCurrentFlashMenuScreen->IsLoaded())
		{
			m_pCurrentFlashMenuScreen->GetFlashPlayer()->Advance(fDeltaTime);
			m_pCurrentFlashMenuScreen->GetFlashPlayer()->Render();
		}
	}
	else if (m_nBlackGraceFrames > 0)
	{
		ColorF cBlack(Col_Black);
		gEnv->pRenderer->ClearBuffer(FRT_CLEAR | FRT_CLEAR_IMMEDIATE,&cBlack);
		if (curFrameID >= m_nBlackGraceFrames)
			m_nBlackGraceFrames = 0;
	}

	if(m_pFlashPlayer)
	{
		m_pFlashPlayer->Advance(fDeltaTime);
		m_pFlashPlayer->Render();
	}

	// TODO: just some quick code from Craig to get connection state displayed; please move to a suitable location later
	/*IGameFramework * pGFW = g_pGame->GetIGameFramework();
	if (INetChannel * pNC = pGFW->GetClientChannel())
	{
		bool show = true;
		char status[512];
		switch (pNC->GetChannelConnectionState())
		{
		case eCCS_StartingConnection:
			strcpy(status, "Waiting for server");
			break;
		case eCCS_InContextInitiation:
			{
				const char * state = "<unknown state>";
				switch (pNC->GetContextViewState())
				{
				case eCVS_Initial:
					state = "Requesting Game Environment";
					break;
				case eCVS_Begin:
					state = "Receiving Game Environment";
					break;
				case eCVS_EstablishContext:
					state = "Loading Game Assets";
					break;
				case eCVS_ConfigureContext:
					state = "Configuring Game Settings";
					break;
				case eCVS_SpawnEntities:
					state = "Spawning Entities";
					break;
				case eCVS_PostSpawnEntities:
					state = "Initializing Entities";
					break;
				case eCVS_InGame:
					state = "In Game";
					break;
				}
				sprintf(status, "%s [%d]", state, pNC->GetContextViewStateDebugCode());
			}
			break;
		case eCCS_InGame:
			show = false;
			strcpy(status, "In Game");
			break;
		case eCCS_Disconnecting:
			strcpy(status, "Disconnecting");
			break;
		default:
			strcpy(status, "Unknown State");
			break;
		}

		float white[] = {1,1,1,1};
		if (show)
			gEnv->pRenderer->Draw2dLabel( 10, 750, 1, white, false, "Connection State: %s", status );
	}*/
}

//-----------------------------------------------------------------------------------------------------

void CFlashMenuObject::SetDisplayFormats()
{
	if(!m_pCurrentFlashMenuScreen)
		return;

	m_pCurrentFlashMenuScreen->Invoke("clearResolutionList");

	SDispFormat *formats = NULL;
	int numFormats = gEnv->pRenderer->EnumDisplayFormats(NULL);
	if(numFormats)
	{
		formats = new SDispFormat[numFormats];
		gEnv->pRenderer->EnumDisplayFormats(formats);
	}

	int lastWidth, lastHeight;
	lastHeight = lastWidth = -1;

	char buffer[64];
	for(int i = 0; i < numFormats; ++i)
	{
		if(lastWidth == formats[i].m_Width && lastHeight == formats[i].m_Height) //only pixel resolution, don't care about color
			continue;

		itoa(formats[i].m_Width, buffer, 10);
		string command(buffer);
		command.append("x");
		itoa(formats[i].m_Height, buffer, 10);
		command.append(buffer);
		m_pCurrentFlashMenuScreen->Invoke("addResolution", command.c_str());

		lastHeight = formats[i].m_Height;
		lastWidth = formats[i].m_Width;
	}

	if(formats)
		delete[] formats;
}

//-----------------------------------------------------------------------------------------------------

class CFlashLoadMovieImage : public IFlashLoadMovieImage
{
public:
	CFlashLoadMovieImage(ISaveGameThumbailPtr pThumbnail) : m_pThumbnail(pThumbnail)
	{
		SwapRB();
	}

	virtual void Release()
	{
		delete this;
	}

	virtual int GetWidth() const 
	{
		return m_pThumbnail->GetWidth();
	}

	virtual int GetHeight() const 
	{
		return m_pThumbnail->GetHeight();
	}

	virtual int GetPitch() const 
	{
		return m_pThumbnail->GetWidth() * m_pThumbnail->GetDepth();
	}

	virtual void* GetPtr() const 
	{
		return (void*) m_pThumbnail->GetImageData();
	}

	// Scaleform Textures must be RGB / RGBA.
	// thumbnails are BGR/BGRA -> swap necessary
	virtual EFmt GetFormat() const
	{
		const int depth = m_pThumbnail->GetDepth();
		if (depth == 3)
			return eFmt_RGB_888;
		if (depth == 4)
			return eFmt_ARGB_8888;
		return eFmt_None;
	}

	void SwapRB()
	{
		if (m_pThumbnail)
		{
			// Scaleform Textures must be RGB / RGBA.
			// thumbnails are BGR/BGRA -> swap necessary
			const int depth = m_pThumbnail->GetDepth();
			const int height = m_pThumbnail->GetHeight();
			const int width = m_pThumbnail->GetWidth();
			const int bpl = depth * width;
			if (depth == 3 || depth == 4)
			{
				uint8* pData = const_cast<uint8*> (m_pThumbnail->GetImageData());
				for (int y = 0; y < height; ++y)
				{
					uint8* pCol = pData + bpl * y;
					for (int x = 0; x< width; ++x, pCol+=depth)
					{
						std::swap(pCol[0], pCol[2]);
					}
				}
			}
		}
	}

	ISaveGameThumbailPtr m_pThumbnail;
};


IFlashLoadMovieImage* CFlashMenuObject::LoadMovie(const char* pFilePath)
{
	bool bResolved = false;
	if (stricmp (PathUtil::GetExt(pFilePath), "thumbnail") == 0)
	{
		string saveGameName = pFilePath;
		PathUtil::RemoveExtension(saveGameName);

		if (m_pPlayerProfileManager == 0)
			return 0;

		IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
		if (pProfile == 0)
			return 0;

		ISaveGameEnumeratorPtr pSGE = pProfile->CreateSaveGameEnumerator();
		if (pSGE == 0)
			return 0;
		ISaveGameThumbailPtr pThumbnail = pSGE->GetThumbnail(saveGameName);
		if (pThumbnail == 0)
			return 0;
		return new CFlashLoadMovieImage(pThumbnail);
	}
	return 0;
}

static const int  THUMBNAIL_DEFAULT_WIDTH  = 256;   // 16:9 
static const int  THUMBNAIL_DEFAULT_HEIGHT = 144;   //
static const int  THUMBNAIL_DEFAULT_DEPTH = 4;   // write out with alpha
static const bool THUMBNAIL_KEEP_ASPECT_RATIO = true; // keep renderer's aspect ratio and surround with black borders

void CFlashMenuObject::OnSaveGame(ISaveGame* pSaveGame)
{
	int w = gEnv->pRenderer->GetWidth();
	int h = gEnv->pRenderer->GetHeight();

	const int imageDepth = THUMBNAIL_DEFAULT_DEPTH;
	int imageHeight = std::min(THUMBNAIL_DEFAULT_HEIGHT, h);
	int imageWidth = imageHeight * 16 / 9;
	// this initializes the data as well (fills with zero currently)
	uint8* pData = pSaveGame->SetThumbnail(0,imageWidth,imageHeight,imageDepth);

	// no thumbnail supported
	if (pData == 0)
		return;

	// initialize to stretch thumbnail
	int captureDestWidth  = imageWidth;
	int captureDestHeight = imageHeight;
	int captureDestOffX   = 0;
	int captureDestOffY   = 0;

	const bool bKeepAspectRatio = THUMBNAIL_KEEP_ASPECT_RATIO;

	// should we keep the aspect ratio of the renderer?
	if (bKeepAspectRatio)
	{
		captureDestHeight = imageHeight;
		captureDestWidth  = captureDestHeight * w / h;

		// adjust for SCOPE formats, like 2.35:1 
		if (captureDestWidth > THUMBNAIL_DEFAULT_WIDTH)
		{
			captureDestHeight = captureDestHeight * THUMBNAIL_DEFAULT_WIDTH / captureDestWidth;
			captureDestWidth  = THUMBNAIL_DEFAULT_WIDTH;
		}

		captureDestOffX = (imageWidth  - captureDestWidth) >> 1;
		captureDestOffY = (imageHeight - captureDestHeight) >> 1;

		// CryLogAlways("CXMLRichSaveGame: TEST_THUMBNAIL_AUTOCAPTURE: capWidth=%d capHeight=%d (off=%d,%d) thmbw=%d thmbh=%d rw=%d rh=%d", 
		//	captureDestWidth, captureDestHeight, captureDestOffX, captureDestOffY, m_thumbnailWidth, m_thumbnailHeight, w,h);

		if (captureDestWidth > imageWidth || captureDestHeight > imageHeight)
		{
			assert (false);
			GameWarning("CFlashMenuObject::OnSaveGame: capWidth=%d capHeight=%d", captureDestWidth, captureDestHeight);
			captureDestHeight = imageHeight;
			captureDestWidth = imageWidth;
			captureDestOffX = captureDestOffY = 0;
		}
	}

	const bool bAlpha = imageDepth == 4;
	const int bpl = imageWidth * imageDepth;
	uint8* pBuf = pData + captureDestOffY * bpl + captureDestOffX * imageDepth;
	gEnv->pRenderer->ReadFrameBuffer(pBuf, imageWidth, w, h, eRB_BackBuffer, bAlpha, captureDestWidth, captureDestHeight); // no inverse needed
}

void CFlashMenuObject::OnActionEvent(const SActionEvent& event)
{
  if(m_multiplayerMenu)
  {
    switch(event.m_event)
    {
      case eAE_connectFailed:
        m_multiplayerMenu->OnUIEvent(SUIEvent(eUIE_connectFailed,event.m_value,event.m_description));
        break;
      case eAE_disconnected:
        //TODO: Show start menu if game not started
        m_multiplayerMenu->OnUIEvent(SUIEvent(eUIE_disconnnect,event.m_value,event.m_description));
        break;
      case eAE_channelCreated:
        m_multiplayerMenu->OnUIEvent(SUIEvent(eUIE_connect));
        break;
    }
  }
}

void CFlashMenuObject::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);

	for(int i=0; i<MENUSCREEN_COUNT; i++)
	{
		if(m_apFlashMenuScreens[i])
		{
			m_apFlashMenuScreens[i]->GetMemoryStatistics(s);
		}
	}
}

void CFlashMenuObject::SetDifficulty(int level)
{
	if(m_pPlayerProfileManager)
	{
		IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
		if(pProfile)
		{
			int iDiff = level;

			if(level > 0 && level < 5)
				pProfile->SetAttribute("Singleplayer.LastSelectedDifficulty", level);
			else
			{
				TFlowInputData data;
				pProfile->GetAttribute("Singleplayer.LastSelectedDifficulty", data, false);
				data.GetValueWithConversion(iDiff);
			}

			g_pGameCVars->g_difficultyLevel = iDiff;
			switch(iDiff)
			{
			case 1:
				gEnv->pConsole->ExecuteString("exec diff_easy");
				break;
			case 3:
				gEnv->pConsole->ExecuteString("exec diff_hard");
				break;
			case 4:
				gEnv->pConsole->ExecuteString("exec diff_bauer");
				break;
			default:
				gEnv->pConsole->ExecuteString("exec diff_normal");
				break;
			}
		}
	}
}
