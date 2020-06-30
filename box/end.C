// -*- mode: c++; coding: utf-8 -*-
/// @file end.C
/// @brief Check use of special object ra::end

// (c) Daniel Llorens - 2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/ra.H"
#include "ra/test.H"

using std::cout, std::endl;

namespace ra {

struct End
{
    using value_type = dim_t;

    constexpr static rank_t rank_s() { return RANK_BAD; }
    constexpr static rank_t rank() { return RANK_BAD; }
    constexpr static dim_t size_s() { return DIM_BAD; }

    constexpr void adv(rank_t const k, dim_t const d) {}
    template <class I> constexpr value_type at(I const & i) const { assert(0); return 0; }

    constexpr static dim_t size(int const k) { return DIM_BAD; }
    constexpr dim_t stride(int const i) const { assert(0); }
    constexpr value_type * flat() const { assert(0); }
};

constexpr End end {};

template <class E>
constexpr decltype(auto) replace_end(dim_t end, E && e)
{
    return std::forward<E>(e);
}

constexpr auto replace_end(dim_t end, End && e)
{
    return scalar(end);
}

template <class Op, class P ...>
constexpr auto replace_end(dim_t end, Expr<Op, std::tuple<P ...>> && e)
{
    return expr(std::forward<decltype(e.op)>(e.op),
                replace_end(end, std::forward<P>(std::get<I>(e.t))) ...);
}

} // ra


int main()
{
    // End is usable in exprs by virtue of having flat() (TODO will fix this).

    ra::Small<int, 3> a {1, 2, 3};
    cout << replace_end(10, a + ra::end) << endl;

    return 0;
}
