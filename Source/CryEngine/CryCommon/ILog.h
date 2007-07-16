
#ifndef _ILOG_H_
#define _ILOG_H_

#include <platform.h>
#include <string>
#include "IMiniLog.h"

//////////////////////////////////////////////////////////////////////////
// ILogCallback is a callback interface to the ILog.
//////////////////////////////////////////////////////////////////////////
struct ILogCallback
{
	virtual void OnWriteToConsole( const char *sText,bool bNewLine ) = 0;
	virtual void OnWriteToFile( const char *sText,bool bNewLine ) = 0;
};

// wiki documentation: http://server4/twiki/bin/view/CryEngine/CryLog
//////////////////////////////////////////////////////////////////////
struct ILog: public IMiniLog
{
	virtual void Release() = 0;

	//set the file used to log to disk
	virtual void	SetFileName(const char *command = NULL) = 0;

	//
	virtual const char*	GetFileName() = 0;

	//all the following functions will be removed are here just to be able to compile the project ---------------------------

	//will log the text both to file and console
	virtual void	Log(const char *szCommand,...) PRINTF_PARAMS(2, 3) = 0;

	virtual void	LogWarning(const char *szCommand,...) PRINTF_PARAMS(2, 3) = 0;

	virtual void	LogError(const char *szCommand,...) PRINTF_PARAMS(2, 3) = 0;

	//will log the text both to the end of file and console
	virtual void	LogPlus(const char *command,...) PRINTF_PARAMS(2, 3) = 0;	

	//log to the file specified in setfilename
  virtual void	LogToFile(const char *command,...) PRINTF_PARAMS(2, 3) = 0;	

	//
	virtual void	LogToFilePlus(const char *command,...) PRINTF_PARAMS(2, 3) = 0;

	//log to console only
	virtual void	LogToConsole(const char *command,...) PRINTF_PARAMS(2, 3) = 0;

	//
	virtual void	LogToConsolePlus(const char *command,...) PRINTF_PARAMS(2, 3) = 0;

	//
	virtual void	UpdateLoadingScreen(const char *command,...) PRINTF_PARAMS(2, 3) = 0;	

	//
	virtual void RegisterConsoleVariables() {}

	//
	virtual void UnregisterConsoleVariables() {}

	//
	virtual void	SetVerbosity( int verbosity ) = 0;

	virtual int		GetVerbosityLevel()=0;

	virtual void  AddCallback( ILogCallback *pCallback ) = 0;
	virtual void  RemoveCallback( ILogCallback *pCallback ) = 0;
};


#if defined(PS3) || defined(LINUX)
	const bool getFilenameNoCase(const char *, string&, const bool);
#endif

//////////////////////////////////////////////////////////////////////
#ifdef _XBOX
inline void _ConvertNameForXBox(char *dst, const char *src)
{
  //! On XBox d:\ represents current working directory (C:\MasterCD)
  //! only back slash (\) can be used
  strcpy(dst, "d:\\");
  if (src[0]=='.' && (src[1]=='\\' || src[1]=='/'))
    strcat(dst, &src[2]);
  else
    strcat(dst, src);
  int len = strlen(dst);
  for (int n=0; dst[n]; n++)
  {
    if ( dst[n] == '/' )
      dst[n] = '\\';
    if (n > 8 && n+3 < len && dst[n] == '\\' && dst[n+1] == '.' && dst[n+2] == '.')
    {
      int m = n+3;
      n--;
      while (dst[n] != '\\')
      {
        n--;
        if (!n)
          break;
      }
      if (n)
      {
        memmove(&dst[n], &dst[m], len-m+1);
        len -= m-n;
        n--;
      }
    }
  }
}
#elif defined(PS3)
inline void _ConvertNameForPS3(char *dst, const char *src)
{
	//on PS3
	extern const char *fopenwrapper_basedir;
	sprintf(dst, "%s/%s", fopenwrapper_basedir, src);
	string adjustedName;
	getFilenameNoCase(dst, adjustedName, true);
}
#endif

//! Everybody should use fxopen instead of fopen
//! so it will work both on PC and XBox
inline FILE * fxopen(const char *file, const char *mode)
{
  //SetFileAttributes(file,FILE_ATTRIBUTE_ARCHIVE);
//	FILE *pFile = fopen("C:/MasterCD/usedfiles.txt","a");
//	if (pFile)
//	{
//		fprintf(pFile,"%s\n",file);
//		fclose(pFile);
//	}

#ifdef _XBOX
  char name[256];
  _ConvertNameForXBox(name, file);
  return fopen(name, mode);
#else
#if defined(PS3) && !defined(__SPU__)
	//this is pnly important during development
	if(strstr(file, SYS_APP_HOME))
		return fopen(file, mode);
	extern const char *fopenwrapper_basedir;
	char name[strlen(fopenwrapper_basedir) + 1 + strlen(file) + 1];
	sprintf(name, "%s/%s", fopenwrapper_basedir, file);
	string adjustedName;
	bool createFlag = false;
	if (strchr(mode, 'w') || strchr(mode, 'a'))
		createFlag = true;
	getFilenameNoCase(name, adjustedName, createFlag);
	return fopen(adjustedName.c_str(), mode);
#else
#if defined(LINUX)
	string adjustedName;
	bool createFlag = false;
	if (strchr(mode, 'w') || strchr(mode, 'a'))
		createFlag = true;
	getFilenameNoCase(file, adjustedName, createFlag);
	return fopen(adjustedName.c_str(), mode);
#else
  return fopen(file, mode);
#endif //LINUX
#endif //PS3
#endif
}

#endif //_ILOG_H_



