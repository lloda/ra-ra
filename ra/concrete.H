
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_CONCRETE_H
#define RA_CONCRETE_H

/// @file concrete.H
/// @brief Obtain concrete type from array expression.

#include "ra/large.H"

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_CONCRETE RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_CONCRETE
        #define RA_CHECK_BOUNDS_RA_CONCRETE 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_CONCRETE==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

template <class E, class Enable=void> struct concrete_type_def;
template <class E> struct concrete_type_def<E, std::enable_if_t<E::rank_s()==RANK_ANY>>
{
    using type = Owned<value_t<E>>;
};
template <class E> struct concrete_type_def<E, std::enable_if_t<E::size_s()==DIM_ANY && E::rank_s()!=RANK_ANY>>
{
    using type = Owned<value_t<E>, E::rank_s()>;
};
template <class E> struct concrete_type_def<E, std::enable_if_t<E::size_s()!=DIM_ANY>>
{
    template <int N, class I=std::make_integer_sequence<int, N>> struct T;
    template <int N, int ... I> struct T<N, std::integer_sequence<int, I ...>>
    {
        using type = Small<value_t<E>, E::size_s(I) ...>;
    };
    using type = typename T<E::rank_s()>::type;
};

template <class E> using concrete_type = typename concrete_type_def<decltype(start(std::declval<E>()))>::type;
template <class E> auto concrete(E && e) { return concrete_type<E>(std::forward<E>(e)); }

// template <class E>
// struct ra_traits_def<E, std::enable_if_t<is_ra<E>>>: public ra_traits_def<concrete_type<E>> {};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_CONCRETE

#endif // RA_CONCRETE_H
