/*========================================== */
#if 1 /* def _STLP_3_COMPATIBILITY */

# define __SGI_STL_PORT  _STLPORT_VERSION

# if defined (_STLP_DEBUG) && ! defined ( __STL_DEBUG )
#  define __STL_DEBUG _STLP_DEBUG
# endif

# if defined (_STLP_USE_NAMESPACES)
#  undef  __STL_USE_NAMESPACES 
#  define __STL_USE_NAMESPACES _STLP_USE_NAMESPACES
# endif

# if defined (_STLP_USE_EXCEPTIONS)
#  undef  __STL_USE_EXCEPTIONS 
#  define __STL_USE_EXCEPTIONS _STLP_USE_EXCEPTIONS
# endif

# if defined (_STLP_BEGIN_NAMESPACE) && ! defined ( __STL_BEGIN_NAMESPACE )
#  define __STL_BEGIN_NAMESPACE _STLP_BEGIN_NAMESPACE
#  define __STL_END_NAMESPACE _STLP_END_NAMESPACE
#  define __STL_VENDOR_STD _STLP_VENDOR_STD
#  define __STL_VENDOR_CSTD _STLP_VENDOR_CSTD
# endif
# endif

/*
# if defined (_STLP_XXX) && ! defined ( __STL_XXX )
#  define __STL_XXX _STLP_XXX
# endif
*/

/* 5.0 -> 4.6 compatibility section */
#if 1 /* def _STLP_46_COMPATIBILITY */

#ifndef  _STLP_USING_NAMESPACE_BUG
# define _STLP_USING_NAMESPACE_BUG
#endif

/* provide a uniform way to access full funclionality */
# define __slist__  __FULL_NAME(slist)
# define __map__  __FULL_NAME(map)
# define __multimap__  __FULL_NAME(multimap)
# define __set__  __FULL_NAME(set)
# define __multiset__  __FULL_NAME(multiset)
# define __list__ __FULL_NAME(list)
# define __hash_map__ __FULL_NAME(hash_map)
# define __hash_multimap__ __FULL_NAME(hash_multimap)
# define __hash_set__       __FULL_NAME(hash_set)
# define __hash_multiset__  __FULL_NAME(hash_multiset)
# define __vector__ __FULL_NAME(vector)

#endif

