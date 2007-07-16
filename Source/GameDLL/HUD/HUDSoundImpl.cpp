#include "StdAfx.h"
#include <ISound.h>
#include "HUD.h"
#include "../Actor.h"
#include "../Game.h"

void CHUD::PlaySound(ESound eSound, bool play)
{
	if(!gEnv->pSoundSystem)
		return;

	const char* strSound;

	switch(eSound)
	{
	case ESound_GenericBeep:
		strSound = "Sounds/interface:suit:generic_beep";
		break;
	case ESound_PresetNavigationBeep:
		strSound = "Sounds/interface:suit:preset_navigation_beep";
		break;
	case ESound_TemperatureBeep:
		strSound = "Sounds/interface:suit:temperature_beep";
		break;
	case ESound_SuitMenuAppear:
		strSound = "sounds/interface:suit:modification_menu_appear";
		break;
	case ESound_SuitMenuDisappear:
		strSound = "Sounds/interface:suit:modification_menu_disappear";
		break;
	case ESound_WeaponModification:
		strSound = "Sounds/interface:suit:weapon_modification_beep";
		break;
	case ESound_BinocularsZoomIn:
		strSound = "Sounds/interface:suit:binocular_zoom_in";
		break;
	case ESound_BinocularsZoomOut:
		strSound = "Sounds/interface:suit:binocular_zoom_out";
		break;
	case ESound_BinocularsSelect:
		strSound = "Sounds/interface:suit:binocular_select";
		break;
	case ESound_BinocularsDeselect:
		strSound = "Sounds/interface:suit:binocular_deselect";
		break;
	case ESound_BinocularsAmbience:
		strSound = "Sounds/interface:suit:binocular_ambience";
		break;
	case ESound_NightVisionSelect:
		strSound = "Sounds/interface:suit:night_vision_select";
		break;
	case ESound_NightVisionDeselect:
		strSound = "Sounds/interface:suit:night_vision_deselect";
		break;
	case ESound_NightVisionAmbience:
		strSound = "Sounds/interface:suit:night_vision_ambience";
		break;
	case ESound_BinocularsLock:
		strSound = "Sounds/interface:suit:binocular_target_locked";
		break;
	case ESound_OpenPopup:
		strSound = "Sounds/interface:menu:pop_up";
		break;
	case ESound_ClosePopup:
		strSound = "Sounds/interface:menu:close";
		break;
	case ESound_TabChanged:
		strSound = "Sounds/interface:menu:screen_change";
		break;
	case ESound_WaterDroplets:
		strSound = "Sounds/interface:hud:hud_water";
		break;
	case ESound_BuyBeep:
		strSound = "Sounds/interface:menu:buy_beep";
		break;
	case ESound_BuyError:
		strSound = "Sounds/interface:menu:buy_error";
		break;
	case ESound_SniperZoomIn:
		strSound = "Sounds/interface:hud:sniper_scope_zoom_in";
		break;
	case ESound_SniperZoomOut:
		strSound = "Sounds/interface:hud:sniper_scope_zoom_out";
		break;
	case ESound_Highlight:
		strSound = "sounds/interface:menu:rollover";
		break;
	case ESound_Select:
		strSound = "sounds/interface:menu:click1";
		break;
	default:
		assert(0);
		return;
	}

	if(play)
	{
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(strSound,0);
		if ( pSound )
		{
			pSound->Play();
			m_soundIDs[eSound] = pSound->GetId();
		}
	}
	else if(m_soundIDs[eSound])
	{
		ISound *pSound = gEnv->pSoundSystem->GetSound(m_soundIDs[eSound]);
		if(pSound)
			pSound->Stop();
	}

}

//-----------------------------------------------------------------------------------------------------

void CHUD::PlayStatusSound(const char* name, bool forceSuitSound)
{
	if(!gEnv->pSoundSystem)
		return;

	//string strSound("languages/dialog/suit/");
	string strSound;

	//we don't want to play the suit sounds at once (because of quick mode change key)
	if(!forceSuitSound &&
			(!stricmp(name, "normal_cloak_on") ||
			!stricmp(name, "maximum_speed") ||
			!stricmp(name, "maximum_strength") ||
			!stricmp(name, "maximum_armor")))
	{
		// look for "m_fSuitChangeSoundTimer"
		return;
	}

	switch(m_iVoiceMode)
	{
	case 0: //male
		strSound.append("suit/male/suit_");
		break;
	case 1: //female
		strSound.append("suit/female/suit_");
		break;
	case 2: //michaelR
		strSound.append("suit/mr/suit_");
		break;
	default:
		return;
		break;
	}

	//VS2 hack ...
	if(!strcmp(name, "normal_cloak_on"))
		strSound.append("modification_engaged");
	else if(!strcmp(name, "normal_cloak_off"))
		return;
	else
		strSound.append(name);

	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(strSound,FLAG_SOUND_3D|FLAG_SOUND_VOICE);
	if ( pSound )
	{
		pSound->SetPosition(g_pGame->GetIGameFramework()->GetClientActor()->GetEntity()->GetWorldPos());
		pSound->Play();
	}
}
