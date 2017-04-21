
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-10.C
/// @brief Regressions using from() with temp lambdas

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"

void * VP;

int main()
{
    TestRecorder tr(std::cout);
    section("temp lambda");
    {
        ra::Owned<ra::Small<double, 2>, 1> V({2*2}, ra::unspecified);
        VP = &V;
        auto i = ra::iota(2);
// cf [ra31] in wrank.H
        ply(from([&](auto i, auto j) { assert(&V==VP); }, i, i));
    }
    return tr.summary();
}
