/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: HUD radar

-------------------------------------------------------------------------
History:
- 2006/2007: Jan M�ller

*************************************************************************/
#ifndef __HUDRADAR_H__
#define __HUDRADAR_H__

//-----------------------------------------------------------------------------------------------------

#include "HUDObject.h"
#include <deque>
#include <list>

class CGameFlashAnimation;
struct IActor;
class CGameRules;
class CPlayer;

#include "ILevelSystem.h"

const static int NUM_TAGGED_ENTITIES = 4;

//radar objects on mini map and radar
enum FlashRadarType	//don't change order (unless you change the flash asset)
{
	EFirstType = 0,
	ETank,
	EAPC,
	ECivilCar,
	ETruck,
	EHovercraft,
	ESpeedBoat,
	EPatrolBoat,
	ESmallBoat,
	ELTV,
	EHeli,
	EVTOL,
	EAAA,	// should be AAA
	ESoundEffect,		// is empty, but we have neither Tank nor APC as an icon
	ENuclearWeapon,
	ETechCharger,
	EWayPoint,
	EPlayer,
	ETaggedEntity,
	ESpawnPoint,
	EFactoryAir,
	EFactoryTank,
	EFactoryPrototype,
	EFactoryVehicle,
	EFactorySea,
	EINVALID1, //EBase,
	EBarracks,
	ESpawnTruck,
	EAmmoTruck,
	EHeadquarter2,
	EHeadquarter,
	//single player only icons
	EAmmoDepot,
	EMineField,
	EMachineGun,
	ECanalization,
	ETutorial,
	ESecretEntrance,
	EHitZone,
	EHitZoneNK,
	EAlienEnergySource,
	EAlienEnergySourcePlus,
	ELastType
};

//factions on mini map
enum FlashRadarFaction
{
	ENeutral,
	EFriend,
	EEnemy,
	EAggressor,
	ESelf
};

//types of objective icons shown on screen
enum FlashOnScreenIcon
{
	eOS_MissionObjective = 0,
	eOS_TACTank,
	eOS_TACWeapon,
	eOS_FactoryAir,
	eOS_FactoryTank,
	eOS_FactoryPrototypes,
	eOS_FactoryVehicle,
	eOS_FactoryNaval,
	eOS_HQKorean,
	eOS_HQUS,
	eOS_Spawnpoint,
	eOS_TeamMate,
	eOS_Purchase,
	eOS_Left,
	eOS_TopLeft,
	eOS_Top,
	eOS_TopRight,
	eOS_Right,
	eOS_BottomRight,
	eOS_Bottom,
	eOS_BottomLeft,
	eOS_AlienEnergyPoint,
	eOS_AlienEnergyPointPlus,
	eOS_HQTarget
};
//-----------------------------------------------------------------------------------------------------

class CHUDRadar : public CHUDObject
{

	friend class CHUD;

	struct RadarSound
	{
		Vec3		m_pos;
		float		m_intensity;
		float		m_spawnTime;
		bool		m_hasChild;
		unsigned int m_id;

		RadarSound(Vec3 pos, float intensity, int id) : m_pos(pos), m_intensity(intensity), m_id(id)
		{
			m_spawnTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			m_hasChild = false;
		}
	};

public:

	struct RadarEntity
	{
		EntityId	m_id;
		RadarEntity(EntityId id)
		{ m_id = id; }
		RadarEntity()
		{ m_id = 0; }

		bool operator<(RadarEntity ent)
		{
			if(ent.m_id > m_id)
				return true;
			return false;
		}
	};

	struct TempRadarEntity : public RadarEntity
	{
		FlashRadarType m_type;
		float			m_spawnTime;
		float			m_timeLimit;
		string		m_text;
		TempRadarEntity() : RadarEntity()
		{}
		TempRadarEntity(EntityId id, FlashRadarType type, float timeLimit = 0.0f, const char* text = NULL) : RadarEntity(id), m_type(type), m_timeLimit(timeLimit)
		{
			m_spawnTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			m_text = text;
		}
	};

	void AddTaggedEntity(EntityId iEntityId);

						CHUDRadar();
	virtual	~	CHUDRadar();

	virtual void PreUpdate();

	void GetMemoryStatistics(ICrySizer * s);

	//~ILevelSystemListener - handled by FlashMenuObject through HUD.cpp
	virtual void OnLoadingComplete(ILevel *pLevel);
	//~ILevelSystemListener

	//permanently add an entity to the radar
	void AddEntityToRadar(EntityId id);
	//add entity temporarily to the radar
	void AddEntityTemporarily(EntityId id, float time = 10.0f);
	//add story entity to the minimap
	void AddStoryEntity(EntityId id, FlashRadarType type = EWayPoint, const char* text = NULL);
	void RemoveStoryEntity(EntityId id);
	//show a single sound at entity(id) on radar
	void ShowSoundOnRadar(Vec3 pos, float intensity);
	//remove an entity from the radar (scanned entity, tac'd entity)
	void RemoveFromRadar(EntityId id);
	//this updates a list of all active missionObjective - tracked Entities
	void UpdateMissionObjective(EntityId id, bool active, const char* description);
	//sets the flash pda
	void SetFlashPDA(CGameFlashAnimation *flashPDA) {m_flashPDA = flashPDA;}
	//sets the flash Radar
	void SetFlashRadar(CGameFlashAnimation *flashRadar) {m_flashRadar = flashRadar;}
	//return nearby entities (non-item)
	ILINE const std::vector<EntityId>*  GetNearbyEntities() const { return &m_entitiesInProximity; }
	//return nearby items (which have entity_flaf_on_radar)
	ILINE const std::vector<EntityId>*	GetNearbyItems() const { return &m_itemsInProximity; }
	//buildings on radar
	ILINE std::vector<RadarEntity> *GetBuildings() {return &m_buildingsOnRadar;}
	//get a list of possible on screen objectives
	ILINE std::vector<EntityId> *GetObjectives() {return &m_possibleOnScreenObjectives;}
	//get a list of all the entities that were scanned by the binoculars
	ILINE const std::deque<RadarEntity>* GetEntitiesList() const { return &m_entitiesOnRadar; }
	//get a list of the tagged entities
	ILINE const EntityId* GetTaggedEntitiesList() const { return m_taggedEntities; }
	//is specified entity tagged ?
	bool IsEntityTagged(const EntityId &id) const;
	//reset all scanned entities ...
	void Reset();
	//reset the tagged entities
	ILINE void ResetTaggedEntities() { for(int i = 0; i < NUM_TAGGED_ENTITIES; ++i) m_taggedEntities[i] = 0; }
	//add all entities in proximity for 10 seconds or forever to the radar
	void StartBroadScan(bool useParameters = false, bool keepEntities = false, Vec3 pos = Vec3(0,0,0), float radius = 0.0f);
	//stop a broadScan (if scan wasn't finished yet, nothing happens)
	void StopBroadScan() { m_startBroadScanTime = 0.0f; }
	//get map grid coordinate of entity
	Vec2i GetMapGridPosition(IEntity *pEntity);
	//get map grid coordinate of world coord
	Vec2i GetMapGridPosition(float x, float y);
	//add/remove team mate from Radar/PDA (teammates show always up)
	void SetTeamMate(EntityId id, bool active);
	//add/remove team mate name from miniMap (mainly for multiplayer)
	void SelectTeamMate(EntityId id, bool active);
	//sets a jamming entity (just using position) - set id == 0 to disable
	void SetJammer(EntityId id, float radius = 0.0f);
	//minimap init
	virtual void InitMap();
	//zoom the minimap stepwise
	virtual bool ZoomPDA(bool bZoomDirection);
	//zoom the minimap stepless
	virtual bool ZoomChangePDA(float p_fValue);
	//set minimap dragging enabled/disabled
	virtual void SetDrag(bool enabled);
	//translates the minimap
	virtual bool DragMap(Vec2 p_vDir);
	//get dragging status
	virtual bool GetDrag() {return m_bDragMap;}
	//reload the minimap to the flash animation
	void ReloadMiniMap();
	//returns the selected teammates vector
	std::vector<EntityId> * GetSelectedTeamMates() { return &m_selectedTeamMates; }

	//show an entity on the radar for a short time only (default is one second)
	ILINE void ShowEntityTemporarily(FlashRadarType type, EntityId id, float timeLimit = 1.0f)
	{
		for(int e = 0; e < m_tempEntitiesOnRadar.size(); ++e)
			if(id == m_tempEntitiesOnRadar[e].m_id)
				return;
		m_tempEntitiesOnRadar.push_back(TempRadarEntity(id, type, timeLimit));
	}

	void Serialize(TSerialize ser);

	//entity classes for comparison
	IEntityClass *m_pVTOL, *m_pHeli, *m_pHunter, *m_pWarrior, *m_pAlien, *m_pTrooper, *m_pGrunt, *m_pPlayerClass,
		*m_pScout, *m_pTankUS, *m_pTankA, *m_pLTVUS, *m_pLTVA, *m_pAAA, *m_pTruck, *m_pAPCUS, *m_pAPCA, *m_pBoatCiv,
		*m_pHover, *m_pBoatUS, *m_pBoatA, *m_pCarCiv, *m_pParachute;

private:

	// CHUDObject
	virtual void	OnUpdate(float fDeltaTime,float fFadeValue);
	// ~CHUDObject

	float m_fTime;

	float					GetRadarSize(IEntity* entity, class CActor* actor);
	bool					IsOnRadar(EntityId id, int first, int last);
	void					AddToRadar(EntityId id);
	bool					ScanObject(EntityId id);
	void					QueueObject(EntityId id);
	bool					IsObjectInQueue(EntityId id);
	bool					IsNextObject(EntityId id);
	bool					CheckObject(IEntity *pEntity, const CCamera &camera, bool checkVelocity=false, bool checkVisibility=false);
	void					UpdateScanner(float frameTime);
	void					ResetScanner();
	void					GatherScannableObjects();
	EntityId			RayCastBinoculars(CPlayer *pPlayer);

	//activate minimap rendering
	ILINE void SetRenderMapOverlay(bool bActive) {	m_renderMiniMap = bActive;}
	//renders player position, AI positions and structures on the overview map (should be replaced in flash)
	void RenderMapOverlay();
	//get an entity's position on the minimap
	bool GetPosOnMap(float inX, float inY, float &outX, float &outY, bool flashCoordinates = true);
	bool GetPosOnMap(IEntity *pEntity, float &outX, float &outY, bool flashCoordinates = true);
	//load map info from file ...
	void LoadMiniMap(const char* mapPath);
	//computes the dimension of the minimap according to the current screen resolution
	void ComputeMiniMapResolution();
	//chooses the right icon for a given vehicle or building entity (class)
	FlashRadarType ChooseType(IEntity* pEntity, bool radarOnly = false);
	//chooses the right icon for a given synched entity type (ammo trucks, tac tanks ... special mp units)
	FlashRadarType GetSynchedEntityType(int type);
	//return whether the entity is friend or foe to the player
	FlashRadarFaction	FriendOrFoe(bool multiplayer, IActor *player, int team, IEntity *entity, CGameRules *pGameRules);
	//helper function to write (flash-) icon data to a double stream (implicitly casting for readability)
	int FillUpDoubleArray(std::vector<double> *doubleArray, double entityId, double iconID, double posX, double posY,
												double rotation, double faction, double scaleX = 100.0, double scaleY = 100.0, double isOnScreenObjective = false, double isCurrentSpawnPoint = false);
	//helper function to scan an area for entities - currently the results are saved in m_entitiesInProximity
	void ScanProximity(Vec3 &pos, float &radius);
	//calculates minimap zoom and translation
	void ComputePositioning(Vec2 playerpos, std::vector<double> *doubleArray);

	//texture handles
	int m_iTextureIDEnemy;
	int m_iTextureIDWaypoint;
	int m_iTextureIDPlayer;
	int m_iTextureIDVehicle;
	int m_iTextureIDAirVehicle;
	int m_iTextureIDSound;

	//sizes of different radar heights / entities
	float		 m_radarDotSizes[4];
	//helpers for Radar scanning
	EntityId m_lookAtObjectID;
	float		 m_lookAtTimer;
	EntityId m_scannerObjectID;
	float		 m_scannerTimer;
	float		 m_startBroadScanTime;
	std::deque<EntityId> m_scannerQueue;
	float		 m_scannerGatherTimer;
	//team / squad mates
	std::vector<EntityId> m_teamMates;
	//absolute radar mode
	bool			m_bAbsoluteMode;
	//in MP there is an enemy nearby
	int			 m_iMultiplayerEnemyNear;

	//Map positioning stuff
	bool m_initMap;
	float m_fPDAZoomFactor;
	float m_fPDATempZoomFactor;
	Vec2 m_vPDAMapTranslation;
	Vec2 m_vPDATempMapTranslation;
	bool m_bDragMap;

	//unique sound id
	unsigned int			 m_soundIdCounter;

	//flash pda
	CGameFlashAnimation *m_flashPDA;
	CGameFlashAnimation *m_flashRadar;

	//pda mini map data
	bool		m_renderMiniMap;
	float		m_mapDivX, m_mapDivY, m_mapGridSizeX, m_mapGridSizeY;
	int			m_miniMapStartX, m_miniMapStartY, m_miniMapEndX, m_miniMapEndY, m_mapDimX, m_mapDimY;
	int			m_startOnScreenX, m_startOnScreenY, m_endOnScreenX, m_endOnScreenY;
	string  m_currentLevel;

	//broad scan parameter
	float		m_bsRadius;
	bool		m_bsUseParameter, m_bsKeepEntries;
	Vec3		m_bsPosition;

	//radar jammer
	EntityId	m_jammerID;
	float			m_jammerRadius;
	float			m_jammingValue;

	//normal scanning timer
	float		m_lastScan;

	//flash optimization for the radar invokes
	float m_fLastViewRot, m_fLastCompassRot, m_fLastStealthValue, m_fLastStealthValueStatic, m_fLastFov;

	//entities in proximity of the player
	std::vector<EntityId> m_entitiesInProximity;
	//items in proximity of the player
	std::vector<EntityId> m_itemsInProximity;
	//tac-bullet marked (tagged) entities
	EntityId	m_taggedEntities[NUM_TAGGED_ENTITIES];
	int				m_taggedPointer;
	//temporarily added entities
	std::vector<TempRadarEntity> m_tempEntitiesOnRadar;
	//special story (SP only) entities
	std::vector<TempRadarEntity> m_storyEntitiesOnRadar;
	//entities on the normal radar
	std::deque<RadarEntity>		m_entitiesOnRadar;
	std::vector<RadarSound>		m_soundsOnRadar;
	std::map<EntityId, string>	m_missionObjectives;
	//additional entities on miniMap in multiplayer
	std::vector<RadarEntity>  m_buildingsOnRadar;
	//teammates that are selected in multiplayer (name is rendered on miniMap)
	std::vector<EntityId>			m_selectedTeamMates;
	//collection of all objectives to show them on screen
	std::vector<EntityId> m_possibleOnScreenObjectives;
	//level / minimap data
	ILevel		*m_pLevelData;

};

//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------