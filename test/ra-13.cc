// -*- mode: c++; coding: utf-8 -*-
/// @file ra-13.cc
/// @brief Checks for ra:: constructing views.

// (c) Daniel Llorens - 2013-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/test.hh"
#include "ra/ra.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

// Originally from ra-0.cc, but triggered need for fixes in Container::init() for gcc 10.1.

int main()
{
    TestRecorder tr(std::cout);
    tr.section("construct View<> from sizes AND strides");
    {
        int p[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        int * pp = &p[0]; // force pointer decay in case we ever enforce p's shape

        // default giving sizes only
        ra::View<int> a(ra::Small<int, 2>({5, 2}), pp);
        tr.test_eq(ra::_0*2 + ra::_1*1 + 1, a);

        // same as default
        ra::View<int> b(ra::Small<ra::Dim, 2>({ra::Dim{5, 2}, ra::Dim{2, 1}}), pp);
        tr.test_eq(ra::_0*2 + ra::_1*1 + 1, b);

        // col major
        ra::View<int> c(ra::Small<ra::Dim, 2>({ra::Dim{5, 1}, ra::Dim{2, 5}}), pp);
        tr.test_eq(ra::_0*1 + ra::_1*5 + 1, c);

        // taking expr - generic col major
        ra::View<int> d(ra::pack<ra::Dim>(ra::Small<int, 3> {5, 1, 2}, ra::Small<int, 3> {1, 0, 5}), pp);
        tr.test_eq(ra::_0*1 + ra::_1*0 + ra::_2*5 + 1, d);
    }
    return tr.summary();
}
