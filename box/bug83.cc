// -*- mode: c++; coding: utf-8 -*-
// ra-ra/box - Triggers gcc 8.3 ICE: in verify_ctor_sanity, at cp/constexpr.c:2805

// (c) Daniel Llorens - 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// reduced from bench/bench-stencil2.cc.
// Bisected to edad804087b3eb0d2c4b2b423aacc4341b7a6736.
// Patched by removing constexpr in big.hh: View(): p(nullptr) {}.

#include <iostream>
#include <iomanip>
#include <random>
#include "ra/big.hh"

int main()
{
    ra::Big<double, 2> A({4, 4}, 0.);
    A = ra::expr([](auto && a) { return a(0, 0); }, ra::iter<2>(A));
}
