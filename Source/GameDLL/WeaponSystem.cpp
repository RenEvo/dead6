/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 18:10:2005   18:00 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include <IEntitySystem.h>
#include <ICryPak.h>
#include <IScriptSystem.h>
#include "IGameObject.h"
#include "Actor.h"
#include "ItemParams.h"
#include "WeaponSystem.h"

#include "Projectile.h"
#include "Bullet.h"
#include "Rock.h"
#include "Rocket.h"
#include "HomingMissile.h"
#include "TacBullet.h"
#include "TagBullet.h"
#include "TacAlienBullet.h"
#include "AVMine.h"
#include "Claymore.h"
#include "EMPField.h"

#include "Single.h"
#include "Automatic.h"
#include "Burst.h"
#include "Rapid.h"
#include "Throw.h"
#include "Plant.h"
#include "Detonate.h"
#include "FreezingBeam.h"
#include "Charge.h"
#include "Shotgun.h"
#include "Tactical.h"
#include "Melee.h"
#include "Heal.h"
#include "Repair.h"
#include "Energise.h"
#include "Scan.h"


#include "IronSight.h"
#include "Scope.h"


template <typename T, typename R> R *CreateIt() { return new T(); };



#define REGISTER_PROJECTILE(name, T)	\
struct C##name##Creator : public IGameObjectExtensionCreatorBase	\
{ \
	T *Create() \
	{ \
		return new T(); \
	} \
	void GetGameObjectExtensionRMIData( void ** ppRMI, size_t * nCount ) \
	{ \
		T::GetGameObjectExtensionRMIData( ppRMI, nCount ); \
	} \
}; \
static C##name##Creator _##name##Creator; \
RegisterProjectile(#name, &_##name##Creator); \

//------------------------------------------------------------------------
CWeaponSystem::CWeaponSystem(CGame *pGame, ISystem *pSystem)
: m_pGame(pGame),
	m_pSystem(pSystem),
	m_pItemSystem(pGame->GetIGameFramework()->GetIItemSystem()),
	m_pPrecache(0),
	m_reloading(false),
	m_recursing(false)
{
	// register fire modes here
	RegisterFireMode("Single", &CreateIt<CSingle, IFireMode>);
	RegisterFireMode("Automatic", &CreateIt<CAutomatic, IFireMode>);
	RegisterFireMode("Burst", &CreateIt<CBurst, IFireMode>);
	RegisterFireMode("Rapid", &CreateIt<CRapid, IFireMode>);
	RegisterFireMode("Throw", &CreateIt<CThrow, IFireMode>);
	RegisterFireMode("Plant", &CreateIt<CPlant, IFireMode>);
	RegisterFireMode("Detonate", &CreateIt<CDetonate, IFireMode>);
	RegisterFireMode("FreezingBeam", &CreateIt<CFreezingBeam, IFireMode>);
	RegisterFireMode("Charge", &CreateIt<CCharge, IFireMode>);
	RegisterFireMode("Shotgun", &CreateIt<CShotgun, IFireMode>);
	//RegisterFireMode("Tactical", &CreateIt<CTactical, IFireMode>);		
	RegisterFireMode("Melee", &CreateIt<CMelee, IFireMode>);
	RegisterFireMode("Heal", &CreateIt<CHeal, IFireMode>);
	RegisterFireMode("Repair", &CreateIt<CRepair, IFireMode>);
	RegisterFireMode("Energise", &CreateIt<CEnergise, IFireMode>);
	RegisterFireMode("Scan", &CreateIt<CScan, IFireMode>);
  
	// register zoom modes here
	RegisterZoomMode("IronSight", &CreateIt<CIronSight, IZoomMode>);
	RegisterZoomMode("Scope", &CreateIt<CScope, IZoomMode>);

	// register projectile classes here
	REGISTER_PROJECTILE(Projectile, CProjectile);
	REGISTER_PROJECTILE(Bullet, CBullet);
	REGISTER_PROJECTILE(Rock, CRock);
	REGISTER_PROJECTILE(Rocket, CRocket);
  REGISTER_PROJECTILE(HomingMissile, CHomingMissile);
	REGISTER_PROJECTILE(TacBullet, CTacBullet);
	REGISTER_PROJECTILE(TagBullet, CTagBullet);
	//REGISTER_PROJECTILE(TacAlienBullet, CTacAlienBullet);
	REGISTER_PROJECTILE(AVExplosive, CAVMine);
	REGISTER_PROJECTILE(ClaymoreExplosive, CClaymore);
	REGISTER_PROJECTILE(EMPField, CEMPField);

	m_pPrecache = gEnv->pConsole->GetCVar("i_precache");

	CBullet::EntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Bullet");
	//Didn't find any better place ...
	CBullet::SetWaterMaterialId();

	m_pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);
}

//------------------------------------------------------------------------
CWeaponSystem::~CWeaponSystem()
{
	// cleanup current projectiles
	for (TProjectileMap::iterator pit = m_projectiles.begin(); pit != m_projectiles.end(); ++pit)
		gEnv->pEntitySystem->RemoveEntity(pit->first, true);

	for (TAmmoTypeParams::iterator it = m_ammoparams.begin(); it != m_ammoparams.end(); ++it)
	{
		delete it->second;
	}

	m_pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);
}

//------------------------------------------------------------------------
void CWeaponSystem::Update(float frameTime)
{
	m_tracerManager.Update(frameTime);
}

//------------------------------------------------------------------------
void CWeaponSystem::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CWeaponSystem::Reload()
{
	m_reloading = true;

	// cleanup current projectiles
	for (TProjectileMap::iterator pit = m_projectiles.begin(); pit != m_projectiles.end();)
    {
        //Bugfix: RemoveEntity removes projectile from map, thus invalidating iterator
        TProjectileMap::iterator next = pit;        
        next++;
        gEnv->pEntitySystem->RemoveEntity(pit->first, true);
        pit = next;
    }
	m_projectiles.clear();

	for (TAmmoTypeParams::iterator it = m_ammoparams.begin(); it != m_ammoparams.end(); ++it)
	{
		delete it->second;
	}
	m_ammoparams.clear();

	m_tracerManager.Reset();

	for (TFolderList::iterator it=m_folders.begin(); it!=m_folders.end(); ++it)
		Scan(it->c_str());

	m_reloading = false;
}

//------------------------------------------------------------------------
void CWeaponSystem::OnLoadingComplete(ILevel *pLevel)
{
	// marcio: precaching of items enabled by default for now
//	ICVar *sys_preload=m_pSystem->GetIConsole()->GetCVar("sys_preload");
//	if ((!sys_preload || sys_preload->GetIVal()) && m_pPrecache->GetIVal())
	{
		for (TAmmoTypeParams::iterator it=m_ammoparams.begin(); it!=m_ammoparams.end(); ++it)
		{
			const IItemParamsNode *params = it->second->pItemParams;
			const IItemParamsNode *geometry = params?params->GetChild("geometry"):0;

			m_pItemSystem->CacheGeometry(geometry);

			// Preload particle assets.
			for (int ch = params->GetChildCount()-1; ch >= 0; ch--)
			{
				const IItemParamsNode *child = params->GetChild(ch);
				if (child)
				{
					CItemParamReader reader(child);
					const char *effect = 0;
					reader.Read("effect", effect);
					if (effect && *effect)
						gEnv->p3DEngine->FindParticleEffect(effect, "WeaponSystem");
				}
			}
		}
	}
}

//------------------------------------------------------------------------
IFireMode *CWeaponSystem::CreateFireMode(const char *name)
{
	TFireModeRegistry::iterator it = m_fmregistry.find(CONST_TEMP_STRING(name));
	if (it != m_fmregistry.end())
		return it->second();
	return 0;
}

//------------------------------------------------------------------------
void CWeaponSystem::RegisterFireMode(const char *name, IFireMode *(*CreateProc)())
{
	m_fmregistry.insert(TFireModeRegistry::value_type(name, CreateProc));
}

//------------------------------------------------------------------------
IZoomMode *CWeaponSystem::CreateZoomMode(const char *name)
{
	TZoomModeRegistry::iterator it = m_zmregistry.find(CONST_TEMP_STRING(name));
	if (it != m_zmregistry.end())
		return it->second();
	return 0;
}

//------------------------------------------------------------------------
void CWeaponSystem::RegisterZoomMode(const char *name, IZoomMode *(*CreateProc)())
{
	m_zmregistry.insert(TZoomModeRegistry::value_type(name, CreateProc));
}

//------------------------------------------------------------------------
CProjectile *CWeaponSystem::SpawnAmmo(IEntityClass* pAmmoType, bool isRemote)
{
	TAmmoTypeParams::const_iterator it = m_ammoparams.find(pAmmoType);
	if (it == m_ammoparams.end())
	{
		GameWarning("Failed to spawn ammo '%s'! Unknown class or entity class not registered...", pAmmoType?pAmmoType->GetName():"");
		return 0;
	}

	const SAmmoParams* pAmmoParams = it->second;

	bool isServer=gEnv->bServer;
	bool isClient=gEnv->bClient;

	if (pAmmoParams->serverSpawn && !isServer)
	{
		if (!pAmmoParams->predictSpawn || isRemote)
			return 0;
	}

	SEntitySpawnParams spawnParams;
	spawnParams.pClass = pAmmoType;
	spawnParams.sName = "";
	spawnParams.nFlags = pAmmoParams->flags | ENTITY_FLAG_NO_PROXIMITY; // No proximity for this entity.

	IEntity *pEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams);
	if (!pEntity)
	{
		GameWarning("Failed to spawn ammo '%s'! Entity creation failed...", pAmmoType->GetName());
		return 0;
	}

	CProjectile *pProjectile = GetProjectile(pEntity->GetId());

	if (pProjectile && !isServer && !isRemote && pAmmoParams->predictSpawn)
		pProjectile->GetGameObject()->RegisterAsPredicted();
	
	return pProjectile;
}

//------------------------------------------------------------------------
bool CWeaponSystem::IsServerSpawn(IEntityClass* pAmmoType) const
{
	if (!pAmmoType)
		return false;

	TAmmoTypeParams::const_iterator it = m_ammoparams.find(pAmmoType);
	if (it == m_ammoparams.end())
		return false;

	return it->second->serverSpawn!=0;
}

//-------------------------------------------	-----------------------------
void CWeaponSystem::RegisterProjectile(const char *name, IGameObjectExtensionCreatorBase *pCreator)
{
	m_projectileregistry.insert(TProjectileRegistry::value_type(name, pCreator));
}

//------------------------------------------------------------------------
const SAmmoParams* CWeaponSystem::GetAmmoParams(IEntityClass* pAmmoType) const
{
	TAmmoTypeParams::const_iterator it = m_ammoparams.find(pAmmoType);
	if (it != m_ammoparams.end())
		return it->second;
	return 0;
}

//------------------------------------------------------------------------
void CWeaponSystem::AddProjectile(IEntity *pEntity, CProjectile *pProjectile)
{
	m_projectiles.insert(TProjectileMap::value_type(pEntity->GetId(), pProjectile));
}

//------------------------------------------------------------------------
void CWeaponSystem::RemoveProjectile(CProjectile *pProjectile)
{
	m_projectiles.erase(pProjectile->GetEntity()->GetId());
}

//------------------------------------------------------------------------
CProjectile *CWeaponSystem::GetProjectile(EntityId entityId)
{
	TProjectileMap::iterator it = m_projectiles.find(entityId);
	if (it != m_projectiles.end())
		return it->second;
	return 0;
}

//------------------------------------------------------------------------
int  CWeaponSystem::QueryProjectiles(SProjectileQuery& q)
{
    IEntityClass* pClass = q.ammoName?gEnv->pEntitySystem->GetClassRegistry()->FindClass(q.ammoName):0;
    m_queryResults.resize(0);
    if(q.box.IsEmpty())
    {
        for(TProjectileMap::iterator it = m_projectiles.begin();it!=m_projectiles.end();++it)
        {
            IEntity *pEntity = it->second->GetEntity();
            if(pClass == 0 || pEntity->GetClass() == pClass)
            m_queryResults.push_back(pEntity);
        }
    }
    else
    {
        for(TProjectileMap::iterator it = m_projectiles.begin();it!=m_projectiles.end();++it)
        {
            IEntity *pEntity = it->second->GetEntity();
            if(q.box.IsContainPoint(pEntity->GetWorldPos()))
            {
                m_queryResults.push_back(pEntity);
            }
        }
    }
    
    q.nCount = int(m_queryResults.size());
    if(q.nCount)
        q.pResults = &m_queryResults[0];
    return q.nCount;
}

//------------------------------------------------------------------------
void CWeaponSystem::Scan(const char *folderName)
{
	string folder = folderName;
	string search = folder;
	search += "/*.*";

	ICryPak *pPak = m_pSystem->GetIPak();

	_finddata_t fd;
	intptr_t handle = pPak->FindFirst(search.c_str(), &fd);

	if (!m_recursing)
		CryLog("Loading ammo XML definitions from '%s'!", folderName);

	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
				continue;

			if (fd.attrib & _A_SUBDIR)
			{
				string subName = folder+"/"+fd.name;
				if (m_recursing)
					Scan(subName.c_str());
				else
				{
					m_recursing=true;
					Scan(subName.c_str());
					m_recursing=false;
				}
				continue;
			}

			if (stricmp(PathUtil::GetExt(fd.name), "xml"))
				continue;

			string xmlFile = folder + string("/") + string(fd.name);
			XmlNodeRef rootNode = m_pSystem->LoadXmlFile(xmlFile.c_str());

			if (!rootNode)
			{
				GameWarning("Invalid XML file '%s'! Skipping...", xmlFile.c_str());
				continue;
			}

			if (!ScanXML(rootNode, xmlFile.c_str()))
				continue;

		} while (pPak->FindNext(handle, &fd) >= 0);
	}

	if (!m_recursing)
		CryLog("Finished loading ammo XML definitions from '%s'!", folderName);

	if (!m_reloading && !m_recursing)
		m_folders.push_back(folderName);
}

//------------------------------------------------------------------------
bool CWeaponSystem::ScanXML(XmlNodeRef &root, const char *xmlFile)
{
	if (strcmpi(root->getTag(), "ammo"))
		return false;

	const char *name = root->getAttr("name");
	if (!name)
	{
		GameWarning("Missing ammo name in XML '%s'! Skipping...", xmlFile);
		return false;
	}

	const char *className = root->getAttr("class");

	if (!className)
	{
		GameWarning("Missing ammo class in XML '%s'! Skipping...", xmlFile);
		return false;
	}

	TProjectileRegistry::iterator it = m_projectileregistry.find(CONST_TEMP_STRING(className));
	if (it == m_projectileregistry.end())
	{
		GameWarning("Unknown ammo class '%s' specified in XML '%s'! Skipping...", className, xmlFile);
		return false;
	}

	const char *scriptName = root->getAttr("script");
	IEntityClassRegistry::SEntityClassDesc classDesc;
	classDesc.sName = name;
	classDesc.sScriptFile = scriptName?scriptName:"";
	//classDesc.pUserProxyData = (void *)it->second;
	//classDesc.pUserProxyCreateFunc = &CreateProxy<CProjectile>;
	classDesc.flags |= ECLF_INVISIBLE;

	if (!m_reloading)
		m_pGame->GetIGameFramework()->GetIGameObjectSystem()->RegisterExtension(name, it->second, &classDesc);

	IItemParamsNode *params = m_pItemSystem->CreateParams();
	params->ConvertFromXML(root);

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
	assert(pClass);

	m_ammoparams.insert(TAmmoTypeParams::value_type(pClass, new SAmmoParams(params, pClass)));

	return true;
}

//------------------------------------------------------------------------
void CWeaponSystem::DebugGun(IConsoleCmdArgs *args)
{
  IGameFramework* pGF = gEnv->pGame->GetIGameFramework();  
  IItemSystem* pItemSystem = pGF->GetIItemSystem();
 
  IActor* pActor = pGF->GetClientActor();
  if (!pActor || !pActor->IsPlayer())
    return;

  IInventory *pInventory = pActor->GetInventory();
  if (!pInventory)
    return;  
  
  // give & select the debuggun 
	EntityId itemId = pInventory->GetItemByClass(CItem::sDebugGunClass);
  if (0 == itemId)        
  {
    // if actor doesn't have it, only give it in editor
    if (!GetISystem()->IsEditor())
      return;

		itemId = pItemSystem->GiveItem(pActor, CItem::sDebugGunClass->GetName(), false, true, true);
  }
  pItemSystem->SetActorItem(pActor, itemId, true);      
}

//------------------------------------------------------------------------
void CWeaponSystem::RefGun(IConsoleCmdArgs *args)
{
	IGameFramework* pGF = gEnv->pGame->GetIGameFramework();  
	IItemSystem* pItemSystem = pGF->GetIItemSystem();

	IActor* pActor = pGF->GetClientActor();
	if (!pActor || !pActor->IsPlayer())
		return;

	IInventory *pInventory = pActor->GetInventory();
	if (!pInventory)
		return;

	// give & select the refgun 
	EntityId itemId = pInventory->GetItemByClass(CItem::sRefWeaponClass);
	if (0 == itemId)        
	{
		// if actor doesn't have it, only give it in editor
		if (!GetISystem()->IsEditor())
			return;

		itemId = pItemSystem->GiveItem(pActor, CItem::sRefWeaponClass->GetName(), false, true, true);
	}
	pItemSystem->SetActorItem(pActor, itemId, true);   

}

void CWeaponSystem::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "WeaponSystem");
	int nSize = sizeof(*this);
	s->AddObject(this,nSize);

	m_tracerManager.GetMemoryStatistics(s);
	s->AddContainer(m_fmregistry);
	s->AddContainer(m_zmregistry);
	s->AddContainer(m_projectileregistry);
	s->AddContainer(m_folders);
	s->AddContainer(m_queryResults);

	{
		SIZER_SUBCOMPONENT_NAME(s, "AmmoParams");
		int nSize = m_ammoparams.size() * sizeof(TAmmoTypeParams::value_type);
		for (TAmmoTypeParams::iterator iter = m_ammoparams.begin(); iter != m_ammoparams.end(); ++iter)
		{
			nSize += iter->second->GetMemorySize();
		}
		s->AddObject(&m_ammoparams,nSize);
	}
	
	{
		SIZER_SUBCOMPONENT_NAME(s, "Projectiles");
		int nSize = m_projectiles.size() * sizeof(TProjectileMap::value_type);
		for (TProjectileMap::iterator iter = m_projectiles.begin(); iter != m_projectiles.end(); ++iter)
		{
			nSize += iter->second->GetMemorySize();
		}
		s->AddObject(&m_projectiles,nSize);
	}
}
