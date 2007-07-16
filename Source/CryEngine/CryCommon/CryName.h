////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CryName.h
//  Version:     v1.00
//  Created:     6/10/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CryName_h__
#define __CryName_h__
#pragma once

#include <ISystem.h>
#include <StlUtils.h>

class CNameTable;

struct INameTable
{
	// Name entry header, immediately after this header in memory starts actual string data.
	struct SNameEntry
	{
		// Reference count of this string.
		int nRefCount;
		// Current length of string.
		int nLength;
		// Size of memory allocated at the end of this class.
		int nAllocSize;
		// Here in memory starts character buffer of size nAllocSize.
		//char data[nAllocSize]

		char* GetStr() { return (char*)(this+1); }
		void  AddRef() { nRefCount++; /*InterlockedIncrement(&_header()->nRefCount);*/};
		int   Release() { return --nRefCount; };
	};

	// Finds an existing name table entry, or creates a new one if not found.
  virtual SNameEntry* GetEntry( const char *str ) = 0;
	// Only finds an existing name table entry, return 0 if not found.
	virtual SNameEntry* FindEntry( const char *str ) = 0;
	// Release existing name table entry.
	virtual void Release( SNameEntry *pEntry ) = 0;
};

#define CRY_NAME_HASHTABLE_SIZE 1024*8

//////////////////////////////////////////////////////////////////////////
class CNameTable : public INameTable
{
public:
	CNameTable()
#ifdef USE_HASH_MAP
		: m_nameMap(CRY_NAME_HASHTABLE_SIZE)
#endif //USE_HASH_MAP
	{}

	// Only finds an existing name table entry, return 0 if not found.
	virtual SNameEntry* FindEntry( const char *str )
	{
		SNameEntry *pEntry = stl::find_in_map( m_nameMap,str,0 );
		return pEntry;
	}

	// Finds an existing name table entry, or creates a new one if not found.
	virtual SNameEntry* GetEntry( const char *str )
	{
		SNameEntry *pEntry = stl::find_in_map( m_nameMap,str,0 );
		if (!pEntry)
		{
			// Create a new entry.
			unsigned int nLen = strlen(str);
			unsigned int allocLen = sizeof(SNameEntry) + (nLen+1)*sizeof(char);
			pEntry = (SNameEntry*)malloc( allocLen );
			pEntry->nRefCount = 0;
			pEntry->nLength = nLen;
			pEntry->nAllocSize = allocLen;
			// Copy string to the end of name entry.
			memcpy( pEntry->GetStr(),str,nLen+1 );

			// put in map.
			//m_nameMap.insert( NameMap::value_type(pEntry->GetStr(),pEntry) );
			m_nameMap[pEntry->GetStr()] = pEntry;
		}
		return pEntry;
	}

	// Release existing name table entry.
	virtual void Release( SNameEntry *pEntry )
	{
		assert(pEntry);
		m_nameMap.erase( pEntry->GetStr() );
		free(pEntry);
	}

private:
#ifdef USE_HASH_MAP
	typedef stl::hash_map<const char*,SNameEntry*,stl::hash_stricmp<const char*> > NameMap;
#else
	typedef std::map<const char*,SNameEntry*,stl::less_stricmp<const char*> > NameMap;
#endif

	NameMap m_nameMap;
};

///////////////////////////////////////////////////////////////////////////////
// Class CCryName.
//////////////////////////////////////////////////////////////////////////
class	CCryName
{
public:
	CCryName();
	CCryName( const CCryName& n );
	CCryName( const char *s );
	CCryName( const char *s,bool bOnlyFind );
	~CCryName();

	CCryName& operator=( const CCryName& n );
	CCryName& operator=( const char *s );

	bool	operator==( const CCryName &n ) const;
	bool	operator!=( const CCryName &n ) const;

	bool	operator==( const char *s ) const;
	bool	operator!=( const char *s ) const;

	bool	operator<( const CCryName &n ) const;
	bool	operator>( const CCryName &n ) const;

	bool	empty() const { return length() == 0; }
	void	reset()	{	_release(m_str);	m_str = 0; }

	const	char*	c_str() const { return (m_str) ? m_str: ""; }
	int	length() const { return _length(); };

	static bool find( const char *str ) { return GetNameTable()->FindEntry(str) != 0; }
  static const char *create( const char *str )
  {
    CCryName name = CCryName(str);
    name._addref(name.c_str());
    return name.c_str();
  }

private:
	typedef INameTable::SNameEntry SNameEntry;

#ifdef USE_STATIC_NAME_TABLE
	static CNameTable* GetNameTable()
	{
		static CNameTable table;
		return &table;
	}
#else
	//static INameTable* GetNameTable() { return GetISystem()->GetINameTable(); }
	static INameTable* GetNameTable() { return gEnv->pNameTable; }
#endif
	SNameEntry* _entry( const char *pBuffer ) const { assert(pBuffer); return ((SNameEntry*)pBuffer)-1; }
	int  _length() const { return (m_str) ? _entry(m_str)->nLength : 0; };
	void _addref( const char *pBuffer ) { if (pBuffer) _entry(pBuffer)->AddRef(); }
	void _release( const char *pBuffer ) {
		if (pBuffer && _entry(pBuffer)->Release() <= 0)
			GetNameTable()->Release(_entry(pBuffer));
	}

	const char *m_str;
};

//////////////////////////////////////////////////////////////////////////
inline CCryName::CCryName()
{
	m_str = 0;
}

//////////////////////////////////////////////////////////////////////////
inline CCryName::CCryName( const CCryName& n )
{
	_addref( n.m_str );
	m_str = n.m_str;
}

//////////////////////////////////////////////////////////////////////////
inline CCryName::CCryName( const char *s )
{
	m_str = 0;
	*this = s;
}

//////////////////////////////////////////////////////////////////////////
inline CCryName::CCryName( const char *s,bool bOnlyFind )
{
	assert(s);
	m_str = 0;
	const char *pBuf = 0;
	if (*s) // if not empty
	{
		SNameEntry *pNameEntry = GetNameTable()->FindEntry(s);
		if (pNameEntry)
		{
			m_str = pNameEntry->GetStr();
			_addref(m_str);
		}
	}
}

inline CCryName::~CCryName()
{
	_release(m_str);
}

//////////////////////////////////////////////////////////////////////////
inline CCryName&	CCryName::operator=( const CCryName &n )
{
	if (m_str != n.m_str)
	{
		_release(m_str);
		m_str = n.m_str;
		_addref(m_str);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
inline CCryName&	CCryName::operator=( const char *s )
{
	assert(s);
	const char *pBuf = 0;
	if (*s) // if not empty
	{
		pBuf = GetNameTable()->GetEntry(s)->GetStr();
	}
	if (m_str != pBuf)
	{
		_release(m_str);
		m_str = pBuf;
		_addref(m_str);
	}
	return *this;
}


//////////////////////////////////////////////////////////////////////////
inline bool	CCryName::operator==( const CCryName &n ) const {
	return m_str == n.m_str;
}

inline bool	CCryName::operator!=( const CCryName &n ) const {
	return !(*this == n);
}

inline bool	CCryName::operator==( const char* str ) const {
	return m_str && stricmp(m_str,str) == 0;
}

inline bool	CCryName::operator!=( const char* str ) const {
	if (!m_str)
		return true;
	return stricmp(m_str,str) != 0;
}

inline bool	CCryName::operator<( const CCryName &n ) const {
	return m_str < n.m_str;
}

inline bool	CCryName::operator>( const CCryName &n ) const {
	return m_str > n.m_str;
}

inline bool	operator==( const string &s,const CCryName &n ) {
	return n == s;
}
inline bool	operator!=( const string &s,const CCryName &n ) {
	return n != s;
}

inline bool	operator==( const char *s,const CCryName &n ) {
	return n == s;
}
inline bool	operator!=( const char *s,const CCryName &n ) {
	return n != s;
}

#endif //__CryName_h__
