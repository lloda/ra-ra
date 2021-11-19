// -*- mode: c++; coding: utf-8 -*-
// Adapted from blitz++/examples/rangexpr.cpp
// Daniel Llorens - 2015

#include "ra/ra.hh"
#include <iostream>
#include <numbers>

using std::cout, std::endl;

int main()
{
    ra::Big<float, 1> x = cos(ra::iota(8) * (2.0 * std::numbers::pi_v<float> / 8));
    cout << x << endl;

    return 0;
}
