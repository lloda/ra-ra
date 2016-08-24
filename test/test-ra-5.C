
// (c) Daniel Llorens - 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-operators.C
/// @brief Tests for regressions on where().

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/mpdebug.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/large.H"

using std::cout; using std::endl;

int main()
{
    TestRecorder tr;

    section("where with rvalue TensorIndex, fails to compile with g++ 5.2 -Os, gives wrong result with -O0");
    {
        tr.test_eq(ra::Small<int, 2> {0, 1},
                   where(ra::Unique<bool, 1> { true, false }, ra::TensorIndex<0>(), ra::TensorIndex<0>()));
        tr.test_eq(ra::Unique<int, 1> { 0, 2 }, where(ra::Unique<bool, 1> { true, false }, 3*ra::_0, 2*ra::_0));
    }

    return tr.summary();
}
