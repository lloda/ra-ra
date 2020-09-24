// -*- mode: c++; coding: utf-8 -*-
/// @file early.cc
/// @brief Tests for early()

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/ra.hh"
#include "ra/test.hh"

using std::cout, std::endl, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("any, every rank 0");
    {
        ra::Big<int, 0> a = 99;
        tr.test(any(a==99));
        tr.test(every(a==99));
    }
    tr.section("any, every static rank");
    {
        ra::Big<int, 2> a({2, 3}, ra::_0*2 + ra::_1*10);
        tr.test(!any(odd(a)));
        tr.test(every(!odd(a)));
    }
    tr.section("any, every dyn rank");
    {
        ra::Big<int> a = ra::iota(9)*2;
        tr.test(!any(odd(a)));
        tr.test(every(!odd(a)));
    }
    return tr.summary();
}
