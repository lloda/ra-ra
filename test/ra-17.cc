// -*- mode: c++; coding: utf-8 -*-
// ra/test - View with custom dimv and generalized iter(Slice)

// (c) Daniel Llorens - 2024-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl;

namespace ra {

template <class P, class Dimv, class Cr> Cell(P, Dimv &&, Cr) -> Cell<P, Dimv, Cr>;

constexpr auto
iter(Slice auto && s, auto c) requires (!is_iterator<decltype(s)>)
{
    using Dimv = std::decay_t<decltype(s)>::Dimv;
    if constexpr (is_ctype<decltype(c)>) {
        if constexpr (is_ctype<Dimv>) {
            return Cell(s.data(), Dimv {}, c);
        } else {
            return Cell(s.data(), RA_FW(s).dimv, c);
        }
    } else {
        return Cell(s.data(), RA_FW(s).dimv, rank_t(c));
    }
}

} // namespace ra

int main()
{
    ra::TestRecorder tr(std::cout);
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto v = ra::ptrview(x);
        static_assert(1==v.step(0));
        tr.test_eq(6, v.len(0));
        tr.test_eq(ra::iota(6, 1), v);
        auto i = iter(v);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto const v = ra::ptrview(x);
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        int x[6] = { 1, 2, 3, 4, 5, 6 };
        auto v = ra::ptrview(x);
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    {
        ra::Small<int, 6> v = { 1, 2, 3, 4, 5, 6 };
        auto i = iter(v, ra::ic<0>);
        tr.test_eq(ra::iota(6, 1), i);
        static_assert(1==i.step(0));
    }
    return tr.summary();
}
