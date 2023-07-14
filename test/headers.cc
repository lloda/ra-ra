// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - ra::iota not std::iota.

// (c) Daniel Llorens - 2013-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/ra.hh"
#include <random>

auto I = ra::iota(998, 1);
auto I1 = I+1;

int main()
{
    return 0;
}
