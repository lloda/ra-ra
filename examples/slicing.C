
// Daniel Llorens - 2015
// Adapted from blitz++/examples/slicing.cpp

#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;

int main()
{
    ra::Owned<int, 2> A({6, 6}, 99);

// Set the upper left quadrant of A to 5
    A(ra::jvec(3), ra::jvec(3)) = 5;

// Set the upper right quadrant of A to an identity matrix
    A(ra::jvec(3), ra::jvec(3, 3)) = { 1, 0, 0, 0, 1, 0,  0, 0, 1 };

// Set the fourth row to 1 (any of these ---trailing ra::all can be omitted).
    A(3, ra::all) = 1;
    A(ra::jvec(1, 3), ra::all) = 1;
    A(3) = 1;

// Set the last two rows to 0 (any of these)
// @TODO we don't have toEnd yet (would be ra::jvec(2, toEnd-2))
    A(ra::jvec(2, 4), ra::all) = 0;
    A(ra::jvec(2, 4)) = 0;

// Set the bottom right element to 8
    A(5, 5) = 8;

    cout << "A = " << A << endl;
    return 0;
}
