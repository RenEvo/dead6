#include "StdAfx.h"
#include "HUD.h"
#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "HUDObituary.h"
#include "GameRules.h"
#include "HUDTextChat.h"
#include "PlayerInput.h"
#include "StringUtils.h"
#include "CryPath.h"
#include "IUIDraw.h"
#include "GameCVars.h"

namespace NSKeyTranslation
{
	typedef CryFixedStringT<512> TFixedString;
	typedef CryFixedWStringT<512> TFixedWString;

	static const int MAX_KEYS = 3;
	SActionMapBindInfo gBindInfo = { 0, 0, 0 };

	// truncate wchar_t to char
	template<class T> void TruncateToChar(const wchar_t* wcharString, T& outString)
	{
		outString.resize(TFixedWString::_strlen(wcharString));
		char* dst = outString.begin();
		const wchar_t* src = wcharString;

		while (char c=(char)(*src++ & 0xFF))
		{
			*dst++ = c;
		}
	}

	// simple expand wchar_t to char
	template<class T> void ExpandToWChar(const char* charString, T& outString)
	{
		outString.resize(TFixedString::_strlen(charString));
		wchar_t* dst = outString.begin();
		const char* src = charString;
		while (const wchar_t c=(wchar_t)(*src++))
		{
			*dst++ = c;
		}
	}


	bool LookupBindInfo(const char* actionMap, const char* actionName, SActionMapBindInfo& bindInfo)
	{
		if (bindInfo.keys == 0)
		{
			bindInfo.keys = new const char*[MAX_KEYS];
			for (int i=0; i<MAX_KEYS; ++i)
				bindInfo.keys[i] = 0;
		}
	
		IActionMapManager* pAmMgr = g_pGame->GetIGameFramework()->GetIActionMapManager();
		if (pAmMgr == 0)
			return false;
		IActionMap* pAM = pAmMgr->GetActionMap(actionMap);
		if (pAM == 0)
			return false;
		return pAM->GetBindInfo(ActionId(actionName), bindInfo, MAX_KEYS);
	}	

	bool LookupBindInfo(const wchar_t* actionMap, const wchar_t* actionName, SActionMapBindInfo& bindInfo)
	{
		CryFixedStringT<64> actionMapString;
		TruncateToChar(actionMap, actionMapString);
		CryFixedStringT<64> actionNameString;
		TruncateToChar(actionName, actionNameString);
		return LookupBindInfo(actionMapString.c_str(), actionNameString.c_str(), bindInfo);
	}

	template<class T> 
	void InsertString(T& inString, size_t pos, const wchar_t* s)
	{
		inString.insert(pos, s);
	}

	template<size_t S> 
	void InsertString(CryFixedWStringT<S>& inString, size_t pos, const char* s)
	{
		CryFixedWStringT<64> wcharString;
		ExpandToWChar(s, wcharString);
		inString.insert(pos, wcharString.c_str());
	}

	/*
	template<size_t S>
	void InsertString(CryFixedStringT<S>& inString, size_t pos, const wchar_t* s)
	{
		CryFixedStringT<64> charString;
		TruncateToChar(s, charString);
		inString.insert(pos, charString.c_str());
	}

	template<size_t S>
	void InsertString(CryFixedStringT<S>& inString, size_t pos, const char* s)
	{
		inString.insert(pos, s);
	}
	*/

	// Replaces all occurrences of %[actionmap:actionid] with the current key binding
	template<size_t S>
	void ReplaceActions(ILocalizationManager* pLocMgr, CryFixedWStringT<S>& inString)
	{
		typedef CryFixedWStringT<S> T;
		static const typename T::value_type* actionPrefix = L"%[";
		static const size_t actionPrefixLen = T::_strlen(actionPrefix);
		static const typename T::value_type actionDelim = L':';
		static const typename T::value_type actionDelimEnd = L']';
		static const char* keyPrefix = "@cc_"; // how keys appear in the localization sheet
		static const size_t keyPrefixLen = strlen(keyPrefix);
		CryFixedStringT<32> fullKeyName;
		static wstring realKeyName;

		size_t pos = inString.find(actionPrefix, 0);

		while (pos != T::npos)
		{
			size_t pos1 = inString.find(actionDelim, pos+actionPrefixLen);
			if (pos1 != T::npos)
			{
				size_t pos2 = inString.find(actionDelimEnd, pos1+1);
				if (pos2 != T::npos)
				{
					// found end of action descriptor
					typename T::value_type* t1 = inString.begin()+pos1;
					typename T::value_type* t2 = inString.begin()+pos2;
					*t1 = 0;
					*t2 = 0;
					const typename T::value_type* actionMapName = inString.begin()+pos+actionPrefixLen;
					const typename T::value_type* actionName = inString.begin()+pos1+1;
					// CryLogAlways("Found: '%S' '%S'", actionMapName, actionName);
					bool bFound = LookupBindInfo(actionMapName, actionName, gBindInfo);
					*t1 = actionDelim; // re-replace ':'
					*t2 = actionDelimEnd; // re-replace ']'
					if (bFound && gBindInfo.nKeys >= 1)
					{
						inString.erase(pos, pos2-pos+1);
						const char* keyName = gBindInfo.keys[0]; // first key
						fullKeyName.assign(keyPrefix, keyPrefixLen);
						fullKeyName.append(keyName);
						bFound = pLocMgr->LocalizeLabel(fullKeyName.c_str(), realKeyName);
						if (bFound)
						{
							InsertString(inString, pos, realKeyName);
						}
						else
						{
							// not found, insert original (untranslated name from actionmap)
							InsertString(inString, pos, keyName);
						}
					}
				}
			}
			pos = inString.find(actionPrefix, pos+1);
		}
	}
}; // namespace NSKeyTranslation

const wchar_t*
CHUD::LocalizeWithParams(const char* label, bool bAdjustActions, const char* param1, const char* param2, const char* param3, const char* param4)
{
	static NSKeyTranslation::TFixedWString finalLocalizedString;

	if (label[0] == '@')
	{
		ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();
		wstring localizedString, finalString;
		pLocMgr->LocalizeLabel(label, localizedString);
		const bool bFormat = param1 || param2 || param3 || param4;
		if(bFormat)
		{
			wstring p1, p2, p3, p4;
			if(param1)
				pLocMgr->LocalizeLabel(param1, p1);
			if(param2)
				pLocMgr->LocalizeLabel(param2, p2);
			if(param3)
				pLocMgr->LocalizeLabel(param3, p3);
			if(param4)
				pLocMgr->LocalizeLabel(param4, p4);
			pLocMgr->FormatStringMessage(finalString, localizedString, 
				p1.empty()?0:p1.c_str(),
				p2.empty()?0:p2.c_str(),
				p3.empty()?0:p3.c_str(),
				p4.empty()?0:p4.c_str());
		}
		else
			finalString = localizedString;

		finalLocalizedString.assign(finalString.c_str(), finalString.length());
		if (bAdjustActions)
		{
			NSKeyTranslation::ReplaceActions(pLocMgr, finalLocalizedString);
		}
	}
	else
	{
		// we expand always to wchar_t, as Flash will translate into wchar anyway
		NSKeyTranslation::ExpandToWChar(label, finalLocalizedString);
		// in non-localized case replace potential line-breaks
		finalLocalizedString.replace(L"\\n",L"\n");
		if (bAdjustActions)
		{
			ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();
			NSKeyTranslation::ReplaceActions(pLocMgr, finalLocalizedString);
		}
	}
	return finalLocalizedString.c_str();
}

void CHUD::DisplayFlashMessage(const char* label, int pos /* = 1 */, const ColorF &col /* = Col_White */, bool formatWStringWithParams /* = false */, const char* paramLabel1 /* = 0 */, const char* paramLabel2 /* = 0 */, const char* paramLabel3 /* = 0 */, const char* paramLabel4 /* = 0 */)
{
	if(!label)
		return;

	unsigned int packedColor = col.pack_rgb888();

	if (pos < 1 || pos > 4)
		pos = 1;

	if(pos == 2 && m_fMiddleTextLineTimeout <= 0.0f)
		m_fMiddleTextLineTimeout = gEnv->pTimer->GetFrameStartTime().GetSeconds() + 3.0f;

	const wchar_t* localizedText = L"";
	if(formatWStringWithParams)
		localizedText = LocalizeWithParams(label, true, paramLabel1, paramLabel2, paramLabel3, paramLabel4);
	else
		localizedText = LocalizeWithParams(label, true);

	SFlashVarValue args[3] = {localizedText, pos, packedColor};
	m_animMessages.Invoke("setMessageText", args, 3);
}


void CHUD::DisplayTempFlashText(const char* label, float seconds, const ColorF &col)
{
	if(seconds > 60.0f)
		seconds = 60.0f;
	m_fMiddleTextLineTimeout = gEnv->pTimer->GetFrameStartTime().GetSeconds() + seconds;
	DisplayFlashMessage(label, 2, col);
}

void CHUD::BattleLogEvent(int type, const char *msg, const char *p0, const char *p1, const char *p2, const char *p3)
{
	wstring localizedString, finalString;
	ILocalizationManager *pLocalizationMan = gEnv->pSystem->GetLocalizationManager();
	pLocalizationMan->LocalizeString(msg, localizedString);

	if (p0)
	{
		wstring p0localized;
		pLocalizationMan->LocalizeString(p0, p0localized);

		wstring p1localized;
		if (p1)
			pLocalizationMan->LocalizeString(p1, p1localized);

		wstring p2localized;
		if (p2)
			pLocalizationMan->LocalizeString(p2, p2localized);

		wstring p3localized;
		if (p3)
			pLocalizationMan->LocalizeString(p3, p3localized);

		pLocalizationMan->FormatStringMessage(finalString, localizedString,
			p0localized.empty()?0:p0localized.c_str(),
			p1localized.empty()?0:p1localized.c_str(),
			p2localized.empty()?0:p2localized.c_str(),
			p3localized.empty()?0:p3localized.c_str());
	}
	else
		finalString=localizedString;

	SFlashVarValue args[2] = {finalString.c_str(), type};
	m_animBattleLog.Invoke("setMPLogText", args, 2);
}

// returns ptr to static string buffer.
const char* GetSoundKey(const char* soundName)
{
	static string buf;
	static const char* prefix = "Languages/dialog/";
	static const int prefixLen = strlen(prefix);

	buf.assign("@");
	// check if it already starts Languages/dialog. then replace it
	if (CryStringUtils::stristr(soundName, prefix) == soundName)
	{
		buf.append (soundName+prefixLen);
	}
	else
	{
		buf.append (soundName);
	}
	PathUtil::RemoveExtension(buf);
	return buf.c_str();
}


void CHUD::ShowSubtitle(ISound *pSound, bool bShow)
{
	assert (pSound != 0);
	if (pSound == 0)
		return;

	const char* soundKey = GetSoundKey(pSound->GetName());
	InternalShowSubtitle(soundKey, pSound, bShow);
}

void CHUD::ShowSubtitle(const char* subtitleLabel, bool bShow)
{
	InternalShowSubtitle(subtitleLabel, 0, bShow);
}

void CHUD::InternalShowSubtitle(const char* subtitleLabel, ISound* pSound, bool bShow)
{
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();
	if (bShow)
	{
		TSubtitleEntries::iterator iter = std::find(m_subtitleEntries.begin(), m_subtitleEntries.end(), subtitleLabel);
		if (iter == m_subtitleEntries.end())
		{
			wstring localizedString;
			const bool bFound = pLocMgr->GetSubtitle(subtitleLabel, localizedString);
			if (bFound)
			{
				SSubtitleEntry entry;
				entry.key = subtitleLabel;
				if (pSound)
				{
					entry.soundId = pSound->GetId();
					float timeToShow = pSound->GetLengthMs() * 0.001f; // msec to sec
#if 0 // make the text stay longer than the sound
					timeToShow = std::min(timeToShow*1.1f, timeToShow+2.0f); // 10 percent longer, but max 2 seconds
#endif
					entry.timeRemaining = timeToShow;
					entry.bPersistant = false;
				}
				else
				{
					entry.soundId = INVALID_SOUNDID;
					entry.timeRemaining = 0.0f;
					entry.bPersistant = true;
				}

				// replace actions
				NSKeyTranslation::TFixedWString finalDisplayString;
				finalDisplayString.assign(localizedString, localizedString.length());
				NSKeyTranslation::ReplaceActions(pLocMgr, finalDisplayString);
				entry.localized.assign(localizedString.c_str(), localizedString.length());

				m_subtitleEntries.push_front(entry);
				if (m_subtitleEntries.size() > 5)
					m_subtitleEntries.resize(5);

				m_bSubtitlesNeedUpdate = true;
			}
		}
	}
	else // if (bShow)
	{
		tSoundID soundId = pSound ? pSound->GetId() : INVALID_SOUNDID;
		TSubtitleEntries::iterator iterEnd = m_subtitleEntries.end();
		for (TSubtitleEntries::iterator iter = m_subtitleEntries.begin(); iter != iterEnd; )
		{
			const SSubtitleEntry& entry = *iter;
			// sounds timeout by themselves!
			if (iter->key == subtitleLabel && soundId == INVALID_SOUNDID /* iter->soundId == soundId*/)
			{
				iter = m_subtitleEntries.erase(iter);
				m_bSubtitlesNeedUpdate = true;
			}
			else
			{
				++iter;
			}
		}
	}
}

void CHUD::SetRadioButtons(bool active, int buttonNo /* = 0 */)
{
	if(active)
	{
		m_animRadioButtons.Invoke("showRadioButtons", buttonNo);
		m_animRadioButtons.SetVisible(true);
	}
	else
	{
		m_animRadioButtons.Invoke("showRadioButtons", 0);
		m_animRadioButtons.SetVisible(false);
	}
}

void CHUD::ShowVirtualKeyboard(bool active)
{
	if(m_pHUDTextChat)
		m_pHUDTextChat->ShowVirtualKeyboard(active);

	m_animGamepadConnected.Reload();
	m_animGamepadConnected.Invoke("GamepadAvailable", active);
}

void CHUD::ObituaryMessage(EntityId targetId, EntityId shooterId, EntityId weaponId, bool headshot)
{
	if (!g_pGame->GetGameRules())
		return;

	ILocalizationManager *pLM=gEnv->pSystem->GetLocalizationManager();
	CItem *pItem=static_cast<CItem *>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId));
	IEntity *pWeapon=gEnv->pEntitySystem->GetEntity(weaponId);

	const char *targetName=g_pGame->GetGameRules()->GetActorNameByEntityId(targetId);
	const char *shooterName=g_pGame->GetGameRules()->GetActorNameByEntityId(shooterId);
	IEntityClass* pWeaponClass=pWeapon?pWeapon->GetClass():0;

	wstring entity[3];
	if (targetName && targetName[0])
		pLM->LocalizeString(targetName, entity[0], true);
	if (shooterName && shooterName[0])
		pLM->LocalizeString(shooterName, entity[1], true);
	if (pWeaponClass)
		pLM->LocalizeString(pWeaponClass->GetName(), entity[2], true);

	wstring localized;
	wstring final;

	if (targetId==shooterId || !shooterId || targetId==weaponId)
	{
		if (pItem && targetId!=weaponId && pWeaponClass)
		{
			pLM->LocalizeString("@mp_ObituaryKilledBy", localized);
			pLM->FormatStringMessage(final, localized, entity[0].c_str(), entity[2].c_str());
		}
		else if (pWeaponClass && weaponId!=shooterId)
		{
			pLM->LocalizeString("@mp_ObituarySuicidedWith", localized);
			pLM->FormatStringMessage(final, localized, entity[0].c_str(), entity[2].c_str());
		}
		else
		{
			pLM->LocalizeString("@mp_ObituarySuicided", localized);
			pLM->FormatStringMessage(final, localized, entity[0].c_str());
		}
	}
	else if (pWeaponClass && pWeaponClass == CItem::sFistsClass)
	{
		if (headshot)
		{
			pLM->LocalizeString("@mp_ObituaryKilledWithHeadPunch", localized);
			pLM->FormatStringMessage(final, localized, entity[1].c_str(), entity[0].c_str());
		}
		else
		{
			pLM->LocalizeString("@mp_ObituaryKilledWithPunch", localized);
			pLM->FormatStringMessage(final, localized, entity[1].c_str(), entity[0].c_str());
		}
	}
	else
	{		
		if (headshot)
		{
			pLM->LocalizeString("@mp_ObituaryKilledWithHeadshot", localized);
		}
		else
		{
			pLM->LocalizeString("@mp_ObituaryKilledWith", localized);
		}
		pLM->FormatStringMessage(final, localized, entity[1].c_str(), entity[0].c_str(), entity[2].c_str());
	}

	m_pHUDObituary->AddMessage(final.c_str());
}

void CHUD::ShowWarningMessage(EWarningMessages message, const char* optionalText)
{
	switch(message)
	{
	case EHUD_SPECTATOR:
		m_animWarningMessages.Invoke("showErrorMessage", "spectator");
		break;
	case EHUD_SWITCHTOTAN:
		m_animWarningMessages.Invoke("showErrorMessage", "switchtotan");
		break;
	case EHUD_SWITCHTOBLACK:
		m_animWarningMessages.Invoke("showErrorMessage", "switchtous");
		break;
	case EHUD_SUICIDE:
		m_animWarningMessages.Invoke("showErrorMessage", "suicide");
		break;
	case EHUD_CONNECTION_LOST:
		m_animWarningMessages.Invoke("showErrorMessage", "connectionlost");
		break;
	case EHUD_OK:
		m_animWarningMessages.Invoke("showErrorMessage", "Box1");
		if(optionalText)
			m_animWarningMessages.Invoke("setErrorText", optionalText);
		break;
	case EHUD_YESNO:
		m_animWarningMessages.Invoke("showErrorMessage", "Box2");
		if(optionalText)
			m_animWarningMessages.Invoke("setErrorText", optionalText);
		break;
	case EHUD_CANCEL:
		m_animWarningMessages.Invoke("showErrorMessage", "Box3");
		if(optionalText)
			m_animWarningMessages.Invoke("setErrorText", optionalText);
		break;
	default:
		return;
		break;
	}

	m_animWarningMessages.SetVisible(true);

	if(m_pModalHUD == &m_animPDA)
		ShowPDA(false);
	SwitchToModalHUD(&m_animWarningMessages,true);
	CPlayer *pPlayer = static_cast<CPlayer *>(g_pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer && pPlayer->GetPlayerInput())
		pPlayer->GetPlayerInput()->DisableXI(true);
}

void CHUD::HandleWarningAnswer(const char* warning /* = NULL */)
{
	SwitchToModalHUD(NULL,false);
	CPlayer *pPlayer = static_cast<CPlayer *>(g_pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer && pPlayer->GetPlayerInput())
		pPlayer->GetPlayerInput()->DisableXI(false);

	m_animWarningMessages.SetVisible(false);

	if(warning)
	{
		if(!strcmp(warning, "suicide"))
		{
			SwitchToModalHUD(NULL,false);
			gEnv->pConsole->ExecuteString("kill me");
		}
		else if(!strcmp(warning, "spectate"))
		{
			gEnv->pConsole->ExecuteString("spectator");
		}
		else if(!strcmp(warning, "switchTeam"))
		{
			CGameRules* pRules = g_pGame->GetGameRules();
			if(pRules->GetTeamCount() > 1)
			{
				const char* command = "team black";
				if(pRules->GetTeamId("black") == pRules->GetTeam(pPlayer->GetEntityId()))
					command = "team tan";
				gEnv->pConsole->ExecuteString(command);
			}
		}
	}
}


void CHUD::UpdateSubtitlesManualRender(float frameTime)
{
	if (m_subtitleEntries.empty() == false)
	{
		for (TSubtitleEntries::iterator iter = m_subtitleEntries.begin(); iter != m_subtitleEntries.end();)
		{
			SSubtitleEntry& entry = *iter;
			if (entry.bPersistant == false)
			{
				entry.timeRemaining -= frameTime;
				const bool bDelete = entry.timeRemaining <= 0.0f;
				if (bDelete)
				{
					TSubtitleEntries::iterator toDelete = iter;
					++iter;
					m_subtitleEntries.erase(toDelete);
					m_bSubtitlesNeedUpdate = true;
					continue;
				}
			}
			++iter;
		}
	}

	if (g_pGameCVars->hud_subtitlesRenderMode == 0)
	{
		if(g_pGameCVars->hud_subtitles) //should be a switch
		{
			m_animSubtitles.Reload();
			if (m_bSubtitlesNeedUpdate)
			{
				// re-set text
				CryFixedWStringT<1024> subtitleString;
				bool bFirst = true;
				TSubtitleEntries::const_iterator iterEnd = m_subtitleEntries.end();
				for (TSubtitleEntries::const_iterator iter = m_subtitleEntries.begin(); iter != iterEnd; ++iter)
				{
					if (!bFirst)
						subtitleString+=L"\n";
					else
						bFirst = false;
					const SSubtitleEntry& entry = *iter;
					subtitleString+=entry.localized;
				}
				m_animSubtitles.Invoke("setText", subtitleString.c_str());
				m_bSubtitlesNeedUpdate = false;
			}
			m_animSubtitles.GetFlashPlayer()->Advance(frameTime);
			m_animSubtitles.GetFlashPlayer()->Render();
		}
		else if(m_animSubtitles.IsLoaded())
		{
			m_animSubtitles.Unload();
		}
	}
	else // manual render
	{
		if (m_subtitleEntries.empty())
			return;

		IUIDraw* pUIDraw = g_pGame->GetIGameFramework()->GetIUIDraw();
		if (pUIDraw==0) // should never happen!
		{
			m_subtitleEntries.clear();
			m_bSubtitlesNeedUpdate = true;
			return;
		}

		IRenderer* pRenderer = gEnv->pRenderer;
		const float x = 0.0f;
		const float maxWidth = 700.0f;
		// float y = 600.0f - (g_pGameCVars->hud_panoramicHeight * 600.0f / 768.0f) + 2.0f;
		float y = 600.0f - (g_pGameCVars->hud_subtitlesHeight * 6.0f) + 1.0f; // subtitles height is in percent of screen height (600.0)

		pUIDraw->PreRender();

		// now draw 2D texts overlay
		for (TSubtitleEntries::iterator iter = m_subtitleEntries.begin(); iter != m_subtitleEntries.end();)
		{
			SSubtitleEntry& entry = *iter;
			ColorF clr = Col_White;
			static const float TEXT_SPACING = 2.0f;
			const float textSize = g_pGameCVars->hud_subtitlesFontSize;
			float sizeX,sizeY;
			const string& textLabel = entry.key;
			if (!textLabel.empty() && textLabel[0] == '@' && entry.localized.empty() == false)
			{
				pUIDraw->GetWrappedTextDimW(m_pDefaultFont,&sizeX, &sizeY, maxWidth, textSize, textSize, entry.localized.c_str());
				pUIDraw->DrawWrappedTextW(m_pDefaultFont,x, y, maxWidth, textSize, textSize, entry.localized.c_str(), clr.a, clr.r, clr.g, clr.b, 
					// UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_LEFT,UIDRAWVERTICAL_TOP);
					UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
			}
			else
			{
				pUIDraw->GetTextDim(m_pDefaultFont,&sizeX, &sizeY, textSize, textSize, textLabel.c_str());
				pUIDraw->DrawText(m_pDefaultFont,x, y, textSize, textSize, textLabel.c_str(), clr.a, clr.r, clr.g, clr.b, 
					UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP,UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_TOP);
			}
			y+=sizeY+TEXT_SPACING;
			++iter;
		}

		pUIDraw->PostRender();
	}

}

//-----------------------------------------------------------------------------------------------------

void CHUD::ShowTutorialText(const wchar_t* text, int pos)
{
	// NB: text is displayed as passed - fetch the localised string before calling this.

	if(text != NULL)
	{
		m_animTutorial.Invoke("setFixPosition", pos);

		m_animTutorial.Invoke("showTutorial", true);
		m_animTutorial.Invoke("setTutorialTextNL",text);
	}
	else
	{
		m_animTutorial.Invoke("showTutorial", false);
	}
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetTutorialTextPosition(int pos)
{
	m_animTutorial.Invoke("setFixPosition", pos);
}

//-----------------------------------------------------------------------------------------------------

void CHUD::SetTutorialTextPosition(float posX, float posY)
{
 SFlashVarValue args[2] = {posX*1024.0f, posY * 768.0f};
	m_animTutorial.Invoke("setPosition", args, 2);
}