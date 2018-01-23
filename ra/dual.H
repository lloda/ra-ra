
// (c) Daniel Llorens - 2013, 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file dual.H
/// @brief Dual numbers for automatic differentiation.

// I used VanderBergen2012, Berland2006. Generally about automatic differentiation:
// http://en.wikipedia.org/wiki/Automatic_differentiation
// From the Taylor expansion of f(a) or f(a, b)...
// f(a+εa') = f(a)+εa'f_a(a)
// f(a+εa', b+εb') = f(a, b)+ε[a'f_a(a, b) b'f_b(a, b)]

#pragma once
#include <iosfwd>
#include "ra/macros.H"
#include <cmath>

using std::abs; using std::sqrt; using std::fma;

template <class T>
struct Dual
{
    T re, du;

    template <class S, class Enable=void>
    struct real_part { struct type {}; };

    template <class S>
    struct real_part<S, std::enable_if_t<!(std::is_same_v<S, std::decay_t<decltype(std::declval<S>().real())>>)>>
    {
        using type = typename S::value_type;
    };

    Dual(T const & r, T const & d): re(r), du(d) {}
    Dual(T const & r): re(r), du(0.) {} // conversions are by default constants.
    Dual(typename real_part<T>::type const & r): re(r), du(0.) {}
    Dual() {}

#define DEF_ASSIGN_OPS(OP)                                              \
    Dual & operator JOIN(OP, =)(T const & r) { *this = *this OP r; return *this; } \
    Dual & operator JOIN(OP, =)(Dual const & r) { *this = *this OP r; return *this; } \
    Dual & operator JOIN(OP, =)(typename real_part<T>::type const & r) { *this = *this OP r; return *this; }
    FOR_EACH(DEF_ASSIGN_OPS, +, -, /, *)
#undef DEF_ASSIGN_OPS
};

// conversions are by default constants.
template <class R> auto dual(Dual<R> const & r) { return r; }
template <class R> auto dual(R const & r) { return Dual<R> { r, 0. }; }

template <class R, class D>
auto dual(R const & r, D const & d)
{
    return Dual<std::common_type_t<R, D>> { r, d };
}

template <class A, class B>
auto operator*(Dual<A> const & a, Dual<B> const & b)
{
    return dual(a.re*b.re, a.re*b.du + a.du*b.re);
}
template <class A, class B>
auto operator*(A const & a, Dual<B> const & b)
{
    return dual(a*b.re, a*b.du);
}
template <class A, class B>
auto operator*(Dual<A> const & a, B const & b)
{
    return dual(a.re*b, a.du*b);
}

template <class A, class B, class C>
auto fma(Dual<A> const & a, Dual<B> const & b, Dual<C> const & c)
{
    return dual(fma(a.re, b.re, c.re), fma(a.re, b.du, fma(a.du, b.re, c.du)));
}

template <class A, class B>
auto operator+(Dual<A> const & a, Dual<B> const & b)
{
    return dual(a.re+b.re, a.du+b.du);
}
template <class A, class B>
auto operator+(A const & a, Dual<B> const & b)
{
    return dual(a+b.re, b.du);
}
template <class A, class B>
auto operator+(Dual<A> const & a, B const & b)
{
    return dual(a.re+b, a.du);
}

template <class A, class B>
auto operator-(Dual<A> const & a, Dual<B> const & b)
{
    return dual(a.re-b.re, a.du-b.du);
}
template <class A, class B>
auto operator-(Dual<A> const & a, B const & b)
{
    return dual(a.re-b, a.du);
}
template <class A, class B>
auto operator-(A const & a, Dual<B> const & b)
{
    return dual(a-b.re, -b.du);
}

template <class A>
auto operator-(Dual<A> const & a)
{
    return dual(-a.re, -a.du);
}

template <class A>
decltype(auto) operator+(Dual<A> const & a)
{
    return a;
}

template <class A>
auto inv(Dual<A> const & a)
{
    auto i = 1./a.re;
    return dual(i, -a.du*(i*i));
}

template <class A, class B>
auto operator/(Dual<A> const & a, Dual<B> const & b)
{
    return a*inv(b);
}

template <class A, class B>
auto operator/(Dual<A> const & a, B const & b)
{
    return a*inv(dual(b));
}

template <class A, class B>
auto operator/(A const & a, Dual<B> const & b)
{
    return dual(a)*inv(b);
}

template <class A>
auto cos(Dual<A> const & a)
{
    return dual(cos(a.re), -sin(a.re)*a.du);
}

template <class A>
auto sin(Dual<A> const & a)
{
    return dual(sin(a.re), +cos(a.re)*a.du);
}

template <class A>
auto cosh(Dual<A> const & a)
{
    return dual(cosh(a.re), +sinh(a.re)*a.du);
}

template <class A>
auto sinh(Dual<A> const & a)
{
    return dual(sinh(a.re), +cosh(a.re)*a.du);
}

template <class A>
auto tan(Dual<A> const & a)
{
    auto c = cos(a.du);
    return dual(tan(a.re), a.du/(c*c));
}

template <class A>
auto exp(Dual<A> const & a)
{
    return dual(exp(a.re), +exp(a.re)*a.du);
}

template <class A, class B>
auto pow(Dual<A> const & a, B const & b)
{
    return dual(pow(a.re, b), +b*pow(a.re, b-1)*a.du);
}

template <class A>
auto log(Dual<A> const & a)
{
    return dual(log(a.re), +a.du/a.re);
}

template <class A>
auto sqrt(Dual<A> const & a)
{
    return dual(sqrt(a.re), +a.du/(2.*sqrt(a.re)));
}

template <class A>
auto sqr(Dual<A> const & a)
{
    return a*a;
}

template <class A>
auto abs(Dual<A> const & a)
{
    return abs(a.re);
}

template <class A>
bool isfinite(Dual<A> const & a)
{
    return isfinite(a.re) && isfinite(a.du);
}

template <class A>
auto xI(Dual<A> const & a)
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
