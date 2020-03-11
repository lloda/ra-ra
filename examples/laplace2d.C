// -*- mode: c++; coding: utf-8 -*-
/// @file laplace2d.C
/// @brief Solve Poisson equation in a rectangle, with linear elements.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Adapted from lsolver/laplace2d.C by Christian Badura, 1998/05.
// FIXME mult() benchmarks well against the Cish original, but cghs dominates
// the computation and we do much worse against BLAS.  Clearly I need to
// benchmark the basic stuff against BLAS.

// FIXME plot like the APL examples do.

#include "ra/ra.H"
#include "ra/test.H"
#include "examples/cghs.H"
#include "ra/bench.H"

using std::cout, std::endl, ra::PI, ra::TestRecorder;

Benchmark::clock::duration tmul(0);

struct StiffMat { double h; };

template <class V, class W>
void mult(StiffMat const & A, V const & v, W & w)
{
    auto t0 = Benchmark::clock::now();
    auto i = ra::iota(v.size(0)-2, 1);
    auto j = ra::iota(v.size(1)-2, 1);
    w(i, j) = 4*v(i, j) -v(i-1, j) -v(i, j-1) -v(i+1, j) -v(i, j+1);
    tmul += (Benchmark::clock::now()-t0);
}

struct MassMat { double h; };

template <class V, class W>
void mult(MassMat const & M, V const & v, W & w)
{
    auto t0 = Benchmark::clock::now();
    auto i = ra::iota(v.size(0)-2, 1);
    auto j = ra::iota(v.size(1)-2, 1);
    w(i, j) = sqrm(M.h) * (v(i, j)/2. + (v(i-1, j) + v(i, j-1) + v(i+1, j) + v(i, j+1) + v(i+1, j+1) + v(i-1, j-1))/12.);
    tmul += Benchmark::clock::now()-t0;
}

// problem: -laplace u=f, with solution g
inline double f(double x, double y)
{
    return 8.*PI*PI*sin(2.*PI*x)*sin(2.*PI*y);
}
inline double g(double x, double y)
{
    return sin(2.*PI*x)*sin(2.*PI*y);
}

int main(int argc, char *argv[])
{
    TestRecorder tr(std::cout);
    auto t0 = Benchmark::clock::now();

    int N = 50;
    double EPS = 1e-5;
    double h = 1./N;

    ra::Big<double, 2> v({N+1, N+1}, 0.), u({N+1, N+1}, 0.), w({N+1, N+1}, 0.), b({N+1, N+1}, 0.);
    auto i = ra::iota(N-1, 1);
    auto j = ra::iota(N-1, 1);
    auto ih = ra::iota(N-1, h, h);
    auto jh = ra::iota(N-1, h, h);
    v(i, j) = from(g, ih, jh);
    w(i, j) = from(f, ih, jh);

    StiffMat sm { h };
    MassMat mm { h };

    mult(mm, w, b);
    ra::Big<double, 3> work({3, N+1, N+1}, ra::none);
    int its = cghs(sm, b, u, work, EPS);
    double max = amax(abs(u-v));

    auto ttot = Benchmark::clock::now()-t0;
    cout << "total time " << Benchmark::toseconds(ttot)/1e-3 << " " << "ms" << endl;
    cout << "mul time " << Benchmark::toseconds(ttot)/1e-3 << " " << "ms" << endl;

    tr.info("max ", max).test_le(max, 0.00463);
    tr.info("its ", its).test_le(its, 67);
    return tr.summary();
}
