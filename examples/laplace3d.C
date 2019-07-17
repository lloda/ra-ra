// -*- mode: c++; coding: utf-8 -*-
/// @file laplace3d.C
/// @brief Solve Poisson equation in a parallelepiped, with linear elements.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Adapted from lsolver/laplace3d.C by Christian Badura, 1998/05.
// This benchmarks fairly close to the Cish original. We spend comparatively
// less time in cghs than in laplace2d.C, so not using BLAS doesn't stand out as
// much as it does there.

#include "ra/ra.H"
#include "ra/test.H"
#include "examples/cghs.H"
#include "ra/bench.H"

using std::cout, std::endl, std::flush, ra::PI;

Benchmark::clock::duration tmul(0);

double const d23=2./3., d16=-1./6., d00=0.;
double const d827=8./27., d427=4./27., d227=2./27., d127=1./27.;
double const SM[8][8] = { {d23, d00, d16, d00, d00, d16, d16, d16},
                          {d00, d23, d00, d16, d16, d00, d16, d16},
                          {d16, d00, d23, d00, d16, d16, d00, d16},
                          {d00, d16, d00, d23, d16, d16, d16, d00},
                          {d00, d16, d16, d16, d23, d00, d16, d00},
                          {d16, d00, d16, d16, d00, d23, d00, d16},
                          {d16, d16, d00, d16, d16, d00, d23, d00},
                          {d16, d16, d16, d00, d00, d16, d00, d23} };
double const MM[8][8] = { {d827, d427, d227, d427, d427, d227, d127, d227},
                          {d427, d827, d427, d227, d227, d427, d227, d127},
                          {d227, d427, d827, d427, d127, d227, d427, d227},
                          {d427, d227, d427, d827, d227, d127, d227, d427},
                          {d427, d227, d127, d227, d827, d427, d227, d427},
                          {d227, d427, d227, d127, d427, d827, d427, d227},
                          {d127, d227, d427, d227, d227, d427, d827, d427},
                          {d227, d127, d227, d427, d427, d227, d427, d827} };

int const ISF[8][3] = { {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} };

struct Element
{
    int v[8];                  // indices of 8 vertices.
    double reg1[8], reg2[8];   // temporary storage.
};

struct Matrix0bnd
{
    ra::Small<double, 8, 8> M; // element matrix.
    double h;                  // scale factor.
    ra::Big<Element, 1> & E;   // elements.
    ra::Big<int, 1> & B;       // indices of boundary elements.
};

template <class A, class V, class W>
void gemv(A const & a, V const & v, W & w)
{
    auto pa = a.data();
    for (int i=0; i<a.size(0); ++i) {
        double tmp=0.;
        for (int j=0; j<a.size(1); ++j) {
            tmp += pa[j*a.size(1)+i]*v[j];
            // tmp += a(i, j)*v[j]; // FIXME is a 1/3 slower; maybe what makes ra::gemv slower overall
        }
        w[i] = tmp;
    }
}

template <class V, class W>
void mult(Matrix0bnd & A, V const & v, W & w)
{
    auto t0 = Benchmark::clock::now();
    w = 0.;
    for_each([&](auto && E)
             {
                 ra::start(E.reg1) = v(E.v);                // vector -> element
                 gemv(A.M, E.reg1, E.reg2);                 // element matrix
                 // ra::start(E.reg2) = ra::gemv(A.M, E.reg1); // FIXME somewhat slower
                 w(E.v) += E.reg2;                          // element -> vector
             },
             A.E);
    w(A.B) = 0; // set boundary
    w *= A.h;   // scale factor
    tmul += Benchmark::clock::now()-t0;
}

struct StiffMatrix: Matrix0bnd
{
    StiffMatrix(double h, ra::Big<Element, 1> & E, ra::Big<int, 1> & B)
        : Matrix0bnd { SM, h, E, B } {};
};

struct MassMatrix: Matrix0bnd
{
    MassMatrix(double h, ra::Big<Element, 1> & E, ra::Big<int, 1> & B)
        : Matrix0bnd { MM, h*h*h, E, B } {};
};

inline int pos(int n, int i, int j, int k)
{
    return n*(n*i + j) + k;
}

// CosCosCosSolution, CosCosCos in [-1, 1]^3
double g(double x, double y, double z)
{
    return cos(.5*PI*x)*cos(.5*PI*y)*cos(.5*PI*z);
}
double f(double x, double y, double z)
{
    return .25*3.*PI*PI*cos(.5*PI*x)*cos(.5*PI*y)*cos(.5*PI*z);
}

int main()
{
    TestRecorder tr(std::cout);

    auto t0 = Benchmark::clock::now();

    int n = 50;
    double EPS = 1e-5;
    double h = 1./n; // space = ±1, so grid step = 2h

    ra::Big<ra::Small<double, 3>, 1> V({(n+1)*(n+1)*(n+1)}, ra::none); // vertex coordinates.
    ra::Big<int, 1> B({6*(n+1)*(n+1)}, ra::none);                      // indices of boundary vertices.
    ra::Big<Element, 1> E({n*n*n}, ra::none);                          // the elements.

// set vertex coordinates.
    auto ip = ra::iota(n+1);
    ply(from([&](auto i, auto j, auto k)
             { V[pos(n+1, i, j, k)] = { -1.+2.*i*h, -1.+2.*j*h, -1.+2.*k*h }; },
             ip, ip, ip));

// set elements.
    auto i = ra::iota(n);
    ply(from([&](auto i, auto j, auto k, auto v)
         {  E(pos(n, i, j, k)).v[v] = pos(n+1, i+ISF[v][0], j+ISF[v][1], k+ISF[v][2]); },
         i, i, i, ra::iota(8)));

// set boundaries (points on edges and corners multiple times --doesn't matter).
    int k = 0;
    for (int i=0; i<=n; ++i) {
        for (int j=0; j<=n; ++j) {
            B[k++] = pos(n+1, 0, i, j);
            B[k++] = pos(n+1, n, i, j);
            B[k++] = pos(n+1, i, 0, j);
            B[k++] = pos(n+1, i, n, j);
            B[k++] = pos(n+1, i, j, 0);
            B[k++] = pos(n+1, i, j, n);
        }
    }

    ra::Big<double, 1> b({(n+1)*(n+1)*(n+1)}, 0.); // right hand side
    ra::Big<double, 1> x({(n+1)*(n+1)*(n+1)}, 0.); // solution vector

// set right side.
    ra::Big<double, 1> aux = map([](auto && Vi) { return f(Vi[0], Vi[1], Vi[2]); }, V);

// integral ~ product with mass matrix.
    MassMatrix MM(h, E, B);
    mult(MM, aux, b);

// solve.
    StiffMatrix SM(h, E, B);
    ra::Big<double, 2> work({3, (n+1)*(n+1)*(n+1)}, ra::none);
    auto t1 = Benchmark::clock::now();
    int its = cghs(SM, b, x, work, EPS);
    auto tsolve = Benchmark::clock::now()-t1;

// compare with exact solution.
    ra::Big<double, 1> c = map([](auto && Vi) { return g(Vi[0], Vi[1], Vi[2]); }, V);
    double err = amax(abs(c-x));

    auto ttot = Benchmark::clock::now()-t0;
    cout << "total time " << Benchmark::toseconds(ttot)/1e-3 << " " << "ms" << endl;
    cout << "solve time " << Benchmark::toseconds(tsolve)/1e-3 << " " << "ms" << endl;
    cout << "mul time " << Benchmark::toseconds(tmul)/1e-3 << " " << "ms" << endl;

    tr.info("err ", err).test_le(err, 0.00033);
    tr.info("its ", its).test_le(its, 1);
    return tr.summary();
}
