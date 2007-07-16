/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 	
	Header for HUD object base class
	Shared by G02 and G04

-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre
- 22:02:2006: Refactored for G04 by Matthew Jack

*************************************************************************/
#ifndef __HUDOBJECT_H__
#define __HUDOBJECT_H__

//-----------------------------------------------------------------------------------------------------

// Forward declaration

struct IGameTokenSystem;

//-----------------------------------------------------------------------------------------------------

class CHUDObject
{
public:

						CHUDObject(bool bVisible=true);
	virtual ~	CHUDObject();

	void SetFadeValue(float fFadeValue);

	void Update(float fDeltaTime);

	virtual void PreUpdate() {};
	virtual void SetParent(void* parent);
	virtual void OnHUDToBeDestroyed() {};

	void GetHUDObjectMemoryStatistics(ICrySizer * s);

protected:

	virtual void OnUpdate(float fDeltaTime,float fFadeValue) = 0;

	// Convenience method for accessing the GameToken system.
	IGameTokenSystem *GetIGameTokenSystem() const;

	float m_fX;
	float m_fY;

	void		*m_parent;

private:

	float m_fFadeValue;
};

//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------