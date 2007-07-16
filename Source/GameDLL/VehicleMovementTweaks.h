/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a class which handle case (group) specific 
tweaking of values of a vehicle movement

-------------------------------------------------------------------------
History:
- 13:06:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEMOVEMENTTWEAKS_H__
#define __VEHICLEMOVEMENTTWEAKS_H__

#include <list>

class CVehicleMovementBase;

class CVehicleMovementTweaks
{
public:

	typedef int TTweakGroupId;
	static const TTweakGroupId InvalidTweakGroupId = -1;

public:

	CVehicleMovementTweaks() {}
	~CVehicleMovementTweaks() {}

	bool Init(const SmartScriptTable &table);
	void AddValue(const char* valueName, float* pValue, bool isRestrictedToMult = false);

	bool UseGroup(TTweakGroupId groupId);
	bool RevertGroup(TTweakGroupId groupId);
	bool RevertValues();

	TTweakGroupId GetGroupId(const char* name);

	void Serialize(TSerialize ser, unsigned aspects);

protected:

	typedef int TValueId;

	enum ETweakValueOperator
	{
		eTVO_Replace = 0,
		eTVO_Multiplier,
	};

	struct SValue
	{
		string name;
		float defaultValue;
		float* pValue;
		bool isRestrictedToMult;
	};

	typedef std::vector <SValue> TValueVector;

	struct SGroup
	{
		string name;

		struct SValueInGroup
		{
			TValueId valueId;
			float value;
			int op;
		};

		typedef std::vector <SValueInGroup> TValueInGroupVector;
		TValueInGroupVector values;

		bool isEnabled;
	};

	typedef std::vector <SGroup> TGroupVector;

protected:

	bool AddGroup(const SmartScriptTable &table);
	TValueId GetValueId(const char* name);

	void ComputeGroups();
	void ComputeGroup(const SGroup& group);

	TValueVector m_values;
	TGroupVector m_groups;
};

#endif
