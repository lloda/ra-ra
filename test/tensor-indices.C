// -*- mode: c++; coding: utf-8 -*-
/// @file tensor-indices.C
/// @brief Compare TensorIndex with test/old.H:OldTensorIndex that required ply_index.

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/bench.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"
#include "test/old.H"

using std::cout, std::endl, std::flush;

int main()
{
    TestRecorder tr;

    // tests for old ra::OldTensorIndex
    {
        tr.info("by_index(OldTensorIndex)").test(ra::by_index<decltype(ra::OldTensorIndex<0> {})>);
        tr.info("by_index(Expr)").test(ra::by_index<decltype(ra::OldTensorIndex<0> {}+ra::OldTensorIndex<1> {})>);
    }
    {
        ra::Unique<int, 2> a({3, 2}, ra::none);
        auto dyn = ra::expr([](int & a, int b) { a = b; }, a.iter(), ra::OldTensorIndex<0> {});
        static_assert(ra::by_index<decltype(dyn)>, "bad by_index test 1");
        ply_index(dyn);
        tr.test_eq(ra::_0, a);
    }
    {
        ra::Unique<int, 2> a({3, 2}, ra::none);
        auto dyn = ra::expr([](int & dest, int const & src) { dest = src; }, a.iter(), ra::OldTensorIndex<0> {});
        static_assert(ra::by_index<decltype(dyn)>, "bad by_index test 2");
        ply_index(dyn);
        tr.test_eq(ra::_0, a);
    }
    // rank 1
    {
        ra::Big<int, 1> a = {0, 0, 0};
        ra::ply_index(map([](auto && i) { std::cout << "i: " << i << std::endl; },
                          a+ra::OldTensorIndex<0> {}));
        ra::ply_index(map([](auto && i) { std::cout << "i: " << i << std::endl; },
                          a+ra::TensorIndex<0> {}));
        ra::ply_ravel(map([](auto && i) { std::cout << "i: " << i << std::endl; },
                          a+ra::TensorIndex<0> {}));
    }
    // rank 2
    {
        ra::Big<int, 2> a = {{0, 0, 0}, {0, 0, 0}};
        ra::ply_index(map([](auto && i, auto && j) { std::cout << "i: " << i << ", " << j << std::endl; },
                          a+ra::OldTensorIndex<0> {}, a+ra::OldTensorIndex<1> {}));
        ra::ply_index(map([](auto && i, auto && j) { std::cout << "i: " << i << ", " << j << std::endl; },
                          a+ra::TensorIndex<0> {}, a+ra::TensorIndex<1> {}));
        ra::ply_ravel(map([](auto && i, auto && j) { std::cout << "i: " << i << ", " << j << std::endl; },
                          a+ra::TensorIndex<0> {}, a+ra::TensorIndex<1> {}));
    }
    // benchmark
    auto taking_view =
        [](TestRecorder & tr, auto && a)
        {
            auto fa = [&a]()
                      {
                          int c = 0;
                          ra::ply_index(ra::map([&c](auto && i, auto && j) { c += 2*i-j; },
                                                a+ra::OldTensorIndex<0> {}, a+ra::OldTensorIndex<1> {}));
                          return c;
                      };
            auto fb = [&a]()
                      {
                          int c = 0;
                          ra::ply_index(ra::map([&c](auto && i, auto && j) { c += 2*i-j; },
                                                a+ra::TensorIndex<0> {}, a+ra::TensorIndex<1> {}));
                          return c;
                      };
            auto fc = [&a]()
                      {
                          int c = 0;
                          ra::ply_ravel(ra::map([&c](auto && i, auto && j) { c += 2*i-j; },
                                                a+ra::TensorIndex<0> {}, a+ra::TensorIndex<1> {}));
                          return c;
                      };

            tr.test_eq(499500000, fa());
            tr.test_eq(499500000, fb());
            tr.test_eq(499500000, fc());

            auto bench = Benchmark {/* repeats */ 30, /* runs */ 30};

            bench.info("vala").report(std::cout, bench.run(fa), 1e-6);
            bench.info("valb").report(std::cout, bench.run(fb), 1e-6);
            bench.info("valc").report(std::cout, bench.run(fc), 1e-6);
        };

    ra::Big<int, 2> const a({1000, 1000}, 0);
    taking_view(tr, a);
    auto b = transpose<1, 0>(a);
    taking_view(tr, b);

    return tr.summary();
}
