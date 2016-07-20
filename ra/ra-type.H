
// (c) Daniel Llorens - 2013-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_TYPE_H
#define RA_TYPE_H

/// @file ra-type.H
/// @brief Type predicates and use-as-xpr wrapper for foreign types.

// Cf https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69515 for g++ -O0 when going
// from static member to variable template.

#include "ra/ra-atom.H"
#include <iterator>

namespace ra {

// specialize this for new types for which is_scalar<> should be true.
template <class A_>
struct is_scalar_def
{
    using A = std::decay_t<A_>;
    constexpr static bool value = !std::is_pointer<A>::value && std::is_scalar<A>::value;
};

template <class A> constexpr bool is_scalar = is_scalar_def<std::decay_t<A> >::value;

template <class C, class Enable=void>
constexpr bool is_slice = false;

template <class C>
constexpr bool is_slice<C, std::enable_if_t<mp::exists<decltype(std::declval<std::decay_t<C>>().iter())>
                                            && mp::exists<decltype(ra_traits<C>::size_s())>>> // do not accept public-derived from C
    = true;

template <class C, class Enable=void>
constexpr bool is_slice_pos_rank = false;

template <class C>
constexpr bool is_slice_pos_rank<C, std::enable_if_t<(is_slice<C> && ra_traits<C>::rank_s()!=0)>> = true;

// @TODO Convert the rest of the predicates as C++17 comes in line.

template <class A, class Enable=void>
struct is_array_iterator
{
    constexpr static bool value = false;
};

template <class A>
struct is_array_iterator<A, std::enable_if_t<mp::exists<decltype(std::declval<std::decay_t<A>>().flat())>>>
{
    // @TODO check the rest of the required interface of A and A::flat() right here.
    constexpr static bool value = true;
};

template <class A, class Enable=void>
struct is_array_iterator_pos_rank
{
    constexpr static bool value = false;
};

template <class A>
struct is_array_iterator_pos_rank<A, std::enable_if_t<std::decay_t<A>::rank_s()!=0> >
{
    constexpr static bool value = is_array_iterator<A>::value;
};

template <class A>
constexpr bool is_ra = is_array_iterator<A>::value || is_slice<A>;

template <class A>
constexpr bool is_ra_pos_rank = is_array_iterator_pos_rank<A>::value || is_slice_pos_rank<A>;

template <class A>
constexpr bool is_ra_zero_rank = is_ra<A> && !is_ra_pos_rank<A>;

template <class A>
constexpr bool is_zero_or_scalar = is_ra_zero_rank<A> || is_scalar<A>;

template <class A, class Enable=void>
constexpr bool is_foreign_vector = false;

// @TODO Forbid pointers b/c ra:: operator<<(ostream) would pick them up. It probably shouldn't.
template <class A>
constexpr bool is_foreign_vector<A, std::enable_if_t<!is_slice<A>
                                                     && mp::exists<decltype(ra_traits<A>::size_s())>
                                                     && !is_zero_or_scalar<A> > >
    = true;

// --------------
// to provide extension of scalar functions to ra:: types
// --------------

template <class ... A>
constexpr bool ra_pos_and_any = mp::Or<mp::bool_t<is_ra_pos_rank<A> > ...>::value
    && mp::And<mp::bool_t<(is_ra<A> || is_scalar<A>)> ...>::value;

// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A>
constexpr bool ra_zero = !mp::And<mp::bool_t<is_scalar<A> > ...>::value
    && mp::And<mp::bool_t<is_zero_or_scalar<A> > ...>::value;

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
inline auto start(T && t)
{
    static_assert(!mp::exists<T>, "bad type for ra:: operator");
}

template <class T, std::enable_if_t<is_array_iterator<T>::value, int> = 0>
inline auto start(T && t)
{
    return std::forward<T>(t);
}

// @TODO not used yet
template <class T, std::enable_if_t<std::is_same<dots_t<std::decay_t<T>::rank_s()>,  std::decay_t<T> >::value, int> =0>
inline auto start(T && t)
{
    return std::forward<T>(t);
}

template <class T, std::enable_if_t<is_slice<T>, int> = 0>
inline auto start(T && t)
{
    return t.iter(); // @BUG if t is rvalue, t.iter() won't retain it...
}

template <class T, std::enable_if_t<is_scalar<T>, int> = 0>
inline auto start(T && t)
{
    return ra::scalar(std::forward<T>(t));
}

template <class T, std::enable_if_t<is_foreign_vector<T>, int> = 0>
inline auto start(T && t)
{
    return ra::vector(std::forward<T>(t));
}

} // namespace ra

#endif // RA_TYPE_H
