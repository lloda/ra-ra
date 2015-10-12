
// (c) Daniel Llorens - 2011

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_ARGLIST_H
#define RA_ARGLIST_H

/// @file arglist.H
/// @brief Manipulate argument list.

#include "ra/macros.H"
#include <tuple>

// @note It's cleaner to have P0, P ... be parameters of the function and not of the class, but that triggers unimplemented features on gcc 4.5--4.6. Also below shuffle_args<>.

template <int i, class T, class Enable=void>
struct nth_arg_impl;

template <int i, class P0, class ... P>
struct nth_arg_impl<i, std::tuple<P0, P ...>, enableifc_<i==0>>
{
    static P0 && f(P0 && p0, P && ... p)
    {
        return std::forward<P0>(p0);
    }
};

template <int i, class P0, class ... P>
struct nth_arg_impl<i, std::tuple<P0, P ...>, enableifc_<(i>0)>>
{
    static_assert(i<=sizeof...(P), "bad argument index");
    using rest = std::tuple<P ...>;
    static auto f(P0 && p0, P && ... p) -> decltype(nth_arg_impl<i-1, rest>::f(std::forward<P>(p) ...))
    {
        return nth_arg_impl<i-1, rest>::f(std::forward<P>(p) ...);
    }
};

template <int i, class ... P>
inline auto nth_arg(P && ... p) -> decltype(nth_arg_impl<i, std::tuple<P ...>>::f(std::forward<P>(p) ...))
{
    return nth_arg_impl<i, std::tuple<P ...>>::f(std::forward<P>(p) ...);
}

template <class II, class TT>
struct shuffle_args_impl;

template <class ... I, class ... T>
struct shuffle_args_impl<std::tuple<I ...>, std::tuple<T ...>>
{
    using II = std::tuple<I ...>;
    using TT = std::tuple<T ...>;
    template <class G>
    static auto f(G && g, T && ... t) -> decltype(g(nth_arg<I::value>(std::forward<T>(t) ...) ...))
    {
        return g(nth_arg<I::value>(std::forward<T>(t) ...) ...);
    }
    template <class G>
    static auto ff(T && ... t) -> decltype(G::f(nth_arg<I::value>(std::forward<T>(t) ...) ...))
    {
        return G::f(nth_arg<I::value>(std::forward<T>(t) ...) ...);
    }
};

// when shuffling doesn't change the types.
template <class II, class G, class ... T>
static auto shuffle_args(G && g, T && ... t)
    -> decltype(shuffle_args_impl<II, std::tuple<T ...>>::template f(std::forward<G>(g), std::forward<T>(t) ...))
{
    return shuffle_args_impl<II, std::tuple<T ...>>::template f(std::forward<G>(g), std::forward<T>(t) ...);
}

// when shuffling changes the types, cannot use a single g.
template <class II, class G, class ... T>
static auto shuffle_args_types(T && ... t)
    -> decltype(shuffle_args_impl<II, std::tuple<T ...>>::template ff<G>(std::forward<T>(t) ...))
{
    return shuffle_args_impl<II, std::tuple<T ...>>::template ff<G>(std::forward<T>(t) ...);
}

#endif // RA_ARGLIST_H
