// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - How args are passed when cell rank > 0.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#ifdef __STDCPP_FLOAT128_T__
#include <stdfloat>
#endif

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("positive cell rank iterators pass reference to sliding view");
    {
        ra::Big<int, 3> A({4, 2, 3}, 0);
        ra::Big<int, 2> B({4, 2}, 1);
        ra::Big<int, 1> C({4}, 10);
        for_each([&](auto & a, auto & b, auto & c)
        {
            a = b + c;
        }, iter<2>(A), iter<1>(B), C);
        tr.test_eq(11, A);
        for_each([&](auto & a, auto & b, auto & c)
        {
            a += b + c;
        }, iter<2>(A), iter<1>(B), C);
        tr.test_eq(22, A);
    }
    {
        ra::Big<int, 3> A({4, 2, 3}, 0);
        ra::Big<int, 2> B({4, 2}, 1);
        ra::Big<int, 1> C({4}, 10);
        for_each([&](auto && a, auto && b, auto & c)
        {
            a += b + c;
        }, iter<2>(A), iter<1>(B), C);
        tr.test_eq(11, A);
    }
    tr.section("using View::operator= on sliding view");
    {
        ra::Big<int, 2> A({3, 2}, 0);
        for_each([&](auto && a)
        {
            a(0) = 3;
            a(1) = 7;
            // a = { 3, 7 }; // FIXME
        }, iter<1>(A));
        ra::Big<int, 2> ref = {{3, 7}, {3, 7}, {3, 7}};
        tr.test_eq(ref, A);
    }
    return tr.summary();
}
