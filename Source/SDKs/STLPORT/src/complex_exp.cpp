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

#include "stlport_prefix.h"
// exp, log, pow for complex<float>, complex<double>, and complex<long double>

#include <numeric>
#include <cmath>
#include <complex>

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// exp
template <class _Tp>
complex<_Tp> expT(const complex<_Tp>& z) {
  _Tp expx = ::exp(z._M_re);
  return complex<_Tp>(expx * ::cos(z._M_im),
                      expx * ::sin(z._M_im));
}
_STLP_DECLSPEC complex<float>  _STLP_CALL exp(const complex<float>& z)
{ return expT(z); }

_STLP_DECLSPEC complex<double> _STLP_CALL exp(const complex<double>& z)
{ return expT(z); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL exp(const complex<long double>& z)
{ return expT(z); }
#endif

//----------------------------------------------------------------------
// log10
template <class _Tp>
complex<_Tp> log10T(const complex<_Tp>& z, const _Tp& ln10_inv) {
  complex<_Tp> r;

  r._M_im = ::atan2(z._M_im, z._M_re) * ln10_inv;
  r._M_re = ::log10(::hypot(z._M_re, z._M_im));
  return r;
}

const float LN10_INVF = 1.f / ::log(10.f);
_STLP_DECLSPEC complex<float> _STLP_CALL log10(const complex<float>& z)
{ return log10T(z, LN10_INVF); }

const double LN10_INV = 1. / ::log10(10.);
_STLP_DECLSPEC complex<double> _STLP_CALL log10(const complex<double>& z)
{ return log10T(z, LN10_INV); }

#if !defined (_STLP_NO_LONG_DOUBLE)
const long double LN10_INVL = 1.l / ::log(10.l);
_STLP_DECLSPEC complex<long double> _STLP_CALL log10(const complex<long double>& z)
{ return log10T(z, LN10_INVL); }
#endif

//----------------------------------------------------------------------
// log
template <class _Tp>
complex<_Tp> logT(const complex<_Tp>& z) {
  complex<_Tp> r;

  r._M_im = ::atan2(z._M_im, z._M_re);
  r._M_re = ::log(::hypot(z._M_re, z._M_im));
  return r;
}
_STLP_DECLSPEC complex<float> _STLP_CALL log(const complex<float>& z)
{ return logT(z); }

_STLP_DECLSPEC complex<double> _STLP_CALL log(const complex<double>& z)
{ return logT(z); }

#ifndef _STLP_NO_LONG_DOUBLE
_STLP_DECLSPEC complex<long double> _STLP_CALL log(const complex<long double>& z)
{ return logT(z); }
# endif

//----------------------------------------------------------------------
// pow
template <class _Tp>
complex<_Tp> powT(const _Tp& a, const complex<_Tp>& b) {
  _Tp logr = ::log(a);
  _Tp x = ::exp(logr * b._M_re);
  _Tp y = logr * b._M_im;

  return complex<_Tp>(x * ::cos(y), x * ::sin(y));
}

template <class _Tp>
complex<_Tp> powT(const complex<_Tp>& z_in, int n) {
  complex<_Tp> z = z_in;
  z = __power(z, (n < 0 ? -n : n), multiplies< complex<_Tp> >());
  if (n < 0)
    return _Tp(1.0) / z;
  else
    return z;
}

template <class _Tp>
complex<_Tp> powT(const complex<_Tp>& a, const _Tp& b) {
  _Tp logr = ::log(::hypot(a._M_re,a._M_im));
  _Tp logi = ::atan2(a._M_im, a._M_re);
  _Tp x = ::exp(logr * b);
  _Tp y = logi * b;

  return complex<_Tp>(x * ::cos(y), x * ::sin(y));
}

template <class _Tp>
complex<_Tp> powT(const complex<_Tp>& a, const complex<_Tp>& b) {
  _Tp logr = ::log(::hypot(a._M_re,a._M_im));
  _Tp logi = ::atan2(a._M_im, a._M_re);
  _Tp x = ::exp(logr * b._M_re - logi * b._M_im);
  _Tp y = logr * b._M_im + logi * b._M_re;

  return complex<_Tp>(x * ::cos(y), x * ::sin(y));
}

_STLP_DECLSPEC complex<float> _STLP_CALL pow(const float& a, const complex<float>& b)
{ return powT(a, b); }

_STLP_DECLSPEC complex<float> _STLP_CALL pow(const complex<float>& z_in, int n)
{ return powT(z_in, n); }

_STLP_DECLSPEC complex<float> _STLP_CALL pow(const complex<float>& a, const float& b)
{ return powT(a, b); }

_STLP_DECLSPEC complex<float> _STLP_CALL pow(const complex<float>& a, const complex<float>& b)
{ return powT(a, b); }

_STLP_DECLSPEC complex<double> _STLP_CALL pow(const double& a, const complex<double>& b)
{ return powT(a, b); }

_STLP_DECLSPEC complex<double> _STLP_CALL pow(const complex<double>& z_in, int n)
{ return powT(z_in, n); }

_STLP_DECLSPEC complex<double> _STLP_CALL pow(const complex<double>& a, const double& b)
{ return powT(a, b); }

_STLP_DECLSPEC complex<double> _STLP_CALL pow(const complex<double>& a, const complex<double>& b)
{ return powT(a, b); }

#if !defined (_STLP_NO_LONG_DOUBLE)
_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const long double& a,
                                                   const complex<long double>& b)
{ return powT(a, b); }


_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const complex<long double>& z_in, int n)
{ return powT(z_in, n); }

_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const complex<long double>& a,
                                                   const long double& b)
{ return powT(a, b); }

_STLP_DECLSPEC complex<long double> _STLP_CALL pow(const complex<long double>& a,
                                                   const complex<long double>& b)
{ return powT(a, b); }
#endif

_STLP_END_NAMESPACE
