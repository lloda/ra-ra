// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Regression test for lvalue exprs in gcc 6.1.

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Failed for Expr:: (now Map), Ryn:: (now Reframe), Vector:: =, += ... on gcc 6.1 due to
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70942. This is kept to show why that
// forward<decltype(y)>(y) is there.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;

using real = double;
template <int rank=ra::ANY> using Ureal = ra::Unique<real, rank>;
template <int rank=ra::ANY> using Uint = ra::Unique<int, rank>;

int main()
{
    TestRecorder tr(std::cout);
//     tr.section("gcc 6.1 A");
//     {
// // Both must be lvalues [ra5]. FIXME check that these fail [ra42]
//         ra::Unique<int, 1> a { 0, 0, 0, 0 };
//         ra::Unique<int, 1> b { 0, 0, 0, 0 };
//         where(ra::_0>0 && ra::_0<3, ra::_0, a) = 99;
//         where(ra::_0>0 && ra::_0<3, a, ra::_0) = 99;
//     }
    tr.section("gcc 6.1 B");
    {
        Ureal<1> a {1, 4, 2, 3};
        Ureal<1> b({4}, 0.);
        b(3-ra::_0) = a;
        tr.test_eq(Ureal<1> {3, 2, 4, 1}, b);
    }
    tr.section("gcc 6.1 C");
    {
        Ureal<1> a = {1, 2, 3, 4};
        Uint<1> i = {3, 1, 2};
        a(i) = ra::Unique<real, 1> {7, 8, 9};
        tr.test_eq(a, Ureal<1> {1, 8, 9, 7});
    }
    tr.section("gcc 6.1 D");
    {
        ra::Big<int, 2> A({4, 4}, 0), B({4, 4}, 10*ra::_0 + ra::_1);
        using coord = ra::Small<int, 2>;
        ra::Big<coord, 1> I = { coord{1, 1}, coord{2, 2} };

        map([&A](auto && c) -> decltype(auto) { return A.at(c); }, I)
            = map([&B](auto && c) { return B.at(c); }, I);
        tr.test_eq(Ureal<2>({4, 4}, {0, 0, 0, 0,  0, 11, 0, 0,  0, 0, 22, 0,  0, 0, 0, 0}), A);
    }
    return tr.summary();
}
