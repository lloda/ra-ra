
// (c) Daniel Llorens - 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-7.C
/// @brief Const / nonconst tests and regressions.

#include <iostream>
#include <iterator>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush; using std::tuple;
using real = double;

int main()
{
    TestRecorder tr(std::cout);

    ra::Big<ra::Small<int, 2>, 0> b = ra::scalar(ra::Small<int, 2> {3, 4});

    // this requires a const/nonconst overload in ScalarFlat [ra39] because start(Scalar) just forwards.
    ([&b](auto const & c) { b.view() = c; })(ra::scalar(ra::Small<int, 2> {1, 2}));
    tr.test_eq(ra::start({1, 2}), b());

    return tr.summary();
}
