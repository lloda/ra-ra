// -*- mode: c++; coding: utf-8 -*-
/// @file ra-5.cc
/// @brief A regression test.

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Regression test for a bug ... caught first in fold_mat @ array.cc.
// Caused by d139794396a0d51dc0c25b0b03b2a2ef0e2760b5 : Remove set() from cell_iterator_big, cell_iterator_small.

#include <iostream>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    {

        ra::Big<int, 1> b = { 2, 1 };
        ra::Big<int, 2> A({3, 5}, ra::_0 - ra::_1);
        ra::Big<int, 2> F({2, 5}, 0);

// This creates View & cell_iterator_big on each call of A(b(0) ...) as the driver is b and A is handled as a generic object with operator().
// This seems unnecessary; I should be able to create a single cell_iterator_big and just bump a pointer as I move through b. Hmm.
        iter<-1>(F) = b*A(b);
        int Fcheck[2][5] = { {4, 2, 0, -2, -4}, {1, 0, -1, -2, -3} };
        tr.test_eq(Fcheck, F);
    }

// Why: if x(0) is a temp, as in here, cell_iterator_big needs a copy of x(0).dim.
// This is achieved by forwarding in start() -> iter() -> View.iter().
    {
        auto demo = [](auto & x) { return iter<0>(x(0)); };

        ra::Big<int, 2> A({3, 5}, 0);
        auto z = demo(A);
        tr.test_eq(5, z.dimv[0].len);
        tr.test_eq(false, std::is_reference_v<decltype(z)::Dimv>);

        auto y = A(0);
        auto yi = iter<0>(y);
        tr.test_eq(true, std::is_reference_v<decltype(yi)::Dimv>);
    }

// const/nonconst begin :p
    {
        ra::Big<int> A({2, 3}, 3);
        auto const b = A();
        int x[6] = { 0, 0, 0, 0, 0, 0 };
        std::copy(b.begin(), b.end(), x);
        tr.test_eq(3, ra::start(x));
    }
    return tr.summary();
}
