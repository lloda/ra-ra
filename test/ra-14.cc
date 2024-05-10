// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Conversion to scalar from Big/View, corner cases.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);

    using int2 = ra::Small<int, 2>;
    tr.section("Values");
    {
        tr.section("From var rank 0 [ra15]");
        {
            ra::Big<int2> a({}, {{ 1, 2 }});
            int2 c = a;
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From var rank 0 through view [ra15]");
        {
            ra::Big<int2> a({}, {{ 1, 2 }});
            tr.test_eq(0, a.rank());
            int2 c = a();
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From var rank 1 through view [ra15]");
        {
            ra::Big<int2> a = {int2{ 1, 2 }}; // [ra16] for the need to say int2
            tr.test_eq(1, a.rank());
            tr.test_eq(1, a.len(0));
            int2 c = a(0);
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
    }
// see also test/const.cc.
    tr.section("References");
    {
        tr.section("From var rank 0 to ref");
        {
            ra::Big<int2> a({}, {{ 1, 2 }});
            int2 & c = a;
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
            c = { 3, 4 };
            tr.test_eq(3, ((int2 &)(a))[0]);
            tr.test_eq(4, ((int2 &)(a))[1]);
        }
        tr.section("From const var rank 0 to ref");
        {
            ra::Big<int2> const a({}, {{ 1, 2 }});
            int2 const & c = a;
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From const var rank 0 to ref");
        {
            static_assert(requires { requires !(std::is_convertible_v<ra::Big<int2> const, int2 &>); });
        }
    }
    return tr.summary();
}
