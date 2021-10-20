// -*- mode: c++; coding: utf-8 -*-
// Pitfalls of assigning to lower rank
// Daniel Llorens - 2021

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
