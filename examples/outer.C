// -*- mode: c++; coding: utf-8 -*-
// Adapted from blitz++/examples/outer.cpp
// Daniel Llorens - 2015

#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

using std::cout; using std::endl; using std::flush;

int main()
{
    ra::Big<float,1> x { 1, 2, 3, 4 }, y { 1, 0, 0, 1 };
    ra::Big<float,2> A({4,4}, 99.);

    x = { 1, 2, 3, 4 };
    y = { 1, 0, 0, 1 };

    ra::TensorIndex<0> i;
    ra::TensorIndex<1> j;

    A = x(i) * y(j);

    cout << A << endl;

// [ra] an alternative

    cout << from(std::multiplies<float>(), x, y) << endl;

// another alternative (broadcasting)

    cout << x * y(ra::newaxis<1>) << endl;

    return 0;
}
