
// (c) Daniel Llorens - 2011, 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-ra-dot.H
/// @brief Benchmark for dot with various array types.

#include <iostream>
#include <iomanip>
#include <chrono>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/ra-large.H"
#include "ra/ra-wrank.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::setw; using std::setprecision;
using ra::Small; using ra::Raw; using ra::Unique; using ra::ra_traits; using ra::dim_t;

auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::milliseconds;
std::string unit_name = "ms";

int const N = 200000;
Small<dim_t, 1> S1 { 24*24 };
Small<dim_t, 2> S2 { 24, 24 };
Small<dim_t, 3> S3 { 8, 8, 9 };
real randr() { return rand(); }
Small<real, 4> random4() { return Small<real, 4> { randr(), randr(), randr(), randr() }; }

template <class VV>
void report(std::string const & desc, time_unit const dt, VV && value)
{
    cout << setw(32) << desc << " " << setw(7) << dt.count() << unit_name << setw(15) << value << endl;
}

#define DEFINE_BY_SMALL(name, kernel)                               \
    template <int M>                                                \
    real name(real a, real b)                                       \
    {                                                               \
        Small<real, M> A (a);                                       \
        Small<real, M> B (b);                                       \
        real x(0.);                                                 \
        auto t0 = now();                                            \
        for (int i=0; i<N*S1[0]/M; ++i) {                           \
            real y(0.);                                             \
            kernel(y, A, B);                                        \
            x += y;                                                 \
        }                                                           \
        time_unit dt(std::chrono::duration_cast<time_unit>(now()-t0));  \
        report(format(STRINGIZE(name), " Small<", M, "> "), dt, x); \
        return x;                                                   \
    }
DEFINE_BY_SMALL(by_small_indexed,
                [](real & y, auto && A, auto && B) {
                    for (int j=0; j!=M; ++j) {
                        y += A(j)*B(j);
                    }})
DEFINE_BY_SMALL(by_small_indexed_raw,
                [](real & y, auto && A, auto && B) {
                    real * a = A.data();
                    real * b = B.data();
                    for (int j=0; j!=M; ++j) {
                        y += a[j]*b[j];
                    }})
#define DEFINE_BY_SMALL_PLY(name, plier)                                \
    DEFINE_BY_SMALL(JOIN(by_small_, plier),                             \
                    [](real & y, auto && A, auto && B) {                \
                        plier(ra::expr([&y](real a, real b) { y += a*b; }, ra::start(A), ra::start(B))); \
                    })
DEFINE_BY_SMALL_PLY(by_small_ply_ravel, ply_ravel)
DEFINE_BY_SMALL_PLY(by_small_ply_index, ply_index)
DEFINE_BY_SMALL_PLY(by_small_plyf, plyf)
DEFINE_BY_SMALL_PLY(by_small_plyf_index, plyf_index)
DEFINE_BY_SMALL_PLY(by_small_ply_either, ply_either)

// optimize() plugs into the definition of operator*, etc.
DEFINE_BY_SMALL(by_small_op,                                            \
                [](real & y, auto && A, auto && B) {                    \
                    y = sum(A*B);                                       \
                })

template <class A, class B>
enableifc_<ra_traits<A>::rank_s()==1, real>
by_raw(A const & a, B const & b)
{
    real x(0.);
    auto t0 = now();
    for (int i=0; i<N; ++i) {
        real y(0.);
        for (int j=0; j<S1[0]; ++j) {
            y += a[j]*b[j];
        }
        x += y;
    }
    time_unit dt(std::chrono::duration_cast<time_unit>(now()-t0));
    report("explicit subscript", dt, x);
    return x;
}

template <class A, class B>
enableifc_<ra_traits<A>::rank_s()==2, real>
by_raw(A const & a, B const & b)
{
    real x(0.);
    auto t0 = now();
    for (int i=0; i<N; ++i) {
        real y(0.);
        for (int j=0; j<S2[0]; ++j) {
            for (int k=0; k<S2[1]; ++k) {
                y += a(j, k)*b(j, k);
            }
        }
        x += y;
    }
    time_unit dt(std::chrono::duration_cast<time_unit>(now()-t0));
    report("explicit subscript", dt, x);
    return x;
}

template <class A, class B>
enableifc_<ra_traits<A>::rank_s()==3, real>
by_raw(A const & a, B const & b)
{
    real x(0.);
    auto t0 = now();
    for (int i=0; i<N; ++i) {
        real y(0.);
        for (int j=0; j<S3[0]; ++j) {
            for (int k=0; k<S3[1]; ++k) {
                for (int l=0; l<S3[2]; ++l) {
                    y += a(j, k, l)*b(j, k, l);
                }
            }
        }
        x += y;
    }
    time_unit dt(std::chrono::duration_cast<time_unit>(now()-t0));
    report("explicit subscript", dt, x);
    return x;
}

#define BY_PLY(NAME, DESC, INSIDE)                                      \
    template <class A, class B>                                         \
    real NAME(A && a, B && b)                                           \
    {                                                                   \
        auto t0 = now();                                                \
        real x(0.);                                                     \
        for (int i=0; i<N; ++i) {                                       \
            real y(0.);                                                 \
            INSIDE;                                                     \
            x += y;                                                     \
        }                                                               \
        time_unit dt(std::chrono::duration_cast<time_unit>(now()-t0));  \
        report(DESC, dt, x);                                            \
        return x;                                                       \
    }

#define BY_PLY_TAGGED(TAG, DESC, PLYER)                                 \
    /* plain */                                                         \
    BY_PLY(JOIN(by_ply1, TAG), DESC " (1-level, no \")",                \
           PLYER(ra::expr([&y](real const a, real const b) { y += a*b; }, \
                             a, b)))                                    \
                                                                        \
    /* separate reduction: compare abstraction penalty with by_ply. */  \
    BY_PLY(JOIN(by_ply2, TAG), DESC " (2-level, no \")",                \
           PLYER(ra::expr([&y](real const a) { y += a; },            \
                             ra::expr([](real const a, real const b) { return a*b; }, \
                                         a, b))))                       \
                                                                        \
    /* using trivial rank conjunction */                                \
    BY_PLY(JOIN(by_ply3, TAG), DESC " (1-level, trivial \")",           \
           PLYER(ra::ryn(ra::verb<0, 0>::make([&y](real const a, real const b) { y += a*b; }), \
                         a, b)))                                        \
                                                                        \
    /* separate reduction: using trivial rank conjunction */            \
    BY_PLY(JOIN(by_ply4, TAG), DESC " (2-level, trivial \")",           \
           PLYER(ra::ryn(ra::verb<0>::make([&y](real const a) { y += a; }), \
                         ra::ryn(ra::verb<0, 0>::make([](real const a, real const b) { return a*b; }), \
                                 a, b))));

BY_PLY_TAGGED(a, "ply_ravel", ra::ply_ravel);
BY_PLY_TAGGED(b, "ply_index", ra::ply_index);
BY_PLY_TAGGED(c, "plyf_index", ra::plyf_index);
BY_PLY_TAGGED(d, "plyf", ra::plyf);

int main()
{
    srand(999);
    real a = randr();
    real b = randr();
    real ref = a*b*S1[0]*N;

    TestRecorder tr;

    real rspec = 1e-9;
    section("small<>");
    {
#define BENCH(n)                                                        \
        tr.quiet().test_rel_error(ref, by_small_indexed_raw<n>(a, b), rspec); \
        tr.quiet().test_rel_error(ref, by_small_indexed<n>(a, b), rspec); \
        tr.quiet().test_rel_error(ref, by_small_ply_ravel<n>(a, b), rspec); \
        tr.quiet().test_rel_error(ref, by_small_ply_index<n>(a, b), rspec); \
        tr.quiet().test_rel_error(ref, by_small_plyf<n>(a, b), rspec);  \
        tr.quiet().test_rel_error(ref, by_small_plyf_index<n>(a, b), rspec); \
        tr.quiet().test_rel_error(ref, by_small_ply_either<n>(a, b), rspec); \
        tr.quiet().test_rel_error(ref, by_small_op<n>(a, b), rspec);
        BENCH(2); cout << endl;
        BENCH(3); cout << endl;
        BENCH(4); cout << endl;
        BENCH(6); cout << endl;
        BENCH(8);
#undef BENCH
    }

    rspec = 1e-11;
    section("std::vector<>");
    {
        std::vector<real> A(S1[0], a);
        std::vector<real> B(S1[0], b);

        tr.quiet().test_rel_error(ref, by_raw(A, B), rspec);
    }
    section("unchecked pointer");
    {
        std::unique_ptr<real []> A { new real[S1[0]] };
        std::unique_ptr<real []> B { new real[S1[0]] };
        std::fill(A.get(), A.get()+S1[0], a);
        std::fill(B.get(), B.get()+S1[0], b);

        tr.quiet().test_rel_error(ref, by_raw(A.get(), B.get()), rspec);
    }
    section("ra:: wrapped std::vector<>");
    {
        auto A = ra::vector(std::vector<real>(S1[0], a));
        auto B = ra::vector(std::vector<real>(S1[0], b));
        std::vector<real> x(16, ALINF);

        x[0] = by_ply1a(A, B);
        x[1] = by_ply2a(A, B);
        x[2] = by_ply3a(A, B);
        x[3] = by_ply4a(A, B);

        x[4] = by_ply1b(A, B);
        x[5] = by_ply2b(A, B);
        x[6] = by_ply3b(A, B);
        x[7] = by_ply4b(A, B);

        x[8] = by_ply1c(A, B);
        x[9] = by_ply2c(A, B);
        x[10] = by_ply3c(A, B);
        x[11] = by_ply4c(A, B);

        x[12] = by_ply1d(A, B);
        x[13] = by_ply2d(A, B);
        x[14] = by_ply3d(A, B);
        x[15] = by_ply4d(A, B);

        tr.quiet().test_rel_error(ref, ra::vector(x), rspec);
    }
    section("raw<1>");
    {
        // reuse a, b, from before..
        ra::Unique<real, 1> A(S1, ra::scalar(a));
        ra::Unique<real, 1> B(S1, ra::scalar(b));
        std::vector<real> x(17, ALINF);

        x[0] = by_raw(A, B);
        x[1] = by_ply1a(A.iter(), B.iter());
        x[2] = by_ply2a(A.iter(), B.iter());
        x[3] = by_ply3a(A.iter(), B.iter());
        x[4] = by_ply4a(A.iter(), B.iter());

        x[5] = by_ply1b(A.iter(), B.iter());
        x[6] = by_ply2b(A.iter(), B.iter());
        x[7] = by_ply3b(A.iter(), B.iter());
        x[8] = by_ply4b(A.iter(), B.iter());

        x[9] = by_ply1c(A.iter(), B.iter());
        x[10] = by_ply2c(A.iter(), B.iter());
        x[11] = by_ply3c(A.iter(), B.iter());
        x[12] = by_ply4c(A.iter(), B.iter());

        x[13] = by_ply1d(A.iter(), B.iter());
        x[14] = by_ply2d(A.iter(), B.iter());
        x[15] = by_ply3d(A.iter(), B.iter());
        x[16] = by_ply4d(A.iter(), B.iter());

        tr.quiet().test_rel_error(ref, ra::vector(x), rspec);
    }
    section("raw<2>");
    {
        ra::Unique<real, 2> A(S2, ra::scalar(a));
        ra::Unique<real, 2> B(S2, ra::scalar(b));
        std::vector<real> x(17, ALINF);

        x[0] = by_raw(A, B);
        x[1] = by_ply1a(A.iter(), B.iter());
        x[2] = by_ply2a(A.iter(), B.iter());
        x[3] = by_ply3a(A.iter(), B.iter());
        x[4] = by_ply4a(A.iter(), B.iter());

        x[5] = by_ply1b(A.iter(), B.iter());
        x[6] = by_ply2b(A.iter(), B.iter());
        x[7] = by_ply3b(A.iter(), B.iter());
        x[8] = by_ply4b(A.iter(), B.iter());

        x[9] = by_ply1c(A.iter(), B.iter());
        x[10] = by_ply2c(A.iter(), B.iter());
        x[11] = by_ply3c(A.iter(), B.iter());
        x[12] = by_ply4c(A.iter(), B.iter());

        x[13] = by_ply1d(A.iter(), B.iter());
        x[14] = by_ply2d(A.iter(), B.iter());
        x[15] = by_ply3d(A.iter(), B.iter());
        x[16] = by_ply4d(A.iter(), B.iter());

        tr.quiet().test_rel_error(ref, ra::vector(x), rspec);
    }
    section("raw<3>");
    {
        ra::Unique<real, 3> A(S3, ra::scalar(a));
        ra::Unique<real, 3> B(S3, ra::scalar(b));
        std::vector<real> x(17, ALINF);

        x[0] = by_raw(A, B);
        x[1] = by_ply1a(A.iter(), B.iter());
        x[2] = by_ply2a(A.iter(), B.iter());
        x[3] = by_ply3a(A.iter(), B.iter());
        x[4] = by_ply4a(A.iter(), B.iter());

        x[5] = by_ply1b(A.iter(), B.iter());
        x[6] = by_ply2b(A.iter(), B.iter());
        x[7] = by_ply3b(A.iter(), B.iter());
        x[8] = by_ply4b(A.iter(), B.iter());

        x[9] = by_ply1c(A.iter(), B.iter());
        x[10] = by_ply2c(A.iter(), B.iter());
        x[11] = by_ply3c(A.iter(), B.iter());
        x[12] = by_ply4c(A.iter(), B.iter());

        x[13] = by_ply1d(A.iter(), B.iter());
        x[14] = by_ply2d(A.iter(), B.iter());
        x[15] = by_ply3d(A.iter(), B.iter());
        x[16] = by_ply4d(A.iter(), B.iter());

        tr.quiet().test_rel_error(ref, ra::vector(x), rspec);
    }
    return tr.summary();
}
