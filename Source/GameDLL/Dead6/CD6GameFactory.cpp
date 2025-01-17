////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Software & Designs, 2007
//
// CD6GameFactory.cpp
//
// Purpose: Dead6 Core Game factory for registering
//	our own gamerules and factory classes
//
// Note: Keep up-to-date with changes from GameFactory.cpp!
//
// File History:
//	- 7/22/07 : File created - KAK
////////////////////////////////////////////////////

#include "stdafx.h"
#include "CD6GameFactory.h"
#include "CD6Game.h"
#include "CD6GameRules.h"

// Dead6 classes
#include "CD6Player.h"
#include "CVehicleMovementHarvester.h"

// Crysis classes
#include "Item.h"
#include "Weapon.h"
#include "VehicleWeapon.h"
#include "AmmoPickup.h"
#include "Binocular.h"
#include "C4.h"
#include "C4Detonator.h"
#include "DebugGun.h"
#include "PlayerFeature.h"
#include "ReferenceWeapon.h"
#include "OffHand.h"
#include "Fists.h"
#include "Lam.h"
#include "GunTurret.h"
#include "ThrowableWeapon.h"
#include "RocketLauncher.h"
#include "VehicleMovementBase.h"
#include "VehicleActionAutomaticDoor.h"
#include "VehicleActionDeployRope.h"
#include "VehicleActionEntityAttachment.h"
#include "VehicleActionLandingGears.h"
#include "VehicleDamageBehaviorBurn.h"
#include "VehicleDamageBehaviorCameraShake.h"
#include "VehicleDamageBehaviorExplosion.h"
#include "VehicleDamageBehaviorTire.h"
#include "VehicleMovementStdWheeled.h"
#include "VehicleMovementHovercraft.h"
#include "VehicleMovementHelicopter.h"
#include "VehicleMovementStdBoat.h"
#include "VehicleMovementTank.h"
#include "VehicleMovementVTOL.h"
#include "VehicleMovementAmphibious.h"
#include "ScriptControlledPhysics.h"
#include "HUD/HUD.h"

#include <IItemSystem.h>
#include <IVehicleSystem.h>
#include <IGameRulesSystem.h>

#define HIDE_FROM_EDITOR(className)																																				\
  { IEntityClass *pItemClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className);\
  pItemClass->SetFlags(pItemClass->GetFlags() | ECLF_INVISIBLE); }																				\

#define REGISTER_GAME_OBJECT(framework, name, script)\
	{\
		IEntityClassRegistry::SEntityClassDesc clsDesc;\
		clsDesc.sName = #name;\
		clsDesc.sScriptFile = script;\
		struct C##name##Creator : public IGameObjectExtensionCreatorBase\
		{\
			C##name *Create()\
			{\
				return new C##name();\
			}\
			void GetGameObjectExtensionRMIData( void ** ppRMI, size_t * nCount )\
			{\
			C##name::GetGameObjectExtensionRMIData( ppRMI, nCount );\
			}\
		};\
		static C##name##Creator _creator;\
		framework->GetIGameObjectSystem()->RegisterExtension(#name, &_creator, &clsDesc);\
	}

#define REGISTER_GAME_OBJECT_EXTENSION(framework, name)\
	{\
		struct C##name##Creator : public IGameObjectExtensionCreatorBase\
		{\
		C##name *Create()\
			{\
			return new C##name();\
			}\
			void GetGameObjectExtensionRMIData( void ** ppRMI, size_t * nCount )\
			{\
			C##name::GetGameObjectExtensionRMIData( ppRMI, nCount );\
			}\
		};\
		static C##name##Creator _creator;\
		framework->GetIGameObjectSystem()->RegisterExtension(#name, &_creator, NULL);\
	}

////////////////////////////////////////////////////
void InitD6GameFactory(IGameFramework *pFramework)
{
	assert(pFramework);
	
////////////////////////////////////////////////////
// !BEGIN KEEP UP TO DATE FROM: GameFactory.cpp
////////////////////////////////////////////////////
	// TODO Replace with our content when applicable

	REGISTER_FACTORY(pFramework, "Player", CD6Player, false);
	REGISTER_FACTORY(pFramework, "Grunt", CPlayer, true);
	REGISTER_FACTORY(pFramework, "Civilian", CPlayer, true);

	// Items
	REGISTER_FACTORY(pFramework, "Item", CItem, false);
	REGISTER_FACTORY(pFramework, "PlayerFeature", CPlayerFeature, false);
	REGISTER_FACTORY(pFramework, "LAM", CLam, false);

	// Weapons
	REGISTER_FACTORY(pFramework, "Weapon", CWeapon, false);
	REGISTER_FACTORY(pFramework, "VehicleWeapon", CVehicleWeapon, false);
	REGISTER_FACTORY(pFramework, "AmmoPickup", CAmmoPickup, false);
	REGISTER_FACTORY(pFramework, "AVMine", CThrowableWeapon, false);
	REGISTER_FACTORY(pFramework, "Claymore", CThrowableWeapon, false);
	REGISTER_FACTORY(pFramework, "Binocular", CBinocular, false);
	REGISTER_FACTORY(pFramework, "C4", CC4, false);
	REGISTER_FACTORY(pFramework, "C4Detonator", CC4Detonator, false);
	REGISTER_FACTORY(pFramework, "DebugGun", CDebugGun, false);
	REGISTER_FACTORY(pFramework, "ReferenceWeapon", CReferenceWeapon, false);
	REGISTER_FACTORY(pFramework, "OffHand", COffHand, false);
	REGISTER_FACTORY(pFramework, "Fists", CFists, false);
	REGISTER_FACTORY(pFramework, "GunTurret", CGunTurret, false);
	REGISTER_FACTORY(pFramework, "RocketLauncher", CRocketLauncher, false);

	// vehicle objects
	IVehicleSystem* pVehicleSystem = pFramework->GetIVehicleSystem();

#define REGISTER_VEHICLEOBJECT(name, obj) \
	REGISTER_FACTORY((IVehicleSystem*)pVehicleSystem, name, obj, false); \
	obj::m_objectId = pVehicleSystem->AssignVehicleObjectId();

	REGISTER_VEHICLEOBJECT("Burn", CVehicleDamageBehaviorBurn);
	REGISTER_VEHICLEOBJECT("CameraShake", CVehicleDamageBehaviorCameraShake);
	REGISTER_VEHICLEOBJECT("Explosion", CVehicleDamageBehaviorExplosion);
	REGISTER_VEHICLEOBJECT("BlowTire", CVehicleDamageBehaviorBlowTire);
	REGISTER_VEHICLEOBJECT("AutomaticDoor", CVehicleActionAutomaticDoor);
	REGISTER_VEHICLEOBJECT("DeployRope", CVehicleActionDeployRope);
	REGISTER_VEHICLEOBJECT("EntityAttachment", CVehicleActionEntityAttachment);
	REGISTER_VEHICLEOBJECT("LandingGears", CVehicleActionLandingGears);

	// vehicle movements
	REGISTER_FACTORY(pVehicleSystem, "Hovercraft", CVehicleMovementHovercraft, false);
	REGISTER_FACTORY(pVehicleSystem, "Helicopter", CVehicleMovementHelicopter, false);
	REGISTER_FACTORY(pVehicleSystem, "StdBoat", CVehicleMovementStdBoat, false);
	REGISTER_FACTORY(pVehicleSystem, "StdWheeled", CVehicleMovementStdWheeled, false);
	REGISTER_FACTORY(pVehicleSystem, "Tank", CVehicleMovementTank, false);
	REGISTER_FACTORY(pVehicleSystem, "VTOL", CVehicleMovementVTOL, false);
	REGISTER_FACTORY(pVehicleSystem, "Amphibious", CVehicleMovementAmphibious, false);
	REGISTER_FACTORY(pVehicleSystem, "Harvester", CVehicleMovementHarvester, false);

	// Custom Extensions
	//REGISTER_GAME_OBJECT(pFramework, CustomFreezing);
	//REGISTER_GAME_OBJECT(pFramework, CustomShatter);

	// Load our gamerules in here
	REGISTER_FACTORY(pFramework, "GameRules", CD6GameRules, false);
	//g_D6Core->pD6GameRules = (CD6GameRules*)(g_D6Core->pD6Game->GetIGameFramework()->GetIGameRulesSystem());

	REGISTER_GAME_OBJECT_EXTENSION(pFramework, ScriptControlledPhysics);

	// TODO Replace with only our gamerules
#ifndef CRYSIS_BETA
	pFramework->GetIGameRulesSystem()->RegisterGameRules("SinglePlayer", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("SinglePlayer", "sp");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("InstantAction", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("InstantAction", "ia");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("InstantAction", "dm");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("InstantAction", "multiplayer/ia/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("TeamInstantAction", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("TeamInstantAction", "tia");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("TeamInstantAction", "tdm");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("TeamInstantAction", "multiplayer/tia/");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("TeamInstantAction", "multiplayer/ia/");

	//pFramework->GetIGameRulesSystem()->RegisterGameRules("TeamAction", "GameRules");
#endif

////////////////////////////////////////////////////
// !END
////////////////////////////////////////////////////
}