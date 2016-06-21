
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-explode-collapse.C
/// @brief Tests for explode() and collapse().

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/ra-large.H"
#include "ra/ra-operators.H"

using std::cout; using std::endl; using std::flush;

using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr(std::cout);

    section("collapse (~ multicomponents in Blitz++");
    {
        section("sub is real to super complex");
        {
            ra::Unique<complex, 2> A({4, 4}, ra::cast<double>(ra::_0)*complex(4, 1) + ra::cast<double>(ra::_1)*complex(1, 4));
            auto B = ra::collapse<double>(A);
            tr.test_eq(real_part(A), B(ra::all, ra::all, 0));
            tr.test_eq(imag_part(A), B(ra::all, ra::all, 1));
        }
        section("sub is int to super Small of rank 1");
        {
            using r2 = ra::Small<int, 2>;
            ra::Unique<r2, 2> A({4, 4}, map([](int i, int j) { return r2 {i+j, i-j}; }, ra::_0, ra::_1));
            auto B = ra::collapse<int>(A);
            tr.test_eq(B(ra::all, ra::all, 0), map([](auto && a) { return a(0); }, A));
            tr.test_eq(B(ra::all, ra::all, 1), map([](auto && a) { return a(1); }, A));
        }
        section("sub is int to super Small of rank 2");
        {
            using super = ra::Small<int, 2, 3>;
            ra::Unique<super, 2> A({2, 2}, map([](int i, int j) { return super(i-j+ra::_0-ra::_1); }, ra::_0, ra::_1));
            auto B = ra::collapse<int>(A);
            for (int i=0; i<super::size(0); ++i) {
                for (int j=0; j<super::size(1); ++j) {
                    tr.test_eq(B(ra::all, ra::all, i, j), map([i, j](auto && a) { return a(i, j); }, A));
                }
            }
        }
        section("sub is Small of rank 1 to super Small of rank 2");
        {
            using super = ra::Small<int, 2, 3>;
            ra::Unique<super, 2> A({2, 2}, map([](int i, int j) { return super(i-j+ra::_0-ra::_1); }, ra::_0, ra::_1));
            using sub = ra::Small<int, 3>;
            auto B = ra::collapse<sub>(A);
// @TODO sub() is used to cover a problem with where() and SmallSlice/SmallArray, since they convert to each other
            tr.test_eq(B(ra::all, ra::all, 0), map([](auto && a) { return sub(a(0)); }, A));
            tr.test_eq(B(ra::all, ra::all, 1), map([](auto && a) { return sub(a(1)); }, A));
        }
        section("sub is real to super complex Small of rank 2");
        {
            using super = ra::Small<complex, 2, 2>;
            ra::Unique<super, 2>
                A({4, 4}, map([](complex a) { return super { a, conj(a), -conj(a), -a }; },
                              ra::cast<double>(ra::_0)*complex(4, 1) + ra::cast<double>(ra::_1)*complex(1, 4)));
            auto B = ra::collapse<double>(A);
            for (int i=0; i<super::size(0); ++i) {
                for (int j=0; j<super::size(1); ++j) {
                    tr.test_eq(B(ra::all, ra::all, i, j, 0), map([i, j](auto && a) { return real_part(a(i, j)); }, A));
                    tr.test_eq(B(ra::all, ra::all, i, j, 1), map([i, j](auto && a) { return imag_part(a(i, j)); }, A));
                }
            }
        }
    }
    return tr.summary();
}
