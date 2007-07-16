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
#ifndef __FLASHMENUOBJECT_H__
#define __FLASHMENUOBJECT_H__

//-----------------------------------------------------------------------------------------------------

#include "IGameFramework.h"
#include "IInput.h"
#include "IFlashPlayer.h"
#include "ILevelSystem.h"
#include "IHardwareMouse.h"

//-----------------------------------------------------------------------------------------------------

class CFlashMenuScreen;
class CGameFlashAnimation;
class CMPHub;
struct IAVI_Reader;
struct IMusicSystem;
struct IVideoPlayer;

//-----------------------------------------------------------------------------------------------------

enum ECrysisProfileColor
{
	CrysisProfileColor_Amber,
	CrysisProfileColor_Blue,
	CrysisProfileColor_Green,
	CrysisProfileColor_Red,
	CrysisProfileColor_White,
	CrysisProfileColor_Default
};

//-----------------------------------------------------------------------------------------------------

class CFlashMenuObject : public IGameFrameworkListener, IHardwareMouseEventListener, IInputEventListener, IFSCommandHandler, ILevelSystemListener, IFlashLoadMovieHandler
{
public:

	enum EMENUSCREEN
	{
		MENUSCREEN_FRONTENDSTART,
		MENUSCREEN_FRONTENDINGAME,
		MENUSCREEN_FRONTENDLOADING,
		MENUSCREEN_FRONTENDTEST,
		MENUSCREEN_COUNT
	};

	void GetMemoryStatistics( ICrySizer * );

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime);
	virtual void OnSaveGame(ISaveGame* pSaveGame);
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
  virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);
	// ~IInputEventListener

	// IFSCommandHandler
	void HandleFSCommand(const char *strCommand,const char *strArgs);
	// ~IFSCommandHandler

	// IFlashLoadMovieImage
	IFlashLoadMovieImage* LoadMovie(const char* pFilePath);
	// ~IFlashLoadMovieImage

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) {}
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error);
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount);
	virtual int KeyNameToKeyCode(const SInputEvent &rInputEvent, uint32 keyType, bool &specialKey);
		//~ILevelSystemListener

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent);
	// ~IHardwareMouseEventListener

	//Profiles screen
	virtual void SetProfile();
	virtual void UpdateMenuColor();
	virtual bool ColorChanged();
	virtual void SetColorChanged();
	static CFlashMenuObject *GetFlashMenuObject();

	ILINE bool IsActive() { return m_bUpdate; }

	void ShowMainMenu();
	void ShowInGameMenu(bool bShow=true);
  void HideInGameMenuNextFrame();
  void UnloadHUDMovies();
  void ReloadHUDMovies();
  bool PlayFlashAnim(const char* pFlashFile);
	bool PlayVideo(const char* pVideoFile, bool origUpscaleMode = true, unsigned int videoOptions = 0);
	void StopVideo();
	void NextIntroVideo();
	void UpdateRatio();
  bool IsOnScreen(EMENUSCREEN screen);
  
	void InitStartMenu();
	void InitIngameMenu();
	void DestroyStartMenu();
	void DestroyIngameMenu();

  CMPHub* GetMPHub()const;

	CFlashMenuScreen *GetMenuScreen(EMENUSCREEN screen) const;

	bool Load();

	enum EEntryMovieState
	{
		eEMS_Start,
		eEMS_EA,
		eEMS_Crytek,
		eEMS_EAGames,
		eEMS_NVidia,
		eEMS_Intel,
		eEMS_Dell,
		eEMS_SofDec,
		eEMS_Stop,
		eEMS_Done,
		eEMS_GameStart,
		eEMS_GameIntro,
		eEMS_GameStop,
		eEMS_GameDone
	};

						CFlashMenuObject();
	virtual ~	CFlashMenuObject();

	ECrysisProfileColor GetCrysisProfileColor();

private:

	void UpdateLevels(const char *gamemode);

	//Profiles screen
	void UpdateProfiles();
	void AddProfile(const char *profileName);
	void SelectProfile(const char *profileName, bool silent = false);
	void DeleteProfile(const char *profileName);
	void SwitchProfiles(const char *oldProfile, const char *newProfile);
	void SelectActiveProfile();

	//Singleplayer screen
	void UpdateSingleplayerDifficulties();
	void StartSingleplayerGame(const char *strDifficulty);
	void UpdateSaveGames();
	void LoadGame(const char *FileName);
	void SaveGame(const char *FileName);
	void DeleteSaveGame(const char *FileName);

	//Options screen
	void SaveActionToMap(const char* actionmap, const char* action, const char *key);
	void ChangeFlashKey(const char *actionmap, const char* action, const char *key, bool bReturn = false, bool bGamePad = false);
	void SetCVar(const char *command, const string& value);
	void SetProfileValue(const char *key, const string& value);
	void UpdateCVar(const char *command);
	void UpdateKeyMenu();
	void RestoreDefaults();
	void SetDifficulty(int level = 0);

	void SetDisplayFormats();

	enum ESound
	{
		ESound_RollOver,
		ESound_Click1,
		ESound_Click2,
		ESound_MenuHighlight,
		ESound_MenuSelect,
		ESound_MenuOpen,
		ESound_MenuClose,
		ESound_ScreenChange
	};

	void PlaySound(ESound eSound);

	void LockPlayerInputs(bool bLock);

	static CFlashMenuObject *s_pFlashMenuObject;

	CFlashMenuScreen *m_apFlashMenuScreens[MENUSCREEN_COUNT];
	CFlashMenuScreen *m_pCurrentFlashMenuScreen;

	IPlayerProfileManager* m_pPlayerProfileManager;

	IMusicSystem *m_pMusicSystem;

	struct SLoadSave
	{
		string name;
		bool save;
	} m_sLoadSave;

	typedef std::map<string,Vec2> ButtonPosMap;
	ButtonPosMap m_avButtonPosition;
	ButtonPosMap m_avLastButtonPosition;
	string m_sCurrentButton;

	void UpdateButtonSnap(const Vec2 mouse);
	void SnapToNextButton(const Vec2 dir);
	void GetButtonClientPos(ButtonPosMap::iterator button, Vec2 &pos);
	void HighlightButton(ButtonPosMap::iterator button);
	void PushButton(ButtonPosMap::iterator button, bool press, bool force);
	ButtonPosMap::iterator FindButton(const TKeyName &shortcut);

	int m_iMaxProgress;

	bool m_bUpdateFrontEnd;

	float m_fMusicFirstTime;
	float m_fLastUnloadFrame;

	bool m_bNoMoveEnabled;
	bool m_bNoMouseEnabled;
	bool m_bLanQuery;
	bool m_bUpdate;
	bool m_bIgnoreEsc;
	bool m_bCatchNextInput;
	bool m_catchGamePad;
	bool m_bDestroyStartMenuPending;
	bool m_bDestroyInGameMenuPending;
	bool m_bVirtualKeyboardFocus;
	string m_sActionToCatch;
	string m_sActionMapToCatch;
	string m_sScreenshot;
	bool m_bTakeScreenshot;
	bool m_bColorChanged;
	bool m_bControllerConnected;
	int m_nBlackGraceFrames;
	int m_nLastFrameUpdateID;
	int m_iGamepadsConnected;
	int m_iWidth;
	int m_iHeight;
	ECrysisProfileColor m_eCrysisProfileColor;
	EEntryMovieState m_stateEntryMovies;
	IFlashPlayer* m_pFlashPlayer;
	IVideoPlayer* m_pVideoPlayer;
	IAVI_Reader* m_pAVIReader;

  CMPHub*      m_multiplayerMenu;
};

//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------
