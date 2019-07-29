// -*- mode: c++; coding: utf-8 -*-
/// @file tensorindex.C
/// @brief Limitations of ra::TensorIndex.

// (c) Daniel Llorens - 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/ra.H"
#include "ra/test.H"

using std::cout, std::endl, std::flush;

int main()
{
    auto i = ra::_0;
    auto j = ra::_1;

    TestRecorder tr(std::cout);
    {
        ra::Big<float, 1> x({4}, i + 1), y { 1, 0, 2, -1 }, z({4}, -1);
        ra::Big<float, 2> A({4, 4}, i - 2*j);
        ra::Big<float, 2> B({4, 4}, 0.);

// the shape of the expression here is determined by A. This works because of an explicit specialization of single-argument select (from()) in wrank.H; the generic version of from() doesn't allow it. Not sure if it should be kept.

        A = x(i) * y(j);
        tr.test_eq(ra::Big<float, 2>({4, 4}, {1, 0, 2, -1,  2, 0, 4, -2,  3, 0, 6, -3,  4, 0, 8, -4}), A);

// this won't do what you might expect because the selection op is implicitly outer-product:
        // z = A(i, i);
// use a non-product selector instead.
        z = 0.;
        z = map(A, i, i);
        tr.info("diagonal").test_eq(ra::Big<float, 1> {1, 0, 6, -4}, z);

// generally expressions using undelimited subscript TensorIndex should use ra::map and not ra::from.
        B = 0.;
        B = map(A, i, j);
        tr.info("copy")
            .test_eq(ra::Big<float, 2>({4, 4}, {1, 0, 2, -1,  2, 0, 4, -2,  3, 0, 6, -3,  4, 0, 8, -4}), B);

        B = 0.;
        B = map(A, j, i);
        tr.info("transpose 1").test_eq(transpose({1, 0}, A), B);

// the map will work on either side of =. Note however that map(B, i, j) = map(A, j, i) won't work b/c the extent of the overall expr is undelimited.
        B = 0.;
        map(B, j, i) = A;
        tr.info("transpose 2").test_eq(transpose({1, 0}, A), B);

// normal rank agreement can be abused to do some kinds of reductions.
        z = 0.;
        z += A;
        tr.info("reduction last axis").test_eq(ra::Big<float, 1> {2, 4, 6, 8}, z);

// however z += map(A, i, j) doesn't work beucase the extent of the driving term is undelimited.
    }

    return tr.summary();
}
