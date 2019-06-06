// -*- mode: c++; coding: utf-8 -*-
/// @file ra-9.C
/// @brief Regressions in array-in-ra::scalar

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/mpdebug.H"
#include "ra/complex.H"
#include "ra/format.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush; using std::tuple;
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
// TODO see related [ra35] below.
    tr.section("ra::start() on foreign types");
    {
        auto ref = std::array<int, 4> {12, 77, 44, 1};
        tr.test_eq(2, expr([](int i) { return i; },
                           ra::start(std::vector<int> {1, 2, 3})).at(ra::Small<int, 1>{1}));
        tr.test_eq(ra::start(ref), expr([](int i) { return i; }, ra::start(std::array<int, 4> {12, 77, 44, 1})));
// [ra01] these require ra::start and ra::Expr to forward in the constructor. Clue of why is in the ra::Unique case below.
        tr.test_eq(ra::start(ref), expr([](int i) { return i; }, ra::start(ra::Big<int, 1> {12, 77, 44, 1})));
        tr.test_eq(ra::start(ref), expr([](int i) { return i; }, ra::start(std::vector<int> {12, 77, 44, 1})));
// these require ra::start and ra::Expr constructors to forward (otherwise CTE), but this makes
// sense, as argname is otherwise always an lref.
        ply_ravel(expr([](int i) { std::cout << "Bi: " << i << std::endl; return i; },
                       ra::start(ra::Unique<int, 1> {12, 77, 44, 1})));
// This depends on ra::Vector constructors moving the Unique through Expr's copying.
        tr.test_eq(ra::vector(ref), expr([](int i) { return i; }, ra::vector(ra::Unique<int, 1> {12, 77, 44, 1})));
    }
// TODO Find out why the ra::Vector() constructors are needed for V=std::array but not for V=std::vector.
    tr.section("[ra35]");
    {
        std::array<int, 2> a1 = {1, 2};
        std::vector<int> a2 = {1, 2};
        auto va1 = ra::vector(a1);
        auto va2 = ra::vector(a2);

        tr.test(std::is_reference_v<decltype(va1.v)>);
        tr.test(std::is_reference_v<decltype(va2.v)>);

        cout << "&(va1.v[0])   " << &(va1.v[0]) << endl;
        cout << "&(va1.p__[0]) " << &(va1.p__[0]) << endl;
        cout << "&va1          " << &va1 << endl;
        tr.test_eq(ra::scalar(&(va1.v[0])), ra::scalar(&(va1.p__[0])));
        tr.test_eq(ra::scalar(&(va1.v[0])), ra::scalar(&(a1[0])));

        cout << "&(va2.v[0])   " << &(va2.v[0]) << endl;
        cout << "&(va2.p__[0]) " << &(va2.p__[0]) << endl;
        cout << "&va2          " << &va2 << endl;
        tr.test_eq(ra::scalar(&(va2.v[0])), ra::scalar(&(va2.p__[0])));
        tr.test_eq(ra::scalar(&(va2.v[0])), ra::scalar(&(a2[0])));

        cout << "---------" << endl;

        for_each([](auto && a, auto && b) { a = b; }, ra::vector(a1), 99);
        tr.test_eq(99, ra::start(a1));

        cout << "---------" << endl;

        auto fun1 = []() { return std::array<int, 2> {1, 2}; };
        auto fun2 = []() { return std::vector<int> {1, 2}; };
        auto v1 = ra::vector(fun1());
        auto v2 = ra::vector(fun2());

        tr.test(!std::is_reference_v<decltype(v1.v)>);
        tr.test(!std::is_reference_v<decltype(v2.v)>);

        cout << "&(v1.v[0])   " << &(v1.v[0]) << endl;
        cout << "&(v1.p__[0]) " << &(v1.p__[0]) << endl;
        cout << "&v1          " << &v1 << endl;
        tr.test_eq(ra::scalar(&(v1.v[0])), ra::scalar(&(v1.p__[0])));

        cout << "&(v2.v[0])   " << &(v2.v[0]) << endl;
        cout << "&(v2.p__[0]) " << &(v2.p__[0]) << endl;
        cout << "&v2          " << &v2 << endl;
        tr.test_eq(ra::scalar(&(v2.v[0])), ra::scalar(&(v2.p__[0])));

        tr.test_eq(ra::vector(fun1()), ra::iota(2, 1));
        tr.test_eq(ra::vector(fun2()), ra::iota(2, 1));
    }
    return tr.summary();
}
