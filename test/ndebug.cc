// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Isolate breakage with -DNDEBUG.

// (c) Daniel Llorens - 2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

int main(int argc, char * * argv)
{
    ra::TestRecorder tr;
    tr.section("-DNDEBUG breaks at ply_ravel etc. order[rank] vlas when traversing dynamic rank objects [ra40]");
    {
        int ap[6] = {0, 1, 2, 3, 4, 5};
        ra::View<int> a({6}, ap);
        tr.test_eq(ra::ptr(ap), a);
    }
    return tr.summary();
}
