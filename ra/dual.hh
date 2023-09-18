// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Dual numbers for automatic differentiation.

// (c) Daniel Llorens - 2013-2023
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
#include "macros.hh"

namespace ra {

using std::abs, std::sqrt, std::fma;

template <class T>
struct Dual
{
    T re, du;

    constexpr static bool is_complex = requires { requires !(std::is_same_v<T, std::decay_t<decltype(std::declval<T>().real())>>); };
    template <class S> struct real_part { struct type {}; };
    template <class S> requires (is_complex) struct real_part<S> { using type = typename S::value_type; };
    using real_type = typename real_part<T>::type;

    constexpr Dual(T const & r, T const & d): re(r), du(d) {}
    constexpr Dual(T const & r): re(r), du(0.) {} // conversions are by default constants.
    constexpr Dual(real_type const & r) requires (is_complex): re(r), du(0.) {}
    constexpr Dual() {}

#define DEF_ASSIGNOPS(OP)                                              \
    constexpr Dual & operator JOIN(OP, =)(T const & r) { *this = *this OP r; return *this; } \
    constexpr Dual & operator JOIN(OP, =)(Dual const & r) { *this = *this OP r; return *this; } \
    constexpr Dual & operator JOIN(OP, =)(real_type const & r) requires (is_complex) { *this = *this OP r; return *this; }
    FOR_EACH(DEF_ASSIGNOPS, +, -, /, *)
#undef DEF_ASSIGNOPS
};

// conversions are by default constants.
template <class R> constexpr auto dual(Dual<R> const & r) { return r; }
template <class R> constexpr auto dual(R const & r) { return Dual<R> { r, 0. }; }

template <class R, class D>
constexpr auto
dual(R const & r, D const & d)
{
    return Dual<std::common_type_t<R, D>> { r, d };
}

template <class A, class B>
constexpr auto
operator*(Dual<A> const & a, Dual<B> const & b)
{
    return dual(a.re*b.re, a.re*b.du + a.du*b.re);
}
template <class A, class B>
constexpr auto
operator*(A const & a, Dual<B> const & b)
{
    return dual(a*b.re, a*b.du);
}
template <class A, class B>
constexpr auto
operator*(Dual<A> const & a, B const & b)
{
    return dual(a.re*b, a.du*b);
}

template <class A, class B, class C>
constexpr auto
fma(Dual<A> const & a, Dual<B> const & b, Dual<C> const & c)
{
    return dual(::fma(a.re, b.re, c.re), ::fma(a.re, b.du, ::fma(a.du, b.re, c.du))); // FIXME shouldn't need ::
}

template <class A, class B>
constexpr auto
operator+(Dual<A> const & a, Dual<B> const & b)
{
    return dual(a.re+b.re, a.du+b.du);
}
template <class A, class B>
constexpr auto
operator+(A const & a, Dual<B> const & b)
{
    return dual(a+b.re, b.du);
}
template <class A, class B>
constexpr auto
operator+(Dual<A> const & a, B const & b)
{
    return dual(a.re+b, a.du);
}

template <class A, class B>
constexpr auto
operator-(Dual<A> const & a, Dual<B> const & b)
{
    return dual(a.re-b.re, a.du-b.du);
}
template <class A, class B>
constexpr auto
operator-(Dual<A> const & a, B const & b)
{
    return dual(a.re-b, a.du);
}
template <class A, class B>
constexpr auto
operator-(A const & a, Dual<B> const & b)
{
    return dual(a-b.re, -b.du);
}

template <class A>
constexpr auto
operator-(Dual<A> const & a)
{
    return dual(-a.re, -a.du);
}

template <class A>
constexpr decltype(auto)
operator+(Dual<A> const & a)
{
    return a;
}

template <class A>
constexpr auto
inv(Dual<A> const & a)
{
    auto i = 1./a.re;
    return dual(i, -a.du*(i*i));
}

template <class A, class B>
constexpr auto
operator/(Dual<A> const & a, Dual<B> const & b)
{
    return a*inv(b);
}

template <class A, class B>
constexpr auto
operator/(Dual<A> const & a, B const & b)
{
    return a*inv(dual(b));
}

template <class A, class B>
constexpr auto
operator/(A const & a, Dual<B> const & b)
{
    return dual(a)*inv(b);
}

template <class A>
constexpr auto
cos(Dual<A> const & a)
{
    return dual(cos(a.re), -sin(a.re)*a.du);
}

template <class A>
constexpr auto
sin(Dual<A> const & a)
{
    return dual(sin(a.re), +cos(a.re)*a.du);
}

template <class A>
constexpr auto
cosh(Dual<A> const & a)
{
    return dual(cosh(a.re), +sinh(a.re)*a.du);
}

template <class A>
constexpr auto
sinh(Dual<A> const & a)
{
    return dual(sinh(a.re), +cosh(a.re)*a.du);
}

template <class A>
constexpr auto
tan(Dual<A> const & a)
{
    auto c = cos(a.du);
    return dual(tan(a.re), a.du/(c*c));
}

template <class A>
constexpr auto
exp(Dual<A> const & a)
{
    return dual(exp(a.re), +exp(a.re)*a.du);
}

template <class A, class B>
constexpr auto
pow(Dual<A> const & a, B const & b)
{
    return dual(pow(a.re, b), +b*pow(a.re, b-1)*a.du);
}

template <class A>
constexpr auto
log(Dual<A> const & a)
{
    return dual(log(a.re), +a.du/a.re);
}

template <class A>
constexpr auto
sqrt(Dual<A> const & a)
{
    return dual(sqrt(a.re), +a.du/(2.*sqrt(a.re)));
}

template <class A>
constexpr auto
sqr(Dual<A> const & a)
{
    return a*a;
}

template <class A>
constexpr auto
abs(Dual<A> const & a)
{
    return abs(a.re);
}

template <class A>
constexpr bool
isfinite(Dual<A> const & a)
{
    return isfinite(a.re) && isfinite(a.du);
}

template <class A>
constexpr auto
xI(Dual<A> const & a)
{
    return dual(xI(a.re), xI(a.du));
}

template <class A>
std::ostream & operator<<(std::ostream & o, Dual<A> const & a)
{
    return o << "[" << a.re << " " << a.du << "]";
}

template <class A>
std::istream & operator>>(std::istream & i, Dual<A> & a)
{
    std::string s;
    i >> s;
    if (s!="[") {
        i.setstate(std::ios::failbit);
        return i;
    }
    a >> a.re;
    a >> a.du;
    i >> s;
    if (s!="]") {
        i.setstate(std::ios::failbit);
        return i;
    }
}

} // namespace ra
