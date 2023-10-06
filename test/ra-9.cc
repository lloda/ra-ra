// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Regressions in array-in-ra::scalar.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, std::tuple, ra::TestRecorder;
using int2 = ra::Small<int, 2>;
using int1 = ra::Small<int, 1>;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("regression [ra21]");
    {
        ra::Big<int2, 1> b { {1, 10}, {2, 20}, {3, 30} };
        b += ra::scalar(int2 { 1, 0 });
        tr.test_eq(ra::Big<int2, 1> { {2, 10}, {3, 20}, {4, 30} }, b);
        ra::Big<int2, 1> c { {1, 0}, {2, 0}, {3, 0} };
    }
    tr.section("regression [ra22]");
    {
        ra::Big<int2, 1> b { {0, 10}, {0, 20}, {0, 30} };
        ra::Big<int2, 1> c { {1, 0}, {2, 0}, {3, 0} };
        tr.test_eq(ra::scalar("3\n2 0 3 0 4 0"), ra::scalar(format(c + ra::scalar(int2 { 1, 0 })))); // FIXME try both -O3 -O0
        b = c + ra::scalar(int2 { 1, 0 });
        tr.test_eq(ra::Big<int2, 1> { {2, 0}, {3, 0}, {4, 0} }, b);
    }
    tr.section("regression [ra24]");
    {
        {
            tr.test_eq(int1{91}, ra::scalar(int1{88}) + ra::scalar(int1{3}));
        }
        {
            auto a = int1{88};
            auto b = int1{3};
            tr.test_eq(int1{91}, a+b);
        }
        {
            auto a = ra::scalar(int1{88});
            auto b = ra::scalar(int1{3});
            tr.test_eq(int1{91}, a+b);
        }
        {
            auto a = ra::scalar(int1{88});
            auto b = ra::scalar(int1{3});
            tr.test_eq(int1{91}, ra::scalar(a)+ra::scalar(b));
        }
    }
    tr.section("regression [ra25]");
    {
        ra::Big<int1, 1> c { {7}, {8} };
        tr.test_eq(ra::scalar("2\n8 9"), ra::scalar(format(c+1)));
    }
    tr.section("regression [ra26]"); // This uses Scalar.at().
    {
        ra::Big<int1, 1> c { {7}, {8} };
        tr.test_eq(ra::scalar("2\n8 9"), ra::scalar(format(c+ra::scalar(int1{1}))));
    }
    tr.section("regression [ra23]");
    {
// This is just to show that it works the same way with 'scalar' = int as with 'scalar' = int2.
        int x = 2;
        ra::Big<int, 1> c { 3, 4 };
        ra::scalar(x) += c + ra::scalar(99);
        tr.test_eq(2+3+99+4+99, x); // FIXME
    }
    {
        int2 x {2, 0};
        ra::Big<int2, 1> c { {1, 3}, {2, 4} };
        ra::scalar(x) += c + ra::scalar(int2 { 1, 99 });
        tr.test_eq(int2{2, 0}+int2{1, 3}+int2{1, 99}+int2{2, 4}+int2{1, 99}, x);
    }
    tr.section("ra::start() on foreign types");
    {
        auto ref = std::array<int, 4> {12, 77, 44, 1};
        tr.test_eq(2, expr([](int i) { return i; },
                           ra::start(std::vector {1, 2, 3})).at(ra::Small<int, 1>{1}));
        tr.test_eq(ra::start(ref), expr([](int i) { return i; }, ra::start(std::array {12, 77, 44, 1})));
// [ra1] these require ra::start and ra::Expr to forward in the constructor. Clue for why is in the ra::Unique case below.
        tr.test_eq(ra::start(ref), expr([](int i) { return i; }, ra::start(ra::Big<int, 1> {12, 77, 44, 1})));
        tr.test_eq(ra::start(ref), expr([](int i) { return i; }, ra::start(std::vector {12, 77, 44, 1})));
// these require ra::start and ra::Expr constructors to forward (otherwise CTE), but this makes sense, as argname is otherwise always an lref.
        ply_ravel(expr([](int i) { std::cout << "Bi: " << i << std::endl; return i; },
                       ra::start(ra::Unique<int, 1> {12, 77, 44, 1})));
    }
// ra::Vector() constructors are needed for V=std::array (cf test/vector-array.cc).
    tr.section("[ra35] - reference");
    {
        std::array a1 = {1, 2};
        std::vector a2 = {1, 2};

        for_each([](auto && a, auto && b) { a = b; }, ra::vector(a1), 99);
        tr.test_eq(99, ra::start(a1));
    }
    tr.section("[ra35] - value");
    {
        auto fun1 = [] { return std::array {7, 2}; };
        auto fun2 = [] { return std::vector {5, 2}; };

        tr.test_eq(ra::start({7, 2}), ra::vector(fun1()));
        tr.test_eq(ra::start({5, 2}), ra::vector(fun2()));
    }
    tr.section("self assigment of ra::vector");
    {
        auto a0 = std::array {0, 10};
        auto a1 = std::array {1, 11};
        ra::vector(a0) = ra::vector(a1);
        tr.test_eq(1, a0[0]);
        tr.test_eq(11, a0[1]);
        tr.test_eq(1, a1[0]);
        tr.test_eq(11, a1[1]);
    }
// This used to be supported, but it really made no sense. Now v0 is a read-only location so this will ct error [ra42]
    // {
    //     auto fun1 = [](int a) { return std::array {a, a+10}; };
    //     auto v0 = ra::vector(fun1(0));
    //     auto v1 = ra::vector(fun1(1));
    //     v0 = v1;
    //     tr.test_eq(1, v0.at(std::array { 0 }));
    //     tr.test_eq(11, v0.at(std::array { 1 }));
    //     tr.test_eq(1, v1.at(std::array { 0 }));
    //     tr.test_eq(11, v1.at(std::array { 1 }));
    //     v1 += v0;
    //     tr.test_eq(1, v0.at(std::array { 0 }));
    //     tr.test_eq(11, v0.at(std::array { 1 }));
    //     tr.test_eq(2, v1.at(std::array { 0 }));
    //     tr.test_eq(22, v1.at(std::array { 1 }));
    // }
    return tr.summary();
}
