#ifndef __TYPEINFO_H
#define __TYPEINFO_H
#pragma once

#include <platform.h>
#include <CryArray.h>

#include <ISystem.h>

typedef const char*	cstr;

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Type info base class

// Although this pragma is also set from platform.h, for some reason it doesn't "take", 
// and we need it here again.
#if defined(XENON) || defined(PS3)
	#pragma bitfield_order( lsb_to_msb )
#endif


template<size_t SIZE>
struct FStaticAlloc
{
	static inline void* Alloc( void* oldptr, int oldsize, int newsize )
	{  
		static char		aMem[SIZE];
		static size_t	nAlloc = 0;

		// Check for realloc; will succeed only if it was the last item alloced.
		char* newptr = aMem + nAlloc - oldsize;
		if (oldptr != 0 && oldptr != newptr)
			// Realloc will succeed only if it was the last item alloced.
			return 0;

		if (nAlloc + newsize-oldsize > SIZE)
			// Out of memory
			return 0;
		nAlloc += newsize-oldsize;
		return newptr;
	}
};


struct CTypeInfo
{
	cstr		Name;
	size_t	Size;

	CTypeInfo( cstr name, size_t size )
		: Name(name), Size(size)
	{}

	template<class T>
		bool IsType() const
	{
		return this == &TypeInfo((T*)0);
	}

	//
	// Data access interface.
	//

	// String conversion flags.
	static const int WRITE_SKIP_DEFAULT	= 1;	// Omit default values on writing.
	static const int WRITE_TRUNCATE_SUB = 2;	// Remove trailing empty sub-vaules.
	static const int READ_SKIP_EMPTY		= 4;	// Do not set values from empty strings (otherwise, set to zero).

	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
	{
		assert(0);
		return "";
	}

	// Get value from string, return success.
	virtual bool FromString(void* data, cstr str, int flags = 0) const
	{
		assert(0);
		return false;
	}

	// Track memory used by any internal structures (not counting object size itself).
	// Add to CrySizer as needed, return remaining mem count.
	virtual size_t GetMemoryUsage(ICrySizer* pSizer, const void* data) const
	{
		return 0;
	}

	//
	// Structure interface.
	//
	struct CVarInfo
	{
		cstr								Name;
		const CTypeInfo&		Type;
		size_t							ArrayDim: 30,
												bBitfield: 1,				// How appropriate.
												bUnionAlias: 1;			// If 2nd or greater element of a union.
		size_t							Offset;

		struct CVarAttr
		{
			cstr	Name;
			float	fValue;
			cstr	sValue;
		};
		DynArray< CVarAttr, FStaticAlloc<0x4000> >	Attrs;

		explicit CVarInfo( const CTypeInfo& type, int dim = 1, cstr name = "" )
			: Type(type), ArrayDim(dim), bBitfield(0), bUnionAlias(0), Offset(0)
		{
			SetName(name);
		}

		// Manipulators, to easily modify type info.
		CVarInfo& SetName(cstr name)					
		{ 
			// Skip prefixes in Name.
			cstr sMain = name;
			while (islower(*sMain) || *sMain == '_')
				sMain++;
			if (isupper(*sMain))
				Name = sMain;
			else
				Name = name;
			return *this;
		}
		CVarInfo& SetOffset(size_t offset)		{ Offset = offset; return *this; }
		CVarInfo& SetArray(int dim)						{ ArrayDim *= dim; return *this; }
		CVarInfo& SetBitfield(int bits)				{ bBitfield = 1; ArrayDim = bits; return *this; }

		CVarInfo& AddAttr(cstr name, float val)	{ CVarAttr attr = { name, val, 0 }; Attrs.push_back(attr); return *this; }
		CVarInfo& AddAttr(cstr name, int val)		{ CVarAttr attr = { name, f32(val), 0 }; Attrs.push_back(attr); return *this; }
		CVarInfo& AddAttr(cstr name, cstr text)	{ CVarAttr attr = { name, 0, text }; Attrs.push_back(attr); return *this; }

		// Access.
		cstr GetDisplayName() const
		{ 
			return Name; 
		}
		size_t GetDim() const
		{
			return bBitfield ? 1 : ArrayDim;
		}
		size_t GetSize() const
		{
			return Type.Size * (bBitfield ? 1 : ArrayDim);
		}
		size_t GetBits() const
		{
			return bBitfield ? ArrayDim : ArrayDim * Type.Size * 8;
		}
	
		// Attribute access.
		bool GetAttr(cstr name) const
		{
			float val;
			return GetAttr(name, val) && val != 0.f;
		}
		bool GetAttr(cstr name, float& val) const;
		bool GetAttr(cstr name, cstr& val) const;

		// Useful functions.
		void* GetAddress(void* base) const
		{
			return (char*)base + Offset;
		}
		bool FromString(void* base, cstr str, int flags = 0) const
		{
			return Type.FromString( (char*)base + Offset, str, flags );
		}
		string ToString(void* base, int flags = 0) const
		{
			return Type.ToString( (char*)base + Offset, flags );
		}
	};

	// Structure var iteration:
	virtual const CVarInfo* NextSubVar(const CVarInfo* pPrev) const		{ return 0; }
	virtual const CVarInfo* FindSubVar(cstr name) const  { return 0; }
	inline bool HasSubVars() const  { return NextSubVar(0) != 0; }
	#define AllSubVars( pVar, Info ) (const CTypeInfo::CVarInfo* pVar = 0; pVar = Info.NextSubVar(pVar); )

	//
	// Enumeration interface.
	//
	struct CEnumElem
	{
		int			Value;
		cstr		FullName;
		cstr		ShortName;
	};
	virtual int EnumElemCount() const										{ return 0; }
	virtual CEnumElem const* EnumElem(int nIndex) const { return 0; }

protected:
	int ToInt(const void* data) const;
	bool FromInt(void* data, int val) const;
};

//---------------------------------------------------------------------------
// Define TypeInfo for a primitive type, without string conversion.
#define TYPE_INFO_PLAIN(T)													\
	template<> const CTypeInfo& TypeInfo(T*) {				\
		static CTypeInfo Info(#T, sizeof(T));						\
		return Info;																		\
	}																									\

//---------------------------------------------------------------------------
// Template for base types, using global To/FromString functions.

template<class T>
struct TTypeInfo: CTypeInfo
{
	TTypeInfo( cstr name )
		: CTypeInfo( name, sizeof(T) )
	{}
	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
	{
		if (def_data)
		{
			// Convert and comapre default values separately.
			string val = ::ToString(*(const T*)data, flags & ~WRITE_SKIP_DEFAULT);
			if (val.length())
			{
				string def_val = ::ToString(*(const T*)def_data, flags & ~WRITE_SKIP_DEFAULT);
				if (val == def_val)
					return string();
			}
			return val;
		}
		else
			return ::ToString(*(const T*)data, flags);
	}
	virtual bool FromString(void* data, cstr str, int flags = 0) const
	{
		if (!*str || !::FromString(*(T*)data, str))
		{
			if (flags & READ_SKIP_EMPTY)
				return false;
			*(T*)data = T();
		}
		return true;
	}
	virtual size_t GetMemoryUsage(ICrySizer* pSizer, void const* data) const
	{
		return 0;
	}
};

//---------------------------------------------------------------------------
// Define TypeInfo for a basic type (undecomposable as far as TypeInfo cares), with external string converters.
#define TYPE_INFO_BASIC(T)													\
	template<> const CTypeInfo& TypeInfo(T*) {				\
		static TTypeInfo< T > Info(#T);									\
		return Info;																		\
	}																									\

// Memory usage override.
template<> 
size_t TTypeInfo<string>::GetMemoryUsage(ICrySizer* pSizer, void const* data) const;

//---------------------------------------------------------------------------
// TypeInfo for structs

struct CStructInfo: CTypeInfo
{
	CStructInfo( cstr name, size_t size, size_t num_vars = 0, CVarInfo* vars = 0 );
	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const;
	virtual bool FromString(void* data, cstr str, int flags = 0) const;
	virtual size_t GetMemoryUsage(ICrySizer* pSizer, void const* data) const;

	virtual const CVarInfo* NextSubVar(const CVarInfo* pPrev) const
	{
		pPrev = pPrev ? pPrev+1 : Vars.begin();
		return pPrev < Vars.end() ? pPrev : 0;
	}
	virtual const CVarInfo* FindSubVar(cstr name) const;

protected:
	Array<CVarInfo>	Vars;

	bool FromStringParse(void* data, cstr& str, int flags) const;
};

//---------------------------------------------------------------------------
// Macros for constructing StructInfos (invoked by AutoTypeInfo.h)

#define STRUCT_INFO_EMPTY_BODY(T)									\
	{																								\
		static CStructInfo Info(#T, sizeof(T));				\
		return Info;																	\
	}																								\

#define STRUCT_INFO_EMPTY(T)											\
	const CTypeInfo& T::TypeInfo()									\
		STRUCT_INFO_EMPTY_BODY(T)											\

#define STRUCT_INFO_BEGIN(T)											\
	const CTypeInfo& T::TypeInfo() {								\
		typedef T STRUCTYPE;													\
		static CStructInfo::CVarInfo Vars[] = {				\

// Version of offsetof that takes the address of base classes.
// In Visual C++ at least, the fake address of 0 will NOT work;
// the compiler shifts ALL base classes to 0.
#define base_offsetof(s, b)			(size_t( static_cast<b*>( reinterpret_cast<s*>(0x100) )) - 0x100)

#define STRUCT_BASE_INFO(BaseType)																					\
			TYPE_INFO(BaseType) .SetOffset(base_offsetof(STRUCTYPE, BaseType)),		\

#define STRUCT_VAR_INFO(VarName, VarType)																		\
			VarType.SetName(#VarName) .SetOffset(offsetof(STRUCTYPE, VarName)),		\

#define ATTR_INFO(Name,Val)			.AddAttr(#Name,Val)
#define TYPE_INFO(t)						CStructInfo::CVarInfo(::TypeInfo((t*)0))
#define TYPE_ARRAY(n, info)			(info).SetArray(n)
#define	TYPE_POINTER(info)			CStructInfo::CVarInfo(::TypeInfo((void**)0))
#define TYPE_REF(info)					CStructInfo::CVarInfo(::TypeInfo((void**)0))

#define STRUCT_BITFIELD_INFO(VarName, VarType, Bits)																						\
			CStructInfo::CVarInfo( ::TypeInfo((VarType*)0) ) .SetName(#VarName) .SetBitfield(Bits),	\

#define STRUCT_INFO_END(T)																										\
	};																																					\
	static CStructInfo Info(#T, sizeof(T), sizeof Vars / sizeof *Vars, Vars);		\
	return Info;																																\
}																																							\

// Template versions

#define STRUCT_INFO_T_EMPTY(T, TArgs, TDecl)	\
	template TDecl STRUCT_INFO_EMPTY(T TArgs)

#define STRUCT_INFO_T_BEGIN(T, TArgs, TDecl)	\
	template TDecl STRUCT_INFO_BEGIN(T TArgs)

#define STRUCT_INFO_T_END(T, TArgs, TDecl)		\
	STRUCT_INFO_END(T TArgs)

#define STRUCT_INFO_T_INSTANTIATE(T, TArgs)		\
	template T TArgs;

// External versions

#define STRUCT_INFO_TYPE_EMPTY(T)							\
	template<> const CTypeInfo& TypeInfo(T*)		\
		STRUCT_INFO_EMPTY_BODY(T)									\

#define STRUCT_INFO_TYPE_BEGIN(T)							\
	template<> const CTypeInfo& TypeInfo(T*) {	\
		typedef T STRUCTYPE;											\
		static CStructInfo::CVarInfo Vars[] = {		\

#define STRUCT_INFO_TYPE_END(T)								\
	STRUCT_INFO_END(T)


// Template versions

#define STRUCT_INFO_TYPE_T_EMPTY(T, TArgs, TDecl)			\
	template TDecl const CTypeInfo& TypeInfo(T TArgs*)	\
		STRUCT_INFO_EMPTY_BODY(T TArgs)										\

//---------------------------------------------------------------------------
// Enum info
struct CEnumInfo: CTypeInfo
{
	CEnumInfo( cstr name, size_t size, size_t num_elems = 0, CEnumElem* elems = 0 );

	virtual string ToString(const void* data, int flags = 0, const void* def_data = 0) const
	{
		if (def_data && ToInt(def_data) == ToInt(data))
			return "";
		return ToString(ToInt(data), flags);
	}
	virtual bool FromString(void* data, cstr str, int flags = 0) const
	{
		int val;
		return FromString(val, str, flags) && FromInt(data, val);
	}
	virtual int EnumElemCount() const
	{
		return Elems.size();
	}
	virtual CEnumElem const* EnumElem(int nIndex) const 
	{ 
		return nIndex < (int)Elems.size() ? &Elems[nIndex] : 0;
	}

protected:
	Array<CEnumElem>	Elems;
	bool							bRegular;
	int								MinValue, MaxValue;

	string ToString(int val, int flags = 0) const;
	bool FromString(int& val, cstr str, int flags = 0) const;
};

#define ENUM_INFO_BEGIN(T)											\
	template<> const CTypeInfo& TypeInfo(T*) {		\
		static CEnumInfo::CEnumElem Elems[] = {			\

#define ENUM_ELEM_INFO(Scope, Elem)							\
			{ Scope Elem, #Elem },

#define ENUM_INFO_END(T)												\
		};																																						\
		static CEnumInfo Info(#T, sizeof(T), sizeof Elems / sizeof *Elems, Elems);		\
		return Info;																																	\
	}																																								\

#include "CryStructPack.h"

#endif // __TYPEINFO_H
