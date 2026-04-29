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
#include "test/mpdebug.hh"

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

    auto bench_type = [&](auto v){
        using Vec = decltype(v);
        auto sum_opt = [&](auto & a, auto & b, auto & c){
            static_assert(std::is_same_v<decltype(opt(a(0)+b(0))), Vec>); // making sure opt is on
            for (int i=0; i<a.len(0); ++i) {
                c(i) = ra::opt(a(i)+b(i));
            }
        };
        auto sum_unopt = [&](auto & a, auto & b, auto & c){
            for (int i=0; i<a.len(0); ++i) {
                c(i) = a(i)+b(i);
            }
        };
        auto sum_ply = [&](auto & a, auto & b, auto & c){
            c = a+b;
        };
        auto bench_all = [&](int reps, int m){
            auto bench = [&tr, &m, &reps](auto && f, char const * tag){
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

                auto bv = Benchmark().reps(reps).runs(3).run([&]() { f(a, b, c); });
                tr.info(Benchmark::report(bv, m), " ", tag).test(true);
            };
            tr.section("[", ra::mp::type_name<decltype(v[0])>(), " x ", Vec::size(), "] block of ", m, " times ", reps);
            bench(sum_opt, "opt");
            bench(sum_unopt, "unopt");
            bench(sum_ply, "ply");
        };
        bench_all(50000, 1000);
    };
    static_assert(ra::align_req<int, 4> == alignof(ra::Small<int, 4>));
    bench_type(ra::Small<int32_t, 2> {});
    bench_type(ra::Small<float, 2> {});
    bench_type(ra::Small<double, 2> {});
    bench_type(ra::Small<int32_t, 4> {});
    bench_type(ra::Small<float, 4> {});
    bench_type(ra::Small<double, 4> {});
    bench_type(ra::Small<int32_t, 8> {});
    bench_type(ra::Small<float, 8> {});
    bench_type(ra::Small<double, 8> {});
    return tr.summary();
}
