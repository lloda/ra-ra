// -*- mode: c++; coding: utf-8 -*-
// ra/test - View with custom dimv and generalized iter(Slice).

// (c) Daniel Llorens - 2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl;

int main()
{
    ra::TestRecorder tr(std::cout);
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto v = ra::viewptr(x);
        static_assert(1==v.step(0));
        tr.test_eq(6, v.len(0));
        tr.test_eq(ra::iota(6, 1), v);
        auto i = iter(v);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto const v = ra::viewptr(x);
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto v = ra::viewptr(x);
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        ra::Small<int, 6> v = { 1, 2, 3, 4, 5, 6 };
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
        static_assert(6==i.len(0));
    }
    {
        ra::Big<int, 1> c = { 1, 2, 3, 4, 5, 6 };
        static_assert(1==c.step(0));
        static_assert(std::is_pointer_v<decltype(c.begin())>);
        auto v = c.view();
        static_assert(std::is_pointer_v<decltype(v.begin())>);
        auto i = iter(c, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
        tr.test(&(c.dimv)==&(i.dimv));
    }
    {
        ra::Big<int> c = { 1, 2, 3, 4, 5, 6 };
        static_assert(std::is_pointer_v<decltype(c.begin())>);
        auto v = c.view();
        static_assert(!std::is_pointer_v<decltype(v.begin())>);
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        tr.test(&(v.dimv)==&(i.dimv));
    }
    return tr.summary();
}
