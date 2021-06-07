// -*- mode: c++; coding: utf-8 -*-
/// @file gcc11.cc
/// @brief Going from gcc 10 to gcc 11

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <vector>
#include <iostream>
#include "ra/ra.hh"
#include "ra/test.hh"

using std::cout, std::endl;

int main()
{
    ra::TestRecorder tr(std::cout);
    return tr.summary();
}
