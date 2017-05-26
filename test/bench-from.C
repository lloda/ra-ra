
// (c) Daniel Llorens - 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bench-from.C
/// @brief Benchmark for ra:: selection ops.

#include <iostream>
#include <iomanip>
#include "ra/test.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/large.H"
#include "ra/wrank.H"
#include "ra/operators.H"
#include "ra/io.H"
#include <chrono>
#include <string>

using std::cout; using std::endl; using std::flush;
auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::nanoseconds;
std::string tunit = "ns";
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
// 0. indexing on raw pointers
            {
                Array1 B({Isize}, 0);
                auto II = I.data();
                auto AA = A.data();
                auto BB = B.data();
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for (int i=0; i<Isize; ++i) {
                        BB[i] = AA[II[i]];
                    }
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " loop on raw pointer []" << endl;
                tr.quiet().test_eq(ra::iota(Isize)*Istep, B);
            }
// 1. vectorized selection
            {
                Array1 B({Isize}, 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    B = A(I);
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " array selection" << endl;
                tr.quiet().test_eq(ra::iota(Isize)*Istep, B);
            }
// 2. write out the indexing loop
            {
                Array1 B({Isize}, 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for_each([&A](auto & b, auto i) { b = A(i); }, B, I);
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " ra::expr on scalar ra:: ()" << endl;
                tr.quiet().test_eq(ra::iota(Isize)*Istep, B);
            }
// 3. loop on scalar selection
            {
                Array1 B({Isize}, 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for (int i=0; i<Isize; ++i) {
                        B(i) = A(I(i));
                    }
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " loop on scalar ra:: ()" << endl;
                tr.quiet().test_eq(ra::iota(Isize)*Istep, B);
            }
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
// 0. 2D indexing on raw pointers
            {
                Array2 B({Isize, Isize}, 0);
                auto II = I.data();
                auto AA = A.data();
                auto BB = B.data();
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for (int i=0; i<Isize; ++i) {
                        for (int j=0; j<Isize; ++j) {
                            BB[i*Isize + j] = AA[II[i]*Asize + II[j]];
                        }
                    }
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " loop on raw pointer []" << endl;
                tr.quiet().test_eq(Istep*(ra::_0 + ra::_1), B);
            }
// 1. vectorized selection
            {
                Array2 B({Isize, Isize}, 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    B = A(I, I);
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " array selection" << endl;
                tr.quiet().test_eq(Istep*(ra::_0 + ra::_1), B);
            }
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
// 0. 3D indexing on raw pointers
            {
                Array3 B({Isize, Isize, Isize}, 0);
                auto II = I.data();
                auto AA = A.data();
                auto BB = B.data();
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for (int i=0; i<Isize; ++i) {
                        for (int j=0; j<Isize; ++j) {
                            for (int k=0; k<Isize; ++k) {
                                BB[k+Isize*(j+Isize*i)] = AA[II[k]+Asize*(II[j]+Asize*II[i])];
                            }
                        }
                    }
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " loop on raw pointer []" << endl;
                tr.quiet().test_eq(Istep*(10000*ra::_0 + 100*ra::_1 + 1*ra::_2), B);
            }
// 1. vectorized selection
            {
                Array3 B({Isize, Isize, Isize}, 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    B = A(I, I, I);
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " array selection" << endl;
                tr.quiet().test_eq(Istep*(10000*ra::_0 + 100*ra::_1 + 1*ra::_2), B);
            }
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
// 0. 3D indexing on raw pointers
            {
                Array4 B(ra::Small<int, 4>(Isize), 0);
                auto II = I.data();
                auto AA = A.data();
                auto BB = B.data();
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for (int i=0; i<Isize; ++i) {
                        for (int j=0; j<Isize; ++j) {
                            for (int k=0; k<Isize; ++k) {
                                for (int l=0; l<Isize; ++l) {
                                    BB[l+Isize*(k+Isize*(j+Isize*i))] = AA[II[l]+Asize*(II[k]+Asize*(II[j]+Asize*II[i]))];
                                }
                            }
                        }
                    }
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " loop on raw pointer []" << endl;
                tr.quiet().test_eq(Istep*(1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3), B);
            }
// 1. vectorized selection
            {
                Array4 B(ra::Small<int, 4>(Isize), 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    B = A(I, I, I, I);
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " array selection" << endl;
                tr.quiet().test_eq(Istep*(1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3), B);
            }
// 2. slice one axis at a time, TODO one way A(i, i, i, i) could work
            {
                Array4 B(ra::Small<int, 4>(Isize), 0);
                time_unit dt(0);
                for (int i=0; i<N; ++i) {
                    auto t0 = now();
                    for (int i=0; i<Isize; ++i) {
                        for (int j=0; j<Isize; ++j) {
                            for (int k=0; k<Isize; ++k) {
                                B(i, j, k) = A(I[i], I[j], I[k])(I);
                            }
                        }
                    }
                    dt += now()-t0;
                }
                cout << std::setw(10) << std::fixed << int(dt.count()/double(N)) << " " << tunit << " array selection on last axis" << endl;
                tr.quiet().test_eq(Istep*(1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3), B);
            }
        };
        tr.section("fixed rank");
        rank1_1111_test(ra::Unique<real, 4>(), 40, 20, 2, 200);
        rank1_1111_test(ra::Unique<real, 4>(), 10, 5, 2, 4*4*4*4*200);
    }
    return tr.summary();
}
