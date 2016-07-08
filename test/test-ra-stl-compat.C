
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-stl-compat.C
/// @brief Using ra:: array & iterator types with the STL algos & types.

// ra:: iterators are only partially STL compatible, because of copiability,
// lack of random access (which for the STL also means linear, but at least for
// 1D expressions it should be available), etc. Check some cases here.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"
#include "ra/ra-io.H"

using std::cout; using std::endl; using std::flush;

int main()
{
    TestRecorder tr;
    section("random access iterators");
    {
        // @TODO rank-0 begin()/end() in ra::Small
        // @TODO others?
    }
    section("copyable iterators, but not random access");
    {
        {
            ra::Owned<int, 1> a = { 1, 2, 3 };
            ra::Owned<int, 1> b = { 0, 0, 0 };
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
        {
            ra::Owned<int, 2> a({2, 3}, ra::_0 - 2*ra::_1);
            ra::Owned<int, 2> b({2, 3}, 99);
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
    }
    section("restricted iterators");
    {

    }
    return tr.summary();
}
