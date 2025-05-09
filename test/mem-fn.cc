// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Using map with pointers to members. Until uniform call syntax is a thing.

// (c) Daniel Llorens - 2018-2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using real = double;

struct example
{
    float b;
    float & f() { return b; }
};

int main()
{
    TestRecorder tr;
    tr.section("pointer to member");
    {
        ra::Big<example, 2> ex({2, 3}, ra::scalar(example {99}));
        map(std::mem_fn(&example::b), ex) = ra::_0-ra::_1;
        tr.test_eq(0, ex(0, 0).b);
        tr.test_eq(-1, ex(0, 1).b);
        tr.test_eq(-2, ex(0, 2).b);
        tr.test_eq(1, ex(1, 0).b);
        tr.test_eq(0, ex(1, 1).b);
        tr.test_eq(-1, ex(1, 2).b);

        map(std::mem_fn(&example::f), ex) = ra::_1-ra::_0;
        tr.test_eq(0, ex(0, 0).b);
        tr.test_eq(1, ex(0, 1).b);
        tr.test_eq(2, ex(0, 2).b);
        tr.test_eq(-1, ex(1, 0).b);
        tr.test_eq(0, ex(1, 1).b);
        tr.test_eq(1, ex(1, 2).b);
    }
    tr.section("now that Map uses std::invoke() we can use the member directly");
    {
        ra::Big<example, 2> ex({2, 3}, ra::scalar(example {99}));
        map(&example::b, ex) = ra::_0-ra::_1;
        tr.test_eq(0, ex(0, 0).b);
        tr.test_eq(-1, ex(0, 1).b);
        tr.test_eq(-2, ex(0, 2).b);
        tr.test_eq(1, ex(1, 0).b);
        tr.test_eq(0, ex(1, 1).b);
        tr.test_eq(-1, ex(1, 2).b);

        map(&example::f, ex) = ra::_1-ra::_0;
        tr.test_eq(0, ex(0, 0).b);
        tr.test_eq(1, ex(0, 1).b);
        tr.test_eq(2, ex(0, 2).b);
        tr.test_eq(-1, ex(1, 0).b);
        tr.test_eq(0, ex(1, 1).b);
        tr.test_eq(1, ex(1, 2).b);
    }
    return tr.summary();
}
