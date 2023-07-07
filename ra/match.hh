// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Prefix matching of array expression templates.

// (c) Daniel Llorens - 2011-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "atom.hh"

namespace ra {

constexpr rank_t
choose_rank(rank_t ra, rank_t rb)
{
    return RANK_BAD==rb ? ra : RANK_BAD==ra ? rb : RANK_ANY==ra ? ra : RANK_ANY==rb ? rb : (ra>=rb ? ra : rb);
}

// TODO Allow infinite rank; need a special value of crank.
constexpr rank_t
dependent_cell_rank(rank_t rank, rank_t crank)
{
    return crank>=0 ? crank // not dependent
        : rank==RANK_ANY ? RANK_ANY // defer
        : (rank+crank);
}

constexpr rank_t
dependent_frame_rank(rank_t rank, rank_t crank)
{
    return rank==RANK_ANY ? RANK_ANY // defer
        : crank>=0 ? (rank-crank) // not dependent
        : -crank;
}

constexpr dim_t
choose_len(dim_t sa, dim_t sb)
{
    return DIM_BAD==sa ? sb : DIM_BAD==sb ? sa : DIM_ANY==sa ? sb : sa;
}

// ct mismatch, abort (FIXME) | ct match, return 0 | rt check needed, return 1

template <class E>
constexpr int
check_expr_s()
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
                                    return fi(fi, int_c<i+1> {}, int_c<choose_len(sk, si)> {},
                                              int_c<(1==vali || sk==DIM_ANY || si==DIM_ANY) ? 1 : 0> {});
                                } else {
                                    return fi(fi, int_c<i+1> {}, int_c<sk> {}, vali);
                                }
                            } else {
                                return vali;
                            }
                        };
                    constexpr int vali = fi(fi, int_c<0> {}, int_c<DIM_BAD> {}, valk);
                    return fk(fk, int_c<k+1> {}, int_c<vali> {});
                } else {
                    return valk;
                }
            };
        return fk(fk, int_c<0> {}, int_c<0> {});
    } else {
        return 1;
    }
}

template <bool fail, class E>
constexpr bool
check_expr(E const & e)
{
    auto fi = [&e](auto && fi, int k, auto i_, int sk)
    {
        constexpr int i = i_;
        if constexpr (i<mp::len<typename E::T>) {
            if (k<std::get<i>(e.t).rank()) {
                dim_t si = std::get<i>(e.t).len(k);
                if (sk==DIM_BAD || si==DIM_BAD || si==sk) {
                    sk = choose_len(sk, si);
                } else {
                    RA_CHECK(!fail, " k ", k, " sk ", sk, " != ", si, ": mismatched dimensions");
                    return false;
                }
            }
            return fi(fi, k, int_c<i+1> {}, sk);
        } else {
            return true;
        }
    };
    rank_t rs = e.rank();
    for (int k=0; k!=rs; ++k) {
        if (!(fi(fi, k, int_c<0> {}, DIM_BAD))) {
            return false;
        }
    }
    return true;
}

template <bool check, class T, class K=mp::iota<mp::len<T>>> struct Match;

template <bool check, IteratorConcept ... P, int ... I>
struct Match<check, std::tuple<P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P ...>;
    T t;

// TODO Maybe on ply, would avoid the check flag and the agree_xxx() mess.
    constexpr
    Match(P ... p_): t(std::forward<P>(p_) ...)
    {
        if constexpr (check) {
            if constexpr (!(has_len<P> || ...)) { // len will be replaced, so check then
                if constexpr (check_expr_s<Match>()) { // may fail
                    RA_CHECK(check_expr<true>(*this));
                }
            }
        }
    }

    template <class T> struct box { using type = T; };

// rank of largest subexpr, so we look at all of them.
    constexpr static rank_t
    rank_s()
    {
        rank_t r = RANK_BAD;
        return ((r = choose_rank(r, ra::rank_s<P>())), ...);
    }

    constexpr static rank_t
    rank()
    requires (DIM_ANY != Match::rank_s())
    {
        return rank_s();
    }

    constexpr rank_t
    rank() const
    requires (DIM_ANY == Match::rank_s())
    {
        rank_t r = RANK_BAD; ((r = choose_rank(r, std::get<I>(t).rank())), ...);
        assert(RANK_ANY!=r); // not at runtime
        return r;
    }

// first positive size, if none first DIM_ANY, if none then DIM_BAD
    constexpr static dim_t
    len_s(int k)
    {
        auto f = [&k]<class A>(dim_t s)
        {
            constexpr rank_t ar = A::rank_s();
            return (ar>=0 && k>=ar) ? s : choose_len(s, A::len_s(k));
        };
        dim_t s = DIM_BAD; ((s>=0 ? s : s = f.template operator()<std::decay_t<P>>(s)), ...);
        return s;
    }

    constexpr static dim_t
    len(int k)
    requires (requires (int kk) { P::len(kk); } && ...)
    {
        return len_s(k);
    }

    constexpr dim_t
    len(int k) const
    requires (!(requires (int kk) { P::len(kk); } && ...))
    {
        auto f = [&k](dim_t s, auto const & a)
        {
            return k>=a.rank() ? s : choose_len(s, a.len(k));
        };
        dim_t s = DIM_BAD; ((s>=0 ? s : s = f(s, std::get<I>(t))), ...);
        assert(DIM_ANY!=s); // not at runtime
        return s;
    }

    constexpr void
    adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }

    constexpr auto
    step(int i) const
    {
        return std::make_tuple(std::get<I>(t).step(i) ...);
    }

    constexpr bool
    keep_step(dim_t st, int z, int j) const
    requires (!(requires (dim_t d, rank_t i, rank_t j) { P::keep_step(d, i, j); } && ...))
    {
        return (std::get<I>(t).keep_step(st, z, j) && ...);
    }
    constexpr static bool
    keep_step(dim_t st, int z, int j)
    requires (requires (dim_t d, rank_t i, rank_t j) { P::keep_step(d, i, j); } && ...)
    {
        return (std::decay_t<P>::keep_step(st, z, j) && ...);
    }
};

} // namespace ra
