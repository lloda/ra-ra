
// (c) Daniel Llorens - 2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-big-0.C
/// @brief Tests specific to WithStorage. Constructors.

#include <iostream>
#include <iterator>
#include "ra/operators.H"
#include "ra/io.H"
#include "ra/view-ops.H"
#include "ra/format.H"
#include "ra/test.H"

using std::cout; using std::endl; using std::flush;

int main()
{
    TestRecorder tr;
    tr.section("constructors");
    {
        tr.section("regression with implicitly declared View constructors [ra38]. Reduced from examples/maxwell.C");
        {
            ra::Big<int, 1> A = {1, 2};
            ra::Big<int, 1> X = {0, 0};
            X(ra::all) = A();
            tr.test_eq(ra::start({1, 2}), X);
        }
        tr.section("need for explicit View::operator=(View const & x) [ra34]");
        {
            ra::Big<int, 1> a {0, 1, 2, 3, 4, 5};
            ra::View<int, 1> const va = a();
            ra::Big<int, 1> x(va); // replacing default operator= by copy-to-view.
            tr.test_eq(x, ra::iota(6));
        }
    }
    tr.section("any rank 1 expression for the shape argument");
    {
        ra::Big<int, 2> a (2+ra::iota(2), {0, 1, 2, 3, 4, 5});
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
    }
    tr.section("also on raw views");
    {
        int ap[6] = {0, 1, 2, 3, 4, 5};
        ra::View<int, 2> a(2+ra::iota(2), ap);
        tr.test_eq(2, a.size(0));
        tr.test_eq(3, a.size(1));
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
        tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
    }
    tr.section("also on raw views with var rank");
    {
        int ap[6] = {0, 1, 2, 3, 4, 5};
        ra::View<int> a(2+ra::iota(2), ap);
        tr.test_eq(2, a.size(0));
        tr.test_eq(3, a.size(1));
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
        tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
    }
    tr.section("nested braces operator=");
    {
        ra::Big<int, 2> a({2, 3}, {0, 1, 2, 3, 4, 5});
        auto ap = a.data();
        a = {{4, 5, 6}, {7, 8, 9}}; // this uses operator=(nested_braces_r)
        tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        a = {{{4, 5, 6}, {7, 8, 9}}}; // this uses the nested_braces_r constructor (!!)
        tr.skip().test_eq(ra::scalar(ap), ra::scalar(a.data())); // FIXME fairly dangerous!
        tr.test_eq(2, a.size(0));
        tr.test_eq(3, a.size(1));
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
    }
    tr.section("nested braces constructor");
    {
        ra::Big<int, 2> a = {{4, 5, 6}, {7, 8, 9}};
        tr.test_eq(2, a.size(0));
        tr.test_eq(3, a.size(1));
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
    }
    tr.section("nested braces for nested type");
    {
        ra::Big<ra::Small<int, 2>, 2> a({2, 2}, { {1, 2}, {2, 3}, {4, 5}, {6, 7} });
        ra::Big<ra::Small<int, 2>, 2> b({{{1, 2},  {2, 3}}, {{4, 5},  {6, 7}}});
        ra::Big<ra::Small<int, 2>, 2> c {{{1, 2},  {2, 3}}, {{4, 5},  {6, 7}}};
        ra::Big<ra::Small<int, 2>, 2> d = {{{1, 2},  {2, 3}}, {{4, 5},  {6, 7}}};
        tr.test_eq(a, b);
    }
    return tr.summary();
}
