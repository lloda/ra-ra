
// (c) Daniel Llorens - 2013-2015, 2019

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file tuple-dynamic.H
/// @brief Using tuples in a dynamic context.

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
template <class C, class T, int ... i>
constexpr C from_tuple_(T && t, int_list<i ...>)
{
    return { std::get<i>(std::forward<T>(t)) ... };
}

template <class C, class T>
constexpr C from_tuple(T && t)
{
    return from_tuple_<C>(std::forward<T>(t), iota<len<std::decay_t<T>>> {});
}

template <class C, class T>
constexpr C tuple_values()
{
    return std::apply([](auto ... t) { return C { decltype(t)::value ... }; }, T {});
}

template <class T, class C> struct map_indices;

template <class C, class ... t>
struct map_indices<std::tuple<t ...>, C>
{
    template <class I> constexpr static C f(I const & i) { return { i[t::value] ... }; };
};

// Do things along a tuple.
template <class T, int k=0> struct on_tuple;

template <class t0, class ... ti, int k>
struct on_tuple<std::tuple<t0, ti ...>, k>
{
    using T = std::tuple<t0, ti ...>;
    using next = on_tuple<std::tuple<ti ...>, k+1>;

    constexpr static int index(int const q)
    {
        return (q==t0::value) ? k : next::index(q);
    }
    constexpr static int ref(int const i)
    {
        return k==i ? t0::value : next::ref(i);
    }
};

template <int k>
struct on_tuple<std::tuple<>, k>
{
    using T = std::tuple<>;

    constexpr static int index(int const q)
    {
        return -1;
    }
    constexpr static int ref(int const i)
    {
        static_assert(true, "bad ref");
        return -1;
    }
};

} // namespace mp
