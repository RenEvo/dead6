#ifndef __GAMECVARS_H__
#define __GAMECVARS_H__

struct SCVars
{	
	float cl_fov;
	float cl_bob;
	float cl_tpvDist;
	float cl_tpvYaw;
	float cl_nearPlane;
	float cl_sprintShake;
	float cl_sensitivityZeroG;
	float cl_sensitivity;
	float cl_strengthscale;
	int		cl_invertMouse;
	int		cl_invertController;
	int		cl_crouchToggle;
	int		cl_fpBody;
	int   cl_hud;
	int		cl_enableGyroFade;
	int		cl_debugSwimming;
	int		cl_debugFreezeShake;
	float cl_frozenSteps;
	float cl_frozenSensMin;
	float cl_frozenSensMax;
	float cl_frozenAngleMin;
	float cl_frozenAngleMax;
	float cl_frozenMouseMult;
	float cl_frozenKeyMult;
	float cl_frozenSoundDelta;
	float cl_frozenShakeScreenMult;
	int		cl_tryme;
	int		cl_tryme_bt_speed;
	int		cl_tryme_bt_ironsight;
	float cl_tryme_targetx;
	float cl_tryme_targety;
	float cl_tryme_targetz;

  int   sv_votingTimeout;
  int   sv_votingCooldown;
  float sv_votingRatio;
  float sv_votingTeamRatio;

	int   sv_input_timeout;

	float hr_rotateFactor;
	float hr_rotateTime;
	float hr_dotAngle;
	float hr_fovAmt;
	float hr_fovTime;

	int		i_staticfiresounds;
	int		i_soundeffects;
	int		i_lighteffects;
	int		i_particleeffects;
	int		i_rejecteffects;
	float i_offset_front;
	float i_offset_up;
	float i_offset_right;
	int		i_unlimitedammo;
  
	float int_zoomAmount;
	float int_zoomInTime;
	float int_moveZoomTime;
	float int_zoomOutTime;

	float pl_inputAccel;

	float g_tentacle_joint_limit;
	int		g_godMode;
	int		g_detachCamera;
	int		g_enableSpeedLean;
	int   g_difficultyLevel;
	float g_walkMultiplier;
	float g_suitSpeedMult;
	float g_suitSpeedMultMultiplayer;
	float g_suitArmorHealthValue;
	float g_suitSpeedEnergyConsumption;
	float g_suitCloakEnergyDrainAdjuster;
	float g_AiSuitEnergyRechargeTime;
	float g_AiSuitHealthRegenTime;
	float g_AiSuitArmorModeHealthRegenTime;
	float g_playerSuitEnergyRechargeTime;
	float g_playerSuitHealthRegenTime;
	float g_playerSuitArmorModeHealthRegenTime;
	float g_frostDecay;
	float g_stanceTransitionSpeed;
	int   g_debugaimlook;
	int		g_enableIdleCheck;
	int		g_playerRespawns;
	float g_playerLowHealthThreshold;
	int		g_punishFriendlyDeaths;

	float g_pp_scale_income;
	float g_pp_scale_price;

	float g_dofset_minScale;
	float g_dofset_maxScale;
	float g_dofset_limitScale;

	float g_dof_minHitScale;
	float g_dof_maxHitScale;
	float g_dof_sampleAngle;
	float g_dof_minAdjustSpeed;
	float g_dof_maxAdjustSpeed;
	float g_dof_averageAdjustSpeed;
	float g_dof_distAppart;
	int		g_dof_ironsight;


	float g_radialBlur;
	int		g_playerFallAndPlay;

	float g_timelimit;
	float g_roundtime;
	int		g_preroundtime;
	int		g_suddendeathtime;
	int		g_roundlimit;
	int		g_fraglimit;
  int		g_fraglead;
  float g_friendlyfireratio;
  int   g_teamkillstokick;
  int   g_revivetime; 
  int   g_autoteambalance;
	int   g_minplayerlimit;
	int   g_minteamlimit;

	int   g_debugNetPlayerInput;
	int   g_debugCollisionDamage;

	float g_trooperProneMinDistance;
	/*	float g_trooperMaxPhysicAnimBlend;
	float g_trooperPhysicAnimBlendSpeed;
	float g_trooperPhysicsDampingAnim;
	*/
	float g_trooperTentacleAnimBlend;
	float g_alienPhysicsAnimRatio;  

	int		g_debug_fscommand;
	int		g_debugDirectMPMenu;
	int		g_skipIntro;
	int		g_useProfile;

	int   g_enableTracers;
	int		g_enableAlternateIronSight;

	int		i_debug_ladders;
	int		pl_debug_movement;
	ICVar*pl_debug_filter;

	int   v_profileMovement;  
	int   v_draw_suspension;
	int   v_draw_slip;
	int   v_pa_surface;    
	int   v_invertPitchControl;  
	float v_wind_minspeed; 
	float v_sprintSpeed;
	int   v_dumpFriction;
	int   v_rockBoats;
  int   v_debugSounds;
	float v_altitudeLimit;
	float v_altitudeLimitLowerOffset;
	float v_airControlSensivity;
	float v_stabilizeVTOL;
	int   v_help_tank_steering;
	
  float v_zeroGSpeedMultSpeed;
	float v_zeroGSpeedMultSpeedSprint;
	float v_zeroGSpeedMultNormal;
	float v_zeroGSpeedMultNormalSprint;
	float v_zeroGUpDown;
	float v_zeroGMaxSpeed;
	float v_zeroGSpeedMaxSpeed;
	float v_zeroGSpeedModeEnergyConsumption;
	int		v_zeroGSwitchableGyro;
	int		v_zeroGEnableGBoots;

	int		hud_mpNamesDuration;
	int		hud_mpNamesNearDistance;
	int		hud_mpNamesFarDistance;
	int		hud_onScreenNearDistance;
	int		hud_onScreenFarDistance;
	float	hud_onScreenNearSize;
	float	hud_onScreenFarSize;
	int		hud_colorLine;
	int		hud_colorOver;
	int		hud_colorText;
	int		hud_voicemode;
	int		hud_enableAlienInterference;
	float	hud_alienInterferenceStrength;
	int		hud_godFadeTime;
	int		hud_enablecrosshair;
	int		hud_crosshair;
	int		hud_chDamageIndicator;
	int		hud_panoramicHeight;
	int		hud_showAllObjectives;
	int		hud_subtitles;
	int   hud_subtitlesRenderMode;
	int   hud_subtitlesHeight;
	int   hud_subtitlesFontSize;
	int		hud_radarBackground;
	int		hud_radarAbsolute;
	int		hud_aspectCorrection;
	float hud_ctrl_Curve_X;
	float hud_ctrl_Curve_Z;
	float hud_ctrl_Coeff_X;
	float hud_ctrl_Coeff_Z;
	int		hud_ctrlZoomMode;
	int   hud_faderDebug;

	float aim_assistSearchBox;
	float aim_assistMaxDistance;
	float aim_assistSnapDistance;
	float aim_assistVerticalScale;
	float aim_assistSingleCoeff;
	float aim_assistAutoCoeff;
	float aim_assistRestrictionTimeout;

	int aim_assistAimEnabled;
	int aim_assistTriggerEnabled;
	int hit_assistSingleplayerEnabled;
	int hit_assistMultiplayerEnabled;
		
	float g_combatFadeTime;
	float g_combatFadeTimeDelay;
	float g_battleRange;

	ICVar*i_debuggun_1;
	ICVar*i_debuggun_2;

	float	tracer_min_distance;
	float	tracer_max_distance;
	float	tracer_min_length;
	float	tracer_max_length;
	float	tracer_min_scale;
	float	tracer_max_scale;
	float	tracer_speed_scale;
	int		tracer_max_count;
	float	tracer_player_radiusSqr;
	int		i_debug_projectiles;
	int		i_auto_turret_target;
	int		i_auto_turret_target_tacshells;
	int		i_debug_zoom_mods;
  int   i_debug_turrets;
  int   i_debug_sounds;
	int		i_debug_mp_flowgraph;
  
	float h_turnSpeed;
	int		h_useIK;
	int		h_drawSlippers;

  ICVar*  g_quickGame_map;
  ICVar*  g_quickGame_mode;
  int     g_quickGame_min_players;
  int     g_quickGame_prefer_lan;
  int     g_quickGame_prefer_favorites;
  int     g_quickGame_prefer_my_country;
  int     g_quickGame_ping1_level;
  int     g_quickGame_ping2_level;
  int     g_quickGame_debug;
	int			g_skip_tutorial;

  int     g_displayIgnoreList;
  int     g_buddyMessagesIngame;

	ICVar*  p_pp_UserDataFolder;
	ICVar*  p_pp_UserSaveGameFolder;
	ICVar*  p_pp_GUID;

	int			g_PSTutorial_Enabled;

	int			g_proneNotUsableWeapon_FixType;
	int			g_proneAimAngleRestrict_Enable;

	int			g_spectate_TeamOnly;

	SCVars()
	{
		memset(this,0,sizeof(SCVars));
	}

	~SCVars() { ReleaseCVars(); }

	void InitCVars(IConsole *pConsole);
	void ReleaseCVars();
};

#endif //__GAMECVARS_H__
