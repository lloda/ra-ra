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
choose_rank(rank_t ra, rank_t rb)
{
    return BAD==rb ? ra : BAD==ra ? rb : ANY==ra ? ra : ANY==rb ? rb : (ra>=rb ? ra : rb);
}

// if non-negative args don't match, pick first (see below). FIXME maybe return invalid.
constexpr dim_t
choose_len(dim_t sa, dim_t sb)
{
    return BAD==sa ? sb : BAD==sb ? sa : ANY==sa ? sb : sa;
}

template <bool checkp, class T, class K=mp::iota<mp::len<T>>> struct Match;

template <bool checkp, IteratorConcept ... P, int ... I>
struct Match<checkp, std::tuple<P ...>, mp::int_list<I ...>>
{
    using T = std::tuple<P ...>;
    T t;

    // 0: fail, 1: rt, 2: pass
    constexpr static int
    check_s()
    {
        if constexpr (sizeof...(P)<2) {
            return 2;
        } else if constexpr (ANY==rank_s()) {
            return 1; // FIXME could be tightened to 2 in some cases
        } else {
            bool tbc = false;
            for (int k=0; k<rank_s(); ++k) {
                dim_t ls = len_s(k);
                if (((k<std::decay_t<P>::rank_s() && ls!=choose_len(std::decay_t<P>::len_s(k), ls)) || ...)) {
                    return 0;
                } else {
                    int anyk = ((k<std::decay_t<P>::rank_s() && (ANY==std::decay_t<P>::len_s(k))) + ...);
                    int fixk = ((k<std::decay_t<P>::rank_s() && (0<=std::decay_t<P>::len_s(k))) + ...);
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
        } else  if constexpr (constexpr int c = check_s(); 0==c) {
            return false;
        } else if constexpr (1==c) {
            for (int k=0; k<rank(); ++k) {
                dim_t ls = len(k);
                if (((k<std::get<I>(t).rank() && ls!=choose_len(std::get<I>(t).len(k), ls)) || ...)) {
                    RA_CHECK(!checkp, "Shape mismatch [", (std::array { std::get<I>(t).len(k) ... }), "] on axis ", k, ".");
                    return false;
                }
            }
        }
        return true;
    }

    constexpr
    Match(P ... p_): t(std::forward<P>(p_) ...)
    {
// TODO Maybe on ply, would avoid the checkp, make agree_xxx() unnecessary.
        if constexpr (checkp && !(has_len<P> || ...)) {
            static_assert(check_s(), "Shape mismatch.");
            RA_CHECK(check());
        }
    }

// rank of largest subexpr, so we look at all of them.
    constexpr static rank_t
    rank_s()
    {
        rank_t r = BAD;
        return ((r=choose_rank(r, ra::rank_s<P>())), ...);
    }

    constexpr static rank_t
    rank()
    requires (ANY != Match::rank_s())
    {
        return rank_s();
    }

    constexpr rank_t
    rank() const
    requires (ANY == Match::rank_s())
    {
        rank_t r = BAD;
        ((r = choose_rank(r, std::get<I>(t).rank())), ...);
        assert(ANY!=r); // not at runtime
        return r;
    }

// first nonnegative size, if none first ANY, if none then BAD
    constexpr static dim_t
    len_s(int k)
    {
        auto f = [&k]<class A>(dim_t s) {
            constexpr rank_t ar = A::rank_s();
            return (ar<0 || k<ar) ? choose_len(s, A::len_s(k)) : s;
        };
        dim_t s = BAD; ((s>=0 ? s : s = f.template operator()<std::decay_t<P>>(s)), ...);
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
        auto f = [&k](dim_t s, auto const & a) {
            return k<a.rank() ? choose_len(s, a.len(k)) : s;
        };
        dim_t s = BAD; ((s>=0 ? s : s = f(s, std::get<I>(t))), ...);
        assert(ANY!=s); // not at runtime
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
};


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
// TODO invalid for ANY (since Dest is compile time). [ra7]

template <class Dest, IteratorConcept A>
struct Reframe
{
    A a;

    constexpr static int orig(int k) { return mp::int_list_index<Dest>(k); }
    constexpr static rank_t rank_s() { return 1+mp::fold<mp::max, ic_t<-1>, Dest>::value; }
    constexpr static rank_t rank() { return rank_s(); }
    constexpr static dim_t len_s(int k)
    {
        int l = orig(k);
        return l>=0 ? std::decay_t<A>::len_s(l) : BAD;
    }
    constexpr dim_t
    len(int k) const
    {
        int l = orig(k);
        return l>=0 ? a.len(l) : BAD;
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
        return a.at(mp::map_indices<dim_t, Dest>(i));
    }
};

// Optimize no-op case.
// TODO If A is CellBig, etc. beat Dest directly on it, same for eventual transpose_expr<>.

template <class Dest, class A>
constexpr decltype(auto)
reframe(A && a)
{
    if constexpr (std::is_same_v<Dest, mp::iota<1+mp::fold<mp::max, ic_t<-1>, Dest>::value>>) {
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
// FIXME flagged by gcc 12.1 -O3
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
// needed for rank_s()==ANY, which don't decay to scalar when used as operator arguments.
    constexpr
    operator decltype(*(flat(op, std::get<I>(t).flat() ...))) ()
    {
        if constexpr (0!=rank_s() && (1!=rank_s() || 1!=size_s<Expr>())) { // for coord types; so ct only
            static_assert(rank_s()==ANY);
            assert(0==rank());
        }
        return *flat();
    }
};

template <class Op, IteratorConcept ... P>
constexpr bool is_special_def<Expr<Op, std::tuple<P ...>>> = (is_special<P> || ...);

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
// explicit agreement checks
// ---------------

template <class ... P>
constexpr bool
agree(P && ... p)
{
    return agree_(ra::start(std::forward<P>(p)) ...);
}

// 0: fail, 1: rt, 2: pass
template <class ... P>
constexpr int
agree_s(P && ... p)
{
    return agree_s_(ra::start(std::forward<P>(p)) ...);
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
    return (Match<false, std::tuple<P ...>> { std::forward<P>(p) ... }).check();
}

template <class ... P>
constexpr int
agree_s_(P && ... p)
{
    return Match<false, std::tuple<P ...>>::check_s();
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


// ---------------------------
// pick
// ---------------------------

template <class T, class J> struct pick_at_type;
template <class ... P, class J> struct pick_at_type<std::tuple<P ...>, J>
{
    using type = mp::apply<std::common_reference_t, std::tuple<decltype(std::declval<P>().at(std::declval<J>())) ...>>;
};

template <std::size_t I, class T, class J>
constexpr pick_at_type<mp::drop1<std::decay_t<T>>, J>::type
pick_at(std::size_t p0, T && t, J const & j)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return std::get<I+1>(t).at(j);
        } else {
            return pick_at<I+1>(p0, t, j);
        }
    } else {
        RA_CHECK(p0==I, " p0 ", p0, " I ", I);
        return std::get<I+1>(t).at(j);
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = mp::apply<std::common_reference_t, std::tuple<decltype(*std::declval<P>()) ...>>;
};

template <std::size_t I, class T>
constexpr pick_star_type<mp::drop1<std::decay_t<T>>>::type
pick_star(std::size_t p0, T && t)
{
    if constexpr (I+2<std::tuple_size_v<std::decay_t<T>>) {
        if (p0==I) {
            return *(std::get<I+1>(t));
        } else {
            return pick_star<I+1>(p0, t);
        }
    } else {
        RA_CHECK(p0==I, " p0 ", p0, " I ", I);
        return *(std::get<I+1>(t));
    }
}

template <class T, class K=mp::iota<mp::len<T>>> struct Pick;

template <IteratorConcept ... P, int ... I>
struct Pick<std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    static_assert(sizeof...(P)>1);

    template <class T_>
    struct Flat
    {
        T_ t;
        template <class S> constexpr void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
        constexpr decltype(auto) operator*() { return pick_star<0>(*std::get<0>(t), t); }
    };

    template <class ... P_>
    constexpr static auto
    flat(P_ && ... p)
    {
        return Flat<std::tuple<P_ ...>> { std::tuple<P_ ...> { std::forward<P_>(p) ... } };
    }

    using Match_ = Match<true, std::tuple<P ...>>;
    using Match_::t, Match_::rank_s, Match_::rank;

// test/ra-9.cc [ra1]
    constexpr Pick(P ... p_): Match_(std::forward<P>(p_) ...) {}
    RA_DEF_ASSIGNOPS_SELF(Pick)
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr decltype(auto)
    flat()
    {
        return flat(std::get<I>(t).flat() ...);
    }
    constexpr decltype(auto)
    at(auto const & j) const
    {
        return pick_at<0>(std::get<0>(t).at(j), t, j);
    }
// needed for xpr with rank_s()==ANY, which don't decay to scalar when used as operator arguments.
    constexpr
    operator decltype(*(flat(std::get<I>(t).flat() ...))) ()
    {
        if constexpr (0!=rank_s() && (1!=rank_s() || 1!=size_s<Pick>())) { // for coord types; so ct only
            static_assert(rank_s()==ANY);
            assert(0==rank());
        }
        return *flat();
    }
};

template <IteratorConcept ... P>
constexpr bool is_special_def<Pick<std::tuple<P ...>>> = (is_special<P> || ...);

template <class ... P> Pick(P && ... p) -> Pick<std::tuple<P ...>>;

template <class ... P>
constexpr auto
pick(P && ... p)
{
    return Pick { start(std::forward<P>(p)) ... };
}

} // namespace ra
