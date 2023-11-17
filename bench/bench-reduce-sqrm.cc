// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - reduce_sqrm() with various array types.

// (c) Daniel Llorens - 2011, 2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iomanip>
#include "ra/bench.hh"

using std::cout, std::endl, std::setw, std::setprecision, ra::TestRecorder;
using real = double;
using real4 = ra::Small<real, 4>;
using ra::sqrm;

int const N = 500000;
ra::Small<ra::dim_t, 1> S1 { 24*24 };
ra::Small<ra::dim_t, 2> S2 { 24, 24 };
ra::Small<ra::dim_t, 3> S3 { 8, 8, 9 };

TestRecorder tr(std::cout);
real y;

template <class BV>
void report(int size, BV const & bv)
{
    tr.info(std::setw(5), std::fixed, Benchmark::avg(bv)/size/1e-9, " ns [", Benchmark::stddev(bv)/size/1e-9 ,"] ", bv.name)
        .test_eq(prod(S1)*N*4*4, y);
}

int main()
{
    Benchmark bm = Benchmark().runs(3);
    report(4,
           bm.name("real4 raw").repeats(N*prod(S1)/4)
           .run_f([&](auto && repeat)
                  {
                      real4 A(7.), B(3.);
                      y = 0.;
                      repeat([&] {
                          for (int j=0; j!=4; ++j) {
                              y += sqrm(A(j)-B(j));
                          }
                      });
                  }));
    report(4,
           bm.name("real4 expr").repeats(N*prod(S1)/4)
           .run_f([&](auto && repeat)
                  {
                      real4 A(7.), B(3.);
                      y = 0.;
                      repeat([&] {
                          y += reduce_sqrm(A-B);
                      });
                  }));
    report(prod(S1),
           bm.name("C array raw").repeats(N)
           .run_f([&](auto && repeat)
                  {
                      ra::Unique<real, 1> A(S1, 7.);
                      ra::Unique<real, 1> B(S1, 3.);
                      y = 0.;
                      repeat([&] {
                          real const * a = A.data();
                          real const * b = B.data();
                          for (int j=0; j<S1[0]; ++j) {
                              y += sqrm(a[j]-b[j]);
                          }
                      });
                  }));
// sqrm+reduction in one op.
    auto traversal = [&](auto && repeat, auto const & a, auto const & b)
                     {
                         y = 0.;
                         repeat([&] {
                             for_each([&](real const a, real const b) { y += sqrm(a, b); }, a, b);
                         });
                     };
// separate reduction: compare abstraction penalty with by_traversal.
    auto traversal2 = [&](auto && repeat, auto const & a, auto const & b)
                      {
                          y = 0.;
                          repeat([&] {
                              for_each([&](real const a) { y += a; },
                                       map([](real const a, real const b) { return sqrm(a, b); },
                                           a, b));
                          });
                      };
    {
        ra::Unique<real, 1> A(S1, 7.);
        ra::Unique<real, 1> B(S1, 3.);
        report(prod(S1), bm.name("ra::Unique<1> ply nested 1").repeats(N).once_f(traversal, A, B));
        report(prod(S1), bm.name("ra::Unique<1> ply nested 2").repeats(N).once_f(traversal2, A, B));
        report(prod(S1), bm.name("ra::Unique<1> raw").repeats(N)
               .once_f([&](auto && repeat)
                       {
                           y = 0.;
                           repeat([&] {
                               for (int j=0; j<S1[0]; ++j) {
                                   y += sqrm(A(j)-B(j));
                               }
                           });
                       }));
    }
    {
        ra::Unique<real, 2> A(S2, 7.);
        ra::Unique<real, 2> B(S2, 3.);
        report(prod(S2), bm.name("ra::Unique<2> ply nested 1").repeats(N).once_f(traversal, A, B));
        report(prod(S2), bm.name("ra::Unique<2> ply nested 2").repeats(N).once_f(traversal2, A, B));
        report(prod(S2), bm.name("ra::Unique<2> raw").repeats(N)
               .once_f([&](auto && repeat)
                       {
                         y = 0.;
                         repeat([&] {
                             for (int j=0; j<S2[0]; ++j) {
                                 for (int k=0; k<S2[1]; ++k) {
                                     y += sqrm(A(j, k)-B(j, k));
                                 }
                             }
                         });
                     }));
    }
    {
        ra::Unique<real, 3> A(S3, 7.);
        ra::Unique<real, 3> B(S3, 3.);
        report(prod(S3), bm.name("ra::Unique<3> ply nested 1").repeats(N).once_f(traversal, A, B));
        report(prod(S3), bm.name("ra::Unique<3> ply nested 2").repeats(N).once_f(traversal2, A, B));
        report(prod(S3), bm.name("ra::Unique<3> raw").repeats(N)
               .once_f([&](auto && repeat)
                       {
                           y = 0.;
                           repeat([&] {
                               for (int j=0; j<S3[0]; ++j) {
                                   for (int k=0; k<S3[1]; ++k) {
                                       for (int l=0; l<S3[2]; ++l) {
                                           y += sqrm(A(j, k, l)-B(j, k, l));
                                       }
                                   }
                               }
                           });
                       }));
    }
    return tr.summary();
}
