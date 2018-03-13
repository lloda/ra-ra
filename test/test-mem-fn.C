
// (c) Daniel Llorens - 2018

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-mem-fn.C
/// @brief Using map with pointers to members. Until uniform call syntax is a thing.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/complex.H"
#include "ra/test.H"
#include "ra/big.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout, std::endl, std::flush;
using real = double;

struct example
{
    int a;
    float b;
};

int main()
{
    TestRecorder tr;
    tr.section("pointer to member");
    {
        ra::Big<example, 2> ex({2, 3}, ra::scalar(example {22, 99}));
        map(std::mem_fn(&example::b), ex) = ra::_0-ra::_1;
        tr.test_eq(0, ex(0, 0).b);
        tr.test_eq(-1, ex(0, 1).b);
        tr.test_eq(-2, ex(0, 2).b);
        tr.test_eq(1, ex(1, 0).b);
        tr.test_eq(0, ex(1, 1).b);
        tr.test_eq(-1, ex(1, 2).b);
    }
    return tr.summary();
}
