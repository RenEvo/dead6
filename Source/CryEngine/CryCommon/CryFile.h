////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cryfile.h
//  Version:     v1.00
//  Created:     3/7/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: File wrapper.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __cryfile_h__
#define __cryfile_h__
#pragma once

#include <ISystem.h>
#include <ICryPak.h>

//////////////////////////////////////////////////////////////////////////
// Defines for CryEngine filetypes extensions.
//////////////////////////////////////////////////////////////////////////
#define CRY_GEOMETRY_FILE_EXT                    "cgf"
#define CRY_CHARACTER_FILE_EXT                   "chr"
#define CRY_CHARACTER_ANIMATION_FILE_EXT         "caf"
#define CRY_CHARACTER_DEFINITION_FILE_EXT        "cdf"
#define CRY_CHARACTER_LIST_FILE_EXT              "cid"
#define CRY_ANIM_GEOMETRY_FILE_EXT               "cga"
#define CRY_ANIM_GEOMETRY_ANIMATION_FILE_EXT     "anm"
#define CRY_COMPILED_FILE_EXT                    "(c)"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Wrapper on file system.
//////////////////////////////////////////////////////////////////////////
class CCryFile
{
public:
	CCryFile();
	CCryFile( const char *filename, const char *mode );
	virtual ~CCryFile();

	virtual bool Open( const char *filename, const char *mode,int nOpenFlagsEx=0 );
	virtual void Close();

	//! Writes data in a file to the current file position.
	virtual size_t Write( const void *lpBuf,size_t nSize );
	//! Reads data from a file at the current file position.
	virtual size_t ReadRaw( void *lpBuf, size_t nSize );
	//! Template version, for automatic size & endian support.
	template<class T>
		inline size_t ReadType( T *pDest, size_t nCount = 1 )
		{
			size_t nRead = ReadRaw( pDest, sizeof(T)*nCount );
			if (m_bSwap)
				SwapEndian(pDest, nCount);
			return nRead;
		}

	//! Turns endian swapping on or off (default on).
	void SetSwap(bool bSwap)
	{
		m_bSwap = bSwap;
	}

	//! Retrieves the length of the file.
	virtual size_t GetLength();

	//! Positions the current file pointer.
	virtual size_t Seek( size_t seek, int mode );
	//! Positions the current file pointer at the beginning of the file.
	void SeekToBegin();
	//! Positions the current file pointer at the end of the file.
	size_t SeekToEnd();
	//! Retrieves the current file pointer.
	size_t GetPosition();

	//! Tests for end-of-file on a selected file.
	virtual bool IsEof();

	//! Flushes any data yet to be written.
	virtual void Flush();

	//! A handle to a pack object.
	FILE* GetHandle() const { return m_file; };

	// Description:
	//    Retrieves the filename of the selected file.
	const char* GetFilename() const { return m_filename.c_str(); };

	// Description:
	//    Retrieves the filename after adjustment to the real relative to engine root path.
	//    Ex. original filename "textures/red.dds" adjusted filename will look like "game/textures/red.dds"
	// Return:
	//    Adjusted filename, this is a pointer to a static string, copy return value if you want to keep it.
	const char* GetAdjustedFilename() const;

	//! Check if file is opened from pak file.
	bool IsInPak() const;

	//! Get path of archive this file is in.
	const char* GetPakPath() const;

private:
	string m_filename;
	FILE *m_file;
	ICryPak *m_pIPak;
	bool m_bSwap;
};

//////////////////////////////////////////////////////////////////////////
// CCryFile implementation.
//////////////////////////////////////////////////////////////////////////
inline CCryFile::CCryFile()
{
	m_file = 0;
	m_pIPak = gEnv->pCryPak;
	m_bSwap = true;
}

//////////////////////////////////////////////////////////////////////////
inline CCryFile::CCryFile( const char *filename, const char *mode )
{
	m_file = 0;
	m_pIPak = gEnv->pCryPak;
	Open( filename,mode );
	m_bSwap = true;
}

//////////////////////////////////////////////////////////////////////////
inline CCryFile::~CCryFile()
{
	Close();
}

//////////////////////////////////////////////////////////////////////////
// for nOpenFlagsEx see ICryPak::EFOpenFlags
//////////////////////////////////////////////////////////////////////////
inline bool CCryFile::Open( const char *filename, const char *mode,int nOpenFlagsEx )
{
	if (m_file)
		Close();
	m_filename = filename;
	m_file = m_pIPak->FOpen( filename,mode,nOpenFlagsEx );
	return m_file != NULL;
}

//////////////////////////////////////////////////////////////////////////
inline void CCryFile::Close()
{
	if (m_file)
	{
		m_pIPak->FClose(m_file);
		m_file = 0;
		m_filename = "";
	}
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::Write( const void *lpBuf,size_t nSize )
{
	assert( m_file );
	return m_pIPak->FWrite( lpBuf,1,nSize,m_file );
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::ReadRaw( void *lpBuf,size_t nSize )
{
	assert( m_file );
	return m_pIPak->FReadRaw( lpBuf,1,nSize,m_file );
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::GetLength()
{
	assert( m_file );
	long curr = m_pIPak->FTell(m_file);
	m_pIPak->FSeek( m_file,0,SEEK_END );
	long size = m_pIPak->FTell(m_file);
	m_pIPak->FSeek(m_file,curr,SEEK_SET);
	return size;
}

#ifdef WIN64
#pragma warning( push )									//AMD Port
#pragma warning( disable : 4267 )
#endif

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::Seek( size_t seek, int mode )
{
	assert( m_file );
	return m_pIPak->FSeek( m_file,seek,mode );
}

#ifdef WIN64
#pragma warning( pop )									//AMD Port
#endif

//////////////////////////////////////////////////////////////////////////
inline void CCryFile::SeekToBegin()
{
	Seek( 0,SEEK_SET );
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::SeekToEnd()
{
	return Seek( 0,SEEK_END );
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::GetPosition()
{
	assert(m_file);
	return m_pIPak->FTell(m_file);
}

//////////////////////////////////////////////////////////////////////////
inline bool CCryFile::IsEof()
{
	assert(m_file);
	return m_pIPak->FEof(m_file) != 0;
}

//////////////////////////////////////////////////////////////////////////
inline void CCryFile::Flush()
{
	assert( m_file );
	m_pIPak->FFlush( m_file );
}

//////////////////////////////////////////////////////////////////////////
inline bool CCryFile::IsInPak() const
{
	if (m_file)
	{
		if (m_pIPak->GetFileArchivePath(m_file) != NULL)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
inline const char* CCryFile::GetPakPath() const
{
	if (m_file)
	{
		const char *sPath = m_pIPak->GetFileArchivePath(m_file);
		if (sPath != NULL)
			return sPath;
	}
	return "";
}

//////////////////////////////////////////////////////////////////////////
inline const char* CCryFile::GetAdjustedFilename() const
{
	static char szAdjustedFile[ICryPak::g_nMaxPath];
	return m_pIPak->AdjustFileName( m_filename.c_str(),szAdjustedFile,0 );
}

#endif // __cryfile_h__
