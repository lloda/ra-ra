// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - Slicing

// Daniel Llorens - 2015
// Adapted from blitz++/examples/slicing.cpp

#include "ra/ra.hh"
#include <iostream>

using std::cout, std::endl, std::flush;

int main()
{
    ra::Big<int, 2> A({6, 6}, 99);

// Set the upper left quadrant of A to 5
    A(ra::iota(3), ra::iota(3)) = 5;

// Set the upper right quadrant of A to an identity matrix
    A(ra::iota(3), ra::iota(3, 3)) = { 1, 0, 0,  0, 1, 0,  0, 0, 1 };

// Set the fourth row to 1 (any of these ---trailing ra::all can be omitted).
    A(3, ra::all) = 1;
    A(ra::iota(1, 3), ra::all) = 1;
    A(3) = 1;

// Set the last two rows to 0. These are equivalent.
// ra::len represents the length of the subscribed axis, so iota(2, len-2) means the last 2 indices on that axis.
    A(ra::iota(2, ra::len-2), ra::all) = 0;
    A(ra::iota(2, ra::len-2)) = 0;
// ra::len cannot be an array element, but the following is valid (and equivalent to the previous two).
    A(ra::len + std::array { -2, -1 }) = 0;

// Set the bottom right element to 8. These are equivalent.
    A(5, 5) = 8;
    A(ra::len-1, ra::len-1) = 8;

    cout << "\nA = " << A << endl;

// Demonstrating multi-axis selectors. Use these to skip over any number of
// axes, just as ra::all skips over one axis.
    ra::Big<int, 3> B({3, 5, 5}, 99);

// These are equivalent.
    B(ra::all, ra::all, 2) = 3.;
    B(ra::dots<2>, 2) = 3.;
    B(ra::dots<>, 2) = 3.;

// These are all equivalent.
    B(1, ra::all, ra::all) = 7.;
    B(1) = 7.;
    B(1, ra::dots<2>) = 7.;

// You can use [] instead of () for multi-indices.
    B[1, ra::all, ra::all] = 7.;
    B[1] = 7.;
    B[1, ra::dots<2>] = 7.;

// These are equivalent.
    B(2, ra::all, 1) = 5.;
    B(2, ra::dots<1>, 1) = 5.;

// These are equivalent.
    B(0, 2, 3) = 1.;
    B(ra::dots<0>, 0, ra::dots<0>, 2, ra::dots<0>, 3, ra::dots<0>) = 1.;

    cout << "\nB = " << B << endl;

    return 0;
}
