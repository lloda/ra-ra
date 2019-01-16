
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
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/bench.H"

using std::cout, std::endl, std::setw, std::setprecision;
using ra::Small, ra::View, ra::Unique, ra::ra_traits;

using real = double;

// -------------------
// variants of the defaults, should be slower if the default is well picked.
// TODO compare with external GEMV/GEVM
// -------------------

enum trans_t { NOTRANS, TRANS };

int main()
{
    TestRecorder tr(std::cout);

    auto gemv_i = [&](auto const & a, auto const & b)
        {
            int const M = a.size(0);
            ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, ra::none);
            for (int i=0; i<M; ++i) {
                c(i) = dot(a(i), b);
            }
            return c;
        };

    auto gemv_j = [&](auto const & a, auto const & b)
        {
            int const M = a.size(0);
            int const N = a.size(1);
            ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, 0.);
            for (int j=0; j<N; ++j) {
                c += a(ra::all, j)*b(j);
            }
            return c;
        };

    auto gevm_j = [&](auto const & b, auto const & a)
        {
            int const N = a.size(1);
            ra::Big<decltype(b(0)*a(0, 0)), 1> c({N}, ra::none);
            for (int j=0; j<N; ++j) {
                c(j) = dot(b, a(ra::all, j));
            }
            return c;
        };

    auto gevm_i = [&](auto const & b, auto const & a)
        {
            int const M = a.size(0);
            int const N = a.size(1);
            ra::Big<decltype(b(0)*a(0, 0)), 1> c({N}, 0.);
            for (int i=0; i<M; ++i) {
                c += b(i)*a(i);
            }
            return c;
        };

    auto bench_all = [&](int k, int m, int n, int reps)
        {
            auto bench_mv = [&tr, &m, &n, &reps](auto && f, char const * tag, trans_t t)
            {
                ra::Big<real, 2> aa({m, n}, ra::_0-ra::_1);
                auto a = t==TRANS ? transpose<1, 0>(aa) : aa();
                ra::Big<real, 1> b({a.size(1)}, 1-2*ra::_0);
                ra::Big<real, 1> ref = gemv(a, b);
                ra::Big<real, 1> c;

                auto bv = Benchmark().repeats(reps).runs(3).run([&]() { c = f(a, b); });
                tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/(m*n)/1e-9, " ns [",
                        Benchmark::stddev(bv)/(m*n)/1e-9 ,"] ", tag, t==TRANS ? " [T]" : " [N]").test_eq(ref, c);
            };

            auto bench_vm = [&tr, &m, &n, &reps](auto && f, char const * tag, trans_t t)
            {
                ra::Big<real, 2> aa({m, n}, ra::_0-ra::_1);
                auto a = t==TRANS ? transpose<1, 0>(aa) : aa();
                ra::Big<real, 1> b({a.size(0)}, 1-2*ra::_0);
                ra::Big<real, 1> ref = gevm(b, a);
                ra::Big<real, 1> c;

                auto bv = Benchmark().repeats(reps).runs(4).run([&]() { c = f(b, a); });
                tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/(m*n)/1e-9, " ns [",
                        Benchmark::stddev(bv)/(m*n)/1e-9 ,"] ", tag, t==TRANS ? " [T]" : " [N]").test_eq(ref, c);
            };

            tr.section(m, " x ", n, " times ", reps);
// some variants are way too slow to check with larger arrays.
            if (k>0) {
                bench_mv(gemv_i, "mv i", NOTRANS);
                bench_mv(gemv_i, "mv i", TRANS);
                bench_mv(gemv_j, "mv j", NOTRANS);
                bench_mv(gemv_j, "mv j", TRANS);
                bench_mv([&](auto const & a, auto const & b) { return gemv(a, b); }, "mv default", NOTRANS);
                bench_mv([&](auto const & a, auto const & b) { return gemv(a, b); }, "mv default", TRANS);

                bench_vm(gevm_i, "vm i", NOTRANS);
                bench_vm(gevm_i, "vm i", TRANS);
                bench_vm(gevm_j, "vm j", NOTRANS);
                bench_vm(gevm_j, "vm j", TRANS);
                bench_vm([&](auto const & a, auto const & b) { return gevm(a, b); }, "vm default", NOTRANS);
                bench_vm([&](auto const & a, auto const & b) { return gevm(a, b); }, "vm default", TRANS);
            }
        };

    bench_all(3, 10, 10, 10000);
    bench_all(3, 100, 100, 100);
    bench_all(3, 500, 500, 1);
    bench_all(3, 10000, 1000, 1);
    bench_all(3, 1000, 10000, 1);
    bench_all(3, 100000, 100, 1);
    bench_all(3, 100, 100000, 1);

    return tr.summary();
}
