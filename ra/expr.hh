// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression templates with prefix matching.

// (c) Daniel Llorens - 2011-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "atom.hh"
#include <functional>

namespace ra {


// --------------------
// prefix match
// --------------------

constexpr rank_t
choose_rank(rank_t ra, rank_t rb) { return BAD==rb ? ra : BAD==ra ? rb : ANY==ra ? ra : ANY==rb ? rb : std::max(ra, rb); }

// pick first if mismatch (see below). FIXME maybe return invalid.
constexpr dim_t
choose_len(dim_t sa, dim_t sb) { return BAD==sa ? sb : BAD==sb ? sa : ANY==sa ? sb : sa; }

template <bool checkp, class T, class K=mp::iota<mp::len<T>>> struct Match;
template <bool checkp, IteratorConcept ... P, int ... I>
struct Match<checkp, std::tuple<P ...>, mp::int_list<I ...>>
{
    std::tuple<P ...> t;
// rank of largest subexpr
    constexpr static rank_t rs = [] { rank_t r=BAD; return ((r=choose_rank(r, ra::rank_s<P>())), ...); }();

// 0: fail, 1: rt, 2: pass
    consteval static int
    check_s()
    {
        if constexpr (sizeof...(P)<2) {
            return 2;
        } else if constexpr (ANY==rs) {
            return 1; // FIXME can be tightened to 2 if all args are rank 0 save one
        } else {
            bool tbc = false;
            for (int k=0; k<rs; ++k) {
                dim_t ls = len_s(k);
                if (((k<ra::rank_s<P>() && ls!=choose_len(std::decay_t<P>::len_s(k), ls)) || ...)) {
                    return 0;
                } else {
                    int anyk = ((k<ra::rank_s<P>() && (ANY==std::decay_t<P>::len_s(k))) + ...);
                    int fixk = ((k<ra::rank_s<P>() && (0<=std::decay_t<P>::len_s(k))) + ...);
                    tbc = tbc || (anyk>0 && anyk+fixk>1);
                }
            }
            return tbc ? 1 : 2;
        }
    }
    constexpr bool
    check() const
    {
        if constexpr (sizeof...(P)<2) {
            return true;
        } else if constexpr (constexpr int c = check_s(); 0==c) {
            return false;
        } else if constexpr (1==c) {
            for (int k=0; k<rank(); ++k) {
                dim_t ls = len(k);
                if (((k<ra::rank(std::get<I>(t)) && ls!=choose_len(std::get<I>(t).len(k), ls)) || ...)) {
                    RA_CHECK(!checkp, "Shape mismatch on axis ", k, " [", (std::array { std::get<I>(t).len(k) ... }), "].");
                    return false;
                }
            }
        }
        return true;
    }

    constexpr
    Match(P ... p_): t(p_ ...) // [ra1]
    {
// TODO Maybe on ply, would make checkp unnecessary, make agree_xxx() unnecessary.
        if constexpr (checkp && !(has_len<P> || ...)) {
            static_assert(check_s(), "Shape mismatch.");
            RA_CHECK(check());
        }
    }

    consteval static rank_t
    rank() requires (ANY!=rs)
    {
        return rs;
    }
    constexpr rank_t
    rank() const requires (ANY==rs)
    {
        rank_t r = BAD;
        ((r = choose_rank(r, ra::rank(std::get<I>(t)))), ...);
        assert(ANY!=r); // not at runtime
        return r;
    }
// first nonnegative size, if none first ANY, if none then BAD
    constexpr static dim_t
    len_s(int k)
    {
        auto f = [&k]<class A>(dim_t s) {
            constexpr rank_t ar = ra::rank_s<A>();
            return (ar<0 || k<ar) ? choose_len(s, A::len_s(k)) : s;
        };
        dim_t s = BAD; ((s>=0 ? s : s = f.template operator()<std::decay_t<P>>(s)), ...);
        return s;
    }
    constexpr static dim_t
    len(int k) requires (requires (int kk) { P::len(kk); } && ...)
    {
        return len_s(k);
    }
    constexpr dim_t
    len(int k) const requires (!(requires (int kk) { P::len(kk); } && ...))
    {
        auto f = [&k](dim_t s, auto const & a) {
            return k<ra::rank(a) ? choose_len(s, a.len(k)) : s;
        };
        dim_t s = BAD; ((s>=0 ? s : s = f(s, std::get<I>(t))), ...);
        assert(ANY!=s); // not at runtime
        return s;
    }
    constexpr auto
    step(int i) const
    {
        return std::make_tuple(std::get<I>(t).step(i) ...);
    }
    constexpr void
    adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }
    constexpr bool
    keep_step(dim_t st, int z, int j) const
    requires (!(requires (dim_t st, rank_t z, rank_t j) { P::keep_step(st, z, j); } && ...))
    {
        return (std::get<I>(t).keep_step(st, z, j) && ...);
    }
    constexpr static bool
    keep_step(dim_t st, int z, int j)
    requires (requires (dim_t st, rank_t z, rank_t j) { P::keep_step(st, z, j); } && ...)
    {
        return (std::decay_t<P>::keep_step(st, z, j) && ...);
    }
    constexpr auto save() const { return std::make_tuple(std::get<I>(t).save() ...); }
    constexpr void load(auto const & pp) { ((std::get<I>(t).load(std::get<I>(pp))), ...); }
    constexpr void mov(auto const & s) { ((std::get<I>(t).mov(std::get<I>(s))), ...); }
};


// ---------------------------
// reframe
// ---------------------------

// Transpose variant for IteratorConcepts. As in transpose(), one names the destination axis for
// each original axis. However, axes may not be repeated. Used in the rank conjunction below.

template <dim_t N, class T> constexpr T samestep = N;
template <dim_t N, class ... T> constexpr std::tuple<T ...> samestep<N, std::tuple<T ...>> = { samestep<N, T> ... };

// Dest is a list of destination axes [l0 l1 ... li ... l(rank(A)-1)].
// The dimensions of the reframed A are numbered as [0 ... k ... max(l)-1].
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// If not, then axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO invalid for ANY, since Dest is compile time. [ra7]

template <class Dest, IteratorConcept A>
struct Reframe
{
    A a;

    constexpr static int orig(int k) { return mp::int_list_index<Dest>(k); }
    consteval static rank_t rank() { return 1+mp::fold<mp::max, ic_t<-1>, Dest>::value; }
    constexpr static dim_t len_s(int k)
    {
        int l=orig(k);
        return l>=0 ? std::decay_t<A>::len_s(l) : BAD;
    }
    constexpr dim_t
    len(int k) const
    {
        int l=orig(k);
        return l>=0 ? a.len(l) : BAD;
    }
    constexpr auto
    step(int k) const
    {
        int l=orig(k);
        return l>=0 ? a.step(l) : samestep<0, decltype(a.step(l))>;
    }
    constexpr void
    adv(rank_t k, dim_t d)
    {
        int l=orig(k);
        if (l>=0) { a.adv(l, d); }
    }
    constexpr bool
    keep_step(dim_t st, int z, int j) const
    {
        int wz=orig(z), wj=orig(j);
        return wz>=0 && wj>=0 && a.keep_step(st, wz, wj);
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        return a.at(mp::map_indices<dim_t, Dest>(i));
    }
    constexpr decltype(auto) operator*() const { return *a; }
    constexpr auto save() const { return a.save(); }
    constexpr void load(auto const & p) { a.load(p); }
// FIXME only if Dest preserves axis order, which is how wrank works, but this limitation should be explicit.
    constexpr void mov(auto const & s) { a.mov(s); }
};

// Optimize no-op case. TODO If A is CellBig, etc. beat Dest on it, same for eventual transpose_expr<>.

template <class Dest, class A>
constexpr decltype(auto)
reframe(A && a)
{
    if constexpr (std::is_same_v<Dest, mp::iota<1+mp::fold<mp::max, ic_t<-1>, Dest>::value>>) {
        return RA_FWD(a);
    } else {
        return Reframe<Dest, A> { RA_FWD(a) };
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
wrank(cranks cranks_, Op && op) { return Verb<cranks, Op> { RA_FWD(op) }; }

template <rank_t ... crank, class Op>
constexpr auto
wrank(Op && op) { return Verb<mp::int_list<crank ...>, Op> { RA_FWD(op) }; }

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

template <class A, class B>
struct max_i
{
    constexpr static int value = (A::value == choose_rank(A::value, B::value)) ? 0 : 1;
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
    template <class VV> constexpr static decltype(auto) op(VV && v) { return FM::op(RA_FWD(v).op); } // cf [ra31]
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
// TODO -crank::value when the actual verb rank is used (eg to use CellBig<... that_rank> instead of just begin()).
    using R = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri>), skip>> ...>;
    template <class VV> constexpr static decltype(auto) op(VV && v) { return RA_FWD(v); }
};


// ---------------
// explicit agreement checks
// ---------------

template <class ... P>
constexpr bool
agree(P && ... p) { return agree_(ra::start(RA_FWD(p)) ...); }

// 0: fail, 1: rt, 2: pass
template <class ... P>
constexpr int
agree_s(P && ... p) { return agree_s_(ra::start(RA_FWD(p)) ...); }

template <class Op, class ... P> requires (is_verb<Op>)
constexpr bool
agree_op(Op && op, P && ... p) { return agree_verb(mp::iota<sizeof...(P)> {}, RA_FWD(op), RA_FWD(p) ...); }

template <class Op, class ... P> requires (!is_verb<Op>)
constexpr bool
agree_op(Op && op, P && ... p) { return agree(RA_FWD(p) ...); }

template <class ... P>
constexpr bool
agree_(P && ... p) { return (Match<false, std::tuple<P ...>> { RA_FWD(p) ... }).check(); }

template <class ... P>
constexpr int
agree_s_(P && ... p) { return Match<false, std::tuple<P ...>>::check_s(); }

template <class V, class ... T, int ... i>
constexpr bool
agree_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return agree_op(FM::op(RA_FWD(v)), reframe<mp::ref<typename FM::R, i>>(ra::start(RA_FWD(t))) ...);
}


// ---------------------------
// operator expression
// ---------------------------

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Expr;
template <class Op, IteratorConcept ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    using Match_ = Match<true, std::tuple<P ...>>;
    using Match_::t, Match_::rs, Match_::rank;
    Op op;

    constexpr Expr(Op op_, P ... p_): Match_(p_ ...), op(op_) {} // [ra1]
    RA_ASSIGNOPS_SELF(Expr)
    RA_ASSIGNOPS_DEFAULT_SET
    constexpr decltype(auto) at(auto const & j) const { return std::invoke(op, std::get<I>(t).at(j) ...); }
    constexpr decltype(auto) operator*() const { return std::invoke(op, *std::get<I>(t) ...); }
// needed for rs==ANY, which don't decay to scalar when used as operator arguments.
    constexpr
    operator decltype(std::invoke(op, *std::get<I>(t) ...)) () const
    {
        if constexpr (0!=rs && (1!=rs || 1!=size_s<Expr>())) { // for coord types; so ct only
            static_assert(rs==ANY);
            RA_CHECK(0==rank(), "Bad scalar conversion from shape [", ra::noshape, ra::shape(*this), "].");
        }
        return *(*this);
    }
};

template <class Op, IteratorConcept ... P>
constexpr bool is_special_def<Expr<Op, std::tuple<P ...>>> = (is_special<P> || ...);

template <class V, class ... T, int ... i>
constexpr auto
expr_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return expr(FM::op(RA_FWD(v)), reframe<mp::ref<typename FM::R, i>>(RA_FWD(t)) ...);
}

template <class Op, class ... P>
constexpr auto
expr(Op && op, P && ... p)
{
    if constexpr (is_verb<Op>) {
        return expr_verb(mp::iota<sizeof...(P)> {}, RA_FWD(op), RA_FWD(p) ...);
    } else {
        return Expr<Op, std::tuple<P ...>> { RA_FWD(op), RA_FWD(p) ... };
    }
}

template <class Op, class ... A>
constexpr auto
map(Op && op, A && ... a)
{
    return expr(RA_FWD(op), start(RA_FWD(a)) ...);
}


// ---------------------------
// pick
// ---------------------------

template <class T, class J> struct pick_at_type;
template <class ... P, class J> struct pick_at_type<std::tuple<P ...>, J>
{
    using type = std::common_reference_t<decltype(std::declval<P>().at(std::declval<J>())) ...>;
};

template <std::size_t I, class T, class J>
constexpr pick_at_type<mp::drop1<std::decay_t<T>>, J>::type
pick_at(std::size_t p0, T && t, J const & j)
{
    constexpr std::size_t N = mp::len<std::decay_t<T>> - 1;
    if constexpr (I < N) {
        return (p0==I) ? std::get<I+1>(t).at(j) : pick_at<I+1>(p0, t, j);
    } else {
        RA_CHECK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = std::common_reference_t<decltype(*std::declval<P>()) ...>;
};

template <std::size_t I, class T>
constexpr pick_star_type<mp::drop1<std::decay_t<T>>>::type
pick_star(std::size_t p0, T && t)
{
    constexpr std::size_t N = mp::len<std::decay_t<T>> - 1;
    if constexpr (I < N) {
        return (p0==I) ? *(std::get<I+1>(t)) : pick_star<I+1>(p0, t);
    } else {
        RA_CHECK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class T, class K=mp::iota<mp::len<T>>> struct Pick;
template <IteratorConcept ... P, int ... I>
struct Pick<std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    using Match_ = Match<true, std::tuple<P ...>>;
    using Match_::t, Match_::rs, Match_::rank;
    static_assert(sizeof...(P)>1);

    constexpr Pick(P ... p_): Match_(p_ ...) {} // [ra1]
    RA_ASSIGNOPS_SELF(Pick)
    RA_ASSIGNOPS_DEFAULT_SET
    constexpr decltype(auto) at(auto const & j) const { return pick_at<0>(std::get<0>(t).at(j), t, j); }
    constexpr decltype(auto) operator*() const { return pick_star<0>(*std::get<0>(t), t); }
// needed for xpr with rs==ANY, which don't decay to scalar when used as operator arguments.
    constexpr
    operator decltype(pick_star<0>(*std::get<0>(t), t)) () const
    {
        if constexpr (0!=rs && (1!=rs || 1!=size_s<Pick>())) { // for coord types; so ct only
            static_assert(rs==ANY);
            RA_CHECK(0==rank(), "Bad scalar conversion from shape [", ra::noshape, ra::shape(*this), "].");
        }
        return *(*this);
    }
};

template <IteratorConcept ... P>
constexpr bool is_special_def<Pick<std::tuple<P ...>>> = (is_special<P> || ...);

template <class ... P> Pick(P && ... p) -> Pick<std::tuple<P ...>>;

template <class ... P> constexpr auto
pick(P && ... p) { return Pick { start(RA_FWD(p)) ... }; }

} // namespace ra
