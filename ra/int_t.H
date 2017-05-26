
// (c) Daniel Llorens - 2005-2012, 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file int_t.H
/// @brief Treat integers as types, and global functions for integers.

#pragma once
#include "ra/macros.H"

namespace mp {

template <int V> using int_t = std::integral_constant<int, V>;
template <bool V> using bool_t = std::integral_constant<bool, V>;

constexpr long pown(int n, int p)
{
    return p==0 ? 1 : n*pown(n, p-1);
}

constexpr long pow2(int n)
{
    return pown(2, n);
}

constexpr long fact(int i)
{
    return (i<2) ? 1 : i*fact(i-1);
}

// Works *almost* to the range of size_t.
constexpr size_t n_over_p(size_t const n, size_t p)
{
    if (p>n) {
        return 0;
    } else if (p>(n-p)) {
        p = n-p;
    }
    size_t v = 1;
    for (size_t i=0; i!=p; ++i) {
        v = v*(n-i)/(i+1);
    }
    return v;
}

} // namespace mp

// TODO All users be int, then this take int.
constexpr bool odd(unsigned int N) { return N & 1; }
constexpr bool any(bool const x) { return x; }
constexpr bool every(bool const x) { return x; }
