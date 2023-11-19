// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Handling of scalar std:: functions that have been extended for arrays in ra::.

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, ra::TestRecorder;
using real = double;

int main()
{
    TestRecorder tr;

    tr.section("std");
    {
        ra::Small<real, 3> a = { 1, 2, 3 };
        ra::Small<real, 3> b = { 4, 5, 6 };
        cout << lerp(a, b, 0.5) << endl; // this is std::lerp put in :: in ra/ra.hh
        cout << lerp(4., 1., 0.5) << endl; // this is ra::lerp found through ADL
    }
    return tr.summary();
}
