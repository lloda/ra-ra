// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - Pitfalls of assigning to lower rank

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/ra.hh"
#include <iostream>

using std::cout, std::endl, std::flush;

int main()
{
// Matching flat array to nested array
    {
        ra::Small<double, 3, 2> a = {{1, 2}, {3, 4}, {5, 6}};
// b(i) = a(i, ∀ j), which in practice is b(i) = a(i, 1)
        ra::Small<ra::Small<double, 2>, 3> b = a;
// 2 2 4 4 6 6 (probably)
        cout << b << endl;
    }
}
