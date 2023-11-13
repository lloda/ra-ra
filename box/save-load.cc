// -*- mode: c++; coding: utf-8 -*-
// ek/box - Alternative traversal logic.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/bench.hh"
#include <iomanip>
#include <chrono>
#include <span>

using std::cout, std::endl, std::flush;

int main()
{
    ra::TestRecorder tr(std::cout);
    tr.section("rank 1");
    {
        auto test1 = [&tr](char const * atag, auto && A, int N)
        {
            auto s = ra::size(A);
            double check = 0.;
            Benchmark bm { N, 3 };
            auto report = [&](std::string const & tag, auto && bv)
            {
                tr.info(std::setw(5), std::fixed, bm.avg(bv)/s/1e-9, " ns [", bm.stddev(bv)/s/1e-9, "] ",
                        atag, " [", ra::noshape, ra::shape(A), "] ", tag)
                    .test_eq(sum(A), check);
            };
            report("ply_ravel",
                   bm.run([&] {
                       check = 0.;
                       ply_ravel(map([&check](auto ai) { check += ai; }, A));
                   }));
            report("ply_ravel_saveload",
                   bm.run([&] {
                       check = 0.;
                       ply_ravel_saveload(map([&check](auto ai) { check += ai; }, A));
                   }));
        };

        tr.section("fixed rank");
        {
            int n = 1000000;
            test1("rank1", ra::Big<double, 1>({n}, ra::_0), 20);
        }
        {
            int n = 1000;
            test1("rank2", ra::Big<double, 2>({n, n}, n*ra::_0 - ra::_1), 20);
        }
        {
            int n = 100;
            test1("rank3", ra::Big<double, 3>({n, n, n}, n*n*ra::_0 - n*ra::_1 + ra::_2), 20);
        }
        tr.section("var rank");
        {
            int n = 1000000;
            test1("rank1", ra::Big<double>({n}, ra::_0), 20);
        }
        {
            int n = 1000;
            test1("rank2", ra::Big<double>({n, n}, n*ra::_0 - ra::_1), 20);
        }
        {
            int n = 100;
            test1("rank3", ra::Big<double>({n, n, n}, n*n*ra::_0 - n*ra::_1 + ra::_2), 20);
        }
    }
    return tr.summary();
}
