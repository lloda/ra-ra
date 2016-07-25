
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

// abs(int) is in ::, so why should I qualify abs(double)? also need max() and min() to be found for the POD types, as they are defined for others, and also found (through ADL).
using std::abs;
using std::max;
using std::min;
using std::isfinite;
using std::fma;
// using std::isinf;  // there's apparently an ::isinf already

#define IS_REAL(T) (std::numeric_limits<T>::is_integer || std::is_floating_point<T>::value)

// As an array op; special definitions for rank 0.
template <class T> inline std::enable_if_t<IS_REAL(T), T> amax(T const x) { return x; }
template <class T> inline std::enable_if_t<IS_REAL(T), T> amin(T const x) { return x; }

#undef IS_REAL

// @TODO see is_scalar in wedge.H>
template <class T, std::enable_if_t<std::is_integral<T>::value, int> =0> T sqr(T const x) { return x*x; }

inline constexpr double sqr(double const x)                         { return x*x; }
inline constexpr double real_part(double const x)                   { return x; }
inline constexpr double imag_part(double const x)                   { return 0.; }
inline constexpr double conj(double const x)                        { return x; }
inline constexpr double mul_conj(double const x, double const y)    { return x*y; }
inline constexpr double fma_conj(double const a, double const b, double const c) { return a*b + c; }

inline constexpr float sqrm(float const x)                          { return x*x; }
inline constexpr double sqrm(double const x)                        { return x*x; }
inline constexpr double sqrm(double const x, double const y)        { return sqrm(x-y); }
inline /* constexpr */ double norm2(double const x)                 { return std::abs(x); }
inline /* constexpr */ double norm2(double const x, double const y) { return std::abs(x-y); }
inline /* constexpr */ double abs(double const x, double const y)   { return std::abs(x-y); }
inline constexpr double dot(double const x, double const y)         { return x*y; }
inline void swap(double & a, double & b)                            { std::swap(a, b); }

inline double rel_error(double const a, double const b)
{
    return (a==0. && b==0.) ? 0. : 2.*abs(a, b)/(abs(a)+abs(b));
}

double const    EPS = std::numeric_limits<double>::epsilon();
double const  ALINF = std::numeric_limits<double>::max();
double const   PINF = std::numeric_limits<double>::infinity();
double const   QNAN = std::numeric_limits<double>::quiet_NaN();

double const     PI = 3.14159265358979323846264338327950288419716939937510582;
double const    PI2 = PI/2.;
double const   EXP1 = 2.71828182845904523536028747135266249775724709369995957;
double const    TAU = 2*PI;
double const   TTAU = TAU*2;
double const   TAU6 = TAU/6;
double const  TAU12 = TAU/12;
// 1/(4*pi).
double const   I4PI = 1./TTAU;
double const  SQRT2 = sqrt(2.);
double const ISQRT2 = 1/sqrt(2.);
double const SQRTPI = sqrt(PI);
double const   LNPI = log(PI);
double const     C0 = 2.99792458e8;
double const     M0 = (4e-7)*PI;
double const     E0 = 1./(sqrt(M0)*C0*C0);
double const  ECHAR = 1.602176487e-19;
double const  EMASS = 9.10938215e-31;
// Impedance of vacuum.
double const     Z0 = 376.730313461;
double const  LOG2E = 1.44269504088896340735992468100189213742664595415299;
double const  LOGE2 = .693147180559945309417232121458176568075500134360255;
// (1+sqrt(5))/2.
double const GOLDEN = 1.61803398874989484820458683436563811772030917980576;

inline double frand() { return double(random())/RAND_MAX; }
inline int irand(int const p) { return int(frand()*p); }
inline double rad2deg(double const r) { return r*(180./PI); }
inline double deg2rad(double const d) { return d*(PI/180.); }

#endif // RA_REAL_H
