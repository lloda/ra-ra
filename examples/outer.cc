// -*- mode: c++; coding: utf-8 -*-
// Adapted from blitz++/examples/outer.cpp
// Daniel Llorens - 2015, 2019

#include "ra/ra.hh"
#include <iostream>

using std::cout, std::endl, std::flush;

int main()
{
    ra::Big<float,1> x { 1, 2, 3, 4 }, y { 1, 0, 0, -1 };
    ra::Big<float,2> A({4,4}, 99.);

    x = { 1, 2, 3, 4 };
    y = { 1, 0, 0, -1 };

    ra::TensorIndex<0> i;
    ra::TensorIndex<1> j;

    A = x(i) * y(j);

    cout << A << endl;

// [ra] alternative I

    cout << from(std::multiplies<float>(), x, y) << endl;

// [ra] alternative II by axis insertion

    cout << (x * y(ra::insert<1>)) << endl;

// [ra] alternative II by transposing

    cout << (x * transpose<1>(y)) << endl;

    return 0;
}
