// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for where() and pick().

// (c) Daniel Llorens - 2014-2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <atomic>
#include "ra/test.hh"

using std::cout, std::endl, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);

    std::atomic<int> counter { 0 };
    auto count = [&counter](auto && x) -> decltype(auto) { ++counter; return x; };

    tr.section("pick");
    {
        ra::Small<double, 3> a0 = { 1, 2, 3 };
        ra::Small<double, 3> a1 = { 10, 20, 30 };
        ra::Small<int, 3> p = { 0, 1, 0 };
        ra::Small<double, 3> a(0.);

        counter = 0;
        a = pick(p, map(count, a0), map(count, a1));
        tr.test_eq(ra::Small<double, 3> { 1, 20, 3 }, a);
        tr.info("pick ETs execute only one branch per iteration").test_eq(3, int(counter));

        counter = 0;
        a = ra::where(p, map(count, a0), map(count, a1));
        tr.test_eq(ra::Small<double, 3> { 10, 2, 30 }, a);
        tr.info("where() is implemented using pick ET").test_eq(3, int(counter));
    }
    tr.section("write to pick");
    {
        ra::Small<double, 2> a0 = { 1, 2 };
        ra::Small<double, 2> a1 = { 10, 20 };
        ra::Small<int, 2> const p = { 0, 1 };
        ra::Small<double, 2> const a = { 7, 9 };

        counter = 0;
        pick(p, map(count, a0), map(count, a1)) = a;
        tr.test_eq(2, int(counter));
        tr.test_eq(ra::Small<double, 2> { 7, 2 }, a0);
        tr.test_eq(ra::Small<double, 2> { 10, 9 }, a1);
        tr.test_eq(ra::Small<double, 2> { 7, 9 }, a);
        tr.test_eq(ra::Small<int, 2> { 0, 1 }, p);
    }
    tr.section("pick works as any other array expression");
    {
        ra::Small<double, 2> a0 = { 1, 2 };
        ra::Small<double, 2> const a1 = { 10, 20 };
        ra::Small<int, 2> const p = { 0, 1 };

        ra::Small<double, 2> q = 3 + pick(p, a0, a1);
        tr.test_eq(ra::Small<int, 2> { 4, 23 }, q);
    }
    tr.section("pick with undefined len iota");
    {
        ra::Small<double, 2> a0 = { 1, 2 };
        ra::Small<double, 2> a1 = { 10, 20 };
        ra::Small<int, 2> const p = { 0, 1 };

        counter = 0;
        pick(p, map(count, a0), map(count, a1)) += ra::_0+5;
        tr.test_eq(2, int(counter));
        tr.test_eq(ra::Small<double, 2> { 6, 2 }, a0);
        tr.test_eq(ra::Small<double, 2> { 10, 26 }, a1);
        tr.test_eq(ra::Small<int, 2> { 0, 1 }, p);
    }
    tr.section("where, scalar W, array arguments in T/F");
    {
        std::array<double, 2> bb {1, 2};
        std::array<double, 2> cc {99, 99};
        auto b = ra::start(bb);
        auto c = ra::start(cc);

        cc[0] = cc[1] = 99;
// pick_star
        c = ra::where(true, b, -b);
        tr.test_eq(1, cc[0]);
        tr.test_eq(2, cc[1]);

// pick_at
        tr.test_eq(1, ra::where(true, b, -b).at(std::array {0}));

// test against a bug where the op in where()'s Map returned a dangling reference when both its args are rvalue refs. This was visible only at certain -O levels.
        cc[0] = cc[1] = 99;
        c = ra::where(true, b-3, -b);
        tr.test_eq(-2, cc[0]);
        tr.test_eq(-1, cc[1]);
    }
    tr.section("where as rvalue");
    {
        tr.test_eq(ra::Unique<int, 1> { 1, 2, 2, 1 }, ra::where(ra::Unique<bool, 1> { true, false, false, true }, 1, 2));
        tr.test_eq(ra::Unique<int, 1> { 17, 2, 3, 17 }
, ra::where(ra::_0>0 && ra::_0<3, ra::Unique<int, 1> { 1, 2, 3, 4 }, 17));
// [raop00] undef len iota returs value; so where()'s lambda must also return value.
        tr.test_eq(ra::Unique<int, 1> { 1, 2, 4, 7 }, ra::where(ra::Unique<bool, 1> { true, false, false, true }, 2*ra::_0+1, 2*ra::_0));
// Using frame matching... TODO directly with ==map?
        ra::Unique<int, 2> a({4, 3}, ra::_0-ra::_1);
        ra::Unique<int, 2> b = ra::where(ra::Unique<bool, 1> { true, false, false, true }, 99, a);
        tr.test_eq(ra::Unique<int, 2> ({4, 3}, { 99, 99, 99, 1, 0, -1, 2, 1, 0, 99, 99, 99 }), b);
    }
    tr.section("where nested");
    {
        {
            ra::Small<int, 3> a {-1, 0, 1};
            ra::Small<int, 3> b = ra::where(a>=0, ra::where(a<1, 77, 99), 44);
            tr.test_eq(ra::Small<int, 3> {44, 77, 99}, b);
        }
        {
            int a = 0;
            ra::Small<int, 2, 2> b = ra::where(a>=0, ra::where(a>=1, 99, 77), 44);
            tr.test_eq(ra::Small<int, 2, 2> {77, 77, 77, 77}, b);
        }
    }
    tr.section("where, scalar W, array arguments in T/F");
    {
        double a = 1./7;
        ra::Small<double, 2> b {1, 2};
        ra::Small<double, 2> c = ra::where(a>0, b, 3.);
        tr.test_eq(ra::Small<double, 2> {1, 2}, c);
    }
    tr.section("where as lvalue, scalar");
    {
        double a=0, b=0;
        bool w = true;
        ra::where(w, a, b) = 99;
        tr.test_eq(a, 99);
        tr.test_eq(b, 0);
        ra::where(!w, a, b) = 77;
        tr.test_eq(99, a);
        tr.test_eq(77, b);
    }
    tr.section("where, scalar + rank 0 array");
    {
        ra::Small<double> a { 33. };
        double b = 22.;
        tr.test_eq(33, double(ra::where(true, a, b)));
        tr.test_eq(22, double(ra::where(true, b, a)));
    }
    tr.section("where as lvalue, xpr [raop01]");
    {
        ra::Unique<int, 1> a { 0, 0, 0, 0 };
        ra::Unique<int, 1> b { 0, 0, 0, 0 };
        ra::where(ra::_0>0 && ra::_0<3, a, b) = 7;
        tr.test_eq(ra::Unique<int, 1> {0, 7, 7, 0}, a);
        tr.test_eq(ra::Unique<int, 1> {7, 0, 0, 7}, b);
        ra::where(ra::_0<=0 || ra::_0>=3, a, b) += 2;
        tr.test_eq(ra::Unique<int, 1> {2, 7, 7, 2}, a);
        tr.test_eq(ra::Unique<int, 1> {7, 2, 2, 7}, b);
// Both must be lvalues; TODO check that either of these is an error.
        // ra::where(ra::_0>0 && ra::_0<3, ra::_0, a) = 99;
        // ra::where(ra::_0>0 && ra::_0<3, a, ra::_0) = 99;
    }
    tr.section("where with rvalue iota<n>(), fails to compile with g++ 5.2 -Os, gives wrong result with -O0");
    {
        tr.test_eq(ra::Small<int, 2> {0, 1}, ra::where(ra::Unique<bool, 1> { true, false }, ra::iota<0>(), ra::iota<0>()));
        tr.test_eq(ra::Unique<int, 1> { 0, 2 }, ra::where(ra::Unique<bool, 1> { true, false }, 3*ra::_0, 2*ra::_0));
    }
    tr.section("&& and || are short-circuiting");
    {
        using bool4 = ra::Small<bool, 4>;
        bool4 a {true, true, false, false}, b {true, false, true, false};
        int i = 0;
        tr.test_eq(bool4 {true, false, false, false}, a && map([&](auto && b) { ++i; return b; }, b));
        tr.info("short circuit test for &&").test_eq(2, i);
        i = 0;
        tr.test_eq(bool4 {true, true, true, false}, a || map([&](auto && b) { ++i; return b; }, b));
        tr.info("short circuit test for &&").test_eq(2, i);
    }
// These tests should fail at compile time. No way to check them yet [ra42].
    // tr.section("size checks");
    // {
    //     ra::Small<int, 3> a = { 1, 2, 3 };
    //     ra::Small<int, 3> b = { 4, 5, 6 };
    //     ra::Small<int, 2> c = 0; // ok if 2 -> 3; the test is for that case.
    //     ra::where(a>b, a, c) += b;
    //     tr.test_eq(ra::Small<int, 3> { 1, 2, 3 }, a);
    //     tr.test_eq(ra::Small<int, 3> { 4, 5, 6 }, b);
    // }
    return tr.summary();
}
