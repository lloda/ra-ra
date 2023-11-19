// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Assign to expr from the same exact type. This bug was revealed by gcc 11.

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <vector>
#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl;

int main()
{
    ra::TestRecorder tr(std::cout);
    {
        ra::Big<int, 2> C = {{0, 0, 0, 0}, {0, 9, 0, 0}, {0, 0, 9, 0}, {0, 0, 0, 0}};
        ra::Big<int, 2> A({4, 4}, 0), B({4, 4}, 9);
        using coord = ra::Small<int, 2>;
        ra::Big<coord, 1> I = { {1, 1}, {2, 2} };

        at(A, I) = 9;
        tr.test_eq(C, A);
        A = 0;
        at(A, I) = at(B, I);
        tr.test_eq(C, A);
    }
    {
        ra::Big<int, 2> A({4, 4}, 4);
        ra::Big<int, 2> B({4, 4}, 8);
        ra::Big<int, 2> C({4, 4}, 2);
        ra::Big<int, 2> D({4, 4}, 0);
        ra::Big<int, 1> s = { 0, 1, 0, 1 };
        ra::Big<int, 1> z = { 1, 0, 0, 1 };

        pick(s, A, B) = pick(z, C, D); // A(0)=D(0); B(1)=C(1); A(2)=C(2); B(3)=D(3);
        tr.test_eq(A, ra::Big<int, 2> {{0, 0, 0, 0}, {4, 4, 4, 4}, {2, 2, 2, 2}, {4, 4, 4, 4}});
        tr.test_eq(B, ra::Big<int, 2> {{8, 8, 8, 8}, {2, 2, 2, 2}, {8, 8, 8, 8}, {0, 0, 0, 0}});
    }
    return tr.summary();
}
