[Back](TechDoc_Architecture_System_TeamManager.md)

# Functionality #

## !STeamHarvesterDef ##
Below are the methods defined in the !STeamHarvesterDef object.

### **_Constructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_CheckFlag_** ###
**Purpose**:
Returns TRUE if the given flag is set

**Arguments**:
  * _nFlag_ - `[In]` Flag to check

**Returns**:
TRUE if flag is set, FALSE if not


### **_SetFlag_** ###
**Purpose**:
Set the flag specified

**Arguments**:
  * _bOn_ - `[In]` TRUE to turn it on, FALSE to clear it
  * _nFlag_ - `[In]` Flag to set

**Returns**:
TRUE on success, FALSE on failure


## !SVehSpawn ##
Below are the methods defined in the !SVehSpawn object.

## !STeamDef ##
Below are the methods defined in the !STeamDef object.

## TeamManager ##
Below are the methods defined in the TeamManager interface.

### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_Initialize_** ###
**Purpose**:
One-time initialization at the start

**Arguments**:
void

**Returns**:
void


### **_Shutdown_** ###
**Purpose**:
One-time clean up at the end

**Arguments**:
void

**Returns**:
void


### **_Reset_** ###
**Purpose**:
Clears all loaded teams and prepares for new team definitions

**Arguments**:
void

**Returns**:
void

**Note**:
Should be called at the start of a level load

### **_ResetGame_** ###
**Purpose**:
Called when the game is reset, such as when the editor game starts up

**Arguments**:
  * _bGameStart_ - `[In]` TRUE if game is starting, FALSE if game is stopping

**Returns**:
void


### **_Update_** ###
**Purpose**:
Update the teams

**Arguments**:
  * _bHaveFocus_ - `[In]` TRUE if game has focus
  * _nUpdateFlags_ - `[In]` Update flags

**Returns**:
void


### **_GetMemoryStatistics_** ###
**Purpose**:
Used by memory management

**Arguments**:
  * _s_ - `[In]` Cry Sizer object

**Returns**:
void


### **_PostInitClient_** ###
**Purpose**:
Called when the local client has finished loading for net synch issues

**Arguments**:
  * _nChannelID_ - `[In]` Network channel ID

**Returns**:
void


### **_CmdDebugTeams_** ###
**Purpose**:
Console command used to debug teams

**Arguments**:
  * _pArgs_ - `[In]` Console command arguments

**Returns**:
void

**Note**:
Delegated from GameRules

### **_CmdDebugObjectives_** ###
**Purpose**:
Console command used to debug team objectives

**Arguments**:
  * _pArgs_ - `[In]` Console command arguments
  * _status_ - `[In]` HUD Objectives status

**Returns**:
void

**Note**:
Delegated from GameRules

### **_LoadTeams_** ###
**Purpose**:
Load team definitions contained in an XML node

**Arguments**:
  * _pNode_ - `[In]` XML node to extract from

**Returns**:
TRUE on success or FALSE on error

**Note**:
Used in conjunction with CNCRules parsing

### **_CreateTeam_** ###
**Purpose**:
Create a team definition

**Arguments**:
  * _pTeamNode_ - `[In]` XML Node containing team data

**Returns**:
ID of the team created or TEAMID\_INVALID on error


### **_RemoveTeam_** ###
**Purpose**:
Remove a team and force anyone on that team onto another.

**Arguments**:
  * _nTeamID_ - `[In]` ID of the team to destroy

**Returns**:
void


### **_GetTeamName_** ###
**Purpose**:
Returns the name of the given team

**Arguments**:
  * _nTeamID_ - `[In]` ID of the team

**Returns**:
void


### **_GetTeamId_** ###
**Purpose**:
Returns the ID of the team with the given ID

**Arguments**:
  * _szName_ - `[In]` Name to search for

**Returns**:
void


### **_GetTeamCount_** ###
**Purpose**:
Returns how many teams are defined

**Arguments**:
void

**Returns**:
void


### **_GetTeamPlayerCount_** ###
**Purpose**:
Returns how many players belong to the given team

**Arguments**:
  * _bInGame_ - `[In]` TRUE to count only players who are currently in the game
  * _nTeamID_ - `[In]` ID of the team

**Returns**:
void


### **_GetTeamPlayer_** ###
**Purpose**:
Returns the player in the given slot on the specified team.

**Arguments**:
  * _nTeamID_ - `[In]` ID of the team
  * _nidx_ - `[In]` Index into the player list

**Returns**:
the ID of the entity in the slot


### **_GetTeamPlayers_** ###
**Purpose**:
Returns the list of players on the given team

**Arguments**:
  * _nTeamID_ - `[In]` ID of the team
  * _players_ - `[Out]` List of players

**Returns**:
void


### **_SetTeam_** ###
**Purpose**:
Put a player on the given team

**Arguments**:
  * _nEntityID_ - `[In]` ID of the entity to put on the team
  * _nTeamID_ - `[In]` ID of the team to use

**Returns**:
TRUE if the team was changed


### **_GetTeam_** ###
**Purpose**:
Get the team an entity belongs to

**Arguments**:
  * _nEntityID_ - `[In]` ID of the entity

**Returns**:
the ID of the team the entity belongs to


### **_SetTeamDefaultSpawnGroup_** ###
**Purpose**:
Sets the team's default spawn group

**Arguments**:
  * _nSpawnGroupId_ - `[In]` ID of spawn group to use
  * _nTeamID_ - `[In]` ID of team

**Returns**:
void


### **_GetTeamDefaultSpawnGroup_** ###
**Purpose**:
Get the spawn group ID being used by the given team

**Arguments**:
  * _nTeamID_ - `[In]` ID of team to get

**Returns**:
ID of the spawn group


### **_RemoveTeamDefaultSpawnGroup_** ###
**Purpose**:
Removes the spawn group being used by the given ID

**Arguments**:
  * _nTeamID_ - `[In]` ID of team

**Returns**:
void


### **_RemoveDefaultSpawnGroupFromTeams_** ###
**Purpose**:
Remove the spawn group from any team that is using it

**Arguments**:
  * _nSpawnGroupId_ - `[In]` ID of spawn group to remove

**Returns**:
void


### **_IsValidTeam_** ###
**Purpose**:
Returns TRUE if the specified team ID or name is valid

**Arguments**:
  * _nID_ - `[In]` Team ID
  * _szName_ - `[In]` Name of the team

**Returns**:
void

**Note**:
Using the name is slower than the ID!

### **_IsPlayerTeam_** ###
**Purpose**:
Returns TRUE if the specified team can be used by a player (for joining rights)

**Arguments**:
  * _nID_ - `[In]` Team ID
  * _szName_ - `[In]` Name of the team

**Returns**:
void

**Note**:
Using the name is slower than the ID!

### **_CreateTeamHarvester_** ###
**Purpose**:
Create a harvester for the team

**Arguments**:
  * _fDir_ - `[In]` Direction to face if not created through the vehicle factory
  * _nID_ - `[In]` Team ID
  * _ppDef_ - `[Out]` Harvester defintion (optional)
  * _szFactoryName_ - `[In]` Name of factory to create it at
  * _vPos_ - `[In]` Where to create the harvester at if not though the vehicle factory

**Returns**:
ID of harvester or HARVESTERID\_INVALID on error


### **_GetTeamHarvesterInfo_** ###
**Purpose**:
Return map of harvesters owned by the given team

**Arguments**:
  * _list_ - `[Out]` List of harvesters
  * _nID_ - `[In]` Team ID

**Returns**:
TRUE on success


### **_GetTeamHarvester_** ###
**Purpose**:
Return the harvestere definition

**Arguments**:
  * _nHarvesterID_ - `[In]` ID of the harvester
  * _nID_ - `[In]` Owning team ID

**Returns**:
STeamHarvesterDef object or NULL on error


### **_RemakeTeamHarvester_** ###
**Purpose**:
Remakes the entity for the given harvester

**Arguments**:
  * _bLeaveEntity_ - `[In]` TRUE if old entity should be left in the world
  * _pDef_ - `[In]` Harvester definition

**Returns**:
TRUE on success, FALSE on error


### **_RemoveTeamHarvesters_** ###
**Purpose**:
Remove all harvesters from the world that belong to the given team

**Arguments**:
  * _nTeamID_ - `[In]` Team ID

**Returns**:
void


### **_GetChannelTeam_** ###
**Purpose**:
Get the team that owns the given channel

**Arguments**:
  * _nChannelID_ - `[In]` Channel ID

**Returns**:
id of Team


### **_GetTeamChannelCount_** ###
**Purpose**:
Returns how many channels a team has

**Arguments**:
  * _bInGame_ - `[In]` TRUE to only count those in the game
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_SetEditorTeam_** ###
**Purpose**:
Set which team you are on in the editor

**Arguments**:
  * _bResetNow_ - `[In]` Reset local player's team now
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_SetTeamCredits_** ###
**Purpose**:
Set everyone on the team's credits

**Arguments**:
  * _nAmount_ - `[In]` Amount to set
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_GiveTeamCredits_** ###
**Purpose**:
Give everyone on the team credits

**Arguments**:
  * _nAmount_ - `[In]` Amount to give
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_TakeTeamCredits_** ###
**Purpose**:
Take from everyone on the team's credits

**Arguments**:
  * _nAmount_ - `[In]` Amount to take
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_AddTeamFlags_** ###
**Purpose**:
Add (turn on) flags for given team

**Arguments**:
  * _nFlags_ - `[In]` Flags to turn on
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_RemoveTeamFlags_** ###
**Purpose**:
Remove (turn off) flags for given team

**Arguments**:
  * _nFlags_ - `[In]` Flags to turn off
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_CheckTeamFlags_** ###
**Purpose**:
Check if team has given flags turned on

**Arguments**:
  * _nFlags_ - `[In]` Flags to check
  * _nTeamID_ - `[In]` team ID

**Returns**:
TRUE if flags are on


### **_AddVehicleSpawnLocation_** ###
**Purpose**:
Add a vehicle spawn location to the team

**Arguments**:
  * _bCanSpawnHarvester_ - `[In]` TRUE if harvesters can spawn here as well
  * _nEntityId_ - `[In]` ID of vehicle spawn point entity
  * _nTeamID_ - `[In]` team ID
  * _szClass_ - `[In]` Building class that can spawn here

**Returns**:
void


### **_RemoveVehicleSpawnLocation_** ###
**Purpose**:
Remove a vehicle spawn location from the team

**Arguments**:
  * _nEntityId_ - `[In]` ID of vehicle spawn point entity
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_GetVehicleSpawnLocations_** ###
**Purpose**:
Get the vehicle spawn locations for the given team

**Arguments**:
  * _locations_ - `[Out]` Spawn locations
  * _nTeamID_ - `[In]` team ID
  * _szClass_ - `[In]` Building class

**Returns**:
TRUE if locations were returned


### **_ValidateSpawnLocation_** ###
**Purpose**:
Check to see if the given spawn location is clear to let a vehicle spawn there

**Arguments**:
  * _nLocationId_ - `[In]` Vehicle spawn ID
  * _szVehicle_ - `[In]` Name of vehicle you want to spawn

**Returns**:
weight of spawn or -1 if invalid


### **_SpawnTeamVehicle_** ###
**Purpose**:
Spawn a vehicle at the given spawn location

**Arguments**:
  * _nLocationId_ - `[In]` Vehicle spawn ID
  * _szVehicle_ - `[In]` Vehicle to spawn

**Returns**:
created vehicle entity or NULL on error


## ScriptBind\_TeamManager ##
Below are the methods defined in the ScriptBind\_TeamManager module.

### **_Constructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_Destructor_** ###
**Purpose**:
Undefined.

**Arguments**:
void

**Returns**:
void


### **_AttachTo_** ###
**Purpose**:
Attaches binding to a team manager

**Arguments**:
void

**Returns**:
void


### **_GetTeam_** ###
**Purpose**:
Returns ID of team with given name

**Arguments**:
  * _szName_ - `[In]` Team name

**Returns**:
void


### **_GetTeamName_** ###
**Purpose**:
Returns name of given team

**Arguments**:
  * _nTeamID_ - `[In]` Team ID

**Returns**:
void


### **_SetEditorTeam_** ###
**Purpose**:
Set the team to use in the editor game

**Arguments**:
  * _nTeamID_ - `[In]` Team ID

**Returns**:
void


### **_SetEditorTeamByName_** ###
**Purpose**:
Set the team to use in the editor game using the name

**Arguments**:
  * _szTeam_ - `[In]` Team name

**Returns**:
void


### **_SetTeamCredits_** ###
**Purpose**:
Set everyone on the team's credits

**Arguments**:
  * _nAmount_ - `[In]` Amount to set
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_GiveTeamCredits_** ###
**Purpose**:
Give everyone on the team credits

**Arguments**:
  * _nAmount_ - `[In]` Amount to give
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_TakeTeamCredits_** ###
**Purpose**:
Take from everyone on the team's credits

**Arguments**:
  * _nAmount_ - `[In]` Amount to take
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_RemoveTeamHarvesters_** ###
**Purpose**:
Remove all harvesters from the world that belong to the given team

**Arguments**:
  * _nTeamID_ - `[In]` Team ID

**Returns**:
void


### **_AddTeamFlags_** ###
**Purpose**:
Add (turn on) flags for given team

**Arguments**:
  * _nFlags_ - `[In]` Flags to turn on
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_RemoveTeamFlags_** ###
**Purpose**:
Remove (turn off) flags for given team

**Arguments**:
  * _nFlags_ - `[In]` Flags to turn off
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_CheckTeamFlags_** ###
**Purpose**:
Check if team has given flags turned on

**Arguments**:
  * _nFlags_ - `[In]` Flags to check
  * _nTeamID_ - `[In]` team ID

**Returns**:
TRUE if flags are on


### **_AddVehicleSpawnLocation_** ###
**Purpose**:
Add a vehicle spawn location to the team

**Arguments**:
  * _bCanSpawnHarvester_ - `[In]` TRUE if harvesters can spawn here as well
  * _nEntityId_ - `[In]` ID of vehicle spawn point entity
  * _nTeamID_ - `[In]` team ID
  * _szClass_ - `[In]` Building class that can spawn here

**Returns**:
void


### **_RemoveVehicleSpawnLocation_** ###
**Purpose**:
Remove a vehicle spawn location from the team

**Arguments**:
  * _nEntityId_ - `[In]` ID of vehicle spawn point entity
  * _nTeamID_ - `[In]` team ID

**Returns**:
void


### **_GetVehicleSpawnLocations_** ###
**Purpose**:
Get the vehicle spawn locations for the given team

**Arguments**:
  * _nTeamID_ - `[In]` team ID
  * _szClass_ - `[In]` Building class

**Returns**:
table of spawn locations

**Note**:
spawn = { bCanSpawnHarvester, nLocationId, nId }

### **_ValidateSpawnLocation_** ###
**Purpose**:
Check to see if the given spawn location is clear to let a vehicle spawn there

**Arguments**:
  * _nLocationId_ - `[In]` Vehicle spawn ID
  * _szVehicle_ - `[In]` Name of vehicle you want to spawn

**Returns**:
nil if location is not valid, otherwise weight of spawn location


### **_SpawnTeamVehicle_** ###
**Purpose**:
Spawn a vehicle at the given spawn location

**Arguments**:
  * _nLocationId_ - `[In]` Vehicle spawn ID
  * _szVehicle_ - `[In]` Name of vehicle you want to spawn

**Returns**:
created vehicle entity script table or nil on error


### **_RegisterGlobals_** ###
**Purpose**:
Registers any global values to the script system

**Arguments**:
void

**Returns**:
void


### **_RegisterMethods_** ###
**Purpose**:
Registers any binding methods to the script system

**Arguments**:
void

**Returns**:
void



# Sub Modules #

## !STeamDef ##
Description: This structure is used to represent a defined team. It contains information including the team's unique ID, the team's name, attribute and script properties.

Member Variables:
  * nID - Team's unique ID
  * nSpawnGroupID - Default spawn group ID used by team
  * nFlags - Team flags (see ETeamFlags)
  * szName - Name of the team
  * szLongName - Long name of the team
  * szScript - Lua script used by the team
  * PlayerList - List of player IDs who are on the team
  * DefHarvester - Default Harvester definition inherited by all harvesters created on the team
  * HarvesterList - List of Harvesters owned by the team
  * VehicleSpawnList - List of all valid vehicle spawn points for the team


## !STeamHarvesterDef ##
Description: This structure is used to represent a harvester owned by a team. It contains information including the harvester's unique ID, the owning team's ID, the vehicle entity's ID, attributes and helper properties. A reference to this definition is contained in each [Harvester Controller](TechDoc_Architecture_Game_HarvesterFlow.md).

Member Variables:
  * fCapacity - How much tiberium the harvester can hold
  * fBuildTime - How long it takes for the harvester to be rebuilt once destroyed
  * fLoadRate - How long it takes the harvester to load one piece of tiberium
  * fUnloadRate - How long it takes the harvester to unload one piece of tiberium
  * fPayloadTimer - Used to record how long the harvester has been loading/unloading
  * fPayload - How much tiberium is currently in the tank
  * ucFlags - Harvester flags
  * vCreateAt - Where harvester is created at if it doesn't go through the vehicle factory
  * fCreateDir - Degree of facing when harvester is created
  * nTeamID - Owning team's ID
  * nID - Harvester's unique ID
  * nEntityID - Vehicle entity ID used to represent the harvester
  * szEntityName - Vehicle entity name to use to represent the harvester
  * SignalQueue - Queue of signals that need processing

## !ETeamFlags ##
Below are the list of valid Team Flags and their meanings:

  * TEAM\_FLAG\_ISPLAYERTEAM - Set if player can be on this team, or strictly AI

[Back](TechDoc_Architecture_System_TeamManager.md)