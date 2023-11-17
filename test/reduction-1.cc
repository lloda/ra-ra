// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Regression for namespace issues surfaced in saveload branch

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;
using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("reductions with references I");
    {
        ra::Big<real, 2> c = {{1, 3, 2}, {7, 1, 3}};
        ra::Big<real, 1> m(3, 0);
        iter<1>(m) = iter<1>(m) + iter<1>(c); // works
        tr.info("sum of columns [ma113]").test_eq(ra::Big<double, 1> {8, 4, 5}, m);
        m = 0;
        iter<1>(m) = max(iter<1>(m), iter<1>(c)); // works also
        tr.info("max of columns [ma113]").test_eq(ra::Big<double, 1> {7, 3, 3}, m);
// using std::max within ra:: caused std::max() to grab T = View when CellBig returned View const &.
// that's why ra::max is declared with DEF_NAME_OP_FWD.
        auto c1 = iter<1>(c);
        auto m1 = iter<1>(m);
        cout << ra::max(*c1, *m1) << endl;
    }
    tr.section("reductions with references II");
    {
        ra::Big<complex, 2> c = {{1, 3, 2}, {7, 1, 3}};
        ra::Big<complex, 1> m(3, 0);
        m = 0;
        map([](auto && ... a) -> decltype(auto) { return real_part(RA_FWD(a) ...); }, iter<1>(m))
            = map([](auto && ... a) -> decltype(auto) { return real_part(RA_FWD(a) ...); }, iter<1>(c));
        tr.info("max of columns [ma113]").test_eq(ra::Big<double, 1> {7, 1, 3}, m);
// still works
        m = 0;
        iter<1>(m) = real_part(iter<1>(c));
        tr.info("max of columns [ma113]").test_eq(ra::Big<double, 1> {7, 1, 3}, m);
// should be the same FIXME doesn't work, and didn't work with the flat() interface either.
        // m = 0;
        // real_part(iter<1>(m)) = real_part(iter<1>(c));
        // tr.info("max of columns [ma113]").test_eq(ra::Big<double, 1> {7, 1, 3}, m);
    }
    tr.section("reductions with references III");
    {
        ra::Big<int, 2> c = {{1, 3, 2}, {7, 1, 3}};
        ra::Big<int, 1> m(3, 0);
        scalar(m) = max(scalar(m), iter<1>(c)); // requires inner forward as in ra.hh DEF_NAME_OP
        tr.info("max of columns I").test_eq(ra::Big<int, 1> {7, 3, 3}, m);
    }
    return tr.summary();
}
