// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for .at(), ra::at()

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

        // check that at() takes arbitrary rank-1 expr as index
        using int2 = ra::Small<int, 2>;
        for (int i=0; i<C.len(0); ++i) {
            for (int j=0; j<C.len(1); ++j) {
                tr.test_eq(C(i, j), C.at(std::array {i, j}));
                tr.test_eq(C(i, j), C.at(int2 {i, 0} + int2 {0, j}));
            }
        }

        A = 0;
        at(A, I2) = map([&B](auto && i) -> decltype(auto) { return B.at(i); }, I2);
        tr.test_eq(C, A);

        A = 0;
        at(A, I2) = at(B, I2);
        tr.test_eq(C, A);

        A = 0;
        at(A, I2) = at(B+1, I2);
        tr.test_eq(at(C, I2)+1, at(A, I2));

        tr.test_eq(35, sum(at(A, I2)));
        tr.test_eq(276, prod(at(A, I2)));

        at_view(A, I1) = std::array { 10, 20 };
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
    tr.section("Small basic");
    {
        ra::Small<double, 3, 2> s { 1, 4, 2, 5, 3, 6 };
        ra::Small<int, 2> i2 { 1, 1 };
        ra::Small<int, 1> i1 { 1 };
        ra::Small<int, 0> i0 { };
        tr.test_eq(2, ra::rank_s<decltype(at_view(s, i0))>());
        tr.test_eq(1, ra::rank_s<decltype(at_view(s, i1))>());
        tr.test_eq(0, ra::rank_s<decltype(at_view(s, i2))>());
    }
    tr.section("Small ops");
    {
        ra::Small<int, 4, 4> C = {{0, 0, 0, 0}, {0, 11, 0, 0}, {0, 0, 22, 0}, {0, 0, 0, 0}};
        ra::Small<int, 4, 4> A = 0;
        ra::Small<int, 4, 4> B = 10*ra::_0 + ra::_1;
        ra::Big<ra::Small<int, 1>, 1> I1 = { {2}, {0} };
        ra::Big<ra::Small<int, 2>, 1> I2 = { {1, 1}, {2, 2} };
        test(C, A, B, I1, I2);
    }
    tr.section("Expr");
    {
        ra::Small<int, 4, 4> C = {{0, 0, 0, 0}, {0, 11, 0, 0}, {0, 0, 22, 0}, {0, 0, 0, 0}};
        ra::Small<int, 4, 4> A = 0;
        ra::Big<ra::Small<int, 1>, 1> I1 = { {2}, {0} };
        ra::Big<ra::Small<int, 2>, 1> I2 = { {1, 1}, {2, 2} };
        test(C, A, 10*ra::_0 + ra::_1, I1, I2);
    }
    tr.section("Misc I");
    {
        ra::Small<int, 4, 4> A = ra::_0 - ra::_1;
        int i[2] = {1, 2};
        tr.test_eq(-1, A.iter().at(i));
    }
    tr.section("Misc II");
    {
        ra::Big<int, 2> A({4, 4}, 0), B({4, 4}, 10*ra::_0 + ra::_1);
        using coord = ra::Small<int, 2>;
        ra::Big<coord, 1> I = { {1, 1}, {2, 2} };

        at(A, I) = at(B, I);
        tr.test_eq(ra::Big<int>({4, 4}, {0, 0, 0, 0, /**/ 0, 11, 0, 0,  /**/ 0, 0, 22, 0,  /**/  0, 0, 0, 0}), A);

// TODO this is why we need ops to have explicit rank.
        at(A, ra::scalar(coord{3, 2})) = 99.;
        tr.test_eq(ra::Big<int>({4, 4}, {0, 0, 0, 0, /**/ 0, 11, 0, 0,  /**/ 0, 0, 22, 0,  /**/  0, 0, 99, 0}), A);
    }
    tr.section("Misc III"); // [ma30]
    {
        ra::Big<int, 2> A = {{100, 101}, {110, 111}, {120, 121}};
        ra::Big<ra::Small<int, 2>, 2> i = {{{0, 1}, {2, 0}}, {{1, 0}, {2, 1}}};
        ra::Big<int, 2> B = at(A, i);
        tr.test_eq(ra::Big<int, 2> {{101, 120}, {110, 121}}, at(A, i));
    }
    tr.section("Misc IV"); // [ma31]
    {
        ra::Big<int, 2> A = {{100, 101}, {110, 111}, {120, 121}};
        tr.strict().test_eq(A, at_view(A, ra::Small<int, 0> {}));
        tr.strict().test_eq(A(2), at_view(A, ra::Small<int, 1> {2}));
        tr.strict().test_eq(A(2, 0), at_view(A, ra::Small<int, 2> {2, 0}));
    }
    tr.section("Misc V"); // [ma32]
    {
        ra::Big<int, 2> A = {{100, 101}, {110, 111}, {120, 121}};
        ra::Big<int, 2> I = {{0, 1}, {2, 0}, {1, 0}};

        // error if 2!=I.len(1). Iterates over scalars
        println(cout, "{}", at(A, I.iter<1>()));
        tr.strict().test_eq(ra::Small<int, 3> {101, 120, 110}, at(A, I.iter<1>()));

        // L=I.len(1) can be 0, 1, 2. Iterates over dynamic rank (2-L)-views
        println(cout, "{}", at_view(A, I.iter<1>()));
        tr.strict().test_eq(ra::Small<int, 3> {101, 120, 110}, at_view(A, I.iter<1>()));
    }
    return tr.summary();
}
