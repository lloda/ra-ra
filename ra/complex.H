
// (c) Daniel Llorens - 2005, 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_COMPLEX_H
#define RA_COMPLEX_H

/// @file complex.H
/// @brief Defines the complex type and standard operations on it.

#include <complex>
#include "ra/real.H"
#include "ra/ra-type.H"

namespace ra {

template <class T> constexpr bool is_scalar_def<std::complex<T> > = true;

} // namespace ra

#define RA_REAL double
#define RA_CPLX std::complex<RA_REAL>

inline constexpr RA_REAL real_part(RA_CPLX && z)       { return z.real(); }
inline constexpr RA_REAL imag_part(RA_CPLX && z)       { return z.imag(); }
inline constexpr RA_REAL real_part(RA_CPLX const & z)  { return z.real(); }
inline constexpr RA_REAL imag_part(RA_CPLX const & z)  { return z.imag(); }
inline RA_REAL & real_part(RA_CPLX & z)                { return reinterpret_cast<RA_REAL *>(&z)[0]; }
inline RA_REAL & imag_part(RA_CPLX & z)                { return reinterpret_cast<RA_REAL *>(&z)[1]; }
inline RA_REAL sqrm(RA_CPLX const x)                   { return sqrm(x.real())+sqrm(x.imag()); }
inline RA_REAL sqrm(RA_CPLX const x, RA_CPLX const y)  { return sqrm(x.real()-y.real())+sqrm(x.imag()-y.imag()); }
inline RA_REAL norm2(RA_CPLX const x)                  { return hypot(x.real(), x.imag()); }
inline RA_REAL norm2(RA_CPLX const x, RA_CPLX const y) { return sqrt(sqrm(x, y)); }
inline RA_REAL abs(RA_CPLX const x, RA_CPLX const y)   { return sqrt(sqrm(x, y)); }
inline RA_CPLX sqr(RA_CPLX const x)                    { return x*x; }
inline RA_CPLX dot(RA_CPLX const x, RA_CPLX const y)   { return x*y; }
inline constexpr RA_CPLX xI(RA_REAL const x)           { return RA_CPLX(0, x); }
inline constexpr RA_CPLX xI(RA_CPLX const z)           { return RA_CPLX(-z.imag(), z.real()); }
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
    return RA_CPLX(+a.real()*b.real()+a.imag()*b.imag(),
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

#endif // RA_COMPLEX_H
