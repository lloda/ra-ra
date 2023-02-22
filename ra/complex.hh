// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Global definitions related to complex types.

// (c) Daniel Llorens - 2005, 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <complex>
#include "real.hh"
#include "bootstrap.hh"

namespace ra {

template <class T> constexpr bool is_scalar_def<std::complex<T>> = true;

} // namespace ra

#define RA_REAL double
#define RA_CPLX std::complex<RA_REAL>

#define FOR_FLOAT(R, C)                                                 \
    inline R arg(C const x)                        { return std::arg(x); } \
    constexpr C xI(R const x)               { return C(0, x); }  \
    constexpr C xI(C const z)               { return C(-z.imag(), z.real()); } \
    constexpr R real_part(C const & z)      { return z.real(); } \
    constexpr R imag_part(C const & z)      { return z.imag(); } \
    constexpr R sqrm(C const x)             { return sqrm(x.real())+sqrm(x.imag()); } \
    constexpr R sqrm(C const x, C const y)  { return sqrm(x.real()-y.real())+sqrm(x.imag()-y.imag()); } \
    inline C sqr(C const x)                        { return x*x; }      \
    inline C dot(C const x, C const y)             { return x*y; }      \
    constexpr R norm2(C const x)            { return hypot(x.real(), x.imag()); } \
    constexpr R norm2(C const x, C const y) { return sqrt(sqrm(x, y)); } \
    constexpr R abs(C const x, C const y)   { return sqrt(sqrm(x, y)); } \
    inline /* constexpr */ R & real_part(C & z)    { return reinterpret_cast<R *>(&z)[0]; } \
    inline /* constexpr */ R & imag_part(C & z)    { return reinterpret_cast<R *>(&z)[1]; }
FOR_FLOAT(double, std::complex<double>);
FOR_FLOAT(float, std::complex<float>);
#undef FOR_FLOAT

inline RA_CPLX fma(RA_CPLX const & a, RA_CPLX const & b, RA_CPLX const & c)
{
    return RA_CPLX(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())),
                   fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag())));
}

// conj(a) * b + c
inline RA_CPLX fma_conj(RA_CPLX const & a, RA_CPLX const & b, RA_CPLX const & c)
{
    return RA_CPLX(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())),
                   fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag())));
}

// conj(a) * b
inline RA_CPLX mul_conj(RA_CPLX const & a, RA_CPLX const & b)
{
    return RA_CPLX(a.real()*b.real()+a.imag()*b.imag(),
                   a.real()*b.imag()-a.imag()*b.real());
}
inline bool isfinite(RA_CPLX const z)
{
    return std::isfinite(z.real()) && std::isfinite(z.imag());
}
inline bool isnan(RA_CPLX const z)
{
    return std::isnan(z.real()) || std::isnan(z.imag());
}
inline bool isinf(RA_CPLX const z)
{
    bool const a = std::isinf(z.real());
    bool const b = std::isinf(z.imag());
    return (a && b) || (a && std::isfinite(z.imag())) || (b && std::isfinite(z.real()));
}
inline void swap(RA_CPLX & a, RA_CPLX & b)
{
    std::swap(a, b);
}
inline RA_CPLX tanh(RA_CPLX const z)
{
    return (z.real()>300.) ? 1. : ((z.real()<-300.) ? -1. : sinh(z)/cosh(z));
}
inline RA_REAL rel_error(RA_CPLX const a, RA_CPLX const b)
{
    return (a==0. && b==0.) ? 0. : 2.*abs(a, b)/(abs(a)+abs(b));
}

#undef RA_CPLX
#undef RA_REAL
