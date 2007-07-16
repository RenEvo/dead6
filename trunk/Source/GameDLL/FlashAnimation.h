/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Flash animation base class

-------------------------------------------------------------------------
History:
- 02:27:2007: Created by Marco Koegler

*************************************************************************/
#ifndef __FLASHANIMATION_H__
#define __FLASHANIMATION_H__

//-----------------------------------------------------------------------------------------------------

struct IFlashPlayer;
struct SFlashVarValue;

class CFlashAnimation
{
public:
	CFlashAnimation();
	virtual ~CFlashAnimation();

	IFlashPlayer*	GetFlashPlayer() const;

	bool	LoadAnimation(const char* name);
	virtual void	Unload();
	bool	IsLoaded() const;

	// these functions act on the flash player
	void SetVisible(bool visible);
	bool GetVisible() const;
	bool IsAvailable(const char* pPathToVar) const;
	bool SetVariable(const char* pPathToVar, const SFlashVarValue& value);
	bool CheckedSetVariable(const char* pPathToVar, const SFlashVarValue& value);
	bool Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult = 0);
	bool CheckedInvoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult = 0);
	// invoke helpers
	bool Invoke(const char* pMethodName, SFlashVarValue* pResult = 0)
	{
		return Invoke(pMethodName, 0, 0, pResult);
	}
	bool Invoke(const char* pMethodName, const SFlashVarValue& arg, SFlashVarValue* pResult = 0)
	{
		return Invoke(pMethodName, &arg, 1, pResult);
	}
	bool CheckedInvoke(const char* pMethodName, SFlashVarValue* pResult = 0)
	{
		return CheckedInvoke(pMethodName, 0, 0, pResult);
	}
	bool CheckedInvoke(const char* pMethodName, const SFlashVarValue& arg, SFlashVarValue* pResult = 0)
	{
		return CheckedInvoke(pMethodName, &arg, 1, pResult);
	}

private:
	IFlashPlayer*	m_pFlashPlayer;

	// shared null player
	static IFlashPlayer*	s_pFlashPlayerNull;
};

#endif //__FLASHANIMATION_H__

