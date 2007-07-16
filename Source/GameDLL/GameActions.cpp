#include "StdAfx.h"
#include "GameActions.h"

#define DECL_ACTION(name) name = #name;
SGameActions::SGameActions()
{
#include "GameActions.actions"
}
#undef DECL_ACTION
