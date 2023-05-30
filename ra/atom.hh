// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Terminal nodes for expression templates.

// (c) Daniel Llorens - 2011-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <vector>
#include <utility>
#include "bootstrap.hh"


// --------------------
// error function
// --------------------

#include <cassert>
// If you define your own RA_ASSERT, you might remove this from here.
#include "format.hh"
#include <iostream>

// https://en.cppreference.com/w/cpp/preprocessor/replace
// See examples/throw.cc for how to override this RA_ASSERT.

#ifndef RA_ASSERT
#define RA_ASSERT(cond, ...)                                            \
    {                                                                   \
        if (std::is_constant_evaluated()) {                             \
            assert(cond /* FIXME maybe one day */);                     \
        } else {                                                        \
            if (bool c = cond; !c) [[unlikely]] {                       \
                std::cerr << ra::format("**** ra: ", ##__VA_ARGS__, " ****") << std::endl; \
                assert(c);                                              \
            }                                                           \
        }                                                               \
    }
#endif

#if defined(RA_DO_CHECK) && RA_DO_CHECK==0
  #define RA_CHECK( ... )
#else
  #define RA_CHECK( ... ) RA_ASSERT( __VA_ARGS__ )
#endif

// This is NOT included by format.hh to allow format() to be used in pre-defining RA_ASSERT.

#define RA_AFTER_CHECK Yes

namespace ra {


// --------------------
// global introspection I
// --------------------

template <class V>
requires (!std::is_void_v<V>)
constexpr dim_t
rank_s()
{
    if constexpr (requires { std::decay_t<V>::rank_s(); }) {
        return std::decay_t<V>::rank_s();
    } else if constexpr (requires { ra_traits<V>::rank_s(); }) {
        return ra_traits<V>::rank_s();
    } else {
        return 0;
    }
}

template <class V>
constexpr rank_t
rank_s(V const &)
{
    return rank_s<V>();
}

template <class V>
requires (!std::is_void_v<V>)
constexpr dim_t
size_s()
{
    if constexpr (requires { std::decay_t<V>::size_s(); }) {
        return std::decay_t<V>::size_s();
    } else if constexpr (requires { ra_traits<V>::size_s(); }) {
        return ra_traits<V>::size_s();
    } else {
        if constexpr (RANK_ANY==rank_s<V>()) {
            return DIM_ANY;
// make it work for non-registered types.
        } else if constexpr (0==rank_s<V>()) {
            return 1;
        } else {
            using V_ = std::decay_t<V>;
            dim_t s = 1;
            for (int i=0; i!=V_::rank_s(); ++i) {
                if (dim_t ss=V_::len_s(i); ss>=0) {
                    s *= ss;
                } else {
                    return ss; // either DIM_ANY or DIM_BAD
                }
            }
            return s;
        }
    }
}

template <class V>
constexpr dim_t
size_s(V const &)
{
    return size_s<V>();
}

template <class V>
constexpr rank_t
rank(V const & v)
{
    if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else if constexpr (requires { ra_traits<V>::rank(v); }) {
        return ra_traits<V>::rank(v);
    } else {
        static_assert(!std::is_same_v<V, V>, "No rank() for this type.");
    }
}

template <class V>
constexpr dim_t
size(V const & v)
{
    if constexpr (requires { v.size(); }) {
        return v.size();
    } else if constexpr (requires { ra_traits<V>::size(v); }) {
        return ra_traits<V>::size(v);
    } else {
        dim_t s = 1;
        for (rank_t k=0; k<rank(v); ++k) { s *= v.len(k); }
        return s;
    }
}

// Avoid, prefer implicit matching.
template <class V>
constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (requires { ra_traits<V>::shape(v); }) {
        return ra_traits<V>::shape(v);
    } else if constexpr (constexpr rank_t rs=rank_s<V>(); rs>=0) {
// FIXME Would prefer to return the map directly
        Small<dim_t, rs> s;
        for (rank_t k=0; k<rs; ++k) { s[k] = v.len(k); }
        return s;
    } else {
        static_assert(RANK_ANY==rs);
        rank_t r = v.rank();
        std::vector<dim_t> s(r);
        for (rank_t k=0; k<r; ++k) { s[k] = v.len(k); }
        return s;
    }
}

// To handle arrays of static/dynamic size.
template <class A>
inline void
resize(A & a, dim_t s)
{
    if constexpr (DIM_ANY==size_s<A>()) {
        a.resize(s);
    } else {
        RA_CHECK(s==dim_t(a.len_s(0)), "Bad resize ", s, " vs ", a.len_s(0));
    }
}


// --------------------
// atom types
// --------------------

// Iterator for rank 0 object. This can be used on foreign objects, or as an alternative to the rank conjunction.
// We still want f(C) to be a specialization in most cases (ie avoid ply(f, C) when C is rank 0).
template <class C>
struct Scalar
{
    template <class C_>
    struct Flat: public Scalar<C_>
    {
        constexpr void operator+=(dim_t d) const {}
        constexpr C_ & operator*() { return this->c; }
        constexpr C_ const & operator*() const { return this->c; } // [ra39]
    };

    C c;

    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k<0, "Bad axis k ", k); std::abort(); }
    constexpr static dim_t len(int k) { RA_CHECK(k<0, "Bad axis k ", k); std::abort(); }

    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t step(int k) { return 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() const { return static_cast<Flat<C> const &>(*this); } // [ra39]
    template <class J> constexpr decltype(auto) at(J && j) const { return c; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class C> constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

template <std::random_access_iterator I, dim_t N>
struct Ptr
{
    static_assert(N>=0 || N==DIM_BAD || N==DIM_ANY);

    I i;
    std::conditional_t<N==DIM_ANY, dim_t, mp::int_t<N>> n;

    constexpr Ptr(I i) requires (N!=DIM_ANY): i(i) {}
    constexpr Ptr(I i, dim_t n) requires (N==DIM_ANY): i(i), n(n) {}
    RA_DEF_ASSIGNOPS_SELF(Ptr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, "Bad axis k ", k); return N; }
    constexpr static dim_t len(int k) requires (N!=DIM_ANY) { RA_CHECK(k==0, "Bad axis k ", k); return N; }
    constexpr dim_t len(int k) const requires (N==DIM_ANY) { RA_CHECK(k==0, "Bad axis k ", k); return n; }

    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += step(k) * d; }
    constexpr auto flat() const { return i; }
    template <class J> decltype(auto) at(J && j) const { RA_CHECK(DIM_BAD==N || inside(j[0], len(0)), " j ", j[0], " size ", len(0)); return i[j[0]]; }
};

template <class I> constexpr auto ptr(I i) { return Ptr<I, DIM_BAD> { i }; }
template <class I, int N> constexpr auto ptr(I i, mp::int_t<N>) { return Ptr<I, N> { i }; }
template <class I> constexpr auto ptr(I i, dim_t n) { return Ptr<I, DIM_ANY> { i, n }; }

template <std::ranges::random_access_range V> constexpr auto
vector(V && v)
{
    constexpr dim_t ct_size = size_s<V>();
// forward preserves rvalueness of v as constness of iterator.
    if constexpr (DIM_ANY==ct_size) {
        return ptr(std::begin(std::forward<V>(v)), std::ssize(v));
    } else {
        return ptr(std::begin(std::forward<V>(v)), mp::int_t<ct_size> {});
    }
}

template <class T, int w=0, dim_t N=DIM_ANY, dim_t S=DIM_ANY>
struct Iota
{
    static_assert(w>=0);
    static_assert(N>=0 || N==DIM_BAD || N==DIM_ANY);

    using ntype = std::conditional_t<N==DIM_ANY, dim_t, mp::int_t<N>>;
    using stype = std::conditional_t<S==DIM_ANY, T, mp::int_t<S>>;

    T i = 0;
    ntype const n = {};
    stype const s = {};
    constexpr static T gets() requires (S!=DIM_ANY) { return S; }
    constexpr T gets() const requires (S==DIM_ANY) { return s; }
    constexpr decltype(auto) set(T const & ii) { i = ii; return *this; };

    struct Flat
    {
        T i;
        stype s;
        constexpr void operator+=(dim_t d) { i += T(d)*T(s); }
        constexpr auto operator*() const { return i; }
    };

    constexpr static rank_t rank_s() { return w+1; };
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k<=w, "Bad axis k ", k); return N; }
    constexpr static dim_t len(int k) requires (N!=DIM_ANY) { RA_CHECK(k<=w, "Bad axis k ", k); return N; }
    constexpr dim_t len(int k) const requires (N==DIM_ANY) { RA_CHECK(k<=w, "Bad axis k ", k); return n; }

    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += T(step(k) * d) * T(s); }
    constexpr auto flat() const { return Flat { i, s }; }
    template <class J> constexpr auto at(J && j) const { return i + T(j[w])*T(s); }
};

template <int w> using TensorIndex = Iota<dim_t, w, DIM_BAD, 1>;

#define DEF_TENSORINDEX(w) constexpr TensorIndex<w> JOIN(_, w) {};
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

constexpr auto iota() { return TensorIndex<0> {}; }

template <class T=dim_t>
constexpr auto
iota(dim_t len, T org=0)
{
    RA_CHECK(len>=0, "Bad iota length ", len);
    return Iota<T, 0, DIM_ANY, 1> { org, len };
}

template <class O=dim_t, class S=O>
constexpr auto
iota(dim_t len, O org, S step)
{
    RA_CHECK(len>=0, "Bad iota length ", len);
    using T = std::common_type_t<O, S>;
    return Iota<T> { T(org), len, T(step) };
}


// --------------
// coerce potential Iterators
// --------------

template <class T>
constexpr void
start(T && t) { static_assert(!std::same_as<T, T>, "Type cannot be start()ed."); }

// undefined len iota (ti) is excluded from optimization and beating. This allows e.g. B = A(... ti ...).
// FIXME there's no need to exclude it from optimization (?)
template <class I> constexpr bool is_iota_ = false;
template <class T, dim_t N, dim_t S> requires (DIM_BAD!=N) constexpr bool is_iota_<Iota<T, 0, N, S>> = true;
RA_IS_DEF(is_iota, (is_iota_<A>))

template <class T> requires (is_foreign_vector<T>)
constexpr auto
start(T && t) { return ra::vector(std::forward<T>(t)); }

template <class T> requires (is_scalar<T>)
constexpr auto
start(T && t) { return ra::scalar(std::forward<T>(t)); }

template <class T>
constexpr auto
start(std::initializer_list<T> v) { return ptr(v.begin(), v.size()); }

// forward declare for Match; implemented in small.hh.
template <class T> requires (is_builtin_array<T>)
constexpr auto
start(T && t);

// neither cell_iterator_big nor cell_iterator_small will retain rvalues [ra4].
template <class T> requires (is_slice<T>)
constexpr auto
start(T && t) { return iter<0>(std::forward<T>(t)); }

RA_IS_DEF(is_ra_scalar, (std::same_as<A, Scalar<decltype(std::declval<A>().c)>>))

template <class T> requires (is_ra_scalar<T>)
constexpr decltype(auto)
start(T && t) { return std::forward<T>(t); }

// iterators need to be restarted on every use (eg ra::cross()) [ra35].
template <class T> requires (is_iterator<T> && !is_ra_scalar<T>)
constexpr auto
start(T && t) { return std::forward<T>(t); }


// --------------------
// global introspection II
// --------------------

// FIXME one of these is ET-generic and the other is slice only, so make up your mind.
// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using value_t = std::decay_t<decltype(*(start(std::declval<A>()).flat()))>;

} // namespace ra
