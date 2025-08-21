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

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::Benchmark;
using real = double;
using ra::dim_t;

// FIXME bigd/bigd & bigs/bigd at loop

int main(int argc, char * * argv)
{
    int reps = argc>1 ? std::stoi(argv[1]) : 1000;
    std::println(cout, "reps = {}", reps);
    ra::TestRecorder tr(cout);
    auto test = [&tr](auto && C, auto && I, int reps, std::string tag)
    {
        if ("warmup"!=tag) tr.section(tag);
        int M = C.len(0);
        int N = C.len(1);
        int O = I.len(0);
        C = 4*ra::_0 + ra::_1;
        I(ra::all, 0) = map([&](auto && i) { return i%M; }, ra::_0 + (std::rand() & 1));
        I(ra::all, 1) = map([&](auto && i) { return i%N; }, ra::_0 + (std::rand() & 1));

        int ref0 = sum(at(C, iter<1>(I))), val0 = 0;
        Benchmark bm { reps, 3 };
        auto report = [&](std::string const & stag, auto && bv)
        {
            if ("warmup"!=tag) tr.info(Benchmark::report(bv, M*N), " ", stag).test_eq(val0, ref0);
        };

        report("direct subscript",
               bm.run([&]{
                   int val = 0;
                   for (int i=0; i<O; ++i) {
// conversion needed for var rank I
                       val += C(ra::dim_t(I(i, 0)), ra::dim_t(I(i, 1)));
                   }
                   val0 = val;
               }));
        report("at member + loop",
               bm.run([&]{
                   int val = 0;
                   for (int i=0; i<O; ++i) {
                       val += C.at(I(i));
                   }
                   val0 = val;
               }));
        report("at op + iter",
               bm.run([&]{
                   val0 = sum(at(C, iter<1>(I)));
               }));
    };

    ra::Big<int, 2> bigsa({100, 4}, ra::none);
    ra::Big<int> bigda({100, 4}, ra::none);
    ra::Small<int, 100, 4> smola;
    ra::Big<int, 2> bigsi({100, 4}, ra::none);
    ra::Big<int> bigdi({100, 4}, ra::none);
    ra::Small<int, 100, 4> smoli;
    test(smola, smoli, reps*10, "warmup");
// regression in b40c2d412be04c4c2b4758a332424c05257f71ff due to CellSmall copy ctor.
    test(smola, smoli, reps*10, "small/small");
    test(bigsa, smoli, reps*10, "bigs/small");
    test(bigda, smoli, reps*10, "bigd/small");
    test(smola, bigsi, reps, "small/bigs");
    test(bigsa, bigsi, reps, "bigs/bigs");
    test(bigda, bigsi, reps, "bigd/bigs");
    test(smola, bigdi, reps, "small/bigd");
    test(bigsa, bigdi, reps, "bigs/bigd");
    test(bigda, bigdi, reps, "bigd/bigd");
    return tr.summary();
}
