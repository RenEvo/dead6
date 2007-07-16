#ifndef __GAMEACTIONS_H__
#define __GAMEACTIONS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IActionMapManager.h>

#define DECL_ACTION(name) ActionId name;
struct SGameActions
{
	SGameActions();
#include "GameActions.actions"
};
#undef DECL_ACTION

#endif //__GAMEACTIONS_H__
