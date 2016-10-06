
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_TYPE_H
#define RA_TYPE_H

/// @file type.H
/// @brief Type predicates and use-as-xpr wrapper for foreign types.

// Cf https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69515 for g++ -O0 when going
// from static member to variable template.

#include "ra/atom.H"
#include <iterator>

namespace ra {

// specialize this for new types for which is_scalar<> should be true. These are foreign types.
template <class A> constexpr bool is_scalar_def = !std::is_pointer<A>::value && std::is_scalar<A>::value;
template <class A> constexpr bool is_scalar = is_scalar_def<std::decay_t<A> >;

// the other class of foreign types we care about.
template <class A> constexpr bool is_foreign_vector_def = false;
template <class A, class Enable=void> constexpr bool is_foreign_vector = is_foreign_vector_def<std::decay_t<A> >;
template <class T, class A> constexpr bool is_foreign_vector_def<std::vector<T, A> > = true;
template <class T, std::size_t N> constexpr bool is_foreign_vector_def<std::array<T, N> > = true;

template <class A, class Enable=void> constexpr bool is_slice = false;
template <class A> constexpr bool
is_slice<A, std::enable_if_t<mp::exists<decltype(std::declval<std::decay_t<A>>().iter())>
                             && mp::exists<decltype(ra_traits<A>::size_s())>>> // reject public-derived from A
    = true;

template <class A, class Enable=void> constexpr bool is_slice_pos_rank = false;
template <class A> constexpr bool
is_slice_pos_rank<A, std::enable_if_t<(is_slice<A> && ra_traits<A>::rank_s()!=0)>> = true;

// @TODO make things is_ra_iterator explicitly, as with is_scalar, and not by poking in the insides.
// @TODO check the rest of the required interface of A and A::flat() right here.
template <class A, class Enable=void> constexpr bool is_ra_iterator = false;
template <class A> constexpr bool
is_ra_iterator<A, std::enable_if_t<mp::exists<decltype(std::declval<std::decay_t<A>>().flat())>>> = true;

template <class A, class Enable=void> constexpr bool is_ra_iterator_pos_rank = false;
template <class A> constexpr bool
is_ra_iterator_pos_rank<A, std::enable_if_t<std::decay_t<A>::rank_s()!=0> > = is_ra_iterator<A>;

template <class A> constexpr bool is_ra = is_ra_iterator<A> || is_slice<A>;
template <class A> constexpr bool is_ra_pos_rank = is_ra_iterator_pos_rank<A> || is_slice_pos_rank<A>;
template <class A> constexpr bool is_ra_zero_rank = is_ra<A> && !is_ra_pos_rank<A>;
template <class A> constexpr bool is_zero_or_scalar = is_ra_zero_rank<A> || is_scalar<A>;

// --------------
// to provide extension of scalar functions to ra:: types
// --------------

// @TODO https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68377 in gcc 6.2
template <class ... A> constexpr bool
ra_pos_and_any = (is_ra_pos_rank<A> || ...) && mp::And<mp::bool_t<(is_ra<A> || is_scalar<A>)> ...>::value;

// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A> constexpr bool
ra_zero = !(is_scalar<A> && ...) && (is_zero_or_scalar<A> && ...);

template <class T>
struct ra_traits_def<T, std::enable_if_t<is_scalar<T> > >
{
    using V = T;
    using value_type = T;
    constexpr static dim_t size(V const & v) { return 1; }
    constexpr static dim_t size_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 0; }
    constexpr static rank_t rank_s() { return 0; }
};

// --------------
// Coerce potential ArrayIterators
// --------------

template <class T, int a>
inline constexpr auto start(T && t)
{
    static_assert(!mp::exists<T>, "bad type for ra:: operator");
}

template <class T, std::enable_if_t<is_ra_iterator<T>, int> = 0>
inline constexpr auto start(T && t)
{
    return std::forward<T>(t);
}

// @TODO not used yet, dots is beaten.
template <class T, std::enable_if_t<std::is_same<dots_t<std::decay_t<T>::rank_s()>, std::decay_t<T> >::value, int> =0>
inline constexpr auto start(T && t)
{
    return std::forward<T>(t);
}

template <class T, std::enable_if_t<is_slice<T>, int> = 0>
inline constexpr auto start(T && t)
{
    return t.iter(); // @BUG if t is rvalue, t.iter() won't retain it. See test-ra-7.C [opt-small].
}

template <class T, std::enable_if_t<is_scalar<T>, int> = 0>
inline constexpr auto start(T && t)
{
    return ra::scalar(std::forward<T>(t));
}

template <class T, std::enable_if_t<is_foreign_vector<T>, int> = 0>
inline constexpr auto start(T && t)
{
    return ra::vector(std::forward<T>(t));
}

// template <class T>
// inline constexpr auto start(T * t)
// {
//     static_assert(!std::is_pointer<T *>::value, "pointers must be started with ptr");
// }

// @TODO one of these is ET-generic and the other is slice only, so make up your mind.
template <class A> using value_t = std::decay_t<decltype(*(ra::start(std::declval<A>()).flat()))>;
template <class V> inline auto size(V const & v) { return ra_traits<V>::size(v); }

} // namespace ra

#endif // RA_TYPE_H
