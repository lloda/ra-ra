
// (c) Daniel Llorens - 2011, 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-dot.H
/// @brief Benchmark for dot with various array types.

#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/large.H"
#include "ra/wrank.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::setw; using std::setprecision;
using ra::Small; using ra::View; using ra::Unique; using ra::ra_traits; using ra::dim_t;
auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::nanoseconds;
std::string tunit = "ns";
using real = double;

int const N = 200000;
Small<dim_t, 1> S1 { 24*24 };
Small<dim_t, 2> S2 { 24, 24 };
Small<dim_t, 3> S3 { 8, 8, 9 };

struct by_raw
{
    constexpr static char const * name = "raw";

    template <class A, class B>
    std::enable_if_t<ra_traits<A>::rank_s()==1, real>
    operator()(A const & a, B const & b)
    {
        real y(0.);
        for (int j=0; j<S1[0]; ++j) {
            y += a[j]*b[j];
        }
        return y;
    }

    template <class A, class B>
    std::enable_if_t<ra_traits<A>::rank_s()==2, real>
    operator()(A const & a, B const & b)
    {
        real y(0.);
        for (int j=0; j<S2[0]; ++j) {
            for (int k=0; k<S2[1]; ++k) {
                y += a(j, k)*b(j, k);
            }
        }
        return y;
    }

    template <class A, class B>
    std::enable_if_t<ra_traits<A>::rank_s()==3, real>
    operator()(A const & a, B const & b)
    {
        real y(0.);
        for (int j=0; j<S3[0]; ++j) {
            for (int k=0; k<S3[1]; ++k) {
                for (int l=0; l<S3[2]; ++l) {
                    y += a(j, k, l)*b(j, k, l);
                }
            }
        }
        return y;
    }
};
constexpr char const * by_raw::name; // @TODO C++17...

#define BY_PLY(NAME, INSIDE)                                        \
    struct NAME {                                                   \
        constexpr static char const * name = STRINGIZE(NAME);       \
        template <class A, class B> real operator()(A && a, B && b) \
        {                                                           \
            real y(0.);                                             \
            INSIDE;                                                 \
            return y;                                               \
        }                                                           \
    };                                                              \
    constexpr char const * NAME::name;  // @TODO C++17...

#define BY_PLY_TAGGED(PLYER)                                            \
    /* plain */                                                         \
    BY_PLY(JOIN(by_1l_, PLYER),                                         \
           ra::PLYER(ra::map([&y](real const a, real const b) { y += a*b; }, \
                             a, b)))                                    \
                                                                        \
    /* separate reduction */                                            \
    BY_PLY(JOIN(by_2l_, PLYER),                                         \
           ra::PLYER(ra::map([&y](real const a) { y += a; },            \
                             ra::map([](real const a, real const b) { return a*b; }, \
                                     a, b))))                           \
                                                                        \
    /* using trivial rank conjunction */                                \
    BY_PLY(JOIN(by_1w_, PLYER),                                         \
           ra::PLYER(ra::map(ra::wrank<0, 0>([&y](real const a, real const b) { y += a*b; }), \
                             a, b)))                                    \
                                                                        \
    /* separate reduction: using trivial rank conjunction */            \
    BY_PLY(JOIN(by_2w_, PLYER),                                         \
           ra::PLYER(ra::map(ra::wrank<0>([&y](real const a) { y += a; }), \
                             ra::map(ra::wrank<0, 0>([](real const a, real const b) { return a*b; }), \
                                     a, b))));

BY_PLY_TAGGED(ply_ravel);
BY_PLY_TAGGED(ply_index);
BY_PLY_TAGGED(plyf_index);
BY_PLY_TAGGED(plyf);

real a, b, ref, rspec;

int main()
{
    std::random_device rand;
    a = rand();
    b = rand();

    ref = a*b*S1[0]*N;

    TestRecorder tr;
    {
        auto bench = [&tr](auto && f, char const * tag, auto s, auto && ref, int reps)
            {
                rspec = 1e-2;
                constexpr int M = ra::ra_traits<decltype(s)>::size(s);
                decltype(s) A(a);
                decltype(s) B(b);
                real y(0.);
                auto t0 = now();
                for (int j=0; j<reps/1000; ++j) {
                    for (int i=0; i<1000; ++i) {
                        y += f(A, B);
                    }
                }
                time_unit dt = std::chrono::duration_cast<time_unit>(now()-t0);
                tr.info(std::setw(7), std::fixed, 10*double(dt.count())/(reps*M), " ", tunit, " x10 ", tag)
                  .test_rel_error(a*b*M*reps, y, rspec);
            };

        auto f_small_indexed_1 = [](auto && A, auto && B)
            {
                real y = 0;
                for (int j=0; j!=A.size(); ++j) {
                    y += A(j)*B(j);
                }
                return y;
            };

        auto f_small_indexed_2 = [](auto && A, auto && B)
            {
                real y = 0;
                for (int i=0; i!=A.size(0); ++i) {
                    for (int j=0; j!=A.size(1); ++j) {
                        y += A(i, j)*B(i, j);
                    }
                }
                return y;
            };

        auto f_small_indexed_3 = [](auto && A, auto && B)
            {
                real y = 0;
                for (int i=0; i!=A.size(0); ++i) {
                    for (int j=0; j!=A.size(1); ++j) {
                        for (int k=0; k!=A.size(2); ++k) {
                            y += A(i, j, k)*B(i, j, k);
                        }
                    }
                }
                return y;
            };

        auto f_small_indexed_raw = [](auto && A, auto && B)
            {
                real * a = A.data();
                real * b = B.data();
                real y = 0;
                for (int j=0; j!=A.size(); ++j) {
                    y += a[j]*b[j];
                }
                return y;
            };

// optimize() plugs into the definition of operator*, etc.
        auto f_small_op = [](auto && A, auto && B)
            {
                return sum(A*B); // @TODO really slow with rank>2 or even rank>1 depending on the sizes
            };

#define DEFINE_SMALL_PLY(name, plier)                                   \
        auto JOIN(f_small_, plier) = [](auto && A, auto && B)           \
            {                                                           \
                real y = 0;                                             \
                plier(ra::map([&y](real a, real b) { y += a*b; }, A, B)); \
                return y;                                               \
            }
        DEFINE_SMALL_PLY(ply_ravel, ply_ravel);
        DEFINE_SMALL_PLY(ply_index, ply_index);
        DEFINE_SMALL_PLY(plyf, plyf);
        DEFINE_SMALL_PLY(plyf_index, plyf_index);
        DEFINE_SMALL_PLY(ply_either, ply_either);

        auto bench_all = [&](auto s, auto && f_small_indexed)
            {
                constexpr int M = ra::ra_traits<decltype(s)>::size(s);
                tr.section("small <", ra::ra_traits<decltype(s)>::shape(s), ">");
                auto extra = [&]() { return int(double(std::rand())*100/RAND_MAX); };
                int reps = (1000*1000*100)/M;
                bench(f_small_indexed, "indexed", s, ref, reps+extra());
                bench(f_small_indexed_raw, "indexed_raw", s, ref, reps+extra());
                bench(f_small_op, "op", s, ref, reps+extra());
                bench(f_small_ply_ravel, "ply_ravel", s, ref, reps+extra());
                bench(f_small_ply_index, "ply_index", s, ref, reps+extra());
                bench(f_small_plyf, "plyf", s, ref, reps+extra());
                bench(f_small_plyf_index, "plyf_index", s, ref, reps+extra());
                bench(f_small_ply_either, "ply_either", s, ref, reps+extra());
            };
        bench_all(ra::Small<real, 2>(), f_small_indexed_1);
        bench_all(ra::Small<real, 3>(), f_small_indexed_1);
        bench_all(ra::Small<real, 4>(), f_small_indexed_1);
        bench_all(ra::Small<real, 8>(), f_small_indexed_1);
        bench_all(ra::Small<real, 2, 2>(), f_small_indexed_2);
        bench_all(ra::Small<real, 3, 3>(), f_small_indexed_2);
        bench_all(ra::Small<real, 4, 4>(), f_small_indexed_2);
        bench_all(ra::Small<real, 3, 3, 3>(), f_small_indexed_3);
    }

    rspec = 1e-11;
    auto bench = [&tr](auto && f, auto && a, auto && b, auto && ref, real rspec, int reps)
        {
            real x = 0.;
            auto t0 = now();
            for (int i=0; i!=reps; ++i) {
                x += f(a, b);
            }
            time_unit dt = std::chrono::duration_cast<time_unit>(now()-t0);
            tr.info(std::setw(7), std::fixed, double(dt.count())/reps, " ", tunit, " x1 ", f.name)
            .test_rel_error(ref, x, rspec);
        };
#define BENCH(f) bench(f {}, A, B, ref, rspec, N);
    tr.section("std::vector<>");
    {
        std::vector<real> A(S1[0], a);
        std::vector<real> B(S1[0], b);
        BENCH(by_raw);
    }
    tr.section("unchecked pointer");
    {
        std::unique_ptr<real []> Au { new real[S1[0]] };
        std::unique_ptr<real []> Bu { new real[S1[0]] };
        auto A = Au.get();
        auto B = Bu.get();
        std::fill(A, A+S1[0], a);
        std::fill(B, B+S1[0], b);
        BENCH(by_raw);
    }
#define BENCH_ALL                                                       \
    FOR_EACH(BENCH, by_raw, by_1l_ply_ravel, by_2l_ply_ravel, by_1w_ply_ravel, by_2w_ply_ravel); \
    FOR_EACH(BENCH, by_1l_ply_index, by_2l_ply_index, by_1w_ply_index, by_2w_ply_index); \
    FOR_EACH(BENCH, by_1l_plyf, by_2l_plyf, by_1w_plyf, by_2w_plyf);    \
    FOR_EACH(BENCH, by_1l_plyf_index, by_2l_plyf_index, by_1w_plyf_index, by_2w_plyf_index);

    tr.section("ra:: wrapped std::vector<>");
    {
        auto A = std::vector<real>(S1[0], a);
        auto B = std::vector<real>(S1[0], b);
        BENCH_ALL;
    }
    tr.section("raw<1>");
    {
        ra::Unique<real, 1> A(S1, a);
        ra::Unique<real, 1> B(S1, b);
        BENCH_ALL;
    }
    tr.section("raw<2>");
    {
        ra::Unique<real, 2> A(S2, a);
        ra::Unique<real, 2> B(S2, b);
        BENCH_ALL;
    }
    tr.section("raw<3>");
    {
        ra::Unique<real, 3> A(S3, a);
        ra::Unique<real, 3> B(S3, b);
        std::vector<real> x(17, ALINF);
        BENCH_ALL;
    }
    return tr.summary();
}
