
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-gemv.H
/// @brief Benchmark for BLAS-2 type ops

// These operations aren't really part of the ET framework, just standalone
// functions.
// Cf bench-gemm.C for BLAS-3 type ops.

#include <iostream>
#include <iomanip>
#include <chrono>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/large.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::setw; using std::setprecision;
using ra::Small; using ra::View; using ra::Unique; using ra::ra_traits; using ra::dim_t;

auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::nanoseconds;
std::string tunit = "ns";
using real = double;

// -------------------
// variants of the defaults, should be slower if the default is well picked.
// @TODO compare with external GEMV/GEVM
// -------------------

int main()
{
    TestRecorder tr(std::cout);

    auto gemv_i = [&](auto const & a, auto const & b)
        {
            dim_t const M = a.size(0);
            ra::Owned<decltype(a(0, 0)*b(0)), 1> c({M}, ra::unspecified);
            for (dim_t i=0; i<M; ++i) {
                c(i) = dot(a(i), b);
            }
            return c;
        };

    auto gemv_j = [&](auto const & a, auto const & b)
        {
            dim_t const M = a.size(0);
            dim_t const N = a.size(1);
            ra::Owned<decltype(a(0, 0)*b(0)), 1> c({M}, 0.);
            for (dim_t j=0; j<N; ++j) {
                c += a(ra::all, j)*b(j);
            }
            return c;
        };

    auto gevm_j = [&](auto const & b, auto const & a)
        {
            dim_t const N = a.size(1);
            ra::Owned<decltype(b(0)*a(0, 0)), 1> c({N}, ra::unspecified);
            for (dim_t j=0; j<N; ++j) {
                c(j) = dot(b, a(ra::all, j));
            }
            return c;
        };

    auto gevm_i = [&](auto const & b, auto const & a)
        {
            dim_t const M = a.size(0);
            dim_t const N = a.size(1);
            ra::Owned<decltype(b(0)*a(0, 0)), 1> c({N}, 0.);
            for (dim_t i=0; i<M; ++i) {
                c += b(i)*a(i);
            }
            return c;
        };

    auto bench_all = [&](int k, dim_t m, dim_t n, dim_t reps)
        {
            auto bench_mv = [&tr, &m, &n, &reps](auto && f, char const * tag)
            {
                time_unit dt(0);
                ra::Owned<real, 2> a({m, n}, ra::_0-ra::_1);
                ra::Owned<real, 1> b({n}, 1-2*ra::_0);
                ra::Owned<real, 1> ref = gemv(a, b);
                ra::Owned<real, 1> c;
                for (dim_t i=0; i<reps; ++i) {
                    auto t0 = now();
                    c = f(a, b);
                    dt += now()-t0;
                }
                tr.info(std::setw(10), std::fixed, dt.count()/(double(reps)*m*n), " ", tunit, " ", tag).test_eq(ref, c);
            };

            auto bench_vm = [&tr, &m, &n, &reps](auto && f, char const * tag)
            {
                time_unit dt(0);
                ra::Owned<real, 2> a({m, n}, ra::_0-ra::_1);
                ra::Owned<real, 1> b({m}, 1-2*ra::_0);
                ra::Owned<real, 1> ref = gevm(b, a);
                ra::Owned<real, 1> c;
                for (dim_t i=0; i<reps; ++i) {
                    auto t0 = now();
                    c = f(b, a);
                    dt += now()-t0;
                }
                tr.info(std::setw(10), std::fixed, dt.count()/(double(reps)*m*n), " ", tunit, " ", tag).test_eq(ref, c);
            };

            tr.section(m, " x ", n, " times ", reps, " = ", (double(reps)*m*n));
// some variants are way too slow to check with larger arrays.
            if (k>0) {
                bench_mv(gemv_i, "mv i");
                bench_mv(gemv_j, "mv j");
                bench_mv([&](auto const & a, auto const & b) { return gemv(a, b); }, "mv default");

                bench_vm(gevm_i, "vm i");
                bench_vm(gevm_j, "vm j");
                bench_vm([&](auto const & a, auto const & b) { return gevm(a, b); }, "vm default");
            }
        };

    bench_all(3, 10, 10, 100000);
    bench_all(3, 100, 100, 100);
    bench_all(3, 500, 500, 1);
    bench_all(3, 10000, 1000, 1);
    bench_all(3, 1000, 10000, 1);
    bench_all(3, 100000, 100, 1);
    bench_all(3, 100, 100000, 1);

    return tr.summary();
}
