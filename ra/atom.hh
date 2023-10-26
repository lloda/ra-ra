// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Terminal nodes for expression templates.

// (c) Daniel Llorens - 2011-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <vector>
#include <utility>
#include <cassert>
#include "bootstrap.hh"


// --------------------
// what to do on errors. See examples/throw.cc for how to customize.
// --------------------

#include <iostream> // might not be needed with a different RA_ASSERT.

#ifndef RA_ASSERT
#define RA_ASSERT(cond, ...)                                            \
    {                                                                   \
        if (std::is_constant_evaluated()) {                             \
            assert(cond /* FIXME show args */);                         \
        } else {                                                        \
            if (!(cond)) [[unlikely]] {                                 \
                std::cerr << ra::format("**** ra (", std::source_location::current(), "): ", \
                                        ##__VA_ARGS__, " ****") << std::endl; \
                std::abort();                                           \
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

constexpr bool inside(dim_t i, dim_t b) { return 0<=i && i<b; }


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
        if constexpr (ANY==rank_s<V>()) {
            return ANY;
        } else if constexpr (0==rank_s<V>()) { // also non-registered types
            return 1;
        } else {
            dim_t s = 1;
            for (int i=0; i!=dV::rank_s(); ++i) {
                dim_t ss = dV::len_s(i);
                if (ss>=0) { s *= ss; } else { return ss; } // ANY or BAD
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
    if constexpr (ANY!=rank_s<V>()) {
        return rank_s<V>();
    } else if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else if constexpr (requires { ra_traits<V>::rank(v); }) {
        return ra_traits<V>::rank(v);
    } else {
        static_assert(always_false<V>, "No rank() for this type.");
    }
}

template <class V>
constexpr dim_t
size(V const & v)
{
    if constexpr (ANY!=size_s<V>()) {
        return size_s<V>();
    } else if constexpr (requires { v.size(); }) {
        return v.size();
    } else if constexpr (requires { ra_traits<V>::size(v); }) {
        return ra_traits<V>::size(v);
    } else {
        dim_t s = 1;
        for (rank_t k=0; k<rank(v); ++k) { s *= v.len(k); }
        return s;
    }
}

// Returns concrete type or const & thereto. Cf operator<<.
// FIXME would return ra:: types, but can't do that for the var rank case so.
template <class V>
constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (requires { ra_traits<V>::shape(v); }) {
        return ra_traits<V>::shape(v);
    } else if constexpr (constexpr rank_t rs=rank_s<V>(); rs>=0) {
        return std::apply([&v](auto ... i) { return std::array<dim_t, rs> { v.len(i) ... }; }, mp::iota<rs> {});
    } else {
        static_assert(ANY==rs);
        auto i = std::ranges::iota_view { 0, v.rank() } | std::views::transform([&v](auto k) { return v.len(k); });
        return std::vector<dim_t>(i.begin(), i.end()); // FIXME C++23 p1206? Still fugly
    }
}

template <class V>
constexpr dim_t
shape(V const & v, int k)
{
    RA_CHECK(inside(k, rank(v)), "Bad axis ", k, " for rank ", rank(v), ".");
    return v.len(k);
}

// To handle arrays of static/dynamic size.
template <class A>
inline void
resize(A & a, dim_t s)
{
    if constexpr (ANY==size_s<A>()) {
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

    consteval static rank_t rank_s() { return 0; }
    consteval static rank_t rank() { return 0; }
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

template <class C> constexpr auto
scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

template <class N> constexpr int
maybe_any = []{
    if constexpr (is_constant<N>) {
        return N::value;
    } else {
        static_assert(std::is_integral_v<N> || !std::is_same_v<N, bool>);
        return ANY;
    }
}();

// IteratorConcept for foreign rank 1 objects.
template <std::bidirectional_iterator I, class N>
struct Ptr
{
    static_assert(is_constant<N> || 0==rank_s<N>());
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(nn==ANY || nn>=0 || nn==BAD);

    I i;
    [[no_unique_address]] N const n = {};

    consteval static rank_t rank_s() { return 1; };
    consteval static rank_t rank() { return 1; }
    // len(k==0) or step(k>=0)
    constexpr static dim_t len_s(int k) { return nn; }
    constexpr static dim_t len(int k) requires (nn!=ANY) { return len_s(k); }
    constexpr dim_t len(int k) const requires (nn==ANY) { return n; }
    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += step(k) * d; }
    constexpr auto flat() const { return i; }
    constexpr decltype(auto) at(auto && j) const requires (std::random_access_iterator<I>)
    {
        RA_CHECK(BAD==nn || inside(j[0], n), "Out of range for len[0]=", n, ": ", j[0], ".");
        return i[j[0]];
    }

    constexpr Ptr(I i, N n): i(i), n(n) {}
    RA_DEF_ASSIGNOPS_SELF(Ptr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class X> using iota_arg = std::conditional_t<is_constant<std::decay_t<X>> || is_scalar<std::decay_t<X>>, std::decay_t<X>, X>;

template <class I, class N=dim_c<BAD>>
constexpr auto
ptr(I && i, N && n = N {})
{
// not decay_t bc of builtin arrays.
    if constexpr (std::ranges::bidirectional_range<std::remove_reference_t<I>>) {
        static_assert(std::is_same_v<dim_c<BAD>, N>, "Object has own length.");
        constexpr dim_t s = size_s<I>();
        if constexpr (ANY==s) {
            return ptr(std::begin(std::forward<I>(i)), std::ssize(i));
        } else {
            return ptr(std::begin(std::forward<I>(i)), ic<s>);
        }
    } else if constexpr (std::bidirectional_iterator<std::decay_t<I>>) {
        if constexpr (std::is_integral_v<N>) {
            RA_CHECK(n>=0, "Bad ptr length ", n, ".");
        }
        return Ptr<std::decay_t<I>, iota_arg<N>> { i, std::forward<N>(n) };
    } else {
        static_assert(always_false<I>, "Bad type for ptr().");
    }
}

// Sequence and IteratorConcept for same. Iota isn't really a terminal, but its exprs must all have rank 0.
// FIXME w is a custom Reframe mechanism inherited from TensorIndex. Generalize/unify
// FIXME Sequence should be its own type, we can't represent a ct origin bc IteratorConcept interface takes up i.
template <int w, class N_, class O, class S_>
struct Iota
{
    using N = std::decay_t<N_>;
    using S = std::decay_t<S_>;

    static_assert(w>=0);
    static_assert(is_constant<S> || 0==rank_s<S>());
    static_assert(is_constant<N> || 0==rank_s<N>());
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(nn==ANY || nn>=0 || nn==BAD);

    [[no_unique_address]] N const n = {};
    O i = {};
    [[no_unique_address]] S const s = {};

    constexpr static S gets() requires (is_constant<S>) { return S {}; }
    constexpr O gets() const requires (!is_constant<S>) { return s; }

    struct Flat
    {
        O i;
        [[no_unique_address]] S const s;
        constexpr void operator+=(dim_t d) { i += O(d)*O(s); }
        constexpr auto operator*() const { return i; }
    };

    consteval static rank_t rank_s() { return w+1; };
    consteval static rank_t rank() { return w+1; }
    // len(0<=k<=w) or step(0<=k)
    constexpr static dim_t len_s(int k) { return k==w ? nn : BAD; }
    constexpr static dim_t len(int k) requires (is_constant<N>) { return len_s(k); }
    constexpr dim_t len(int k) const requires (!is_constant<N>) { return k==w ? n : BAD; }
    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { i += O(step(k) * d) * O(s); }
    constexpr auto flat() const { return Flat { i, s }; }
    constexpr auto at(auto && j) const
    {
        RA_CHECK(BAD==nn || inside(j[0], n), "Out of range for len[0]=", n, ": ", j[0], ".");
        return i + O(j[w])*O(s);
    }
};

template <int w=0, class O=dim_t, class N=dim_c<BAD>, class S=dim_c<1>>
constexpr auto
iota(N && n = N {}, O && org = 0,
     S && s = [] {
         if constexpr (std::is_integral_v<S>) {
             return S(1);
         } else if constexpr (is_constant<S>) {
             static_assert(1==S::value);
             return S {};
         } else {
             static_assert(always_false<S>, "Bad step type for Iota.");
         }
     }())
{
    if constexpr (std::is_integral_v<N>) {
        RA_CHECK(n>=0, "Bad iota length ", n, ".");
    }
    return Iota<w, iota_arg<N>, iota_arg<O>, iota_arg<S>> { std::forward<N>(n), std::forward<O>(org), std::forward<S>(s) };
}

#define DEF_TENSORINDEX(w) constexpr auto JOIN(_, w) = iota<w>();
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

RA_IS_DEF(is_iota, false)
// BAD is excluded from beating to allow B = A(... i ...) to use B's len. FIXME find a way?
template <class N, class O, class S>
constexpr bool is_iota_def<Iota<0, N, O, S>> = (BAD != Iota<0, N, O, S>::nn);

template <class I>
constexpr bool
inside(I const & i, dim_t l) requires (is_iota<I>)
{
    return (inside(i.i, l) && inside(i.i+(i.n-1)*i.s, l)) || (0==i.n /* don't bother */);
}

// Never ply(), solely to be rewritten.
struct Len
{
    consteval static rank_t rank_s() { return 0; }
    consteval static rank_t rank() { return 0; }
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
start(T && t) { static_assert(always_false<T>, "Type cannot be start()ed."); }

template <class T> requires (is_fov<T>)
constexpr auto
start(T && t) { return ptr(std::forward<T>(t)); }

template <class T>
constexpr auto
start(std::initializer_list<T> v) { return ptr(v.begin(), v.size()); }

template <class T> requires (is_scalar<T>)
constexpr auto
start(T && t) { return scalar(std::forward<T>(t)); }

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
