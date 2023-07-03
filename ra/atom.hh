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


// --------------------
// error function
// --------------------

#include <cassert>
#include "bootstrap.hh"
// If you define your own RA_ASSERT, you might remove this from here.
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
    using dV = std::decay_t<V>;
    if constexpr (requires { dV::rank_s(); }) {
        return dV::rank_s();
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
    using dV = std::decay_t<V>;
    if constexpr (requires { dV::size_s(); }) {
        return dV::size_s();
    } else if constexpr (requires { ra_traits<V>::size_s(); }) {
        return ra_traits<V>::size_s();
    } else {
        if constexpr (RANK_ANY==rank_s<V>()) {
            return DIM_ANY;
// make it work for non-registered types.
        } else if constexpr (0==rank_s<V>()) {
            return 1;
        } else {
            dim_t s = 1;
            for (int i=0; i!=dV::rank_s(); ++i) {
                if (dim_t ss=dV::len_s(i); ss>=0) {
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

// To avoid, prefer implicit matching.
template <class V>
constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (requires { ra_traits<V>::shape(v); }) {
        return ra_traits<V>::shape(v);
    } else if constexpr (constexpr rank_t rs=rank_s<V>(); rs>=0) {
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

// IteratorConcept for rank 0 object. This can be used on foreign objects, or as an alternative to the rank conjunction.
// We still want f(C) to be a specialization in most cases (ie avoid ply(f, C) when C is rank 0).
template <class C>
struct Scalar
{
    C c;

    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k<0, "Bad axis ", k); std::abort(); }
    constexpr static dim_t len(int k) { RA_CHECK(k<0, "Bad axis ", k); std::abort(); }

    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t step(int k) { return 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() const { return *this; } // [ra39]
    constexpr decltype(auto) at(auto && j) const { return c; }

// use self as Flat
    constexpr void operator+=(dim_t d) const {}
    constexpr C & operator*() { return this->c; }
    constexpr C const & operator*() const { return this->c; } // [ra39]
};

template <class C> constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

// IteratorConcept for foreign rank 1 objects.
template <std::random_access_iterator I, dim_t N>
struct Ptr
{
    static_assert(N>=0 || N==DIM_BAD || N==DIM_ANY);

    I i;
    [[no_unique_address]] std::conditional_t<N==DIM_ANY, dim_t, int_c<N>> n;

    constexpr Ptr(I i) requires (N!=DIM_ANY): i(i) {}
    constexpr Ptr(I i, dim_t n) requires (N==DIM_ANY): i(i), n(n) {}
    RA_DEF_ASSIGNOPS_SELF(Ptr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, "Bad axis ", k); return N; }
    constexpr static dim_t len(int k) requires (N!=DIM_ANY) { RA_CHECK(k==0, "Bad axis ", k); return N; }
    constexpr dim_t len(int k) const requires (N==DIM_ANY) { RA_CHECK(k==0, "Bad axis ", k); return n; }

    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += step(k) * d; }
    constexpr auto flat() const { return i; }
    constexpr decltype(auto) at(auto && j) const
    {
        RA_CHECK(DIM_BAD==N || inside(j[0], len(0)), " j ", j[0], " size ", len(0));
        return i[j[0]];
    }
};

template <class I> constexpr auto ptr(I i) { return Ptr<I, DIM_BAD> { i }; }
template <class I, int N> constexpr auto ptr(I i, int_c<N>) { return Ptr<I, N> { i }; }
template <class I> constexpr auto ptr(I i, dim_t n) { return Ptr<I, DIM_ANY> { i, n }; }

template <std::ranges::random_access_range V> constexpr auto
vector(V && v)
{
    if constexpr (constexpr dim_t s = size_s<V>(); DIM_ANY==s) {
        return ptr(std::begin(std::forward<V>(v)), std::ssize(v));
    } else {
        return ptr(std::begin(std::forward<V>(v)), int_c<s> {});
    }
}

// Sequence and IteratorConcept for same.
// FIXME Sequence should be its own type, we can't represent a ct origin bc IteratorConcept interface takes up i.
template <int w, class O, class N, class S>
struct Iota
{
    constexpr static dim_t nn = [] { if constexpr (is_constant<N>) { return N::value; } else { return DIM_ANY; } }();
    constexpr static dim_t ss = [] { if constexpr (is_constant<S>) { return S::value; } else { return DIM_ANY; } }();

    static_assert(w>=0);
    static_assert(is_constant<N> || 0==rank_s<N>());
    static_assert(is_constant<S> || 0==rank_s<S>());
    static_assert(nn>=0 || nn==DIM_BAD || (!is_constant<N> && nn==DIM_ANY)); // forbid N dim_c<DIM_ANY>

    O i = {};
    [[no_unique_address]] N const n = {};
    [[no_unique_address]] S const s = {};

    constexpr static O gets() requires (is_constant<S>) { return ss; }
    constexpr O gets() const requires (!is_constant<S>) { return s; }
    constexpr decltype(auto) set(O const & ii) { i = ii; return *this; };

    struct Flat
    {
        O i;
        S s;
        constexpr void operator+=(dim_t d) { i += O(d)*O(s); }
        constexpr auto operator*() const { return i; }
    };

    constexpr static rank_t rank_s() { return w+1; };
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k<=w, "Bad axis", k); return k==w ? nn : DIM_BAD; }
    constexpr static dim_t len(int k) requires (nn!=DIM_ANY) { RA_CHECK(k<=w, "Bad axis ", k); return k==w ? nn : DIM_BAD; }
    constexpr dim_t len(int k) const requires (nn==DIM_ANY) { RA_CHECK(k<=w, "Bad axis ", k); return k==w ? n : DIM_BAD; }

    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += O(step(k) * d) * O(s); }
    constexpr auto flat() const { return Flat { i, s }; }
    constexpr auto at(auto && j) const { return i + O(j[w])*O(s); }
};

template <int w> using TensorIndex = Iota<w, dim_t, dim_c<DIM_BAD>, dim_c<1>>;

template <class T>
constexpr auto
default_one()
{
    if constexpr (std::is_integral_v<T>) {
        return T(1);
    } else if constexpr (is_constant<T>) {
        static_assert(1==T::value);
        return T {};
    }
}

template <int w=0, class O=dim_t, class N=dim_c<DIM_BAD>, class S=dim_c<1>>
constexpr auto
iota(N && n = N {}, O && org = 0, S && s = default_one<S>())
{
    if constexpr (std::is_integral_v<N>) {
        RA_CHECK(n>=0, "Bad iota length ", n);
    }
    using OO = std::conditional_t<is_constant<std::decay_t<O>> || is_scalar<std::decay_t<O>>, std::decay_t<O>, O>;
    using NN = std::conditional_t<is_constant<std::decay_t<N>> || is_scalar<std::decay_t<N>>, std::decay_t<N>, N>;
    using SS = std::conditional_t<is_constant<std::decay_t<S>> || is_scalar<std::decay_t<S>>, std::decay_t<S>, S>;
    return Iota<w, OO, NN, SS> { std::forward<O>(org), std::forward<N>(n), std::forward<S>(s) };
}

#define DEF_TENSORINDEX(w) constexpr TensorIndex<w> JOIN(_, w);
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX


// --------------
// coerce potential Iterators
// --------------

template <class T>
constexpr void
start(T && t) { static_assert(mp::always_false<T>, "Type cannot be start()ed."); }

// undefined len iota (ti) is excluded from optimization and beating to allow e.g. B = A(... ti ...).
// FIXME find a way?
RA_IS_DEF(is_iota, false)
template <class O, class N, class S>
constexpr bool is_iota_def<Iota<0, O, N, S>> = (DIM_BAD != Iota<0, O, N, S>::nn);

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

// neither CellBig nor CellSmall will retain rvalues [ra4].
template <class T> requires (is_slice<T>)
constexpr auto
start(T && t) { return iter<0>(std::forward<T>(t)); }

RA_IS_DEF(is_ra_scalar, (std::same_as<A, Scalar<decltype(std::declval<A>().c)>>))

template <class T> requires (is_ra_scalar<T>)
constexpr decltype(auto)
start(T && t) { return std::forward<T>(t); }

// iterators need to be restarted on each use (eg ra::cross()) [ra35].
template <class T> requires (is_iterator<T> && !is_ra_scalar<T>)
constexpr auto
start(T && t) { return std::forward<T>(t); }


// --------------------
// global introspection II
// --------------------

// also used to paper over Scalar<X> vs X
template <class A>
constexpr decltype(auto)
FLAT(A && a)
{
    if constexpr (is_scalar<A>) {
        return std::forward<A>(a); // avoid dangling temp in this case [ra8]
    } else {
        return *(ra::start(std::forward<A>(a)).flat());
    }
}

// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using value_t = std::decay_t<decltype(FLAT(std::declval<A>()))>;

} // namespace ra
