// -*- mode: c++; coding: utf-8 -*-
/// @file newaxis.C
/// @brief Stepwise checks of future ra::newaxis (WIP)

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/format.H"
#include "ra/test.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout, std::endl, std::flush;

namespace ra {

template <int n> struct newaxos_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};

template <int n=1> constexpr newaxos_t<n> newaxos = newaxos_t<n>();

template <int n> struct is_beatable_def<newaxos_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = 0;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <int n, class ... I>
inline dim_t select_loop(Dim * dim, Dim const * dim_src, newaxos_t<n> newaxos, I && ... i)
{
    for (Dim * end = dim+n; dim!=end; ++dim) {
        dim->size = DIM_BAD;
        dim->stride = 0;
    }
    return select_loop(dim, dim_src, std::forward<I>(i) ...);
}

} // namespace ra


int main()
{
    TestRecorder tr(std::cout);
    tr.section("view");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 2, 3, 4}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        auto c = a(ra::newaxos<1>);
// size is DIM_BAD as expected.
        cout << shape(c) << endl;
// first argument drives, which works here.
        cout << decltype(b+c)::A << endl;
        cout << (b+c) << endl;
// size is wrong because Expr selects driver at compile time - this is not enough for newaxos matching.
// FIXME shouldn't ply or print DIM_BAD sized expr.
        for_each([](auto && x) { cout << ". " << x; }, (c+b)); cout << endl;
        cout << (c+b) << endl;
// FIXME shouldn't ply or print DIM_BAD sized View.
        for_each([](auto && x) { cout << ". " << x; }, c); cout << endl;
        cout << c << endl;
    }
    return tr.summary();
}
