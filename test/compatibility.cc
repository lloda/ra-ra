// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Using builtin types and std:: together with ra::.

// (c) Daniel Llorens - 2014, 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <ranges>
#include <iostream>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, std::string, ra::TestRecorder;
using ra::mp::int_c, ra::mp::int_list;

int main()
{
    TestRecorder tr;
    tr.section("Tests for std:: types");
    {
        tr.section("frame match ra::start on 1st axis");
        {
            std::vector const a = { 1, 2, 3 };
            ra::Big<int, 2> b ({3, 2}, ra::start(a));
            tr.test_eq(a[0], b(0));
            tr.test_eq(a[1], b(1));
            tr.test_eq(a[2], b(2));
         }
// TODO actually whether unroll is avoided depends on ply, have a way to require it [ra3]
        tr.section("frame match ra::start on 1st axis, forbid unroll");
        {
            std::vector const a = { 1, 2, 3 };
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
                std::vector const a = { 10, 20, 30 };
                std::vector const b = { 1, 2 };
                ra::Big<int, 2> c = map(ra::wrank<0, 1>([](int a, int b) { return a + b; }), a, b);
                tr.test_eq(ra::Big<int, 2>({3, 2}, {11, 12, 21, 22, 31, 32}), c);
            }
        }
        tr.section("= operators on ra::start");
        {
            std::vector a { 1, 2, 3 };
            ra::start(a) *= 3;
            tr.test_eq(ra::start(std::vector { 3, 6, 9 }), ra::start(a));
        }
        tr.section("automatic conversion of foreign vectors in mixed ops");
        {
            std::vector a { 1, 2, 3 };
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
    tr.section("builtin arrays shape/size/rank");
    {
// cf small-0.cc
        int a[3][4] = {};
        tr.test_eq(2, int_c<ra::rank(a)>::value);
        tr.test_eq(3, int_c<ra::shape(a)(0)>::value);
        tr.test_eq(4, int_c<ra::shape(a)(1)>::value);
        tr.test_eq(12, int_c<ra::size(a)>::value);
    }
    tr.section("operators take foreign types");
    {
        std::vector x = {1, 2, 3};
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

        cout << ra::mp::type_name<decltype(o)>() << endl;
        cout << ra::mp::type_name<decltype(p)>() << endl;
        cout << ra::mp::type_name<decltype(q)>() << endl;
        cout << ra::mp::type_name<decltype(ra::start(q))>() << endl;
        cout << ra::mp::type_name<std::remove_all_extents_t<decltype(q)>>() << endl;
    }
    {
        int o[2];
        int p[3][2];
        int q[4][3][2];
        int r[][2] = {{1, 2}, {3, 4}};
        static_assert(std::is_same<ra::builtin_array_lens_t<decltype(o)>,
                      int_list<2>>::value);
        static_assert(std::is_same<ra::builtin_array_lens_t<decltype(p)>,
                      int_list<3, 2>>::value);
        static_assert(std::is_same<ra::builtin_array_lens_t<decltype(q)>,
                      int_list<4, 3, 2>>::value);
        static_assert(std::is_same<ra::builtin_array_lens_t<decltype(r)>,
                      int_list<2, 2>>::value);
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
    tr.section("char arrays");
    {
        auto quote = [](auto && o) { return ra::format(o); };
        {
            char hello[] = "hello";
            tr.test_eq(string("hello"), quote(hello)); // not ra:: types
            tr.test_eq(string("hello"), quote(ra::scalar(hello)));
            tr.test_eq(std::vector<char> {'h', 'e', 'l', 'l', 'o', 0}, ra::start(hello));
            tr.test_eq(6, size_s(ra::start(hello)));
            tr.test_eq(6, size_s(ra::vector(hello))); // [ra2]
            tr.test_eq(ra::vector(string("hello\0")), ra::ptr(hello, 5)); // char by char
        }
        cout << endl;
        {
            char const * hello = "hello";
            tr.test_eq(string("hello"), quote(hello));
            tr.test_eq(ra::scalar(string("hello")), ra::scalar(hello));
            // cout << ra::start(hello) << endl; // cannot be start()ed
            // cout << ra::vector(hello) << endl; // same, but FIXME improve error message
        }
    }
    return tr.summary();
}
