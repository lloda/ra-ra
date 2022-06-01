// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Type predicates.

// (c) Daniel Llorens - 2013-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <array>
#include <ranges>
#include <cstdint>
#include "bootstrap.hh"

#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A> constexpr bool JOIN(NAME, _def) = false;         \
    template <class A> requires (PRED) constexpr bool JOIN(NAME, _def) < A > = true; \
    template <class A> constexpr bool NAME = JOIN(NAME, _def)< std::decay_t< A >>;

namespace ra {

// ra_traits are for foreign types only. FIXME Not sure this is the interface I want.


// --------------
// scalar, foreign or not. Specialize _def for more.
// --------------

RA_IS_DEF(is_scalar, (!std::is_pointer_v<A> && std::is_scalar_v<A>))
template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;
// template <> constexpr bool is_scalar_def<std::string_view> = true; // [ra13]

template <class T> requires (is_scalar<T>)
struct ra_traits_def<T>
{
    using V = T;
    constexpr static std::array<dim_t, 0> shape(V const & v) { return std::array<dim_t, 0> {}; }
    constexpr static dim_t size(V const & v) { return 1; }
    constexpr static dim_t size_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 0; }
    constexpr static rank_t rank_s() { return 0; }
};


// --------------
// type classification I
// --------------

// TODO make things is_iterator explicitly, as with is_scalar, and not by poking in the insides.
RA_IS_DEF(is_iterator, IteratorConcept<A>)
RA_IS_DEF(is_iterator_pos_rank, is_iterator<A> && A::rank_s()!=0)
// TODO use concept for is_slice.
RA_IS_DEF(is_slice, SliceConcept<A>)
RA_IS_DEF(is_slice_pos_rank, is_slice<A> && A::rank_s()!=0)

template <class A> constexpr bool is_ra = is_iterator<A> || is_slice<A>;
template <class A> constexpr bool is_ra_pos_rank = is_iterator_pos_rank<A> || is_slice_pos_rank<A>; // internal only FIXME
template <class A> constexpr bool is_ra_zero_rank = is_ra<A> && !is_ra_pos_rank<A>;
template <class A> constexpr bool is_zero_or_scalar = is_ra_zero_rank<A> || is_scalar<A>;


// --------------
// foreign vectors.
// --------------

// ra_traits defined in small.hh.
template <class A> constexpr bool is_builtin_array = std::is_array_v<std::remove_cv_t<std::remove_reference_t<A>>>;

// std::string is std::ranges::range, but if we have it as is_scalar, we can't have it as is_foreign_vector.
RA_IS_DEF(is_foreign_vector, (!is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::random_access_range<A>))

// not using decay_t bc of builtin arrays.
template <class A> using ra_traits = ra_traits_def<std::remove_cv_t<std::remove_reference_t<A>>>;

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


// --------------
// type classification II
// --------------

template <class ... A> constexpr bool ra_pos_and_any = (is_ra_pos_rank<A> || ...) && ((is_ra<A> || is_scalar<A> || is_foreign_vector<A> || is_builtin_array<A>) && ...);
// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A> constexpr bool ra_zero = !(is_scalar<A> && ...) && (is_zero_or_scalar<A> && ...);

} // namespace ra
