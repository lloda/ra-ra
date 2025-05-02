// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Constructors and assignment for Small.

// (c) Daniel Llorens - 2014, 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// See also small-1.cc.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using complex = std::complex<double>;
using ra::int_list, ra::int_c;

struct test_type { int a; };

std::string typecheck(auto && a)
{
    return std::source_location::current().function_name();
};

int main()
{
    TestRecorder tr;
    tr.section("std::array is used internally so these are fundamental");
    {
        static_assert(3==ra::size(std::array { 3, 4, 5 }));
        static_assert(3==ra::size_s(std::array { 3, 4, 5 }));
    }
    tr.section("Small isn't an aggregate so T; and T {}; are the same, unlike std::array");
    {
        std::array<int, 9> a; // default init, unlike {} which is aggregate init
        cout << ra::start(a) << endl;
        ra::Small<int, 9> b; // default init, just like {}
        cout << b << endl;
        ra::Small<int, 3> c(ra::none); // if you want to be explicit
        cout << c << endl;
    }
    tr.section("can't really init std::array with constant");
    {
        std::array<int, 9> a = {1};
        tr.test_eq(1, a[0]);
        tr.test_eq(0, a[1]);
        ra::Small<int, 9> b = {1};
        tr.test_eq(1, b);
    }
    tr.section("predicates");
    {
        tr.test(std::is_standard_layout_v<ra::Small<float, 2>>);
        ra::Small<int, 2, 3> a;
        static_assert(std::input_iterator<decltype(a.begin())>);
    }
    tr.section("BUG ambiguous case I");
    {
        double U[] = { 1, 2, 3 };
        ra::Small<ra::Big<double, 1>, 1> u = { {U} };
        tr.test_eq(U, u[0]);
        // std::cout << "---------" << std::endl;
        // ra::Small<ra::Big<double, 1>, 1> v = { U }; // BUG [ra45] (Big inits as empty, then assignment fails)
        // std::cout << "---------" << std::endl;
        // cout << v << endl;
    }
    // tr.section("BUG ambiguous case II");
    // {
    //     ra::Small<int, 1, 2> a({ 1, 2 }); // BUG ambiguous [ra46]
    //     // ra::Small<int, 1, 2> a { 1, 2 }; // works as ravel
    //     // ra::Small<int, 1, 2> a {{ 1, 2 }}; // works
    //     // ra::Small<int, 1, 2> a({{ 1, 2 }}); // works
    //     tr.test_eq(1, a(0, 0));
    //     tr.test_eq(2, a(0, 1));
    // }
/*
  Small nested constructors rely on initialized_list so that we can size-check
  the arguments. This check has to be done at run time but I consider that
  preferable to the builtin array syntax which defect errors non-detectable
  (even though excess errors are compile time).

  Still, if we ever want to do that, it's good to know that we can construct
  higher rank builtin array types like this.
*/
    tr.section("= unbraced");
    {
        ra::Small<int, 2> a = {9, 3};
        tr.test_eq(9, a[0]);
        tr.test_eq(3, a[1]);
    }
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
    tr.section("top level generics on builtin arrays");
    {
        int a[3] = {1, 2, 3};
        tr.test_eq(1, ra::rank(a));
        tr.test_eq(3, ra::size(a));
        tr.test_eq(3, ra::shape(a)[0]);
    }
    tr.section("top level generics on builtin arrays II");
    {
        int a[2][3] = {{1, 2, 3}, {4, 5, 6}};
        tr.test_eq(2, ra::rank(a));
        tr.test_eq(6, ra::size(a));
        tr.test_eq(2, ra::shape(a)[0]);
        tr.test_eq(3, ra::shape(a)[1]);
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
        tr.section("variants I wish didn't work :-/ just checking for regressions");
        {
            {
                ra::Small<int, 2> a({9});
                tr.test_eq(9, a[0]);
                tr.test_eq(9, a[1]);
            }
            {
                ra::Small<int, 2> a {9};
                tr.test_eq(9, a[0]);
                tr.test_eq(9, a[1]);
            }
            {
                ra::Small<complex, 2> a = {9.};
                tr.test_eq(9., a[0]);
                tr.test_eq(9., a[1]);
            }
        }
    }
    tr.section("empty");
    {
        ra::Small<int, 0> x;
        tr.test_eq(0, x.size());
    }
    tr.section("rank 0");
    {
        tr.section("default");
        {
            ra::Small<int> x;
            x = 3;
            tr.test_eq(3, x());
        }
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
        tr.section("= unbraced");
        {
            ra::Small<int, 2> a = {9, 3};
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
        tr.section("paren raw");
        {
            ra::Small<int, 2> a(9, 3);
            tr.test_eq(9, a[0]);
            tr.test_eq(3, a[1]);
        }
        tr.section("= scalar init");
        {
            ra::Small<int, 2> a = 9;
            tr.test_eq(9, a[0]);
            tr.test_eq(9, a[1]);
        }
        tr.section("= scalar init with non-ra::scalar type"); // [ra44]
        {
            ra::Small<test_type, 2> a = test_type { 9 };
            tr.test_eq(9, a[0].a);
            tr.test_eq(9, a[1].a);
            a = test_type { 3 };
            tr.test_eq(3, a[0].a);
            tr.test_eq(3, a[1].a);
        }
// Braced versions used to work but now they don't. This is on purpose.
        // tr.section("= braced");
        // {
        //     ra::Small<int, 2> a = {{9, 3}};
        //     tr.test_eq(9, a[0]);
        //     tr.test_eq(3, a[1]);
        // }
        // tr.section("raw braced");
        // {
        //     ra::Small<int, 2> a {{9, 3}};
        //     tr.test_eq(9, a[0]);
        //     tr.test_eq(3, a[1]);
        // }
        // tr.section("paren braced");
        // {
        //     ra::Small<int, 2> a({{9, 3}});
        //     tr.test_eq(9, a[0]);
        //     tr.test_eq(3, a[1]);
        // }
    }
    tr.section("rank 2");
    {
        tr.section("rank 2 (paren unun)");
        {
            ra::Small<double, 2, 2> a({1, 2}, {3, 4});
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (paren un)");
        {
            ra::Small<double, 2, 2> a({{1, 2}, {3, 4}});
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (= unbraced)");
        {
            ra::Small<double, 2, 2> a = {{1., 2.}, {3., 4.}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (raw unbraced) I");
        {
            ra::Small<double, 2, 2> a {{1, 2}, {3, 4}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("rank 2 (raw unbraced) II");
        {
            ra::Small<double, 2, 3> a {{1, 2, 3}, {4, 5, 6}};
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(0, 2));
            tr.test_eq(4., a(1, 0));
            tr.test_eq(5., a(1, 1));
            tr.test_eq(6., a(1, 2));
        }
        tr.section("rank 2 (paren raw)");
        {
            ra::Small<double, 2, 2> a({1, 2}, {3, 4});
            tr.test_eq(1., a(0, 0));
            tr.test_eq(2., a(0, 1));
            tr.test_eq(3., a(1, 0));
            tr.test_eq(4., a(1, 1));
        }
        tr.section("size 1, higher rank");
        {
            ra::Small<int, 1, 1> a = {{4}}; // nested
            tr.test_eq(4, a(0, 0));
            ra::Small<int, 1, 1> b = {4}; // ravel
            tr.test_eq(4, b(0, 0));
        }
        tr.section("singular sizes I");
        {
            ra::Small<int, 1, 2> a = {{4, 3}}; // nested. This requires the nested constructor with size(0)==1.
            tr.test_eq(4, a(0, 0));
            tr.test_eq(3, a(0, 1));
            ra::Small<int, 1, 2> b = {4, 3}; // ravel
            tr.test_eq(4, b(0, 0));
            tr.test_eq(3, b(0, 1));
            ra::Small<int, 1, 2> c = {4}; // scalar
            tr.test_eq(4, c(0, 0));
            tr.test_eq(4, c(0, 1));
            // ra::Small<int, 1, 3> d = {4, 2}; // ravel // [ra42] FIXME cannot check ct errors yet
            // tr.test_eq(4, b(0, 0));
            // tr.test_eq(2, b(0, 1));
        }
// Braced versions used to work but now they don't. This is on purpose.
        // tr.section("rank 2 (paren braced)");
        // {
        //     ra::Small<double, 2, 2> a({{{1, 2}, {3, 4}}});
        //     tr.test_eq(1., a(0, 0));
        //     tr.test_eq(2., a(0, 1));
        //     tr.test_eq(3., a(1, 0));
        //     tr.test_eq(4., a(1, 1));
        // }
        // tr.section("rank 2 (= braced)");
        // {
        //     ra::Small<double, 2, 2> a = {{{1, 2}, {3, 4}}};
        //     tr.test_eq(1., a(0, 0));
        //     tr.test_eq(2., a(0, 1));
        //     tr.test_eq(3., a(1, 0));
        //     tr.test_eq(4., a(1, 1));
        // }
        // tr.section("rank 2 (raw braced)");
        // {
        //     ra::Small<double, 2, 2> a {{{1, 2}, {3, 4}}};
        //     tr.test_eq(1., a(0, 0));
        //     tr.test_eq(2., a(0, 1));
        //     tr.test_eq(3., a(1, 0));
        //     tr.test_eq(4., a(1, 1));
        // }
    }
    tr.section("higher rank, unbraced");
    {
        ra::Small<int, 2, 3, 4, 2> a = {{{{ 1,  2}, { 3,  4}, { 5,  6}, { 7,  8}},
                                         {{ 9, 10}, {11, 12}, {13, 14}, {15, 16}},
                                         {{17, 18}, {19, 20}, {21, 22}, {23, 24}}},
                                        {{{25, 26}, {27, 28}, {29, 30}, {31, 32}},
                                         {{33, 34}, {35, 36}, {37, 38}, {39, 40}},
                                         {{41, 42}, {43, 44}, {45, 46}, {47, 48}}}};
        // FIXME reshape for Small or something! Big/View have these things.
        ra::Small<int, 2, 3, 4, 2> b = 0;
        ra::ViewSmall<int, int_list<48>, int_list<1>> c(b.data());
        c = ra::iota(48, 1);
        tr.test_eq(b, a);
        tr.test_eq(2, ra::shape(a, 0));
        tr.test_eq(3, ra::shape(a, 1));
        tr.test_eq(4, ra::shape(a, 2));
        tr.test_eq(2, ra::shape(a, 3));
    }
    tr.section("item constructor");
    {
        ra::Small<int, 2, 2> a = {{1, 2}, ra::iota(2, 33)};
        tr.test_eq(1, a(0, 0));
        tr.test_eq(2, a(0, 1));
        tr.test_eq(33, a(1, 0));
        tr.test_eq(34, a(1, 1));
    }
    tr.section("raveling constructor");
    {
        ra::Small<int, 2, 2> a = {1, 2, 3, 4};
        tr.test_eq(1, a(0, 0));
        tr.test_eq(2, a(0, 1));
        tr.test_eq(3, a(1, 0));
        tr.test_eq(4, a(1, 1));
    }
    tr.section("raveling constructor from iterators");
    {
        int AA[4] = { 1, 2, 3, 4 };
        auto a = ra::from_ravel<ra::Small<int, 2, 2>>(std::ranges::subrange(AA, AA+4));
        tr.test_eq(1, a(0, 0));
        tr.test_eq(2, a(0, 1));
        tr.test_eq(3, a(1, 0));
        tr.test_eq(4, a(1, 1));
        auto b = ra::from_ravel<ra::Small<int, 2, 2>>(AA);
        tr.test_eq(1, b(0, 0));
        tr.test_eq(2, b(0, 1));
        tr.test_eq(3, b(1, 0));
        tr.test_eq(4, b(1, 1));
        auto c = ra::from_ravel<ra::Small<int, 2, 2>>(ra::Small<int, 4> { 1, 2, 3, 4});
        tr.test_eq(1, c(0, 0));
        tr.test_eq(2, c(0, 1));
        tr.test_eq(3, c(1, 0));
        tr.test_eq(4, c(1, 1));
    }
    tr.section("nested Small I");
    {
        ra::Small<ra::Small<int, 2>, 2> a = {{1, 2}, {3, 4}};
        tr.test_eq(1, a(0)(0));
        tr.test_eq(2, a(0)(1));
        tr.test_eq(3, a(1)(0));
        tr.test_eq(4, a(1)(1));
    }
    tr.section("nested Small II");
    {
        auto test = [&](auto && a)
                    {
                        tr.test_eq(1, a(0)(0, 0));
                        tr.test_eq(2, a(0)(0, 1));
                        tr.test_eq(3, a(0)(0, 2));
                        tr.test_eq(4, a(0)(1, 0));
                        tr.test_eq(5, a(0)(1, 1));
                        tr.test_eq(6, a(0)(1, 2));
                        tr.test_eq(10, a(1)(0, 0));
                        tr.test_eq(20, a(1)(0, 1));
                        tr.test_eq(30, a(1)(0, 2));
                        tr.test_eq(40, a(1)(1, 0));
                        tr.test_eq(50, a(1)(1, 1));
                        tr.test_eq(60, a(1)(1, 2));
                    };
        using In = ra::Small<int, 2, 3>;
        ra::Small<In, 2> a = {{{1, 2, 3}, {4, 5, 6}}, {{10, 20, 30}, {40, 50, 60}}};
        test(a);
        ra::Small<In, 2> b = {In {{1, 2, 3}, {4, 5, 6}}, In {{10, 20, 30}, {40, 50, 60}}};
        test(b);
        int x[2][3] = {{1, 2, 3}, {4, 5, 6}};
        int y[2][3] = {{10, 20, 30}, {40, 50, 60}};
        ra::Small<In, 2> c = {x, y};
        test(c);
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
        // tr.section("operator= rank 2 braced"); // Doesn't work but cannot check ct errors [ra42]
        // {
        //     ra::Small<int, 2, 2> a = 0;
        //     a = {{{1, 2}, {3, 4}}};
        //     tr.test_eq(1, a(0, 0));
        //     tr.test_eq(2, a(0, 1));
        //     tr.test_eq(3, a(1, 0));
        //     tr.test_eq(4, a(1, 1));
        // }
        tr.section("operator= of view into view");
        {
            {
                ra::Small<int, 2, 2> a = {{1, 2}, {3, 4}};
                ra::ViewSmall<int, int_list<2>, int_list<1>> a0 = a(0);
                ra::ViewSmall<int, int_list<2>, int_list<1>> a1 = a(1);
                a0 = a1;
                tr.test_eq(ra::Small<int, 2, 2> {{3, 4}, {3, 4}}, a);
            }
            {
                ra::Small<int, 2, 2> a = {{1, 2}, {3, 4}};
                ra::ViewSmall<int, int_list<2>, int_list<1>> a0 = a(0);
                a0 = a(1);
                tr.test_eq(ra::Small<int, 2, 2> {{3, 4}, {3, 4}}, a);
            }
        }
        tr.section("repeat a test from test/big-0 [ra38]");
        {
            ra::Small<double, 2> A = {1, 2};
            ra::Small<double, 2> X = {0, 0};
            X(ra::all) = A();
            tr.test_eq(ra::start({1, 2}), X);
        }
        tr.section("operator=(scalar) on non-unit steps");
        {
            ra::Small<int, 2, 2> a(0);
            diag(a) = 1;
            tr.test_eq(ra::Small<int, 2, 2> {{1, 0}, {0, 1}}, a);
        }
    }
// These tests should fail at compile time. No way to check them yet [ra42].
    // tr.section("size checks");
    // {
    //     ra::Small<int, 3> a = { 1, 2, 3 };
    //     ra::Small<int, 2> b = { 4, 5 };
    //     ra::Small<int, 3> c = a+b;
    //     cout << c << endl;
    // }
// self type assigment
    {
        ra::Small<int, 3> a = 0;
        ra::Small<int, 3> b = {9, 8, 7};
        ra::Small<int, 3> c = {9, 8, 7};
        auto pa = a.data();
        tr.test_eq(a, 0);
        cout << typecheck(a()) << endl;
        a() = b();
        tr.test_eq(a, c);
        tr.test_eq(ra::scalar(a.data()), ra::scalar(pa));
        a = 0;
        tr.test_eq(a, 0);
        cout << typecheck(a()) << endl;
        a = b;
        tr.test_eq(a, c);
        tr.test_eq(ra::scalar(a.data()), ra::scalar(pa));
    }
// ra::shape / ra::size are static for Small types
    {
        ra::Small<int, 3, 4> a = 0;
        tr.test_eq(2, int_c<rank(a)>::value);
        tr.test_eq(3, int_c<shape(a)[0]>::value);
        tr.test_eq(4, int_c<shape(a)[1]>::value);
// FIXME std::size makes this ambiguous without the qualifier, which looks wrong to me :-/
        tr.test_eq(12, int_c<ra::size(a)>::value);
    }
    return tr.summary();
}
