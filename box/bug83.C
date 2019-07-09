// -*- mode: c++; coding: utf-8 -*-
/// @file bug83.C
/// @brief Triggers gcc 8.3 ICE: in verify_ctor_sanity, at cp/constexpr.c:2805

// (c) Daniel Llorens - 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// reduced from bench/bench-stencil2.C.
// Patched by removing constexpr in big.H: View(): p(nullptr) {}.

#include <iostream>
#include <iomanip>
#include <random>
#include "ra/big.H"

int main()
{
    ra::Big<double, 2> A({4, 4}, 0.);
    A = ra::expr([](auto && a) { return a(0, 0); }, ra::iter<2>(A));
}
