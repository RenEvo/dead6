////////////////////////////////////////////////////
// C&C: The Dead 6 - Core File
// Copyright (C), RenEvo Studios, 2007
//
// IPurchaseSystem.h
//
// Purpose: Interface object
//	Describes a purchase system for controlling team
//	purchases
//
// File History:
//	- 8/12/07 : File created - Dan
////////////////////////////////////////////////////
#ifndef _IPURCHASESYSTEM_H_
#define _IPURCHASESYSTEM_H_
#include <map>

class ITeamManager;
class CD6Player;
struct SBuildingClassDef;

////////////////////////////////////////////////////
// Defines the possible purchase types
enum PurchaseType
{
	PT_CHARACTER=0,	// Character purchase
	PT_VEHICLE,		// Vehicle purchase
	PT_WEAPON,		// Weapon purchase
	PT_AMMO,		// Ammo purchase
	PT_HEALTH,		// Health purchase
	PT_REFILL,		// Both ammo and health purchase
	PT_OTHER		// Other purchases
};

////////////////////////////////////////////////////
// Purchase option definition
struct SPurchaseOption
{
	string szOptionName;	// String that is to be displayed on the purchase menu
	SPurchaseType type;		// Type of the purchase (e.g. Character, vehicle etc...
	std::list<SBuildingClassDef*> prereqs;	// Building that must be alive for the purchase to work
	std::list<string> items;// Items that are awarded upon purchase
	float buildTime;		// Time it takes to build the items
	float cost;				// Cost of the purchase
	string szImageFileName; // Path of the texture image that is displayed on the purchase menu
};

typedef std::map<unsigned int, SPurchaseOption> PurchaseMap;

////////////////////////////////////////////////////
class IPurchaseSystem
{
public:
	////////////////////////////////////////////////////
	// Destructor
	////////////////////////////////////////////////////
	virtual ~IPurchaseSystem(){}

	////////////////////////////////////////////////////
	// Initialize
	//
	// Purpose: One-time initialization at the start
	////////////////////////////////////////////////////
	virtual void Initialize() = 0;

	////////////////////////////////////////////////////
	// Shutdown
	//
	// Purpose: One-time clean up at the end
	////////////////////////////////////////////////////
	virtual void Shutdown() = 0;

	////////////////////////////////////////////////////
	// SetTeamManager
	//
	// Purpose: Set the team manager
	//
	// In:	tm - Team manager to use
	////////////////////////////////////////////////////
	virtual void SetTeamManager(ITeamManager const& tm) = 0;

	////////////////////////////////////////////////////
	// GetTeamManager
	//
	// Purpose: Returns the TeamManager in use
	////////////////////////////////////////////////////
	virtual ITeamManager const* GetTeamManager() const = 0;

	////////////////////////////////////////////////////
	// GetOptionByName
	//
	// Purpose: Finds and returns a Purchase Option with
	//			the specified name.
	// 
	// In:	name - The name of the Purchase Option
	//
	// Returns a pointer to a SPurchaseOption with the 
	//		   specified name or NULL if no such option
	//		   exists.
	//
	// Note: Slower, use ID if you have it!
	////////////////////////////////////////////////////
	virtual SPurchaseOption const* GetOptionByName(char const* name) = 0;

	////////////////////////////////////////////////////
	// GetOptionByID
	//
	// Purpose:	Finds and returns a Purchase Option with
	//			the specified ID.
	//
	// In:	ID - The ID of the option
	//
	// Returns a pointer to a SPurchaseOption with the
	//		   specified name or NULL if no such option
	//		   exists.
	////////////////////////////////////////////////////
	virtual SPurchaseOption const* GetOptionByID(unsigned int ID) = 0;

	////////////////////////////////////////////////////
	// RequestPurchase
	//
	// Purpose:	Purchases the specified purchase option
	//			for the specified player.
	//
	// In:	player - The player to award the purchase to
	//		option - The Purchase Option to award
	//
	// Returns true if the purchase was successful or 
	//		   false if the purchase is not available.
	////////////////////////////////////////////////////
	virtual bool RequestPurchase(CD6Player& player, SPurchaseOption const& option) = 0;

	////////////////////////////////////////////////////
	// IsOptionAvailable
	//
	// Purpose: Checks if the specified purchase option
	//			is available for purchase
	//
	// In:	option - The purchase option to check
	//
	// Returns true if the purchase option is available
	//		   or false if the option is not available.
	////////////////////////////////////////////////////
	virtual bool IsOptionAvailable(SPurchaseOption const& option) = 0;
};

#endif // ifndef _IPURCHASESYSTEM_H_