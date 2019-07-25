// -*- mode: c++; coding: utf-8 -*-
/// @file wrank.H
/// @brief Rank conjunction for expression templates.

// (c) Daniel Llorens - 2013-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/expr.H"

// TODO Adopt frame matching as in Match (no driver).

namespace ra {


// ---------------------------
// reframe - a variant of transpose that works on any array iterator.
// As in transpose, one names the destination axis for each original axis.
// However, unlike general transpose, axes may not be repeated.
// The main application is the rank conjunction below.
// ---------------------------

template <class T>
struct zerostride
{
    constexpr static T f() { return T(0); }
};

template <class ... T>
struct zerostride<std::tuple<T ...>>
{
    constexpr static std::tuple<T ...> f() { return std::make_tuple(zerostride<T>::f() ...); }
};

// Live is a list of 'live' destination axes [l0 l1 ... li ... l(rank(A)-1)].
// The dimensions of the reframed A are numbered as [0 ... k ... max(l)-1].
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// If not, then axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO invalid for RANK_ANY (since Live is compile tile). [ra07]

template <class Live, class A>
struct Reframe
{
    A a;

    constexpr static int live(int k) { return mp::int_list_index<Live>(k); }
    constexpr static rank_t rank_s() { return 1+mp::fold<mp::max, mp::int_t<-1>, Live>::value; }
    constexpr static rank_t rank() { return rank_s(); }
    constexpr static dim_t size_s(int k)
    {
        int l = live(k);
        return l>=0 ? std::decay_t<A>::size_s(l) : DIM_BAD;
    }
    constexpr dim_t size(int k) const
    {
        int l = live(k);
        return l>=0 ? a.size(l) : DIM_BAD;
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        if (int l = live(k); l>=0) {
            a.adv(l, d);
        }
    }
    constexpr auto stride(int k) const
    {
        int l = live(k);
        return l>=0 ? a.stride(l) : zerostride<decltype(a.stride(l))>::f();
    }
    constexpr bool keep_stride(dim_t step, int z, int j) const
    {
        int wz = live(z);
        int wj = live(j);
        return wz>=0 && wj>=0 && a.keep_stride(step, wz, wj);
    }
    template <class I> constexpr decltype(auto) at(I const & i)
    {
        return a.at(mp::map_indices<std::array<dim_t, mp::len<Live>>, Live>(i));
    }
    constexpr decltype(auto) flat() { return a.flat(); }
};

// Optimize the no-op case.
// TODO If A is cell_iterator, etc. beat Live directly on that... same for an eventual transpose_expr<>.

template <class Live, class A> decltype(auto)
reframe(A && a)
{
    if constexpr (std::is_same_v<Live, mp::iota<1+mp::fold<mp::max, mp::int_t<-1>, Live>::value>>) {
        return std::forward<A>(a);
    } else {
        return Reframe<Live, A> { std::forward<A>(a) };
    }
}


// ---------------------------
// verbs and rank conjunction
// ---------------------------

template <class cranks_, class Op_>
struct Verb
{
    using cranks = cranks_;
    using Op = Op_;
    Op op;
};

RA_IS_DEF(is_verb, (std::is_same_v<A, Verb<typename A::cranks, typename A::Op>>))

template <class cranks, class Op> inline constexpr
auto wrank(cranks cranks_, Op && op)
{
    return Verb<cranks, Op> { std::forward<Op>(op) };
}

template <rank_t ... crank, class Op> inline constexpr
auto wrank(Op && op)
{
    return Verb<mp::int_list<crank ...>, Op> { std::forward<Op>(op) };
}

template <class R, class skip, class frank>
using AddFrameAxes = mp::append<R, mp::iota<frank::value, skip::value>>;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

template <class A, class B>
struct max_i
{
    constexpr static int value = gt_rank(A::value, B::value) ? 0 : 1; // 0 if ra wins, else 1
};

// Get a list (per argument) of lists of live axes.
// The last frame match is handled by standard prefix matching.

template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "bad args");

    using T = std::tuple<Ti ...>;
    using R_ = std::tuple<Ri ...>;

// live = number of live axes on this frame, for each argument.
// TODO functions of arg rank, negative, inf.
    static_assert((((rank_s<Ti>() - mp::len<Ri> - crank::value)>=0) && ...), "bad ranks");
    using live = mp::int_list<(rank_s<Ti>() - mp::len<Ri> - crank::value) ...>;
    constexpr static int driver = mp::IndexOf<max_i, live>::value;

// add actual axes to result.
    using skips = mp::makelist<sizeof...(Ti), mp::int_t<skip>>;
    using FM = Framematch<W, T, mp::map<AddFrameAxes, R_, skips, live>, skip + mp::ref<live, driver>::value>;
    using R = typename FM::R;

    template <class VV> static decltype(auto) op(VV && v) { return FM::op(std::forward<VV>(v).op); } // cf [ra31]
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    using R_ = std::tuple<Ri ...>;

// TODO -crank::value when the actual verb rank is used (e.g. to use cell_iterator<A, that_rank> instead of just begin()).
    static_assert((((rank_s<Ti>() - mp::len<Ri>)>=0) && ...), "bad ranks");
    using live = mp::int_list<(rank_s<Ti>() - mp::len<Ri>) ...>;
    constexpr static int driver = mp::IndexOf<max_i, live>::value;

    using skips = mp::makelist<sizeof...(Ti), mp::int_t<skip>>;
    using R = mp::map<AddFrameAxes, R_, skips, live>;

    template <class VV> static decltype(auto) op(VV && v) { return std::forward<VV>(v); }
};

template <class V, class ... T, int ... i> inline constexpr auto
ryn(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return expr(FM::op(std::forward<V>(v)), reframe<mp::ref<typename FM::R, i>>(std::forward<T>(t)) ...);
}

// TODO partial specialization means no universal ref :-/
#define DEF_EXPR_VERB(MOD)                                              \
    template <class cranks, class Op, class ... T> inline constexpr auto \
    expr(Verb<cranks, Op> MOD v, T && ... t)                            \
    {                                                                   \
        return ryn(mp::iota<sizeof...(T)> {}, std::forward<decltype(v)>(v), std::forward<T>(t) ...); \
    }
FOR_EACH(DEF_EXPR_VERB, &&, &, const &)
#undef DEF_EXPR_VERB

} // namespace ra
