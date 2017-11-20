
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-stl-compat.C
/// @brief Using ra:: array & iterator types with the STL algos & types.

// ra:: iterators are only partially STL compatible, because of copiability,
// lack of random access (which for the STL also means linear, but at least for
// 1D expressions it should be available), etc. Check some cases here.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush;

int main()
{
    TestRecorder tr;
    tr.section("random access iterators");
    {
        // TODO rank-0 begin()/end() in ra::Small
        // TODO others?
    }
    tr.section("copyable iterators, but not random access");
    {
        {
            ra::Big<int, 1> a = { 1, 2, 3 };
            ra::Big<int, 1> b = { 0, 0, 0 };
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
        {
            ra::Big<int, 2> a({2, 3}, ra::_0 - 2*ra::_1);
            ra::Big<int, 2> b({2, 3}, 99);
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
        {
            ra::Small<int, 2, 3> a(ra::_0 - 2*ra::_1);
            ra::Small<int, 2, 3> b(99);
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
    }
    tr.section("raw, slippery pointers");
    {
        ra::Big<int, 1> a = {1, 2, 3};
        int b[] = { +1, -1, +1 };
        tr.test_eq(ra::Small<int, 3> {2, 1, 4}, a + ra::ptr(b));
        ra::ptr(b) = ra::Small<int, 3> {7, 4, 5};
        tr.test_eq(ra::Small<int, 3> {7, 4, 5}, ra::ptr(b));

        int cp[3] = {1, 2, 3};
        // ra::Big<int, 1> c({3}, &cp[0]); // forbidden, confusing for higher rank c (pointer matches as rank 1).
        ra::Big<int, 1> c({3}, ra::ptr(cp));
        tr.test_eq(ra::Small<int, 3> {1, 2, 3}, c);
    }
    tr.section("[ra12] check that begin() and end() match for empty views");
    {
        ra::Big<int, 3> aa({0, 2, 3}, 0.);
        auto a = aa(ra::all, 1);
        tr.info("begin ", a.begin().ii.c.p, " end ", a.end().ii.c.p).test(a.begin()==a.end());
    }
    return tr.summary();
}
