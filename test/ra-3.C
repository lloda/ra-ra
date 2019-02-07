
// (c) Daniel Llorens - 2013, 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file ra-3.C
/// @brief Checks for ra:: arrays, assignment.

#include <iostream>
#include <iterator>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"

using std::cout; using std::endl; using std::flush;
using std::tuple;
using real = double;

int main()
{
    TestRecorder tr;
    tr.section("[ra06] assignment cases with scalar or RANK_ANY arguments");
    {
        tr.section("assignment of 0 rank <- scalar expr");
        {
            ra::Unique<real, 0> a ({}, ra::scalar(99));
            tr.test_eq(99, a());
            a = ra::scalar(77);
            tr.test_eq(77, a());
        }
        tr.section("assignment of var rank <- scalar expr");
        {
            ra::Unique<real> a ({3, 2}, ra::scalar(99));
            tr.test_eq(99, a(0, 0));
            tr.test_eq(99, a(0, 1));
            tr.test_eq(99, a(1, 0));
            tr.test_eq(99, a(1, 1));
            tr.test_eq(99, a(2, 0));
            tr.test_eq(99, a(2, 1));
            a = ra::scalar(77);
            tr.test_eq(77, a(0, 0));
            tr.test_eq(77, a(0, 1));
            tr.test_eq(77, a(1, 0));
            tr.test_eq(77, a(1, 1));
            tr.test_eq(77, a(2, 0));
            tr.test_eq(77, a(2, 1));
        }
        tr.section("assignment of var rank <- lower rank expr I");
        {
            ra::Unique<real, 1> b ({3}, {1, 2, 3});
            ra::Unique<real> a ({3, 2}, ra::scalar(99));
            a  = b.iter();
            tr.test_eq(1, a(0, 0));
            tr.test_eq(1, a(0, 1));
            tr.test_eq(2, a(1, 0));
            tr.test_eq(2, a(1, 1));
            tr.test_eq(3, a(2, 0));
            tr.test_eq(3, a(2, 1));
        }
        tr.section("construction of var rank <- lower rank expr II");
        {
            ra::Unique<real, 2> b ({3, 2}, {1, 2, 3, 4, 5, 6});
            ra::Unique<real> a ({3, 2, 4}, ra::scalar(99));
            a  = b.iter();
            for (int i=0; i<3; ++i) {
                for (int j=0; j<2; ++j) {
                    tr.test_eq(i*2+j+1, b(i, j));
                    for (int k=0; k<4; ++k) {
                        tr.test_eq(b(i, j), a(i, j, k));
                    }
                }
            }
        }
// this succeeds because of the two var ranks, the top rank comes first (and so it's selected as driver). TODO Have run time driver selection so this is safe.
        tr.section("construction of var rank <- lower rank expr III (var rank)");
        {
            ra::Unique<real> b ({3}, {1, 2, 3});
            ra::Unique<real> a ({3, 2}, ra::scalar(99));
            a = b.iter();
            tr.test_eq(1, a(0, 0));
            tr.test_eq(1, a(0, 1));
            tr.test_eq(2, a(1, 0));
            tr.test_eq(2, a(1, 1));
            tr.test_eq(3, a(2, 0));
            tr.test_eq(3, a(2, 1));
        }
// driver selection is done at compile time (see Expr::DRIVER). Here it'll be the var rank expr, which results in an error at run time. TODO Do run time driver selection to avoid this error.
        // tr.section("construction of var rank <- higher rank expr");
        // {
        //     ra::Unique<real> b ({3, 2}, {1, 2, 3, 4, 5, 6});
        //     cout << "b: " << b << endl;
        //     ra::Unique<real> a ({4}, ra::scalar(99));
        //     a = b.iter();
        //     cout << "a: " << a << endl;
        // }
    }
    return tr.summary();
}
