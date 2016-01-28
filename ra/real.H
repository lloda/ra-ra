
// (c) Daniel Llorens - 2005, 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_REAL_H
#define RA_REAL_H

/// @file real.H
/// @brief Define real number type and related global definitions.

#include "ra/int_t.H"
#include <limits>
#include <cstdlib>
#include <cmath>
#include <algorithm>

using real = double;

// abs(int) is in ::, so why should I qualify abs(double)? also need max() and min() to be found for the POD types, as they are defined for others, and also found (through ADL).
using std::abs;
using std::max;
using std::min;
using std::isfinite;
using std::fma;
// using std::isinf;  // there's apparently an ::isinf already

#define IS_REAL(T) (std::numeric_limits<T>::is_integer || std::is_floating_point<T>::value)

// As an array op; special definitions for rank 0.
template <class T> inline enableifc_<IS_REAL(T), T> amax(T const x) { return x; }
template <class T> inline enableifc_<IS_REAL(T), T> amin(T const x) { return x; }

#undef IS_REAL

// @TODO see is_scalar in wedge.H>
template <class T, enableif_<std::is_integral<T>, int> =0> T sqr(T const x) { return x*x; }

inline constexpr real sqr(real const x)                 { return x*x; }
inline constexpr real real_part(real const x)           { return x; }
inline constexpr real imag_part(real const x)           { return 0.; }
inline constexpr real conj(real const x)                { return x; }
inline constexpr real mul_conj(real const x, real const y) { return x*y; }
inline constexpr real fma_conj(real const a, real const b, real const c) { return a*b + c; }

inline constexpr float sqrm(float const x)              { return x*x; }
inline constexpr real sqrm(real const x)                { return x*x; }
inline constexpr real sqrm(real const x, real const y)  { return sqrm(x-y); }
inline /* constexpr */ real norm2(real const x)               { return std::abs(x); }
inline /* constexpr */ real norm2(real const x, real const y) { return std::abs(x-y); }
inline /* constexpr */ real abs(real const x, real const y)   { return std::abs(x-y); }
inline constexpr real dot(real const x, real const y)   { return x*y; }
inline void swap(real & a, real & b)                    { std::swap(a, b); }

inline real rel_error(real const a, real const b)
{
    return (a==0. && b==0.) ? 0. : 2.*abs(a, b)/(abs(a)+abs(b));
}

real const    EPS = std::numeric_limits<real>::epsilon();
real const  ALINF = std::numeric_limits<real>::max();
real const   PINF = std::numeric_limits<real>::infinity();
real const   QNAN = std::numeric_limits<real>::quiet_NaN();

// These TAU, PI, PI2 are rounded down.
real const     PI = 3.14159265358979323846264338327950288419716939937510582;
real const    PI2 = 1.57079632679489661923132169163975144209858469968755291;
real const   EXP1 = 2.71828182845904523536028747135266249775724709369995957;
real const    TAU = 6.28318530717958647692528676655900576839433879875021164195;
real const   TTAU = 12.5663706143591729538505735331180115367886775975004232839;
real const    PI3 = 1.04719755119659774615421446109316762806572313312503527366;
real const    PI6 = .523598775598298873077107230546583814032861567;
/// 1/(4*pi).
real const   I4PI = .07957747154594766788444188168625718101722982287;
real const  SQRT2 = 1.41421356237309504880168872420969807856967187537694;
real const ISQRT2 = .707106781186547524400844362104849039284835937688474036588;
real const SQRTPI = 1.772453850905516027298167483341145182797549456122387;
real const   LNPI = 1.14472988584940017414342735135305871164729481291531157151;
real const     C0 = 2.99792458e8;
real const     E0 = 8.854187817e-12;
real const     M0 = (4e-7)*PI;
/// Impedance of vacuum.
real const     Z0 = 376.730313461;
real const  LOG2E = 1.44269504088896340735992468100189213742664595415299;
real const  LOGE2 = .693147180559945309417232121458176568075500134360255;
/// (1+sqrt(5))/2.
real const GOLDEN = 1.61803398874989484820458683436563811772030917980576;
real const  ECHAR = 1.602176487e-19;
real const  EMASS = 9.10938215e-31;

inline real frand() { return real(random())/RAND_MAX; }
inline int irand(int const p) { return int(frand()*p); }
inline real rad2deg(real const r) { return r*(180./PI); }
inline real deg2rad(real const d) { return d*(PI/180.); }

#endif // RA_REAL_H
