// -*- mode: c++; coding: utf-8 -*-
/// @file return-expr.C
/// @brief Show how careful you have to be when you return an expr object from a function.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// For other examples see fun::project_on_plane or ra::normv. (TODO Those need tests).

#include <iostream>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/ra.H"
#include <sstream>

using std::cout, std::endl, std::flush;
using real = double;

template <class A>
inline auto retex_vs(A const & a, real const c)
{
    return a-std::move(c); // @NOTE without move(), ra::scalar -> ra::Scalar<real const &>, which is dangling on return.
}

template <class A>
inline auto retex_vsref(A const & a, real const & c)
{
    return a-c;
}

int main()
{
    TestRecorder tr;
    auto test = [&tr](auto t, real const x)
        {
            using V = decltype(t);
            {
                V a = {1, 0, 0};
                std::ostringstream o;
                o << retex_vs(a, x);
                cout << "q1. " << o.str() << endl;

                V p;
                std::istringstream i(o.str());
                i >> p;
                cout << "q2. " << p << endl;
                tr.test_eq(1-x, p[0]);
                tr.test_eq(-x, p[1]);
                tr.test_eq(-x, p[2]);

                V q = retex_vs(a, 2);
                cout << "q3. " << q << endl;
                tr.test_eq(1-x, p[0]);
                tr.test_eq(-x, p[1]);
                tr.test_eq(-x, p[2]);
            }
            using V = decltype(t);
            {
                V a = {1, 0, 0};
                std::ostringstream o;
                o << retex_vsref(a, x);
                cout << "q1. " << o.str() << endl;

                V p;
                std::istringstream i(o.str());
                i >> p;
                cout << "q2. " << p << endl;
                tr.test_eq(1-x, p[0]);
                tr.test_eq(-x, p[1]);
                tr.test_eq(-x, p[2]);

                V q = retex_vsref(a, 2);
                cout << "q3. " << q << endl;
                tr.test_eq(1-x, p[0]);
                tr.test_eq(-x, p[1]);
                tr.test_eq(-x, p[2]);
            }
        };
    tr.section("vs, small");
    {
        test(ra::Small<real, 3>(), 2);
        test(ra::Small<real, 3>(), 3);
        test(ra::Small<real, 3>(), 4);
    }
    tr.section("vs, large");
    {
        test(ra::Big<real, 1>(), 2);
        test(ra::Big<real, 1>(), 3);
        test(ra::Big<real, 1>(), 4);
    }
    return tr.summary();
}
