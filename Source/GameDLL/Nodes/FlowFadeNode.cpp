#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "Item.h"
#include "GameRules.h"
#include "Nodes/G2FlowBaseNode.h"
#include "HUD/HUD.h"
#include "HUD/HUDObject.h"

class CHUDFader
{
public:
	CHUDFader()
	{
		m_pRenderer = gEnv->pRenderer;
		assert (m_pRenderer != 0);

		m_color = Col_Black;
		m_pTexture = 0;
		m_bActive = false;
	}

	~CHUDFader()
	{
		SAFE_RELEASE(m_pTexture);
	}

	void SetTexture(const string& texName)
	{
		SAFE_RELEASE(m_pTexture);
		if (texName.empty())
			return;

		m_pTexture = m_pRenderer->EF_LoadTexture(texName.c_str(),FT_DONT_RELEASE,eTT_2D);
		if (m_pTexture != 0)
			m_pTexture->SetClamp(true);
	}

	const char* GetDebugName()
	{
		if (m_pTexture)
			return m_pTexture->GetName();
		return "<no texture>";
	}

	void SetColor(const ColorF& color)
	{
		m_color = color;
	}

	void SetActive(bool bActive)
	{
		m_bActive = bActive;
	}

	bool IsActive() const { return m_bActive; }

	virtual void Update(float fDeltaTime)
	{
		if (m_bActive == false)
			return;

		m_pRenderer->Draw2dImage(0.0f,0.0f,800.0f,600.0f,
			m_pTexture ? m_pTexture->GetTextureID() : 0,
			0.0f,1.0f,1.0f,0.0f, // tex coords
			0.0f, // angle
			m_color.r, m_color.g, m_color.b, m_color.a
			);
	}


protected:
	IRenderer			*m_pRenderer;
	ColorF         m_color;
	ITexture*      m_pTexture;
	bool           m_bActive;
};

static const int NUM_FADERS = 4;

class CMasterFader : public CHUDObject
{
public:
	CMasterFader() : CHUDObject(true)
	{
		m_bRegistered = false;
		memset(m_pHUDFader, 0, sizeof(m_pHUDFader));
	}

	~CMasterFader()
	{
		UnRegister();
		for (int i=0; i<NUM_FADERS; ++i)
			SAFE_DELETE(m_pHUDFader[i]);
	}

	CHUDFader* GetHUDFader(int group)
	{
		if (m_bRegistered == false)
			Register();
		if (group < 0 || group >= NUM_FADERS)
			return 0;
		if (m_pHUDFader[group] == 0)
			m_pHUDFader[group] = new CHUDFader;
		return m_pHUDFader[group];
	}

	// CHUDObject
	virtual void OnUpdate(float fDeltaTime,float fFadeValue)
	{
		int nActive = 0;
		for (int i=0; i<NUM_FADERS; ++i)
		{
			if (m_pHUDFader[i])
			{
				m_pHUDFader[i]->Update(fDeltaTime);
				if (m_pHUDFader[i]->IsActive())
					++nActive;
			}
		}
		if (g_pGame->GetCVars()->hud_faderDebug != 0)
		{
			const float x = 5.0f;
			float y = 100.0f;
			float white[] = {1,1,1,1};
			gEnv->pRenderer->Draw2dLabel( x, y, 1.2f, white, false, "HUDFader Active: %d", nActive );
			y+=12.0f;
			if (nActive > 0)
			{
				for (int i=0; i<NUM_FADERS; ++i)
				{
					if (m_pHUDFader[i] && m_pHUDFader[i]->IsActive())
					{
						gEnv->pRenderer->Draw2dLabel( x, y, 1.2f, white, false, "  Fader %d : Texture='%s'", i, m_pHUDFader[i]->GetDebugName());
						y+=10.0f;
					}
				}
			}
		}
	}

	virtual void OnHUDToBeDestroyed()
	{
		m_bRegistered = false;
	}
	// ~CHUDObject

	void Register()
	{
		if (m_bRegistered == false)
		{
			CHUD* pHUD = g_pGame->GetHUD();
			if (pHUD)
			{
				pHUD->RegisterHUDObject(this);
				m_bRegistered = true;
			}
		}
	}

	void UnRegister()
	{
		if (m_bRegistered == true)
		{
			CHUD* pHUD = g_pGame->GetHUD();
			if (pHUD)
			{
				pHUD->DeregisterHUDObject(this);
				m_bRegistered = false;
			}
		}
	}


	bool m_bRegistered;
	CHUDFader* m_pHUDFader[NUM_FADERS];
};

CMasterFader* g_pMasterFader = 0;

CHUDFader* GetHUDFader(int group)
{
	if (g_pMasterFader == 0)
		g_pMasterFader = new CMasterFader();
	if (g_pMasterFader)
		return g_pMasterFader->GetHUDFader(group);
	else 
		return 0;
}


class CFlowFadeNode : public CFlowBaseNode
{
	CHUDFader* GetFader(SActivationInfo * pActInfo)
	{
		const int& group = GetPortInt(pActInfo, EIP_FadeGroup);
		return GetHUDFader(group);
	}

public:
	CFlowFadeNode( SActivationInfo * pActInfo )
	{
	}

	~CFlowFadeNode()
	{
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowFadeNode(pActInfo);
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		// not really QS/QL prepared at the moment
		// so, we stop any fading on load
		if (ser.IsReading())
		{
			StopFader(pActInfo);
		}
	}

	void StopFader(SActivationInfo * pActInfo)
	{
		CHUDFader* pFader = GetFader(pActInfo);
		if (pFader)
			pFader->SetActive(false);
	}

	void StartFader(SActivationInfo * pActInfo)
	{
		CHUDFader* pFader = GetFader(pActInfo);
		if (pFader != 0)
		{
			pFader->SetTexture(GetPortString(pActInfo, EIP_TextureName));
			pFader->SetActive(true);
		}
	}

	void Interpol(SActivationInfo * pActInfo, const float fPosition)
	{
		CHUDFader* pFader = GetFader(pActInfo);
		if (pFader != 0)
		{
			ColorF col;
			col.a = m_direction > 0.0f ? fPosition : 1.0 - fPosition;
			const Vec3 fadeColor = GetPortVec3(pActInfo, EIP_Color);
			col.r = fadeColor[0];
			col.g = fadeColor[1];
			col.b = fadeColor[2];
			pFader->SetColor(col);
			ActivateOutput(pActInfo, EOP_FadeColor, fadeColor);
		}
	}

	enum EInputPorts
	{
		EIP_FadeGroup = 0,
		EIP_FadeIn,
		EIP_FadeOut,
		EIP_InTime,
		EIP_OutTime,
		EIP_Color,
		EIP_TextureName
	};

	enum EOutputPorts
	{
		EOP_FadedIn = 0,
		EOP_FadedOut,
		EOP_FadeColor
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig<int> ("FadeGroup", 0, _HELP("Fade Group [0-3]"), 0, _UICONFIG("enum_int:0=0,1=1,2=2,3=3")),
			InputPortConfig_Void("FadeIn", _HELP("Fade back from the specified color back to normal screen")),
			InputPortConfig_Void("FadeOut", _HELP("Fade the screen to the specified color")),
			InputPortConfig<float>("FadeInTime", 2.0f, _HELP("Duration of fade in")),
			InputPortConfig<float>("FadeOutTime", 2.0f, _HELP("Duration of fade out")),
			InputPortConfig<Vec3> ("color_FadeColor"),
			InputPortConfig<string> ("tex_TextureName", _HELP("Texture Name")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("FadedIn", _HELP("FadedIn")),
			OutputPortConfig_Void("FadedOut", _HELP("FadedOut")),
			OutputPortConfig<Vec3> ("CurColor", _HELP("Current Faded Color")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Controls Screen Fading.");
		config.SetCategory(EFLN_WIP);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			StopFader(pActInfo);
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_FadeIn))
				{
					StopFader(pActInfo);
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
					m_startTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
					m_endTime = m_startTime + GetPortFloat(pActInfo, EIP_InTime) * 1000.0f;
					StartFader(pActInfo);	
					m_direction = -1.0f;
					Interpol(pActInfo, 0.0f);
				}
				if (IsPortActive(pActInfo, EIP_FadeOut))
				{
					StopFader(pActInfo);
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
					m_startTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
					m_endTime = m_startTime + GetPortFloat(pActInfo, EIP_OutTime) * 1000.0f;
					StartFader(pActInfo);
					m_direction = 1.0f;
					Interpol(pActInfo, 0.0f);
				}
			}
			break;
		case eFE_Update:
			{
				const float fTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
				const float fDuration = m_endTime - m_startTime;
				float fPosition;
				if (fDuration <= 0.0)
					fPosition = 1.0;
				else
				{
					fPosition = (fTime - m_startTime) / fDuration;
					fPosition = CLAMP(fPosition, 0.0f, 1.0f);
				}
				if (fTime >= m_endTime)
				{
					if (m_direction < 0.0f)
					{
						StopFader(pActInfo);
						ActivateOutput(pActInfo, EOP_FadedIn, true);
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);					
					}
					else
					{
						ActivateOutput(pActInfo, EOP_FadedOut, true);
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);					
					}
				}

				Interpol(pActInfo, fPosition);
				// caused by stop or by serialize call maybe
			}
			break;
		}
	}
	protected:
	float m_startTime;
	float m_endTime;
	float m_direction;
};

REGISTER_FLOW_NODE("CrysisFX:ScreenFader", CFlowFadeNode);

