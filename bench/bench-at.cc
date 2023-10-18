// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - Benchmark for at() operator.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include "ra/test.hh"
#include "ra/bench.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;
using ra::dim_t;

// FIXME Bigd/Bigd at loop is an outlier, maybe look into CellBig::flat().

int main()
{
    ra::TestRecorder tr(std::cout);

    auto test = [&tr](auto && C, auto && I, int reps)
    {
        int M = C.len(0);
        int N = C.len(1);
        int O = I.len(0);
        C = 4*ra::_0 + ra::_1;
        I(ra::all, 0) = map([&](auto && i) { return i%M; }, ra::_0 + (std::rand() & 1));
        I(ra::all, 1) = map([&](auto && i) { return i%N; }, ra::_0 + (std::rand() & 1));

        int ref0 = sum(at(C, iter<1>(I))), val0 = 0;

        Benchmark bm { reps, 3 };
        auto report = [&](std::string const & tag, auto && bv)
        {
            tr.info(std::setw(5), std::fixed, bm.avg(bv)/M/N/1e-9, " ns [", bm.stddev(bv)/M/N/1e-9, "] ", tag)
                .test_eq(val0, ref0);
        };

        report("direct subscript",
               bm.run([&] {
                   int val = 0;
                   for (int i=0; i<O; ++i) {
                       val += C(dim_t(I(i, 0)), dim_t(I(i, 1))); // conversions needed when I has runtime rank
                   }
                   val0 = val;
               }));
        report("at member + loop",
               bm.run([&] {
                   int val = 0;
                   for (int i=0; i<O; ++i) {
                       val += C.at(I(i));
                   }
                   val0 = val;
               }));
        report("at op + iter",
               bm.run([&] {
                   val0 = sum(at(C, iter<1>(I)));
               }));
    };

    tr.section("Bigs/Bigs");
    {
        ra::Big<int, 2> C({1000, 4}, ra::none);
        ra::Big<int, 2> I({1000, 2}, ra::none);
        test(C, I, 100);
    }
    tr.section("Bigd/Bigd");
    {
        ra::Big<int> C({1000, 4}, ra::none);
        ra::Big<int> I({1000, 2}, ra::none);
        test(C, I, 100);
    }
    tr.section("Small/Small");
    {
        ra::Small<int, 10, 4> C;
        ra::Small<int, 10, 2> I;
        test(C, I, 10000);
    }
    tr.section("Bigs/Small");
    {
        ra::Big<int, 2> C({1000, 4}, ra::none);
        ra::Small<int, 3, 2> I;
        test(C, I, 1000);
    }
    tr.section("Bigd/Small");
    {
        ra::Big<int> C({1000, 4}, ra::none);
        ra::Small<int, 3, 2> I;
        test(C, I, 1000);
    }
// FIXME not supported atm bc small.at output type depends on the length of the subscript.
    // tr.section("Small/Bigs");
    // {
    //     ra::Small<int, 10, 4> C;
    //     ra::Big<int, 2> I({1000, 2}, ra::none);
    //     test(C, I, 1000);
    // }

    return tr.summary();
}
