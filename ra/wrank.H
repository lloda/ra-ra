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

#if defined(RA_CHECK_BOUNDS) && RA_CHECK_BOUNDS==0
  #define CHECK_BOUNDS( cond )
#else
  #define CHECK_BOUNDS( cond ) RA_ASSERT( cond, 0 )
#endif

// TODO Adopt frame matching as in Match (no driver).

namespace ra {

template <class cranks, class Op>
struct Verb
{
    using R = cranks;
    Op op;
};

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

template <class A> using ValidRank = mp::int_t<(A::value>=0)>;

template <class R, class skip, class frank>
using AddFrameAxes = mp::append<R, mp::iota<frank::value, skip::value>>;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

// FIXME Replace the frame matching mechanism by driverless, per-axis Match

template <class A, class B>
struct max_i
{
    constexpr static int value = gt_rank(A::value, B::value) ? 0 : 1; // 0 if ra wins, else 1
};

template <class T> using largest_i_tuple = mp::IndexOf<max_i, T>;

// Get a list (per argument) of lists of live axes.
// The last frame match is not done; that relies on rest axis handling of each argument (ignoring axis spec beyond their own rank). TODO Reexamine that.
// Case where V has rank.
template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "bad args");

    using T = std::tuple<Ti ...>;
    using R_ = std::tuple<Ri ...>;

// TODO functions of arg rank, negative, inf.
// live = number of live axes on this frame, for each argument.
    using live = mp::int_list<(std::decay_t<Ti>::rank_s()-mp::len<Ri>-crank::value) ...>;
    static_assert(mp::apply<mp::andb, mp::map<ValidRank, live>>::value, "bad ranks");

// select driver for this stage.
    constexpr static int driver = largest_i_tuple<live>::value;

// add actual axes to result.
    using skips = mp::makelist<sizeof...(Ti), mp::int_t<skip>>;
    using FM = Framematch<W, T, mp::map<AddFrameAxes, R_, skips, live>,
                          skip + mp::ref<live, driver>::value>;
    using R = typename FM::R;
    constexpr static int depth = mp::ref<live, driver>::value + FM::depth;

// drill down in V to get innermost Op (cf [ra31]).
    template <class VV> static decltype(auto) op(VV && v) { return FM::op(std::forward<VV>(v).op); }
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    using R_ = std::tuple<Ri ...>;

// TODO -crank::value when the actual verb rank is used (e.g. to use cell_iterator<A, that_rank> instead of just begin()).
    using live = mp::int_list<(std::decay_t<Ti>::rank_s()-mp::len<Ri>) ...>;
    static_assert(mp::apply<mp::andb, mp::map<ValidRank, live>>::value, "bad ranks");
    constexpr static int driver = largest_i_tuple<live>::value;

    using skips = mp::makelist<sizeof...(Ti), mp::int_t<skip>>;
    using R = mp::map<AddFrameAxes, R_, skips, live>;
    constexpr static int depth = mp::ref<live, driver>::value;

    template <class VV> static decltype(auto) op(VV && v) { return std::forward<VV>(v); }
};

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

// Wraps each argument of an expression using wrank.
// no shape(), size_s(), rank_s(), rank() -> this is above.
template <class LiveAxes, int depth, class A>
struct ApplyFrames
{
    A a;

    constexpr static int live(int k) { return mp::int_list_index<LiveAxes>(k); }

    template <class I>
    constexpr decltype(auto) at(I const & i)
    {
        return a.at(mp::map_indices<std::array<dim_t, mp::len<LiveAxes>>, LiveAxes>(i));
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
    constexpr decltype(auto) flat() { return a.flat(); }
};

template <class LiveAxes, int depth, class A>
decltype(auto) applyframes(A && a)
{
// No-op case. TODO Maybe apply to any Iota<n> where n<=depth.
// TODO If A is cell_iterator, etc. beat LiveAxes directly on that... same for an eventual transpose_expr<>.
    if constexpr (std::is_same_v<LiveAxes, mp::iota<depth>>) {
        return std::forward<A>(a);
    } else {
        return ApplyFrames<LiveAxes, depth, A> { std::forward<A>(a) };
    }
}

// like Expr, except don't do driver selection here, but leave it to the args. The args may need to be ApplyFrames... don't know yet.
// forward decl in atom.H.
template <class FM, class Op, class ... P, int ... I>
struct Ryn<FM, Op, std::tuple<P ...>, mp::int_list<I ...>>
{
    Op op;
    std::tuple<P ...> t;

    template <int iarg>
    constexpr bool check()
    {
        for (int k=0; k!=rank(); ++k) { // TODO with static rank or sizes, can peval.
            dim_t s0 = size(k);
            dim_t sk = std::get<iarg>(t).size(k);
            if (sk!=s0 && sk!=DIM_BAD) { // TODO See Match::check(); maybe just sk>=0.
                return false;
            }
        }
        return true;
    }
    constexpr Ryn(Op op_, P ... t_): op(std::forward<Op>(op_)), t(std::forward<P>(t_) ...)
    {
        CHECK_BOUNDS((check<I>() && ... ) && "mismatched shapes");
    }

    template <class J>
    constexpr decltype(auto) at(J const & i)
    {
        return op(std::get<I>(t).at(i) ...);
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }
    constexpr bool keep_stride(dim_t step, int z, int j) const
    {
        return (std::get<I>(t).keep_stride(step, z, j) && ...);
    }
    constexpr auto stride(int i) const
    {
        return std::make_tuple(std::get<I>(t).stride(i) ...);
    }
    constexpr auto flat()
    {
        return ra::flat(op, std::get<I>(t).flat() ...);
    }

// Use the first arg that gives size(k)>=0; valid by ApplyFrame.
// TODO if k were static, we could pick the driving arg from axisdrivers. Only need bool from that.
    template <int iarg=0>
    constexpr dim_t size(int k) const
    {
        if constexpr (iarg<sizeof...(P)) {
            dim_t s = std::get<iarg>(t).size(k);
            return s>=0 ? s : size<iarg+1>(k);
        } else {
            abort(); return DIM_BAD;
        }
    }
    constexpr static dim_t size_s() { return DIM_ANY; } // BUG [ra07]
    constexpr static dim_t size_s(int i) { return DIM_ANY; } // BUG [ra07]
    constexpr static rank_t rank() { return FM::depth; } // TODO Invalid for RANK_ANY [ra07]
    constexpr static rank_t rank_s() { return FM::depth; } // TODO Invalid for RANK_ANY [ra07]
    constexpr auto shape() const
    {
        std::array<dim_t, FM::depth> s {};
        for (int k=0; k!=FM::depth; ++k) {
            s[k] = size(k);
            CHECK_BOUNDS(s[k]!=DIM_BAD);
        }
        return s;
    }

// forward to make sure value y is not misused as ref [ra05]
#define DEF_RYN_ASSIGNOPS(OP)                                           \
    template <class X> void operator OP(X && x)                         \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_RYN_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_RYN_ASSIGNOPS
};

template <class FM, class Op, class ... T> inline constexpr
auto ryn(Op && op, T && ... t)
{
    return Ryn<FM, Op, std::tuple<T ...>> { std::forward<Op>(op), std::forward<T>(t) ... }; // (note 1)
}

template <class V, class ... T, int ... i> inline constexpr
auto number_ryn(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return ryn<FM>(FM::op(std::forward<V>(v)), applyframes<mp::ref<typename FM::R, i>, FM::depth>(std::forward<T>(t)) ...);
}

// TODO partial specialization means no universal ref :-/
#define DEF_EXPR_VERB(MOD)                                              \
    template <class cranks, class Op, class ... P> inline constexpr     \
    auto expr(Verb<cranks, Op> MOD v, P && ... t)                       \
    {                                                                   \
        return number_ryn(mp::iota<sizeof...(P)> {}, std::forward<decltype(v)>(v), std::forward<P>(t) ...); \
    }
FOR_EACH(DEF_EXPR_VERB, &&, &, const &)
#undef DEF_EXPR_VERB

// ---------------------------
// from, after APL, like (from) in guile-ploy
// TODO integrate with is_beatable shortcuts, operator() in the various array types.
// ---------------------------

template <class I>
struct index_rank_
{
    using type = mp::int_t<std::decay_t<I>::rank_s()>; // see ra_traits for ra::types (?)
    static_assert(type::value!=RANK_ANY, "dynamic rank unsupported");
    static_assert(std::decay_t<I>::size_s()!=DIM_BAD, "undelimited extent subscript unsupported");
};

template <class I> using index_rank = typename index_rank_<I>::type;

template <class II, int drop, class Op> inline constexpr
decltype(auto) from_partial(Op && op)
{
    if constexpr (drop==mp::len<II>) {
        return std::forward<Op>(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, mp::int_t<0>>, mp::drop<II, drop>> {},
                     from_partial<II, drop+1>(std::forward<Op>(op)));
    }
}

// TODO we should be able to do better by slicing at each dimension, etc. But verb<> only supports rank-0 for the innermost op.
template <class A, class ... I> inline constexpr
auto from(A && a, I && ... i)
{
    if constexpr (0==sizeof...(i)) {
        return a();
    } else if constexpr (1==sizeof...(i)) {
// support dynamic rank for 1 arg only (see test in test/from.C).
        return expr(std::forward<A>(a), start(std::forward<I>(i) ...));
    } else {
        using II = mp::map<index_rank, mp::tuple<decltype(start(std::forward<I>(i))) ...>>;
        return expr(from_partial<II, 1>(std::forward<A>(a)), start(std::forward<I>(i)) ...);
    }
}

} // namespace ra

#undef CHECK_BOUNDS
