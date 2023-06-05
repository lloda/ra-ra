// -*- mode: c++; coding: utf-8 -*-
// ra/test - Const transfer from Container to View

// (c) Daniel Llorens - 2021-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/mpdebug.hh"
#include "ra/complex.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;

template <class T> constexpr bool is_cref_v = std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>;
template <class T> constexpr bool is_ncref_v = std::is_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>;

int main()
{
    TestRecorder tr(std::cout);

    auto test =
        [&](auto & a, auto & b)
        {
            tr.test(is_ncref_v<decltype(*(a.data()))>);
            tr.test(is_cref_v<decltype(*(b.data()))>);
            tr.test(is_ncref_v<decltype(*(a().data()))>);
            tr.test(is_cref_v<decltype(*(b().data()))>);
            tr.test(is_ncref_v<decltype(*(a(ra::all).data()))>);
            tr.test(is_cref_v<decltype(*(b(ra::all).data()))>);
        };
    tr.section("dynamic rank");
    {
        ra::Big<int> a = {1, 2, 3, 4};
        ra::Big<int> const b = {9, 8, 7, 6};
        test(a, b);
    }
    tr.section("static rank");
    {
        ra::Big<int, 1> a = {1, 2, 3, 4};
        ra::Big<int, 1> const b = {9, 8, 7, 6};
        test(a, b);
    }
    tr.section("static dimensions");
    {
        ra::Small<int, 4> a = {1, 2, 3, 4};
        ra::Small<int, 4> const b = {9, 8, 7, 6};
        test(a, b);
    }
    return tr.summary();
}
