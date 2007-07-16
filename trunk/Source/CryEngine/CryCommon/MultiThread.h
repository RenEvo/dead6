////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   MultiThread.h
//  Version:     v1.00
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MultiThread_h__
#define __MultiThread_h__
#pragma once

#if !defined(__SPU__)
	long   CryInterlockedIncrement( int volatile *lpAddend );
	long   CryInterlockedDecrement( int volatile *lpAddend );
	long   CryInterlockedExchangeAdd(long volatile * lpAddend, long Value);
	long	 CryInterlockedCompareExchange(long volatile * dst, long exchange, long comperand);
	void*  CryCreateCriticalSection();
	void   CryDeleteCriticalSection( void *cs );
	void   CryEnterCriticalSection( void *cs );
	bool   CryTryCriticalSection( void *cs );
	void   CryLeaveCriticalSection( void *cs );

#if defined(PS3)
	#include <sys/ppu_thread.h>
	//the Global functions are required to perform leightweight locks between all PPU threads and all SPUs
	//for non PS3 this is obvious useless
	void*  CryCreateCriticalSectionGlobal();
	void   CryDeleteCriticalSectionGlobal( void *cs );
	void   CryEnterCriticalSectionGlobal( void *cs );
	bool   CryTryCriticalSectionGlobal( void *cs );
	void   CryLeaveCriticalSectionGlobal( void *cs );
#else
	#define CryCreateCriticalSectionGlobal CryCreateCriticalSection
	#define CryDeleteCriticalSectionGlobal CryDeleteCriticalSection
	#define CryEnterCriticalSectionGlobal CryEnterCriticalSection
	#define CryTryCriticalSectionGlobal CryTryCriticalSection 
	#define CryLeaveCriticalSectionGlobal CryLeaveCriticalSection
#endif

inline void CrySpinLock(volatile int *pLock,int checkVal,int setVal)
{ 
#ifdef _CPU_X86
# ifdef __GNUC__
	register int val;
	__asm__(
		"0:  mov %[checkVal], %%eax\n"
		"    lock cmpxchg %[setVal], (%[pLock])\n"
		"    jnz 0b"
		: "=m" (*pLock)
		: [pLock] "r" (pLock), "m" (*pLock),
		  [checkVal] "m" (checkVal),
		  [setVal] "r" (setVal)
		: "eax"
		);
# else
	__asm
	{
		mov edx, setVal
		mov ecx, pLock
Spin:
		// Trick from Intel Optimizations guide
#ifdef _CPU_SSE
		pause
#endif 
		mov eax, checkVal
		lock cmpxchg [ecx], edx
		jnz Spin
	}
# endif
#else
# if defined(PS3)
	register int val;
	__asm__(
		"0:  lwarx   %[val], 0, %[pLock]\n"
		"    cmpw    %[checkVal], %[val]\n"
		"    bne-    0b\n"
		"    stwcx.  %[setVal], 0, %[pLock]\n"
		"    bne-    0b\n"
		: [val] "=&r" (val), "=m" (*pLock)
		: [pLock] "r" (pLock), "m" (*pLock),
		  [checkVal] "r" (checkVal),
		  [setVal] "r" (setVal)
		);
# else
	// NOTE: The code below will fail on 64bit architectures!
	while(_InterlockedCompareExchange((volatile long*)pLock,setVal,checkVal)!=checkVal);
# endif
#endif
}

//////////////////////////////////////////////////////////////////////////
inline void CryInterlockedAdd(volatile int *pVal, int iAdd)
{
#ifdef _CPU_X86
# ifdef __GNUC__
	__asm__(
		"lock add %[iAdd], (%[pVal])\n"
		: "=m" (*pVal)
		: [pVal] "r" (pVal), "m" (*pVal), [iAdd] "r" (iAdd)
		);
# else
	__asm
	{
		mov edx, pVal
		mov eax, iAdd
		lock add [edx], eax
	}
# endif
#else
# if defined(PS3)
	register int val;
	__asm__(
		"0:  lwarx   %[val], 0, %[pVal]\n"
		"    add     %[val], %[val], %[iAdd]\n"
		"    stwcx.  %[val], 0, %[pVal]\n"
		"    bne-    0b\n"
		: [val] "=&r" (val), "=m" (*pVal)
		: [pVal] "r" (pVal), "m" (*pVal),
			[iAdd] "r" (iAdd)
		);
# else
	// NOTE: The code below will fail on 64bit architectures!
	_InterlockedExchangeAdd((volatile long*)pVal,iAdd);
# endif
#endif
}

//////////////////////////////////////////////////////////////////////////
struct ReadLock
{
	ReadLock(volatile int &rw)
	{
		CryInterlockedAdd(prw=&rw,1);
		volatile char *pw=(volatile char*)&rw+2; for(;*pw;);
	}
	~ReadLock()
	{
		CryInterlockedAdd(prw,-1);
	}
private:
	volatile int *prw;
};

struct ReadLockCond
{
	ReadLockCond(volatile int &rw,int bActive)
	{
		if (bActive)
		{
			CryInterlockedAdd(&rw,1);
			bActivated = 1;
			volatile char *pw=(volatile char*)&rw+2; for(;*pw;);
		}
		else
		{
			bActivated = 0;
		}
		prw = &rw; 
	}
	void SetActive(int bActive=1) { bActivated = bActive; }
	void Release() { CryInterlockedAdd(prw,-bActivated); }
	~ReadLockCond()
	{
		CryInterlockedAdd(prw,-bActivated);
	}

private:
	volatile int *prw;
	int bActivated;
};

//////////////////////////////////////////////////////////////////////////
struct WriteLock
{
	WriteLock(volatile int &rw) { CrySpinLock(&rw,0,0x10000); prw=&rw; }
	~WriteLock() { CryInterlockedAdd(prw,-0x10000); }
private:
	volatile int *prw;
};

//////////////////////////////////////////////////////////////////////////
struct WriteLockCond
{
	WriteLockCond(volatile int &rw,int bActive=1)
	{
		if (bActive)
			CrySpinLock(&rw,0,iActive=0x10000);
		else 
			iActive = 0;
		prw = &rw; 
	}
	~WriteLockCond() { CryInterlockedAdd(prw,-iActive); }
	void SetActive(int bActive=1) { iActive = -bActive & 0x10000; }
	void Release() { CryInterlockedAdd(prw,-iActive); }
private:
	volatile int *prw;
	int iActive;
};

//////////////////////////////////////////////////////////////////////////
class CCryMutex
{
public:
	CCryMutex() : m_pCritSection(CryCreateCriticalSection())
	{}

	~CCryMutex()
	{
		CryDeleteCriticalSection(m_pCritSection);
	}

	class CLock
	{
	public:
		CLock( CCryMutex& mtx ) : m_pCritSection(mtx.m_pCritSection)
		{
			CryEnterCriticalSection(m_pCritSection);
		}
		~CLock()
		{
			CryLeaveCriticalSection(m_pCritSection);
		}

	private:
		CLock( const CLock& );
		CLock& operator=( const CLock& );

		void *m_pCritSection;
	};

private:
	CCryMutex( const CCryMutex& );
	CCryMutex& operator=( const CCryMutex& );

	void *m_pCritSection;
};

//for PS3 we need additional global locking primitives to lock between all PPU threads and all SPUs
#if defined(PS3)
class CCryMutexGlobal
{
public:
	CCryMutexGlobal() : m_pCritSection(CryCreateCriticalSectionGlobal())
	{}

	~CCryMutexGlobal()
	{
		CryDeleteCriticalSectionGlobal(m_pCritSection);
	}

	class CLock
	{
	public:
		CLock( CCryMutexGlobal& mtx ) : m_pCritSection(mtx.m_pCritSection)
		{
			CryEnterCriticalSectionGlobal(m_pCritSection);
		}
		~CLock()
		{
			CryLeaveCriticalSectionGlobal(m_pCritSection);
		}

	private:
		CLock( const CLock& );
		CLock& operator=( const CLock& );

		void *m_pCritSection;
	};

private:
	CCryMutexGlobal( const CCryMutexGlobal& );
	CCryMutexGlobal& operator=( const CCryMutexGlobal& );

	void *m_pCritSection;
};
#else
	//no impl. needed for non PS3	
	#define CCryMutexGlobal CCryMutex
#endif

//////////////////////////////////////////////////////////////////////////
class CCryThread
{
public:
	CCryThread( void (*func)(void *), void * p );
	~CCryThread();
	static void SetName( const char * name );

private:
	CCryThread( const CCryThread& );
	CCryThread& operator=( const CCryThread& );
#if defined(PS3)
	sys_ppu_thread_t m_handle;
#elif defined(LINUX)
	pthread_t m_handle;
#else
	THREAD_HANDLE m_handle;
#endif
};

#endif //__SPU__
#endif // __MultiThread_h__
