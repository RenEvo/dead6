//////////////////////////////////////////////////////////////////////
//
//	Crytek Serialization framework
//	
//	File: ISerialize.h
//  Description: main header file
//
//	History:
//	-27/07/2004: Craig Tiller, Created
//
//////////////////////////////////////////////////////////////////////

#ifndef __ISERIALIZE_H__
#define __ISERIALIZE_H__

#pragma once

#include "IEntity.h"
#include "IScriptSystem.h"

#include <vector>
#include <list>
#include <set>
#include <map>
#include <deque>
#include "MiniQueue.h"


class CTimeValue;
// Forward template declaration
template<class T,class U>
class InterpolatedValue_tpl;

// unfortunately this needs to be here - should be in CryNetwork somewhere
struct SNetObjectID
{
	static const uint16 InvalidId = ~uint16(0);

	SNetObjectID() : id(InvalidId), salt(0) {}
	SNetObjectID(uint16 i, uint16 s) : id(i), salt(s) {}

	uint16 id;
	uint16 salt;

	ILINE bool operator!() const
	{
		return id==InvalidId;
	}
	typedef uint16 (SNetObjectID::*unknown_bool_type);
	ILINE operator unknown_bool_type() const
	{
		return !!(*this)? &SNetObjectID::id : NULL;
	}
	ILINE bool operator!=( const SNetObjectID& rhs ) const
	{
		return !(*this == rhs);
	}
	ILINE bool operator==( const SNetObjectID& rhs ) const
	{
		return id==rhs.id && salt==rhs.salt;
	}
	ILINE bool operator<( const SNetObjectID& rhs ) const
	{
		return id<rhs.id || (id==rhs.id && salt<rhs.salt);
	}
	ILINE bool operator>( const SNetObjectID& rhs ) const
	{
		return id>rhs.id || (id==rhs.id && salt>rhs.salt);
	}

	bool IsLegal() const
	{
		return salt != 0;
	}

	const char * GetText( char * tmpBuf = 0 ) const
	{
		static char singlebuf[64];
		if (!tmpBuf)
			tmpBuf = singlebuf;
		if (id == InvalidId)
			sprintf(tmpBuf, "<nil>");
		else if (!salt)
			sprintf(tmpBuf, "illegal:%d:%d", id, salt);
		else
			sprintf(tmpBuf, "%d:%d", id, salt);
		return tmpBuf;
	}

	uint32 GetAsUint32() const
	{
		return (uint32(salt)<<16) | id;
	}

	AUTO_STRUCT_INFO
};

// this enumeration details what "kind" of serialization we are
// performing, so that classes that want to, for instance, tailor
// the data they present depending on where data is being written
// to can do so
enum ESerializationTarget
{
	eST_SaveGame,
	eST_Network,
	eST_Script
};

// this inner class defines an interface so that OnUpdate
// functions can be passed abstractly through to concrete
// serialization classes
struct ISerializeUpdateFunction
{
	virtual void Execute() = 0;
};

// concrete implementation of IUpdateFunction for a general functor class
template <class F_Update>
class CSerializeUpdateFunction : public ISerializeUpdateFunction
{
public:
	CSerializeUpdateFunction( F_Update& update ) : m_rUpdate(update) {}

	virtual void Execute()
	{
		m_rUpdate();
	}

private:
	F_Update m_rUpdate;
};

// the ISerialize is intended to be implemented by objects that need
// to read and write from various data sources, in such a way that
// different tradeoffs can be balanced by the object that is being
// serialized, and so that objects being serialized need only write
// a single function in order to be read from and written to
struct ISerialize
{
	static const int ENUM_POLICY_TAG = 0xe0000000;

	ILINE ISerialize() {}
	// this is for string values -- they need special support
	virtual void ReadStringValue( const char * name, string &curValue, uint32 policy = 0 ) = 0;
	virtual void WriteStringValue( const char * name, string& buffer, uint32 policy = 0 ) = 0;
	// this function should be implemented to call the passed in interface
	// if we are reading, and to not call it if we are writing
	virtual void Update( ISerializeUpdateFunction * pUpdate ) = 0;
	
	//////////////////////////////////////////////////////////////////////////
	// these functions should be implemented to deal with groups
	//////////////////////////////////////////////////////////////////////////

	// Begins a serialization group - must be matched by an EndGroup
	// szName is preferably as short as possible for performance reasons
	// Spaces in szName cause undefined behaviour, use alpha characters,underscore and numbers only for a name.
	virtual void BeginGroup( const char * szName ) = 0;
	virtual bool BeginOptionalGroup( const char * szName, bool condition ) = 0;
	virtual void EndGroup() = 0;
	//////////////////////////////////////////////////////////////////////////

	virtual bool IsReading() const = 0;
	virtual bool ShouldCommitValues() const = 0;
	virtual ESerializationTarget GetSerializationTarget() const = 0;
	virtual bool Ok() const = 0;

	// declare all primitive Value() implementations
#define SERIALIZATION_TYPE(T) \
	virtual void Value( const char * name, T& x, uint32 policy ) = 0;
#include "SerializationTypes.h"
#undef SERIALIZATION_TYPE

	// declare all primitive Value() implementations
#define SERIALIZATION_TYPE(T) \
	virtual void ValueWithDefault( const char * name, T& x, const T& defaultValue ) = 0;
#include "SerializationTypes.h"
#undef SERIALIZATION_TYPE

	template <class B>
	void Value( const char * name, B& x )
	{
		Value(name, x, 0);
	}

	template <class B>
	void Value( const char * name, B& x, uint32 policy );
};

// this class provides a wrapper so that ISerialize can be used much more
// easily; it is a template so that if we need to wrap a more specific
// ISerialize implementation we can do so easily
template <class TISerialize>
class CSerializeWrapper
{
public:
	CSerializeWrapper( TISerialize * pSerialize ) :
			m_pSerialize(pSerialize)
	{
	}

	// we provide a wrapper around the abstract implementation
	// ISerialize to allow easy changing of our
	// interface, and easy implementation of our details.
	// some of the wrappers are trivial, however for consistency, they
	// have been made to follow the trend.

	// the value function allows us to declare that a value needs
	// to be serialized/deserialized; we can pass a serialization policy
	// in order to compress the value, and an update function to allow
	// us to be informed of when this value is changed
	template <typename T_Value>
	ILINE void Value( const char * szName, T_Value& value, int policy )
	{
		m_pSerialize->Value( szName, value, policy );
	}

	template <typename T_Value>
	ILINE void Value( const char * szName, T_Value& value )
	{
		m_pSerialize->Value( szName, value );
	}
#if defined(PS3) && (defined(CELL_ARCH_CEB) || !defined(__LP32__))
#pragma message ( "Please fix me, Craig!" )
	ILINE void Value( const char * szName, std::size_t& value )
	{
		assert(0);
	}
#endif
	void Value( const char * szName, string& value, int policy )
	{
		if (IsWriting())
		{
			m_pSerialize->WriteStringValue(szName, value, policy);
		}
		else
		{
			if (GetSerializationTarget()!=eST_Script)
				value = "";
			m_pSerialize->ReadStringValue(szName, value, policy);
		}
	}
	ILINE void Value( const char * szName, string& value )
	{
		Value(szName, value, 0);
	}
	void Value( const char * szName, const string& value, int policy )
	{
		if (IsWriting())
		{
			m_pSerialize->WriteStringValue(szName, const_cast<string&>(value), policy);
		}
		else
		{
			assert( 0 && "This function can only be used for Writing" );
		}
	}
	ILINE void Value( const char * szName, const string& value )
	{
		Value(szName, value, 0);
	}
	template <class T_Class, typename T_Value>
	void Value( const char * szName, T_Class * pInst, T_Value (T_Class::*get)() const, void (T_Class::*set)(T_Value) )
	{
		if (IsWriting())
		{
			T_Value temp = (pInst->*get)();
			Value(szName, temp);
		}
		else
		{
			T_Value temp;
			Value(szName, temp);
			(pInst->*set)(temp);
		}
	}

	void Value( const char * name, IScriptTable * pTable )
	{
		ScriptAnyValue any(pTable);
		Value(name, any);
	}

	void Value( const char * name, SmartScriptTable& pTable )
	{
		ScriptAnyValue any(pTable);
		Value(name, any);
		if (IsReading())
		{
			if (any.type == ANY_TTABLE)
				pTable = any.table;
			else
				pTable = SmartScriptTable();
		}
	}

    template<class T, class TT>
    void Value(const char* name, InterpolatedValue_tpl<T,TT>& val)
    {
        if (IsWriting())
        {
            T a = val.Get();
            Value(name, a);
        }

        if (IsReading())
        {
            T a;
            Value(name,a);
            val.SetGoal(a);
        }
    }

    template<class T, class TT>
    void Value(const char* name, InterpolatedValue_tpl<T,TT>& val,int policy)
    {
        if (IsWriting())
        {
            T a = val.Get();
            Value(name, a, policy);
        }

        if (IsReading())
        {
            T a;
            Value(name,a,policy);
            val.SetGoal(a);
        }
    }

	template <typename T>
	void ValueWithDefault( const char * name, T& x, const T &defaultValue )
	{
		m_pSerialize->ValueWithDefault(name,x,defaultValue);
	}

	template <class T_Class, typename T_Value, class T_SerializationPolicy>
	void Value( const char * szName, T_Class * pInst, T_Value (T_Class::*get)() const, void (T_Class::*set)(T_Value),
		const T_SerializationPolicy& policy )
	{
		if (IsWriting())
		{
			T_Value temp = (pInst->*get)();
			Value(szName, temp, policy);
		}
		else
		{
			T_Value temp;
			Value(szName, temp, policy);
			(pInst->*set)(temp);
		}
	}

	// a value that is written by referring to a map of key/value pairs - we receive the key, and write the value
	template <class T_Key, class T_Map>
	void MappedValue( const char * szName, T_Key& value, const T_Map& mapper )
	{
		typedef typename T_Map::ValueType T_Value;

		if (IsWriting())
		{
			T_Value write = mapper.KeyToValue(value);
			Value(szName, write);
		}
		else
		{
			T_Value read;
			Value(szName, read);
			value = mapper.ValueToKey(read);
		}
	}

#define CONTAINER_VALUE(container_type, insert_function) \
	template <typename T_Value, class Allocator> \
	void Value( const char * name, container_type<T_Value, Allocator>& cont ) \
	{ \
		if (!BeginOptionalGroup(name,true)) return; \
		if (IsWriting()) \
		{ \
			uint32 count = cont.size(); \
			Value("Size", count); \
			for (typename container_type<T_Value, Allocator>::iterator iter = cont.begin(); iter != cont.end(); ++iter) \
			{ \
				BeginGroup("i");\
				T_Value value = *iter; \
				Value("v", value); \
				EndGroup();\
			} \
		} \
		else \
		{ \
			cont.clear(); \
			uint32 count = 0; \
			Value("Size", count); \
			while (count--) \
			{ \
				BeginGroup("i");\
				T_Value temp; \
				Value("v", temp); \
				cont.insert_function(temp); \
				EndGroup();\
			} \
		} \
		EndGroup(); \
	} \
	template <typename T_Value, class T_Map> \
	void MappedValue( const char * name, container_type<T_Value>& cont, const T_Map& mapper ) \
	{ \
		if (!BeginOptionalGroup(name,true)) return; \
		if (IsWriting()) \
		{ \
			uint32 count = cont.size(); \
			Value("Size", count); \
			for (typename container_type<T_Value>::iterator iter = cont.begin(); iter != cont.end(); ++iter) \
			{\
				BeginGroup("i");\
				MappedValue("v", *iter, mapper); \
				EndGroup();\
			}\
		} \
		else \
		{ \
			cont.clear(); \
			uint32 count = 0; \
			Value("Size", count); \
			while (count--) \
			{ \
				BeginGroup("i");\
				T_Value temp; \
				MappedValue("v", temp, mapper); \
				cont.insert_function(temp); \
				EndGroup();\
			} \
		} \
		EndGroup(); \
	}

#define PAIR_CONTAINER_VALUE(container_type, insert_function) \
	template <typename T_Value1,typename T_Value2, class Allocator> \
	void Value( const char * name, container_type<std::pair<T_Value1,T_Value2>, Allocator>& cont ) \
	{ \
		if (!BeginOptionalGroup(name,true)) return; \
		if (IsWriting()) \
		{ \
			uint32 count = cont.size(); \
			Value("Size", count); \
			for (typename container_type<std::pair<T_Value1,T_Value2>, Allocator>::iterator iter = cont.begin(); iter != cont.end(); ++iter) \
			{ \
				BeginGroup("i");\
				T_Value1 value1 = *iter->first; \
				T_Value2 value2 = *iter->second; \
				Value("v1", value1); \
				Value("v2", value2); \
				EndGroup();\
			} \
		} \
		else \
		{ \
			cont.clear(); \
			uint32 count = 0; \
			Value("Size", count); \
			while (count--) \
			{ \
				BeginGroup("i");\
				T_Value1 temp1; \
				T_Value2 temp2; \
				Value("v1", temp1); \
				Value("v2", temp2); \
				cont.insert_function(std::pair<T_Value1,T_Value2>(temp1,temp2)); \
				EndGroup();\
			} \
		} \
		EndGroup(); \
	} \

	CONTAINER_VALUE(std::vector, push_back);
	CONTAINER_VALUE(std::list, push_back);
	CONTAINER_VALUE(std::set, insert);
	CONTAINER_VALUE(std::deque, push_back);

	PAIR_CONTAINER_VALUE(std::list, push_back);
	PAIR_CONTAINER_VALUE(std::vector, push_back);

	template <typename T_Value, uint8 N>
	void Value( const char * name, MiniQueue<T_Value, N>& cont )
	{
		if (!BeginOptionalGroup(name,true)) return;
		if (IsWriting())
		{
			uint32 count = cont.Size();
			Value("Size", count);
			for (typename MiniQueue<T_Value, N>::SIterator iter = cont.Begin(); iter != cont.End(); ++iter)
			{
				T_Value value = *iter;
				Value("Value", value);
			}
		}
		else
		{
			cont.Clear();
			uint32 count = 0;
			Value("Size", count);
			while (count--)
			{
				T_Value temp;
				Value("Value", temp);
				cont.Push(temp);
			}
		}
		EndGroup();
	}
	template <typename T_Value, uint8 N, class T_Map>
	void MappedValue( const char * name, MiniQueue<T_Value, N>& cont, const T_Map& mapper )
	{
		if (!BeginOptionalGroup(name,true)) return;
		if (IsWriting())
		{
			uint32 count = cont.Size();
			Value("Size", count);
			for (typename MiniQueue<T_Value, N>::SIterator iter = cont.Begin(); iter != cont.End(); ++iter)
			{
				BeginGroup("i");
				MappedValue("Value", *iter, mapper);
				EndGroup();
			}
		}
		else
		{
			cont.Clear();
			uint32 count = 0;
			Value("Size", count);
			while (count--)
			{
				BeginGroup("i");
				T_Value temp;
				MappedValue("Value", temp, mapper);
				cont.Push(temp);
				EndGroup();
			}
		}
		EndGroup();
	}

#define MAP_CONTAINER_VALUE(container_type) \
	template <typename T_Key, typename T_Value> \
	void Value( const char * name, container_type<T_Key, T_Value>& cont ) \
	{ \
		BeginGroup(name); \
		if (IsWriting()) \
		{ \
			uint32 count = cont.size(); \
			Value("Size", count); \
			for (typename container_type<T_Key, T_Value>::iterator iter = cont.begin(); iter != cont.end(); ++iter) \
			{ \
				T_Key tempKey = iter->first; \
				BeginGroup("pair"); \
				Value("k", tempKey); \
				Value("v", iter->second); \
				EndGroup(); \
			} \
		} \
		else \
		{ \
			cont.clear(); \
			uint32 count; \
			Value("Size", count); \
			while (count--) \
			{ \
				std::pair<T_Key, T_Value> temp; \
				BeginGroup("pair"); \
				Value("k", temp.first); \
				Value("v", temp.second); \
				EndGroup(); \
				cont.insert(temp); \
			} \
		} \
		EndGroup(); \
	}

	MAP_CONTAINER_VALUE(std::map);
	MAP_CONTAINER_VALUE(std::multimap);

	template <typename T_Value>
	ILINE void EnumValue( const char * szName, T_Value& value, 
												T_Value first, T_Value last )
	{
		int32 nValue = int32(value) - first;
		Value( szName, nValue, ISerialize::ENUM_POLICY_TAG | (last-first) );
		value = T_Value(nValue + first);
	}
	template <typename T_Value, class T_Class>
	ILINE void EnumValue( const char * szName, 
		T_Class * pClass, T_Value (T_Class::*GetValue)() const, void (T_Class::*SetValue)(T_Value), 
		T_Value first, T_Value last )
	{
		bool w = IsWriting();
		int nValue;
		if (w)
			nValue = int32((pClass->*GetValue)()) - first;
		Value( szName, nValue, ISerialize::ENUM_POLICY_TAG | (last-first) );
		if (!w)
			(pClass->*SetValue)(T_Value(nValue+first));
	}
	/*
	// we can request that a functor be called whenever our values
	// are being updated by calling this function
	template <class F_Update>
	ILINE void OnUpdate( F_Update& update )
	{
		CUpdateFunction<F_Update> func(update);
		m_pSerialize->Update( &func );
	}
	template <class T>
	ILINE void OnUpdate( T * pCls, void (T::*func)() )
	{
		class CFunc : public IUpdateFunction
		{
		public:
			CFunc( T * pCls, void (T::*func)() ) : m_pCls(pCls), m_func(func) {}
			virtual void Execute()
			{
				(m_pCls->*m_func)();
			}
		private:
			T * m_pCls;
			void (T::*m_func)();
		};
		CFunc ifunc( pCls, func );
		m_pSerialize->Update( &ifunc );
	}
	*/
	// groups help us find common data
	ILINE void BeginGroup( const char * szName )
	{
		m_pSerialize->BeginGroup( szName );
	}
	ILINE bool BeginOptionalGroup( const char * szName, bool condition )
	{
		return m_pSerialize->BeginOptionalGroup( szName, condition );
	}
	ILINE void EndGroup()
	{
		m_pSerialize->EndGroup();
	}

	// fetch the serialization target
	ILINE ESerializationTarget GetSerializationTarget() const
	{
		return m_pSerialize->GetSerializationTarget();
	}

	ILINE bool IsWriting() const
	{
		return !m_pSerialize->IsReading();
	}
	ILINE bool IsReading() const
	{
		return m_pSerialize->IsReading();
	}
	ILINE bool ShouldCommitValues() const
	{
		assert( m_pSerialize->IsReading() );
		return m_pSerialize->ShouldCommitValues();
	}

	ILINE bool Ok() const
	{
		return m_pSerialize->Ok();
	}

	friend ILINE TISerialize * GetImpl( CSerializeWrapper<TISerialize> ser )
	{
		return ser.m_pSerialize;
	}

	operator CSerializeWrapper<ISerialize>()
	{
		return CSerializeWrapper<ISerialize>( m_pSerialize );
	}

private:
	TISerialize * m_pSerialize;
};

// default serialize class to use!!
typedef CSerializeWrapper<ISerialize> TSerialize;

// simple struct to declare something serializable... useful for
// exposition
struct ISerializable
{
	virtual void SerializeWith( TSerialize ) = 0;
};

template <class B>
void ISerialize::Value( const char * name, B& x, uint32 policy )
{
	if (!BeginOptionalGroup(name,true)) return;
	x.Serialize( TSerialize(this) );
	EndGroup();
}

#endif
