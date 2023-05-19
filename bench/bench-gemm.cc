// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - BLAS-3 type ops.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// These operations aren't really part of the ET framework, just standalone functions.
// Cf bench-gemv.cc for BLAS-2 type ops.
// FIXME Benchmark w/o allocation.

#include <iostream>
#include <iomanip>
#include "ra/test.hh"
#include "ra/complex.hh"
#include "ra/ra.hh"
#include "ra/bench.hh"

using std::cout, std::endl, std::setw, std::setprecision, ra::TestRecorder;
using ra::Small, ra::View, ra::Unique, ra::dim_t;
using real = double;

// -------------------
// variants of the defaults, should be slower if the default is well picked.
// -------------------

template <class A, class B, class C> inline void
gemm_block_3(ra::View<A, 2> const & a, ra::View<B, 2> const & b, ra::View<C, 2> c)
{
    dim_t const m = a.len(0);
    dim_t const p = a.len(1);
    dim_t const n = b.len(1);
// terminal, using reduce_k, see below
    if (max(m, max(p, n))<=64) {
        for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })),
                 c, a, b);
// split a's rows
    } else if (m>=max(p, n)) {
        gemm_block_3(a(ra::iota(m/2)), b, c(ra::iota(m/2)));
        gemm_block_3(a(ra::iota(m-m/2, m/2)), b, c(ra::iota(m-m/2, m/2)));
// split b's columns
    } else if (n>=max(m, p)) {
        gemm_block_3(a, b(ra::all, ra::iota(n/2)), c(ra::all, ra::iota(n/2)));
        gemm_block_3(a, b(ra::all, ra::iota(n-n/2, n/2)), c(ra::all, ra::iota(n-n/2, n/2)));
// split a's columns and b's rows
    } else {
        gemm_block_3(a(ra::all, ra::iota(p/2)), b(ra::iota(p/2)), c);
        gemm_block_3(a(ra::all, ra::iota(p-p/2, p/2)), b(ra::iota(p-p/2, p/2)), c);
    }
}

#if RA_USE_BLAS==1

extern "C" {
#include <cblas.h>
}

constexpr CBLAS_TRANSPOSE
fliptr(CBLAS_TRANSPOSE t)
{
    if (t==CblasTrans) {
        return CblasNoTrans;
    } else if (t==CblasNoTrans) {
        return CblasTrans;
    } else {
        assert(0 && "BLAS doesn't support this transpose");
        abort();
    }
}

constexpr bool
istr(CBLAS_TRANSPOSE t)
{
    return (t==CblasTrans) || (t==CblasConjTrans);
}

template <class A> inline void
lead_and_order(A const & a, int & ld, CBLAS_ORDER & order)
{
    if (a.step(1)==1) {
        order = CblasRowMajor;
        ld = a.step(0);
    } else if (a.step(0)==1) {
        order = CblasColMajor;
        ld = a.step(1);
    } else {
        order = CblasRowMajor;
        ld = 0;
        assert(0 && "not a BLAS-supported array");
    }
}

inline void
gemm_blas_3(ra::View<double, 2> const & A, ra::View<double, 2> const & B, ra::View<double, 2> C)
{
    CBLAS_TRANSPOSE ta = CblasNoTrans;
    CBLAS_TRANSPOSE tb = CblasNoTrans;
    int ldc, lda, ldb;
    CBLAS_ORDER orderc, ordera, orderb;
    lead_and_order(C, ldc, orderc);
    lead_and_order(A, lda, ordera);
    lead_and_order(B, ldb, orderb);
    int K = A.len(1-istr(ta));
    assert(K==B.len(istr(tb)) && "mismatched A/B");
    assert(C.len(0)==A.len(istr(ta)) && "mismatched C/A");
    assert(C.len(1)==B.len(1-istr(tb)) && "mismatched C/B");
    if (ordera!=orderc) {
        ta = fliptr(ta);
    }
    if (orderb!=orderc) {
        tb = fliptr(tb);
    }
    if (C.size()>0) {
        cblas_dgemm(orderc, ta, tb, C.len(0), C.len(1), K, 1., A.data(), lda, B.data(), ldb, 0, C.data(), ldc);
    }
}
inline auto
gemm_blas(ra::View<double, 2> const & a, ra::View<double, 2> const & b)
{
    ra::Big<decltype(a(0, 0)*b(0, 0)), 2> c({a.len(0), b.len(1)}, 0);
    gemm_blas_3(a, b, c);
    return c;
}
#endif // RA_USE_BLAS

int main()
{
    TestRecorder tr(std::cout);

    auto gemm_block = [&](auto const & a, auto const & b)
        {
            ra::Big<decltype(a(0, 0)*b(0, 0)), 2> c({a.len(0), b.len(1)}, 0);
            gemm_block_3(a, b, c);
            return c;
        };

    auto gemm_k = [&](auto const & a, auto const & b)
        {
            dim_t const M = a.len(0);
            dim_t const N = b.len(1);
            ra::Big<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::none);
            for (dim_t i=0; i<M; ++i) {
                for (dim_t j=0; j<N; ++j) {
                    c(i, j) = dot(a(i), b(ra::all, j));
                }
            }
            return c;
        };

// See test/wrank.cc "outer product variants" for the logic.
// TODO based on this, allow a Blitz++ like notation C(i, j) = sum(A(i, k)*B(k, j), k). Maybe using TensorIndex now that that works with ply_ravel.
    auto gemm_reduce_k = [&](auto const & a, auto const & b)
        {
            dim_t const M = a.len(0);
            dim_t const N = b.len(1);
            using T = decltype(a(0, 0)*b(0, 0));
            ra::Big<T, 2> c({M, N}, T());
            for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })),
                     c, a, b);
            return c;
        };

#define DEFINE_GEMM_RESTRICT(NAME_K, NAME_IJ, RESTRICT)     \
    auto NAME_K = [&](auto const & a, auto const & b)       \
        {                                                   \
            dim_t const M = a.len(0);                      \
            dim_t const N = b.len(1);                      \
            dim_t const K = a.len(1);                      \
            using T = decltype(a(0, 0)*b(0, 0));            \
            ra::Big<T, 2> c({M, N}, T());                   \
            T * RESTRICT cc = c.data();                     \
            T const * RESTRICT aa = a.data();               \
            T const * RESTRICT bb = b.data();               \
            for (dim_t i=0; i<M; ++i) {                     \
                for (dim_t j=0; j<N; ++j) {                 \
                    for (dim_t k=0; k<K; ++k) {             \
                        cc[i*N+j] += aa[i*K+k] * bb[k*N+j]; \
                    }                                       \
                }                                           \
            }                                               \
            return c;                                       \
        };                                                  \
                                                            \
    auto NAME_IJ = [&](auto const & a, auto const & b)      \
        {                                                   \
            dim_t const M = a.len(0);                      \
            dim_t const N = b.len(1);                      \
            dim_t const K = a.len(1);                      \
            using T = decltype(a(0, 0)*b(0, 0));            \
            ra::Big<T, 2> c({M, N}, T());                   \
            T * RESTRICT cc = c.data();                     \
            T const * RESTRICT aa = a.data();               \
            T const * RESTRICT bb = b.data();               \
            for (dim_t k=0; k<K; ++k) {                     \
                for (dim_t i=0; i<M; ++i) {                 \
                    for (dim_t j=0; j<N; ++j) {             \
                        cc[i*N+j] += aa[i*K+k] * bb[k*N+j]; \
                    }                                       \
                }                                           \
            }                                               \
            return c;                                       \
        };
DEFINE_GEMM_RESTRICT(gemm_k_raw, gemm_ij_raw, /* */)
DEFINE_GEMM_RESTRICT(gemm_k_raw_restrict, gemm_ij_raw_restrict, __restrict__)
#undef DEFINE_GEMM_RESTRICT

    auto bench_all = [&](int k, int m, int p, int n, int reps)
        {
            auto bench = [&](auto && f, char const * tag)
            {
                ra::Big<real, 2> a({m, p}, ra::_0-ra::_1);
                ra::Big<real, 2> b({p, n}, ra::_1-2*ra::_0);
                ra::Big<real, 2> ref = gemm(a, b);
                ra::Big<real, 2> c;

                auto bv = Benchmark().repeats(reps).runs(3).run([&]() { c = f(a, b); });
                tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/(m*n*p)/1e-9, " ns [",
                        Benchmark::stddev(bv)/(m*n*p)/1e-9 ,"] ", tag).test_eq(ref, c);
            };

            tr.section(m, " (", p, ") ", n, " times ", reps);
// some variants are way too slow to check with larger arrays.
            if (k>2) {
                bench(gemm_k, "k");
            }
            if (k>1) {
                bench(gemm_k_raw, "k_raw");
                bench(gemm_k_raw_restrict, "k_raw_restrict");
            }
            if (k>0) {
                bench(gemm_reduce_k, "reduce_k");
                bench(gemm_ij_raw, "ij_raw");
                bench(gemm_ij_raw_restrict, "ij_raw_restrict");
            }
            bench(gemm_block, "block");
#if RA_USE_BLAS==1
            bench(gemm_blas, "blas");
#endif
            bench([&](auto const & a, auto const & b) { return gemm(a, b); }, "default");
        };

    bench_all(3, 10, 10, 10, 10000);
    bench_all(2, 100, 100, 100, 100);
    bench_all(2, 500, 400, 500, 1);
    bench_all(1, 10000, 10, 1000, 1);
    bench_all(1, 1000, 10, 10000, 1);
    bench_all(1, 100000, 10, 100, 1);
    bench_all(1, 100, 10, 100000, 1);

    return tr.summary();
}
