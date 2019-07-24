
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-optimize.H
/// @brief Benchmark RA_DO_OPT_SMALLVECTOR (DNW)
// TODO Everything...

#define RA_DO_OPT 0 // disable automatic use, so we can compare with (forced) and without
#define RA_DO_OPT_IOTA 1

#ifdef RA_DO_OPT_SMALLVECTOR // bench requires 1 to be meaningful.
#undef RA_DO_OPT_SMALLVECTOR
#endif
#define RA_DO_OPT_SMALLVECTOR 1

#include "ra/operators.H"
#include "ra/test.H"
#include "ra/bench.H"
#include "ra/mpdebug.H"
#include <iostream>
#include <iomanip>

using std::cout, std::endl, std::setw, std::setprecision;
using ra::Small, ra::View, ra::Unique, ra::ra_traits;

using Vec = ra::Small<float, 4>;

int main()
{
    TestRecorder tr(std::cout);

    tr.section("small vector ops through vector extensions, other types / sizes");
    {
        ra::Small<double, 4> a = 1 + ra::_0;
        ra::Small<double, 2, 4> b = 33 - ra::_1;
        auto c = optimize(a + b(1));
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
                    for (int i=0; i<a.size(0); ++i) {
                        c(i) = ra::optimize(a(i)+b(i));
                        static_assert(std::is_same_v<decltype(optimize(a(i)+b(i))), Vec>); // making sure opt is on
                    }
                };

            auto sum_unopt =
                [&](auto & a, auto & b, auto & c)
                {
                    for (int i=0; i<a.size(0); ++i) {
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

                            ra::View<Vec, 1> a({m}, astore);
                            ra::View<Vec, 1> b({m}, bstore);
                            ra::View<Vec, 1> c({m}, cstore);

                            a = +ra::_0 +1;
                            b = -ra::_0 -1;
                            c = 99;

                            auto bv = Benchmark().repeats(reps).runs(3).run([&]() { f(a, b, c); });
                            tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/(m)/1e-9, " ns [",
                                    Benchmark::stddev(bv)/(m)/1e-9 ,"] ", tag).test(true);
                        };

                    tr.section("[", (std::is_same_v<float, typename Vec::value_type> ? "float" : "double"),
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
