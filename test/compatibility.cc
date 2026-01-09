// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Using builtin types and std:: together with ra::.

// (c) Daniel Llorens - 2014, 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, std::string, ra::TestRecorder;
using ra::int_c, ra::ilist_t;

int main()
{
    TestRecorder tr;
    tr.section("Tests for std:: types");
    {
        tr.section("frame match ra::iter on 1st axis");
        {
            std::vector const a = { 1, 2, 3 };
            ra::Big<int, 2> b ({3, 2}, ra::iter(a));
            tr.test_eq(a[0], b(0));
            tr.test_eq(a[1], b(1));
            tr.test_eq(a[2], b(2));
         }
// TODO actually whether unroll is avoided depends on ply, have a way to require it [ra3]
        tr.section("frame match ra::iter on 1st axis, forbid unroll");
        {
            std::vector const a = { 1, 2, 3 };
            ra::Big<int, 3> b ({3, 4, 2}, ra::none);
            transpose(b, {0, 2, 1}) = ra::iter(a);
            tr.test_eq(a[0], b(0));
            tr.test_eq(a[1], b(1));
            tr.test_eq(a[2], b(2));
        }
        tr.section("frame match ra::iter on some axis other than 1st");
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
        tr.section("= operators on ra::iter");
        {
            std::vector a { 1, 2, 3 };
            ra::iter(a) *= 3;
            tr.test_eq(ra::iter(std::vector { 3, 6, 9 }), ra::iter(a));
        }
        tr.section("automatic conversion of foreign vectors in mixed ops");
        {
            std::vector a { 1, 2, 3 };
            ra::Big<int, 1> b { 10, 20, 30 };
            tr.test_eq(ra::iter({11, 22, 33}), a+b);
        }
    }
    tr.section("builtin arrays as foreign arrays");
    {
        int const a[] = {1, 2, 3};
        tr.info("builtin array is enough to drive").test_eq(ra::iter({1, 3, 5}), (ra::_0 + a));
        int const b[][3] = {{1, 2, 3}, {4, 5, 6}};
        tr.info("builtin array handles 2 dimensions").test_eq(ra::Small<int, 2, 3>{1, 1, 1,  5, 5, 5}, (ra::_0 + b - ra::_1));
        int const c[2][2][2][2] = {{{{0, 1}, {2, 3}}, {{4, 5}, {6, 7}}}, {{{8, 9}, {10, 11}}, {{12, 13}, {14, 15}}}};
        tr.info("builtin array handles 4 dimensions").test_eq(ra::Small<int, 2, 2, 2, 2>(ra::_0*8 + ra::_1*4 + ra::_2*2 + ra::_3), c);
        // ra::iter(c) = 99; // FIXME test that this fails at ct.
    }
    tr.section("builtin arrays shape/size/rank");
    {
// cf small-0.cc
        int a[3][4] = {};
        tr.test_eq(2, int_c<ra::rank(a)>::value);
        tr.test_eq(3, int_c<ra::shape(a)[0]>::value);
        tr.test_eq(4, int_c<ra::shape(a)[1]>::value);
        tr.test_eq(12, int_c<ra::size(a)>::value);
    }
// shape on std:: types
    {
        tr.test_eq(ra::iter({3}), ra::shape(std::array<int, 3> {1, 2, 3}));
        tr.test_eq(ra::iter({4}), ra::shape(std::vector<int> {1, 2, 3, 4}));
    }

    tr.section("operators take foreign types");
    {
        std::vector x = {1, 2, 3};
        tr.test_eq(6, sum(ra::iter(x)));
        tr.test_eq(6, ra::sum(x));
    }
    tr.section("spot use of scalar");
    {
        struct W { int x; };
        ra::Big<W, 1> w = { {1}, {2} };
        tr.test_eq(ra::iter({8, 9}), map([](auto && a, auto && b) { return a.x + b.x; }, w, ra::scalar(W {7})));
        w() = W {3};
        tr.test_eq(3, map([](auto && a) { return a.x; }, w));
    }
    {
        int o[4];
        using O = decltype(o);
        O p[2];
        int q[2][4];

        cout << ra::mp::type_name<decltype(o)>() << endl;
        cout << ra::mp::type_name<decltype(p)>() << endl;
        cout << ra::mp::type_name<decltype(q)>() << endl;
        cout << ra::mp::type_name<decltype(ra::iter(q))>() << endl;
        cout << ra::mp::type_name<std::remove_all_extents_t<decltype(q)>>() << endl;
    }
    {
        int o[2];
        static_assert(1==ra::rank(o));
        static_assert(2==ra::iter(o).len(0));

        int p[3][2];
        static_assert(2==ra::rank(p));
        static_assert(3==ra::shape(p)[0]);
        static_assert(2==ra::shape(p)[1]);

        int q[4][3][2];
        static_assert(3==ra::rank(q));
        static_assert(4==ra::shape(q)[0]);
        static_assert(3==ra::shape(q)[1]);
        static_assert(2==ra::shape(q)[2]);

        static_assert(3==ra::iter(q).rank());
        static_assert(4==ra::iter(q).len(0));
        static_assert(3==ra::iter(q).len(1));
        static_assert(2==ra::iter(q).len(2));

        int r[][2] = {{1, 2}, {3, 4}};
        static_assert(std::rank<decltype(r)>::value==2);
        static_assert(2==ra::rank(r));
        static_assert(2==ra::shape(r)[0]);
        static_assert(2==ra::shape(r)[1]);
    }
    tr.section("example from the manual [ma106]");
    {
        int p[] = {1, 2, 3};
        int * z = p;
        ra::Big<int, 1> q {1, 2, 3};
        q += p; // ok, q is ra::, p is foreign object with size info
        tr.test_eq(ra::iter({2, 4, 6}), q);
        ra::iter(p) += q; // can't redefine operator+=(int[]), foreign needs ra::iter()
        tr.test_eq(ra::iter({3, 6, 9}), p);
        // z += q; // error: raw pointer needs ra::ptr()
        ra::ptr(z) += p; // ok, size is determined by foreign object p
        tr.test_eq(ra::iter({6, 12, 18}), p);
    }
    tr.section("char arrays");
    {
        auto quote = [](auto && o) { return ra::format(o); };
        {
            char hello[] = "hello";
            tr.test_eq(string("hello"), quote(hello)); // not ra:: types
            tr.test_eq(std::vector<char> {'h', 'e', 'l', 'l', 'o', 0}, quote(ra::scalar(hello)));
            tr.test_eq(std::vector<char> {'h', 'e', 'l', 'l', 'o', 0}, ra::iter(hello));
            tr.test_eq(6, size_s(ra::iter(hello)));
            tr.test_eq(6, size_s(ra::ptr(hello)));
            tr.test_eq(ra::ptr(string("hello\0")), ra::ptr((char *)hello)); // char by char
        }
        cout << endl;
        {
            char const * hello = "hello";
            tr.test_eq(string("hello"), quote(hello));
            tr.test_eq(ra::scalar(string("hello")), ra::scalar(hello));
            // cout << ra::iter(hello) << endl; // error, cannot be iter()ed
            // cout << ra::ptr(hello) << endl; // error, pointer has no size
        }
    }
    return tr.summary();
}
