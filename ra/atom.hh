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

// Default storage for Big - see https://stackoverflow.com/a/21028912.
// Allocator adaptor that interposes construct() calls to convert value initialization into default initialization.
template <typename T, typename A=std::allocator<T>>
struct default_init_allocator: public A
{
    using a_t = std::allocator_traits<A>;
    using A::A;

    template <typename U>
    struct rebind
    {
        using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
    };

    template <typename U>
    void construct(U * ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
    {
        ::new(static_cast<void *>(ptr)) U;
    }
    template <typename U, typename... Args>
    void construct(U * ptr, Args &&... args)
    {
        a_t::construct(static_cast<A &>(*this), ptr, RA_FWD(args)...);
    }
};

template <class T> using vector_default_init = std::vector<T, default_init_allocator<T>>;


// --------------------
// introspection I
// --------------------

template <class VV> requires (!std::is_void_v<VV>)
consteval rank_t
rank_s()
{
    using V = std::remove_cvref_t<VV>;
    if constexpr (is_builtin_array<V>) {
        return std::rank_v<V>;
    } else if constexpr (is_fov<V>) {
        return 1;
    } else if constexpr (requires { V::rank_s(); }) {
        return V::rank_s();
    } else {
        return 0;
    }
}

template <class V> constexpr rank_t rank_s(V const &) { return rank_s<V>(); }

template <class V>
constexpr rank_t
rank(V const & v)
{
    if constexpr (ANY!=rank_s<V>()) {
        return rank_s<V>();
    } else if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else {
        static_assert(always_false<V>, "No rank() for this type.");
    }
}

template <class VV> requires (!std::is_void_v<VV>)
consteval dim_t
size_s()
{
    using V = std::remove_cvref_t<VV>;
    constexpr rank_t rs = rank_s<V>();
    if constexpr (0==rs) {
        return 1;
    } else if constexpr (is_builtin_array<V>) {
        return std::apply([] (auto ... i) { return (std::extent_v<V, i> * ... * 1); }, mp::iota<rs> {});
    } else if constexpr (is_fov<V> && requires { std::tuple_size<V>::value; }) {
        return std::tuple_size_v<V>;
    } else if constexpr (is_fov<V> || rs==ANY) {
        return ANY;
    } else if constexpr (requires { V::size_s(); }) {
        return V::size_s();
    } else {
        dim_t s = 1;
        for (int i=0; i<rs; ++i) {
            if (dim_t ss=V::len_s(i); ss>=0) { s *= ss; } else { return ss; } // ANY or BAD
        }
        return s;
    }
}

template <class V> constexpr dim_t size_s(V const &) { return size_s<V>(); }

template <class V>
constexpr dim_t
size(V const & v)
{
    if constexpr (ANY!=size_s<V>()) {
        return size_s<V>();
    } else if constexpr (is_fov<V>) {
        return std::ssize(v);
    } else if constexpr (requires { v.size(); }) {
        return v.size();
    } else {
        dim_t s = 1;
        for (rank_t k=0; k<rank(v); ++k) { s *= v.len(k); }
        return s;
    }
}

// Returns concrete types, value or const &. FIXME return ra:: types, but only if it's in all cases.
template <class V>
constexpr decltype(auto)
shape(V const & v)
{
    constexpr rank_t rs = rank_s<V>();
// FIXME __cpp_constexpr >= 202211L to return references to the constexpr cases
    if constexpr (is_builtin_array<V>) {
        return std::apply([] (auto ... i) { return std::array<dim_t, rs> { std::extent_v<V, i> ... }; }, mp::iota<rs> {});
    } else if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (0==rs) {
        return std::array<dim_t, 0> {};
    } else if constexpr (1==rs) {
        return std::array<dim_t, 1> { ra::size(v) };
    } else if constexpr (1<rs) {
        return std::apply([&v](auto ... i) { return std::array<dim_t, rs> { v.len(i) ... }; }, mp::iota<rs> {});
    } else {
        static_assert(ANY==rs);
        auto i = std::ranges::iota_view { 0, rank(v) } | std::views::transform([&v](auto k) { return v.len(k); });
        return vector_default_init<dim_t>(i.begin(), i.end()); // FIXME C++23 p1206? Still fugly
    }
}

template <class V>
constexpr dim_t
shape(V const & v, int k)
{
    RA_CHECK(inside(k, rank(v)), "Bad axis ", k, " for rank ", rank(v), ".");
    return v.len(k);
}

template <class A>
constexpr void
resize(A & a, dim_t s)
{
    if constexpr (ANY==size_s<A>()) {
        RA_CHECK(s>=0, "Bad resize ", s, ".");
        a.resize(s);
    } else {
        RA_CHECK(s==start(a).len(0) || BAD==s, "Bad resize ", s, " vs ", start(a).len(0), ".");
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
    constexpr static dim_t step(int k) { return 0; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() const { return *this; } // [ra39]
    constexpr decltype(auto) at(auto && j) const { return c; }
// self as Flat
    constexpr void operator+=(dim_t d) const {}
    constexpr C & operator*() { return c; }
    constexpr C const & operator*() const { return c; } // [ra39]

    RA_DEF_ASSIGNOPS_DEFAULT_SET
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

    consteval static rank_t rank_s() { return 1; };
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

    consteval static rank_t rank_s() { return w+1; };
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
struct Len
{
    consteval static rank_t rank_s() { return 0; }
    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static dim_t step(int k) { std::abort(); }
    constexpr static void adv(rank_t k, dim_t d) { std::abort(); }
    constexpr static bool keep_step(dim_t st, int z, int j) { std::abort(); }
    constexpr static Len const & flat() { std::abort(); }
    constexpr void operator+=(dim_t d) const { std::abort(); }
    constexpr dim_t operator*() const { std::abort(); }
};

constexpr Len len {};

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

// neither CellBig nor CellSmall will retain rvalues [ra4].
template <class T> requires (is_slice<T>)
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


// --------------------
// introspection II
// --------------------

// also used to paper over Scalar<X> vs X
template <class A>
constexpr decltype(auto)
FLAT(A && a)
{
    if constexpr (is_scalar<A>) {
        return RA_FWD(a); // avoid dangling temp in this case [ra8]
    } else {
        return *(ra::start(RA_FWD(a)).flat());
    }
}

// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using value_t = std::decay_t<decltype(FLAT(std::declval<A>()))>;

} // namespace ra
