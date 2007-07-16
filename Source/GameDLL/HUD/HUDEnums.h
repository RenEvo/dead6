enum ESound
{
	ESound_Hud_First,
	ESound_GenericBeep,
	ESound_PresetNavigationBeep,
	ESound_TemperatureBeep,
	ESound_SuitMenuAppear,
	ESound_SuitMenuDisappear,
	ESound_WeaponModification,
	ESound_BinocularsZoomIn,
	ESound_BinocularsZoomOut,
	ESound_BinocularsSelect,
	ESound_BinocularsDeselect,
	ESound_BinocularsAmbience,
	ESound_NightVisionSelect,
	ESound_NightVisionDeselect,
	ESound_NightVisionAmbience,
	ESound_BinocularsLock,
	ESound_SniperZoomIn,
	ESound_SniperZoomOut,
	ESound_OpenPopup,
	ESound_ClosePopup,
	ESound_TabChanged,
	ESound_WaterDroplets,
	ESound_BuyBeep,
	ESound_BuyError,
	ESound_Highlight,
	ESound_Select,
	ESound_Hud_Last
};

enum EHUDBATTLELOG_EVENTTYPE
{
	eBLE_Information=2,
	eBLE_Currency=1,
	eBLE_Warning=3,
	eBLE_System
};

enum EWarningMessages
{
	EHUD_SPECTATOR,
	EHUD_SWITCHTOTAN,
	EHUD_SWITCHTOBLACK,
	EHUD_SUICIDE,
	EHUD_CONNECTION_LOST,
	EHUD_OK,
	EHUD_YESNO,
	EHUD_CANCEL
};

//HUDQuickMenu / PDa suit modes
enum EQuickMenuButtons
{
	EQM_ARMOR = 0,
	EQM_WEAPON,
	EQM_CLOAK,
	EQM_STRENGTH,
	EQM_SPEED
};

//locking types
enum ELockingType
{
	eLT_Locking			= 0,
	eLT_Locked			= 1,
	eLT_Static			= 2
};

enum EHUDGAMERULES
{
	EHUD_SINGLEPLAYER,
	EHUD_INSTANTACTION,
	EHUD_POWERSTRUGGLE,
	EHUD_TEAMACTION,
};
