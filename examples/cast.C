// -*- mode: c++; coding: utf-8 -*-
// Adapted from blitz++/examples/cast.cpp
// Daniel Llorens - 2015

#include "ra/operators.H"
#include "ra/io.H"
#include <iostream>

using std::cout; using std::endl;

int main()
{
    ra::Big<int, 1> A { 1, 2, 3, 5 }, B { 2, 2, 2, 7 };
    ra::Big<float, 1> C({4}, 0);

// OT: this is a peculiarity of ra:: : C had to be initialized with the right
// size or C = A/B below will fail because of a shape mismatch. This is so C =
// ... works the same way whether C is an owned type or a view. You can always
// initialize with a new object:
    {
        ra::Big<float, 1> D;
        D = ra::Big<float, 1>(A / B);
    }

    C = A / B;
    cout << C << endl;

    C = A / ra::cast<float>(B);
    cout << C << endl;

    return 0;
}
