/////////////////////////////////////////////////////////////////////////////
//
// Crytek Source File
// Copyright (C), Crytek Studios, 2001-2006.
//
// Description: Public include file for the multi-threading API.
//
// History:
// Jun 20, 2006: Created by Sascha Demetrio
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CryThread_h_
#define _CryThread_h_ 1

#define _GNU_SOURCE 1

// Lock types:
//
// CRYLOCK_NONE
//   An empty dummy lock where the lock/unlock operations do nothing.
//
// CRYLOCK_FAST
//   A fast (non-recursive) mutex.
//
// CRYLOCK_RECURSIVE
//   A recursive mutex.
enum CryLockType
{
	CRYLOCK_NONE = 0,
#if !defined(PS3)
	CRYLOCK_FAST = 1,
#endif
	CRYLOCK_RECURSIVE = 2,
};

#if defined(PS3)
	#define CRYLOCK_FAST CRYLOCK_RECURSIVE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void CryThreadSetName( unsigned int nThreadId,const char *sThreadName );
const char* CryThreadGetName( unsigned int nThreadId );

/////////////////////////////////////////////////////////////////////////////
//
// Primitive locks and conditions.
//
// Primitive locks are represented by instances of class CryLock<Type> and
// CryRWLock<Type>.
//
// Conditions are represented by instances of class
// CryCond<LockClass, impliedLock> where LockClass is the class of lock to be
// associated with the condition and impliedLock is a flag indicating if an
// implied lock should be associated with the condition.
//
// CryCondLock<> acts like CryLock, but is always appropriate for CryCond

template<CryLockType Type> class CryLock
{
	/* Unsupported lock type. */
};

template<CryLockType Type> class CryCondLock
{
	/* Unsupported lock type. */
};

template<CryLockType Type> class CryRWLock
{
	/* Unsupported lock type. */
};

template<class LockClass, bool impliedLock = true> class CryCond
{
	/* Unsupported lock class. */
};

template<> class CryLock<CRYLOCK_NONE>
{
	CryLock(const CryLock<CRYLOCK_NONE>&);
	void operator = (const CryLock<CRYLOCK_NONE>&);

public:
	CryLock() { }
  void Lock() { }
	bool TryLock() { return true; }
  void Unlock() { }
#ifndef NDEBUG
	bool IsLocked() { return true; }
#endif
};

template<> class CryRWLock<CRYLOCK_NONE>
{
	CryRWLock(const CryRWLock<CRYLOCK_NONE>&);
	void operator = (const CryRWLock<CRYLOCK_NONE>&);

public:
	CryRWLock() { }
	void RLock() { }
	bool TryRLock() { return true; }
	void WLock() { }
	bool TryWLock() { return true; }
	void Lock() { }
	bool TryLock() { return true; }
	void Unlock() { }
};

typedef CryLock<CRYLOCK_NONE> CryNoLock;
#if defined(PS3)
	typedef CryLock<CRYLOCK_RECURSIVE> CryFastLock;
#else
	typedef CryLock<CRYLOCK_FAST> CryFastLock;
#endif
typedef CryLock<CRYLOCK_RECURSIVE> CryRecursiveLock;

template<> class CryCond<CryNoLock, false>
{
	CryCond(const CryCond<CryNoLock, false>&);
	void operator = (const CryCond<CryNoLock, false>&);

public:
  CryCond() { }
  CryCond(CryNoLock &Lock) { }

  void Notify() { }
  void NotifySingle() { }
  void Wait() { }
	bool TimedWait(uint32 milliseconds) { return true; }
};

template<> class CryCond<CryNoLock, true> : public CryCond<CryNoLock, false>
{
  CryNoLock m_Lock;

	CryCond(const CryCond<CryNoLock, true>&);
	void operator = (const CryCond<CryNoLock, true>&);

public:
	CryCond() { }
  void Lock() { }
	bool TryLock() { return true; }
  void Unlock() { }
  CryNoLock &GetLock() { return m_Lock; }
};

template<class LockClass> class CryAutoLock
{
private:
	LockClass &m_Lock;

	CryAutoLock();
	CryAutoLock(const CryAutoLock<LockClass>&);
	void operator = (const CryAutoLock<LockClass>&);

public:
	CryAutoLock(LockClass &Lock) : m_Lock(Lock) { m_Lock.Lock(); }
	~CryAutoLock() { m_Lock.Unlock(); }
};

/////////////////////////////////////////////////////////////////////////////
//
// Threads.

// Base class for runnable objects.
//
// A runnable is an object with a Run() and a Cancel() method.  The Run()
// method should perform the runnable's job.  The Cancel() method may be
// called by another thread requesting early termination of the Run() method.
// The runnable may ignore the Cancel() call, the default implementation of
// Cancel() does nothing.
class CryRunnable
{
public:
	virtual ~CryRunnable() { }
	virtual void Run() = 0;
	virtual void Cancel() { }
};

// Class holding information about a thread.
//
// A reference to the thread information can be obtained by calling GetInfo()
// on the CrySimpleThread (or derived class) instance.
//
// NOTE:
// If the code is compiled with NO_THREADINFO defined, then the GetInfo()
// method will return a reference to a static dummy instance of this
// structure.  It is currently undecided if NO_THREADINFO will be defined for
// release builds!
struct CryThreadInfo
{
	// The symbolic name of the thread.
	//
	// You may set this name directly or through the SetName() method of
	// CrySimpleThread (or derived class).
	string m_Name;

	// A thread identification number.
	// The number is unique but architecture specific.  Do not assume anything
	// about that number except for being unique.
	//
	// This field is filled when the thread is started (i.e. before the Run()
	// method or thread routine is called).  It is advised that you do not
	// change this number manually.
	uint32 m_ID;
};

// Simple thread class.
//
// CrySimpleThread is a simple wrapper around a system thread providing
// nothing but system-level functionality of a thread.  There are two typical
// ways to use a simple thread:
//
// 1. Derive from the CrySimpleThread class and provide an implementation of
//    the Run() (and optionally Cancel()) methods.
// 2. Specify a runnable object when the thread is started.  The default
//    runnable type is CryRunnable.
//
// The Runnable class specfied as the template argument must provide Run()
// and Cancel() methods compatible with the following signatures:
//
//   void Runnable::Run();
//   void Runnable::Cancel();
//
// If the Runnable does not support cancellation, then the Cancel() method
// should do nothing.
//
// The same instance of CrySimpleThread may be used for multiple thread
// executions /in sequence/, i.e. it is valid to re-start the thread by
// calling Start() after the thread has been joined by calling Join().
template<class Runnable = CryRunnable> class CrySimpleThread;

// Standard thread class.
//
// The class provides a lock (mutex) and an associated condition variable.  If
// you don't need the lock, then you should used CrySimpleThread instead of
// CryThread.
template<class Runnable = CryRunnable> class CryThread;

// Include architecture specific code.
#if defined(LINUX) || defined(PS3)
#include <CryThread_pthreads.h>
#elif defined(WIN32) || defined(WIN64)
#include <CryThread_windows.h>
#elif defined(XENON)
#include <CryThread_windows.h>
#else
// Put other platform specific includes here!
#endif

// Thread class.
//
// CryThread is an extension of CrySimpleThread providing a lock (mutex) and a
// condition variable per instance.
template<class Runnable> class CryThread
	: public CrySimpleThread<Runnable>
{
	CryFastLock m_Lock;
	CryCond<CryFastLock> m_Cond;

	CryThread(const CryThread<Runnable>&);
	void operator = (const CryThread<Runnable>&);

public:
	CryThread() { }
	void Lock() { m_Lock.Lock(); }
	bool TryLock() { return m_Lock.TryLock(); }
	void Unlock() { m_Lock.Unlock(); }
	void Wait() { m_Cond.Wait(m_Lock); }
	// Timed wait on the associated condition.
	//
	// The 'milliseconds' parameter specifies the relative timeout in
	// milliseconds.  The method returns true if a notification was received and
	// false if the specified timeout expired without receiving a notification.
	//
	// UNIX note: the method will _not_ return if the calling thread receives a
	// signal.  Instead the call is re-started with the _original_ timeout
	// value.  This misfeature may be fixed in the future.
	bool TimedWait(uint32 milliseconds)
	{
		return m_Cond.TimedWait(m_Lock, milliseconds);
	}
	void Notify() { m_Cond.Notify(); }
	void NotifySingle() { m_Cond.NotifySingle(); }
	CryFastLock &GetLock() { return m_Lock; }
};

#endif

// vim:ts=2

