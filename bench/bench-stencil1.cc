// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - Stencil-as-view (rank 1).

// (c) Daniel Llorens - 2016-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iomanip>
#include <random>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::Benchmark;
using real = double;

int nx = 1000000;
int ts = 10;

auto I = ra::iota(nx-2, 1);

constexpr ra::Small<real, 3> mask = { 1, -2, 1 };

#define THEOP template <class A_, class Anext_, class Astencil_> __attribute__((noinline)) \
   auto operator()(A_ & A, Anext_ & Anext, Astencil_ & Astencil)

// sensitive to RA_CHECK.
struct f_raw
{
    THEOP
    {
        for (int i=1; i+1<nx; ++i) {
            Anext(i) = -2*A(i) + A(i+1) + A(i-1);
        }
        std::swap(A.cp, Anext.cp);
    };
};

// about as fast as f_raw, but no stencil. Insensitive to RA_CHECK.
struct f_slices
{
    THEOP
    {
        Anext(I) = -2*A(I) + A(I+1) + A(I-1);
        std::swap(A.cp, Anext.cp);
    };
};

// with stencil, about as fast as f_raw. Sensitive to RA_CHECK.
struct f_stencil_explicit
{
    THEOP
    {
        Astencil.cp = A.data();
        Anext(I) = map([](auto && A) { return -2*A(1) + A(2) + A(0); },
                       ra::iter<1>(Astencil));
        std::swap(A.cp, Anext.cp);
    };
};

// sum() inside uses run time sizes and 1-dim ply_ravel loop which is a bit slower. TODO
struct f_stencil_arrayop
{
    THEOP
    {
        Astencil.cp = A.data();
        Anext(I) = map([](auto && s) { return sum(s*mask); }, ra::iter<1>(Astencil));
        std::swap(A.cp, Anext.cp);
    };
};

// allows traversal order to be chosen between all 6 axes in ply_ravel. 30x slower. TODO
struct f_sumprod
{
    THEOP
    {
        Astencil.cp = A.data();
        Anext(I) = 0; // TODO miss notation for sum-of-axes without preparing destination...
        Anext(I) += map(ra::wrank<1, 1>(std::multiplies<>()), Astencil, mask);
        std::swap(A.cp, Anext.cp);
    };
};

// variant of the above, much faster (TODO).
struct f_sumprod2
{
    THEOP
    {
        Astencil.cp = A.data();
        Anext(I) = 0;
        ply_fixed(map(ra::wrank<0, 1, 1>([](auto && A, auto && B, auto && C) { A += B*C; }), Anext(I), Astencil, mask));
        std::swap(A.cp, Anext.cp);
    };
};

int main()
{
    TestRecorder tr(std::cout);

    std::random_device rand;
    real value = rand();

    auto bench = [&](auto & A, auto & Anext, auto & Astencil, auto && ref, auto && tag, auto && f)
                 {
                     auto bv = Benchmark().repeats(ts).runs(3)
                         .once_f([&](auto && repeat)
                                 {
                                     Anext = 0.;
                                     A = value;
                                     repeat([&]() { f(A, Anext, Astencil); });
                                 });
                     tr.info(Benchmark::report(bv, A.size()), " ", tag)
                         .test_rel(ref, A, 1e-11);
                 };

    ra::Big<real, 1> Aref;

    tr.section("static rank");
    {
        ra::Big<real, 1> A({nx}, 1.);
        ra::Big<real, 1> Anext({nx}, 0.);
        auto Astencil = stencil(A, 1, 1);
        ra::print(cout, "Astencil ", fmt({ .sep0 = "|" }, Astencil(0, ra::dots<1>))) << endl;
#define BENCH(ref, op) bench(A, Anext, Astencil, ref, RA_STRINGIZE(op), op {});
        BENCH(A, f_raw);
        Aref = ra::Big<real, 1>(A);
        BENCH(Aref, f_slices);
        BENCH(Aref, f_stencil_explicit);
        BENCH(Aref, f_stencil_arrayop);
        BENCH(Aref, f_sumprod);
        BENCH(Aref, f_sumprod2);
#undef BENCH
    }
    tr.section("dynamic rank");
    {
        ra::Big<real> B({nx}, 1.);
        ra::Big<real> Bnext({nx}, 0.);
        auto Bstencil = stencil(B, 1, 1);
        ra::print(cout, "Bstencil ", fmt({ .sep0 = "|" }, Bstencil(0, ra::dots<1>))) << endl;
#define BENCH(ref, op) bench(B, Bnext, Bstencil, ref, RA_STRINGIZE(op), op {});
        // BENCH(Aref, f_raw); // TODO very slow
        BENCH(Aref, f_slices);
        BENCH(Aref, f_stencil_explicit);
        BENCH(Aref, f_stencil_arrayop);
#undef BENCH
    }
    return tr.summary();
}
