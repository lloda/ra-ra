// -*- mode: c++; coding: utf-8 -*-
// ra-ra/examples - Demo shape agreement rules

// (c) Daniel Llorens - 2015-2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/ra.hh"
#include <iostream>

using std::cout, std::endl;

int main()
{
    // The general shape agreement rule is 'prefix agreement': all the first
    // dimensions must match, all the second dimensions, etc. If some arguments
    // have lower rank than others, then the missing dimensions are ignored.

    // For example:
    ra::Big<float, 3> A({3, 4, 5}, 1.);
    ra::Big<float, 2> B({3, 4}, 2.);
    ra::Big<float, 1> C({3}, 3.);
    ra::Big<float, 3> X({3, 4, 5}, 99.);

    // In the following expression, the shapes of the arguments are:
    // A: [3 4 5]
    // B: [3 4]
    // C: [3]
    // X: [3 4 5] (taken from the shape of the right hand side)
    // All the first dimensions are 3, all the second dimensions are 4, and all
    // the third dimensions are 5, so the expression is valid.
    // Note that the agreement rules are applied equally to the destination argument.
    X = map([](auto && a, auto && b, auto && c) { return a+b-c; }, A, B, C);
    cout << "\nX: " << X << endl;
    // (you can write the expression above as X = A+B-C).

    // This rule comes from the array language J (for function rank 0; see J's
    // documentation). Obvious examples include:
    {
        // multiply any array by a scalar. The shape of a scalar is [];
        // therefore, a scalar agrees with anything.
        ra::Big<float, 2> X = B*7.;
        cout << "\nB*7: " << X << endl;
    }
    {
        // multiply each row of B by a different element of C, X(i, j) = B(i, j)*C(i)
        ra::Big<float, 2> X = B*C;
        cout << "\nB*C: " << X << endl;
    }
    {
        // multiply arrays componentwise (identical shapes agree).
        ra::Big<float, 2> X = B*B;
        cout << "\nB*B: " << X << endl;
    }

    // Some special expressions, such as tensor indices, do not have a
    // shape. Therefore they need to be accompanied by some other expression
    // that does have a shape, or the overall expression is not valid.
    {
        constexpr auto i = ra::iota<0>();
        constexpr auto j = ra::iota<1>();
    // That's why you can do
        ra::Big<float, 2> X({3, 4}, i-j);
        cout << "\ni-j: " << X << endl;
    // but the following would be invalid:
        // ra::Big<float, 2> X = i-j; // no shape to construct X with
    }
    // Axis insertion lets you match arguments more flexibly than simple prefix matching.
    {
        ra::Big<float, 2> A({3, 4}, 0);
        ra::Big<float, 1> b({3}, ra::_0);
        ra::Big<float, 1> c({4}, ra::_0);

        // Compare:

        // [3 4] matches [3] - normal prefix matching. Assign b(i) to A(i, ...)
        A = b;
        cout << "\nA: " << A << endl;

        // [3 4] matches [X 4] - skip 1 dimension when matching. Assign c(i) to A(..., i)
        A = c(ra::insert<1>);
        cout << "\nA: " << A << endl;
    }

    return 0;
}
