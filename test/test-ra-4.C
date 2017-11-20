
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-4.C
/// @brief Regresion tests for attempt at simplification.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/expr.H"

using std::cout; using std::endl; using std::flush;
using complex = std::complex<double>;

template <class AA>
double sqrm_ai(AA && a)
{
    double c(0.);
    ply(ra::expr([&c](complex const a) { c += sqrm(a); }, a));
    return c;
}

int main()
{
    TestRecorder tr;
    ra::Unique<complex, 1> a({3}, {1, 2, 3});
    tr.test_eq(14, sqrm_ai(ra::expr([](complex a) { return a; }, a.iter())));
    return tr.summary();
}
