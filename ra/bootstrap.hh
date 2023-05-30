// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Before all other ra:: includes.

// (c) Daniel Llorens - 2013-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "tuples.hh"


// ---------------------
// Default #defines.
// ---------------------
// Since these are tested with #if, define them at the very top so accidental nodef isn't mistaken for 0.

// benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_DO_OPT_SMALLVECTOR
#define RA_DO_OPT_SMALLVECTOR 0
#endif

// no real downside to this.
#ifndef RA_DO_OPT_IOTA
#define RA_DO_OPT_IOTA 1
#endif

namespace ra {

constexpr int VERSION = 19;

using rank_t = int;
using dim_t = std::ptrdiff_t;
static_assert(sizeof(rank_t)>=4 && sizeof(dim_t)>=4);
static_assert(std::is_signed_v<rank_t> && std::is_signed_v<dim_t>);

// Negative numbers are used in some places as 'frame rank' in contrast to 'cell rank', so these numbers limit the rank that ra:: can handle besides the range of rank_t.

constexpr dim_t DIM_ANY = -1944444444; // only on ct values: not known at ct, but maybe at rt
constexpr rank_t RANK_ANY = -1944444444;
constexpr dim_t DIM_BAD = -1988888888; // undefined
constexpr rank_t RANK_BAD = -1988888888;

constexpr dim_t
dim_prod(dim_t a, dim_t b)
{
    return (a==DIM_ANY) ? DIM_ANY : ((b==DIM_ANY) ? DIM_ANY : a*b);
}

constexpr rank_t
rank_sum(rank_t a, rank_t b)
{
    return (a==RANK_ANY) ? RANK_ANY : ((b==RANK_ANY) ? RANK_ANY : a+b);
}

constexpr rank_t
rank_diff(rank_t a, rank_t b)
{
    return (a==RANK_ANY) ? RANK_ANY : ((b==RANK_ANY) ? RANK_ANY : a-b);
}

constexpr bool
inside(dim_t i, dim_t b)
{
    return i>=0 && i<b;
}

constexpr bool
inside(dim_t i, dim_t a, dim_t b)
{
    return i>=a && i<b;
}


// ---------------------
// concepts (WIP) - not sure i want duck typing, tbr.
// ---------------------

template <class P, class S>
concept FlatConcept = requires (P p, S d)
{
    { *p };
    { p += d };
};

template <class A>
concept IteratorConcept = requires (A a, rank_t k, dim_t d, rank_t i, rank_t j)
{
// FIXME we still allow ply(&) in some places. Cf also test/types.cc.
    { std::decay_t<A>::rank_s() } -> std::convertible_to<rank_t>;
    { a.rank() } -> std::convertible_to<rank_t>;
    { a.len(k) } -> std::same_as<dim_t>;
    { a.adv(k, d) } -> std::same_as<void>;
    { a.step(k) };
    { a.keep_step(d, i, j) } -> std::same_as<bool>;
    { a.flat() } -> FlatConcept<decltype(a.step(k))>;
};

template <class A>
concept SliceConcept = requires (A a)
{
    { A::rank_s() } -> std::convertible_to<rank_t>;
    { a.rank() } -> std::convertible_to<rank_t>;
    { a.iter() } -> IteratorConcept;
};


// ---------------------
// other types, forward decl
// ---------------------

enum none_t { none }; // used in array constructors to mean ‘don't initialize’.
struct no_arg {}; // used in array constructors to mean ‘don't instantiate’

template <class C> struct Scalar; // for type predicates
template <class T, rank_t RANK=RANK_ANY> struct View; // for cell_iterator_big
template <class V> struct ra_traits_def;

template <class S> struct default_steps_ {};
template <class tend> struct default_steps_<std::tuple<tend>> { using type = mp::int_list<1>; };
template <> struct default_steps_<std::tuple<>> { using type = mp::int_list<>; };

template <class t0, class t1, class ... ti>
struct default_steps_<std::tuple<t0, t1, ti ...>>
{
    using rest = typename default_steps_<std::tuple<t1, ti ...>>::type;
    static int const step0 = t1::value * mp::first<rest>::value;
    using type = mp::cons<mp::int_t<step0>, rest>;
};

template <class S> using default_steps = typename default_steps_<S>::type;

template <int n> struct dots_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};
template <int n> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};

template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

// Used by cell_iterator_big / cell_iterator_small.
template <class C>
struct CellFlat
{
    C c;
    constexpr void operator+=(dim_t const s) { c.p += s; }
    constexpr C & operator*() { return c; }
};

// Common to View / SmallBase. TODO Shouldn't it work on ... foreign vectors? arbitrary exprs?
template <int cell_rank, class A> constexpr auto
iter(A && a) { return std::forward<A>(a).template iter<cell_rank>(); }

// Used in big.hh (selectors, etc).
template <class A, class ... I> constexpr auto from(A && a, I && ... i);

// Extended in operators.hh. TODO All users be int, then this take int.
constexpr bool any(bool const x) { return x; }
constexpr bool every(bool const x) { return x; }
constexpr bool odd(unsigned int N) { return N & 1; }


// ---------------------
// nested braces for Small initializers
// ---------------------

// This logically belongs in ra/small.hh, but it's here so that shape() can return ra:: types.

// The general SmallArray has 4 constructors,
// 1. The empty constructor.
// 2. The scalar constructor. This is needed when T isn't registered as ra::scalar, which isn't required purely for container use.
// 3. The ravel constructor.
// 4. The nested constructor.
// When SmallArray has rank 1, or the first dimension is empty, or the shape is [1] or [], several of the constructors above become ambiguous. We solve this by defining the constructor arguments to variants of no_arg.

template <class T, class lens>
struct nested_tuple;

// ambiguity with empty constructor and scalar constructor.
// if len(0) is 0, then prefer the empty constructor.
// if len(0) is 1...
template <class lens> constexpr bool no_nested = (mp::first<lens>::value<1);
template <> constexpr bool no_nested<mp::nil> = true;
template <> constexpr bool no_nested<mp::int_list<1>> = true;
template <class T, class lens>
using nested_arg = std::conditional_t<no_nested<lens>,
                                      std::tuple<no_arg>, // match the template for SmallArray.
                                      typename nested_tuple<T, lens>::list>;

// ambiguity with scalar constructors (for rank 0) and nested_tuple (for rank 1).
template <class lens> constexpr bool no_ravel = ((mp::len<lens> <=1) || (mp::apply<mp::prod, lens>::value <= 1));
template <class T, class lens>
using ravel_arg = std::conditional_t<no_ravel<lens>,
                                     std::tuple<no_arg, no_arg>, // match the template for SmallArray.
                                     mp::makelist<mp::apply<mp::prod, lens>::value, T>>;

template <class T, class lens, class steps = default_steps<lens>> struct SmallView; // for cell_iterator_small
template <class T, class lens, class steps = default_steps<lens>,
          class nested_arg_ = nested_arg<T, lens>, class ravel_arg_ = ravel_arg<T, lens>>
struct SmallArray;
template <class T, dim_t ... lens> using Small = SmallArray<T, mp::int_list<lens ...>>;

template <class T>
struct nested_tuple<T, mp::nil>
{
    using sub = no_arg;
    using list = std::tuple<no_arg>; // match the template for SmallArray.
};

template <class T, int S0>
struct nested_tuple<T, mp::int_list<S0>>
{
    using sub = T;
    using list = mp::makelist<S0, T>;
};

template <class T, int S0, int S1, int ... S>
struct nested_tuple<T, mp::int_list<S0, S1, S ...>>
{
    using sub = Small<T, S1, S ...>;
    using list = mp::makelist<S0, sub>;
};

} // namespace ra

#include <array>
#include <ranges>
#include <cstdint>

namespace ra {


// --------------
// type classification
// --------------

// FIXME https://wg21.link/p2841r0 ?
#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A> constexpr bool JOIN(NAME, _def) = requires { requires PRED; }; \
    template <class A> constexpr bool NAME = JOIN(NAME, _def)< std::decay_t< A >>;

// ra_traits are for foreign types only. FIXME Not sure this is the interface I want.

RA_IS_DEF(is_scalar, (!std::is_pointer_v<A> && std::is_scalar_v<A>))
template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;
// template <> constexpr bool is_scalar_def<std::string_view> = true; // [ra13]

template <class T> requires (is_scalar<T>)
struct ra_traits_def<T>
{
    using V = T;
    constexpr static auto shape(V const & v) { return std::array<dim_t, 0> {}; }
    constexpr static dim_t size(V const & v) { return 1; }
    constexpr static dim_t size_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 0; }
    constexpr static rank_t rank_s() { return 0; }
};

// TODO make things is_iterator explicitly, as with is_scalar, and not by poking in the insides.
RA_IS_DEF(is_iterator, IteratorConcept<A>)
RA_IS_DEF(is_iterator_pos_rank, IteratorConcept<A> && A::rank_s()!=0)
RA_IS_DEF(is_slice, SliceConcept<A>)
RA_IS_DEF(is_slice_pos_rank, SliceConcept<A> && A::rank_s()!=0)

template <class A> constexpr bool is_ra = is_iterator<A> || is_slice<A>;
template <class A> constexpr bool is_ra_pos_rank = is_iterator_pos_rank<A> || is_slice_pos_rank<A>; // internal only FIXME
template <class A> constexpr bool is_ra_zero_rank = is_ra<A> && !is_ra_pos_rank<A>;
template <class A> constexpr bool is_zero_or_scalar = is_ra_zero_rank<A> || is_scalar<A>;

// ra_traits defined in small.hh.
template <class A> constexpr bool is_builtin_array = std::is_array_v<std::remove_cvref_t<A>>;

// std::string is std::ranges::range, but if we have it as is_scalar, we can't have it as is_foreign_vector.
RA_IS_DEF(is_foreign_vector, (!is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::random_access_range<A>))

// not using decay_t bc of builtin arrays.
template <class A> using ra_traits = ra_traits_def<std::remove_cvref_t<A>>;

// FIXME should be able to use std::span(V).extent (maybe p2325r3?) [ra2]
template <class V>
requires (is_foreign_vector<V> && requires { std::tuple_size<V>::value; })
struct ra_traits_def<V>
{
    constexpr static dim_t N = std::tuple_size_v<V>;
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { N }; }
    constexpr static dim_t size(V const & v) { return N; }
    constexpr static dim_t size_s() { return N; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; };
};

template <class V>
requires (is_foreign_vector<V> && !(requires { std::tuple_size<V>::value; }))
struct ra_traits_def<V>
{
// FIXME unqualified ssize fails on std::ranges::iota_view - looks iffy
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { std::ssize(v) }; }
    constexpr static dim_t size(V const & v) { return std::ssize(v); }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; }
};

template <class T>
struct ra_traits_def<std::initializer_list<T>>
{
    using V = std::initializer_list<T>;
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { ssize(v) }; }
    constexpr static dim_t size(V const & v) { return ssize(v); }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; }
};

template <class ... A> constexpr bool ra_pos_and_any = (is_ra_pos_rank<A> || ...) && ((is_ra<A> || is_scalar<A> || is_foreign_vector<A> || is_builtin_array<A>) && ...);
// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A> constexpr bool ra_zero = !(is_scalar<A> && ...) && (is_zero_or_scalar<A> && ...);

} // namespace ra
