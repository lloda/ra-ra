// -*- mode: c++; coding: utf-8 -*-
/// @file ra-7.cc
/// @brief Const / nonconst tests and regressions.

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;
using real = double;

int main()
{
    TestRecorder tr(std::cout);

    ra::Big<ra::Small<int, 2>, 0> b = ra::scalar(ra::Small<int, 2> {3, 4});

    // this requires a const/nonconst overload in ScalarFlat [ra39] because start(Scalar) just forwards.
    ([&b](auto const & c) { b = c; })(ra::scalar(ra::Small<int, 2> {1, 2}));
    tr.test_eq(ra::start({1, 2}), b());

    return tr.summary();
}
