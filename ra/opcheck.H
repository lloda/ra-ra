// -*- mode: c++; coding: utf-8 -*-
/// @file opcheck.H
/// @brief Check array argument agreement and select driving argument.

// (c) Daniel Llorens - 2011-2013, 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/bootstrap.H"

namespace ra {

// Pick arg of largest rank as driver. Var rank counts as Inf, because it might
// be larger. (This might be wrong; proper pick should be done at run time.) For
// equal rank, prefer static size, because sizes must match. When I can pick
// either, pick always the first.
// TODO Should check dimensions down to smallest rank, as old opcheck() (??)

// |-------------+----------+----------+--------------------|
// | ranks B \ A | RANK_BAD | RANK_ANY | fixed              |
// |-------------+----------+----------+--------------------|
// | RANK_BAD    | A        | A        | A                  |
// |-------------+----------+----------+--------------------|
// | RANK_ANY    | B        | A        | B                  |
// |-------------+----------+----------+--------------------|
// | fixed       | B        | A        | A>B or check sizes |
// |-------------+----------+----------+--------------------|

// |-------------+---------+---------+-------|
// | sizes B \ A | DIM_BAD | DIM_ANY | fixed |
// |-------------+---------+---------+-------|
// | DIM_BAD     | A       | A       | A     |
// |-------------+---------+---------+-------|
// | DIM_ANY     | B       | A       | A     |
// |-------------+---------+---------+-------|
// | fixed       | B       | B       | A>=B  |
// |-------------+---------+---------+-------|

constexpr bool gt_rank(rank_t ra, rank_t rb)
{
    return rb==RANK_BAD
             ? 1
             : rb==RANK_ANY
               ? ra==RANK_ANY
               : ra==RANK_BAD
                   ? 0
                   : ra==RANK_ANY
                     ? 1
                     : ra>=rb;
}

constexpr bool gt_size(dim_t sa, dim_t sb)
{
    return sb==DIM_BAD
             ? 1
             : sa==DIM_BAD
               ? 0
               : sb==DIM_ANY
                 ? 1
                 : (sa!=DIM_ANY && sa>=sb);
}

template <class A, class B>
struct pick_driver
{
    constexpr static int ra = A::rank_s();
    constexpr static int rb = B::rank_s();

    constexpr static bool value_ =
// check by rank
        rb==RANK_BAD
          ? 1
          : rb==RANK_ANY
            ? ra==RANK_ANY
            : ra==RANK_BAD
                ? 0
                : ra==RANK_ANY
                  ? 1
                  : ra>rb
                    ? 1
                    : ra<rb
                      ? 0
// check by size
                      : gt_size(A::size_s(), B::size_s());

    constexpr static int value = value_ ? 0 : 1; // 0 if A wins over B, else 1
};

template <class A, class B>
struct max_i
{
    constexpr static int value = gt_rank(A::value, B::value) ? 0 : 1; // 0 if ra wins, else 1
};

template <class ... P> using driver_index = mp::IndexOf<pick_driver, std::tuple<P ...>>;
template <class ... P> using largest_i = mp::IndexOf<max_i, std::tuple<P ...>>;
template <class T> using largest_i_tuple = mp::IndexOf<max_i, T>;

// TODO Allow infinite rank; need a special value of crank for that.
inline constexpr rank_t dependent_cell_rank(rank_t rank, rank_t crank)
{
    return crank>=0 ? crank // not dependent
        : rank==RANK_ANY ? RANK_ANY // defer
        : (rank+crank);
}

inline constexpr rank_t dependent_frame_rank(rank_t rank, rank_t crank)
{
    return rank==RANK_ANY ? RANK_ANY // defer
        : crank>=0 ? (rank-crank) // not dependent
        : -crank;
}

} // namespace ra
