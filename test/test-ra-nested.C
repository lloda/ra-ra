
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-nested.C
/// @brief Using nested arrays as if they were arrays if higher rank.

// @TODO Make more things work and work efficiently.

#include <iostream>
#include <sstream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;
template <class T> using Vec = ra::Owned<T, 1>;

int main()
{
    TestRecorder tr;
    section("operators on nested arrays");
    {
// default init is required to make vector-of-vector, but I still want Vec {} to mean 'an empty vector' and not a default-init vector.
        {
            auto c = Vec<int> {};
            tr.test_equal(0, c.size(0));
            tr.test_equal(0, c.size());
        }
        {
            auto c = Vec<Vec<int>> { Vec<int> {}, Vec<int> {1}, Vec<int> {1, 2} };
            std::ostringstream os;
            cout << "c: " << c << "\n" << endl;
            os << c;
            std::istringstream is(os.str());
            Vec<Vec<int>> d;
            is >> d;
            cout << "d: " << d << "\n" << endl;
            tr.test_equal(3, d.size());
            tr.test_equal(d[0], Vec<int>{});
            tr.test_equal(d[1], Vec<int>{1});
            tr.test_equal(d[2], Vec<int>{1, 2});
// @TODO Actually nested 'as if higher rank' should allow just (every(c==d)). This is explicit nesting.
            tr.test(every(ra::expr([](auto & c, auto & d) { return every(c==d); }, c.iter(), d.iter())));
        }
    }
    return tr.summary();
}
