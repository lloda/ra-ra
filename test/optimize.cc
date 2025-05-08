// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Check that ra::optimize() does what it's supposed to do.

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#define RA_OPT // disable so we can compare with (forced) and without
#ifndef RA_OPT_SMALLVECTOR // test is for 1; forcing 0 skips that part of the test.
#define RA_OPT_SMALLVECTOR 1
#endif

#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, ra::TestRecorder;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("optimizing static size Iotas");
    {
        tr.test_eq(4, optimize(ra::iota(ra::int_c<4>()) + ra::iota(4, 0, 2)).nn);
        tr.test_eq(4, optimize(ra::iota(4) + ra::iota(ra::int_c<4>(), 0, 2)).nn);
        tr.test_eq(ra::start({0, 3, 6, 9}), optimize(ra::iota(ra::int_c<4>()) + ra::iota(4, 0, 2)));
        tr.test_eq(4, optimize(ra::iota(ra::int_c<4>()) - ra::iota(4, 0, 2)).nn);
        tr.test_eq(4, optimize(ra::iota(4) - ra::iota(ra::int_c<4>(), 0, 2)).nn);
        tr.test_eq(ra::start({0, -1, -2, -3}), optimize(ra::iota(4) - ra::iota(ra::int_c<4>(), 0, 2)));
    }
    tr.section("Iota ops, expr Iotas WIP");
    {
        tr.info("naked").test(ra::is_iota<decltype(ra::iota(ra::len))>);
        tr.info("nop").test(ra::is_iota<decltype(wlen(10, ra::iota(ra::len)))>);
// works unopt bc Match avoid checking if has_len
        tr.test_eq(ra::iota(10, 0, 2), wlen(10, ra::iota(ra::len) + ra::iota(ra::len)));
// works, but opt runs at + site, not after wlen (FIXME?)
        tr.test_eq(ra::iota(10, 0, 2), optimize(wlen(10, ra::iota(ra::len) + ra::iota(ra::len))));
// FIXME don't work, because optimize() can't determine the match-length of the result iota
        // tr.info("+, naked").test(ra::is_iota<decltype(optimize(ra::iota(ra::len) + ra::iota(ra::len)))>);
        // tr.info("+").test(ra::is_iota<decltype(wlen(10, optimize(ra::iota(ra::len) + ra::iota(ra::len))))>);
        // tr.test_eq(ra::iota(10, 0, 2), wlen(10, optimize(ra::iota(ra::len) + ra::iota(ra::len))));
    }
    tr.section("misc/sanity");
    {
        cout << ra::is_iota<ra::Len> << endl;
        tr.test_eq(ra::iota(4, 1, 2), ra::Big<int, 1> {1, 3, 5, 7});
        {
            auto z = ra::iota(5, 1.5);
            tr.info("iota with real org I").test_eq(1.5, z.i);
            tr.info("iota with complex org I").test_eq(1.5+ra::start({0, 1, 2, 3, 4}), z);
        }
        {
            auto z = optimize(ra::iota(5, complex(1., 1.)));
            tr.info("iota with complex org I").test_eq(complex(1., 1.), z.i);
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
            tr.info("ops with non-integers don't reduce iota by default").test(!std::is_same_v<decltype(i), decltype(j)>);
        }
    }
    tr.section("Iota ops, plus");
    {
        static_assert(ra::iota_op<ra::Scalar<int>>);
        static_assert(ra::is_iota<decltype(ra::iota(10, long(10)))>);
        auto test = [&tr](auto && org)
            {
                auto i = ra::iota(5, org);
                auto j = i+1;
                auto k1 = optimize(i+1);
                static_assert(ra::is_iota<decltype(k1)>);
                auto k2 = optimize(1+i);
                auto k3 = optimize(ra::iota(5)+1);
                auto k4 = optimize(1+ra::iota(5));
                auto k5 = optimize(1.5+ra::iota(5));
                auto k6 = optimize(ra::iota(5)-0.5);
                tr.info("not optimized w/ blank RA_OPT").test(!std::is_same_v<decltype(i), decltype(j)>);
// it's actually a Iota
                tr.test_eq(org+1, k1.i);
                tr.test_eq(org+1, k1.i);
                tr.test_eq(org+1, k2.i);
                tr.test_eq(org+1, k3.i);
                tr.test_eq(org+1, k4.i);
                tr.test_eq(org+1.5, k5.i);
                tr.test_eq(org-0.5, k6.i);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), j);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k1);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k2);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k3);
                tr.test_eq(1+ra::start({0, 1, 2, 3, 4}), k4);
                tr.test_eq(1.5+ra::start({0, 1, 2, 3, 4}), k5);
                tr.test_eq(ra::start({0, 1, 2, 3, 4})-0.5, k6);
            };
        test(int(0));
        test(double(0));
        test(float(0));
    }
    tr.section("Iota ops, negate");
    {
        auto test = [&tr](auto && org)
            {
                auto i = ra::iota(5, org);
                auto j = -i;
                auto k1 = optimize(-i);
                static_assert(ra::is_iota<decltype(k1)>);
                tr.info("not optimized w/ blank RA_OPT").test(!std::is_same_v<decltype(i), decltype(j)>);
// it's actually a Iota
                tr.test_eq(-org, k1.i);
                tr.test_eq(-ra::start({0, 1, 2, 3, 4}), j);
                tr.test_eq(-ra::start({0, 1, 2, 3, 4}), k1);
            };
        test(int(0));
        test(double(0));
        test(float(0));
    }
    tr.section("Iota ops, multiplies");
    {
        auto test = [&tr](auto && org)
        {
            auto i = ra::iota(5, org);
            auto j = i*2;
            auto k1 = optimize(i*2);
            auto k2 = optimize(2*i);
            auto k3 = optimize(ra::iota(5)*2);
            auto k4 = optimize(2*ra::iota(5));
            tr.info("not optimized w/ blank RA_OPT").test(!std::is_same_v<decltype(i), decltype(j)>);
// it's actually a Iota
            tr.test_eq(0, k1.i);
            tr.test_eq(0, k2.i);
            tr.test_eq(0, k3.i);
            tr.test_eq(0, k4.i);
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
#if RA_OPT_SMALLVECTOR==1
    tr.section("small vector ops through vector extensions");
    {
        using Vec = ra::Small<double, 4>;
        Vec const r {6, 8, 10, 12};

// [ra4] Map holds iterators which hold pointers so auto y = Vec {1, 2, 3, 4} + Vec {5, 6, 7, 8} would hold pointers to lost temps. This is revealed by gcc 6.2. Cf ra::start(iter). So this example only works bc it's optimized.
        auto x = optimize(Vec {1, 2, 3, 4} + Vec {5, 6, 7, 8});
        tr.info("optimization rvalue terms").test(std::is_same_v<decltype(x), Vec>);
        tr.test_eq(r, x);

        Vec a {1, 2, 3, 4}, b {5, 6, 7, 8};
        auto y = a + b;
        auto z = optimize(a + b);
        tr.info("optimization of lvalue terms").test(std::is_same_v<decltype(z), Vec>);
        tr.info("not optimized by default, yet").test(!std::is_same_v<decltype(y), Vec>);
        tr.test_eq(r, y);
        tr.test_eq(r, z);

        auto q = optimize(a + r);
        tr.info("optimization of const lvalue terms").test(std::is_same_v<decltype(q), Vec>);
        tr.test_eq(ra::start({7, 10, 13, 16}), q);

        ra::Small<double, 4, 4> c = 1 + ra::_1;

        auto d = optimize(c(0) + b);
        tr.info("optimization of view").test(std::is_same_v<decltype(d), Vec>);
        tr.test_eq(r, d);
    }
    tr.section("small vector ops through vector extensions, other types / sizes");
    {
        ra::Small<double, 8> a = 1 + ra::_0;
        ra::Small<double, 4, 8> b = 33 - ra::_1;
        auto c = optimize(a + b(3));
        tr.info("optimization of view").test(std::is_same_v<decltype(c), ra::Small<double, 8>>);
        tr.test_eq(34, c);
    }
#endif // RA_OPT_SMALLVECTOR==1
    return tr.summary();
}
