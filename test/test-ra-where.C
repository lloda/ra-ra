
// (c) Daniel Llorens - 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Regression test for a bug in where() when both T/F arms are Expr.

#include "ra/ra-operators.H"
#include "ra/test.H"

using std::cout; using std::endl;
using real = double;

int main()
{
    TestRecorder tr(std::cout);
    section("where, scalar W, array arguments in T/F");
    {
        std::array<real, 2> bb {{1, 2}};
        std::array<real, 2> cc {{99, 99}};
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
    section("where as rvalue");
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
    section("where nested");
    {
        {
            ra::Small<int, 3> a {-1, 0, 1};
            ra::Small<int, 3> b = where(a>=0, where(a>=1, 99, 77), 44);
            tr.test_eq(ra::Small<int, 3> {44, 77, 99}, b);
        }
        {
            int a = 0;
            ra::Small<int, 2, 2> b = where(a>=0, where(a>=1, 99, 77), 44);
            tr.test_eq(ra::Small<int, 2, 2> {77, 77, 77, 77}, b);
        }
    }
    section("where, scalar W, array arguments in T/F");
    {
        real a = 1./7;
        ra::Small<real, 2> b {1, 2};
        ra::Small<real, 2> c = where(a>0, b, 3.);
        tr.test_eq(ra::Small<real, 2> {1, 2}, c);
    }
    section("where as lvalue, scalar");
    {
        real a=0, b=0;
        bool w = true;
        where(w, a, b) = 99;
        tr.test_eq(a, 99);
        tr.test_eq(b, 0);
        where(!w, a, b) = 77;
        tr.test_eq(99, a);
        tr.test_eq(77, b);
    }
    section("where, scalar + rank 0 array");
    {
        ra::Small<real> a { 33. };
        real b = 22.;
        tr.test_eq(33, real(where(true, a, b)));
        tr.test_eq(22, real(where(true, b, a)));
    }
    section("where as lvalue, xpr [raop01]");
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
    return tr.summary();
}
