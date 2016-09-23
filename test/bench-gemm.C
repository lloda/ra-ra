
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-gemm.H
/// @brief Benchmark for BLAS-3 type ops

// These operations aren't really part of the ET framework, just standalone
// functions. Benchmarking out of curiosity; they are all massively slower than
// dgemm in CBLAS (about 20 times slower for the 1000x1000 case, maybe 4-5
// single threaded).

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
// variants of the defaults, mostly slower. @TODO compare with external GEMM
// -------------------

template <class A, class B, class C>
inline void
gemm_block_3(ra::View<A, 2> const & a, ra::View<B, 2> const & b, ra::View<C, 2> c)
{
    dim_t const m = a.size(0);
    dim_t const p = a.size(1);
    dim_t const n = b.size(1);
// terminal, using reduce_k, see below
    if (max(m, max(p, n))<=64) {
        for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })), c, a, b);
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

int main()
{
    TestRecorder tr(std::cout);

    auto gemm_block = [&](auto const & a, auto const & b)
        {
            ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({a.size(0), b.size(1)}, 0.);
            gemm_block_3(a, b, c);
            return c;
        };

    auto gemm_k = [&](auto const & a, auto const & b)
        {
            dim_t const M = a.size(0);
            dim_t const N = b.size(1);
            ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::unspecified);
            for (dim_t i=0; i<M; ++i) {
                for (dim_t j=0; j<N; ++j) {
                    c(i, j) = dot(a(i), b(ra::all, j));
                }
            }
            return c;
        };

// See test-wrank.C "outer product variants" for the logic.
// @TODO based on this, allow a Blitz++ like notation C(i, j) = sum(A(i, k)*B(k, j), k) without actually using TensorIndex (e.g. no ply_index).
    auto gemm_reduce_k = [&](auto const & a, auto const & b)
        {
            dim_t const M = a.size(0);
            dim_t const N = b.size(1);
            ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::unspecified);
            for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })),
                     c, a, b);
            return c;
        };

#define DEFINE_GEMM_RESTRICT(NAME_K, NAME_IJ, RESTRICT)     \
    auto NAME_K = [&](auto const & a, auto const & b)       \
        {                                                   \
            dim_t const M = a.size(0);                      \
            dim_t const N = b.size(1);                      \
            dim_t const K = a.size(1);                      \
            using T = decltype(a(0, 0)*b(0, 0));            \
            ra::Owned<T, 2> c({M, N}, ra::unspecified);     \
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
            dim_t const M = a.size(0);                      \
            dim_t const N = b.size(1);                      \
            dim_t const K = a.size(1);                      \
            using T = decltype(a(0, 0)*b(0, 0));            \
            ra::Owned<T, 2> c({M, N}, ra::unspecified);     \
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

    auto bench_all = [&](int k, dim_t m, dim_t p, dim_t n, dim_t reps)
        {
            auto bench = [&tr, &m, &p, &n, &reps](auto && f, char const * tag)
            {
                time_unit dt(0);
                ra::Owned<real, 2> a({m, p}, ra::_0-ra::_1);
                ra::Owned<real, 2> b({p, n}, ra::_1-2*ra::_0);
                ra::Owned<real, 2> ref = gemm(a, b);
                ra::Owned<real, 2> c;
                for (dim_t i=0; i<reps; ++i) {
                    auto t0 = now();
                    c = f(a, b);
                    dt += now()-t0;
                }
                tr.info(std::setw(10), std::fixed, dt.count()/(double(reps)*m*n*p), " ", tunit, " ", tag).test_eq(ref, c);
            };

            tr.section(m, " (", p, ") ", n, " times ", reps, " = ", (double(reps)*m*n*p));
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
            bench([&](auto const & a, auto const & b) { return gemm(a, b); }, "default");
        };

    bench_all(3, 10, 10, 10, 100000);
    bench_all(2, 100, 100, 100, 100);
    bench_all(2, 500, 400, 500, 1);
    bench_all(1, 10000, 10, 1000, 1);
    bench_all(1, 1000, 10, 10000, 1);
    bench_all(1, 100000, 10, 100, 1);
    bench_all(1, 100, 10, 100000, 1);

    return tr.summary();
}
