// -*- mode: c++; coding: utf-8 -*-
/// @file owned.cc
/// @brief Array operations limited to ra::Big.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "ra/complex.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;

int main()
{
    TestRecorder tr;
    tr.section("resize first dimension");
    {
        auto test = [&tr](auto const & ref, auto & a, int newsize, int testsize)
            {
                a.resize(newsize);
                tr.test_eq(ref.rank(), a.rank());
                tr.test_eq(newsize, a.len(0));
                for (int i=1; i<a.rank(); ++i) {
                    tr.test_eq(ref.len(i), a.len(i));
                }
                tr.test_eq(ref(ra::iota(testsize)), a(ra::iota(testsize)));
            };
        {
            ra::Big<int, 2> a({5, 3}, ra::_0 - ra::_1);
            ra::Big<int, 2> ref = a;
            test(ref, a, 5, 5);
            test(ref, a, 8, 5);
            test(ref, a, 3, 3);
            test(ref, a, 5, 3);
        }
        {
            ra::Big<int, 1> a({2}, 3);
            a.resize(4, 9);
            tr.test_eq(3, a[0]);
            tr.test_eq(3, a[1]);
            tr.test_eq(9, a[2]);
            tr.test_eq(9, a[3]);
        }
        {
            ra::Big<int, 3> a({0, 3, 2}, ra::_0 - ra::_1 + ra::_2); // BUG If <int, 2>, I get [can't drive] instead of [rank error].
            ra::Big<int, 3> ref0 = a;
            test(ref0, a, 3, 0);
            a = 77.;
            ra::Big<int, 3> ref1 = a;
            test(ref1, a, 5, 3);
        }
    }
    tr.section("push back");
    {
        real check[] = { 2, 3, 4, 7 };
        auto test = [&tr, &check](auto && z)
            {
                tr.test_eq(0, z.len(0));
                tr.test_eq(1, z.stride(0));
                for (int i=0; i<4; ++i) {
                    z.push_back(check[i]);
                    tr.test_eq(i+1, z.size());
                    for (int j=0; j<=i; ++j) {
                        tr.test_eq(check[j], z[j]);
                    }
                }
            };
        test(ra::Big<real, 1>());
        ra::Big<real> z = ra::Big<real, 1>();
        test(z);
    }
    return tr.summary();
}
