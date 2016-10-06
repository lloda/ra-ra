
// (c) Daniel Llorens - 2014-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-where.C
/// @brief Tests for where() and pick().

#include <atomic>
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"

using std::cout; using std::endl;

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
        a = where(p, map(count, a0), map(count, a1));
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
        ra::Small<double, 2> a1 = { 10, 20 };
        ra::Small<int, 2> const p = { 0, 1 };

        ra::Small<double, 2> q = 3 + pick(p, a0, a1);
        tr.test_eq(ra::Small<int, 2> { 4, 23 }, q);
    }
    tr.section("pick with TensorIndex");
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
        std::array<double, 2> bb {{1, 2}};
        std::array<double, 2> cc {{99, 99}};
        auto b = ra::vector(bb);
        auto c = ra::vector(cc);

        cc[0] = cc[1] = 99;
        c = where(true, b, -b);
        tr.test_eq(1, cc[0]);
        tr.test_eq(2, cc[1]);

// test against a bug where the op in where()'s Expr returned a dangling reference when both its args are rvalue refs. This was visible only at certain -O levels.
        cc[0] = cc[1] = 99;
        c = where(true, b-3, -b);
        tr.test_eq(-2, cc[0]);
        tr.test_eq(-1, cc[1]);
    }
    tr.section("where as rvalue");
    {
        tr.test_eq(ra::Unique<int, 1> { 1, 2, 2, 1 }, where(ra::Unique<bool, 1> { true, false, false, true }, 1, 2));
        tr.test_eq(ra::Unique<int, 1> { 17, 2, 3, 17 }
, where(ra::_0>0 && ra::_0<3, ra::Unique<int, 1> { 1, 2, 3, 4 }, 17));
// [raop00] TensorIndex returs value; so where()'s lambda must also return value.
        tr.test_eq(ra::Unique<int, 1> { 1, 2, 4, 7 }, where(ra::Unique<bool, 1> { true, false, false, true }, 2*ra::_0+1, 2*ra::_0));
// Using frame matching... @TODO directly with ==expr?
        ra::Unique<int, 2> a({4, 3}, ra::_0-ra::_1);
        ra::Unique<int, 2> b = where(ra::Unique<bool, 1> { true, false, false, true }, 99, a);
        tr.test_eq(ra::Unique<int, 2> ({4, 3}, { 99, 99, 99, 1, 0, -1, 2, 1, 0, 99, 99, 99 }), b);
    }
    tr.section("where nested");
    {
        {
            ra::Small<int, 3> a {-1, 0, 1};
            ra::Small<int, 3> b = where(a>=0, where(a<1, 77, 99), 44);
            tr.test_eq(ra::Small<int, 3> {44, 77, 99}, b);
        }
        {
            int a = 0;
            ra::Small<int, 2, 2> b = where(a>=0, where(a>=1, 99, 77), 44);
            tr.test_eq(ra::Small<int, 2, 2> {77, 77, 77, 77}, b);
        }
    }
    tr.section("where, scalar W, array arguments in T/F");
    {
        double a = 1./7;
        ra::Small<double, 2> b {1, 2};
        ra::Small<double, 2> c = where(a>0, b, 3.);
        tr.test_eq(ra::Small<double, 2> {1, 2}, c);
    }
    tr.section("where as lvalue, scalar");
    {
        double a=0, b=0;
        bool w = true;
        where(w, a, b) = 99;
        tr.test_eq(a, 99);
        tr.test_eq(b, 0);
        where(!w, a, b) = 77;
        tr.test_eq(99, a);
        tr.test_eq(77, b);
    }
    tr.section("where, scalar + rank 0 array");
    {
        ra::Small<double> a { 33. };
        double b = 22.;
        tr.test_eq(33, double(where(true, a, b)));
        tr.test_eq(22, double(where(true, b, a)));
    }
    tr.section("where as lvalue, xpr [raop01]");
    {
        ra::Unique<int, 1> a { 0, 0, 0, 0 };
        ra::Unique<int, 1> b { 0, 0, 0, 0 };
        where(ra::_0>0 && ra::_0<3, a, b) = 7;
        tr.test_eq(ra::Unique<int, 1> {0, 7, 7, 0}, a);
        tr.test_eq(ra::Unique<int, 1> {7, 0, 0, 7}, b);
        where(ra::_0<=0 || ra::_0>=3, a, b) += 2;
        tr.test_eq(ra::Unique<int, 1> {2, 7, 7, 2}, a);
        tr.test_eq(ra::Unique<int, 1> {7, 2, 2, 7}, b);
// Both must be lvalues; @TODO check that either of these is an error.
        // where(ra::_0>0 && ra::_0<3, ra::_0, a) = 99;
        // where(ra::_0>0 && ra::_0<3, a, ra::_0) = 99;
    }
    tr.section("where with rvalue TensorIndex, fails to compile with g++ 5.2 -Os, gives wrong result with -O0");
    {
        tr.test_eq(ra::Small<int, 2> {0, 1},
                   where(ra::Unique<bool, 1> { true, false }, ra::TensorIndex<0>(), ra::TensorIndex<0>()));
        tr.test_eq(ra::Unique<int, 1> { 0, 2 }, where(ra::Unique<bool, 1> { true, false }, 3*ra::_0, 2*ra::_0));
    }
    return tr.summary();
}
