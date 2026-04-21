// -*- mode: c++; coding: utf-8 -*-
// ra/test - A bug/footgun with nested arrays

// (c) Daniel Llorens - 2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"
#include "test/mpdebug.hh"

using std::cout, std::endl;

// This happens because iterators, in general, only keep pointers, so when an expr that is built on local objects is returned, those objects are gone and the pointers are left dangling. Could be fixed in Cell = iter(Slice), if I kept the slice itself in Cell.

int main()
{
    ra::TestRecorder tr(std::cout);
    tr.section("the bug");
    {
        auto f = [](int i){ return ra::Small<double, 2>{1+i, 3+i}; };
        ra::Big<int, 1> a = {1};
        tr.section("ok");
        cout << map(f, a) << endl;
        tr.section("ok");
        cout << map([](auto const & x) { return concrete(x*2); }, map(f, a)) << endl;
        tr.section("bug");
        cout << *(map(f, a)*2) << endl; // <----- the bug skip -gfi /opt skip -gfi /usr
    }
    return tr.summary();
}
