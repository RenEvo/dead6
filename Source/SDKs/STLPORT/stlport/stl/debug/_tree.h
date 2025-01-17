/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_INTERNAL_DBG_TREE_H
#define _STLP_INTERNAL_DBG_TREE_H

#include <stl/debug/_iterator.h>
#include <stl/_function.h>
#include <stl/_alloc.h>

#  undef _DBG_Rb_tree
#  define _DBG_Rb_tree _Rb_tree

# define _STLP_DBG_TREE_SUPER __WORKAROUND_DBG_RENAME(Rb_tree) <_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc>

_STLP_BEGIN_NAMESPACE

# ifdef _STLP_DEBUG_USE_DISTINCT_VALUE_TYPE_HELPERS
template <class _Key, class _Compare, 
          class _Value, class _KeyOfValue, class _Traits, class _Alloc >
inline _Value*
value_type(const  _DBG_iter_base< _STLP_DBG_TREE_SUPER >&) {
  return (_Value*)0;
}
template <class _Key, class _Compare, 
          class _Value, class _KeyOfValue, class _Traits, class _Alloc >
inline bidirectional_iterator_tag
iterator_category(const  _DBG_iter_base< _STLP_DBG_TREE_SUPER >&) {
  return bidirectional_iterator_tag();
}
# endif
template <class _Key, class _Compare, 
          class _Value, class _KeyOfValue, class _Traits, 
          _STLP_DBG_ALLOCATOR_SELECT(_Value) >
class _DBG_Rb_tree : public _STLP_DBG_TREE_SUPER {
  typedef _STLP_DBG_TREE_SUPER _Base;
  typedef _DBG_Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc> _Self;
protected:
  __owned_list _M_iter_list;

public:
  __IMPORT_CONTAINER_TYPEDEFS(_Base)
  typedef typename _Base::key_type key_type;
  
  typedef typename _Traits::_NonConstTraits _NonConstIteTraits;
  typedef typename _Traits::_ConstTraits    _ConstIteTraits;
  typedef _DBG_iter<_Base, _DbgTraits<_NonConstIteTraits> > iterator;
  typedef _DBG_iter<_Base, _DbgTraits<_ConstIteTraits> >    const_iterator;

  _STLP_DECLARE_BIDIRECTIONAL_REVERSE_ITERATORS;

protected:
  //typedef typename _Base::key_param_type key_param_type;
  //typedef typename _Base::val_param_type val_param_type;

  _Base* _Get_base() { return this; }
  void _Invalidate_iterator(const iterator& __it) { 
    __invalidate_iterator(&_M_iter_list,__it); 
  }
  void _Invalidate_iterators(const iterator& __first, const iterator& __last) {
    __invalidate_range(&_M_iter_list, __first, __last);
  }

  typedef typename _Base::iterator _Base_iterator;
  typedef typename _Base::const_iterator _Base_const_iterator;

public:
  _DBG_Rb_tree() : _STLP_DBG_TREE_SUPER(), 
    _M_iter_list(_Get_base()) {}
  _DBG_Rb_tree(const _Compare& __comp) : 
    _STLP_DBG_TREE_SUPER(__comp), _M_iter_list(_Get_base()) {}
  _DBG_Rb_tree(const _Compare& __comp, const allocator_type& __a): 
    _STLP_DBG_TREE_SUPER(__comp, __a), _M_iter_list(_Get_base()) {}
  _DBG_Rb_tree(const _Self& __x):
    _STLP_DBG_TREE_SUPER(__x), _M_iter_list(_Get_base()) {}

  _DBG_Rb_tree(__move_source<_Self> src):
    _STLP_DBG_TREE_SUPER(__move_source<_Base>(src.get())), _M_iter_list(_Get_base()) {
    src.get()._M_iter_list._Invalidate_all();
  }

  ~_DBG_Rb_tree() {}

  _Self& operator=(const _Self& __x) {
    if (this != &__x) {
      //Should not invalidate end iterator:
      _Invalidate_iterators(this->begin(), this->end());
      _Base::operator=((const _Base&)__x);
    }
    return *this;
  }
  
  iterator begin() { return iterator(&_M_iter_list,_Base::begin()); }
  const_iterator begin() const { return const_iterator(&_M_iter_list, _Base::begin()); }
  iterator end() { return iterator(&_M_iter_list, _Base::end()); }
  const_iterator end() const { return const_iterator(&_M_iter_list,_Base::end()); }

public:
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { 
    return const_reverse_iterator(end()); 
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { 
    return const_reverse_iterator(begin());
  }
  void swap(_Self& __t) {
    _Base::swap(__t);
    _M_iter_list._Swap_owners(__t._M_iter_list);
  }
    
public:

  iterator find(const key_type& __x) {
    return iterator(&_M_iter_list, _Base::find(__x));    
  }
  const_iterator find(const key_type& __x) const {
    return const_iterator(&_M_iter_list, _Base::find(__x));    
  }

  iterator lower_bound(const key_type& __x) {
    return iterator(&_M_iter_list, _Base::lower_bound(__x));    
  }
  const_iterator lower_bound(const key_type& __x) const {
    return const_iterator(&_M_iter_list, _Base::lower_bound(__x));    
  }

  iterator upper_bound(const key_type& __x) {
    return iterator(&_M_iter_list, _Base::upper_bound(__x));    
  }
  const_iterator upper_bound(const key_type& __x) const {
    return const_iterator(&_M_iter_list, _Base::upper_bound(__x));    
  }

  pair<iterator,iterator> equal_range(const key_type& __x) {
    return pair<iterator, iterator>(iterator(&_M_iter_list, _Base::lower_bound(__x)),
                                    iterator(&_M_iter_list, _Base::upper_bound(__x)));
  }
  pair<const_iterator, const_iterator> equal_range(const key_type& __x) const {
    return pair<const_iterator,const_iterator>(const_iterator(&_M_iter_list, _Base::lower_bound(__x)),
                                               const_iterator(&_M_iter_list, _Base::upper_bound(__x)));
  }

  pair<iterator,iterator> equal_range_unique(const key_type& __x) {
    _STLP_STD::pair<_Base_iterator, _Base_iterator> __p;
    __p = _Base::equal_range_unique(__x);
    return pair<iterator, iterator>(iterator(&_M_iter_list, __p.first), iterator(&_M_iter_list, __p.second));
  }
  pair<const_iterator, const_iterator> equal_range_unique(const key_type& __x) const {
    _STLP_STD::pair<_Base_const_iterator, _Base_const_iterator> __p;
    __p = _Base::equal_range_unique(__x);
    return pair<const_iterator, const_iterator>(const_iterator(&_M_iter_list, __p.first), 
                                                const_iterator(&_M_iter_list, __p.second));
  }

  pair<iterator,bool> insert_unique(const value_type& __x) {
    _STLP_STD::pair<_Base_iterator, bool> __res = _Base::insert_unique(__x);
    return pair<iterator,bool>( iterator(&_M_iter_list, __res.first), __res.second ) ;
  }
  iterator insert_equal(const value_type& __x) {
    return iterator(&_M_iter_list, _Base::insert_equal(__x));
  }

  iterator insert_unique(iterator __position, const value_type& __x) {
    _STLP_DEBUG_CHECK(__check_if_owner(&_M_iter_list,__position))
    return iterator(&_M_iter_list, _Base::insert_unique(__position._M_iterator, __x));
  }
  iterator insert_equal(iterator __position, const value_type& __x) {
    _STLP_DEBUG_CHECK(__check_if_owner(&_M_iter_list,__position))
    return iterator(&_M_iter_list, _Base::insert_equal(__position._M_iterator, __x));
  }

#ifdef _STLP_MEMBER_TEMPLATES  
  template<class _InputIterator>
  void insert_equal(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _Base::insert_equal(__first, __last);
  }
  template<class _InputIterator>
  void insert_unique(_InputIterator __first, _InputIterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _Base::insert_unique(__first, __last);
  }
#else /* _STLP_MEMBER_TEMPLATES */
  void insert_unique(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _Base::insert_unique(__first._M_iterator, __last._M_iterator);
  }
  void insert_unique(const value_type* __first, const value_type* __last) {
    _STLP_DEBUG_CHECK(__check_ptr_range(__first,__last))
    _Base::insert_unique(__first, __last);    
  }
  void insert_equal(const_iterator __first, const_iterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last))
    _Base::insert_equal(__first._M_iterator, __last._M_iterator);
  }
  void insert_equal(const value_type* __first, const value_type* __last) {
    _STLP_DEBUG_CHECK(__check_ptr_range(__first,__last))
    _Base::insert_equal(__first, __last);
  }
#endif /* _STLP_MEMBER_TEMPLATES */

  void erase(iterator __position) {
    _STLP_DEBUG_CHECK(__check_if_owner(&_M_iter_list,__position))
    _STLP_DEBUG_CHECK(_Dereferenceable(__position))
    _Invalidate_iterator(__position);
    _Base::erase(__position._M_iterator);
  }
  size_type erase(const key_type& __x) {
    pair<_Base_iterator,_Base_iterator> __p = _Base::equal_range(__x);
    size_type __n = distance(__p.first, __p.second);
    _Invalidate_iterators(iterator(&_M_iter_list, __p.first), iterator(&_M_iter_list, __p.second));
    _Base::erase(__p.first, __p.second);
    return __n;
  }
  size_type erase_unique(const key_type& __x) {
    _Base_iterator __i = _Base::find(__x);
    if (__i != _Base::end()) {
      _Invalidate_iterator(iterator(&_M_iter_list, __i));
      _Base::erase(__i);
      return 1;
    }
    return 0;
  }

  void erase(iterator __first, iterator __last) {
    _STLP_DEBUG_CHECK(__check_range(__first,__last, this->begin(), this->end()))
    _Invalidate_iterators(__first, __last);
    _Base::erase(__first._M_iterator, __last._M_iterator);    
  }
  void erase(const key_type* __first, const key_type* __last) {
    while (__first != __last) erase(*__first++);
  }

  void clear() {
    //should not invalidate end:
    _Invalidate_iterators(this->begin(), this->end());
    _Base::clear();
  }      
};

#define _STLP_TEMPLATE_HEADER template <class _Key, class _Compare, class _Value, class _KeyOfValue, class _Traits, class _Alloc>
#define _STLP_TEMPLATE_CONTAINER _DBG_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>
#define _STLP_TEMPLATE_CONTAINER_BASE _STLP_DBG_TREE_SUPER
#include <stl/debug/_relops_cont.h>
#undef _STLP_TEMPLATE_CONTAINER_BASE
#undef _STLP_TEMPLATE_CONTAINER
#undef _STLP_TEMPLATE_HEADER
         
#ifdef _STLP_CLASS_PARTIAL_SPECIALIZATION
template <class _Key, class _Compare, class _Value, class _KeyOfValue, class _Traits, class _Alloc>
struct __move_traits<_DBG_Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc> > :
  __move_traits_aux<_STLP_DBG_TREE_SUPER >
{};
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

_STLP_END_NAMESPACE

#undef _STLP_DBG_TREE_SUPER
#undef _DBG_Rb_tree

#endif /* _STLP_INTERNAL_DBG_TREE_H */

// Local Variables:
// mode:C++
// End:
