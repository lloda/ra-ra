
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-sum-cols.C
/// @brief Benchmark various ways to sum columns.

#include <iostream>
#include <iomanip>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/large.H"
#include "ra/wrank.H"
#include "ra/operators.H"
#include "ra/io.H"
#include <chrono>

using std::cout; using std::endl; using std::flush;
auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::nanoseconds;
std::string tunit = "ns";
using real = double;

int main()
{
    TestRecorder tr(cout);
    cout.precision(4);

    auto bench = [&tr](auto && f, char const * tag, int m, int n, int reps)
        {
            time_unit dt(0);
            ra::Owned<real, 2> a({m, n}, ra::_0 - ra::_1);
            ra::Owned<real, 1> ref({m}, 0);
            ref += a;
            ra::Owned<real, 1> c({m}, ra::unspecified);
            for (int i=0; i<reps; ++i) {
                c = 0.;
                auto t0 = now();
                f(c, a);
                dt += now()-t0;
            }
            tr.info(std::setw(10), std::fixed, (dt.count()/double(m*n*reps)), " ", tunit, " ", tag).test_eq(ref, c);
        };

    auto f_raw = [](auto & c, auto const & a)
        {
            real * __restrict__ ap = a.data();
            real * __restrict__ cp = c.data();
            ra::dim_t const m = a.size(0);
            ra::dim_t const n = a.size(1);
            for (ra::dim_t i=0; i!=m; ++i) {
                for (ra::dim_t j=0; j!=n; ++j) {
                    cp[i] += ap[i*n+j];
                }
            }
        };
    auto f_sideways = [](auto & c, auto const & a)
        {
            for (int j=0, jend=a.size(1); j<jend; ++j) {
                c += a(ra::all, j);
            }
        };
    auto f_accumcols = [](auto & c, auto const & a)
        {
            for_each([](auto & c, auto && a) { c = sum(a); }, c, iter<1>(a));
        };
    auto f_wrank1 = [](auto & c, auto const & a)
        {
            for_each(ra::wrank<0, 0>([](auto & c, auto && a) { c += a; }), c, a);
        };
    auto f_framematch = [](auto & c, auto const & a)
        {
            c += a; // bump c after each row, so it cannot be raveled
        };

    auto bench_all = [&](int m, int n, int reps)
        {
            section(m, " x ", n, " times ", reps);
            bench(f_raw, "raw", m, n, reps);
            bench(f_sideways, "sideways", m, n, reps);
            bench(f_accumcols, "accumcols", m, n, reps);
            bench(f_wrank1, "wrank1", m, n, reps);
            bench(f_framematch, "framematch", m, n, reps);
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
