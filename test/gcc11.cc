// -*- mode: c++; coding: utf-8 -*-
/// @file gcc11.cc
/// @brief going from gcc 10 to gcc 11

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <vector>
#include <iostream>
#include "ra/ra.hh"
#include "ra/test.hh"

using std::cout, std::endl;

int main()
{
    ra::TestRecorder tr(std::cout);
    {
        ra::Big<int, 2> C = {{0, 0, 0, 0}, {0, 11, 0, 0}, {0, 0, 22, 0}, {0, 0, 0, 0}};
        ra::Big<int, 2> A({4, 4}, 0), B({4, 4}, 10*ra::_0 + ra::_1);
        using coord = ra::Small<int, 2>;
        ra::Big<coord, 1> I = { {1, 1}, {2, 2} };

        A = 0;
        map([&A](auto && i) -> decltype(auto) { return A.at(i); }, I)
            = map([&B](auto && i) -> decltype(auto) { return B.at(i); }, I);
        tr.test_eq(C, A);

        A = 0;
        map([&A](auto && i) -> decltype(auto) { return A.at(i); }, I)
            = at(B, I);
        tr.test_eq(C, A);

        A = 0;
        at(A, I)
            = map([&B](auto && i) -> decltype(auto) { return B.at(i); }, I);
        tr.test_eq(C, A);

        A = 0;
        at(A, I) = at(B, I);
        tr.test_eq(C, A);

        A = 0;
        at(A, I) = at(B+1, I);
        tr.test_eq(at(C, I)+1, at(A, I));
    }
    return tr.summary();
}
