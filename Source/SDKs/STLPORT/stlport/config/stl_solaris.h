
/* include system features file */
# include <sys/feature_tests.h>

/* system-dependent defines */

# if defined (__SunOS_5_8) && ! defined (_STLP_HAS_NO_NEW_C_HEADERS) && ( __cplusplus >= 199711L)
#  define _STLP_HAS_NATIVE_FLOAT_ABS
# endif

#if defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 >= 4)
# define _STLP_RAND48 1
#endif

#if (defined(_XOPEN_SOURCE) && (_XOPEN_VERSION - 0 == 4)) || defined (__SunOS_5_6)
# define _STLP_WCHAR_SUNPRO_EXCLUDE 1
# define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#endif

/* boris : this should always be defined for Solaris 5 & 6. Any ideas how to do it? */
# if !(defined ( __KCC ) && __KCC_VERSION > 3400 ) && \
  ((defined(__SunOS_5_5_1) || defined(__SunOS_5_6) ))
#  ifndef _STLP_NO_NATIVE_MBSTATE_T
#   define _STLP_NO_NATIVE_MBSTATE_T 1
#  endif
# endif /* KCC */

/* For SPARC we use lightweight synchronization */
# if defined (__sparc) /* && (defined (_REENTRANT) || defined (_PTHREADS)) */
#  if ( (defined (__GNUC__) && defined (__sparc_v9__)) || \
        defined (__sparcv9) ) \
       && !defined(_NOTHREADS) && !defined (_STLP_NO_SPARC_SOLARIS_THREADS)
#    define _STLP_SPARC_SOLARIS_THREADS
#    define _STLP_THREADS_DEFINED
#  endif
# endif

/*
 * Hmm, I don't found in Solaris 9 system headers definition like __SunOS_5_9
 * (defined in SunPro?); I also can't find functions like fmodf (again,
 * I found modff in libc, but no acosf etc.). Strange, I saw __cosf functions
 * (built-in?) at least with gcc some time ago, but don't see ones with
 * gcc 3.3.2 on SunOS sparc-solaris1 5.9 Generic_112233-03 sun4u sparc SUNW,Ultra-60
 * from Sorceforge's CF.
 *    2005-12-15, - ptr
 *
 * P.S. That's why I add two defines:
 */

# ifdef __GNUC__
#  define _STLP_NO_VENDOR_MATH_F
#  define _STLP_NO_VENDOR_MATH_L
# endif
