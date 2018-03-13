
// (c) Daniel Llorens - 2014, 2016-2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-small-0.C
/// @brief Small constructors and assignment.
// See test-small-1.C.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/small.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/big.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/mpdebug.H"

using std::cout; using std::endl; using std::flush;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr;
    tr.section("basic");
    {
        tr.test(std::is_standard_layout<ra::Small<float, 2>>::value);
    }
/*
  Small nested constructors rely on initialized_list so that we can size-check
  the arguments. This check has to be done at run time but I consider that
  preferable to the builtin array syntax which defect errors non-detectable
  (even though excess errors are compile time).

  Still, if we ever want to do that, it's good to know that we can construct
  higher rank builtin array types like this.
*/
    tr.section("basic facts about builtin arrays");
    {
        using A0 = double[3];
        using A1 = A0[2];
        using A2 = double[2][3];

        A1 x1 = {{1, 2, 3}, {4, 5, 6}};
        A2 x2 = {{1, 2, 3}, {4, 5, 6}};

        tr.test_eq(2u, std::rank<A1>::value);
        tr.test_eq(2, ra::start(x1).rank());
        tr.test_eq(2u, std::rank<A2>::value);
        tr.test_eq(2, ra::start(x2).rank());

        tr.test_eq(ra::start(x2), x1);
    }
    tr.section("scalar constructors");
    {
        tr.section("scalar paren unbraced");
        {
            ra::Small<int, 2> a(9);
            tr.test_eq(9, a[0]);
            tr.test_eq(9, a[1]);
        }
        tr.section("scalar op unbraced");
        {
            ra::Small<complex, 2> a = 9.;
            tr.test_eq(9., a[0]);
            tr.test_eq(9., a[1]);
        }
/*
  These all are array constructors and fail the size check. Note the
  inconsistency with the list constructors which always admit one redundant pair of
  braces. WISH they didn't.
*/
        {
            // ra::Small<int, 2> a({9});
            // ra::Small<int, 2> a {9};
            // ra::Small<complex, 2> a = {9.};
        }
    }
    tr.section("rank 0");
    {
        tr.section("rank 0 paren unbraced");
        {
            ra::Small<int> a(9);
            tr.test_eq(9, a());
        }
        tr.section("rank 0 paren braced");
        {
            ra::Small<int> a({9});
            tr.test_eq(9, a());
        }
        tr.section("rank 0 op unbraced");
        {
            ra::Small<int> a = 9;
            tr.test_eq(9, a());
        }
        tr.section("rank 0 op braced");
        {
            ra::Small<int> a = {9};
            tr.test_eq(9, a());
        }
        tr.section("rank 0 raw braced");
        {
            ra::Small<int> a {9};
            tr.test_eq(9, a());
        }
    }
    tr.section("rank 1");
    {
        tr.section("= braced");
        {
            ra::Small<int, 2> a = {{9, 3}};
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
        tr.section("= unbraced");
        {
            ra::Small<int, 2> a = {9, 3};
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
        tr.section("raw braced");
        {
            ra::Small<int, 2> a {{9, 3}};
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
        tr.section("raw unbraced");
        {
            ra::Small<int, 2> a {9, 3};
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
        tr.section("paren unbraced");
        {
            ra::Small<int, 2> a({9, 3});
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
        tr.section("paren braced");
        {
            ra::Small<int, 2> a({{9, 3}});
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
    }
    tr.section("rank 2");
    {
        tr.section("rank 2 (paren un)");
        {
            ra::Small<double, 2, 2> a({{1, 2}, {3, 4}});
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (paren braced)");
        {
            ra::Small<double, 2, 2> a({{{1, 2}, {3, 4}}});
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (= braced)");
        {
            ra::Small<double, 2, 2> a = {{{1, 2}, {3, 4}}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (= unbraced)");
        {
            ra::Small<double, 2, 2> a = {{1, 2}, {3, 4}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (raw unbraced)");
        {
            ra::Small<double, 2, 2> a {{1, 2}, {3, 4}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (raw braced)");
        {
            ra::Small<double, 2, 2> a {{{1, 2}, {3, 4}}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
    }
    tr.section("higher rank");
    {
        ra::Small<int, 2, 3, 4, 2> a = {{{{ 1,  2}, { 3,  4}, { 5,  6}, { 7,  8}},
                                         {{ 9, 10}, {11, 12}, {13, 14}, {15, 16}},
                                         {{17, 18}, {19, 20}, {21, 22}, {23, 24}}},
                                        {{{25, 26}, {27, 28}, {29, 30}, {31, 32}},
                                         {{33, 34}, {35, 36}, {37, 38}, {39, 40}},
                                         {{41, 42}, {43, 44}, {45, 46}, {47, 48}}}};
        // FIXME reshape for Small or something! Big/View have these things.
        ra::Small<int, 2, 3, 4, 2> b = 0;
        ra::SmallView<int, mp::int_list<48>, mp::int_list<1>> c(b.data());
        c = ra::iota(48, 1);
        tr.test_eq(b, a);
    }
    tr.section("raveling constructor");
    {
        ra::Small<int, 2, 2> a = {1, 2, 3, 4};
        tr.test_eq(1, a(0, 0));
        tr.test_eq(2, a(0, 1));
        tr.test_eq(3, a(1, 0));
        tr.test_eq(4, a(1, 1));
    }
    tr.section("nested Small");
    {
        ra::Small<ra::Small<int, 2>, 2> a = {{1, 2}, {3, 4}};
        tr.test_eq(1, a(0)(0));
        tr.test_eq(2, a(0)(1));
        tr.test_eq(3, a(1)(0));
        tr.test_eq(4, a(1)(1));
    }
    tr.section("operator=");
    {
        tr.section("operator= rank 1");
        {
            ra::Small<complex, 2> a { 3, 4 };
            a = complex(99.);
            tr.test_eq(99., a[0]);
            tr.test_eq(99., a[1]);
            a = 88.;
            tr.test_eq(88., a[0]);
            tr.test_eq(88., a[1]);
            a += 1.;
            tr.test_eq(89., a[0]);
            tr.test_eq(89., a[1]);
        }
        tr.section("operator= rank 2 unbraced");
        {
            ra::Small<int, 2, 2> a = 0;
            a = {{1, 2}, {3, 4}};
            tr.test_eq(1, a(0, 0));
            tr.test_eq(2, a(0, 1));
            tr.test_eq(3, a(1, 0));
            tr.test_eq(4, a(1, 1));
        }
        tr.section("operator= rank 2 braced");
        {
            ra::Small<int, 2, 2> a = 0;
            a = {{{1, 2}, {3, 4}}};
            tr.test_eq(1, a(0, 0));
            tr.test_eq(2, a(0, 1));
            tr.test_eq(3, a(1, 0));
            tr.test_eq(4, a(1, 1));
        }
        tr.section("operator= of view into view");
        {
            {
                ra::Small<int, 2, 2> a = {{1, 2}, {3, 4}};
                ra::SmallView<int, mp::int_list<2>, mp::int_list<1>> a0 = a(0);
                ra::SmallView<int, mp::int_list<2>, mp::int_list<1>> a1 = a(1);
                a0 = a1;
                tr.test_eq(ra::Small<int, 2, 2> {{3, 4}, {3, 4}}, a);
            }
            {
                ra::Small<int, 2, 2> a = {{1, 2}, {3, 4}};
                ra::SmallView<int, mp::int_list<2>, mp::int_list<1>> a0 = a(0);
                a0 = a(1);
                tr.test_eq(ra::Small<int, 2, 2> {{3, 4}, {3, 4}}, a);
            }
        }
        tr.section("repeat a test from test-big-0 [ra38]");
        {
            ra::Small<double, 2> A = {1, 2};
            ra::Small<double, 2> X = {0, 0};
            X(ra::all) = A();
            tr.test_eq(ra::start({1, 2}), X);
        }
    }
    return tr.summary();
}
