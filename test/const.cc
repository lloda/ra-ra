// -*- mode: c++; coding: utf-8 -*-
/// @file const.cc
/// @brief Const transfer from Container to View

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/mpdebug.hh"
#include "ra/complex.hh"
#include "ra/test.hh"
#include "ra/ra.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;

template <class T> struct is_constref;
template <class T> struct is_constref<T const &> : std::true_type {};
template <class T> struct is_constref<T &> : std::false_type {};
template <class T> constexpr bool is_constref_v = is_constref<T>::value;

int main()
{
    TestRecorder tr(std::cout);

    auto test =
        [&](auto & a, auto & b)
        {
            tr.test(!is_constref_v<decltype(*(a.data()))>);
            tr.skip().test(is_constref_v<decltype(*(b.data()))>); // FIXME [ra47]
            tr.test(!is_constref_v<decltype(*(a().data()))>);
            tr.test(is_constref_v<decltype(*(b().data()))>);
            tr.test(!is_constref_v<decltype(*(a(ra::all).data()))>);
            tr.test(is_constref_v<decltype(*(b(ra::all).data()))>);
        };

    {
        ra::Big<int> a = {1, 2, 3, 4};
        ra::Big<int> const b = {9, 8, 7, 6};
        test(a, b);
    }
    {
        ra::Big<int, 1> a = {1, 2, 3, 4};
        ra::Big<int, 1> const b = {9, 8, 7, 6};
        test(a, b);
    }
    return tr.summary();
}
