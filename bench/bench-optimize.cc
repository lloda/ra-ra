// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - ET optimization.

// (c) Daniel Llorens - 2017-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#define RA_OPT // disable so we can compare with (forced) and without
#ifdef RA_OPT_SMALL // bench requires 1 to be meaningful.
#undef RA_OPT_SMALL
#endif
#define RA_OPT_SMALL 1

#include <iostream>
#include <iomanip>
#include "ra/test.hh"

using std::cout, std::endl;
using ra::TestRecorder, ra::Benchmark, ra::Small, ra::ViewBig, ra::Unique;

using Vec = ra::Small<float, 4>;

int main()
{
    TestRecorder tr(std::cout);

    tr.section("small vector ops through vector extensions, other types / sizes");
    {
        ra::Small<double, 4> a = 1 + ra::_0;
        ra::Small<double, 2, 4> b = 33 - ra::_1;
        auto c = opt(a + b(1));
        tr.info("optimization of view").test(std::is_same_v<decltype(c), ra::Small<double, 4>>);
        tr.test_eq(34, c);
    }

    auto bench_type =
        [&](auto v)
        {
            using Vec = decltype(v);
            auto sum_opt =
                [&](auto & a, auto & b, auto & c)
                {
                    for (int i=0; i<a.len(0); ++i) {
                        c(i) = ra::opt(a(i)+b(i));
                        static_assert(std::is_same_v<decltype(opt(a(i)+b(i))), Vec>); // making sure opt is on
                    }
                };

            auto sum_unopt =
                [&](auto & a, auto & b, auto & c)
                {
                    for (int i=0; i<a.len(0); ++i) {
                        c(i) = a(i)+b(i);
                    }
                };

            auto bench_all =
                [&](int reps, int m)
                {
                    auto bench =
                        [&tr, &m, &reps](auto && f, char const * tag)
                        {
// FIXME need alignment knobs for Big
                            alignas (alignof(Vec)) Vec astore[m];
                            alignas (alignof(Vec)) Vec bstore[m];
                            alignas (alignof(Vec)) Vec cstore[m];

                            ra::ViewBig<Vec *, 1> a({m}, astore);
                            ra::ViewBig<Vec *, 1> b({m}, bstore);
                            ra::ViewBig<Vec *, 1> c({m}, cstore);

                            a = +ra::_0 +1;
                            b = -ra::_0 -1;
                            c = 99;

                            auto bv = Benchmark().repeats(reps).runs(3).run([&]() { f(a, b, c); });
                            tr.info(Benchmark::report(bv, m), " ", tag)
                                .test(true);
                        };

                    tr.section("[", (std::is_same_v<float, std::decay_t<decltype(std::declval<Vec>()[0])>> ? "float" : "double"),
                               " x ", Vec::size(), "] block of ", m, " times ", reps);
                    bench(sum_opt, "opt");
                    bench(sum_unopt, "unopt");
                };

            bench_all(50000, 10);
            bench_all(50000, 100);
            bench_all(50000, 1000);
        };
    bench_type(ra::Small<float, 2> {});
    bench_type(ra::Small<double, 2> {});
    bench_type(ra::Small<float, 4> {});
    bench_type(ra::Small<double, 4> {});
    bench_type(ra::Small<float, 8> {});
    bench_type(ra::Small<double, 8> {});
    return tr.summary();
}
