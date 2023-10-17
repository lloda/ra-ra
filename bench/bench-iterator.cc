// -*- mode: c++; coding: utf-8 -*-
// ra-ra/iterator - STLIterator

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iomanip>
#include <string>
#include "ra/test.hh"
#include "ra/bench.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;

// FIXME Small cases get reduced statically by -O3 (at least gcc11-13), which isn't very useful.

int main()
{
    TestRecorder tr(cout);
    cout.precision(4);
    tr.section("rank 1");
    {
        auto test1 = [&tr](char const * atag, auto && A, int N)
        {
            auto s = ra::size(A);
            Benchmark bm { N, 3 };
            auto report = [&](std::string const & tag, auto && bv)
            {
                tr.info(std::setw(5), std::fixed, bm.avg(bv)/s/1e-9, " ns [", bm.stddev(bv)/s/1e-9, "] ",
                        atag, " [", ra::noshape, ra::shape(A), "] ", tag)
                    .test_eq(ra::iota(s), A);
            };
            report("range for",
                   bm.run([&] {
                       int i = 0;
                       for (auto & a: A) {
                           a = i;
                           ++i;
                       }
                   }));
            report("ply undef",
                   bm.run([&] {
                       A = ra::_0;
                   }));
            report("ply def",
                   bm.run([&] {
                       A = ra::iota(s);
                   }));
        };

        tr.section("static dimensions");
        test1("small", ra::Small<real, 10>(), 50000000);

        tr.section("static rank");
        test1("unique", ra::Unique<real, 1>({10000}, ra::none), 5000);
        test1("view", ra::Unique<real, 1>({10000}, ra::none).view(), 5000);

        tr.section("var rank");
        test1("unique", ra::Unique<real>({10000}, ra::none), 5000);
        test1("view", ra::Unique<real>({10000}, ra::none).view(), 5000);
    }
    tr.section("rank 2");
    {
        auto test2 = [&tr](char const * atag, auto && A, int N)
        {
            auto s = ra::size(A);
            Benchmark bm { N, 3 };
            auto report = [&](std::string const & tag, auto && bv)
            {
                auto B = ra::Unique<real, 1>({s}, A.begin(), s);
                tr.info(std::setw(5), std::fixed, bm.avg(bv)/s/1e-9, " ns [", bm.stddev(bv)/s/1e-9, "] ",
                        atag, " [", ra::noshape, ra::shape(A), "] ", tag)
                    .test_eq(ra::iota(s), B);
            };
            report("range for",
                   bm.run([&] {
                       int i = 0;
                       for (auto & a: A) {
                           a = i;
                           ++i;
                       }
                   }));
        };

        tr.section("static dimensions");
        test2("small", ra::Small<real, 10, 10>(), 1000000);
        test2("small transposed", ra::transpose<1, 0>(ra::Small<real, 10, 10>()), 1000000);

        tr.section("static rank");
        test2("unique", ra::Unique<real, 2>({1000, 1000}, ra::none), 100);
        test2("view", ra::Unique<real, 2>({1000, 1000}, ra::none).view(), 100);
        test2("transposed view", ra::transpose<1, 0>(ra::Unique<real, 2>({1000, 1000}, ra::none)), 100);

        tr.section("var rank");
        test2("unique", ra::Unique<real>({1000, 1000}, ra::none), 100);
        test2("view", ra::Unique<real>({1000, 1000}, ra::none).view(), 100);
        test2("transposed view", ra::transpose<1, 0>(ra::Unique<real>({1000, 1000}, ra::none)), 100);
    }
    return tr.summary();
}
