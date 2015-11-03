
// (c) Daniel Llorens - 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-ra-mmul.H
/// @brief Benchmark for BLAS-3 type ops

// These operations aren't really part of the ET framework, just
// standalone functions. Benchmarking out of curiosity.

#include <iostream>
#include <iomanip>
#include <chrono>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::setw; using std::setprecision;
using ra::Small; using ra::Raw; using ra::Unique; using ra::ra_traits; using ra::dim_t;

auto now() { return std::chrono::high_resolution_clock::now(); }
template <class DT> auto ms(DT && dt) { return std::chrono::duration_cast<std::chrono::milliseconds>(dt).count(); }
template <class DT> auto us(DT && dt) { return std::chrono::duration_cast<std::chrono::microseconds>(dt).count(); }

// -------------------
// variants of the defaults, mostly slower. @TODO compare with external GEMM
// -------------------

template <class S, class T>
inline auto
mm_mul_k(ra::Raw<S, 2> const & a, ra::Raw<T, 2> const & b)
{
    int const M = a.size(0);
    int const N = b.size(1);
    ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::default_init);
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(ra::all, j));
        }
    }
    return c;
}

// See test-ra-wrank "outer product variants" for the rationale.
// @TODO based on this, allow a Blitz++ like notation C(i, j) = sum(A(i, k)*B(k, j), k) without actually using TensorIndex (e.g. no ply_index).
template <class S, class T>
inline auto
mm_mul_reduce_k(ra::Raw<S, 2> const & a, ra::Raw<T, 2> const & b)
{
    int const M = a.size(0);
    int const N = b.size(1);
    ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::default_init);
    ra::ply_either(ra::ryn(ra::wrank<1, 1, 2>::make(ra::wrank<1, 0, 1>::make([](auto & c, auto && a, auto && b) { c += a*b; })),
                           start(c), start(a), start(b)));
    return c;
}

#define DEFINE_MM_MUL_RESTRICT(NAME_K, NAME_IJ, RESTRICT)               \
    template <class S, class T>                                         \
    inline auto                                                         \
    NAME_K(ra::Raw<S, 2> const & a, ra::Raw<T, 2> const & b)            \
    {                                                                   \
        int const M = a.size(0);                                        \
        int const N = b.size(1);                                        \
        int const K = a.size(1);                                        \
        ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::default_init); \
        T * RESTRICT cc = c.data();                                     \
        T const * RESTRICT aa = a.data();                               \
        T const * RESTRICT bb = b.data();                               \
        for (int i=0; i<M; ++i) {                                       \
            for (int j=0; j<N; ++j) {                                   \
                for (int k=0; k<K; ++k) {                               \
                    cc[i*N+j] += aa[i*K+k] * bb[k*N+j];                 \
                }                                                       \
            }                                                           \
        }                                                               \
        return c;                                                       \
    }                                                                   \
                                                                        \
    template <class S, class T>                                         \
    inline auto                                                         \
    NAME_IJ(ra::Raw<S, 2> const & a, ra::Raw<T, 2> const & b)           \
    {                                                                   \
        int const M = a.size(0);                                        \
        int const N = b.size(1);                                        \
        int const K = a.size(1);                                        \
        ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::default_init); \
        T * RESTRICT cc = c.data();                                     \
        T const * RESTRICT aa = a.data();                               \
        T const * RESTRICT bb = b.data();                               \
        for (int k=0; k<K; ++k) {                                       \
            for (int i=0; i<M; ++i) {                                   \
                for (int j=0; j<N; ++j) {                               \
                    cc[i*N+j] += aa[i*K+k] * bb[k*N+j];                 \
                }                                                       \
            }                                                           \
        }                                                               \
        return c;                                                       \
    }
DEFINE_MM_MUL_RESTRICT(mm_mul_k_raw, mm_mul_ij_raw, /* */)
DEFINE_MM_MUL_RESTRICT(mm_mul_k_raw_restrict, mm_mul_ij_raw_restrict, __restrict__)
#undef DEFINE_MM_MUL_RESTRICT

int main()
{
    TestRecorder tr(std::cout);
    int SS[] = { 4, 16, 64, 256, 768 };
    for (auto S: SS) {
        int const N = max(1, (50*1000*1000) / (S*S*S));
        ra::Owned<real, 2> A({S, S}, ra::_0-ra::_1);
        ra::Owned<real, 2> B({S, S}, ra::_1-2*ra::_0);
        cout << "N: " << N << endl;

#define MM_MUL_BENCHMARK(OP, OUTVAR)                                    \
        ra::Owned<real, 2> OUTVAR({S, S}, 0);                           \
        {                                                               \
            std::chrono::duration<float> dt(0);                         \
            auto t0 = now();                                            \
            for (int i=0; i<N; ++i) {                                   \
                OUTVAR += OP(A, B);                                     \
            }                                                           \
            dt += (now()-t0);                                           \
            cout << "S: " << S << setw(26) << " " STRINGIZE(OP) ": " << setw(10) << setprecision(8) << (ms(dt)/double(N)) << " ms / iter " << endl; \
        }

        MM_MUL_BENCHMARK(mm_mul_k, C0)
        MM_MUL_BENCHMARK(mm_mul_reduce_k, C1)
        MM_MUL_BENCHMARK(mm_mul_k_raw, C2)
        MM_MUL_BENCHMARK(mm_mul_k_raw_restrict, C2b)
        MM_MUL_BENCHMARK(mm_mul_ij_raw, C3)
        MM_MUL_BENCHMARK(mm_mul_ij_raw_restrict, C3b)
        MM_MUL_BENCHMARK(mm_mul, C4)

        tr.quiet().test_rel_error(C0, C0, 0.);
        tr.quiet().test_rel_error(C0, C1, 0.);
        tr.quiet().test_rel_error(C0, C2, 0.);
        tr.quiet().test_rel_error(C0, C2b, 0.);
        tr.quiet().test_rel_error(C0, C3, 0.);
        tr.quiet().test_rel_error(C0, C3b, 0.);
    }
    return tr.summary();
}
