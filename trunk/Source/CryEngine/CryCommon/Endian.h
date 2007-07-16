////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   Endian.h
//  Version:     v1.00
//  Created:     16/2/2006 by Scott,Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Endian_h__
#define __Endian_h__
#pragma once

//////////////////////////////////////////////////////////////////////////
// Meta-type support.
//////////////////////////////////////////////////////////////////////////

// Currently enable type info for all platforms.
#if !defined(ENABLE_TYPE_INFO)
#define ENABLE_TYPE_INFO
#endif
#ifdef ENABLE_TYPE_INFO

struct CTypeInfo;

// If TypeInfo exists for T, it is accessed via TypeInfo(T*).
// Default TypeInfo() is implemented by a static struct member function.
template<class T>
inline const CTypeInfo& TypeInfo(T*)
{
	return T::TypeInfo();
}

// Declare a class's TypeInfo member
#define STRUCT_INFO		\
	static const CTypeInfo& TypeInfo();

// Declare an override for a type without TypeInfo() member (e.g. basic type)
#define DECLARE_TYPE_INFO(Type)		\
	template<> const CTypeInfo& TypeInfo(Type*);

// Template version.
#define DECLARE_TYPE_INFO_T(Type)		\
	template<class T> const CTypeInfo& TypeInfo(Type<T>*);

// Type info declaration, with additional prototypes for string conversions.
#define BASIC_TYPE_INFO(Type)										\
	string ToString(Type const& val, int flags);	\
	bool FromString(Type& val, const char *s);	\
	DECLARE_TYPE_INFO(Type)

#else

#define STRUCT_INFO
#define DECLARE_TYPE_INFO(Type)
#define DECLARE_TYPE_INFO_T(Type)

#endif

// Specify automatic tool generation of TypeInfo bodies.
#define AUTO_STRUCT_INFO							STRUCT_INFO
#define AUTO_TYPE_INFO								DECLARE_TYPE_INFO
#define AUTO_TYPE_INFO_T							DECLARE_TYPE_INFO_T

#define AUTO_STRUCT_INFO_LOCAL				STRUCT_INFO
#define AUTO_TYPE_INFO_LOCAL					DECLARE_TYPE_INFO
#define AUTO_TYPE_INFO_LOCAL_T				DECLARE_TYPE_INFO_T

// Overrides for basic types.
BASIC_TYPE_INFO(bool)

BASIC_TYPE_INFO(char)
BASIC_TYPE_INFO(signed char)
BASIC_TYPE_INFO(unsigned char)
BASIC_TYPE_INFO(short)
BASIC_TYPE_INFO(unsigned short)
BASIC_TYPE_INFO(int)
BASIC_TYPE_INFO(unsigned int)
BASIC_TYPE_INFO(long)
BASIC_TYPE_INFO(unsigned long)
BASIC_TYPE_INFO(int64)
BASIC_TYPE_INFO(uint64)

BASIC_TYPE_INFO(float)
BASIC_TYPE_INFO(double)

BASIC_TYPE_INFO(void*)

BASIC_TYPE_INFO(string)

//////////////////////////////////////////////////////////////////////////
// Endian support: If not needed, define empty SwapEndian function
//////////////////////////////////////////////////////////////////////////

#ifdef NEED_ENDIAN_SWAP

// The endian swapping function.
void SwapEndian(void* pData, size_t nCount, const CTypeInfo& Info, size_t nSizeCheck);

template<class T>
inline void SwapEndian(T* t, size_t nCount)
{
	SwapEndian(t, nCount, TypeInfo((T*)0), sizeof(T));
}

#else	// NEED_ENDIAN_SWAP

// Null endian swapping.
template<class T>
inline void SwapEndian(T* t, size_t nCount)
{}

#endif	// NEED_ENDIAN_SWAP

// Derivative functions.
template<class T>
inline void SwapEndian(T& t)
{
	SwapEndian(&t, 1);
}

template<class T>
inline T SwapEndianValue(T t)
{
	SwapEndian(t);
	return t;
}

// Object-oriented data extraction for endian-swapping reading.
template<class T>
inline T* StepData(unsigned char*& pData, int* pnDataSize = NULL, int nCount = 1, bool bSwap = true)
{
	T* Elems = (T*)pData;
	if (bSwap)
		SwapEndian(Elems, nCount);
	pData += sizeof(T)*nCount;
	if (pnDataSize)
		*pnDataSize -= sizeof(T)*nCount;
	return Elems;
}

template<class T>
inline void StepData(T*& Result, unsigned char*& pData, int* pnDataSize = NULL, int nCount = 1, bool bSwap = true)
{
	Result = StepData<T>(pData, pnDataSize, nCount, bSwap);
}

template<class T>
inline void StepDataCopy(T* Dest, unsigned char*& pData, int* pnDataSize = NULL, int nCount = 1, bool bSwap = true)
{
	memcpy(Dest, StepData<T>(pData, pnDataSize, nCount, bSwap), nCount*sizeof(T));
}

#endif // __Endian_h__
