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

// -------------------
// variants of the defaults, should be slower if the default is well picked.
// TODO compare with external GEMV/GEVM
// -------------------

enum trans_t { NOTRANS, TRANS };

int main()
{
    TestRecorder tr(std::cout);
    cout << "RA_FMA is " << RA_FMA << endl;

// this is faster for small matrices, so maybe indexer() has some overhead :-/
    auto gemv_0 = [&](auto const & a, auto const & b){
        int const M = a.len(0);
        ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, 0.);
        auto pa = a.data();
        for (int i=0; i<M; ++i) {
            for (int j=0; j<a.len(1); ++j) {
                c[i] += pa[j*a.step(1) + a.step(0)*i]*b[j];
            }
        }
        return c;
    };

    auto gemv_i = [&](auto const & a, auto const & b){
        int const M = a.len(0);
        ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, ra::none);
        for (int i=0; i<M; ++i) {
            c(i) = dot(a(i), b);
        }
        return c;
    };

    auto gemv_j = [&](auto const & a, auto const & b){
        int const M = a.len(0);
        int const N = a.len(1);
        ra::Big<decltype(a(0, 0)*b(0)), 1> c({M}, 0.);
        for (int j=0; j<N; ++j) {
            c += a(ra::all, j)*b(j);
        }
        return c;
    };

    auto gevm_j = [&](auto const & b, auto const & a){
        int const N = a.len(1);
        ra::Big<decltype(b(0)*a(0, 0)), 1> c({N}, ra::none);
        for (int j=0; j<N; ++j) {
            c(j) = dot(b, a(ra::all, j));
        }
        return c;
    };

    auto gevm_i = [&](auto const & b, auto const & a){
        int const M = a.len(0);
        int const N = a.len(1);
        ra::Big<decltype(b(0)*a(0, 0)), 1> c({N}, 0.);
        for (int i=0; i<M; ++i) {
            c += b(i)*a(i);
        }
        return c;
    };

    auto bench_all = [&](int k, int m, int n, int reps){
        auto bench_mv = [&tr, &m, &n, &reps](auto & v, auto && f, char const * tag, trans_t t){
            ra::Big<double, 2> aa({m, n}, ra::_0-ra::_1);
            auto a = t==TRANS ? transpose(aa) : aa();
            ra::Big<double, 1> b({a.len(1)}, 1-2*ra::_0);
            ra::Big<double, 1> ref = gemv(a, b);
            ra::Big<double, 1> c;

            auto bv = Benchmark().name(tag).reps(reps).runs(3).run([&]{ c = f(a, b); });
            tr.info(Benchmark::report(bv, m*n), " ", tag, t==TRANS ? " [T]" : " [N]").test_eq(ref, c);
            v.push_back(bv);
        };

        auto bench_vm = [&tr, &m, &n, &reps](auto & v, auto && f, char const * tag, trans_t t){
            ra::Big<double, 2> aa({m, n}, ra::_0-ra::_1);
            auto a = t==TRANS ? transpose(aa) : aa();
            ra::Big<double, 1> b({a.len(0)}, 1-2*ra::_0);
            ra::Big<double, 1> ref = gevm(b, a);
            ra::Big<double, 1> c;

            auto bv = Benchmark().name(tag).reps(reps).runs(4).run([&]{ c = f(b, a); });
            tr.info(Benchmark::report(bv, m*n), " ", tag, t==TRANS ? " [T]" : " [N]").test_eq(ref, c);
            v.push_back(bv);
        };

        tr.section(m, " x ", n, " times ", reps);
// FIXME average TRANS & NOTRANS.
        auto report = [&](auto & v){
            std::ranges::sort(v, [](auto & a, auto & b){ return a.avg<b.avg; });
            std::println(std::cout, "Best > {}{}{:nS{ / }{}}{}.", ra::esc::cyan, ra::esc::bold, map(&Benchmark::Value::name, v), ra::esc::reset);
        };
// some variants are way too slow to check with larger arrays.
        if (k>0) {
            {
                std::vector<Benchmark::Value> v;
                bench_mv(v, gemv_0, "0N", NOTRANS);
                bench_mv(v, gemv_0, "0T", TRANS);
                bench_mv(v, gemv_i, "iN", NOTRANS);
                bench_mv(v, gemv_i, "iT", TRANS);
                bench_mv(v, gemv_j, "jN", NOTRANS);
                bench_mv(v, gemv_j, "jT", TRANS);
                bench_mv(v, [](auto const & a, auto const & b){ return gemv(a, b); }, "defaultN", NOTRANS);
                bench_mv(v, [](auto const & a, auto const & b){ return gemv(a, b); }, "defaultT", TRANS);
                report(v);
            }
            {
                std::vector<Benchmark::Value> v;
                bench_vm(v, gevm_i, "iN", NOTRANS);
                bench_vm(v, gevm_i, "iT", TRANS);
                bench_vm(v, gevm_j, "jN", NOTRANS);
                bench_vm(v, gevm_j, "jT", TRANS);
                bench_vm(v, [](auto const & a, auto const & b){ return gevm(a, b); }, "defaultN", NOTRANS);
                bench_vm(v, [](auto const & a, auto const & b){ return gevm(a, b); }, "defaultT", TRANS);
                report(v);
            }
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
