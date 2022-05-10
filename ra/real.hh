// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Global definitions related to float/double.

// (c) Daniel Llorens - 2005, 2015, 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "macros.hh"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <cmath>

// just as max() and min() are found for ra:: types w/o qualifying (through ADL) they should also be found for the POD types.
// besides, gcc still leaks cmath functions into the global namespace, so by default e.g. sqrt would be C double sqrt(double) instead of the overload set.
using std::abs, std::max, std::min, std::fma, std::clamp, std::sqrt, std::pow, std::exp,
    std::swap, std::isfinite, std::isinf, std::lerp;

#define RA_IS_REAL(T) (std::numeric_limits<T>::is_integer || std::is_floating_point_v<T>)
#define RA_REAL_OVERLOAD_CE(T) template <class T> requires (RA_IS_REAL(T)) inline constexpr T
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
    inline constexpr T mul_conj(T const x, T const y)              { return x*y; } \
    inline constexpr T sqrm(T const x, T const y)                  { return sqrm(x-y); } \
    inline constexpr T dot(T const x, T const y)                   { return x*y; } \
    inline /* constexpr clang */ T fma_conj(T const a, T const b, T const c) { return fma(a, b, c); } \
    inline /* constexpr clang */ T norm2(T const x, T const y)     { return std::abs(x-y); } \
    inline /* constexpr clang */ T abs(T const x, T const y)       { return std::abs(x-y); } \
    inline /* constexpr clang */ T rel_error(T const a, T const b) { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*abs(a, b)/den; }
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT
