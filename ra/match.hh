// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Prefix matching of array expression templates.

// (c) Daniel Llorens - 2011-2013, 2015-2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "bootstrap.hh"
#include "check.hh"

namespace ra {

inline constexpr
bool gt_rank(rank_t ra, rank_t rb)
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

inline constexpr
bool gt_len(dim_t sa, dim_t sb)
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
inline constexpr
rank_t dependent_cell_rank(rank_t rank, rank_t crank)
{
    return crank>=0 ? crank // not dependent
        : rank==RANK_ANY ? RANK_ANY // defer
        : (rank+crank);
}

inline constexpr
rank_t dependent_frame_rank(rank_t rank, rank_t crank)
{
    return rank==RANK_ANY ? RANK_ANY // defer
        : crank>=0 ? (rank-crank) // not dependent
        : -crank;
}

inline constexpr
dim_t choose_len(dim_t sa, dim_t sb)
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

// ct mismatch, abort (FIXME) | ct match, return 0 | rt check needed, return 1

template <class E>
inline constexpr
int check_expr_s()
{
    using T = typename E::T;
    constexpr rank_t rs = E::rank_s();
    if constexpr (rs>=0) {
        constexpr auto fk =
            [](auto && fk, auto k_, auto valk) consteval
            {
// FIXME until something like P1045R1 [](..., constexpr auto k_, ...)
                constexpr int k = k_;
                if constexpr (k<rs) {
                    constexpr auto fi =
                        [](auto && fi, auto i_, auto sk_, auto vali) consteval
                        {
                            constexpr dim_t sk = sk_;
                            constexpr int i = i_;
                            if constexpr (i<mp::len<T>) {
                                using Ti = std::decay_t<mp::ref<T, i>>;
                                if constexpr (k<Ti::rank_s()) {
                                    constexpr dim_t si = Ti::len_s(k);
                                    static_assert(sk<0 || si<0 || si==sk, "mismatched static dimensions");
                                    return fi(fi, mp::int_t<i+1> {}, mp::int_t<choose_len(sk, si)> {},
                                              mp::int_t<(1==vali || sk==DIM_ANY || si==DIM_ANY) ? 1 : 0> {});
                                } else {
                                    return fi(fi, mp::int_t<i+1> {}, mp::int_t<sk> {}, vali);
                                }
                            } else {
                                return vali;
                            }
                        };
                    constexpr int vali = fi(fi, mp::int_t<0> {}, mp::int_t<DIM_BAD> {}, valk);
                    return fk(fk, mp::int_t<k+1> {}, mp::int_t<vali> {});
                } else {
                    return valk;
                }
            };
        return fk(fk, mp::int_t<0> {}, mp::int_t<0> {});
    } else {
        return 1;
    }
}

template <bool fail, class E>
inline constexpr bool
check_expr(E const & e)
{
    auto fi = [&e](auto && fi, int k, auto i_, int sk)
    {
        constexpr int i = i_;
        if constexpr (i<mp::len<typename E::T>) {
            if (k<std::get<i>(e.t).rank()) {
                dim_t si = std::get<i>(e.t).len(k);
                if (sk==DIM_BAD || si==DIM_BAD || si==sk) {
                    return fi(fi, k, mp::int_t<i+1> {}, choose_len(sk, si));
                } else {
                    if (fail) {
                        RA_CHECK(false, " k ", k, " sk ", sk, " != ", si, ": mismatched dimensions");
                    } else {
                        return false;
                    }
                }
            } else {
                return fi(fi, k, mp::int_t<i+1> {}, sk);
            }
        } else {
            return true;
        }
    };
    rank_t rs = e.rank();
    for (int k=0; k!=rs; ++k) {
        if (!(fi(fi, k, mp::int_t<0> {}, DIM_BAD))) {
            return false;
        }
    }
    return true;
}

template <bool check, class T, class K=mp::iota<mp::len<T>>> struct Match;

template <bool check, class ... P, int ... I>
struct Match<check, std::tuple<P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P ...>;
    T t;

// TODO Maybe on ply? That would enable ra::check(i+j) instead of ra::check(+, i, j), avoid the check flag, and avoid the agree_xxx() mess.
    constexpr Match(P ... p_): t(std::forward<P>(p_) ...)
    {
        if constexpr (check && check_expr_s<Match>()) {
            RA_CHECK(check_expr<true>(*this));
        }
    }

    template <class T> struct box { using type = T; };

// rank of largest subexpr. This is true for either prefix or suffix match.
    constexpr static rank_t rank_s()
    {
        return mp::fold_tuple(RANK_BAD, mp::map<box, T> {},
                              [](rank_t r, auto a)
                              {
                                  constexpr rank_t ar = ra::rank_s<typename decltype(a)::type>();
                                  return gt_rank(r, ar) ?  r : ar;
                              });
    }
    constexpr rank_t rank() const
    {
        if constexpr (constexpr rank_t rs=rank_s(); rs==RANK_ANY) {
            return mp::fold_tuple(RANK_BAD, t,
                                  [](rank_t r, auto && a)
                                  {
                                      rank_t ar = a.rank();
                                      assert(ar!=RANK_ANY); // cannot happen at runtime
                                      return gt_rank(r, ar) ?  r : ar;
                                  });
        } else {
            return rs;
        }
    }

// any size which is not DIM_BAD.
    constexpr static dim_t len_s(int k)
    {
        dim_t s = mp::fold_tuple(DIM_BAD, mp::map<box, T> {},
                                 [&k](dim_t s, auto a)
                                 {
                                     using A = std::decay_t<typename decltype(a)::type>;
                                     constexpr rank_t ar = A::rank_s();
                                     if (s!=DIM_BAD) {
                                         return s;
                                     } else if (ar>=0 && k>=ar) {
                                         return s;
                                     } else {
                                         dim_t zz = A::len_s(k);
                                         return zz;
                                     }
                                 });
        return s;
    }

// do early exit with fold_tuple (and with len_s(k)).
    constexpr dim_t len(int k) const
    {
        if (dim_t ss=len_s(k); ss==DIM_ANY) {
            auto f = [this, &k](auto && f, auto i_)
                     {
                         constexpr int i = i_;
                         if constexpr (i<std::tuple_size_v<T>) {
                             auto const & a = std::get<i>(this->t);
                             if (k<a.rank()) {
                                 dim_t as = a.len(k);
                                 if (as!=DIM_BAD) {
                                     assert(as!=DIM_ANY); // cannot happen at runtime
                                     return as;
                                 } else {
                                     return f(f, mp::int_t<i+1> {});
                                 }
                             } else {
                                 return f(f, mp::int_t<i+1> {});
                             }
                         } else {
                             assert(0);
                             return DIM_BAD;
                         }
                     };
            return f(f, mp::int_t<0> {});
        } else {
            return ss;
        }
    }

    constexpr void adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }

    constexpr auto step(int i) const
    {
        return std::make_tuple(std::get<I>(t).step(i) ...);
    }

    constexpr bool keep_step(dim_t st, int z, int j) const
        requires (!(requires (dim_t d, rank_t i, rank_t j) { P::keep_step(d, i, j); } && ...))
    {
        return (std::get<I>(t).keep_step(st, z, j) && ...);
    }
    constexpr static bool keep_step(dim_t st, int z, int j)
        requires (requires (dim_t d, rank_t i, rank_t j) { P::keep_step(d, i, j); } && ...)
    {
        return (std::decay_t<P>::keep_step(st, z, j) && ...);
    }
};

} // namespace ra
