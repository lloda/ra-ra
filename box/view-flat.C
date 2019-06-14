// -*- mode: c++; coding: utf-8 -*-
/// @file view-flat.C
/// @brief Can I do View<Iota>, or what does it look like?

// (c) Daniel Llorens - 2018
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/operators.H"
#include "ra/io.H"
#include "ra/test.H"
#include "ra/format.H"
#include <iomanip>
#include <chrono>

using std::cout, std::endl, std::flush;

namespace ra {



} // namespace ra

int main()
{
    auto a = ra::iota(12);
    cout << a << endl;
    return 0;
}
