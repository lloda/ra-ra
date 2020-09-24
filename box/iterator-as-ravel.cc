// -*- mode: c++; coding: utf-8 -*-
/// @file iterator-as-ravel.cc
/// @brief Using ptr(begin()) work as a lazy ravel (WIP)

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/ra.hh"
#include "ra/test.hh"

using std::cout, std::endl, std::flush;

int main()
{
    TestRecorder tr;
// this is trivial since Big::begin() is a raw pointer.
    tr.section("not driving, order I");
    {
        ra::Big<int, 2> a = {{1, 2}, {3, 4}, {5, 6}};
// same rank
        auto x = concrete(ra::ptr(a.begin()) * ra::Small<int, 5> { 1, 2, 3, 4, 5 });
        cout << x << endl;
// rank extension
        auto y = concrete(ra::ptr(a.begin()) * ra::Small<int, 2, 3> { {1, 2, 3}, {4, 5, 6} });
        cout << y << endl;
    }
// // the interesting part [FIXME We are missing advance() on stl-like iterators & also += [] secondarily].
//     tr.section("not driving, order II");
//     {
//         ra::Big<int, 2> a_ = {{1, 2}, {3, 4}, {5, 6}};
//         auto a = transpose<1, 0>(a_);
// // same rank
//         cout << (ra::ptr(a.begin()) * ra::Small<int, 5> { 1, 2, 3, 4, 5 }) << endl;
//     }
    return tr.summary();
}
