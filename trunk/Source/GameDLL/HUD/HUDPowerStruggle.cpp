/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: PowerStruggle mode HUD code (refactored from old HUD code)

-------------------------------------------------------------------------
History:
- 21:02:20067  20:00 : Created by Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "HUDPowerStruggle.h"

#include "HUD.h"
#include "HUDRadar.h"
#include "GameFlashAnimation.h"
#include "Menus/FlashMenuObject.h"
#include "../Game.h"
#include "../GameCVars.h"
#include "../GameRules.h"
#include "Weapon.h"
#include "Menus/OptionsManager.h"

#define HUD_CALL_LISTENERS_PS(func) \
{ \
	if (g_pHUD->m_hudListeners.empty() == false) \
	{ \
		g_pHUD->m_hudTempListeners = g_pHUD->m_hudListeners; \
		for (std::vector<CHUD::IHUDListener*>::iterator tmpIter = g_pHUD->m_hudTempListeners.begin(); tmpIter != g_pHUD->m_hudTempListeners.end(); ++tmpIter) \
			(*tmpIter)->func; \
	} \
}

static inline bool SortByPrice(const CHUDPowerStruggle::SItem &rItem1,const CHUDPowerStruggle::SItem &rItem2)
{
	return rItem1.iPrice < rItem2.iPrice;
}

//-----------------------------------------------------------------------------------------------------

CHUDPowerStruggle::CHUDPowerStruggle(CHUD *pHUD, CGameFlashAnimation *pBuyMenu, CGameFlashAnimation *pBuyIcon, CGameFlashAnimation *pCaptureProgress) : 
	g_pHUD(pHUD), g_pBuyMenu(pBuyMenu), g_pBuyIcon(pBuyIcon), g_pCaptureProgress(pCaptureProgress)
{
	m_bInBuyZone = false;
	m_eCurBuyMenuPage = E_AMMO;
	m_nkLeft = true; //NK is left on default
	m_factoryTypes.resize(5);
	m_teamId = 0;
	m_gotpowerpoints = false;
	m_protofactory = 0;
	m_capturing = false;
	m_captureProgress = 0.0f;

	m_constructing=false;
	m_constructionQueued=false;
	m_constructionTime=0.0f;
	m_constructionTimer=0.0f;
	m_reviveCycle=0.0f;
	m_reviveCycleEnd=0.0f;

	m_animSwingOMeter.Load("Libs/UI/HUD_Swing-O-Meter.gfx", eGFD_Center, eFAF_ManualRender|eFAF_Visible);
	m_animSwingOMeter.GetFlashPlayer()->SetVisible(true);
}

CHUDPowerStruggle::~CHUDPowerStruggle()
{
}

void DrawBar(float x, float y, float width, float height, float border, float progress, ColorF &color0, ColorF &color1, const char *text, ColorF &textColor, float bgalpha)
{
	float sy=gEnv->pRenderer->ScaleCoordY(y);
	float sx=gEnv->pRenderer->ScaleCoordX(x+width*0.5f);

	ColorF interp;
	interp.lerpFloat(color0, color1, progress);

	float currw=width*progress;
	gEnv->pRenderer->Draw2dImage(x-border, y-border, width+border+border, height+border+border, 0, 0, 0, 0, 0, 0, 0.0f,0.0f, 0.0f, bgalpha);

	gEnv->pRenderer->Draw2dImage(x, y, currw, height, 0, 0, 0, 0, 0, 0, interp.r, interp.g, interp.b, 0.75f);
	gEnv->pRenderer->Draw2dImage(x+currw, y, width-currw, height, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.35f);

	if (text && text[0])
		gEnv->pRenderer->Draw2dLabel(sx, sy, 1.6f, (float*)&textColor, true, "%s", text);
}

void DrawSemiCircle(float x, float y, float r, int slices, float start, float end, ColorF &center, ColorF &color0, ColorF &color1)
{
	gEnv->pRenderer->SelectTMU(0);
	gEnv->pRenderer->EnableTMU(false);

	static std::vector<ushort>																indices;
	static std::vector<struct_VERTEX_FORMAT_P3F_COL4UB_TEX2F> vertices;
	vertices.resize(0);
	indices.resize(0);

	struct_VERTEX_FORMAT_P3F_COL4UB_TEX2F vtx;
	vtx.color.dcolor=center.pack_argb8888();
	vtx.xyz[0]=x;
	vtx.xyz[1]=y;
	vtx.xyz[2]=0.0f;
	vtx.st[0]=0.0f;
	vtx.st[1]=0.0f;

	vertices.push_back(vtx);

	float time=end-start;
	int nslices=cry_ceilf((slices*time*gf_PI)/gf_PI);
	float t=start;
	float dt=time/nslices;

	ColorF color;

	for (int i=0;i<nslices;i++)
	{
		if (t+dt>end)
			dt=end-t;
		
		color.lerpFloat(color0, color1, t/time);
		vtx.color.dcolor=color.pack_argb8888();
		vtx.xyz[0]=x+r*cry_sinf(t*gf_PI*2.0f);
		vtx.xyz[1]=y-r*cry_cosf(t*gf_PI*2.0f);
		vertices.push_back(vtx);

		color.lerpFloat(color0, color1, (t+dt)/time);
		vtx.color.dcolor=color.pack_argb8888();
		vtx.xyz[0]=x+r*cry_sinf((t+dt)*gf_PI*2.0f);
		vtx.xyz[1]=y-r*cry_cosf((t+dt)*gf_PI*2.0f);
		vertices.push_back(vtx);

		indices.push_back(0);
		indices.push_back(1+i*2+0);
		indices.push_back(1+i*2+1);

		t+=dt;
	}

	gEnv->pRenderer->DrawDynVB(&vertices[0], &indices[0], vertices.size(), indices.size(), R_PRIMV_TRIANGLES);
}

void CHUDPowerStruggle::OnUpdate(float fDeltaTime,float fFadeValue)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	
	if (pGameRules && !stricmp(pGameRules->GetEntity()->GetClass()->GetName(), "PowerStruggle"))
	{
		int teamId=0;
		IActor *pLocalActor=g_pGame->GetIGameFramework()->GetClientActor();
		if (pLocalActor)
			teamId=pGameRules->GetTeam(pLocalActor->GetEntityId());

		if (!m_gotpowerpoints || !m_protofactory)
		{
			m_powerpoints.resize(0);
			m_turrets.resize(0);

			IEntityClass *pAlienEnergyPoint=gEnv->pEntitySystem->GetClassRegistry()->FindClass("AlienEnergyPoint");
			IEntityClass *pTurret=gEnv->pEntitySystem->GetClassRegistry()->FindClass("AutoTurret");
			IEntityClass *pAATurret=gEnv->pEntitySystem->GetClassRegistry()->FindClass("AutoTurretAA");
			IEntityClass *pFactory=gEnv->pEntitySystem->GetClassRegistry()->FindClass("Factory");

			IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
			while (!pIt->IsEnd())
			{
				if (IEntity *pEntity = pIt->Next())
				{
					IEntityClass *pClass=pEntity->GetClass();

					if (pClass == pAlienEnergyPoint)
					{
						m_powerpoints.push_back(pEntity->GetId());
					}
					else if (pClass == pFactory)
					{
						SmartScriptTable props;
						if (pEntity->GetScriptTable()->GetValue("Properties", props))
						{
							int proto=0;
							if (props->GetValue("bPowerStorage", proto) && proto)
								m_protofactory=pEntity->GetId();
						}
					}
					else if (pClass == pTurret || pClass == pAATurret)
					{
						m_turrets.push_back(pEntity->GetId());
					}
				}
			}

			m_gotpowerpoints=true;
		}

		if (teamId==0)
			teamId=1;

		if (teamId!=m_teamId)
		{
			m_animSwingOMeter.Invoke("setOwnTeam", teamId==1?"NK":"US");
			m_teamId=teamId;
		}

		float power[2]={0.0f};
		float turret[2]={0.0f};
		int aliens[2]={0};
		EntityId proto[2]={0};

		GetTeamStatus(1, power[0], turret[0], aliens[0], proto[0]);
		GetTeamStatus(2, power[1], turret[1], aliens[1], proto[1]);

		if (teamId==1)
		{
			SFlashVarValue aliensarg[2]={aliens[0], aliens[1]};
			m_animSwingOMeter.Invoke("showAliens", aliensarg, 2);

			if (proto[0])
			{
				SFlashVarValue glowarg(1);
				m_animSwingOMeter.Invoke("showGlow", &glowarg, 1);
			}

			SFlashVarValue turretarg[2]={turret[0]*100.0f, turret[1]*100.0f};
			m_animSwingOMeter.Invoke("setStatusBars", turretarg, 2);

			SFlashVarValue powerarg[2]={cry_floorf(power[0]), cry_floorf(power[1])};
			m_animSwingOMeter.Invoke("setLoadBar", powerarg, 2);
		}
		else
		{
			SFlashVarValue aliensarg[2]={aliens[1], aliens[0]};
			m_animSwingOMeter.Invoke("showAliens", aliensarg, 2);

			if (proto[0])
			{
				SFlashVarValue glowarg(2);
				m_animSwingOMeter.Invoke("showGlow", &glowarg, 1);
			}

			SFlashVarValue turretarg[2]={turret[1]*100.0f, turret[0]*100.0f};
			m_animSwingOMeter.Invoke("setStatusBars", turretarg, 2);

			SFlashVarValue powerarg[2]={cry_floorf(power[1]), cry_floorf(power[0])};
			m_animSwingOMeter.Invoke("setLoadBar", powerarg, 2);

		}

		int time=(int)pGameRules->GetRemainingGameTime();
		CryFixedStringT<32> timeFormatter;
		timeFormatter.Format("%02d:%02d", time/60, time%60);
		SFlashVarValue timearg(timeFormatter.c_str());
		m_animSwingOMeter.Invoke("setTimer", &timearg, 1);

		m_animSwingOMeter.GetFlashPlayer()->Advance(fDeltaTime);
		m_animSwingOMeter.GetFlashPlayer()->Render();

		gEnv->pRenderer->Set2DMode(true, 800.0f, 600.0f);
		gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA);
		gEnv->pRenderer->SetWhiteTexture();
		//gEnv->pRenderer->SetState(GS_NODEPTHTEST);

		static char text[32];
		if (m_capturing)
		{
			int icap=(int)(m_captureProgress*100.0f);
			sprintf(text, "%d%%", icap);
			DrawBar(16.0f, 80.0f, 72.0f, 14.0f, 2.0f, m_captureProgress, Col_DarkGray, Col_LightGray, text, Col_White, fabsf(cry_sinf(gEnv->pTimer->GetCurrTime()*2.5f)));
		}

		if (m_reviveCycle>0.0f)
		{
			float remaining=(m_reviveCycleEnd-gEnv->pTimer->GetFrameStartTime()).GetSeconds()/m_reviveCycle;
			remaining=CLAMP(remaining, 0.0f, 1.0f);

			ColorF color=Col_Aquamarine*0.9f;
			color.a=0.75f;
			DrawSemiCircle(400, 300, 30, 20, 0.0f, 1.0f-remaining, color, color, color);

			ColorF bg=Col_Black;
			bg.a=0.35f;
			DrawSemiCircle(400, 300, 30, 20, 1.0f-remaining, 1.0f, bg, bg, bg);
		}

		if (m_constructing)
		{
			if (!m_constructionQueued)
			{
				sprintf(text, "ETA: %.1f", m_constructionTimer);
				DrawBar(16.0f, 100.0f, 72.0f, 14.0f, 2.0f, 1.0f-(m_constructionTimer/m_constructionTime), Col_DarkSlateBlue, Col_LightSteelBlue, text, Col_White, fabsf(cry_sinf(gEnv->pTimer->GetCurrTime()*2.5f)));

				m_constructionTimer-=fDeltaTime;
				if (m_constructionTimer<0.0f)
					m_constructionTimer=0.0f;
			}
			else
				DrawBar(16.0f, 100.0f, 72.0f, 14.0f, 2.0f, 0.0f, Col_DarkSlateBlue, Col_LightSteelBlue, "ETA: N/A", Col_White, fabsf(cry_sinf(gEnv->pTimer->GetCurrTime()*2.5f)));
		}

		gEnv->pRenderer->Set2DMode(false, 0.0f, 0.0f);
	}
}

//-----------------------------------------------------------------------------------------------------

int CHUDPowerStruggle::GetPlayerPP()
{
	int pp = 0;
	CGameRules *pGameRules = g_pGame->GetGameRules();
	IScriptTable *pScriptTable = pGameRules->GetEntity()->GetScriptTable();
	if(pScriptTable)
	{
		int key = 0;
		pScriptTable->GetValue("PP_AMOUNT_KEY", key);
		pGameRules->GetSynchedEntityValue(g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId(), TSynchedKey(key), pp);
	}
	return pp;
}

//-----------------------------------------------------------------------------------------------------

int CHUDPowerStruggle::GetPlayerTeamScore()
{
	int iTeamScore = 0;
	CGameRules *pGameRules = g_pGame->GetGameRules();
	IScriptTable *pScriptTable = pGameRules->GetEntity()->GetScriptTable();
	if(pScriptTable)
	{
		int key = 0;
		pScriptTable->GetValue("TEAMSCORE_TEAM0_KEY", key);
		EntityId uiPlayerID = g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId();
		int iTeamID = pGameRules->GetTeam(uiPlayerID);
		pGameRules->GetSynchedGlobalValue(TSynchedKey(key+iTeamID), iTeamScore);
	}
	return iTeamScore;
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::Buy(const char* item, bool reload)
{
	string buy("buy ");

	SItem itemdef;
	if (GetItemFromName(item, itemdef) && itemdef.bAmmoType)
		buy="buyammo ";

	buy.append(item);
	gEnv->pConsole->ExecuteString(buy.c_str());

	if(reload)
	{
		InitEquipmentPacks();
		UpdatePackageList();
		UpdateBuyList();
	}
}

//-----------------------------------------------------------------------------------------------------
void CHUDPowerStruggle::BuyPackage(int index)
{
	if(index>=0 && m_EquipmentPacks.size()>index)
	{

		CPlayer *pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer && pPlayer->GetInventory())
		{
			std::vector<SItem>::const_iterator it = m_EquipmentPacks[index].itemArray.begin();
			for(; it != m_EquipmentPacks[index].itemArray.end(); ++it)
			{
				SItem item = (*it);
				if(item.bAmmoType)
					Buy(item.strName, false);
				else
				{
					if(item.iInventoryID==0  || !item.isUnique)
						Buy(item.strName, false);

					CWeapon *pWeapon = g_pHUD->GetWeapon();
					if(pWeapon)
					{
						IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(item.strClass.c_str());
						IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pPlayer->GetInventory()->GetItemByClass(pClass));
						if(pItem && pItem != pWeapon)
						{
							const bool bAddAccessory = pWeapon->GetAccessory(item.strClass.c_str()) == 0;
							if(bAddAccessory)
							{
								pWeapon->SwitchAccessory(item.strClass.c_str());
								HUD_CALL_LISTENERS_PS(WeaponAccessoryChanged(pWeapon, item.strClass.c_str(), bAddAccessory));
							}
						}
					}
				}
			}
		}
		InitEquipmentPacks();
		UpdatePackageList();
		UpdateCurrentPackage();
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::DeletePackage(int index)
{
	if(index>=0 && m_EquipmentPacks.size()>index)
	{
		m_EquipmentPacks.erase(m_EquipmentPacks.begin() + index);
	}
	SaveEquipmentPacks();
}

//-----------------------------------------------------------------------------------------------------

CHUDPowerStruggle::SEquipmentPack CHUDPowerStruggle::LoadEquipmentPack(int index)
{
	SEquipmentPack newPack;
	newPack.iPrice = 0;
	char strPathName[256];
	sprintf(strPathName,"Multiplayer.EquipmentPacks.%d.Name",index);
	char strPathNums[256];
	sprintf(strPathNums,"Multiplayer.EquipmentPacks.%d.NumItems",index);
	g_pGame->GetOptions()->GetProfileValue(strPathName, newPack.strName);
	int numEntries = 0;
	if(g_pGame->GetOptions()->GetProfileValue(strPathNums,numEntries))
	{
		for(int i(0); i<numEntries; ++i)
		{
			string entry;
			char strPath[256];
			sprintf(strPath,"Multiplayer.EquipmentPacks.%d.%d",index,i);
			if(g_pGame->GetOptions()->GetProfileValue(strPath,entry))
			{
				SItem item;
				if(GetItemFromName(entry, item))
				{
					newPack.itemArray.push_back(item);
					if(item.iInventoryID<=0)
						newPack.iPrice += item.iPrice;
				}
			}
		}
	}
	return newPack;
}

void CHUDPowerStruggle::RequestNewLoadoutName(string &name)
{
	const char *compareString = "Unnamed";
	//define name
	int maxNumber = 0;
	for(int i(0); i<m_EquipmentPacks.size(); ++i)
	{
		SEquipmentPack *pack = &m_EquipmentPacks[i];
		string strPackName = pack->strName;
		int index = strPackName.find(compareString) ;
		if(index==0)
		{
			strPackName = strPackName.substr(strlen(compareString),strlen(strPackName));
			int number = atoi(strPackName);
			if(number>maxNumber)
				maxNumber = number;
		}
	}
	++maxNumber;
	char number[64];
	sprintf(number,"%02d",maxNumber);
	name = "Unnamed";
	name.append(number);
}

void CHUDPowerStruggle::SavePackage(const char *name, int index)
{
	SEquipmentPack pack;
	if(strlen(name)>0)
	{
		pack.strName = name;
	}
	else
	{
		RequestNewLoadoutName(pack.strName);
	}
	pack.iPrice = 0;

	std::vector<const char*> getArray;
	if(g_pBuyMenu)
	{
		int iSize = g_pBuyMenu->GetFlashPlayer()->GetVariableArraySize("m_backArray");
		getArray.resize(iSize);
		g_pBuyMenu->GetFlashPlayer()->GetVariableArray(FVAT_ConstStrPtr,"m_backArray", 0, &getArray[0], iSize);
		int size = getArray.size();
	}
	std::vector<const char*>::const_iterator it = getArray.begin();
	for(; it != getArray.end(); ++it)
	{
		SItem item;
		if(GetItemFromName(*it,item))
			pack.itemArray.push_back(item);
			pack.iPrice += item.iPrice;
	}

	if(index>=0 && m_EquipmentPacks.size()>index)
	{
		m_EquipmentPacks[index] = pack;
	}
	else
	{
		m_EquipmentPacks.push_back(pack);
	}
	SaveEquipmentPacks();
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::SaveEquipmentPacks()
{

	g_pGame->GetOptions()->SaveValueToProfile("Multiplayer.EquipmentPacks.NumPacks",(int)m_EquipmentPacks.size());

	{
		for(int i(0); i<m_EquipmentPacks.size(); ++i)
		{
			SEquipmentPack pack = m_EquipmentPacks[i];
			char strPathNum[256];
			sprintf(strPathNum,"Multiplayer.EquipmentPacks.%d.NumItems",i);
			char strPathName[256];
			sprintf(strPathName,"Multiplayer.EquipmentPacks.%d.Name",i);
			g_pGame->GetOptions()->SaveValueToProfile(strPathNum,(int)pack.itemArray.size());
			g_pGame->GetOptions()->SaveValueToProfile(strPathName,pack.strName);
			for(int j(0); j<pack.itemArray.size(); ++j)
			{
				char strPath[256];
				sprintf(strPath,"Multiplayer.EquipmentPacks.%d.%d",i,j);
				g_pGame->GetOptions()->SaveValueToProfile(strPath,pack.itemArray[j].strName);
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::InitEquipmentPacks()
{
	m_EquipmentPacks.resize(0);
	string strPathNums = "Multiplayer.EquipmentPacks.NumPacks";
	int numEntries = 0;
	if(g_pGame->GetOptions()->GetProfileValue(strPathNums,numEntries))
	{
		for(int i(0); i<numEntries; ++i)
		{
			SEquipmentPack pack = LoadEquipmentPack(i);
			m_EquipmentPacks.push_back(pack);
		}
	}
}

//-----------------------------------------------------------------------------------------------------
bool CHUDPowerStruggle::GetItemFromName(const char *name, SItem &item)
{
	IScriptTable *pGameRulesScriptTable = g_pGame->GetGameRules()->GetEntity()->GetScriptTable();
	SmartScriptTable pItemListScriptTable;

	float scale = g_pGameCVars->g_pp_scale_price;

	std::vector<string> tableList;
	tableList.push_back("weaponList");
	tableList.push_back("ammoList");
	tableList.push_back("vehicleList");
	tableList.push_back("equipList");
	tableList.push_back("protoList");

	std::vector<string>::const_iterator it = tableList.begin();
	for(; it != tableList.end(); ++it)
	{
		if(pGameRulesScriptTable->GetValue(*it,pItemListScriptTable))
		{
			IScriptTable::Iterator iter = pItemListScriptTable->BeginIteration();
			while(pItemListScriptTable->MoveNext(iter))
			{
				if(ANY_TTABLE != iter.value.type)
					continue;

				IScriptTable *pEntry = iter.value.table;

				bool invisible=false;
				if (pEntry->GetValue("invisible", invisible) && invisible)
					continue;

				const char *id=0;
				if (pEntry->GetValue("id", id) && !strcmp(id, name))
				{
					bool ammo=false;

					CreateItemFromTableEntry(pEntry, item);

					item.bAmmoType = pEntry->GetValue("ammo", ammo) && ammo;
					pEntry->EndIteration(iter);
					return true;
				}
			}
			pItemListScriptTable->EndIteration(iter);
		}
	}
	return false;
}

//------------------------------------------------------------------------

void CHUDPowerStruggle::ShowCaptureProgress(bool show)
{
	g_pCaptureProgress->SetVisible(show);
	m_capturing=show;
}

//------------------------------------------------------------------------
void CHUDPowerStruggle::SetCaptureProgress(float progress)
{
	g_pCaptureProgress->Invoke("setCapturing", true);
	m_captureProgress=progress;
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::CreateItemFromTableEntry(IScriptTable *pItemScriptTable, SItem &item)
{
	char *strId = NULL;
	char *strDesc = NULL;
	char *strClass = NULL;
	int	iPrice = 0;
	float level = 0.0f;
	bool unique = true;

	pItemScriptTable->GetValue("id",strId);
	pItemScriptTable->GetValue("name",strDesc);
	pItemScriptTable->GetValue("class",strClass);
	pItemScriptTable->GetValue("price",iPrice);
	pItemScriptTable->GetValue("uniqueId",unique);
	pItemScriptTable->GetValue("level",level);

	float scale = g_pGameCVars->g_pp_scale_price;

	item.strName = strId;
	item.strDesc = strDesc;
	item.iPrice = (int)(iPrice*scale);
	item.level = level;
	item.strClass = strClass;
	item.isUnique = unique;

	EntityId inventoryItem = 0;
	if(strClass)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		if(pActor)
		{
			IInventory *pInventory = pActor->GetInventory();
			if(pInventory)
			{
				IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(strClass);
				inventoryItem = pInventory->GetItemByClass(pClass);

				if(IItem *pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(inventoryItem))
				{
					if(pClass == CItem::sSOCOMClass)
					{
						bool bSlave = pItem->IsDualWieldSlave();
						bool bMaster = pItem->IsDualWieldMaster();

						if(!bSlave && !bMaster)
							inventoryItem = 0;
					}

					if (!pItem->CanSelect())
						inventoryItem = 0;
				}
			}
		}
	}
	item.iInventoryID = (int)inventoryItem;
};


bool CHUDPowerStruggle::WeaponUseAmmo(CWeapon *pWeapon, IEntityClass* pAmmoType)
{
	int nfm=pWeapon->GetNumOfFireModes();

	for (int fm=0; fm<nfm; fm++)
	{
		IFireMode *pFM = pWeapon->GetFireMode(fm);
		if (pFM && pFM->GetAmmoType() == pAmmoType)
			return true;
	}

	return false;
}

bool CHUDPowerStruggle::CanUseAmmo(IEntityClass* pAmmoType)
{
	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return false;

	IInventory *pInventory = pActor->GetInventory();
	if (!pInventory)
		return false;

	if (!pAmmoType)
		return true;

	int n=pInventory->GetCount();
	for (int i=0; i<n; i++)
	{
		if (EntityId itemId=pInventory->GetItem(i))
		{
			if (IItem *pItem=gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId))
			{
				CWeapon *pWeapon=static_cast<CWeapon *>(pItem->GetIWeapon());
				if (!pWeapon)
					continue;

				if (WeaponUseAmmo(pWeapon, pAmmoType))
					return true;
			}
		}
	}

	if (IVehicle *pVehicle=pActor->GetLinkedVehicle())
	{
		int n=pVehicle->GetWeaponCount();
		for (int i=0; i<n; i++)
		{
			if (EntityId weaponId=pVehicle->GetWeaponId(i))
			{
				if (IItem *pItem=gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId))
				{
					CWeapon *pWeapon=static_cast<CWeapon *>(pItem->GetIWeapon());
					if (!pWeapon)
						continue;

					if (WeaponUseAmmo(pWeapon, pAmmoType))
						return true;
				}
			}
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::GetItemList(EBuyMenuPage itemType, std::vector<SItem> &itemList)
{
	const char *list = "weaponList";
	if(itemType==E_AMMO)
	{
		list = "ammoList";
	}
	else if(itemType==E_VEHICLES)
	{
		list = "vehicleList";
	}
	else if(itemType==E_EQUIPMENT)
	{
		list = "equipList";
	}
	else if(itemType==E_PROTOTYPES)
	{
		list = "protoList";
	}

	SmartScriptTable pItemListScriptTable;

	IScriptTable *pGameRulesScriptTable = g_pGame->GetGameRules()->GetEntity()->GetScriptTable();

	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(!pActor)
		return;
	IInventory *pInventory = pActor->GetInventory();
	if (!pInventory)
		return;

	float scale = g_pGameCVars->g_pp_scale_price;

	if(pGameRulesScriptTable->GetValue(list,pItemListScriptTable))
	{
		IScriptTable::Iterator iter = pItemListScriptTable->BeginIteration();
		while(pItemListScriptTable->MoveNext(iter))
		{
			if(ANY_TTABLE != iter.value.type)
				continue;

			IScriptTable *pItemScriptTable = iter.value.table;

			char *strId = NULL;
			char *strName = NULL;
			char *strClass = NULL;
			int iPrice = 0;
			float level = 0.0f;
			bool bUnique = false;
			bool bInvisible = false;

			pItemScriptTable->GetValue("invisible",bInvisible);
			if (bInvisible)
				continue;

			pItemScriptTable->GetValue("id",strId);
			if(itemType != E_AMMO)
				pItemScriptTable->GetValue("level", level);
			else
			{
				IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(strId);
				if (!CanUseAmmo(pClass))
					continue;
			}

			pItemScriptTable->GetValue("name",strName);
			pItemScriptTable->GetValue("class",strClass);
			pItemScriptTable->GetValue("uniqueId",bUnique);
			pItemScriptTable->GetValue("price",iPrice);

			SItem item;
			item.strName = strId;
			item.strDesc = strName;
			item.strClass = strClass;
			item.iPrice = (int)(iPrice*scale);
			item.level = level;
			item.isUnique = bUnique;
			item.iCount = 0;
			item.iMaxCount = 1;
			EntityId inventoryItem = 0;
			if(strClass)
			{
				IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(strClass);
				inventoryItem = pInventory->GetItemByClass(pClass);
				IItem *pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(inventoryItem);
				if(pItem)
				{
					item.isWeapon = pItem->CanDrop();
					CWeapon *pWeapon=static_cast<CWeapon *>(pItem->GetIWeapon());
					if(pWeapon)
					{
						IFireMode *fireMode = pWeapon->GetFireMode(0);
						if(fireMode)
						{
							IEntityClass *ammoType = fireMode->GetAmmoType();
							if(ammoType)
							{
								item.iMaxCount = pInventory->GetAmmoCapacity(ammoType);
								item.iCount = pInventory->GetAmmoCount(ammoType);
							}
						}
					}

					if(pClass == CItem::sSOCOMClass)
					{
						bool bSlave = pItem->IsDualWieldSlave();
						bool bMaster = pItem->IsDualWieldMaster();
						if(!bSlave && !bMaster)
							//change to double pistols
							inventoryItem = -1;
					}
					if (!pItem->CanSelect())
						inventoryItem = -2;
				}
				else
				{
					inventoryItem = 0;
				}
			}

			if (bUnique || (int)inventoryItem < 0)
				item.iInventoryID = (int)inventoryItem;
			else
				item.iInventoryID = 0;
			itemList.push_back(item);
		}
		pItemListScriptTable->EndIteration(iter);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::UpdateBuyList(const char *page)
{
	if(gEnv->bMultiplayer)
	{
		if(page)
		{
			m_eCurBuyMenuPage = ConvertToBuyList(page);
		}
		PopulateBuyList();
	}
}

//-----------------------------------------------------------------------------------------------------

CHUDPowerStruggle::EBuyMenuPage CHUDPowerStruggle::ConvertToBuyList(const char *page)
{
	if(page)
	{
		static EBuyMenuPage pages[] = { E_WEAPONS, E_WEAPONS, E_AMMO, E_EQUIPMENT, E_VEHICLES, E_PROTOTYPES };
		static const int numPages = sizeof(pages) / sizeof(pages[0]);
		int pageIndex = atoi(page); // returns 0, if no conversion made
		pageIndex = pageIndex < 0 ? 0 : (pageIndex < numPages ? pageIndex : 0);
		return pages[pageIndex];
	}
	return E_WEAPONS;
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::PopulateBuyList()
{
	if (!g_pGame->GetIGameFramework()->GetClientActor())
		return;

	if(!g_pBuyMenu)
		return;

	if(!g_pHUD->IsBuyMenuActive())
		return;

	EBuyMenuPage itemType = m_eCurBuyMenuPage;

	IFlashPlayer *pFlashPlayer = g_pBuyMenu->GetFlashPlayer();

	std::vector<SItem> itemList;

	pFlashPlayer->Invoke0("Root.PDAArea.clearAllEntities");

	GetItemList(itemType, itemList);

	IEntity *pFirstVehicleFactory = NULL;

	EntityId uiPlayerID = g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId();
	int playerTeam = g_pGame->GetGameRules()->GetTeam(uiPlayerID);
	CGameRules *pGameRules = g_pGame->GetGameRules();
	bool bInvalidBuyZone = true;
	float CurBuyZoneLevel = 0.0f;
	{
		std::vector<EntityId>::const_iterator it = m_currentBuyZones.begin();
		for(; it != m_currentBuyZones.end(); ++it)
		{
			if(pGameRules->GetTeam(*it) == playerTeam)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(*it);
				if(IsFactoryType(*it, itemType))
				{
					float level = g_pHUD->CallScriptFunction(pEntity,"GetPowerLevel");
					if(level>CurBuyZoneLevel)
						CurBuyZoneLevel = level;
					bInvalidBuyZone = false;
					if(itemType == E_VEHICLES)
					{
						pFirstVehicleFactory = pEntity;
					}
				}
			}
		}
	}

	//std::sort(itemList.begin(),itemList.end(),SortByPrice);
	std::vector<string> itemArray;

	char tempBuf[256];

	for(std::vector<SItem>::iterator iter=itemList.begin(); iter!=itemList.end(); ++iter)
	{
		SItem item = (*iter);
		const char* sReason = "ready";

		if(bInvalidBuyZone)
		{
			sReason = "buyzone";
		}
		else
		{
			if(CurBuyZoneLevel<item.level)
				sReason = "level";

			if(itemType == E_VEHICLES)
			{
				if(!CanBuild(pFirstVehicleFactory,item.strName.c_str()))
				{
					continue;
				}
			}
		}
		if(!item.isWeapon)
		{
			if(item.iInventoryID>0 && item.isUnique)
			{
				sReason = "inventory";
			}
			if(item.iInventoryID==-2 && item.isUnique)
			{
				sReason = "inventory";
			}
			if(item.iCount >= item.iMaxCount)
			{
				sReason = "inventory";
			}
			item.iInventoryID = 0;
		}
		itemArray.push_back(item.strName.c_str());
		itemArray.push_back(item.strDesc.c_str());
		_snprintf(tempBuf,sizeof(tempBuf),"%d",item.iPrice);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemArray.push_back(tempBuf);
		itemArray.push_back(sReason);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",item.iInventoryID);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemArray.push_back(tempBuf);
	}

	SFlashVarValue arg[5] = {(int)m_factoryTypes[0], (int)m_factoryTypes[1], (int)m_factoryTypes[2], (int)m_factoryTypes[3], (int)m_factoryTypes[4]};
	g_pBuyMenu->CheckedInvoke("Root.PDAArea.Tabs.activateTabs",arg,5);

	int size = itemArray.size();
	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(itemArray[i].c_str());
		}

		pFlashPlayer->SetVariableArray(FVAT_ConstStrPtr, "Root.PDAArea.m_allValues", 0, &pushArray[0], size);
	}

	pFlashPlayer->Invoke0("Root.PDAArea.updateList");
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::UpdatePackageList()
{
	if(!g_pBuyMenu)
		return;

	std::vector<string>itemList;
	char tempBuf[256];
	std::vector<SEquipmentPack>::const_iterator it = m_EquipmentPacks.begin();
	for(; it != m_EquipmentPacks.end(); ++it)
	{
		SEquipmentPack pack = (*it);
		itemList.push_back(pack.strName);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",pack.iPrice);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemList.push_back(tempBuf);
	}

	SFlashVarValue arg[5] = {(int)m_factoryTypes[0], (int)m_factoryTypes[1], (int)m_factoryTypes[2], (int)m_factoryTypes[3], (int)m_factoryTypes[4]};
	g_pBuyMenu->CheckedInvoke("Root.PDAArea.Tabs.activateTabs",arg,5);

	int size = itemList.size();

	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(itemList[i].c_str());
		}

		g_pBuyMenu->GetFlashPlayer()->SetVariableArray(FVAT_ConstStrPtr, "Root.PDAArea.m_allValues", 0, &pushArray[0], pushArray.size());
	}

	g_pBuyMenu->Invoke("Root.PDAArea.updatePackageList");
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::UpdateCurrentPackage()
{
	if(!g_pBuyMenu)
		return;

	g_pBuyMenu->Invoke("Root.PDAArea.updateCurrentPackage");
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::OnSelectPackage(int index)
{
	SEquipmentPack pack = m_EquipmentPacks[index];
	std::vector<string>itemList;
	char tempBuf[256];
	std::vector<SItem>::const_iterator it = pack.itemArray.begin();
	for(; it != pack.itemArray.end(); ++it)
	{
		const SItem *item = &*it;
		itemList.push_back(item->strName);
		itemList.push_back(item->strDesc);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",item->iPrice);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemList.push_back(tempBuf);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",item->iInventoryID);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemList.push_back(tempBuf);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",(int)(item->isUnique));
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemList.push_back(tempBuf);
	}
	int size = itemList.size();

	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(itemList[i].c_str());
		}

		g_pBuyMenu->GetFlashPlayer()->SetVariableArray(FVAT_ConstStrPtr, "Root.PDAArea.m_allValues", 0, &pushArray[0], size);
	}
	g_pBuyMenu->Invoke("Root.PDAArea.updatePackageContent");
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::UpdateModifyPackage(int index)
{
	SEquipmentPack pack = m_EquipmentPacks[index];
	std::vector<string>itemList;
	std::vector<SItem>::const_iterator it = pack.itemArray.begin();
	for(; it != pack.itemArray.end(); ++it)
	{
		const SItem *item = &*it;
		itemList.push_back(item->strName);
		itemList.push_back(item->strDesc);
		char strArg2[16];
		sprintf(strArg2,"%d",item->iPrice);
		itemList.push_back(strArg2);
	}
	int size = itemList.size();

	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(itemList[i].c_str());
		}

		g_pBuyMenu->GetFlashPlayer()->SetVariableArray(FVAT_ConstStrPtr, "POPUP.POPUP_NewPackage.m_allValues", 0, &pushArray[0], size);
	}
	g_pBuyMenu->Invoke("POPUP.POPUP_NewPackage.updateContentList");
}

//-----------------------------------------------------------------------------------------------------


void CHUDPowerStruggle::UpdatePackageItemList(const char *page)
{
	if (!g_pGame->GetIGameFramework()->GetClientActor())
		return;

	if(!g_pBuyMenu)
		return;

	IFlashPlayer *pFlashPlayer = g_pBuyMenu->GetFlashPlayer();

	pFlashPlayer->Invoke0("POPUP.POPUP_NewPackage.clearAllEntities");

	std::vector<SItem> itemList;
	EBuyMenuPage itemType = ConvertToBuyList(page);

	GetItemList(itemType, itemList);

	EntityId uiPlayerID = g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId();
	int playerTeam = g_pGame->GetGameRules()->GetTeam(uiPlayerID);
	CGameRules *pGameRules = g_pGame->GetGameRules();

	std::vector<string> itemArray;
	char tempBuf[256];

	//std::sort(itemList.begin(),itemList.end(),SortByPrice);

	for(std::vector<SItem>::iterator iter=itemList.begin(); iter!=itemList.end(); ++iter)
	{
		SItem item = (*iter);
		if(item.iPrice == 0) continue;

		itemArray.push_back(item.strName.c_str());
		itemArray.push_back(item.strDesc.c_str());
		_snprintf(tempBuf,sizeof(tempBuf),"%d",item.iPrice);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemArray.push_back(tempBuf);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",(int)item.iInventoryID);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemArray.push_back(tempBuf);
		_snprintf(tempBuf,sizeof(tempBuf),"%d",(int)item.isUnique);
		tempBuf[sizeof(tempBuf)-1]='\0';
		itemArray.push_back(tempBuf);
	}

	int size = itemArray.size();

	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(itemArray[i].c_str());
		}

		pFlashPlayer->SetVariableArray(FVAT_ConstStrPtr, "POPUP.POPUP_NewPackage.m_allValues", 0, &pushArray[0], size);
	}

	pFlashPlayer->Invoke0("POPUP.POPUP_NewPackage.updatePackageList");
}

//-----------------------------------------------------------------------------------------------------

void CHUDPowerStruggle::UpdateBuyZone(bool trespassing, EntityId zone)
{
	if(zone)
	{
		if(trespassing)
		{
			if(!stl::find(m_currentBuyZones, zone))
				m_currentBuyZones.push_back(zone);
		}
		else
		{
			std::vector<EntityId>::iterator it = m_currentBuyZones.begin();
			for(; it != m_currentBuyZones.end(); ++it)
			{
				if(*it == zone)
				{
					m_currentBuyZones.erase(it);
					break;
				}
			}
		}
	}

	m_factoryTypes[0] = false;
	m_factoryTypes[1] = false;
	m_factoryTypes[2] = false;
	m_factoryTypes[3] = false;
	m_factoryTypes[4] = false;

	if (IActor *pActor=g_pGame->GetIGameFramework()->GetClientActor())
	{
		//check whether the player is in a buy zone, he can use ...
		EntityId uiPlayerID = pActor->GetEntityId();
		int playerTeam = g_pGame->GetGameRules()->GetTeam(uiPlayerID);
		int inBuyZone = false;
		CGameRules *pGameRules = g_pGame->GetGameRules();
		{
			std::vector<EntityId>::const_iterator it = m_currentBuyZones.begin();
			for(; it != m_currentBuyZones.end(); ++it)
			{
				if(pGameRules->GetTeam(*it) == playerTeam)
				{
					if(IsFactoryType(*it,E_WEAPONS))
						m_factoryTypes[0] = true;
					if(IsFactoryType(*it,E_AMMO))
						m_factoryTypes[1] = true;
					if(IsFactoryType(*it,E_EQUIPMENT))
						m_factoryTypes[2] = true;
					if(IsFactoryType(*it,E_VEHICLES))
						m_factoryTypes[3] = true;
					if(IsFactoryType(*it,E_PROTOTYPES))
						m_factoryTypes[4] = true;
					inBuyZone = true;
				}
			}
		}

		if(inBuyZone)
		{
			IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor();
			if(pActor && pActor->GetLinkedVehicle())
				inBuyZone = 2;	//this is a service zone
		}

		if(g_pHUD->IsBuyMenuActive())
		{
			SFlashVarValue arg[5] = {(int)m_factoryTypes[0], (int)m_factoryTypes[1], (int)m_factoryTypes[2], (int)m_factoryTypes[3], (int)m_factoryTypes[4]};
			g_pBuyMenu->CheckedInvoke("Root.PDAArea.Tabs.activateTabs",arg,5);
		}

		//update buy zone
		m_bInBuyZone = (inBuyZone)?true:false;
		if(g_pBuyIcon)
		{
			if(inBuyZone)
			{
				g_pBuyIcon->SetVisible(true);
				g_pBuyIcon->Invoke("setCapturing", false);
			}
			else
				g_pBuyIcon->SetVisible(false);
		}

	}
}

//-----------------------------------------------------------------------------------------------------

bool CHUDPowerStruggle::IsFactoryType(EntityId entity, EBuyMenuPage type)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entity);
	if(!pEntity) return false;
	int iBuyZoneFlags = g_pHUD->CallScriptFunction(pEntity,"GetBuyFlags");
	if(iBuyZoneFlags&((int)type))
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CHUDPowerStruggle::CanBuild(IEntity *pEntity, const char *vehicle)
{
	if(!pEntity) return false;
	IScriptTable *pScriptTable = pEntity->GetScriptTable();
	HSCRIPTFUNCTION scriptFuncHelper = NULL;

	bool buyable = false;
	//sum up available vehicles if tech level is high enough
	if(pScriptTable->GetValue("CanBuild", scriptFuncHelper) && scriptFuncHelper)
	{
		Script::CallReturn(gEnv->pScriptSystem, scriptFuncHelper, pScriptTable, vehicle, buyable);
		gEnv->pScriptSystem->ReleaseFunc(scriptFuncHelper);
		scriptFuncHelper = NULL;
	}
	return buyable;
}

//-----------------------------------------------------------------------------------------------------
void CHUDPowerStruggle::HideSOM(bool hide)
{
	if(hide)
	{
		if(m_animSwingOMeter.IsLoaded() && m_animSwingOMeter.GetFlashPlayer()->GetVisible())
			m_animSwingOMeter.GetFlashPlayer()->SetVisible(false);
	}
	else 
	{
		if(m_animSwingOMeter.IsLoaded())
			if(!m_animSwingOMeter.GetFlashPlayer()->GetVisible())
				m_animSwingOMeter.GetFlashPlayer()->SetVisible(true);
	}
}

void CHUDPowerStruggle::GetTeamStatus(int teamId, float &power, float &turretstatus, int &controlledAliens, EntityId &prototypeFactoryId)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	power=0.0f;
	if (pGameRules->GetSynchedGlobalValueType(300+teamId)==eSVT_Int)
	{
		int p;
		pGameRules->GetSynchedGlobalValue(300+teamId, p);
		power=(float)p;
	}
	else
		pGameRules->GetSynchedGlobalValue(300+teamId, power);

	int owned=0;
	for (std::vector<EntityId>::iterator it=m_powerpoints.begin(); it!=m_powerpoints.end(); it++)
	{
		if (pGameRules->GetTeam(*it)==teamId)
			owned++;
	}

	controlledAliens=owned;

	float totalTurretLife=0.0f;
	float currentTurretLife=0.0f;
	for (int t=0;t<m_turrets.size();t++)
	{
		if (pGameRules->GetTeam(m_turrets[t])!=teamId)
			continue;

		if (CItem *pItem=static_cast<CItem *>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_turrets[t])))
		{
			currentTurretLife+=MAX(0,pItem->GetStats().health);
			totalTurretLife+=pItem->GetProperties().hitpoints;
		}
	}

	turretstatus=currentTurretLife/totalTurretLife;

	// draw proto factory ownership indicator
	prototypeFactoryId=0;
	if (m_protofactory && pGameRules->GetTeam(m_protofactory)==teamId)
		prototypeFactoryId=m_protofactory;
}

void CHUDPowerStruggle::ShowConstructionProgress(bool show, bool queued, float time)
{
	m_constructing=show;
	m_constructionQueued=queued;
	m_constructionTime=time;
	m_constructionTimer=time;
}

void CHUDPowerStruggle::ReviveCycle(float total, float remaining)
{
	m_reviveCycle=total;
	m_reviveCycleEnd=gEnv->pTimer->GetFrameStartTime()+CTimeValue(remaining);
}