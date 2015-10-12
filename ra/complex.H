
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

template <class T>
struct is_scalar_def<std::complex<T> >
{
    static bool const value = true;
};

} // namespace ra

using complex = std::complex<double>;

inline real real_part(complex const z)               { return z.real(); }
inline real imag_part(complex const z)               { return z.imag(); }
inline real sqrm(complex const x)                    { return sqrm(x.real())+sqrm(x.imag()); }
inline real sqrm(complex const x, complex const y)   { return sqrm(x.real()-y.real())+sqrm(x.imag()-y.imag()); }
inline real norm2(complex const x)                   { return hypot(x.real(), x.imag()); }
inline real norm2(complex const x, complex const y)  { return sqrt(sqrm(x, y)); }
inline real abs(complex const x, complex const y)    { return sqrt(sqrm(x, y)); }
inline complex sqr(complex const x)                  { return x*x; }
inline complex dot(complex const x, complex const y) { return x*y; }
inline complex xI(real const x)                      { return complex(0, x); }
inline complex xI(complex const z)                   { return complex(-z.imag(), z.real()); }
inline complex fma(complex const & a, complex const & b, complex const & c)
{
    return complex(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())),
                   fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag())));
}

// conj(a) * b + c
inline complex fma_conj(complex const & a, complex const & b, complex const & c)
{
    return complex(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())),
                   fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag())));
}

// conj(a) * b
inline complex mul_conj(complex const & a, complex const & b)
{
    return complex(+a.real()*b.real()+a.imag()*b.imag(),
                   a.real()*b.imag()-a.imag()*b.real());
}
inline bool isfinite(complex const z)
{
    return std::isfinite(z.real()) && std::isfinite(z.imag());
}
inline bool isnan(complex const z)
{
    return std::isnan(z.real()) || std::isnan(z.imag());
}
inline bool isinf(complex const z)
{
    bool const a = std::isinf(z.real());
    bool const b = std::isinf(z.imag());
    return (a && b) || (a && std::isfinite(z.imag())) || (b && std::isfinite(z.real()));
}
inline void swap(complex & a, complex & b)
{
    std::swap(a, b);
}
inline complex tanh(complex const z)
{
    return (z.real()>300.) ? 1. : ((z.real()<-300.) ? -1. : sinh(z)/cosh(z));
}
inline real rel_error(complex const a, complex const b)
{
    return (a==0. && b==0.) ? 0. : 2.*abs(a, b)/(abs(a)+abs(b));
}

#endif // RA_COMPLEX_H
