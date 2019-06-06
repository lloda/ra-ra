// -*- mode: c++; coding: utf-8 -*-
// -*- mode: c++; coding: utf-8 -*-
// Adapted from blitz++/examples/rangexpr.cpp
// Daniel Llorens - 2015

#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

using std::cout, std::endl, ra::PI;

int main()
{
    ra::Big<float, 1> x = cos(ra::iota(8) * (2.0 * PI / 8));
    cout << x << endl;

    return 0;
}
