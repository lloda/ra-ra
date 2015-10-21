
// (c) Daniel Llorens - 2005-2012

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_INT_T_H
#define RA_INT_T_H

/// @file int_t.H
/// @brief Treat integers as types, and global functions for integers.

#include "ra/macros.H"

namespace mp {

// cf http://stackoverflow.com/questions/14395967/proper-initialization-of-static-constexpr-array-in-class-template
template <int V> struct int_t
{
    constexpr static int value = V;
};
template <int V> int constexpr int_t<V>::value;

long constexpr pown(int n, int p)
{
    return p==0 ? 1 : n*pown(n, p-1);
}

long constexpr pow2(int n)
{
    return pown(2, n);
}

long constexpr fact(int i)
{
    return (i<2) ? 1 : i*fact(i-1);
}

template <int N, int P>
struct n_over_p
{
    static_assert(N>=P, "bad args to n_over_p");
    static long constexpr value = fact(N)/fact(P)/fact(N-P);
};

// @TODO See what we can replace by signbit, copysign, or sgn x === ((x>0)-(x<0)).

} // namespace mp

// @TODO All users be int, then this take int.
inline constexpr bool odd(unsigned int N) { return N & 1; }
inline constexpr bool any(bool const x) { return x; }
inline constexpr bool every(bool const x) { return x; }

#endif // RA_INT_T_H
