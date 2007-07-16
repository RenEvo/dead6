////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ProjectDefines.h
//  Version:     v1.00
//  Created:     3/30/2004 by MartinM.
//  Compilers:   Visual Studio.NET
//  Description: to get some defines available in every CryEngine project 
// -------------------------------------------------------------------------
//  History:
//    July 20th 2004 - Mathieu Pinard
//    Updated the structure to handle more easily different configurations
//
////////////////////////////////////////////////////////////////////////////

#ifndef PROJECTDEFINES_H
#define PROJECTDEFINES_H

// Needed so we use our own terminate function in platform_impl.h
#define _STLP_DEBUG_TERMINATE

//VS2005
#if _MSC_VER >= 1400
//#pragma warning( default  : 4996 )  // warning C4996: 'stricmp' was declared deprecated
#endif


// Turns on/off stlport debug mode - needs to be done everywhere (i.e. can't just
// do it per module cos this makes stl behave oddly) and everything needs to be build in debug.
// you also need to add the following to the top of stlport/stl/debug/_debug.c
/*
#include <windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
*/
//#define _STLP_DEBUG

//#		define EXCLUDE_SCALEFORM_SDK
//#		define EXCLUDE_CRI_SDK
//#		define EXCLUDE_GPU_PARTICLE_PHYSICS


#define _DATAPROBE

#endif // PROJECTDEFINES_H
