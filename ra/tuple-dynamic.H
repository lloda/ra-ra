// -*- mode: c++; coding: utf-8 -*-
/// @file tuple-dynamic.H
/// @brief Using tuples in a dynamic context.

// (c) Daniel Llorens - 2013-2015, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Think about struct <int k> { int const value = k; constexpr static int static_value = k; }
// where the tuple can be cast to an array ref.

#pragma once
#include "ra/tuple-list.H"
#include <cassert>

namespace mp {

// like std::make_trom_tuple, but use brace constructor (e.g. for std::array).
// FIXME forward t, e.g. https://vittorioromeo.info/index/blog/capturing_perfectly_forwarded_objects_in_lambdas.html
// until that's fixed, avoid using this for non trivial stuff.
// template <class C, class T>
// constexpr C from_tuple(T const & t)
// {
//     return std::apply([&t](auto ... i)
//                       { return C { std::get<decltype(i)::value>(t) ... }; },
//                       iota<len<std::decay_t<T>>> {});
// }

// like std::make_trom_tuple, but use brace constructor (e.g. for std::array).
template <class C, class T, int ... i> inline constexpr
C from_tuple_(T && t, int_list<i ...>)
{
    return { std::get<i>(std::forward<T>(t)) ... };
}

template <class C, class T> inline constexpr
C from_tuple(T && t)
{
    return from_tuple_<C>(std::forward<T>(t), iota<len<std::decay_t<T>>> {});
}

template <class C, class T> inline constexpr
C tuple_values()
{
    return std::apply([](auto ... t) { return C { decltype(t)::value ... }; }, T {});
}

template <class C, class T, class I> inline constexpr
C map_indices(I const & i)
{
    return std::apply([&i](auto ... t) { return C { i[decltype(t)::value] ... }; }, T {});
};

template <class T, int k=0> inline constexpr
int int_list_index(int i)
{
    if constexpr (k>=mp::len<T>) {
        return -1;
    } else {
        return (i==mp::ref<T, k>::value) ? k : int_list_index<T, k+1>(i);
    }
}

template <class K, class T, class F, class I = int_t<0>>
constexpr auto
fold_tuple(K && k, T && t, F && f, I && i = int_t<0> {})
{
    if constexpr (I::value==std::tuple_size_v<std::decay_t<T>>) {
        return k;
    } else {
        return fold_tuple(f(k, std::get<i>(t)), t, f, int_t<I::value+1> {});
    }
}

} // namespace mp
