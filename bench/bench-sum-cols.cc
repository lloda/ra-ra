// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - Various ways to sum columns.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iomanip>
#include "ra/ra.hh"
#include "ra/test.hh"
#include "ra/bench.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;

int main()
{
    TestRecorder tr(cout);
    cout.precision(4);

    auto bench =
        [&tr](char const * tag, int m, int n, int reps, auto && f)
        {
            ra::Big<real, 2> a({m, n}, ra::_0 - ra::_1);
            ra::Big<real, 1> ref({m}, 0);
            ref += a*reps;
            ra::Big<real, 1> c({m}, ra::none);

            auto bv = Benchmark().repeats(reps).runs(3)
                .once_f([&](auto && repeat) { c = 0.; repeat([&]() { f(c, a); }); });
            tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/(m*n)/1e-9, " ns [",
                    Benchmark::stddev(bv)/(m*n)/1e-9 ,"] ", tag).test_eq(ref, c);
        };

    auto bench_all =
        [&](int m, int n, int reps)
        {
            tr.section(m, " x ", n, " times ", reps);
            bench("raw", m, n, reps,
                  [](auto & c, auto const & a)
                  {
                      real * __restrict__ ap = a.data();
                      real * __restrict__ cp = c.data();
                      ra::dim_t const m = a.len(0);
                      ra::dim_t const n = a.len(1);
                      for (ra::dim_t i=0; i!=m; ++i) {
                          for (ra::dim_t j=0; j!=n; ++j) {
                              cp[i] += ap[i*n+j];
                          }
                      }
                  });
            bench("sideways", m, n, reps,
                  [](auto & c, auto const & a)
                  {
                      for (int j=0, jend=a.len(1); j<jend; ++j) {
                          c += a(ra::all, j);
                      }
                  });
            bench("accumcols", m, n, reps,
                  [](auto & c, auto const & a)
                  {
                      for_each([](auto & c, auto && a) { c += sum(a); }, c, iter<1>(a));
                  });
            bench("wrank1", m, n, reps,
                  [](auto & c, auto const & a)
                  {
                      for_each(ra::wrank<0, 0>([](auto & c, auto && a) { c += a; }), c, a);
                  });
            bench("framematch", m, n, reps,
                  [](auto & c, auto const & a)
                  {
                      c += a; // bump c after each row, so it cannot be raveled
                  });
        };

    bench_all(1, 1000000, 20);
    bench_all(10, 100000, 20);
    bench_all(100, 10000, 20);
    bench_all(1000, 1000, 20);
    bench_all(10000, 100, 20);
    bench_all(100000, 10, 20);
    bench_all(1000000, 1, 20);

    bench_all(1, 10000, 2000);
    bench_all(10, 1000, 2000);
    bench_all(100, 100, 2000);
    bench_all(1000, 10, 2000);
    bench_all(10000, 1, 2000);

    return tr.summary();
}
