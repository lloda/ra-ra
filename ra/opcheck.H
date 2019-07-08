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

#ifndef RA_CHECK_BOUNDS_OPCHECK
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_OPCHECK 1
  #else
    #define RA_CHECK_BOUNDS_OPCHECK RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_OPCHECK==0
  #define CHECK_BOUNDS( cond )
#else
  #define CHECK_BOUNDS( cond ) RA_ASSERT( cond, 0 )
#endif

namespace ra {

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


// ---------------------
// New Expr
// ---------------------

constexpr dim_t
chosen_size(dim_t sa, dim_t sb)
{
    if (sa==DIM_BAD) {
        return sb;
    } else if (sb==DIM_BAD) {
        return sa;
    } else if (sa==DIM_ANY) {
        return sb;
    } else {
        return sa;
    }
}

template <class E>
constexpr bool check_expr(E const & e)
{
    using T = typename E::T;
    constexpr rank_t rs = E::rank_s();
// the static checks are done regardless.
    if constexpr (rs>=0) {
        constexpr auto fk =
            [](auto && fk, auto k_)
            {
                constexpr int k = decltype(k_)::value;
                if constexpr (k<rs) {
                    constexpr auto fi =
                        [](auto && fi, auto i_, auto sk_)
                        {
                            constexpr dim_t sk = decltype(sk_)::value;
                            constexpr int i = decltype(i_)::value;
                            if constexpr (i<mp::len<T>) {
                                using Ti = std::decay_t<mp::ref<T, i>>;
                                if constexpr (k<Ti::rank_s()) {
                                    constexpr dim_t si = Ti::size_s(k);
                                    static_assert(sk<0 || si<0 || si==sk, "mismatched static dimensions");
                                    fi(fi, mp::int_t<i+1> {}, mp::int_t<chosen_size(sk, si)> {});
                                } else {
                                    fi(fi, mp::int_t<i+1> {}, mp::int_t<sk> {});
                                }
                            }
                        };
                    fi(fi, mp::int_t<0> {}, mp::int_t<DIM_BAD> {});
                    return fk(fk, mp::int_t<k+1> {});
                } else {
                    return 0;
                }
            };
        constexpr int check = fk(fk, mp::int_t<0> {});
        static_assert(check==0);
    }
    if constexpr (E::size_s()==DIM_ANY) {
        rank_t rs = e.rank();
        for (int k=0; k!=rs; ++k) {
            auto fi =
                [&k, &e](auto && fi, auto i_, int sk)
                {
                    constexpr int i = decltype(i_)::value;
                    if constexpr (i<mp::len<T>) {
                        if (k<std::get<i>(e.t).rank()) {
                            dim_t si = std::get<i>(e.t).size(k);
                            RA_ASSERT((sk==DIM_BAD || si==DIM_BAD || si==sk),
                                      " k ", k, " sk ", sk, " != ", si, ": mismatched dimensions");
                            fi(fi, mp::int_t<i+1> {}, chosen_size(sk, si));
                        } else {
                            fi(fi, mp::int_t<i+1> {}, sk);
                        }
                    }
                };
            fi(fi, mp::int_t<0> {}, DIM_BAD);
        }
    }
    return true;
}

} // namespace ra
