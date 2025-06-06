// -*- mode: c++; coding: utf-8 -*-
// ra-ra/bench - iota used as Blitz++'s TensorIndex.

// (c) Daniel Llorens - 2019-2020
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::Benchmark;

int main()
{
    TestRecorder tr;
// rank 1
    {
        ra::Big<int, 1> a = {0, 0, 0};
        ra::ply(map([](auto && i) { std::cout << "i: " << i << std::endl; },
                    a+ra::iota<0>()));
        ra::ply_ravel(map([](auto && i) { std::cout << "i: " << i << std::endl; },
                          a+ra::iota<0>()));
    }
// rank 2
    {
        ra::Big<int, 2> a = {{0, 0, 0}, {0, 0, 0}};
        ra::ply(map([](auto && i, auto && j) { std::cout << "i: " << i << ", " << j << std::endl; },
                    a+ra::iota<0>(), a+ra::iota<1>()));
        ra::ply_ravel(map([](auto && i, auto && j) { std::cout << "i: " << i << ", " << j << std::endl; },
                          a+ra::iota<0>(), a+ra::iota<1>()));
    }
// benchmark
    auto taking_view =
        [](TestRecorder & tr, auto && a)
        {
            auto fa = [&a]()
                      {
                          int c = 0;
                          ra::ply(ra::map([&c](auto && i, auto && j) { c += 2*i-j; },
                                          a+ra::iota<0>(), a+ra::iota<1>()));
                          return c;
                      };
            auto fb = [&a]()
                      {
                          int c = 0;
                          ra::ply_ravel(ra::map([&c](auto && i, auto && j) { c += 2*i-j; },
                                                a+ra::iota<0>(), a+ra::iota<1>()));
                          return c;
                      };

            tr.test_eq(499500000, fa());
            tr.test_eq(499500000, fb());

            auto bench = Benchmark {/* repeats */ 30, /* runs */ 30};

            bench.info("vala").report(std::cout, bench.run(fa), 1e-6);
            bench.info("valb").report(std::cout, bench.run(fb), 1e-6);
        };

    ra::Big<int, 2> const a({1000, 1000}, 0);
    taking_view(tr, a);
    auto b = transpose(a);
    taking_view(tr, b);

    return tr.summary();
}
