// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Constexpr checks.

// (c) Daniel Llorens - 2013-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"

int main()
{
    ra::TestRecorder tr;
    tr.section("iota Flat constexpr");
    {
        constexpr ra::Small<int, 7> a = { 0, 1, 2, 3, 4, 5, 6 };
        constexpr ra::Small<int, 7> v = map([&a](int i) { return a[i]*a[i]; }, ra::iota(7));
        tr.test_eq(v, sqr(a));
    }
    return tr.summary();
}
