// -*- mode: c++; coding: utf-8 -*-
/// @file concrete.hh
/// @brief Obtain concrete type from array expression.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/big.hh"

namespace ra {

template <class E> struct concrete_type_def_1;

template <class E>
requires (size_s<E>()==DIM_ANY)
struct concrete_type_def_1<E>
{
    using type = Big<value_t<E>, E::rank_s()>;
};

template <class E>
requires (size_s<E>()!=DIM_ANY)
struct concrete_type_def_1<E>
{
    template <class I> struct T;
    template <int ... I> struct T<mp::int_list<I ...>>
    {
        using type = Small<value_t<E>, E::size_s(I) ...>;
    };
    using type = typename T<mp::iota<E::rank_s()>>::type;
};

template <class E>
struct concrete_type_def
{
    using type = std::conditional_t<
        is_scalar<E>,
        E, // scalars are their own concrete_type.
        typename concrete_type_def_1<std::decay_t<decltype(start(std::declval<E>()))>>::type>;
};

template <class E> using concrete_type = std::decay_t<typename concrete_type_def<E>::type>;
template <class E> inline auto concrete(E && e) { return concrete_type<E>(std::forward<E>(e)); }

// FIXME replace ra_traits::make

template <class E, class X> inline auto
with_same_shape(E && e, X && x)
{
    if constexpr (size_s<concrete_type<E>>()!=DIM_ANY) {
        return concrete_type<E>(std::forward<X>(x));
    } else {
        return concrete_type<E>(ra::shape(e), std::forward<X>(x));
    }
}

template <class E> inline auto
with_same_shape(E && e)
{
    if constexpr (size_s<concrete_type<E>>()!=DIM_ANY) {
        return concrete_type<E>();
    } else {
        return concrete_type<E>(ra::shape(e), ra::none);
    }
}

template <class E, class S, class X> inline auto
with_shape(S && s, X && x)
{
    if constexpr (size_s<concrete_type<E>>()!=DIM_ANY) {
        return concrete_type<E>(std::forward<X>(x));
    } else {
        return concrete_type<E>(std::forward<S>(s), std::forward<X>(x));
    }
}

template <class E, class S, class X> inline auto
with_shape(std::initializer_list<S> && s, X && x)
{
    if constexpr (size_s<concrete_type<E>>()!=DIM_ANY) {
        return concrete_type<E>(std::forward<X>(x));
    } else {
        return concrete_type<E>(s, std::forward<X>(x));
    }
}

} // namespace ra
