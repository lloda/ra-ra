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
    int reps = argc>1 ? std::stoi(argv[1]) : 10000;
    std::println(cout, "reps = {}", reps);
    ra::TestRecorder tr(cout);
    tr.section("rank 2");
    {
        auto test2 = [&tr](auto && C, auto && I, int reps, std::string tag)
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
        test2(smola, smoli, reps, "warmup");
// regression in b40c2d412be04c4c2b4758a332424c05257f71ff due to CellSmall copy ctor.
        test2(smola, smoli, reps, "small/small");
        test2(bigsa, smoli, reps, "bigs/small");
        test2(bigda, smoli, reps, "bigd/small");
        test2(smola, bigsi, reps, "small/bigs");
        test2(bigsa, bigsi, reps, "bigs/bigs");
        test2(bigda, bigsi, reps, "bigd/bigs");
        test2(smola, bigdi, reps, "small/bigd");
        test2(bigsa, bigdi, reps, "bigs/bigd");
        test2(bigda, bigdi, reps, "bigd/bigd");
    }
    tr.section("rank 1");
    {
        auto test1 = [&tr](auto && C, auto && I, int reps, std::string tag)
        {
            if ("warmup"!=tag) tr.section(tag);
            int M = C.len(0);
            int O = I.len(0);
            I(ra::all, 0) = map([&](auto && i) { return i%M; }, ra::_0 + (std::rand() & 1));

            int ref0 = sum(at(C, iter<1>(I))), val0 = 0;
            Benchmark bm { reps, 3 };
            auto report = [&](std::string const & stag, auto && bv)
            {
                if ("warmup"!=tag) tr.info(Benchmark::report(bv, M), " ", stag).test_eq(val0, ref0);
            };

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

// especially Ptr, if it differs from ViewSmall/ViewBig
        auto iotaa = ra::iota(100, 1, 4);
        ra::Big<int, 1> bigsa({100}, 4*ra::_0);
        ra::Big<int> bigda({100}, 4*ra::_0);
        ra::Small<int, 100> smola = 4*ra::_0;
        ra::Big<int, 2> bigsi({100, 1}, ra::none);
        ra::Big<int> bigdi({100, 1}, ra::none);
        ra::Small<int, 100, 1> smoli;
        test1(smola, smoli, reps, "warmup");
        test1(smola, smoli, reps, "small/small");
        test1(bigsa, smoli, reps, "bigs/small");
        test1(bigda, smoli, reps, "bigd/small");
        test1(smola, bigsi, reps, "small/bigs");
        test1(bigsa, bigsi, reps, "bigs/bigs");
        test1(bigda, bigsi, reps, "bigd/bigs");
        test1(smola, bigdi, reps, "small/bigd");
        test1(bigsa, bigdi, reps, "bigs/bigd");
        test1(bigda, bigdi, reps, "bigd/bigd");
        test1(iotaa, smoli, reps, "iota/small");
        test1(iotaa, bigsi, reps, "iota/bigs");
        test1(iotaa, bigdi, reps, "iota/bigd");
    }
    return tr.summary();
}
