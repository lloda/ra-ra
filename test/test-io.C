
// (c) Daniel Llorens - 2013-2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-io.C
/// @brief IO checks for ra::. Some ported from old test-traversal.C.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/view-ops.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout; using std::endl; using std::flush;

template <int i> using TI = ra::TensorIndex<i>;
using int3 = ra::Small<int, 3>;
using int2 = ra::Small<int, 2>;

// TestRecorder wants its args to be array elements.
namespace ra { template <> constexpr bool is_scalar_def<std::string> = true; }

template <class AA, class CC>
void iocheck(TestRecorder & tr, AA && a, CC && check)
{
    std::ostringstream o;
    o << a;
    cout << "\nwritten: " << o.str() << endl;
    std::istringstream i(o.str());
    std::decay_t<CC> c;
    i >> c;
    cout << "\nread: " << c << endl;
// TODO start() outside b/c where(bool, vector, vector) not handled. ra::where(bool, scalar, scalar) should work at least.
// TODO specific check in TestRecorder
    tr.test_eq(ra::start(start(check).shape()), ra::start(start(c).shape()));
    tr.test_eq(check, c);
}

int main()
{
    TestRecorder tr;
    tr.section("IO format parameters (I)");
    {
        ra::Small<int, 2, 2> A {1, 2, 3, 4};
        std::ostringstream o;
        o << format_array(A, true, "|", "-");
        tr.test_eq(o.str(), std::string("1|2-3|4"));
    }
    tr.section("IO format parameters (II)");
    {
        ra::Owned<int, 2> A({2, 2}, {1, 2, 3, 4});
        std::ostringstream o;
        o << format_array(A, false, "|", "-");
        tr.test_eq(o.str(), std::string("1|2-3|4"));
    }
    tr.section("common arrays or slices");
    {
        ra::Unique<int, 2> a({5, 3}, ra::_0 - ra::_1);
        ra::Unique<int, 2> ref({5, 3}, a); // TODO how about an explicit copy() function?
        iocheck(tr.info("output of Unique (1)"), a, ref);
        iocheck(tr.info("output of Unique (1)"), a, ref);
    }
    tr.section("[ra02a] printing Expr");
    {
        iocheck(tr.info("output of expr (1)"),
                ra::expr([](double i) { return -i; }, start(ra::Small<double, 3>{0, 1, 2})),
                ra::Small<double, 3>{0, -1, -2});
        iocheck(tr.info("output of expr (1)"),
                ra::expr([](double i) { return -i; }, start(ra::Small<double, 3, 2, 3> (ra::_0 - ra::_1 + ra::_2))),
                (ra::Small<double, 3, 2, 3> (-(ra::_0 - ra::_1 + ra::_2))));
    }
    {
        ra::Unique<int, 2> a({2, 3}, { 1, 2, 3, 4, 5, 6 });
        iocheck(tr.info("output of expr (2)"),
                ra::expr([](int i) { return -i; }, a.iter()),
                ra::Unique<int, 2>({2, 3}, { -1, -2, -3, -4, -5, -6 }));
    }
    tr.section("[ra02b] printing array iterators");
    {
        ra::Unique<int, 2> a({3, 2}, { 1, 2, 3, 4, 5, 6 });
        iocheck(tr.info("output of array through its iterator"), a.iter(), a);
// note that transpose({1, 0}, ...) will have dynamic rank, so the type expected from read must also.
        iocheck(tr.info("output of transposed array through its iterator"),
                transpose({1, 0}, a).iter(),
                ra::Unique<int>({2, 3}, { 1, 3, 5, 2, 4, 6 }));
    }
    tr.section("[ra02c] printing array iterators");
    {
        ra::Small<int, 3, 2> a { 1, 2, 3, 4, 5, 6 };
        iocheck(tr.info("output of array through its iterator"), a.iter(), a);
        iocheck(tr.info("output of transposed array through its iterator"),
                transpose<1, 0>(a).iter(),
                ra::Small<int, 2, 3> { 1, 3, 5, 2, 4, 6 });
    }
    tr.section("IO can handle tensorindex, too");
    {
        iocheck(tr.info("output of expr (1)"),
                ra::expr([](double i, auto j) { return -i*double(j); }, ra::Small<double, 3>{0, 1, 2}.iter(), TI<0>()),
                ra::Small<double, 3>{0, -1, -4});
    }
    tr.section("IO of var rank expression");
    {
        ra::Small<int, 2, 2> A {1, 2, 3, 4};
        ra::Unique<int> B({2, 2}, {1, 2, 3, 4});
        iocheck(tr.info("var rank expr"), A+B, ra::Unique<int>({2, 2}, { 2, 4, 6, 8 }));
    }
    return tr.summary();
}
