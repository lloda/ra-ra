// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - BLAS-3 type ops.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// These operations aren't really part of the ET framework, just standalone functions.
// Cf bench-gemv.cc for BLAS-2 type ops.
// FIXME Bench w/o allocation.
// FIXME Bench offloading, e.g. RA_USE_BLAS=1 GOMP_DEBUG=0 CXXFLAGS="-O3 -fopenmp" LINKFLAGS="-fopenmp" scons -j6 -k  bench/bench-gemm.test

#include <iostream>
#include <iomanip>
#include "ra/test.hh"
#include <omp.h>

using std::cout, std::endl, ra::TestRecorder, ra::Benchmark;
using ra::Small, ra::ViewBig, ra::Unique, ra::dim_t, ra::all;
using real = double;

void
gemm1(auto && a, auto && b, auto & c)
{
    for_each(ra::wrank<1, 2, 1>(ra::wrank<0, 1, 1>([](auto && a, auto && b, auto & c) { ra::maybe_fma(a, b, c); })),
             RA_FW(a), RA_FW(b), RA_FW(c));
}
void
gemm2(auto && a, auto && b, auto & c)
{
    dim_t K=a.len(1);
    for (int k=0; k<K; ++k) {
        c += from(std::multiplies<>(), a(all, k), b(k)); // FIXME fma
    }
}
void
gemm3(auto && a, auto && b, auto & c)
{
    dim_t K=a.len(1);
    for (int k=0; k<K; ++k) {
        for_each(ra::wrank<0, 1, 1>([](auto && a, auto && b, auto & c) { ra::maybe_fma(a, b, c); }), a(all, k), b(k), c);
    }
}
void
gemm4(auto && a, auto && b, auto & c)
{
    dim_t M=a.len(0), N=b.len(1);
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(all, j));
        }
    }
}

// -------------------
// variants of the defaults, should be slower if the default is well picked.
// -------------------

template <class A, class B, class C>
inline void
gemm_block(ra::ViewBig<A, 2> const & a, ra::ViewBig<B, 2> const & b, ra::ViewBig<C, 2> c)
{
    dim_t const m = a.len(0);
    dim_t const p = a.len(1);
    dim_t const n = b.len(1);
// terminal, using reduce_k, see below
    if (max(m, max(p, n))<=64) {
        gemm(a, b, c);
// split a's rows
    } else if (m>=max(p, n)) {
        gemm_block(a(ra::iota(m/2)), b, c(ra::iota(m/2)));
        gemm_block(a(ra::iota(m-m/2, m/2)), b, c(ra::iota(m-m/2, m/2)));
// split b's columns
    } else if (n>=max(m, p)) {
        gemm_block(a, b(all, ra::iota(n/2)), c(all, ra::iota(n/2)));
        gemm_block(a, b(all, ra::iota(n-n/2, n/2)), c(all, ra::iota(n-n/2, n/2)));
// split a's columns and b's rows
    } else {
        gemm_block(a(all, ra::iota(p/2)), b(ra::iota(p/2)), c);
        gemm_block(a(all, ra::iota(p-p/2, p/2)), b(ra::iota(p-p/2, p/2)), c);
    }
}

template <class PTR, class CPTR>
void
gemm_k_raw(auto const & a, auto const & b, auto & c)
{
    dim_t const M = a.len(0);
    dim_t const N = b.len(1);
    dim_t const K = a.len(1);
    PTR cc = c.data();
    CPTR aa = a.data();
    CPTR bb = b.data();
    for (dim_t k=0; k<K; ++k) {
        for (dim_t i=0; i<M; ++i) {
            for (dim_t j=0; j<N; ++j) {
                cc[i*N+j] += aa[i*K+k] * bb[k*N+j];
            }
        }
    }
}

template <class PTR, class CPTR>
void
gemm_ij_raw(auto const & a, auto const & b, auto & c)
{
    dim_t const M = a.len(0);
    dim_t const N = b.len(1);
    dim_t const K = a.len(1);
    PTR cc = c.data();
    CPTR aa = a.data();
    CPTR bb = b.data();
    for (dim_t i=0; i<M; ++i) {
        for (dim_t j=0; j<N; ++j) {
            for (dim_t k=0; k<K; ++k) {
                cc[i*N+j] += aa[i*K+k] * bb[k*N+j];
            }
        }
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

template <class P>
void
gemm_blas(ra::ViewBig<P, 2> const & A, ra::ViewBig<P, 2> const & B, ra::ViewBig<P, 2> C)
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
        if constexpr (std::is_same_v<P, double *>) {
            cblas_dgemm(orderc, ta, tb, C.len(0), C.len(1), K, real(1.), A.data(), lda, B.data(), ldb, 0, C.data(), ldc);
        } else if constexpr (std::is_same_v<P, float *>) {
            cblas_sgemm(orderc, ta, tb, C.len(0), C.len(1), K, real(1.), A.data(), lda, B.data(), ldb, 0, C.data(), ldc);
        } else {
            static_assert(false, "Bad type for BLAS");
        }
    }
}
#endif // RA_USE_BLAS

int main()
{
    TestRecorder tr(std::cout);
    cout << "RA_FMA is " << RA_FMA << endl;

    auto gemm_k = [&](auto const & a, auto const & b, auto & c)
    {
        dim_t const M = a.len(0);
        dim_t const N = b.len(1);
        for (dim_t i=0; i<M; ++i) {
            for (dim_t j=0; j<N; ++j) {
                c(i, j) = dot(a(i), b(all, j));
            }
        }
        return c;
    };

    auto bench_all = [&](int k, int m, int p, int n, int reps)
    {
        auto bench = [&](auto && f, char const * tag, real rerr=0)
        {
            ra::Big<real, 2> a({m, p}, ra::_0-ra::_1);
            ra::Big<real, 2> b({p, n}, ra::_1-2*ra::_0);
            ra::Big<real, 2> ref = gemm(a, b);
            ra::Big<real, 2> c({m, n}, 0.);

            auto bv = Benchmark().repeats(reps).runs(3).run([&]() { f(a, b, c); });
            tr.info(Benchmark::report(bv, m*n*p), " ", tag)
                .test_rel(ref, c, rerr);
        };

        tr.section(m, " (", p, ") ", n, " times ", reps);
#define ZEROFIRST(GEMM) [&](auto const & a, auto const & b, auto & c) { c = 0; GEMM(a, b, c); }
#define NOTZEROFIRST(GEMM) [&](auto const & a, auto const & b, auto & c) { GEMM(a, b, c); }
// some variants are too slow to check with larger arrays.
        if (k>2) {
            bench(NOTZEROFIRST(gemm_k), "k");
        }
        if (k>0) {
            bench(ZEROFIRST((gemm_k_raw<real *,  real const *>)), "k_raw");
            bench(ZEROFIRST((gemm_k_raw<real * __restrict__,  real const * __restrict__>)), "k_raw_restrict");
        }
        if (k>0) {
            bench(ZEROFIRST((gemm_ij_raw<real *,  real const *>)), "ij_raw");
            bench(ZEROFIRST((gemm_ij_raw<real * __restrict__,  real const * __restrict__>)), "ij_raw_restrict");
        }
        bench(ZEROFIRST(gemm_block), "block");
        bench(ZEROFIRST(gemm1), "gemm1");
        bench(ZEROFIRST(gemm2), "gemm2");
        bench(ZEROFIRST(gemm3), "gemm3");
        bench(ZEROFIRST(gemm4), "gemm4");
#if RA_USE_BLAS==1
        bench(ZEROFIRST(gemm_blas), "blas", 100*std::numeric_limits<real>::epsilon()); // ahem
#endif
        bench(ZEROFIRST(gemm), "default");
    };

    bench_all(3, 4, 4, 4, 100);
    bench_all(3, 10, 10, 10, 10);
    bench_all(2, 100, 100, 100, 10);
    bench_all(2, 500, 400, 500, 1);
    bench_all(1, 10000, 10, 1000, 1);
    bench_all(1, 1000, 10, 10000, 1);
    bench_all(1, 100000, 10, 100, 1);
    bench_all(1, 100, 10, 100000, 1);

    return tr.summary();
}
