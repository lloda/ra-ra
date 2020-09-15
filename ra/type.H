// -*- mode: c++; coding: utf-8 -*-
/// @file type.H
/// @brief Type predicates.

// (c) Daniel Llorens - 2013-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <iterator>
#include "ra/traits.H"

namespace ra {

// specialize this for new types for which is_scalar<> should be true. These are foreign types.
RA_IS_DEF(is_scalar, (!std::is_pointer_v<A> && std::is_scalar_v<A>))
RA_IS_DEF(has_traits, (mp::exists<decltype(ra_traits<A>::size)>))

template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;

// other foreign types we care about.
template <class A> constexpr bool is_builtin_array = std::is_array_v<std::remove_cv_t<std::remove_reference_t<A>>>;
template <class A> constexpr bool is_foreign_vector_def = false;
template <class A, class Enable=void> constexpr bool is_foreign_vector = is_foreign_vector_def<std::decay_t<A>>;
template <class T, class A> constexpr bool is_foreign_vector_def<std::vector<T, A>> = true;
template <class T, std::size_t N> constexpr bool is_foreign_vector_def<std::array<T, N>> = true;

// TODO make things is_iterator explicitly, as with is_scalar, and not by poking in the insides.
// TODO check the rest of the required interface of A and A::flat() right here. Concepts...
RA_IS_DEF(is_iterator, (mp::exists<decltype(std::declval<A>().flat())>))
RA_IS_DEF(is_iterator_pos_rank, is_iterator<A> && A::rank_s()!=0)
RA_IS_DEF(is_slice, (mp::exists<decltype(std::declval<A>().iter())> && has_traits<A>)) // require traits to reject public-derived from A
RA_IS_DEF(is_slice_pos_rank, is_slice<A> && A::rank_s()!=0)

template <class A> constexpr bool is_ra = is_iterator<A> || is_slice<A>;
template <class A> constexpr bool is_ra_pos_rank = is_iterator_pos_rank<A> || is_slice_pos_rank<A>;
template <class A> constexpr bool is_ra_zero_rank = is_ra<A> && !is_ra_pos_rank<A>;
template <class A> constexpr bool is_zero_or_scalar = is_ra_zero_rank<A> || is_scalar<A>;

// --------------
// to provide extension of scalar functions to ra:: types
// --------------

template <class ... A> constexpr bool ra_pos_and_any = (is_ra_pos_rank<A> || ...) && ((is_ra<A> || is_scalar<A> || is_foreign_vector<A> || is_builtin_array<A>) && ...);
// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A> constexpr bool ra_zero = !(is_scalar<A> && ...) && (is_zero_or_scalar<A> && ...);

template <class T>
struct ra_traits_def<T, std::enable_if_t<is_scalar<T>>>
{
    using V = T;
    using value_type = T;
    constexpr static std::array<dim_t, 0> shape(V const & v) { return std::array<dim_t, 0> {}; }
    constexpr static dim_t size(V const & v) { return 1; }
    constexpr static dim_t size_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 0; }
    constexpr static rank_t rank_s() { return 0; }
};

} // namespace ra
