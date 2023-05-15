// -*- mode: c++; coding: utf-8 -*-
// ra/test - Exercise step(k) with k>=rank()

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("case I");
    {
        ra::Small<float, 2, 3> A {{1, 2, 3}, {1, 2, 3}};
        ra::Small<float, 2> B {-1, +1};
        ra::Small<float, 2, 3> C {{99, 99, 99}, {99, 99, 99}};
        C = A * B;

        ra::Small<float, 2> D = {0, 0};
        D += A * B;
        tr.test_eq(-6, D[0]);
        tr.test_eq(6, D[1]);
    }
    tr.section("case II");
    {
        ra::Big<float, 2> A {{1, 2, 3}, {1, 2, 3}};
        ra::Big<float, 1> B {-1, +1};
        ra::Big<float, 2> C({2, 3}, 99.);
        C = A * B;

        ra::Big<float, 1> D({2}, 0.);
        D += A * B;
        tr.test_eq(-6, D[0]);
        tr.test_eq(6, D[1]);
    }
    tr.section("case III");
    {
        ra::Big<float> A ({2, 3}, {1, 2, 3, 1, 2, 3});
        ra::Big<float> B {-1, +1};
        ra::Big<float> C({2, 3}, 99.);
        C = A * B;

        ra::Big<float> D({2}, 0.);
        D += A * B;
        tr.test_eq(-6, D[0]);
        tr.test_eq(6, D[1]);
    }
    return tr.summary();
}
