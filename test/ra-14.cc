// -*- mode: c++; coding: utf-8 -*-
// ra/test - Conversion to scalar from Big/View, corner cases.

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
    tr.section("Values");
    {
        tr.section("From var rank 1 through view [ra15]");
        {
            ra::Big<ra::Small<int, 2>> a = {{ 1, 2 }};
            cout << a.rank() << endl;
            cout << "a(0): " << a(0) << endl;
            ra::Small<int, 2> c = a(0);
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From var rank 0 through view [ra15]");
        {
            ra::Big<ra::Small<int, 2>> a({}, {{ 1, 2 }});
            cout << a.rank() << endl;
            cout << "a(): " << a() << endl;
            ra::Small<int, 2> c = a();
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From var rank 0  [ra15]");
        {
            ra::Big<ra::Small<int, 2>> a({}, {{ 1, 2 }});
            ra::Small<int, 2> c = a;
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
    }
// see also test/const.cc.
    tr.section("References");
    {
        tr.section("From var rank 0 to ref");
        {
            ra::Big<ra::Small<int, 2>> a({}, {{ 1, 2 }});
            ra::Small<int, 2> & c = a;
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From const var rank 0 to ref");
        {
            ra::Big<ra::Small<int, 2>> const a({}, {{ 1, 2 }});
            ra::Small<int, 2> const & c = a;
            tr.test_eq(1, c[0]);
            tr.test_eq(2, c[1]);
        }
        tr.section("From const var rank 0 to ref");
        {
            static_assert(requires { requires !(std::is_convertible_v<ra::Big<ra::Small<int, 2>> const, ra::Small<int, 2> &>); });
        }
    }
    return tr.summary();
}
