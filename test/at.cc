// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for .at().

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

    auto test = [&](auto && C, auto && A, auto && B, auto && I1, auto && I2)
    {
        A = 0;
        map([&A](auto && i) -> decltype(auto) { return A.at(i); }, I2)
            = map([&B](auto && i) -> decltype(auto) { return B.at(i); }, I2);
        tr.test_eq(C, A);

        A = 0;
        map([&A](auto && i) -> decltype(auto) { return A.at(i); }, I2)
            = at(B, I2);
        tr.test_eq(C, A);

        A = 0;
        at(A, I2)
            = map([&B](auto && i) -> decltype(auto) { return B.at(i); }, I2);
        tr.test_eq(C, A);

        A = 0;
        at(A, I2) = at(B, I2);
        tr.test_eq(C, A);

        A = 0;
        at(A, I2) = at(B+1, I2);
        tr.test_eq(at(C, I2)+1, at(A, I2));

        tr.test_eq(35, sum(at(A, I2)));
        tr.test_eq(276, prod(at(A, I2)));

        at(A, I1) = std::array { 10, 20 };
        tr.test_eq(std::array { 20, 12, 10, 0 }, A(ra::all, 1));
        tr.test_eq(std::array { 20, 0, 10, 0 }, A(ra::all, 3));
    };

    tr.section("Big");
    {
        ra::Big<int, 2> C = {{0, 0, 0, 0}, {0, 11, 0, 0}, {0, 0, 22, 0}, {0, 0, 0, 0}};
        ra::Big<int, 2> A({4, 4}, 0), B({4, 4}, 10*ra::_0 + ra::_1);
        ra::Big<ra::Small<int, 1>, 1> I1 = { {2}, {0} };
        ra::Big<ra::Small<int, 2>, 1> I2 = { {1, 1}, {2, 2} };
        test(C, A, B, I1, I2);
    }
    tr.section("Small");
    {
        ra::Small<int, 4, 4> C = {{0, 0, 0, 0}, {0, 11, 0, 0}, {0, 0, 22, 0}, {0, 0, 0, 0}};
        ra::Small<int, 4, 4> A = 0;
        ra::Small<int, 4, 4> B = 10*ra::_0 + ra::_1;
        ra::Big<ra::Small<int, 1>, 1> I1 = { {2}, {0} };
        ra::Big<ra::Small<int, 2>, 1> I2 = { {1, 1}, {2, 2} };
        test(C, A, B, I1, I2);
    }
    return tr.summary();
}
