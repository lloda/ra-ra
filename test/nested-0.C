// -*- mode: c++; coding: utf-8 -*-
/// @file nested-0.C
/// @brief Using nested arrays as if they were arrays if higher rank.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Make more things work and work efficiently.

#include <iostream>
#include <sstream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/ra.H"

using std::cout, std::endl, std::flush, std::cerr, ra::TestRecorder;
template <class T, int N> using Array = ra::Big<T, N>;
template <class T> using Vec = Array<T, 1>;

int main()
{
    TestRecorder tr;
    tr.section("operators on nested arrays");
    {
// default init is required to make vector-of-vector, but I still want Vec {} to mean 'an empty vector' and not a default-init vector.
        {
            auto c = Vec<int> {};
            tr.test_eq(0, c.size(0));
            tr.test_eq(0, c.size());
        }
        {
            auto c = Vec<Vec<int>> { {} , {1}, {1, 2} };
            std::ostringstream os;
            os << c;
            std::istringstream is(os.str());
            Vec<Vec<int>> d;
            is >> d;
            tr.test_eq(3, d.size());
            tr.test_eq(d[0], Vec<int>{});
            tr.test_eq(d[1], Vec<int>{1});
            tr.test_eq(d[2], Vec<int>{1, 2});
// TODO Actually nested 'as if higher rank' should allow just (every(c==d)). This is explicit nesting.
            tr.test(every(ra::expr([](auto & c, auto & d) { return every(c==d); }, c.iter(), d.iter())));
        }
    }
    tr.section("selector experiments");
    {
// These is an investigation of how to make a(ra::all, i) or a(i, ra::all) work.
// The problem with a(ra::all, i) here is that we probably want to leave the iteration on ra::all for last. Otherwise the indexing is redone for each rank-1 cell.
        Vec<int> i = {0, 3, 1, 2};
        Array<double, 2> a({4, 4}, ra::_0-ra::_1);
        Array<double, 2> b = from([](auto && a, auto && i) -> decltype(auto) { return a(i); }, a.iter<1>(), start(i));
        tr.test_eq(a(0, i), b(0));
        tr.test_eq(a(1, i), b(1));
        tr.test_eq(a(2, i), b(2));
        tr.test_eq(a(3, i), b(3));
// The problem with a(i) = a(i, ra::all) is that a(i) returns a nested expression, so it isn't equivalent to a(i, [0 1 ...]), and if we want to write it as a rank 2 expression, we can't use from() as above because the iterator we want is a(i).iter(), it depends on i.
// So ...
    }
    tr.section("copying btw arrays nested in the same way");
    {
        Vec<ra::Small<int, 2> > a {{1, 2}, {3, 4}, {5, 6}};
        ra::Small<ra::Small<int, 2>, 3> b = a;
        tr.test_eq(ra::Small<int, 2> {1, 2}, b(0));
        tr.test_eq(ra::Small<int, 2> {3, 4}, b(1));
        tr.test_eq(ra::Small<int, 2> {5, 6}, b(2));
        b(0) = ra::Small<int, 2> {7, 9};
        b(1) = ra::Small<int, 2> {3, 4};
        b(2) = ra::Small<int, 2> {1, 6};
        a = b;
        tr.test_eq(ra::Small<int, 2> {7, 9}, a(0));
        tr.test_eq(ra::Small<int, 2> {3, 4}, a(1));
        tr.test_eq(ra::Small<int, 2> {1, 6}, a(2));
    }
    tr.section("TODO copying btw arrays nested in different ways");
    {
        Vec<ra::Small<int, 2> > a {{1, 2}, {3, 4}, {5, 6}};
        Array<int, 2> b({3, 2}, {1, 2, 3, 4, 5, 6});
// there's one level of matching so a(0) matches to b(0, 0) and b(0, 1), a(1) to b(1, 0) and b(1, 1), etc. So this results in a(0) = 1 overwritten with a(0) = 2, etc. finally a = [[2, 2], [4, 4], [6, 6]]. Probably not what we want.
        a = b;
        // b = a; // dnc b/c [x, y] ← z is ok but z ← [x, y] is not.
        cout << a << endl;
    }
    return tr.summary();
}
