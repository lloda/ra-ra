// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Definitions related to complex types.

// (c) Daniel Llorens - 2005, 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cmath>
#include <limits>
#include <complex>
#include <algorithm> // for clamp()

// abs() needs no qualifying for ra:: types (ADL), shouldn't need it on pods either. FIXME maybe let user decide.
// std::max/min are special, see DEF_NAME in ra.hh.
using std::max, std::min, std::abs, std::fma, std::sqrt, std::pow, std::exp, std::swap,
      std::isfinite, std::isinf, std::isnan, std::clamp, std::lerp, std::conj, std::expm1;

#define FOR_FLOAT(T)                                      \
    constexpr T conj(T x) { return x; }                   \
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

#define FOR_FLOAT(R, C)                                                 \
    constexpr C                                                         \
    fma(C const & a, C const & b, C const & c)                          \
    {                                                                   \
        return C(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())), \
                 fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))); \
    }
FOR_FLOAT(float, std::complex<float>)
FOR_FLOAT(double, std::complex<double>)
#undef FOR_FLOAT

namespace ra {

// As an array op; special definitions for rank 0.
template <class T> constexpr bool ra_is_real = std::numeric_limits<T>::is_integer || std::is_floating_point_v<T>;
template <class T> requires (ra_is_real<T>) constexpr T amax(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T amin(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T sqr(T const & x)  { return x*x; }

#define FOR_FLOAT(T)                                                    \
    constexpr T arg(T x)                { return T(0); }                \
    constexpr T conj(T x)               { return x; }                   \
    constexpr T mul_conj(T x, T y)      { return x*y; }                 \
    constexpr T sqrm(T x)               { return sqr(x); }              \
    constexpr T sqrm(T x, T y)          { return sqr(x-y); }            \
    constexpr T dot(T x, T y)           { return x*y; }                 \
    constexpr T fma_conj(T a, T b, T c) { return fma(a, b, c); }        \
    constexpr T norm2(T x)              { return std::abs(x); }         \
    constexpr T norm2(T x, T y)         { return std::abs(x-y); }       \
    constexpr T rel_error(T a, T b)     { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    constexpr T & real_part(T & x)      { return x; } \
    constexpr T imag_part(T x)          { return T(0); }
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

// FIXME few still inline should eventually be constexpr.
#define FOR_FLOAT(R, C)                                                 \
    inline R arg(C x)                  { return std::arg(x); }          \
    constexpr C sqr(C x)               { return x*x; }                  \
    constexpr C dot(C x, C y)          { return x*y; }                  \
    constexpr bool isfinite(C z)       { return std::isfinite(z.real()) && std::isfinite(z.imag()); } \
    constexpr bool isnan(C z)          { return std::isnan(z.real()) || std::isnan(z.imag()); } \
    constexpr bool isinf(C z)          { return (std::isinf(z.real()) || std::isinf(z.imag())) && !isnan(z); } \
    constexpr C xI(R x)                { return C(0, x); }              \
    constexpr C xI(C z)                { return C(-z.imag(), z.real()); } \
    constexpr R real_part(C const & z) { return z.real(); }             \
    constexpr R imag_part(C const & z) { return z.imag(); }             \
    inline R & real_part(C & z)        { return reinterpret_cast<R *>(&z)[0]; } \
    inline R & imag_part(C & z)        { return reinterpret_cast<R *>(&z)[1]; } \
    constexpr R sqrm(C x)              { return sqr(x.real())+sqr(x.imag()); } \
    constexpr R sqrm(C x, C y)         { return sqr(x.real()-y.real())+sqr(x.imag()-y.imag()); } \
    constexpr R norm2(C x)             { return hypot(x.real(), x.imag()); } \
    constexpr R norm2(C x, C y)        { return sqrt(sqrm(x, y)); }     \
    inline R rel_error(C a, C b)       { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    /* conj(a) * b + c */                                               \
    constexpr C                                                         \
    fma_conj(C const & a, C const & b, C const & c)                     \
    {                                                                   \
        return C(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())), \
                 fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    /* conj(a) * b */                                                   \
    constexpr C                                                         \
    mul_conj(C const & a, C const & b)                                  \
    {                                                                   \
        return C(a.real()*b.real()+a.imag()*b.imag(),                   \
                 a.real()*b.imag()-a.imag()*b.real());                  \
    }
FOR_FLOAT(float, std::complex<float>)
FOR_FLOAT(double, std::complex<double>)
#undef FOR_FLOAT

} // namespace ra
