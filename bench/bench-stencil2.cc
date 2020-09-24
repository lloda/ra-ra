// -*- mode: c++; coding: utf-8 -*-
/// @file bench-stencil2.cc
/// @brief Stencil-as-view.

// (c) Daniel Llorens - 2016-2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Bad performance, see also bench-stencil[13].cc.

#include <iostream>
#include <iomanip>
#include <random>
#include "ra/ra.hh"
#include "ra/test.hh"
#include "ra/bench.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;

int nx = 1000;
int ny = 1000;
int ts = 10;

auto I = ra::iota(nx-2, 1);
auto J = ra::iota(ny-2, 1);

constexpr ra::Small<real, 3, 3> mask = { 0, 1, 0,
                                         1, -4, 1,
                                         0, 1, 0 };

#define THEOP template <class A_, class Anext_, class Astencil_> __attribute__((noinline)) \
   auto operator()(A_ & A, Anext_ & Anext, Astencil_ & Astencil)

// sensitive to RA_DO_CHECK.
struct f_raw
{
    THEOP
    {
        for (int i=1; i+1<nx; ++i) {
            for (int j=1; j+1<ny; ++j) {
                Anext(i, j) = -4*A(i, j)
                    + A(i+1, j) + A(i, j+1)
                    + A(i-1, j) + A(i, j-1);
            }
        }
        std::swap(A.p, Anext.p);
    };
};

// about as fast as f_raw, but no stencil. Insensitive to RA_DO_CHECK.
struct f_slices
{
    THEOP
    {
        Anext(I, J) = -4*A(I, J)
            + A(I+1, J) + A(I, J+1)
            + A(I-1, J) + A(I, J-1);
        std::swap(A.p, Anext.p);
    };
};

// with stencil, about as fast as f_raw. Sensitive to RA_DO_CHECK.
struct f_stencil_explicit
{
    THEOP
    {
        Astencil.p = A.data();
        Anext(I, J) = map([](auto && A) { return -4*A(1, 1)
                    + A(2, 1) + A(1, 2)
                    + A(0, 1) + A(1, 0); },
            iter<2>(Astencil));
        std::swap(A.p, Anext.p);
    };
};

// sum() inside uses run time sizes and 2-dim ply_ravel loop which is slower (2x w/gcc). TODO
struct f_stencil_arrayop
{
    THEOP
    {
        Astencil.p = A.data();
        Anext(I, J) = map([](auto && s) { return sum(s*mask); }, iter<2>(Astencil));
        std::swap(A.p, Anext.p);
    };
};

// allows traversal order to be chosen between all 6 axes in ply_ravel. 30x slower. TODO
struct f_sumprod
{
    THEOP
    {
        Astencil.p = A.data();
        Anext(I, J) = 0; // TODO miss notation for sum-of-axes without preparing destination...
        Anext(I, J) += map(ra::wrank<2, 2>(ra::times()), Astencil, mask);
        std::swap(A.p, Anext.p);
    };
};

// variant of the above, much faster (TODO).
struct f_sumprod2
{
    THEOP
    {
        Astencil.p = A.data();
        Anext(I, J) = 0;
        plyf(map(ra::wrank<0, 2, 2>([](auto && A, auto && B, auto && C) { A += B*C; }), Anext(I, J), Astencil, mask));
        std::swap(A.p, Anext.p);
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
            tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/A.size()/1e-9, " ns [",
                    Benchmark::stddev(bv)/A.size()/1e-9 ,"] ", tag)
                .skip().test_rel_error(ref, A, 1e-10);
        };

    ra::Big<real, 2> Aref;

    tr.section("static rank");
    {
        ra::Big<real, 2> A({nx, ny}, 1.);
        ra::Big<real, 2> Anext({nx, ny}, 0.);
        auto Astencil = stencil(A, 1, 1);
        cout << "Astencil " << format_array(Astencil(0, 0, ra::dots<2>), "|", " ") << endl;
#define BENCH(ref, op) bench(A, Anext, Astencil, ref, STRINGIZE(op), op {});
        BENCH(A, f_raw);
        Aref = ra::Big<real, 2>(A);
        BENCH(Aref, f_slices);
        BENCH(Aref, f_stencil_explicit);
        BENCH(Aref, f_stencil_arrayop);
        BENCH(Aref, f_sumprod);
        BENCH(Aref, f_sumprod2);
#undef BENCH
    }
    tr.section("dynamic rank");
    {
        ra::Big<real> B({nx, ny}, 1.);
        ra::Big<real> Bnext({nx, ny}, 0.);
        auto Bstencil = stencil(B, 1, 1);
        cout << "Bstencil " << format_array(Bstencil(0, 0, ra::dots<2>), "|", " ") << endl;
#define BENCH(ref, op) bench(B, Bnext, Bstencil, ref, STRINGIZE(op), op {});
        // BENCH(Aref, f_raw); // TODO very slow
        BENCH(Aref, f_slices);
        BENCH(Aref, f_stencil_explicit);
        BENCH(Aref, f_stencil_arrayop);
#undef BENCH
    }
    return tr.summary();
}
