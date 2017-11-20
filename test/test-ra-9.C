
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-9.C
/// @brief Regressions in array-in-ra::scalar

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
    return tr.summary();
}
