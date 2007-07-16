////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   PS3CryCache.h
//  Version:     v1.00
//  Created:     02/02/2007 by Michael Glueck.
//  Compilers:   Visual Studio.NET
//  Description: Software Cache spiecific definitions
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_CACHE_H_
#define _CRY_CACHE_H_
#pragma once

#if defined(__SPU__)

#if defined __CRYCG__
	#define __CRYCG_NOINLINE__ __attribute__ ((crycg_attr ("noinline")))
	#if defined __cplusplus
		#if !defined SPU_MAIN_PTR
			template <typename T>
			__CRYCG_NOINLINE__ T *__spu_main_ptr(T *ptr) { return ptr; } 
			#define SPU_MAIN_PTR(PTR) __spu_main_ptr((PTR)) 
		#endif 
		#if !defined SPU_MAIN_REF template <typename T> __CRYCG_NOINLINE__ T &__spu_main_ref(T &ref) { return ref; } 
			#define SPU_MAIN_REF(REF) __spu_main_ref((REF)) 
		#endif 
		#if !defined SPU_LOCAL_PTR template <typename T> __CRYCG_NOINLINE__ T *__spu_local_ptr(T *ptr) { return ptr; } 
			#define SPU_LOCAL_PTR(PTR) __spu_local_ptr((PTR)) 
		#endif 
		#if !defined SPU_LOCAL_REF template <typename T> __CRYCG_NOINLINE__ T &__spu_local_ref(T &ref) { return ref; } 
			#define SPU_LOCAL_REF(REF) __spu_local_ref((REF)) 
		#endif 
		#if !defined SPU_LINK_PTR template <typename T, typename L> __CRYCG_NOINLINE__ T *__spu_link_ptr(T *ptr, L *) { return ptr; } template <typename T, typename L> __CRYCG_NOINLINE__ T *__spu_link_ptr(T *ptr, L &) { return ptr; } 
			#define SPU_LINK_PTR(PTR, LINK) __spu_link_ptr((PTR), (LINK)) 
		#endif 
		#if !defined SPU_LINK_REF template <typename T, typename L> __CRYCG_NOINLINE__ T &__spu_link_ref(T &ref, L *) { return ref; } template <typename T, typename L> __CRYCG_NOINLINE__ T &__spu_link_ref(T &ref, L &) { return ref; } 
			#define SPU_LINK_REF(REF, LINK) __spu_link_ref((PTR), (LINK)) 
		#endif 
	#endif /* __cplusplus */
	#if !defined SPU_DOMAIN_MAIN
		#define SPU_DOMAIN_MAIN __attribute__ ((crycg_domain ("MAIN"))) 
	#endif 
	#if !defined SPU_DOMAIN_LOCAL 
		#define SPU_DOMAIN_LOCAL __attribute__ ((crycg_domain ("LOCAL"))) 
	#endif 
	#if !defined SPU_DOMAIN_LINK 
		#define SPU_DOMAIN_LINK(ID) __attribute__ ((crycg_domain (ID))) 
	#endif
#else /* __CRYCG__ */
	typedef uint32_t uint32;
	extern void SPUAddCacheWriteRangeAsync(uint32_t, uint32_t);
	#define __cache_range_write_async(cpFrom, cpTo) SPUAddCacheWriteRangeAsync((cpFrom), (cpTo))
	extern int CryInterlockedAdd(int volatile *pDst, const int cVal);
	extern int CryInterlockedIncrement(int volatile *pDst);
	extern int CryInterlockedDecrement(int volatile *pDst);
	extern void CrySpinLock(volatile int *pLock, int checkVal, int setVal);
	extern void ReleaseSpinLock(volatile int *pLock, int setVal);
	extern void* CryCreateCriticalSection();
	extern void CryDeleteCriticalSection( void *cs );
	extern void CryEnterCriticalSection( void *cs );
	extern bool CryTryCriticalSection( void *cs );
	extern void CryLeaveCriticalSection( void *cs );
	extern void* CryCreateCriticalSectionGlobal();
	extern void CryDeleteCriticalSectionGlobal( void *cs );
	extern void CryEnterCriticalSectionGlobal( void *cs );
	extern bool CryTryCriticalSectionGlobal( void *cs );
	extern void CryLeaveCriticalSectionGlobal( void *cs );


	#define __CRYCG_NOINLINE__
	#if defined __cplusplus
		#if !defined SPU_MAIN_PTR
			#define SPU_MAIN_PTR(PTR) (PTR)
		#endif
		#if !defined SPU_MAIN_REF
			#define SPU_MAIN_REF(REF) (REF)
		#endif
		#if !defined SPU_LOCAL_PTR
			#define SPU_LOCAL_PTR(PTR) (PTR)
		#endif
		#if !defined SPU_LOCAL_REF
			#define SPU_LOCAL_REF(REF) (REF)
		#endif
		#if !defined SPU_LINK_PTR
			#define SPU_LINK_PTR(PTR, LINK) (PTR)
		#endif
		#if !defined SPU_LINK_REF
			#define SPU_LINK_REF(REF, LINK) (PTR)
		#endif
	#endif /* __cplusplus */
	#if !defined SPU_DOMAIN_MAIN
		#define SPU_DOMAIN_MAIN
	#endif
	#if !defined SPU_DOMAIN_LOCAL
		#define SPU_DOMAIN_LOCAL
	#endif
	#if !defined SPU_DOMAIN_LINK
		#define SPU_DOMAIN_LINK(ID)
	#endif
#endif /* __CRYCG__ */

//prefix of code generator referring to bubble names (later replaced by IDs)
#define BUBBLE_PREFIX "__spu_bub_id_"
#define DECL_BUB(a) extern int __spu_bub_id_##a;
#define BUB_ID(a) __spu_bub_id_##a

#define BUB_CROSS_CALL_PREFIX "__spu_bub_fnct_id_"
#define DECL_BUB_CROSS_CALL(a) extern vec_ushort8 __spu_bub_fnct_id_##a;
#define CROSS_CALL_ID(a) __spu_bub_fnct_id_##a

//separator for ::
#define GLOB_VAR_SEP "__S"
#define GLOB_VAR_PREFIX "__spu_global_var_"
#define GLOB_VAR_BASE_SYMBOL "NPPU::CJobManSPU::scInitalSPULoader"
#if defined(__SPU__)
	//the name space "::" must be replaced by __
	#define DECL_GLOB_VAR(a) extern int __spu_global_var_##a;
	#define RESOLVE_GLOB_VAR_ADDR(a) __cache_resolve_global_var_addr(__spu_global_var_##a)
#else
	#define DECL_GLOB_VAR(a)
	#define RESOLVE_GLOB_VAR_ADDR(a)
#endif

#endif //__SPU__
#endif //_CRY_CACHE_H_
