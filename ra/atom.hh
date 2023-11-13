// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Terminal nodes for expression templates.

// (c) Daniel Llorens - 2011-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <utility>
#include <cassert>
#include "bootstrap.hh"


// --------------------
// error handling. See examples/throw.cc for how to customize.
// --------------------

#include <iostream> // might not be needed with a different RA_ASSERT.
#ifndef RA_ASSERT
#define RA_ASSERT(cond, ...)                                            \
    {                                                                   \
        if (std::is_constant_evaluated()) {                             \
            assert(cond /* FIXME show args */);                         \
        } else {                                                        \
            if (!(cond)) [[unlikely]] {                                 \
                std::cerr << ra::format("**** ra (", std::source_location::current(), "): ", ##__VA_ARGS__, " ****") << std::endl; \
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
// terminal types
// --------------------

// Rank-0 IteratorConcept. Can be used on foreign objects, or as alternative to the rank conjunction.
// We still want f(scalar(C)) to be f(C) and not map(f, C), this is controlled by tomap/toreduce.
template <class C>
struct Scalar
{
    C c;
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static dim_t step(int k) { return 0; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() const { return *this; } // [ra39]
    constexpr decltype(auto) at(auto && j) const { return c; }
// self as Flat
    constexpr void operator+=(dim_t d) const {}
    constexpr decltype(auto) operator*() { return c; }
    constexpr decltype(auto) operator*() const { return c; } // [ra39]

// save-load interface
    // constexpr decltype(auto) operator*() { return c; }
    constexpr int save() const { return 0; } // FIXME
    constexpr void load(int ii) {} // FIXME
    constexpr void adv0(dim_t d) {}
};

template <class C> constexpr auto
scalar(C && c) { return Scalar<C> { RA_FWD(c) }; }

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

    constexpr Ptr(I i, N n): i(i), n(n) {}
    RA_DEF_ASSIGNOPS_SELF(Ptr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    consteval static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { return nn; } // len(k==0) or step(k>=0)
    constexpr static dim_t len(int k) requires (nn!=ANY) { return len_s(k); }
    constexpr dim_t len(int k) const requires (nn==ANY) { return n; }
    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr void adv(rank_t k, dim_t d) { i += step(k) * d; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr auto flat() const { return i; }
    constexpr decltype(auto) at(auto && j) const requires (std::random_access_iterator<I>)
    {
        RA_CHECK(BAD==nn || inside(j[0], n), "Out of range for len[0]=", n, ": ", j[0], ".");
        return i[j[0]];
    }

// save-load interface
    constexpr auto save() const { return i; }
    constexpr void load(I ii) { i = ii; }
    constexpr decltype(auto) operator*() { return *i; }
    constexpr void adv0(dim_t d) { i += d; }
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
            return ptr(std::begin(RA_FWD(i)), std::ssize(i));
        } else {
            return ptr(std::begin(RA_FWD(i)), ic<s>);
        }
    } else if constexpr (std::bidirectional_iterator<std::decay_t<I>>) {
        if constexpr (std::is_integral_v<N>) {
            RA_CHECK(n>=0, "Bad ptr length ", n, ".");
        }
        return Ptr<std::decay_t<I>, iota_arg<N>> { i, RA_FWD(n) };
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

    consteval static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { return k==w ? nn : BAD; } // len(0<=k<=w) or step(0<=k)
    constexpr static dim_t len(int k) requires (is_constant<N>) { return len_s(k); }
    constexpr dim_t len(int k) const requires (!is_constant<N>) { return k==w ? n : BAD; }
    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr void adv(rank_t k, dim_t d) { i += O(step(k) * d) * O(s); }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr auto flat() const { return Flat { i, s }; }
    constexpr auto at(auto && j) const
    {
        RA_CHECK(BAD==nn || inside(j[0], n), "Out of range for len[0]=", n, ": ", j[0], ".");
        return i + O(j[w])*O(s);
    }

// save-load interface
    constexpr auto save() const { return i; }
    constexpr void load(O ii) { i = ii; }
    constexpr auto operator*() { return i; }
    constexpr void adv0(dim_t d) { i += O(d)*O(s); }
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
    return Iota<w, iota_arg<N>, iota_arg<O>, iota_arg<S>> { RA_FWD(n), RA_FWD(org), RA_FWD(s) };
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
constexpr struct Len
{
    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static dim_t step(int k) { std::abort(); }
    constexpr static void adv(rank_t k, dim_t d) { std::abort(); }
    constexpr static bool keep_step(dim_t st, int z, int j) { std::abort(); }
    constexpr static Len const & flat() { std::abort(); }
    constexpr void operator+=(dim_t d) const { std::abort(); }
    constexpr dim_t operator*() const { std::abort(); }
// save-load interface TODO
    constexpr void adv0(dim_t d) { std::abort(); }
} len;

// protect exprs with Len from reduction.
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
start(T && t) { return ra::ptr(RA_FWD(t)); }

template <class T>
constexpr auto
start(std::initializer_list<T> v) { return ra::ptr(v.begin(), v.size()); }

template <class T> requires (is_scalar<T>)
constexpr auto
start(T && t) { return ra::scalar(RA_FWD(t)); }

// forward declare for Match; implemented in small.hh.
template <class T> requires (is_builtin_array<T>)
constexpr auto
start(T && t);

// TODO fovs? arbitrary exprs?
template <int cr, class A> constexpr auto
iter(A && a) { return RA_FWD(a).template iter<cr>(); }

// neither CellBig nor CellSmall will retain rvalues [ra4].
template <SliceConcept T>
constexpr auto
start(T && t) { return iter<0>(RA_FWD(t)); }

RA_IS_DEF(is_ra_scalar, (std::same_as<A, Scalar<decltype(std::declval<A>().c)>>))

template <class T> requires (is_ra_scalar<T>)
constexpr decltype(auto)
start(T && t) { return RA_FWD(t); }

// iterators need to be restarted on each use (eg ra::cross()) [ra35].
template <class T> requires (is_iterator<T> && !is_ra_scalar<T>)
constexpr auto
start(T && t) { return RA_FWD(t); }

} // namespace ra
