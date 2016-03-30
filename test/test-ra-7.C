
// (c) Daniel Llorens - 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// @TODO Check that Iota+scalar, etc. is also a Iota. Currently, it's an Expr,
// because the ops below are overriden by the generic ops in ra-operators.H.

#define RA_OPTIMIZE 0 // disable automatic use, so we can compare with (forced) and without
#define RA_OPTIMIZE_IOTA 1 // enable all definitions

#ifndef RA_OPTIMIZE_SMALLVECTOR // test is for 1; forcing 0 makes that part of the test moot
#define RA_OPTIMIZE_SMALLVECTOR 1
#endif

#include "ra/ra-operators.H"
#include "ra/test.H"

using std::cout; using std::endl;

int main()
{
    TestRecorder tr(std::cout);
    auto i = ra::jvec(5);
    section("optimize is nop by default");
    {
        auto l = optimize(i*i);
        tr.test_eq(ra::vector({0, 1, 4, 9, 16}), l);
    }
    section("operations with Iota, plus");
    {
        auto j = i+1;
        auto k1 = optimize(i+1);
        auto k2 = optimize(1+i);
        auto k3 = optimize(ra::jvec(5)+1);
        auto k4 = optimize(1+ra::jvec(5));
// it's actually a Iota
        tr.test_eq(1, k1.org_);
        tr.test_eq(1, k2.org_);
        tr.test_eq(1, k3.org_);
        tr.test_eq(1, k4.org_);
        tr.test_eq(ra::vector({1, 2, 3, 4, 5}), j);
        tr.test_eq(ra::vector({1, 2, 3, 4, 5}), k1);
        tr.test_eq(ra::vector({1, 2, 3, 4, 5}), k2);
        tr.test_eq(ra::vector({1, 2, 3, 4, 5}), k3);
        tr.test_eq(ra::vector({1, 2, 3, 4, 5}), k4);
    }
    section("operations with Iota, times");
    {
        auto j = i*2;
        auto k1 = optimize(i*2);
        auto k2 = optimize(2*i);
        auto k3 = optimize(ra::jvec(5)*2);
        auto k4 = optimize(2*ra::jvec(5));
// it's actually a Iota
        tr.test_eq(0, k1.org_);
        tr.test_eq(0, k2.org_);
        tr.test_eq(0, k3.org_);
        tr.test_eq(0, k4.org_);
        tr.test_eq(ra::vector({0, 2, 4, 6, 8}), j);
        tr.test_eq(ra::vector({0, 2, 4, 6, 8}), k1);
        tr.test_eq(ra::vector({0, 2, 4, 6, 8}), k2);
        tr.test_eq(ra::vector({0, 2, 4, 6, 8}), k3);
        tr.test_eq(ra::vector({0, 2, 4, 6, 8}), k4);
    }
#if RA_OPTIMIZE_SMALLVECTOR==1
    section("small vector ops through vector extensions");
    {
        auto x = optimize(ra::Small<double, 4>{1, 2, 3, 4} + ra::Small<double, 4>{5, 6, 7, 8});
        auto y = ra::Small<double, 4>{1, 2, 3, 4} + ra::Small<double, 4>{5, 6, 7, 8};
        static_assert(std::is_same<decltype(x), ra::Small<double, 4> >::value, "bad optimization");
        static_assert(!std::is_same<decltype(y), ra::Small<double, 4> >::value, "bad non-optimization");
        tr.test_eq(x, y);
    }
#endif
    return tr.summary();
}
