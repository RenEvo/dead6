/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999 
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */ 
// WARNING: This is an internal header file, included by other C++
// standard library headers.  You should not attempt to use this header
// file directly.


#ifndef _STLP_INTERNAL_NUM_PUT_H
#define _STLP_INTERNAL_NUM_PUT_H

#ifndef _STLP_INTERNAL_NUMPUNCT_H
# include <stl/_numpunct.h>
#endif

#ifndef _STLP_INTERNAL_CTYPE_H
# include <stl/_ctype.h>
#endif

#ifndef _STLP_INTERNAL_OSTREAMBUF_ITERATOR_H
# include <stl/_ostreambuf_iterator.h>
#endif

#ifndef _STLP_INTERNAL_IOSTREAM_STRING_H
# include <stl/_iostream_string.h>
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// num_put facet

# ifdef _STLP_LIMITED_DEFAULT_TEMPLATES
template <class _CharT, class _OutputIter>  
# else
template <class _CharT, class _OutputIter = ostreambuf_iterator<_CharT, char_traits<_CharT> > >  
# endif
class num_put: public locale::facet
{
  friend class _Locale_impl;
public:
  typedef _CharT      char_type;
  typedef _OutputIter iter_type;

  explicit num_put(size_t __refs = 0) : locale::facet(__refs) {}

# ifndef _STLP_NO_BOOL
  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                bool __val) const {
    return do_put(__s, __f, __fill, __val);
  }
# endif
  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
               long __val) const {
    return do_put(__s, __f, __fill, __val);
  }

  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                unsigned long __val) const {
    return do_put(__s, __f, __fill, __val);
  }

# ifdef _STLP_LONG_LONG
  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                _STLP_LONG_LONG __val) const {
    return do_put(__s, __f, __fill, __val);
  }

  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                unsigned _STLP_LONG_LONG __val) const {
    return do_put(__s, __f, __fill, __val);
  }
# endif

  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                double __val) const {
    return do_put(__s, __f, __fill, (double)__val);
  }

# ifndef _STLP_NO_LONG_DOUBLE
  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                long double __val) const {
    return do_put(__s, __f, __fill, __val);
  }
# endif

  iter_type put(iter_type __s, ios_base& __f, char_type __fill,
                const void * __val) const {
    return do_put(__s, __f, __fill, __val);
  }

  static _STLP_STATIC_MEMBER_DECLSPEC locale::id id;

protected:
  ~num_put() {}   
# ifndef _STLP_NO_BOOL
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, bool __val) const;
# endif
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, long __val) const;
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, unsigned long __val) const;
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, double __val) const;
# ifndef _STLP_NO_LONG_DOUBLE
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, long double __val) const;
# endif

# ifdef _STLP_LONG_LONG
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, _STLP_LONG_LONG __val) const;
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, 
                           unsigned _STLP_LONG_LONG __val) const ;
# endif /* _STLP_LONG_LONG  */
  virtual _OutputIter do_put(_OutputIter __s, ios_base& __f, _CharT __fill, const void* __val) const;
};

#ifdef _STLP_USE_TEMPLATE_EXPORT
_STLP_EXPORT_TEMPLATE_CLASS num_put<char, ostreambuf_iterator<char, char_traits<char> > >;
// _STLP_EXPORT_TEMPLATE_CLASS num_put<char, char*>;
#  ifndef _STLP_NO_WCHAR_T
_STLP_EXPORT_TEMPLATE_CLASS num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
// _STLP_EXPORT_TEMPLATE_CLASS num_put<wchar_t, wchar_t*>;
#  endif /* _STLP_NO_WCHAR_T */
#endif

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION)

template <class _Integer>
char* _STLP_CALL
__write_integer_backward(char* __buf, ios_base::fmtflags __flags, _Integer __x);

void  _STLP_CALL __string_to_float(const string&, float&);
void  _STLP_CALL __string_to_float(const string&, double&);
/*
 * Returns the position on the right of the digits that has to be concidered
 * for the application of the grouping policy.
 */
extern size_t _STLP_CALL __write_float(__iostring&, ios_base::fmtflags, int, double);
# ifndef _STLP_NO_LONG_DOUBLE
void  _STLP_CALL __string_to_float(const string&, long double&);
extern size_t _STLP_CALL __write_float(__iostring&, ios_base::fmtflags, int, long double);
# endif

/*
 * Gets the digits of the integer part.
 */
void _STLP_CALL __get_floor_digits(__iostring&, _STLP_LONG_DOUBLE);

template <class _CharT>
void _STLP_CALL __get_money_digits(_STLP_BASIC_IOSTRING(_CharT) &, ios_base&, _STLP_LONG_DOUBLE);

# ifndef _STLP_NO_WCHAR_T
extern void _STLP_CALL __convert_float_buffer(__iostring const&, __iowstring&, const ctype<wchar_t>&, wchar_t, bool = true);
# endif
extern void _STLP_CALL __adjust_float_buffer(__iostring&, char);

extern char* _STLP_CALL
__write_integer(char* buf, ios_base::fmtflags flags, long x);

extern ptrdiff_t _STLP_CALL __insert_grouping(char* first, char* last, const string&, char, char, char, int);
extern void _STLP_CALL __insert_grouping(__iostring&, size_t, const string&, char, char, char, int);
# ifndef _STLP_NO_WCHAR_T
extern ptrdiff_t _STLP_CALL __insert_grouping(wchar_t*, wchar_t*, const string&, wchar_t, wchar_t, wchar_t, int);
extern void _STLP_CALL __insert_grouping(__iowstring&, size_t, const string&, wchar_t, wchar_t, wchar_t, int);
# endif

#endif /* _STLP_EXPOSE_STREAM_IMPLEMENTATION */

# if defined (__BORLANDC__) && defined (_RTLDLL)
inline void _Stl_loc_init_num_put() {
  
  num_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index = 14;
  num_put<char, char*>::id._M_index = 15;
  
#  ifndef _STLP_NO_WCHAR_T
  num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > ::id._M_index = 33;
  num_put<wchar_t, wchar_t*>::id._M_index = 34;
#  endif
  
}
 
# endif /* __BORLANDC__ && _RTLDLL */

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_num_put.c>
#endif

#endif /* _STLP_INTERNAL_NUMERIC_FACETS_H */

// Local Variables:
// mode:C++
// End:
