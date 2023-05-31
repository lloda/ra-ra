// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Global definitions related to complex types.

// (c) Daniel Llorens - 2005, 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "bootstrap.hh"
#include <algorithm>
#include <complex>
#include <limits>
#include <cmath>

// just as max() and min() are found for ra:: types w/o qualifying (through ADL) they should also be found for the POD types.
// besides, gcc still leaks cmath functions into the global namespace, so by default e.g. sqrt would be C double sqrt(double) instead of the overload set.
// cf http://ericniebler.com/2014/10/21/customization-point-design-in-c11-and-beyond/
using std::abs, std::max, std::min, std::fma, std::clamp, std::sqrt, std::pow, std::exp,
    std::swap, std::isfinite, std::isinf, std::lerp;

#define RA_IS_REAL(T) (std::numeric_limits<T>::is_integer || std::is_floating_point_v<T>)
#define RA_REAL_OVERLOAD_CE(T) template <class T> requires (RA_IS_REAL(T)) constexpr T
// As an array op; special definitions for rank 0.
RA_REAL_OVERLOAD_CE(T) arg(T const x) { return 0; }
RA_REAL_OVERLOAD_CE(T) amax(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) amin(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) sqr(T const x) { return x*x; }
RA_REAL_OVERLOAD_CE(T) real_part(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) imag_part(T const x) { return 0.; }
RA_REAL_OVERLOAD_CE(T) conj(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) sqrm(T const x) { return x*x; }
RA_REAL_OVERLOAD_CE(T) norm2(T const x) { return std::abs(x); }
#undef RA_REAL_OVERLOAD_CE
#undef RA_IS_REAL

#define FOR_FLOAT(T)                                                    \
    constexpr T mul_conj(T const x, T const y)  { return x*y; }  \
    constexpr T sqrm(T const x, T const y)      { return sqrm(x-y); } \
    constexpr T dot(T const x, T const y)       { return x*y; }  \
    constexpr T fma_conj(T const a, T const b, T const c) { return fma(a, b, c); } \
    constexpr T norm2(T const x, T const y)     { return std::abs(x-y); } \
    constexpr T abs(T const x, T const y)       { return std::abs(x-y); } \
    constexpr T rel_error(T const a, T const b) { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*abs(a, b)/den; }
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

namespace ra {

template <class T> constexpr bool is_scalar_def<std::complex<T>> = true;

} // namespace ra

#define FOR_FLOAT(R, C)                                                 \
    inline /* constexpr */ R arg(C const x)     { return std::arg(x); } \
    constexpr C xI(R const x)                   { return C(0, x); }     \
    constexpr C xI(C const z)                   { return C(-z.imag(), z.real()); } \
    constexpr R real_part(C const & z)          { return z.real(); }    \
    constexpr R imag_part(C const & z)          { return z.imag(); }    \
    constexpr R sqrm(C const x)                 { return sqrm(x.real())+sqrm(x.imag()); } \
    constexpr R sqrm(C const x, C const y)      { return sqrm(x.real()-y.real())+sqrm(x.imag()-y.imag()); } \
    constexpr C sqr(C const x)                  { return x*x; }         \
    constexpr C dot(C const x, C const y)       { return x*y; }         \
    constexpr R norm2(C const x)                { return hypot(x.real(), x.imag()); } \
    constexpr R norm2(C const x, C const y)     { return sqrt(sqrm(x, y)); } \
    constexpr R abs(C const x, C const y)       { return sqrt(sqrm(x, y)); } \
    inline /* constexpr */ R & real_part(C & z) { return reinterpret_cast<R *>(&z)[0]; } \
    inline /* constexpr */ R & imag_part(C & z) { return reinterpret_cast<R *>(&z)[1]; }
FOR_FLOAT(double, std::complex<double>);
FOR_FLOAT(float, std::complex<float>);
#undef FOR_FLOAT

#define RA_REAL double
#define RA_CPLX std::complex<RA_REAL>

constexpr RA_CPLX
fma(RA_CPLX const & a, RA_CPLX const & b, RA_CPLX const & c)
{
    return RA_CPLX(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())),
                   fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag())));
}

// conj(a) * b + c
constexpr RA_CPLX
fma_conj(RA_CPLX const & a, RA_CPLX const & b, RA_CPLX const & c)
{
    return RA_CPLX(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())),
                   fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag())));
}

// conj(a) * b
constexpr RA_CPLX
mul_conj(RA_CPLX const & a, RA_CPLX const & b)
{
    return RA_CPLX(a.real()*b.real()+a.imag()*b.imag(),
                   a.real()*b.imag()-a.imag()*b.real());
}

constexpr bool
isfinite(RA_CPLX const z)
{
    return std::isfinite(z.real()) && std::isfinite(z.imag());
}

constexpr bool
isnan(RA_CPLX const z)
{
    return std::isnan(z.real()) || std::isnan(z.imag());
}

constexpr bool
isinf(RA_CPLX const z)
{
    bool const a = std::isinf(z.real());
    bool const b = std::isinf(z.imag());
    return (a && b) || (a && std::isfinite(z.imag())) || (b && std::isfinite(z.real()));
}

constexpr void
swap(RA_CPLX & a, RA_CPLX & b)
{
    std::swap(a, b);
}

constexpr RA_REAL
rel_error(RA_CPLX const a, RA_CPLX const b)
{
    return (a==0. && b==0.) ? 0. : 2.*abs(a, b)/(abs(a)+abs(b));
}

#undef RA_CPLX
#undef RA_REAL
