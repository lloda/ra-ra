
// (c) Daniel Llorens - 2005, 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file real.H
/// @brief Define real number type and related global definitions.

#pragma once
#include "ra/int_t.H"
#include <limits>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace ra {

template <class T=double> constexpr T EPS = std::numeric_limits<T>::epsilon();
constexpr double  ALINF = std::numeric_limits<double>::max();
constexpr double   PINF = std::numeric_limits<double>::infinity();
constexpr double   QNAN = std::numeric_limits<double>::quiet_NaN();
constexpr double     PI = 3.14159265358979323846264338327950288419716939937510582;
constexpr double    PI2 = PI/2.;
constexpr double   EXP1 = 2.71828182845904523536028747135266249775724709369995957;
constexpr double    TAU = 2*PI;
constexpr double   TTAU = TAU*2;
constexpr double   TAU6 = TAU/6;
constexpr double  TAU12 = TAU/12;
constexpr double   I4PI = 1./TTAU;
constexpr double     C0 = 2.99792458e8;
constexpr double     M0 = (4e-7)*PI;
constexpr double  ECHAR = 1.602176487e-19;
constexpr double  EMASS = 9.10938215e-31;
constexpr double     Z0 = 376.730313461;
constexpr double  LOG2E = 1.44269504088896340735992468100189213742664595415299;
constexpr double  LOGE2 = .693147180559945309417232121458176568075500134360255;
constexpr double GOLDEN = 1.61803398874989484820458683436563811772030917980576; // (1+√5)/2.
// constexpr clang
static const double     E0 = 1./(M0*C0*C0);
static const double  SQRT2 = sqrt(2.);
static const double ISQRT2 = 1/sqrt(2.);
static const double SQRTPI = sqrt(PI);
static const double   LNPI = log(PI);

}

// just as max() and min() are found for ra:: types w/o qualifying (through ADL) they should also be found for the POD types.
// besides, gcc still leaks cmath functions into the global namespace, so by default e.g. sqrt will be C double sqrt(double) and not the overload set.
using std::abs, std::max, std::min, std::fma, std::clamp, std::sqrt, std::pow, std::exp;
using std::swap, std::isfinite, std::isinf;

#define RA_IS_REAL(T) (std::numeric_limits<T>::is_integer || std::is_floating_point<T>::value)
#define RA_REAL_OVERLOAD_CE(T) template <class T> inline constexpr std::enable_if_t<RA_IS_REAL(T), T>
#define RA_REAL_OVERLOAD(T) template <class T> inline std::enable_if_t<RA_IS_REAL(T), T>
// As an array op; special definitions for rank 0.
RA_REAL_OVERLOAD_CE(T) arg(T const x) { return 0; }
RA_REAL_OVERLOAD_CE(T) amax(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) amin(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) sqr(T const x) { return x*x; }
RA_REAL_OVERLOAD_CE(T) real_part(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) imag_part(T const x) { return 0.; }
RA_REAL_OVERLOAD_CE(T) conj(T const x) { return x; }
RA_REAL_OVERLOAD_CE(T) sqrm(T const x) { return x*x; }
RA_REAL_OVERLOAD(T) norm2(T const x) { return std::abs(x); } // constexpr clang
#undef RA_REAL_OVERLOAD_CE
#undef RA_REAL_OVERLOAD
#undef RA_IS_REAL

#define FOR_FLOAT(T)                                                    \
    inline constexpr T mul_conj(T const x, T const y)            { return x*y; } \
    inline constexpr T sqrm(T const x, T const y)                { return sqrm(x-y); } \
    inline constexpr T dot(T const x, T const y)                 { return x*y; } \
    inline /* constexpr clang */ T fma_conj(T const a, T const b, T const c) { return fma(a, b, c); } /* constexpr clang */ \
    inline /* constexpr clang */ T norm2(T const x, T const y)               { return std::abs(x-y); } \
    inline /* constexpr clang */ T abs(T const x, T const y)                 { return std::abs(x-y); } \
    inline /* constexpr clang */ T rel_error(T const a, T const b)           { return (a==0. && b==0.) ? 0. : 2.*abs(a, b)/(abs(a)+abs(b)); }
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

namespace ra {
    inline constexpr double rad2deg(double const r)              { return r*(360./ra::TAU); }
    inline constexpr double deg2rad(double const d)              { return d*(ra::TAU/360.); }
} // namespace ra
