// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Operation nodes for expression templates.

// (c) Daniel Llorens - 2011-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "match.hh"

namespace ra {


// ---------------------------
// reframe
// ---------------------------

// Reframe is a variant of transpose that works on any IteratorConcept.  As in transpose(), one names
// the destination axis for each original axis.  However, unlike general transpose, axes may not be
// repeated. The main application is the rank conjunction below.

template <class T> constexpr T zerostep = 0;
template <class ... T> constexpr std::tuple<T ...> zerostep<std::tuple<T ...>> = { zerostep<T> ... };

// Dest is a list of destination axes [l0 l1 ... li ... l(rank(A)-1)].
// The dimensions of the reframed A are numbered as [0 ... k ... max(l)-1].
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// If not, then axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO invalid for RANK_ANY (since Dest is compile time). [ra7]

template <class Dest, IteratorConcept A>
struct Reframe
{
    A a;

    constexpr static int orig(int k) { return mp::int_list_index<Dest>(k); }
    constexpr static rank_t rank_s() { return 1+mp::fold<mp::max, mp::int_c<-1>, Dest>::value; }
    constexpr static rank_t rank() { return rank_s(); }
    constexpr static dim_t len_s(int k)
    {
        int l = orig(k);
        return l>=0 ? std::decay_t<A>::len_s(l) : DIM_BAD;
    }
    constexpr dim_t
    len(int k) const
    {
        int l = orig(k);
        return l>=0 ? a.len(l) : DIM_BAD;
    }
    constexpr void
    adv(rank_t k, dim_t d)
    {
        if (int l = orig(k); l>=0) {
            a.adv(l, d);
        }
    }
    constexpr auto
    step(int k) const
    {
        int l = orig(k);
        return l>=0 ? a.step(l) : zerostep<decltype(a.step(l))>;
    }
    constexpr bool
    keep_step(dim_t st, int z, int j) const
    {
        int wz = orig(z);
        int wj = orig(j);
        return wz>=0 && wj>=0 && a.keep_step(st, wz, wj);
    }
    constexpr decltype(auto)
    flat()
    {
        return a.flat();
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        return a.at(mp::map_indices<std::array<dim_t, mp::len<Dest>>, Dest>(i));
    }
};

// Optimize no-op case.
// TODO If A is CellBig, etc. beat Dest directly on it, same for eventual transpose_expr<>.

template <class Dest, class A>
constexpr decltype(auto)
reframe(A && a)
{
    if constexpr (std::is_same_v<Dest, mp::iota<1+mp::fold<mp::max, mp::int_c<-1>, Dest>::value>>) {
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

template <class cranks, class Op>
constexpr auto
wrank(cranks cranks_, Op && op)
{
    return Verb<cranks, Op> { std::forward<Op>(op) };
}

template <rank_t ... crank, class Op>
constexpr auto
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
    constexpr static int value = gt_rank(A::value, B::value) ? 0 : 1;
};

// Get a list (per argument) of lists of live axes. The last frame match is handled by standard prefix matching.

template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
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
    static_assert(sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
// TODO -crank::value when the actual verb rank is used (eg to use CellBig<A, that_rank> instead of just begin()).
    using R = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri>), skip>> ...>;
    template <class VV> static decltype(auto) op(VV && v) { return std::forward<VV>(v); }
};


// ---------------------------
// general expression
// ---------------------------

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Expr;

template <class Op, IteratorConcept ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    template <class T>
    struct Flat
    {
        Op & op;
        T t;
        template <class S> constexpr void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
// FIXME gcc 12.1 flags this (-O3 only).
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        constexpr decltype(auto) operator*() { return std::invoke(op, *std::get<I>(t) ...); }
#pragma GCC diagnostic pop
    };

    template <class ... F>
    constexpr static auto
    flat(Op & op, F && ... f)
    {
        return Flat<std::tuple<F ...>> { op, { std::forward<F>(f) ... } };
    }

    using Match_ = Match<true, std::tuple<P ...>>;
    using Match_::t, Match_::rank_s, Match_::rank;
    Op op;

// test/ra-9.cc [ra1]
    constexpr Expr(Op op_, P ... p_): Match_(std::forward<P>(p_) ...), op(std::forward<Op>(op_)) {}
    RA_DEF_ASSIGNOPS_SELF(Expr)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr decltype(auto)
    at(auto const & j) const
    {
        return std::invoke(op, std::get<I>(t).at(j) ...);
    }
    constexpr decltype(auto)
    flat() // FIXME can't be const bc of Flat::op. Carries over to Pick / Reframe .flat() ...
    {
        return flat(op, std::get<I>(t).flat() ...);
    }
// needed for rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(flat(op, std::get<I>(t).flat() ...))) ()
    {
// for coord types; so ct only
        if constexpr ((rank_s()!=1 || size_s<Expr>()!=1) && rank_s()!=0) {
            static_assert(rank_s()==RANK_ANY);
            assert(rank()==0);
        }
        return *flat();
    }
};

template <class V, class ... T, int ... i>
constexpr auto
expr_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return expr(FM::op(std::forward<V>(v)), reframe<mp::ref<typename FM::R, i>>(std::forward<T>(t)) ...);
}

template <class Op, class ... P>
constexpr auto
expr(Op && op, P && ... p)
{
    if constexpr (is_verb<Op>) {
        return expr_verb(mp::iota<sizeof...(P)> {}, std::forward<Op>(op), std::forward<P>(p) ...);
    } else {
        return Expr<Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(p) ... };
    }
}

template <class Op, class ... A>
constexpr auto
map(Op && op, A && ... a)
{
    return expr(std::forward<Op>(op), start(std::forward<A>(a)) ...);
}


// ---------------
// explicit agreement checks. FIXME provide separate agree_s().
// ---------------

template <class ... P>
constexpr bool
agree(P && ... p)
{
    return agree_(ra::start(std::forward<P>(p)) ...);
}

template <class Op, class ... P>
constexpr bool
agree_op(Op && op, P && ... p)
{
    return agree_op_(std::forward<Op>(op), ra::start(std::forward<P>(p)) ...);
}

template <class ... P>
constexpr bool
agree_(P && ... p)
{
    return check_expr<false>(Match<false, std::tuple<P ...>> { std::forward<P>(p) ... });
}

template <class Op, class ... P>
constexpr bool
agree_op_(Op && op, P && ... p)
{
    if constexpr (is_verb<Op>) {
        return agree_verb(mp::iota<sizeof...(P)> {}, std::forward<Op>(op), std::forward<P>(p) ...);
    } else {
        return agree_(std::forward<P>(p) ...);
    }
}

template <class V, class ... T, int ... i>
constexpr bool
agree_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return agree_op_(FM::op(std::forward<V>(v)), reframe<mp::ref<typename FM::R, i>>(std::forward<T>(t)) ...);
}

} // namespace ra
