/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: PowerStruggle mode HUD (BuyMenu) code (refactored from old HUD code)

-------------------------------------------------------------------------
History:
- 21:02:20067  20:00 : Created by Jan Müller

*************************************************************************/

#include "HUDObject.h"

#include "HUD.h"

class CGameFlashAnimation;

class CHUDPowerStruggle : public CHUDObject
{
	friend class CHUD;
public:

	CHUDPowerStruggle(CHUD *pHUD, CGameFlashAnimation *pBuyMenu, CGameFlashAnimation *pBuyItem, CGameFlashAnimation *pCaptureProgress);
	~CHUDPowerStruggle();

	void OnUpdate(float fDeltaTime,float fFadeValue);

	//buyable item types
	enum EBuyMenuPage
	{
		E_WEAPONS				= 1 << 0,
		E_VEHICLES			= 1 << 1,
		E_EQUIPMENT			= 1 << 2,
		E_AMMO					= 1 << 3,
		E_PROTOTYPES		= 1 << 4
	};

	struct SItem
	{
		string strName;
		string strDesc;
		string strClass;
		int iPrice;
		float level;
		int iInventoryID;
		bool isWeapon;
		bool isUnique;
		bool bAmmoType;
		int iCount;
		int iMaxCount;
	};

	struct SEquipmentPack
	{
		string strName;
		int iPrice;
		std::vector<SItem> itemArray;
	};

	virtual bool IsFactoryType(EntityId entity, EBuyMenuPage type);
	virtual bool CanBuild(IEntity *pEntity, const char *vehicle);

	void UpdateBuyZone(bool trespassing, EntityId zone);
	void UpdateBuyList(const char *page = NULL);

	//	Swing-O-Meter interface
	// sets current client team
	void SetSOMTeam(int teamId);
	// hide/unhide SOM
	void HideSOM(bool hide);

	void GetTeamStatus(int teamId, float &power, float &turretstatus, int &controlledAliens, EntityId &prototypeFactoryId);

	void ShowCaptureProgress(bool show);
	void SetCaptureProgress(float progress);

	void ShowConstructionProgress(bool show, bool queued, float time);
	void ReviveCycle(float total, float remaining);

private:

	int GetPlayerPP();
	int GetPlayerTeamScore();
	void Buy(const char* item, bool reload = true);

	void DeletePackage(int index);
	void BuyPackage(int index);

	void InitEquipmentPacks();
	void SaveEquipmentPacks();
	SEquipmentPack LoadEquipmentPack(int index);
	void SavePackage(const char *name, int index = -1);
	void RequestNewLoadoutName(string &name);
	void CreateItemFromTableEntry(IScriptTable *pItemScriptTable, SItem &item);
	bool GetItemFromName(const char *name, SItem &item);

	bool WeaponUseAmmo(CWeapon *pWeapon, IEntityClass* pAmmoType);
	bool CanUseAmmo(IEntityClass* pAmmoType);
	void GetItemList(EBuyMenuPage itemType, std::vector<SItem> &itemList);
	void PopulateBuyList();
	void UpdatePackageItemList(const char *page);
	EBuyMenuPage ConvertToBuyList(const char *page);
	void UpdatePackageList();
	void UpdateCurrentPackage();
	void OnSelectPackage(int index);
	void UpdateModifyPackage(int index);

	//****************************************** MEMBER VARIABLES ***********************************

	//current active buy zones
	std::vector<EntityId> m_currentBuyZones;
	//standing in buy zone ?
	bool m_bInBuyZone;
	//-2=inactive, -1=catch page, 1-0=catch item on page
	int m_iCatchBuyMenuPage;	
	//available equipment packs
	std::vector<SEquipmentPack> m_EquipmentPacks;
	//current buy menu page
	EBuyMenuPage	m_eCurBuyMenuPage;
	//buy menu flash movie - managed by HUD
	CGameFlashAnimation *g_pBuyMenu;
	CGameFlashAnimation *g_pBuyIcon;
	CGameFlashAnimation *g_pCaptureProgress;
	//swing-o-meter - managed here
	CGameFlashAnimation m_animSwingOMeter;
	//the main hud
	CHUD *g_pHUD;
	//currently available buy menu pages
	std::vector<bool> m_factoryTypes;
	//swing-o-meter setup
	bool	m_nkLeft;

	//new powerstruggle rules 
	std::vector<EntityId> m_powerpoints;
	std::vector<EntityId> m_turrets;
	bool m_gotpowerpoints;
	EntityId m_protofactory;

	bool m_capturing;
	float m_captureProgress;

	bool m_constructing;
	bool m_constructionQueued;
	float m_constructionTime;
	float m_constructionTimer;
	float m_reviveCycle;
	CTimeValue m_reviveCycleEnd;


	int m_teamId;
};
