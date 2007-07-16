////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2004.
// -------------------------------------------------------------------------
//  File name:   PS3Specific.h
//  Version:     v1.00
//  Created:     05/03/2004 by MichaelG.
//  Compilers:   Visual Studio.NET, GCC 3.2
//  Description: Specific to PS3 declarations, inline functions etc.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CRY_COMMON_PS3_SPECIFIC_HDR_
#define _CRY_COMMON_PS3_SPECIFIC_HDR_

//////////////////////////////////////////////////////////////////////////
// Standart includes.
//////////////////////////////////////////////////////////////////////////
#include <ctype.h>
//#include <stdlib.h> 
//#include <time.h>
//#include <pthread.h>
//#include <pu_thread.h>
#include <math.h>
#include <string.h>
#include <errno.h>
//#include <sys/io.h>
#include <stddef.h>
#include <float.h>
#if !defined(__SPU__)
	#include <sys/types.h>
	#include <sys/paths.h>
	#include <sys/select.h>
	#include <sys/socket.h>
//////////////////////////////////////////////////////////////////////////

	//temp definitions for GetAsynchKeyState, should not be used on PS3
	#define VK_SCROLL         0x91
	#define VK_UP             0x26
	#define VK_CONTROL        0x11
	#define VK_DOWN           0x28
	#define VK_CONTROL        0x11
	#define VK_RIGHT          0x27
	#define VK_LEFT           0x25
	#define VK_SPACE          0x20

	#define _PS3

	//this defines enables all file loading from hard disk
	#define USE_HDD0
	#if defined(USE_HDD0)
		#include <sysutil/sysutil_common.h>
		#include <sysutil/sysutil_gamedata.h>
		extern char *g_pCurDirHDD0;
		extern int g_pCurDirHDD0Len;
	#endif
	//-------------------------------------socket stuff------------------------------------------

	#define select socketselect
	#define closesocket socketclose

	#define SOCKET int
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)

	typedef struct in_addr_windows 
	{
		union 
		{
			struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
			struct { unsigned short s_w1,s_w2; } S_un_w;
			unsigned int S_addr;
		} S_un;
	}in_addr_windows;

	#define WSAEINTR SYS_NET_EINTR
	#define WSAEBADF SYS_NET_EBADF
	#define WSAEACCES SYS_NET_EACCES
	#define WSAEFAULT SYS_NET_EFAULT
	#define WSAEACCES SYS_NET_EACCES
	#define WSAEFAULT SYS_NET_EFAULT
	#define WSAEINVAL SYS_NET_EINVAL
	#define WSAEMFILE SYS_NET_EMFILE
	#define WSAEWOULDBLOCK SYS_NET_EAGAIN
	#define WSAEINPROGRESS SYS_NET_EINPROGRESS
	#define WSAEALREADY SYS_NET_EALREADY
	#define WSAENOTSOCK SYS_NET_ENOTSOCK 
	#define WSAEDESTADDRREQ SYS_NET_EDESTADDRREQ
	#define WSAEMSGSIZE SYS_NET_EMSGSIZE
	#define WSAEPROTOTYPE SYS_NET_EPROTOTYPE
	#define WSAENOPROTOOPT SYS_NET_ENOPROTOOPT
	#define WSAEPROTONOSUPPORT SYS_NET_EPROTONOSUPPORT
	#define WSAESOCKTNOSUPPORT SYS_NET_ESOCKTNOSUPPORT
	#define WSAEOPNOTSUPP SYS_NET_EOPNOTSUPP
	#define WSAEPFNOSUPPORT SYS_NET_EPFNOSUPPORT
	#define WSAEAFNOSUPPORT SYS_NET_EAFNOSUPPORT
	#define WSAEADDRINUSE SYS_NET_EADDRINUSE
	#define WSAEADDRNOTAVAIL SYS_NET_EADDRNOTAVAIL
	#define WSAENETDOWN SYS_NET_ENETDOWN
	#define WSAENETUNREACH SYS_NET_ENETUNREACH
	#define WSAENETRESET SYS_NET_ENETRESET
	#define WSAECONNABORTED SYS_NET_ECONNABORTED
	#define WSAECONNRESET SYS_NET_ECONNRESET
	#define WSAENOBUFS SYS_NET_ENOBUFS
	#define WSAEISCONN SYS_NET_EISCONN
	#define WSAENOTCONN SYS_NET_ENOTCONN
	#define WSAESHUTDOWN SYS_NET_ESHUTDOWN
	#define WSAETOOMANYREFS SYS_NET_ETOOMANYREFS
	#define WSAETIMEDOUT SYS_NET_ETIMEDOUT
	#define WSAECONNREFUSED SYS_NET_ECONNREFUSED
	#define WSAELOOP SYS_NET_ELOOP
	#define WSAENAMETOOLONG SYS_NET_ENAMETOOLONG
	#define WSAEHOSTDOWN SYS_NET_EHOSTDOWN
	#define WSAEHOSTUNREACH SYS_NET_EHOSTUNREACH
	#define WSAENOTEMPTY SYS_NET_ENOTEMPTY
	#define WSAEPROCLIM SYS_NET_EPROCLIM
	#define WSAEUSERS SYS_NET_EUSERS
	#define WSAEDQUOT SYS_NET_EDQUOT
	#define WSAESTALE SYS_NET_ESTALE
	#define WSAEREMOTE SYS_NET_EREMOTE

	#define WSAHOST_NOT_FOUND (1024 + 1)
	#define WSATRY_AGAIN (1024 + 2)
	#define WSANO_RECOVERY (1024 + 3)
	#define WSANO_DATA (1024 + 4)
	#define WSANO_ADDRESS (WSANO_DATA)

	#define SD_RECEIVE      SHUT_RD
	#define SD_SEND         SHUT_WR
	#define SD_BOTH         SHUT_RDWR

	//-------------------------------------end socket stuff------------------------------------------

	#define SCOPED_ENABLE_FLOAT_EXCEPTIONS

	// Flag indicating if the system threading library should be used instead
	// of the POSIX threading library.
	//#undef USE_SYSTEM_THREADS
	#define USE_SYSTEM_THREADS 1

#endif //__SPU__

#define SIZEOF_PTR 4

//built in stack allocation
#if !defined(alloca) && !defined(__SPU__)
	#define alloca(size) __builtin_alloca(size)	
#endif

#undef IF
#if !defined(__SPU__)
	//dummy for cache control in spu jobs (must stay compilable on PPU too)
	inline void SPUAddCacheWriteRangeAsync(const unsigned int, const unsigned int){}
	#define __cache_range_write_async(a,b)
#endif//__SPU__
#if !defined(__SPU__) || !defined(SUPP_BRANCH_HINTS)
	#define IF(a, b) if((a))
	#define WHILE(a, b) while((a))
#else
	#define IF(a, b) if(__builtin_expect((a), (b)))
	#define WHILE(a, b) while(__builtin_expect((a), (b)))
#endif //!defined(__SPU__) || !defined(SUPP_BRANCH_HINTS)
//#define _CPU_X86

#define DEBUG_BREAK
#define RC_EXECUTABLE "rc"
#if defined(__SPU__)
	#define __forceinline __attribute__((always_inline))
#else
	#define __forceinline inline
#endif
#if !defined(USE_STATIC_NAME_TABLE)
#define USE_STATIC_NAME_TABLE 1
#endif
//#if !defined(_STLP_HASH_MAP)
//#define _STLP_HASH_MAP 1
//#endif
#define USE_CRT 1
#define TYPENAME(x) "<Not Supported>"

#define stricmp strcasecmp


//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
typedef void*								LPVOID;
#define VOID            		void
#define PVOID								void*

typedef signed char         int8;
typedef signed char         INT8;
typedef signed short        int16;
typedef signed short        INT16;
typedef signed int					int32;
typedef int									INT32;
typedef signed long long		int64;
typedef signed long long		INT64;
typedef unsigned char				uint8;
typedef unsigned char				UINT8;
typedef unsigned short			uint16;
typedef unsigned short			UINT16;
typedef unsigned int				uint32;
typedef unsigned int				UINT32;
typedef unsigned long long	uint64;

typedef float               f32;
typedef double              f64;
typedef double              real;  //biggest float-type on this machine

/*
	pointers on SPU side are 4 byte rather than on PPU with 8 byte
	if a structure is to be used on PPU and SPU, they need to be the same in memory
	therefore pointers need to be specified with _spu_pad_(index) after the name:
		struct Foo
		{
			int* p _spu_pad_(0);
			int* p1 _spu_pad_(1);
		};
	if multiple pointers are within a struct, use _spu_pad_(0), _spu_pad_(1) for the 2nd, 3rd and so on
*/
#if defined(__SPU__)
	#include <stdint.h>
#endif
typedef intptr_t INT_PTR, *PINT_PTR;
typedef uintptr_t UINT_PTR, *PUINT_PTR;

typedef char *LPSTR, *PSTR;

typedef long LONG_PTR, *PLONG_PTR, *PLONG;
typedef unsigned long ULONG_PTR, *PULONG_PTR;

typedef unsigned int				DWORD;
typedef uintptr_t						DWORD_PTR;
typedef unsigned int*				LPDWORD;
typedef unsigned char				BYTE;
typedef unsigned short			WORD;
typedef int                 INT;
typedef unsigned int        UINT;
typedef float               FLOAT;
typedef void*								HWND;
typedef UINT_PTR 						WPARAM;
typedef LONG_PTR 						LPARAM;
typedef LONG_PTR 						LRESULT;
#define PLARGE_INTEGER LARGE_INTEGER*
typedef const char *LPCSTR, *PCSTR;
typedef long long						LONGLONG;
typedef	ULONG_PTR						SIZE_T;
typedef unsigned char				byte;

//shortens alignment declaration
#define _ALIGN(num) __attribute__ ((aligned(num)))	
#define _PACK __attribute__ ((packed))

#define ILINE __forceinline

#define UINT32_C(x)	x##U
#define INT32_C(x)	x
#define UINT64_C(x)	x##Ull
#define INT64_C(x)	x##ll
#define UINT16_C(x)	x
#define INT16_C(x)	x
#define UINT8_C(x)	x
#define INT8_C(x)	x


//#define PHYSICS_EXPORTS

// MSVC compiler-specific keywords
#define _inline inline
#define __cdecl
#define _cdecl
#define _stdcall
#define __stdcall
#define _fastcall
#define __fastcall

// Safe memory freeing
#ifndef SAFE_DELETE
	#define SAFE_DELETE(p)			{ if(p) { delete (p);		(p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete[] (p);		(p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p)			{ if(p) { (p)->Release();	(p)=NULL; } }
#endif

#ifndef SAFE_RELEASE_FORCE
	#define SAFE_RELEASE_FORCE(p)			{ if(p) { (p)->ReleaseForce();	(p)=NULL; } }
#endif

#define DEFINE_ALIGNED_DATA( type, name, alignment ) \
	type __attribute__ ((aligned(alignment))) name;
#define DEFINE_ALIGNED_DATA_STATIC( type, name, alignment ) \
	static type __attribute__ ((aligned(alignment))) name;
#define DEFINE_ALIGNED_DATA_CONST( type, name, alignment ) \
	const type __attribute__ ((aligned(alignment))) name;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD_PTR)(w) >> 8))

#define CALLBACK
#define WINAPI

#ifndef __cplusplus
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define TCHAR wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif
typedef char CHAR;
typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWCH, *PWCH;
typedef const WCHAR *LPCWCH, *PCWCH;
typedef WCHAR *NWPSTR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef WCHAR *LPUWSTR, *PUWSTR;

typedef const WCHAR *LPCWSTR, *PCWSTR;
typedef const WCHAR *LPCUWSTR, *PCUWSTR;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define FILE_ATTRIBUTE_NORMAL               0x00000080

typedef int							BOOL;
typedef long						LONG;
typedef unsigned long		ULONG;
typedef int 						HRESULT;

#define _PTRDIFF_T_DEFINED

#define TRUE 1
#define FALSE 0

#ifndef MAX_PATH
	#if defined(USE_HDD0)
		#define MAX_PATH CELL_GAMEDATA_PATH_MAX
	#else
		#define MAX_PATH 256
	#endif
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#ifdef __cplusplus
/*	static pthread_mutex_t mutex_t;
template<typename T>
const volatile T InterlockedIncrement(volatile T* pT)
{
pthread_mutex_lock(&mutex_t);
++(*pT);
pthread_mutex_unlock(&mutex_t);
return *pT;
}

template<typename T>
const volatile T InterlockedDecrement(volatile T* pT)
{
pthread_mutex_lock(&mutex_t);
--(*pT);
pthread_mutex_unlock(&mutex_t);
return *pT;
}
*/

template<typename S, typename T>
inline const S& __min(const S& rS, const T& rT)
{
	return (rS <= rT)?rS : (const S&)rT;
}

template<typename S, typename T>
inline const S& __max(const S& rS, const T& rT)
{
	return (rS >= rT)?rS : (const S&)rT;
}

// Order bitfields LSB first, matching little-endian systems.
// Then we can endian-convert bitfields by just endian-converting the containing ints.
#pragma bitfield_order( lsb_to_msb )

//////////////////////////////////////////////////////////////////////////
#define NEED_ENDIAN_SWAP

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Multi platform Hi resolution ticks function, should only be used for profiling.
//////////////////////////////////////////////////////////////////////////
#if !defined(__SPU__)
	__forceinline uint64 CryGetTicks()
	{
		uint64 time = 0;
		__asm volatile ("mftb %0": [time] "=r" (time));
		return time;
	}
#endif

#endif //__cplusplus

#if !defined(__SPU__)
  // PS3_Win32Wrapper.h now directly included by platform.h.
	//#include "PS3_Win32Wrapper.h"

#endif //SPU

//conditional selects

//implements branch free: 
//return (cA > cB)?cA : cB;
#if !defined(__SPU__)
template <class T>
T CondSelMax(const T cA, const T cB);
template <class T>
T CondSelMin(const T cA, const T cB);

template <class T>
__attribute__((always_inline))
inline T CondSelMax(const T cA, const T cB)
{
	const uint32 cMinMask = (uint32)(((int32)(cA - cB)) >> 31);
	return (cA & ~cMinMask | cB & cMinMask);
}

//implements branch free for integer types: 
//return (cA < cB)?cA : cB;
template <class T>
__attribute__((always_inline))
inline T CondSelMin(const T cA, const T cB)
{
	const uint32 cMinMask = (uint32)(((int32)(cB - cA)) >> 31);
	return (cA & ~cMinMask | cB & cMinMask);
}

template <>
__attribute__((always_inline))
inline float CondSelMin<float>(const float cA, const float cB)
{
	return (cA < cB)?cA : cB;
}

template <>
__attribute__((always_inline))
inline float CondSelMax<float>(const float cA, const float cB)
{
	return (cA > cB)?cA : cB;
}
#endif

#endif //_CRY_COMMON_PS3_SPECIFIC_HDR_
// vim:ts=2:sw=2:tw=78

