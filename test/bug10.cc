// -*- mode: c++; coding: utf-8 -*-
// ek/box - Bug copying viewsmall<small> to small<small> :-/

// (c) Daniel Llorens - 2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/small.hh"
#include <iostream>
#include "test/mpdebug.hh"

using std::cout;

// FIXME Bug only shows up with all of: --no-sanitize -O3, no assert (1)

int main()
{
    ra::Small<double, 2, 2> a = {{1, 2}, {4, 5}};
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wuninitialized" // !!
    ra::Small<ra::Small<double, 2>, 2> b = ra::explode<ra::Small<double, 2>>(a);
#pragma GCC diagnostic pop
    // assert(a(0, 0)==b(0)(0)); // (1)
    println(cout, "b {:c:l}", b);
    // assert(1==b(0)(0) && 2==b(0)(1) && 4==b(1)(0) && 5==b(1)(1));
}
