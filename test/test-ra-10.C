
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
#include "ra/mpdebug.H"

using std::cout; using std::endl;

void * VP;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("temp lambda");
    {
        ra::Big<ra::Small<double, 2>, 1> V({2*2}, ra::none);
        VP = &V;
        auto i = ra::iota(1);
// goes through Expr, ref or rvalue works just the same.
        ply(from([&](auto i) { tr.info("fwd lambda 1").test(&V==VP); }, i));
// goes through Ryn.
        auto f = [&](auto i, auto j) { tr.info("fwd lambda 2 ref").test(&V==VP); };
        ply(from(f, i, i));
// goes through Ryn. This requires the forward in [ra31].
        ply(from([&](auto i, auto j) { tr.info("fwd lambda 2 rvalue").test(&V==VP); }, i, i));
    }
    return tr.summary();
}
