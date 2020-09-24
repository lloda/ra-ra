// -*- mode: c++; coding: utf-8 -*-
/// @file wrank.hh
/// @brief Rank conjunction for expression templates.

// (c) Daniel Llorens - 2013-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/match.hh"

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

// Dest is a list of destination axes [l0 l1 ... li ... l(rank(A)-1)].
// The dimensions of the reframed A are numbered as [0 ... k ... max(l)-1].
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// If not, then axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO invalid for RANK_ANY (since Dest is compile time). [ra07]

template <class Dest, class A>
struct Reframe
{
    A a;

    constexpr static int orig(int k) { return mp::int_list_index<Dest>(k); }
    constexpr static rank_t rank_s() { return 1+mp::fold<mp::max, mp::int_t<-1>, Dest>::value; }
    constexpr static rank_t rank() { return rank_s(); }
    constexpr static dim_t size_s(int k)
    {
        int l = orig(k);
        return l>=0 ? std::decay_t<A>::size_s(l) : DIM_BAD;
    }
    constexpr dim_t size(int k) const
    {
        int l = orig(k);
        return l>=0 ? a.size(l) : DIM_BAD;
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        if (int l = orig(k); l>=0) {
            a.adv(l, d);
        }
    }
    constexpr auto stride(int k) const
    {
        int l = orig(k);
        return l>=0 ? a.stride(l) : zerostride<decltype(a.stride(l))>::f();
    }
    constexpr bool keep_stride(dim_t st, int z, int j) const
    {
        int wz = orig(z);
        int wj = orig(j);
        return wz>=0 && wj>=0 && a.keep_stride(st, wz, wj);
    }
    template <class I> constexpr decltype(auto) at(I const & i)
    {
        return a.at(mp::map_indices<std::array<dim_t, mp::len<Dest>>, Dest>(i));
    }
    constexpr decltype(auto) flat() { return a.flat(); }
};

// Optimize no-op case.
// TODO If A is cell_iterator, etc. beat Dest directly on that... same for an eventual transpose_expr<>.

template <class Dest, class A> decltype(auto)
reframe(A && a)
{
    if constexpr (std::is_same_v<Dest, mp::iota<1+mp::fold<mp::max, mp::int_t<-1>, Dest>::value>>) {
        return std::forward<A>(a);
    } else {
        return Reframe<Dest, A> { std::forward<A>(a) };
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

template <class cranks, class Op> inline constexpr auto
wrank(cranks cranks_, Op && op)
{
    return Verb<cranks, Op> { std::forward<Op>(op) };
}

template <rank_t ... crank, class Op> inline constexpr auto
wrank(Op && op)
{
    return Verb<mp::int_list<crank ...>, Op> { std::forward<Op>(op) };
}

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

template <class A, class B>
struct max_i
{
    constexpr static int value = gt_rank(A::value, B::value) ? 0 : 1; // 0 if ra wins, else 1
};

// Get a list (per argument) of lists of live axes. The last frame match is handled by standard prefix matching.

template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "bad args");
// live = number of live axes on this frame, for each argument. // TODO crank negative, inf.
    using live = mp::int_list<(rank_s<Ti>() - mp::len<Ri> - crank::value) ...>;
    using frameaxes = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri> - crank::value), skip>> ...>;
    using FM = Framematch<W, std::tuple<Ti ...>, frameaxes, skip + mp::ref<live, mp::indexof<max_i, live>>::value>;
    using R = typename FM::R;

    template <class VV> static decltype(auto) op(VV && v) { return FM::op(std::forward<VV>(v).op); } // cf [ra31]
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(Ri), "bad args");
// TODO -crank::value when the actual verb rank is used (e.g. to use cell_iterator<A, that_rank> instead of just begin()).
    using R = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri>), skip>> ...>;

    template <class VV> static decltype(auto) op(VV && v) { return std::forward<VV>(v); }
};

} // namespace ra
