
// Daniel Llorens - 2015
// Adapted from blitz++/examples/cast.cpp
// Nested heterogeneous arrays

#include "ra/ra-operators.H"
#include <iostream>

using std::cout; using std::endl;

int main()
{
    ra::Owned<ra::Owned<int, 1>, 1> A({3}, ra::unspecified);

    A(0).resize(3);
    A(0) = { 0, 1, 2 };

    A(1).resize(5);
    A(1) = { 5, 7, 18, 2, 1 };

    A(2).resize(4);
    A(2) = pow(ra::_0+1, 2);

    cout << "A = " << A << endl;

    // A = [ [ 0 1 2 ] [ 5 7 18 2 1 ] [ 1 4 9 16 ] ]

    // [ra] Note that this is written with the shape in front, so (without the brackets)
    // [3 [3 [0 1 2]]  [5 [5 7 18 2 1]] [4 [1 4 9 16]]]
    // Therefore, the type of A must be known to read this back in.

    return 0;
}
