
// (c) Daniel Llorens - 2013-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_TYPE_H
#define RA_TYPE_H

/// @file ra-type.H
/// @brief Type predicates and use-as-xpr wrapper for foreign types.

#include "ra/ra-atom.H"
#include <iterator>

namespace ra {

// specialize this for new types for which is_scalar<> should be true.
template <class A_>
struct is_scalar_def
{
    using A = std::decay_t<A_>;
    static bool const value = mp::And<mp::Not<std::is_pointer<A> >, std::is_scalar<A> >::value;
};

template <class A> using is_scalar = is_scalar_def<std::decay_t<A> >;

template <class C, class Enable=void>
struct is_slice
{
    static bool const value = false;
};

template <class C>
struct is_slice<C, enableif_<mp::And<mp::exists<decltype(std::declval<std::decay_t<C>>().iter())>,
                                     mp::exists<decltype(ra_traits<C>::size_s())>>>> // do not accept public-derived from C
{
    static bool const value = true;
};

template <class C, class Enable=void>
struct is_slice_pos_rank
{
    static bool const value = false;
};

template <class C>
struct is_slice_pos_rank<C, enableifc_<(is_slice<C>::value && ra_traits<C>::rank_s()!=0)>>
{
    static bool const value = true;
};

template <class A, class Enable=void>
struct is_array_iterator
{
    static bool const value = false;
};

template <class A>
struct is_array_iterator<A, enableif_<mp::exists<decltype(std::declval<std::decay_t<A>>().flat())>>>
{
    // @TODO check the rest of the required interface of A and A::flat() right here.
    static bool const value = true;
};

template <class A, class Enable=void>
struct is_array_iterator_pos_rank
{
    static bool const value = false;
};

template <class A>
struct is_array_iterator_pos_rank<A, enableifc_<std::decay_t<A>::rank_s()!=0> >
{
    static bool const value = is_array_iterator<A>::value;
};

template <class A>
struct is_ra
{
    static bool const value = mp::Or<is_array_iterator<A>, is_slice<A>>::value;
};

template <class A>
struct is_ra_pos_rank
{
    static bool const value = mp::Or<is_array_iterator_pos_rank<A>, is_slice_pos_rank<A> >::value;
};

template <class A>
struct is_ra_zero_rank
{
    static bool const value = mp::And<is_ra<A>, mp::Not<is_ra_pos_rank<A>>>::value;
};

template <class A>
struct is_zero_or_scalar
{
    static bool const value = mp::Or<is_ra_zero_rank<A>, is_scalar<A> >::value;
};

template <class A, class Enable=void>
struct is_foreign_vector
{
    constexpr static bool value = false;
};

// @TODO Forbid pointers b/c ra:: operator<<(ostream) would pick them up. It probably shouldn't.
template <class A>
struct is_foreign_vector<A, enableif_<mp::And<mp::Not<is_slice<A>>, mp::exists<decltype(ra_traits<A>::size_s())>>>>
{
    constexpr static bool value = true;
};

// --------------
// to provide extension of scalar functions to ra:: types
// --------------

template <class ... A>
struct ra_pos_and_any
{
    static bool const value = mp::And<mp::Or<is_ra_pos_rank<A> ...>,
                                      mp::And<mp::Or<is_ra<A>, is_scalar<A> > ...> >::value;
};

// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A>
struct ra_zero
{
    static bool const value = mp::And<mp::Not<mp::And<is_scalar<A> ...> >,
                                      mp::And<is_zero_or_scalar<A> ...> >::value;
};

// --------------
// Coerce potential ArrayIterators
// --------------

template <class T, int a>
auto start(T && t)
{
    static_assert(!mp::exists<T>::value, "bad type for ra:: operator");
}

template <class T, enableif_<is_array_iterator<T>, int> = 0>
auto start(T && t)
{
    return std::forward<T>(t);
}
template <class T, enableif_<is_slice<T>, int> = 0>
auto start(T && t)
{
    return t.iter(); // @BUG if t is rvalue, t.iter() won't retain it...
}

template <class T, enableif_<is_scalar<T>, int> = 0>
auto start(T && t)
{
    return ra::scalar(std::forward<T>(t));
}

template <class T, enableif_<is_foreign_vector<T>, int> = 0>
auto start(T && t)
{
    return ra::vector(std::forward<T>(t));
}

} // namespace ra

#endif // RA_TYPE_H
