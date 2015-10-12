
// (c) Daniel Llorens - 2011-2013, 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_OPCHECK_H
#define RA_OPCHECK_H

/// @file ra-opcheck.H
/// @brief Check array argument agreement and select driving argument.

#include "ra/tuple-list.H"
#include "ra/ra-bootstrap.H"

namespace ra {

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

// Pick arg of largest rank as driver. Var rank counts as Inf. For equal rank, prefer static size. When I can pick either, pick always the first.
// @todo Should check dimensions down to smallest rank, as old opcheck() (??)

template <class A, class B>
struct pick_driver
{
    constexpr static int ra = A::rank_s();
    constexpr static int rb = B::rank_s();
    constexpr static int sa = A::size_s();
    constexpr static int sb = B::size_s();

    static_assert((ra==RANK_BAD) == (sa==DIM_BAD), "bad bad for A");
    static_assert((rb==RANK_BAD) == (sb==DIM_BAD), "bad bad for B");

    constexpr static bool value_ =
// check by rank
        sb==RANK_BAD
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
                      : sb==DIM_ANY
                        ? 1
                        : (sa!=DIM_ANY && sa>=sb);

    constexpr static int value = value_ ? 0 : 1; // 0 if ra wins, else 1
};

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

template <class A, class B>
struct max_i
{
    constexpr static int value = gt_rank(A::value, B::value) ? 0 : 1; // 0 if ra wins, else 1
};

template <class ... P> using largest_rank = mp::IndexOf<pick_driver, std::tuple<P ...>>;
template <class ... P> using largest_i = mp::IndexOf<max_i, std::tuple<P ...>>;
template <class T> using largest_i_tuple = mp::IndexOf<max_i, T>;

} // namespace ra

#endif // RA_OPCHECK_H
