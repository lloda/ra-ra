// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Terminal nodes for expression templates + prefix matching.

// (c) Daniel Llorens - 2011-2023
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

constexpr bool inside(dim_t i, dim_t b) { return i>=0 && i<b; }
constexpr bool inside(dim_t i, dim_t a, dim_t b) { return i>=a && i<b; }


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

template <class V> constexpr rank_t rank_s(V const &) { return rank_s<V>(); }

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

template <class V> constexpr dim_t size_s(V const &) { return size_s<V>(); }

template <class V>
constexpr rank_t
rank(V const & v)
{
    if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else if constexpr (requires { ra_traits<V>::rank(v); }) {
        return ra_traits<V>::rank(v);
    } else {
        static_assert(mp::always_false<V>, "No rank() for this type.");
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

// Avoid using for matching or agreement check.
// operator<< depends on this returning a concrete type.
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
        RA_CHECK(s==dim_t(a.len_s(0)), "Bad resize ", s, " vs ", a.len_s(0), ".");
    }
}


// --------------------
// terminal types
// --------------------

// IteratorConcept for rank 0 object. This can be used on foreign objects, or as an alternative to the rank conjunction.
// We still want f(C) to be a specialization in most cases (ie avoid ply(f, C) when C is rank 0).
template <class C>
struct Scalar
{
    C c;

    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }

    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t step(int k) { return 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() const { return *this; } // [ra39]
    constexpr decltype(auto) at(auto && j) const { return c; }

// use self as Flat
    constexpr void operator+=(dim_t d) const {}
    constexpr C & operator*() { return this->c; }
    constexpr C const & operator*() const { return this->c; } // [ra39]

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class C> constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

// IteratorConcept for foreign rank 1 objects.
template <std::random_access_iterator I, class N>
struct Ptr
{
    static_assert(is_constant<N> || 0==rank_s<N>());
    constexpr static dim_t nn = [] { if constexpr (is_constant<N>) { return N::value; } else { return DIM_ANY; } }();
    static_assert(nn>=0 || nn==DIM_BAD || (!is_constant<N> && nn==DIM_ANY));

    I i;
    [[no_unique_address]] N const n = {};

    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, "Bad axis ", k); return nn; }
    constexpr static dim_t len(int k) requires (nn!=DIM_ANY) { return len_s(k); }
    constexpr dim_t len(int k) const requires (nn==DIM_ANY) { RA_CHECK(k==0, "Bad axis ", k); return n; }

    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += step(k) * d; }
    constexpr auto flat() const { return i; }
    constexpr decltype(auto) at(auto && j) const
    {
        RA_CHECK(DIM_BAD==nn || inside(j[0], n), "Out of range ", j[0], " for length ", n, ".");
        return i[j[0]];
    }

    constexpr Ptr(I i, N n): i(i), n(n) {}
    RA_DEF_ASSIGNOPS_SELF(Ptr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class I, class N=dim_c<DIM_BAD>>
constexpr auto
ptr(I i, N && n = N {})
{
    if constexpr (std::is_integral_v<N>) {
        RA_CHECK(n>=0, "Bad ptr length ", n);
    }
    using NN = std::conditional_t<is_constant<std::decay_t<N>> || is_scalar<std::decay_t<N>>, std::decay_t<N>, N>;
    return Ptr<I, NN> { i, std::forward<N>(n) };
}

template <std::ranges::random_access_range V>
constexpr auto
vector(V && v)
{
    if constexpr (constexpr dim_t s = size_s<V>(); DIM_ANY==s) {
        return ptr(std::begin(std::forward<V>(v)), std::ssize(v));
    } else {
        return ptr(std::begin(std::forward<V>(v)), int_c<s> {});
    }
}

// Sequence and IteratorConcept for same. Iota isn't really a terminal, but its exprs must all have rank 0.
// FIXME Sequence should be its own type, we can't represent a ct origin bc IteratorConcept interface takes up i.
template <int w, class O, class N, class S>
struct Iota
{
    static_assert(w>=0);
    static_assert(is_constant<S> || 0==rank_s<S>());
    static_assert(is_constant<N> || 0==rank_s<N>());
    constexpr static dim_t nn = [] { if constexpr (is_constant<N>) { return N::value; } else { return DIM_ANY; } }();
    constexpr static dim_t ss = [] { if constexpr (is_constant<S>) { return S::value; } else { return DIM_ANY; } }();
    static_assert((!is_constant<N> && nn==DIM_ANY) || nn>=0 || nn==DIM_BAD);
    static_assert((!is_constant<S> && ss==DIM_ANY) || ss>=0);

    O i = {};
    [[no_unique_address]] N const n = {};
    [[no_unique_address]] S const s = {};

    constexpr static O gets() requires (is_constant<S>) { return ss; }
    constexpr O gets() const requires (!is_constant<S>) { return s; }

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
    constexpr static dim_t len(int k) requires (nn!=DIM_ANY) { return len_s(k); }
    constexpr dim_t len(int k) const requires (nn==DIM_ANY) { RA_CHECK(k<=w, "Bad axis ", k); return k==w ? n : DIM_BAD; }

    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += O(step(k) * d) * O(s); }
    constexpr auto flat() const { return Flat { i, s }; }
    constexpr auto at(auto && j) const
    {
        RA_CHECK(DIM_BAD==nn || inside(j[0], n), "Out of range ", j[0], " for length ", n, ".");
        return i + O(j[w])*O(s);
    }
};

template <class T>
constexpr auto
default_1()
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
iota(N && n = N {}, O && org = 0, S && s = default_1<S>())
{
    if constexpr (std::is_integral_v<N>) {
        RA_CHECK(n>=0, "Bad iota length ", n);
    }
    using OO = std::conditional_t<is_constant<std::decay_t<O>> || is_scalar<std::decay_t<O>>, std::decay_t<O>, O>;
    using NN = std::conditional_t<is_constant<std::decay_t<N>> || is_scalar<std::decay_t<N>>, std::decay_t<N>, N>;
    using SS = std::conditional_t<is_constant<std::decay_t<S>> || is_scalar<std::decay_t<S>>, std::decay_t<S>, S>;
    return Iota<w, OO, NN, SS> { std::forward<O>(org), std::forward<N>(n), std::forward<S>(s) };
}

#define DEF_TENSORINDEX(w) constexpr auto JOIN(_, w) = iota<w>();
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

// Never ply(), solely to be rewritten.
struct Len
{
    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static void adv(rank_t k, dim_t d) { std::abort(); }
    constexpr static dim_t step(int k) { std::abort(); }
    constexpr static bool keep_step(dim_t st, int z, int j) { std::abort(); }
    constexpr static Len const & flat() { std::abort(); }
    constexpr void operator+=(dim_t d) const { std::abort(); }
    constexpr dim_t operator*() const { std::abort(); }
};

constexpr Len len {};

// don't try to reduce operations with Len.
template <> constexpr bool is_special_def<Len> = true;
RA_IS_DEF(has_len, false);


// --------------
// coerce potential Iterators
// --------------

template <class T>
constexpr void
start(T && t) { static_assert(mp::always_false<T>, "Type cannot be start()ed."); }

RA_IS_DEF(is_iota, false)
// DIM_BAD is excluded from beating to allow B = A(... ti ...). FIXME find a way?
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


// --------------------
// prefix match
// --------------------

constexpr rank_t
choose_rank(rank_t ra, rank_t rb)
{
    return RANK_BAD==rb ? ra : RANK_BAD==ra ? rb : RANK_ANY==ra ? ra : RANK_ANY==rb ? rb : (ra>=rb ? ra : rb);
}

// TODO Allow infinite rank; need a special value of crank.
constexpr rank_t
dependent_cell_rank(rank_t rank, rank_t crank)
{
    return crank>=0 ? crank // not dependent
        : rank==RANK_ANY ? RANK_ANY // defer
        : (rank+crank);
}

constexpr rank_t
dependent_frame_rank(rank_t rank, rank_t crank)
{
    return rank==RANK_ANY ? RANK_ANY // defer
        : crank>=0 ? (rank-crank) // not dependent
        : -crank;
}

// if non-negative args don't match, pick first (see below). FIXME maybe return invalid.
constexpr dim_t
choose_len(dim_t sa, dim_t sb)
{
    return DIM_BAD==sa ? sb : DIM_BAD==sb ? sa : DIM_ANY==sa ? sb : sa;
}

template <bool checkp, class T, class K=mp::iota<mp::len<T>>> struct Match;

template <bool checkp, IteratorConcept ... P, int ... I>
struct Match<checkp, std::tuple<P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P ...>;
    T t;

    // 0: fail, 1: rt, 2: pass
    constexpr static int
    check_s()
    {
        if constexpr (sizeof...(P)<2) {
            return 2;
        } else if constexpr (RANK_ANY==rank_s()) {
            return 1; // FIXME could be tightened to 2 in some cases
        } else {
            bool tbc = false;
            for (int k=0; k<rank_s(); ++k) {
                dim_t ls = len_s(k);
                if (((k<std::decay_t<P>::rank_s() && ls!=choose_len(std::decay_t<P>::len_s(k), ls)) || ...)) {
                    return 0;
                } else {
                    int anyk = ((k<std::decay_t<P>::rank_s() && (DIM_ANY==std::decay_t<P>::len_s(k))) + ...);
                    int fixk = ((k<std::decay_t<P>::rank_s() && (0<=std::decay_t<P>::len_s(k))) + ...);
                    tbc = tbc || (anyk>0 && anyk+fixk>1);
                }
            }
            return tbc ? 1 : 2;
        }
    }

    constexpr bool
    check() const
    {
        if constexpr (sizeof...(P)<2) {
            return true;
        } else  if constexpr (constexpr int c = check_s(); 0==c) {
            return false;
        } else if constexpr (1==c) {
            for (int k=0; k<rank(); ++k) {
                dim_t ls = len(k);
                if (((k<std::get<I>(t).rank() && ls!=choose_len(std::get<I>(t).len(k), ls)) || ...)) {
                    RA_CHECK(!checkp, "Shape mismatch [", (std::array { std::get<I>(t).len(k) ... }), "] on axis ", k, ".");
                    return false;
                }
            }
        }
        return true;
    }

    constexpr
    Match(P ... p_): t(std::forward<P>(p_) ...)
    {
// TODO Maybe on ply, would avoid the checkp, make agree_xxx() unnecessary.
        if constexpr (checkp && !(has_len<P> || ...)) {
            static_assert(check_s(), "Shape mismatch.");
            RA_CHECK(check());
        }
    }

// rank of largest subexpr, so we look at all of them.
    constexpr static rank_t
    rank_s()
    {
        rank_t r = RANK_BAD;
        return ((r=choose_rank(r, ra::rank_s<P>())), ...);
    }

    constexpr static rank_t
    rank()
    requires (DIM_ANY != Match::rank_s())
    {
        return rank_s();
    }

    constexpr rank_t
    rank() const
    requires (DIM_ANY == Match::rank_s())
    {
        rank_t r = RANK_BAD;
        ((r = choose_rank(r, std::get<I>(t).rank())), ...);
        assert(RANK_ANY!=r); // not at runtime
        return r;
    }

// first nonnegative size, if none first DIM_ANY, if none then DIM_BAD
    constexpr static dim_t
    len_s(int k)
    {
        auto f = [&k]<class A>(dim_t s) {
            constexpr rank_t ar = A::rank_s();
            return (ar<0 || k<ar) ? choose_len(s, A::len_s(k)) : s;
        };
        dim_t s = DIM_BAD; ((s>=0 ? s : s = f.template operator()<std::decay_t<P>>(s)), ...);
        return s;
    }

    constexpr static dim_t
    len(int k)
    requires (requires (int kk) { P::len(kk); } && ...)
    {
        return len_s(k);
    }

    constexpr dim_t
    len(int k) const
    requires (!(requires (int kk) { P::len(kk); } && ...))
    {
        auto f = [&k](dim_t s, auto const & a) {
            return k<a.rank() ? choose_len(s, a.len(k)) : s;
        };
        dim_t s = DIM_BAD; ((s>=0 ? s : s = f(s, std::get<I>(t))), ...);
        assert(DIM_ANY!=s); // not at runtime
        return s;
    }

    constexpr void
    adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }

    constexpr auto
    step(int i) const
    {
        return std::make_tuple(std::get<I>(t).step(i) ...);
    }

    constexpr bool
    keep_step(dim_t st, int z, int j) const
    requires (!(requires (dim_t d, rank_t i, rank_t j) { P::keep_step(d, i, j); } && ...))
    {
        return (std::get<I>(t).keep_step(st, z, j) && ...);
    }
    constexpr static bool
    keep_step(dim_t st, int z, int j)
    requires (requires (dim_t d, rank_t i, rank_t j) { P::keep_step(d, i, j); } && ...)
    {
        return (std::decay_t<P>::keep_step(st, z, j) && ...);
    }
};

} // namespace ra
