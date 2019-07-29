
// (c) Daniel Llorens - 2015, 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-from.C
/// @brief Benchmark for ra:: selection ops.

#include <iostream>
#include <iomanip>
#include <string>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/ra.H"
#include "ra/bench.H"

using std::cout, std::endl, std::flush;
using real = double;

int main()
{
    TestRecorder tr(cout);
    cout.precision(4);
    tr.section("rank1(rank1)");
    {
        auto rank1_test = [&tr](auto A_, int Asize, int Isize, int Istep, int N)
        {
            cout << "select " << Isize << " step " << Istep << " from " << Asize << endl;
            using Array1 = std::decay_t<decltype(A_)>;
            Array1 A = ra::iota(Asize);
            ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
            Array1 B({Isize}, 0);
            auto II = I.data();
            auto AA = A.data();
            auto BB = B.data();

            Benchmark bm { N, 3 };
            auto report = [&](std::string const & tag, auto && bv)
                          {
                              tr.info(std::setw(5), std::fixed, bm.avg(bv)/B.size()/1e-9, " ns [", bm.stddev(bv)/B.size()/1e-9, "] ", tag)
                                  .test_eq(ra::iota(Isize)*Istep, B);
                          };

            report("indexing on raw pointers",
                   bm.run([&]()
                          {
                              for (int i=0; i<Isize; ++i) {
                                  BB[i] = AA[II[i]];
                              }
                          }));
            report("vectorized selection",
                   bm.run([&]()
                          {
                              B = A(I);
                          }));
            report("write out the indexing loop",
                   bm.run([&]()
                          {
                              for_each([&A](auto & b, auto i) { b = A(i); }, B, I);
                          }));
            report("loop on scalar selection",
                   bm.run([&]()
                          {
                              for (int i=0; i<Isize; ++i) {
                                  B(i) = A(I(i));
                              }
                          }));
        };

        tr.section("fixed rank");
        rank1_test(ra::Unique<real, 1>(), 10000, 500, 20, 10000);
        rank1_test(ra::Unique<real, 1>(), 1000, 50, 20, 10*10000);
        rank1_test(ra::Unique<real, 1>(), 100, 5, 20, 100*10000);
        rank1_test(ra::Unique<real, 1>(), 10000, 500, 2, 10000);
        rank1_test(ra::Unique<real, 1>(), 1000, 50, 2, 10*10000);
        rank1_test(ra::Unique<real, 1>(), 100, 5, 2, 100*10000);

        tr.section("var rank");
        rank1_test(ra::Unique<real>(), 10000, 500, 20, 10000);
        rank1_test(ra::Unique<real>(), 1000, 50, 20, 10*10000);
        rank1_test(ra::Unique<real>(), 100, 5, 20, 100*10000);
        rank1_test(ra::Unique<real>(), 10000, 500, 2, 10000);
        rank1_test(ra::Unique<real>(), 1000, 50, 2, 10*10000);
        rank1_test(ra::Unique<real>(), 100, 5, 2, 100*10000);
    }
    tr.section("rank2(rank1, rank1)");
    {
        auto rank1_11_test = [&tr](auto A_, int Asize, int Isize, int Istep, int N)
        {
            cout << "select " << Isize << " step " << Istep << " from " << Asize << endl;
            using Array2 = std::decay_t<decltype(A_)>;
            Array2 A({Asize, Asize}, ra::_0 + ra::_1);
            ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
            Array2 B({Isize, Isize}, 0);
            auto II = I.data();
            auto AA = A.data();
            auto BB = B.data();

            Benchmark bm { N, 3 };
            auto report = [&](std::string const &tag, auto && bv)
                          {
                              tr.info(std::setw(5), std::fixed, bm.avg(bv)/B.size()/1e-9, " ns [", bm.stddev(bv)/B.size()/1e-9, "] ", tag)
                                  .test_eq(Istep*(ra::_0 + ra::_1), B);
                          };

            report("2D indexing on raw pointers",
                   bm.run([&]()
                          {
                              for (int i=0; i<Isize; ++i) {
                                  for (int j=0; j<Isize; ++j) {
                                      BB[i*Isize + j] = AA[II[i]*Asize + II[j]];
                                  }
                              }
                          }));
            report("vectorized selection",
                   bm.run([&]()
                          {
                              B = A(I, I);
                          }));
        };
        tr.section("fixed rank");
        rank1_11_test(ra::Unique<real, 2>(), 1000, 50, 20, 10000);
        rank1_11_test(ra::Unique<real, 2>(), 100, 5, 20, 10*10*10000);
        rank1_11_test(ra::Unique<real, 2>(), 1000, 50, 2, 10000);
        rank1_11_test(ra::Unique<real, 2>(), 100, 5, 2, 10*10*10000);
        rank1_11_test(ra::Unique<real, 2>(), 10, 5, 2, 10*10*10000);

        tr.section("var rank");
        rank1_11_test(ra::Unique<real>(), 1000, 50, 20, 10000);
        rank1_11_test(ra::Unique<real>(), 100, 5, 20, 10*10*10000);
        rank1_11_test(ra::Unique<real>(), 1000, 50, 2, 10000);
        rank1_11_test(ra::Unique<real>(), 100, 5, 2, 10*10*10000);
        rank1_11_test(ra::Unique<real>(), 10, 5, 2, 10*10*10000);
    }
    tr.section("rank3(rank1, rank1, rank1)");
    {
        auto rank1_111_test = [&tr](auto A_, int Asize, int Isize, int Istep, int N)
        {
            cout << "select " << Isize << " step " << Istep << " from " << Asize << endl;
            using Array3 = std::decay_t<decltype(A_)>;
            Array3 A({Asize, Asize, Asize}, 10000*ra::_0 + 100*ra::_1 + 1*ra::_2);
            ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
            Array3 B({Isize, Isize, Isize}, 0);
            auto II = I.data();
            auto AA = A.data();
            auto BB = B.data();

            Benchmark bm { N, 3 };
            auto report = [&](std::string const &tag, auto && bv)
                          {
                              tr.info(std::setw(5), std::fixed, bm.avg(bv)/B.size()/1e-9, " ns [", bm.stddev(bv)/B.size()/1e-9, "] ", tag)
                                  .test_eq(Istep*(10000*ra::_0 + 100*ra::_1 + 1*ra::_2), B);
                          };

            report("3D indexing on raw pointers",
                   bm.run([&]()
                          {
                              for (int i=0; i<Isize; ++i) {
                                  for (int j=0; j<Isize; ++j) {
                                      for (int k=0; k<Isize; ++k) {
                                          BB[k+Isize*(j+Isize*i)] = AA[II[k]+Asize*(II[j]+Asize*II[i])];
                                      }
                                  }
                              }
                          }));
            report("vectorized selection",
                   bm.run([&]()
                          {
                              B = A(I, I, I);
                          }));
        };
        tr.section("fixed rank");
        rank1_111_test(ra::Unique<real, 3>(), 40, 20, 2, 4000);
        rank1_111_test(ra::Unique<real, 3>(), 100, 5, 20, 4*4*4*4000);
        rank1_111_test(ra::Unique<real, 3>(), 10, 5, 2, 4*4*4*4000);
    }
    tr.section("rank4(rank1, rank1, rank1, rank1)");
    {
        auto rank1_1111_test = [&tr](auto A_, int Asize, int Isize, int Istep, int N)
        {
            cout << "select " << Isize << " step " << Istep << " from " << Asize << endl;
            using Array4 = std::decay_t<decltype(A_)>;
            ra::Unique<real, 4> A(ra::Small<int, 4>(Asize), 1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3);
            ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
            Array4 B(ra::Small<int, 4>(Isize), 0);
            auto II = I.data();
            auto AA = A.data();
            auto BB = B.data();

            Benchmark bm { N, 3 };
            auto report = [&](std::string const &tag, auto && bv)
                          {
                              tr.info(std::setw(5), std::fixed, bm.avg(bv)/B.size()/1e-9, " ns [", bm.stddev(bv)/B.size()/1e-9, "] ", tag)
                                  .test_eq(Istep*(1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3), B);
                          };

            report("3D indexing on raw pointers",
                   bm.run([&]()
                          {
                              for (int i=0; i<Isize; ++i) {
                                  for (int j=0; j<Isize; ++j) {
                                      for (int k=0; k<Isize; ++k) {
                                          for (int l=0; l<Isize; ++l) {
                                              BB[l+Isize*(k+Isize*(j+Isize*i))] = AA[II[l]+Asize*(II[k]+Asize*(II[j]+Asize*II[i]))];
                                          }
                                      }
                                  }
                              }
                          }));
            report("vectorized selection",
                   bm.run([&]()
                          {
                              B = A(I, I, I, I);
                          }));
            report("slice one axis at a time", // TODO one way A(i, i, i, i) could work
                   bm.run([&]()
                          {
                              for (int i=0; i<Isize; ++i) {
                                  for (int j=0; j<Isize; ++j) {
                                      for (int k=0; k<Isize; ++k) {
                                          B(i, j, k) = A(I[i], I[j], I[k])(I);
                                      }
                                  }
                              }
                          }));
        };
        tr.section("fixed rank");
        rank1_1111_test(ra::Unique<real, 4>(), 40, 20, 2, 200);
        rank1_1111_test(ra::Unique<real, 4>(), 10, 5, 2, 4*4*4*4*200);
    }
    return tr.summary();
}
