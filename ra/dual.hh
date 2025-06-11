// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Dual numbers for automatic differentiation.

// (c) Daniel Llorens - 2013-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// See VanderBergen2012, Berland2006. Generally about automatic differentiation:
// http://en.wikipedia.org/wiki/Automatic_differentiation
// From the Taylor expansion of f(a) or f(a, b)...
// f(a+εa') = f(a)+εa'f_a(a)
// f(a+εa', b+εb') = f(a, b)+ε[a'f_a(a, b) b'f_b(a, b)]

#pragma once
#include <cmath>
#include <iosfwd>
#include "base.hh"

namespace ra {

using std::abs, std::sqrt, std::fma;

template <class T>
struct Dual
{
    T re, du;

    constexpr static bool is_complex = requires (T & a) { []<class R>(std::complex<R> &){}(a); };
    template <class S> struct real_part { struct type {}; };
    template <class S> requires (is_complex) struct real_part<S> { using type = typename S::value_type; };
    using real_type = typename real_part<T>::type;

    constexpr Dual(T const & r, T const & d): re(r), du(d) {}
    constexpr Dual(T const & r): re(r), du(0.) {} // conversions are by default constants.
    constexpr Dual(real_type const & r) requires (is_complex): re(r), du(0.) {}
    constexpr Dual() {}

#define ASSIGNOPS(OP)                                              \
    constexpr Dual & operator JOIN(OP, =)(T const & r) { *this = *this OP r; return *this; } \
    constexpr Dual & operator JOIN(OP, =)(Dual const & r) { *this = *this OP r; return *this; } \
    constexpr Dual & operator JOIN(OP, =)(real_type const & r) requires (is_complex) { *this = *this OP r; return *this; }
    FOR_EACH(ASSIGNOPS, +, -, /, *)
#undef ASSIGNOPS
};

template <class A> concept is_dual = requires (A & a) { []<class T>(Dual<T> &){}(a); };

constexpr auto dual(is_dual auto const & r) { return r; }
template <class R> constexpr auto dual(R const & r) { return Dual<R> { r, 0. }; }

template <class R, class D>
constexpr auto dual(R const & r, D const & d)
{ return Dual<std::common_type_t<R, D>> { r, d }; }

constexpr auto operator*(is_dual auto const & a, is_dual auto const & b)
{ return dual(a.re*b.re, a.re*b.du + a.du*b.re); }

constexpr auto operator*(auto const & a, is_dual auto const & b)
{ return dual(a*b.re, a*b.du); }

constexpr auto operator*(is_dual auto const & a, auto const & b)
{ return dual(a.re*b, a.du*b); }

constexpr auto fma(is_dual auto const & a, is_dual auto const & b, is_dual auto const & c)
{ return dual(fma(a.re, b.re, c.re), fma(a.re, b.du, fma(a.du, b.re, c.du))); }

constexpr auto operator+(is_dual auto const & a, is_dual auto const & b)
{ return dual(a.re+b.re, a.du+b.du); }

constexpr auto operator+(auto const & a, is_dual auto const & b)
{ return dual(a+b.re, b.du); }

constexpr auto operator+(is_dual auto const & a, auto const & b)
{ return dual(a.re+b, a.du); }

constexpr auto operator-(is_dual auto const & a, is_dual auto const & b)
{ return dual(a.re-b.re, a.du-b.du); }

constexpr auto operator-(is_dual auto const & a, auto const & b)
{ return dual(a.re-b, a.du); }

constexpr auto operator-(auto const & a, is_dual auto const & b)
{ return dual(a-b.re, -b.du); }

constexpr auto operator-(is_dual auto const & a)
{ return dual(-a.re, -a.du); }

constexpr decltype(auto) operator+(is_dual auto const & a)
{ return a; }

constexpr auto sqr(is_dual auto const & a)
{ return a*a; }

constexpr auto inv(is_dual auto const & a)
{ auto i = 1./a.re; return dual(i, -a.du*sqr(i)); }

constexpr auto operator/(is_dual auto const & a, is_dual auto const & b)
{ return a*inv(b); }

constexpr auto operator/(is_dual auto const & a, auto const & b)
{ return a*inv(dual(b)); }

constexpr auto operator/(auto const & a, is_dual auto const & b)
{ return dual(a)*inv(b); }

constexpr auto cos(is_dual auto const & a)
{ return dual(cos(a.re), -sin(a.re)*a.du); }

constexpr auto sin(is_dual auto const & a)
{ return dual(sin(a.re), +cos(a.re)*a.du); }

constexpr auto cosh(is_dual auto const & a)
{ return dual(cosh(a.re), +sinh(a.re)*a.du); }

constexpr auto sinh(is_dual auto const & a)
{ return dual(sinh(a.re), +cosh(a.re)*a.du); }

constexpr auto tan(is_dual auto const & a)
{ auto c = cos(a.du); return dual(tan(a.re), a.du/(c*c)); }

constexpr auto exp(is_dual auto const & a)
{ return dual(exp(a.re), +exp(a.re)*a.du); }

constexpr auto pow(is_dual auto const & a, auto const & b)
{ return dual(pow(a.re, b), +b*pow(a.re, b-1)*a.du); }

constexpr auto log(is_dual auto const & a)
{  return dual(log(a.re), +a.du/a.re); }

constexpr auto sqrt(is_dual auto const & a)
{ return dual(sqrt(a.re), +a.du/(2.*sqrt(a.re))); }

constexpr auto abs(is_dual auto const & a)
{ return abs(a.re); }

constexpr bool isfinite(is_dual auto const & a)
{ return isfinite(a.re) && isfinite(a.du); }

constexpr auto xi(is_dual auto const & a)
{ return dual(xi(a.re), xi(a.du)); }

std::ostream &
operator<<(std::ostream & o, is_dual auto const & a)
{ return o << "[" << a.re << " " << a.du << "]"; }

std::istream &
operator>>(std::istream & i, is_dual auto & a)
{
    char s;
    i >> s;
    if (s!='[') {
        i.setstate(std::ios::failbit);
    } else {
        i >> a.re >> a.du >> s;
        if (s!=']') {
            i.setstate(std::ios::failbit);
        }
    }
    return i;
}

} // namespace ra
