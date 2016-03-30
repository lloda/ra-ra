
// (c) Daniel Llorens - 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-operators.C
/// @brief Tests for operators on ra:: expr templates.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/mpdebug.H"
#include "ra/ra-operators.H"
#include "ra/ra-large.H"

using std::cout; using std::endl;

int main()
{
    TestRecorder tr;

    section("where with rvalue TensorIndex, fails to compile with g++ 5.2 -Os, gives wrong result with -O0");
    {
        // ply_index(where(ra::Unique<bool, 1> { true, false }, ra::TensorIndex<0>(), ra::TensorIndex<0>())); // WORKS
        cout << where(ra::Unique<bool, 1> { true, false }, ra::TensorIndex<0>(), ra::TensorIndex<0>()) << endl; // DOESN'T WORK
        // cout << "\nx\n" << endl;
        // ply_index(expr([](int a) { cout << "a: " << a << endl; }, where(ra::Unique<bool, 1> { true, false }, 3, 2)));
        // cout << where(ra::Unique<bool, 1> { true, false }, ra::_0, ra::_0) << endl << endl;
        // cout << where(ra::Unique<bool, 1> { true, false }, 3*ra::_0, ra::Unique<int, 1> { 0, 2 }) << endl << endl;
        // cout << ra::Unique<int, 1> { 1, -2 } + ra::_0 << endl;
        // cout << where(ra::Unique<bool, 1> { true, false }, ra::Unique<int, 1> { 0, 3 }, ra::Unique<int, 1> { 0, 2 }) << endl;
        // tr.test_eq(ra::Unique<int, 1> { 0, 2 }, where(ra::Unique<bool, 1> { true, false }, 3*ra::_0, 2*ra::_0));
    }

    return tr.summary();
}
