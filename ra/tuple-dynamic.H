
// (c) Daniel Llorens - 2013-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file tuple-dynamic.H
/// @brief Use tuples with dynamic parameters.

// TODO Think about struct <int k> { int const value = k; constexpr static int static_value = k; }
// where the tuple can be cast to an array ref.

#pragma once
#include <tuple>
#include <cassert>

namespace mp {

template <class T, class C> struct tuple_copy;

template <class C, class ... t>
struct tuple_copy<std::tuple<t ...>, C>
{
    constexpr static C f() { return { t::value ... }; } // clang bug with -Wmissing-braces TODO
};

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

    template <class I, class T, std::size_t N> static void map_indices(I const & i, std::array<T, N> & j)
    {
        static_assert(N==k, "bad indices in map_indices");
    }
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
