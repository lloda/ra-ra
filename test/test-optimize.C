
// (c) Daniel Llorens - 2014-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-optimize.C
/// @brief Check that ra::optimize() does what it's supposed to do.

#define RA_OPTIMIZE 0 // disable automatic use, so we can compare with (forced) and without
#define RA_OPTIMIZE_IOTA 1 // enable all definitions

#ifndef RA_OPTIMIZE_SMALLVECTOR // test is for 1; forcing 0 makes that part of the test moot
#define RA_OPTIMIZE_SMALLVECTOR 1
#endif

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"

using std::cout; using std::endl;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("misc/sanity");
    {
        tr.test_eq(ra::iota(4, 1, 2), ra::Owned<int, 1> {1, 3, 5, 7});
        {
            auto z = ra::iota(5, 1.5);
            tr.info("iota with real org I").test_eq(1.5, z.org_);
            tr.info("iota with complex org I").test_eq(1.5+ra::start({0, 1, 2, 3, 4}), z);
        }
        {
            auto z = optimize(ra::iota(5, complex(1., 1.)));
            tr.info("iota with complex org I").test_eq(complex(1., 1.), z.org_);
            tr.info("iota with complex org II").test_eq(complex(1., 1.)+ra::start({0., 1., 2., 3., 4.}), z);
        }
        {
            auto i = ra::iota(5);
            auto l = optimize(i*i);
            tr.info("optimize is nop by default").test_eq(ra::start({0, 1, 4, 9, 16}), l);
        }
        {
            auto i = ra::iota(5);
            auto j = i*3.;
            tr.info("ops with non-integers don't reduce iota by default").test(!std::is_same<decltype(i), decltype(j)>::value);
        }
    }
    tr.section("operations with Iota, plus");
    {
        auto test = [&tr](auto && org)
            {
                auto i = ra::iota(5, org);
                auto j = i+1;
                auto k1 = optimize(i+1);
                auto k2 = optimize(1+i);
                auto k3 = optimize(ra::iota(5)+1);
                auto k4 = optimize(1+ra::iota(5));
                tr.info("not optimized w/ RA_OPTIMIZE=0").test(!std::is_same<decltype(i), decltype(j)>::value);
// it's actually a Iota
                tr.test_eq(org+1, k1.org_);
                tr.test_eq(org+1, k1.org_);
                tr.test_eq(org+1, k2.org_);
                tr.test_eq(org+1, k3.org_);
                tr.test_eq(org+1, k4.org_);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), j);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k1);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k2);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k3);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k4);
            };
        test(int(0));
        test(double(0));
        test(float(0));
    }
    tr.section("operations with Iota, times");
    {
        auto test = [&tr](auto && org)
        {
            auto i = ra::iota(5, org);
            auto j = i*2;
            auto k1 = optimize(i*2);
            auto k2 = optimize(2*i);
            auto k3 = optimize(ra::iota(5)*2);
            auto k4 = optimize(2*ra::iota(5));
            tr.info("not optimized w/ RA_OPTIMIZE=0").test(!std::is_same<decltype(i), decltype(j)>::value);
// it's actually a Iota
            tr.test_eq(0, k1.org_);
            tr.test_eq(0, k2.org_);
            tr.test_eq(0, k3.org_);
            tr.test_eq(0, k4.org_);
            tr.test_eq(2*ra::start({0, 1, 2, 3, 4}), j);
            tr.test_eq(2*ra::start({0, 1, 2, 3, 4}), k1);
            tr.test_eq(2*ra::start({0, 1, 2, 3, 4}), k2);
            tr.test_eq(2*ra::start({0, 1, 2, 3, 4}), k3);
            tr.test_eq(2*ra::start({0, 1, 2, 3, 4}), k4);
        };
        test(int(0));
        test(double(0));
        test(float(0));
    }
#if RA_OPTIMIZE_SMALLVECTOR==1
    tr.section("small vector ops through vector extensions [opt-small]");
    {
        using Vec = ra::Small<double, 4>;
        auto x = optimize(Vec {1, 2, 3, 4} + Vec {5, 6, 7, 8});
// BUG Expr holds iterators which hold pointers so auto y = Vec {1, 2, 3, 4} + Vec {5, 6, 7, 8} would hold pointers to lost temps. This is revealed by gcc 6.2. Cf ra::start(iter).
        Vec a {1, 2, 3, 4}, b {5, 6, 7, 8};
        auto y = a + b;
        tr.info("bad optimization").test(std::is_same<decltype(x), Vec>::value);
        tr.info("bad non-optimization").test(!std::is_same<decltype(y), Vec>::value);
        tr.test_eq(y, x);
    }
#endif
    return tr.summary();
}
