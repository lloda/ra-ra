// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - Selection ops in ra::

// (c) Daniel Llorens - 2015, 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iomanip>
#include <string>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::Benchmark;
using real = double;

constexpr int nbm = 3;
ra::TestRecorder tr(cout);

inline auto
top_report(auto & bm, std::string const & tag, auto const & ref, auto const & B, auto && fun)
{
    tr.info(Benchmark::report(bm.run(fun), B.size()), " ", tag).test_eq(ref, B);
};

void
rank1_test(auto A_, int Asize, int Isize, int Istep, int N)
{
    std::println(cout, "select {} step {} from {}", Isize, Istep, Asize);
    using Array1 = std::decay_t<decltype(A_)>;
    Array1 A = ra::iota(Asize);
    ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
    Array1 B({Isize}, 0);
    auto II = I.data();
    auto AA = A.data();
    auto BB = B.data();
    auto ref = ra::iota(Isize)*Istep;
    Benchmark bm { N, nbm };
    auto report = [&](std::string const & tag, auto && fun) { top_report(bm, tag, ref, B, fun); };

    report("indexing on raw pointers", [&]{
        for (int i=0; i<Isize; ++i) {
            BB[i] = AA[II[i]];
        }
    });
    report("vectorized selection", [&]{
        B = A(I);
    });
    report("write out the indexing loop", [&]{
        for_each([&A](auto & b, auto i) { b = A(i); }, B, I);
    });
    report("loop on scalar selection", [&]{
        for (int i=0; i<Isize; ++i) {
            B(i) = A(I(i));
        }
    });
}

void
rank1_11_test(auto A_, int Asize, int Isize, int Istep, int N)
{
    std::println(cout, "select {} step {} from {}", Isize, Istep, Asize);
    using Array2 = std::decay_t<decltype(A_)>;
    Array2 A({Asize, Asize}, ra::_0 + ra::_1);
    ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
    Array2 B({Isize, Isize}, 0);
    auto II = I.data();
    auto AA = A.data();
    auto BB = B.data();
    auto ref = Istep*(ra::_0 + ra::_1);
    Benchmark bm { N, 3 };
    auto report = [&](std::string const & tag, auto && fun) { top_report(bm, tag, ref, B, fun); };

    report("2D indexing on raw pointers", [&]{
        for (int i=0; i<Isize; ++i) {
            for (int j=0; j<Isize; ++j) {
                BB[i*Isize + j] = AA[II[i]*Asize + II[j]];
            }
        }
    });
    report("vectorized selection", [&]{
        B = A(I, I);
    });
}

void
rank1_111_test(auto A_, int Asize, int Isize, int Istep, int N)
{
    std::println(cout, "select {} step {} from {}", Isize, Istep, Asize);
    using Array3 = std::decay_t<decltype(A_)>;
    Array3 A({Asize, Asize, Asize}, 10000*ra::_0 + 100*ra::_1 + 1*ra::_2);
    ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
    Array3 B({Isize, Isize, Isize}, 0);
    auto II = I.data();
    auto AA = A.data();
    auto BB = B.data();
    auto ref = Istep*(10000*ra::_0 + 100*ra::_1 + 1*ra::_2);
    Benchmark bm { N, 3 };
    auto report = [&](std::string const & tag, auto && fun) { top_report(bm, tag, ref, B, fun); };

    report("3D indexing on raw pointers", [&]{
        for (int i=0; i<Isize; ++i) {
            for (int j=0; j<Isize; ++j) {
                for (int k=0; k<Isize; ++k) {
                    BB[k+Isize*(j+Isize*i)] = AA[II[k]+Asize*(II[j]+Asize*II[i])];
                }
            }
        }
    });
    report("vectorized selection", [&]{
        B = A(I, I, I);
    });
}

void
rank1_1111_test(auto A_, int Asize, int Isize, int Istep, int N)
{
    std::println(cout, "select {} step {} from {}", Isize, Istep, Asize);
    using Array4 = std::decay_t<decltype(A_)>;
    ra::Unique<real, 4> A(ra::Small<int, 4>(Asize), 1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3);
    ra::Unique<int, 1> I = ra::iota(Isize)*Istep;
    Array4 B(ra::Small<int, 4>(Isize), 0);
    auto II = I.data();
    auto AA = A.data();
    auto BB = B.data();
    auto ref = Istep*(1000000*ra::_0 + 10000*ra::_1 + 100*ra::_2 + 1*ra::_3);
    Benchmark bm { N, 3 };
    auto report = [&](std::string const & tag, auto && fun) { top_report(bm, tag, ref, B, fun); };

    report("4D indexing on raw pointers", [&]{
        for (int i=0; i<Isize; ++i) {
            for (int j=0; j<Isize; ++j) {
                for (int k=0; k<Isize; ++k) {
                    for (int l=0; l<Isize; ++l) {
                        BB[l+Isize*(k+Isize*(j+Isize*i))] = AA[II[l]+Asize*(II[k]+Asize*(II[j]+Asize*II[i]))];
                    }
                }
            }
        }
    });
    report("vectorized selection", [&]{
        B = A(I, I, I, I);
    });
// TODO one way A(i, i, i, i) could work
    report("slice one axis at a time", [&]{
        for (int i=0; i<Isize; ++i) {
            for (int j=0; j<Isize; ++j) {
                for (int k=0; k<Isize; ++k) {
                    B(i, j, k) = A(I[i], I[j], I[k])(I);
                }
            }
        }
    });
}

int main(int argc, char * * argv)
{
    cout.precision(4);
    int nit = argc < 2 ? 5000 : std::stoi(argv[1]);
    int which = argc < 3 ? 0 : std::stoi(argv[2]);
    std::println(cout, "bench-from nit {} nbm {}", nit, nbm);
    if (1==which || 0==which) {
        tr.section("fixed r1(r1)");
        rank1_test(ra::Unique<real, 1>(), 10000, 500, 20, nit);
        rank1_test(ra::Unique<real, 1>(), 1000, 50, 20, 10*nit);
        rank1_test(ra::Unique<real, 1>(), 100, 5, 20, 100*nit);
        rank1_test(ra::Unique<real, 1>(), 10000, 500, 2, nit);
        rank1_test(ra::Unique<real, 1>(), 1000, 50, 2, 10*nit);
        rank1_test(ra::Unique<real, 1>(), 100, 5, 2, 100*nit);
        tr.section("var r1(r1)");
        rank1_test(ra::Unique<real>(), 10000, 500, 20, nit);
        rank1_test(ra::Unique<real>(), 1000, 50, 20, 10*nit);
        rank1_test(ra::Unique<real>(), 100, 5, 20, 100*nit);
        rank1_test(ra::Unique<real>(), 10000, 500, 2, nit);
        rank1_test(ra::Unique<real>(), 1000, 50, 2, 10*nit);
        rank1_test(ra::Unique<real>(), 100, 5, 2, 100*nit);
    }
    if (2==which || 0==which) {
        tr.section("fixed r2(r1, r1)");
        rank1_11_test(ra::Unique<real, 2>(), 1000, 50, 20, nit);
        rank1_11_test(ra::Unique<real, 2>(), 100, 5, 20, 10*10*nit);
        rank1_11_test(ra::Unique<real, 2>(), 1000, 50, 2, nit);
        rank1_11_test(ra::Unique<real, 2>(), 100, 5, 2, 10*10*nit);
        rank1_11_test(ra::Unique<real, 2>(), 10, 5, 2, 10*10*nit);
        tr.section("var r2(r1, r1)");
        rank1_11_test(ra::Unique<real>(), 1000, 50, 20, nit);
        rank1_11_test(ra::Unique<real>(), 100, 5, 20, 10*10*nit);
        rank1_11_test(ra::Unique<real>(), 1000, 50, 2, nit);
        rank1_11_test(ra::Unique<real>(), 100, 5, 2, 10*10*nit);
        rank1_11_test(ra::Unique<real>(), 10, 5, 2, 10*10*nit);
    }
    if (3==which || 0==which) {
        tr.section("fixed r3(r1, r1, r1)");
        rank1_111_test(ra::Unique<real, 3>(), 40, 20, 2, 2000);
        rank1_111_test(ra::Unique<real, 3>(), 100, 5, 20, 4*4*4*2000);
        rank1_111_test(ra::Unique<real, 3>(), 10, 5, 2, 4*4*4*2000);
    }
    if (4==which || 0==which) {
        tr.section("var r4(r1, r1, r1, r1)");
        rank1_1111_test(ra::Unique<real, 4>(), 40, 20, 2, 100);
        rank1_1111_test(ra::Unique<real, 4>(), 10, 5, 2, 4*4*4*4*100);
    }
    return tr.summary();
}
