// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Regressions using from() with temp lambdas.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, ra::TestRecorder;

void * VP;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("temp lambda");
    {
        ra::Big<ra::Small<double, 2>, 1> V({2*2}, ra::none);
        VP = &V;
        auto i = ra::iota(1);
// doesn't use Reframe, ref or rvalue works just the same.
        ply(from([&](auto i) { tr.info("fwd lambda 1").test(&V==VP); }, i));
// uses Reframe.
        auto f = [&](auto i, auto j) { tr.info("fwd lambda 2 ref").test(&V==VP); };
        ply(from(f, i, i));
// uses Reframe. This requires the forward in [ra31].
        ply(from([&](auto i, auto j) { tr.info("fwd lambda 2 rvalue").test(&V==VP); }, i, i));
    }
    return tr.summary();
}
