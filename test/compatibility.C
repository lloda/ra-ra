// -*- mode: c++; coding: utf-8 -*-
/// @file compatibility.C
/// @brief Using std:: and ra:: together.

// (c) Daniel Llorens - 2014, 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/mpdebug.H"

using std::cout, std::endl, std::flush;

int main()
{
    TestRecorder tr;
    tr.section("Tests for std:: types");
    {
        tr.section("frame match ra::start on 1st axis");
        {
            std::vector<int> const a = { 1, 2, 3 };
            ra::Big<int, 2> b ({3, 2}, ra::start(a));
            tr.test_eq(a[0], b(0));
            tr.test_eq(a[1], b(1));
            tr.test_eq(a[2], b(2));
         }
// TODO actually whether unroll is avoided depends on ply, have a way to require it [ra03]
        tr.section("frame match ra::start on 1st axis, forbid unroll");
        {
            std::vector<int> const a = { 1, 2, 3 };
            ra::Big<int, 3> b ({3, 4, 2}, ra::none);
            transpose({0, 2, 1}, b) = ra::start(a);
            tr.test_eq(a[0], b(0));
            tr.test_eq(a[1], b(1));
            tr.test_eq(a[2], b(2));
        }
        tr.section("frame match ra::start on some axis other than 1st");
        {
            {
                ra::Big<int, 1> const a = { 10, 20, 30 };
                ra::Big<int, 1> const b = { 1, 2 };
                ra::Big<int, 2> c = map(ra::wrank<0, 1>([](int a, int b) { return a + b; }), a, b);
                tr.test_eq(ra::Big<int, 2>({3, 2}, {11, 12, 21, 22, 31, 32}), c);
            }
            {
                std::vector<int> const a = { 10, 20, 30 };
                std::vector<int> const b = { 1, 2 };
                ra::Big<int, 2> c = map(ra::wrank<0, 1>([](int a, int b) { return a + b; }), a, b);
                tr.test_eq(ra::Big<int, 2>({3, 2}, {11, 12, 21, 22, 31, 32}), c);
            }
        }
        tr.section("= operators on ra::start");
        {
            std::vector<int> a { 1, 2, 3 };
            ra::start(a) *= 3;
            tr.test_eq(ra::start(std::vector<int> { 3, 6, 9 }), ra::start(a));
        }
        tr.section("automatic conversion of foreign vectors in mixed ops");
        {
            std::vector<int> a { 1, 2, 3 };
            ra::Big<int, 1> b { 10, 20, 30 };
            tr.test_eq(ra::start({11, 22, 33}), a+b);
        }
    }
    tr.section("builtin arrays as foreign arrays");
    {
        int const a[] = {1, 2, 3};
        tr.info("builtin array is enough to drive").test_eq(ra::start({1, 3, 5}), (ra::_0 + a));
        int const b[][3] = {{1, 2, 3}, {4, 5, 6}};
        tr.info("builtin array handles 2 dimensions").test_eq(ra::Small<int, 2, 3>{1, 1, 1,  5, 5, 5}, (ra::_0 + b - ra::_1));
        int const c[2][2][2][2] = {{{{0, 1}, {2, 3}}, {{4, 5}, {6, 7}}}, {{{8, 9}, {10, 11}}, {{12, 13}, {14, 15}}}};
        tr.info("builtin array handles 4 dimensions").test_eq(ra::Small<int, 2, 2, 2, 2>(ra::_0*8 + ra::_1*4 + ra::_2*2 + ra::_3), c);
        // ra::start(c) = 99; // FIXME test that this fails at ct.
    }
    tr.section("operators take foreign types");
    {
        std::vector<int> x = {1, 2, 3};
        tr.test_eq(6, sum(ra::start(x)));
        tr.test_eq(6, ra::sum(x));
    }
    tr.section("spot use of scalar");
    {
        struct W { int x; };
        ra::Big<W, 1> w = { {1}, {2} };
        tr.test_eq(ra::start({8, 9}), map([](auto && a, auto && b) { return a.x + b.x; }, w, ra::scalar(W {7})));
    }
    {
        int o[4];
        using O = decltype(o);
        O p[2];
        int q[2][4];

        cout << mp::type_name<decltype(o)>() << endl;
        cout << mp::type_name<decltype(p)>() << endl;
        cout << mp::type_name<decltype(q)>() << endl;
        cout << mp::type_name<decltype(ra::start(q))>() << endl;
        cout << mp::type_name<std::remove_all_extents_t<decltype(q)>>() << endl;
    }
    {
        int o[2];
        int p[3][2];
        int q[4][3][2];
        int r[][2] = {{1, 2}, {3, 4}};
        static_assert(std::is_same<ra::builtin_array_sizes_t<decltype(o)>,
                      mp::int_list<2>>::value);
        static_assert(std::is_same<ra::builtin_array_sizes_t<decltype(p)>,
                      mp::int_list<3, 2>>::value);
        static_assert(std::is_same<ra::builtin_array_sizes_t<decltype(q)>,
                      mp::int_list<4, 3, 2>>::value);
        static_assert(std::is_same<ra::builtin_array_sizes_t<decltype(r)>,
                      mp::int_list<2, 2>>::value);
        static_assert(std::rank<decltype(r)>::value==2);
    }
    tr.section("example from the manual [ma106]");
    {
        int p[] = {1, 2, 3};
        int * z = p;
        ra::Big<int, 1> q {1, 2, 3};
        q += p; // ok, q is ra::, p is foreign object with size info
        tr.test_eq(ra::start({2, 4, 6}), q);
        ra::start(p) += q; // can't redefine operator+=(int[]), foreign needs ra::start()
        tr.test_eq(ra::start({3, 6, 9}), p);
        // z += q; // error: raw pointer needs ra::ptr()
        ra::ptr(z) += p; // ok, size is determined by foreign object p
        tr.test_eq(ra::start({6, 12, 18}), p);
    }
    return tr.summary();
}
