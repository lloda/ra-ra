
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-large.C
/// @brief Tests specific to WithStorage.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/small.H"
#include "ra/iterator.H"
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/large.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/mpdebug.H"

using std::cout; using std::endl; using std::flush;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr;
    tr.section("push_back");
    {
        using int2 = ra::Small<int, 2>;
        std::vector<int2> a;
        a.push_back({1, 2});
        ra::Owned<int2, 1> b;
        b.push_back({1, 2});

        int2 check[1] = {{1, 2}};
        tr.test_eq(check, ra::start(a));
        tr.test_eq(check, b);
    }
    return tr.summary();
}
