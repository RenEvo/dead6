/*
 * Copyright (c) 2004
 * Francois Dumont
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

//Included from _string.h, no need for macro guarding.

#if defined (_STLP_DEBUG)
#  define basic_string _STLP_NON_DBG_NAME(str)
#endif

#define _STLP_NO_MEM_T_STRING_BASE _STLP_NO_MEM_T_NAME(str)<_CharT, _Traits, _Alloc>

template <class _CharT, class _Traits, class _Alloc>
class basic_string : public _STLP_NO_MEM_T_STRING_BASE
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && \
    !defined (basic_string)
                   , public __stlport_class<basic_string<_CharT, _Traits, _Alloc> >
#endif
{
protected:                        // Protected members inherited from base.
  typedef basic_string<_CharT, _Traits, _Alloc> _Self;
  typedef _STLP_NO_MEM_T_STRING_BASE _Base;
  typedef typename _Base::_NonDbgBase _NonDbgBase;
  typedef _Base _DbgBase;
  typedef typename _NonDbgBase::_Char_Is_POD _Char_Is_POD;

public:

  __IMPORT_WITH_REVERSE_ITERATORS(_NonDbgBase)

  typedef typename _NonDbgBase::_Iterator_category _Iterator_category;
  typedef typename _NonDbgBase::traits_type traits_type;
  typedef typename _NonDbgBase::_Reserve_t _Reserve_t;

public:                         // Constructor, destructor, assignment.
  explicit basic_string(const allocator_type& __a = allocator_type())
    : _STLP_NO_MEM_T_STRING_BASE(__a) {}

  basic_string(_Reserve_t __r, size_t __n,
               const allocator_type& __a = allocator_type())
    : _STLP_NO_MEM_T_STRING_BASE(__r, __n, __a) {}

  basic_string(const _Self& __s) 
    : _STLP_NO_MEM_T_STRING_BASE(__s) {}

  basic_string(const _Self& __s, size_type __pos, size_type __n = _NonDbgBase::npos,
               const allocator_type& __a = allocator_type()) 
    : _STLP_NO_MEM_T_STRING_BASE(__s, __pos, __n, __a) {}

  basic_string(const _CharT* __s, size_type __n,
               const allocator_type& __a = allocator_type()) 
    : _STLP_NO_MEM_T_STRING_BASE(__s, __n, __a) {}

  basic_string(const _CharT* __s,
               const allocator_type& __a = allocator_type())
    : _STLP_NO_MEM_T_STRING_BASE(__s, __a) {}

  basic_string(size_type __n, _CharT __c,
               const allocator_type& __a = allocator_type())
    : _STLP_NO_MEM_T_STRING_BASE(__n, __c, __a) {}

  basic_string(__move_source<_Self> src)
    : _STLP_NO_MEM_T_STRING_BASE(__move_source<_Base>(src.get())) {}
  
  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
#if !(defined(__MRC__) || (defined(__SC__) && !defined(__DMC__))) //*ty 04/30/2001 - mpw compilers choke on this ctor
  template <class _InputIterator> 
  basic_string(_InputIterator __f, _InputIterator __l,
               const allocator_type & __a _STLP_ALLOCATOR_TYPE_DFL)
    : _STLP_NO_MEM_T_STRING_BASE(_Base::_CalledFromWorkaround_t(), __a) {
    typedef typename _Is_integer<_InputIterator>::_Integral _Integral;
    _M_initialize_dispatch(__f, __l, _Integral());
  }
#  if defined (_STLP_NEEDS_EXTRA_TEMPLATE_CONSTRUCTORS)
  template <class _InputIterator> 
  basic_string(_InputIterator __f, _InputIterator __l)
    : _STLP_NO_MEM_T_STRING_BASE(_Base::_CalledFromWorkaround_t(), allocator_type()) {
    typedef typename _Is_integer<_InputIterator>::_Integral _Integral;
    _M_initialize_dispatch(__f, __l, _Integral());
  }
#  endif
#endif /* !__MRC__ || (__SC__ && !__DMC__) */

  _Self& operator=(const _Self& __s) {
    _NonDbgBase::operator=(__s);
    return *this;
  }

  _Self& operator=(const _CharT* __s) {
    _NonDbgBase::operator=(__s);
    return *this;
  }

  _Self& operator=(_CharT __c) {
    _NonDbgBase::operator=(__c);
    return *this;
  }

private:          
  template <class _InputIter> 
  void _M_range_initialize(_InputIter __f, _InputIter __l,
                           const input_iterator_tag &__tag) {
    this->_M_allocate_block();
    this->_M_construct_null(this->_M_Finish());
    _STLP_TRY {
      _M_appendT(__f, __l, __tag);
    }
    _STLP_UNWIND(this->_M_destroy_range())
  }

  template <class _ForwardIter> 
  void _M_range_initialize(_ForwardIter __f, _ForwardIter __l, 
                           const forward_iterator_tag &) {
    difference_type __n = distance(__f, __l);
    this->_M_allocate_block(__n + 1);
#if defined (_STLP_USE_SHORT_STRING_OPTIM)
    if (this->_M_using_static_buf()) {
      _M_copyT(__f, __l, this->_M_Start());
      this->_M_finish = this->_M_Start() + __n;
    }
    else
#endif /* _STLP_USE_SHORT_STRING_OPTIM */
    this->_M_finish = uninitialized_copy(__f, __l, this->_M_Start());
    this->_M_terminate_string();
  }

  template <class _InputIter> 
  void _M_range_initializeT(_InputIter __f, _InputIter __l) {
    _M_range_initialize(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
  }

  template <class _Integer> 
  void _M_initialize_dispatch(_Integer __n, _Integer __x, const __true_type& /*_Integral*/) {
    this->_M_allocate_block(__n + 1);
#if defined (_STLP_USE_SHORT_STRING_OPTIM)
    if (this->_M_using_static_buf()) {
      _Traits::assign(this->_M_Start(), __n, __x);
      this->_M_finish = this->_M_Start() + __n;
    }
    else
#endif /* _STLP_USE_SHORT_STRING_OPTIM */
    this->_M_finish = __uninitialized_fill_n(this->_M_Start(), __n, __x, _Char_Is_POD());
    this->_M_terminate_string();
  }

  template <class _InputIter> 
  void _M_initialize_dispatch(_InputIter __f, _InputIter __l, const __false_type& /*_Integral*/) {
    _M_range_initializeT(__f, __l);
  }
    
public:                         // Append, operator+=, push_back.
  _Self& operator+=(const _Self& __s) {
    _NonDbgBase::operator+=(__s);
    return *this;
  }
  _Self& operator+=(const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::operator+=(__s);
    return *this; 
  }
  _Self& operator+=(_CharT __c) {
    _NonDbgBase::operator+=(__c);
    return *this; 
  }

  _Self& append(const _Self& __s) {
    _NonDbgBase::append(__s);
    return *this;
  }

  _Self& append(const _Self& __s,
                size_type __pos, size_type __n) {
    _NonDbgBase::append(__s, __pos, __n);
    return *this;
  }

  _Self& append(const _CharT* __s, size_type __n) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::append(__s, __n);
    return *this;
  }
  _Self& append(const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::append(__s);
    return *this;
  }
  _Self& append(size_type __n, _CharT __c) {
    _NonDbgBase::append(__n, __c);
    return *this;
  }

  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  _Self& append(_InputIter __first, _InputIter __last) {
    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
    return _M_append_dispatch(__first, __last, _Integral());
  }

#if !defined (_STLP_NO_METHOD_SPECIALIZATION) && !defined (_STLP_NO_EXTENSIONS)
  //See equivalent assign method remark.
  _Self& append(const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__f)_STLP_FIX_LITERAL_BUG(__l)
    _NonDbgBase::append(__f, __l);
    return *this;
  }
#endif

private:                        // Helper functions for append.

  template <class _InputIter> 
  _Self& _M_appendT(_InputIter __first, _InputIter __last, 
                   const input_iterator_tag &) {
    for ( ; __first != __last ; ++__first)
      _NonDbgBase::push_back(*__first);
    return *this;
  }

  template <class _ForwardIter> 
  _Self& _M_appendT(_ForwardIter __first, _ForwardIter __last, 
                    const forward_iterator_tag &)  {
    if (__first != __last) {
      const size_type __old_size = this->size();
      difference_type __n = distance(__first, __last);
      if (__STATIC_CAST(size_type,__n) > this->max_size() || __old_size > this->max_size() - __STATIC_CAST(size_type,__n))
        this->_M_throw_length_error();
      if (__old_size + __n > this->capacity()) {
        const size_type __len = __old_size +
          (max)(__old_size, __STATIC_CAST(size_type,__n)) + 1;
        pointer __new_start = this->_M_end_of_storage.allocate(__len);
        pointer __new_finish = __new_start;
        _STLP_TRY {
          __new_finish = uninitialized_copy(this->_M_Start(), this->_M_Finish(), __new_start);
          __new_finish = uninitialized_copy(__first, __last, __new_finish);
          this->_M_construct_null(__new_finish);
        }
        _STLP_UNWIND((_STLP_STD::_Destroy_Range(__new_start,__new_finish),
          this->_M_end_of_storage.deallocate(__new_start,__len)))
          this->_M_destroy_range();
        this->_M_deallocate_block();
        this->_M_reset(__new_start, __new_finish, __new_start + __len);
      }
      else {
        _ForwardIter __f1 = __first;
        ++__f1;
#if defined (_STLP_USE_SHORT_STRING_OPTIM)
        if (this->_M_using_static_buf())
          _M_copyT(__f1, __last, this->_M_Finish() + 1);
        else
#endif /* _STLP_USE_SHORT_STRING_OPTIM */
          uninitialized_copy(__f1, __last, this->_M_Finish() + 1);
        _STLP_TRY {
          this->_M_construct_null(this->_M_Finish() + __n);
        }
        _STLP_UNWIND(this->_M_destroy_ptr_range(this->_M_Finish() + 1, this->_M_Finish() + __n))
        _Traits::assign(*this->_M_finish, *__first);
        this->_M_finish += __n;
      }
    }
    return *this;  
  }
  
  template <class _Integer>
  _Self& _M_append_dispatch(_Integer __n, _Integer __x, const __true_type& /*Integral*/) {
    return append((size_type) __n, (_CharT) __x);
  }

  template <class _InputIter>
  _Self& _M_append_dispatch(_InputIter __f, _InputIter __l, const __false_type& /*Integral*/) {
    return _M_appendT(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
  }

public:                         // Assign
  
  _Self& assign(const _Self& __s) {
    _NonDbgBase::assign(__s);
    return *this;
  }

  _Self& assign(const _Self& __s, 
                size_type __pos, size_type __n) {
    _NonDbgBase::assign(__s, __pos, __n);
    return *this;
  }

  _Self& assign(const _CharT* __s, size_type __n) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::assign(__s, __n);
    return *this;
  }

  _Self& assign(const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::assign(__s);
    return *this;
  }

  _Self& assign(size_type __n, _CharT __c) {
    _NonDbgBase::assign(__n, __c);
    return *this;
  }

private:                        // Helper functions for assign.

  template <class _Integer> 
  _Self& _M_assign_dispatch(_Integer __n, _Integer __x, const __true_type& /*_Integral*/) {
    return assign((size_type) __n, (_CharT) __x);
  }

  template <class _InputIter> 
  _Self& _M_assign_dispatch(_InputIter __f, _InputIter __l, const __false_type& /*_Integral*/)  {
    pointer __cur = this->_M_Start();
    while (__f != __l && __cur != this->_M_Finish()) {
      _Traits::assign(*__cur, *__f);
      ++__f;
      ++__cur;
    }
    if (__f == __l)
      _NonDbgBase::erase(__cur, this->_M_Finish());
    else
      _M_appendT(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
    return *this;
  }
  
public:
  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter> 
  _Self& assign(_InputIter __first, _InputIter __last) {
    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
    return _M_assign_dispatch(__first, __last, _Integral());
  }

#if !defined (_STLP_NO_METHOD_SPECIALIZATION) && !defined (_STLP_NO_EXTENSIONS)
  /* This method is not part of the standard and is a specialization of the 
   * template method assign. It is only granted for convenience to call assign
   * with mixed parameters iterator and const_iterator.
   */
  _Self& assign(const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__f)_STLP_FIX_LITERAL_BUG(__l)
    _NonDbgBase::assign(__f, __l);
    return *this;
  }
#endif

public:                         // Insert

  _Self& insert(size_type __pos, const _Self& __s) {
    _NonDbgBase::insert(__pos, __s);
    return *this;
  }

  _Self& insert(size_type __pos, const _Self& __s,
                size_type __beg, size_type __n) {
    _NonDbgBase::insert(__pos, __s, __beg, __n);
    return *this;
  }
  _Self& insert(size_type __pos, const _CharT* __s, size_type __n) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::insert(__pos, __s, __n);
    return *this;
  }

  _Self& insert(size_type __pos, const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::insert(__pos, __s);
    return *this;
  }
    
  _Self& insert(size_type __pos, size_type __n, _CharT __c) {
    _NonDbgBase::insert(__pos, __n, __c);
    return *this;
  }
  
  iterator insert(iterator __p, _CharT __c) {
    return _NonDbgBase::insert(__p, __c);
  }

  void insert(iterator __p, size_t __n, _CharT __c) {
    _NonDbgBase::insert(__p, __n, __c);
  }

  // Check to see if _InputIterator is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  void insert(iterator __p, _InputIter __first, _InputIter __last) {
    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
    _M_insert_dispatch(__p, __first, __last, _Integral());
  }

private:  // Helper functions for insert.

  void _M_insert(iterator __p, const _CharT* __f, const _CharT* __l, bool __self_ref) {
    _STLP_FIX_LITERAL_BUG(__f)_STLP_FIX_LITERAL_BUG(__l)
    _NonDbgBase::_M_insert(__p, __f, __l, __self_ref); 
  }
                 
  template <class _ForwardIter>
  void _M_insert_overflow(iterator __position, _ForwardIter __first, _ForwardIter __last,
                          difference_type __n) {
    const size_type __old_size = this->size();        
    const size_type __len = __old_size + (max)(__old_size, __STATIC_CAST(size_type,__n)) + 1;
    pointer __new_start = this->_M_end_of_storage.allocate(__len);
    pointer __new_finish = __new_start;
    _STLP_TRY {
      __new_finish = uninitialized_copy(this->_M_Start(), __position, __new_start);
      __new_finish = uninitialized_copy(__first, __last, __new_finish);
      __new_finish = uninitialized_copy(__position, this->_M_Finish(), __new_finish);
      this->_M_construct_null(__new_finish);
    }
    _STLP_UNWIND((_STLP_STD::_Destroy_Range(__new_start,__new_finish),
                  this->_M_end_of_storage.deallocate(__new_start,__len)))
    this->_M_destroy_range();
    this->_M_deallocate_block();
    this->_M_reset(__new_start, __new_finish, __new_start + __len);
  }

  template <class _InputIter> 
  void _M_insertT(iterator __p, _InputIter __first, _InputIter __last,
                  const input_iterator_tag &) {
    for ( ; __first != __last; ++__first) {
      __p = insert(__p, *__first);
      ++__p;
    }
  }

  template <class _ForwardIter>
  void _M_insertT(iterator __position, _ForwardIter __first, _ForwardIter __last, 
                  const forward_iterator_tag &) {
    if (__first != __last) {
      difference_type __n = distance(__first, __last);
      if (this->_M_end_of_storage._M_data - this->_M_finish >= __n + 1) {
        const difference_type __elems_after = this->_M_finish - __position;
        if (__elems_after >= __n) {
#if defined (_STLP_USE_SHORT_STRING_OPTIM)
          if (this->_M_using_static_buf())
            _NonDbgBase::_M_copy((this->_M_Finish() - __n) + 1, this->_M_Finish() + 1, this->_M_Finish() + 1);
          else
#endif /* _STLP_USE_SHORT_STRING_OPTIM */
          uninitialized_copy((this->_M_Finish() - __n) + 1, this->_M_Finish() + 1, this->_M_Finish() + 1);
          this->_M_finish += __n;
          _Traits::move(__position + __n, __position, (__elems_after - __n) + 1);
          _M_copyT(__first, __last, __position);
        }
        else {
          pointer __old_finish = this->_M_Finish();
          _ForwardIter __mid = __first;
          advance(__mid, __elems_after + 1);
#if defined (_STLP_USE_SHORT_STRING_OPTIM)
          if (this->_M_using_static_buf())
            _M_copyT(__mid, __last, this->_M_Finish() + 1);
          else
#endif /* _STLP_USE_SHORT_STRING_OPTIM */
          uninitialized_copy(__mid, __last, this->_M_Finish() + 1);
          this->_M_finish += __n - __elems_after;
          _STLP_TRY {
#if defined (_STLP_USE_SHORT_STRING_OPTIM)
            if (this->_M_using_static_buf())
              _NonDbgBase::_M_copy(__position, __old_finish + 1, this->_M_Finish());
            else
#endif /* _STLP_USE_SHORT_STRING_OPTIM */
            uninitialized_copy(__position, __old_finish + 1, this->_M_Finish());
            this->_M_finish += __elems_after;
          }
          _STLP_UNWIND((this->_M_destroy_ptr_range(__old_finish + 1, this->_M_Finish()), 
                        this->_M_finish = __old_finish))
          _M_copyT(__first, __mid, __position);
        }
      }
      else {
        _M_insert_overflow(__position, __first, __last, __n);
      }
    }
  }

  template <class _Integer>
  void _M_insert_dispatch(iterator __p, _Integer __n, _Integer __x,
                          const __true_type& /*Integral*/) {
    insert(__p, (size_type) __n, (_CharT) __x);
  }
  
  template <class _InputIter>
  void _M_insert_dispatch(iterator __p, _InputIter __first, _InputIter __last,
                          const __false_type& /*Integral*/) {
    _STLP_FIX_LITERAL_BUG(__p)
    /*
     * Within the basic_string implementation we are only going to check for
     * self referencing if iterators are string iterators or _CharT pointers.
     * A user could encapsulate those iterator within their own iterator interface
     * and in this case lead to a bad behavior, this is a known limitation.
     */
    typedef typename _AreSameUnCVTypes<_InputIter, iterator>::_Ret _IsIterator;
    typedef typename _AreSameUnCVTypes<_InputIter, const_iterator>::_Ret _IsConstIterator;
    typedef typename _Lor2<_IsIterator, _IsConstIterator>::_Ret _CheckInside;
    _M_insert_aux(__p, __first, __last, _CheckInside());
  }
  
  template <class _RandomIter>
  void _M_insert_aux (iterator __p, _RandomIter __first, _RandomIter __last,
                      const __true_type& /*_CheckInside*/) {
    _STLP_FIX_LITERAL_BUG(__p)
    _M_insert(__p, &(*__first), &(*__last), _NonDbgBase::_M_inside(&(*__first)));
  }
  
  template<class _InputIter>
  void _M_insert_aux (iterator __p, _InputIter __first, _InputIter __last,
                      const __false_type& /*_CheckInside*/) {
    _STLP_FIX_LITERAL_BUG(__p)
    _M_insertT(__p, __first, __last, _STLP_ITERATOR_CATEGORY(__first, _InputIter));
  }

  template <class _InputIterator>
  void _M_copyT(_InputIterator __first, _InputIterator __last, pointer __result) {
    _STLP_FIX_LITERAL_BUG(__p)
    for ( ; __first != __last; ++__first, ++__result)
      _Traits::assign(*__result, *__first);
  }

#if !defined (_STLP_NO_METHOD_SPECIALIZATION)
  void _M_copyT(const _CharT* __f, const _CharT* __l, _CharT* __res) {
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l) _STLP_FIX_LITERAL_BUG(__res)
    _NonDbgBase::_M_copy(__f, __l, __res);
  }
#endif

public:                         // Erase.

  _Self& erase(size_type __pos = 0, size_type __n = _NonDbgBase::npos) {
    _NonDbgBase::erase(__pos, __n);
    return *this;
  }

  iterator erase(iterator __pos) {
    _STLP_FIX_LITERAL_BUG(__pos)
    return _NonDbgBase::erase(__pos);
  }

  iterator erase(iterator __first, iterator __last) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    return _NonDbgBase::erase(__first, __last);
  }

public:                         // Replace.  (Conceptually equivalent
                                // to erase followed by insert.)
  _Self& replace(size_type __pos, size_type __n, const _Self& __s) {
    _NonDbgBase::replace(__pos, __n, __s);
    return *this;
  }

  _Self& replace(size_type __pos1, size_type __n1, const _Self& __s,
                 size_type __pos2, size_type __n2) {
    _NonDbgBase::replace(__pos1, __n1, __s, __pos2, __n2);
    return *this;
  }

  _Self& replace(size_type __pos, size_type __n1,
                 const _CharT* __s, size_type __n2) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::replace(__pos, __n1, __s, __n2);
    return *this;
  }

  _Self& replace(size_type __pos, size_type __n1, const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::replace(__pos, __n1, __s);
    return *this;
  }

  _Self& replace(size_type __pos, size_type __n1,
                 size_type __n2, _CharT __c) {
    _NonDbgBase::replace(__pos, __n1, __n2, __c);
    return *this;
  }

  _Self& replace(iterator __first, iterator __last, const _Self& __s) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    _NonDbgBase::replace(__first, __last, __s);
    return *this;
  }

  _Self& replace(iterator __first, iterator __last,
                 const _CharT* __s, size_type __n) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::replace(__first, __last, __s, __n);
    return *this;
  }

  _Self& replace(iterator __first, iterator __last,
                 const _CharT* __s) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__s)
    _NonDbgBase::replace(__first, __last, __s);
    return *this;
  }

  _Self& replace(iterator __first, iterator __last, 
                 size_type __n, _CharT __c) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    _NonDbgBase::replace(__first, __last, __n, __c);
    return *this;
  }

  // Check to see if _InputIter is an integer type.  If so, then
  // it can't be an iterator.
  template <class _InputIter>
  _Self& replace(iterator __first, iterator __last,
                 _InputIter __f, _InputIter __l) {
    _STLP_FIX_LITERAL_BUG(__first)_STLP_FIX_LITERAL_BUG(__last)
    typedef typename _Is_integer<_InputIter>::_Integral _Integral;
    return _M_replace_dispatch(__first, __last, __f, __l,  _Integral());
  }

#if !defined (_STLP_NO_METHOD_SPECIALIZATION) && !defined (_STLP_NO_EXTENSIONS)
  _Self& replace(iterator __first, iterator __last,
                 const _CharT* __f, const _CharT* __l) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    _NonDbgBase::replace(__first, __last, __f, __l);
    return *this;
  }
#endif

protected:                        // Helper functions for replace.
  _Self& _M_replace(iterator __first, iterator __last,
                    const _CharT* __f, const _CharT* __l, bool __self_ref) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    _STLP_FIX_LITERAL_BUG(__f) _STLP_FIX_LITERAL_BUG(__l)
    _NonDbgBase::_M_replace(__first, __last, __f, __l, __self_ref);
    return *this;
  }
                     
  template <class _Integer>
  _Self& _M_replace_dispatch(iterator __first, iterator __last,
                             _Integer __n, _Integer __x, const __true_type& /*IsIntegral*/) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    return replace(__first, __last, (size_type) __n, (_CharT) __x);
  }

  template <class _InputIter> 
  _Self& _M_replace_dispatch(iterator __first, iterator __last,
                             _InputIter __f, _InputIter __l, const __false_type& /*IsIntegral*/) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    typedef typename _AreSameUnCVTypes<_InputIter, iterator>::_Ret _IsIterator;
    typedef typename _AreSameUnCVTypes<_InputIter, const_iterator>::_Ret _IsConstIterator;
    typedef typename _Lor2<_IsIterator, _IsConstIterator>::_Ret _CheckInside;
    return _M_replace_aux(__first, __last, __f, __l, _CheckInside());
  }
  
  template <class _RandomIter>
  _Self& _M_replace_aux(iterator __first, iterator __last,
                        _RandomIter __f, _RandomIter __l, __true_type const& /*_CheckInside*/) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    return _M_replace(__first, __last, &(*__f), &(*__l), _NonDbgBase::_M_inside(&(*__f)));
  }
  
  template <class _InputIter>
  _Self& _M_replace_aux(iterator __first, iterator __last,
                     _InputIter __f, _InputIter __l, __false_type const& /*_CheckInside*/) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    return _M_replaceT(__first, __last, __f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIter));
  }
  
  template <class _InputIter>
  _Self& _M_replaceT(iterator __first, iterator __last,
                     _InputIter __f, _InputIter __l, const input_iterator_tag&__ite_tag) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    for ( ; __first != __last && __f != __l; ++__first, ++__f)
      _Traits::assign(*__first, *__f);
    if (__f == __l)
      _NonDbgBase::erase(__first, __last);
    else
      _M_insertT(__last, __f, __l, __ite_tag);
    return *this;
  }

  template <class _ForwardIter> 
  _Self& _M_replaceT(iterator __first, iterator __last,
                     _ForwardIter __f, _ForwardIter __l, const forward_iterator_tag &__ite_tag) {
    _STLP_FIX_LITERAL_BUG(__first) _STLP_FIX_LITERAL_BUG(__last)
    difference_type __n = distance(__f, __l);
    const difference_type __len = __last - __first;
    if (__len >= __n) {
      _M_copyT(__f, __l, __first);
      _NonDbgBase::erase(__first + __n, __last);
    }
    else {
      _ForwardIter __m = __f;
      advance(__m, __len);
      _M_copyT(__f, __m, __first);
      _M_insertT(__last, __m, __l, __ite_tag);
    }
    return *this;
  }

public:                         // Other modifier member functions.

  void swap(_Self& __s) 
  { _NonDbgBase::swap(__s); }

public:                         // Substring.

  _Self substr(size_type __pos = 0, size_type __n = _NonDbgBase::npos) const
  { return _Self(*this, __pos, __n, get_allocator()); }

#if defined (_STLP_USE_TEMPLATE_EXPRESSION) && !defined (_STLP_DEBUG)
#  define _STLP_STRING_SUM_BASE _STLP_NO_MEM_T_STRING_BASE
#  define _STLP_STRING_BASE_SCOPE _NonDbgBase::
#  include <stl/_string_sum_methods.h>
#  undef _STLP_STRING_BASE_SCOPE
#  undef _STLP_STRING_SUM_BASE
#endif /* _STLP_USE_TEMPLATE_EXPRESSION */
};

#undef _STLP_NO_MEM_T_STRING_BASE
#undef basic_string

#if defined (_STLP_DEBUG)
_STLP_END_NAMESPACE
#  include <stl/debug/_string_workaround.h>
_STLP_BEGIN_NAMESPACE
#endif
