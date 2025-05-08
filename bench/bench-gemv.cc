// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - BLAS-2 type ops.

// (c) Daniel Llorens - 2017-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// These operations aren't really part of the ET framework, just standalone functions.
// Cf bench-gemm.cc for BLAS-3 type ops.

#include <iostream>
#include <iomanip>
#include "ra/test.hh"

using std::cout, std::endl, ra::TestRecorder, ra::Benchmark;
using ra::Small, ra::ViewBig, ra::Unique;

using real = double;

// -------------------
// variants of the defaults, should be slower if the default is well picked.
// TODO compare with external GEMV/GEVM
// -------------------

enum trans_t { NOTRANS, TRANS };

int main()
{
    TestRecorder tr(std::cout);
    cout << "RA_FMA is " << RA_FMA << endl;

    auto gemv_i = [&](auto const & a, auto const & b)
        {
            int const M = a.len(0);
            ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, ra::none);
            for (int i=0; i<M; ++i) {
                c(i) = dot(a(i), b);
            }
            return c;
        };

    auto gemv_j = [&](auto const & a, auto const & b)
        {
            int const M = a.len(0);
            int const N = a.len(1);
            ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, 0.);
            for (int j=0; j<N; ++j) {
                c += a(ra::all, j)*b(j);
            }
            return c;
        };

    auto gevm_j = [&](auto const & b, auto const & a)
        {
            int const N = a.len(1);
            ra::Big<decltype(b(0)*a(0, 0)), 1> c({N}, ra::none);
            for (int j=0; j<N; ++j) {
                c(j) = dot(b, a(ra::all, j));
            }
            return c;
        };

    auto gevm_i = [&](auto const & b, auto const & a)
        {
            int const M = a.len(0);
            int const N = a.len(1);
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
                auto a = t==TRANS ? transpose(aa) : aa();
                ra::Big<real, 1> b({a.len(1)}, 1-2*ra::_0);
                ra::Big<real, 1> ref = gemv(a, b);
                ra::Big<real, 1> c;

                auto bv = Benchmark().repeats(reps).runs(3).run([&]() { c = f(a, b); });
                tr.info(Benchmark::report(bv, m*n), " ", tag, t==TRANS ? " [T]" : " [N]")
                    .test_eq(ref, c);
            };

            auto bench_vm = [&tr, &m, &n, &reps](auto && f, char const * tag, trans_t t)
            {
                ra::Big<real, 2> aa({m, n}, ra::_0-ra::_1);
                auto a = t==TRANS ? transpose(aa) : aa();
                ra::Big<real, 1> b({a.len(0)}, 1-2*ra::_0);
                ra::Big<real, 1> ref = gevm(b, a);
                ra::Big<real, 1> c;

                auto bv = Benchmark().repeats(reps).runs(4).run([&]() { c = f(b, a); });
                tr.info(Benchmark::report(bv, m*n), " ", tag, t==TRANS ? " [T]" : " [N]")
                    .test_eq(ref, c);
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

    bench_all(3, 10, 10, 1000);
    bench_all(3, 100, 100, 10);
    bench_all(3, 500, 500, 1);
    bench_all(3, 10000, 1000, 1);
    bench_all(3, 1000, 10000, 1);
    bench_all(3, 100000, 100, 1);
    bench_all(3, 100, 100000, 1);

    return tr.summary();
}
