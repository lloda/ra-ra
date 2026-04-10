// -*- mode: c++; coding: utf-8 -*-
// ra/test - View with custom dimv and iter(general Slice).

// (c) Daniel Llorens - 2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl;

template <class P, int R>
auto ctad(ra::ViewBig<P, R> const & a)
{
    return std::make_tuple<sizeof(std::remove_pointer_t<P>), R>;
}

int main()
{
    ra::TestRecorder tr(std::cout);
#if 0 // FIXME
    tr.section("CTAD for ViewBig");
    {
        ra::Big<float, 1> a({10}, 0.);
        auto [xa, ya] = ctad(a());
        tr.test_eq(sizeof(float), xa);
        tr.test_eq(1, ya);
        ra::Big<double, 2> b({3, 3}, 0.);
        auto [xb, yb] = ctad(b());
        tr.test_eq(sizeof(double), xb);
        tr.test_eq(2, yb);
    }
#endif
    tr.section("Custom dimv and iter(general Slice)");
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto v = ra::view(x);
        static_assert(1==v.step(0));
        tr.test_eq(6, v.len(0));
        tr.test_eq(ra::iota(6, 1), v);
        auto i = iter(v);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto const v = ra::view(x);
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto v = ra::view(x);
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
