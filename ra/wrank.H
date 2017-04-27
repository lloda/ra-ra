
// (c) Daniel Llorens - 2013-2017

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_WRANK_H
#define RA_WRANK_H

/// @file wrank.H
/// @brief Rank conjunction for expression templates.

#include "ra/expr.H"

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_WRANK RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_WRANK
        #define RA_CHECK_BOUNDS_RA_WRANK 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_WRANK==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

// @TODO Make it work with fixed size types.
// @TODO Make it work with var rank types.

namespace ra {

template <class cranks, class Op>
struct Verb
{
    using R = cranks;
    Op op;
};

template <class cranks, class Op> constexpr inline auto
wrank(cranks cranks_, Op && op)
{
    return Verb<cranks, Op> { std::forward<Op>(op) };
}

template <rank_t ... crank, class Op> constexpr inline auto
wrank(Op && op)
{
    return Verb<mp::int_list<crank ...>, Op> { std::forward<Op>(op) };
}

template <class A>
struct ValidRank
{
    using type = mp::int_t<(A::value>=0)>;
};

template <class R, class skip, class frank>
struct AddFrameAxes
{
    using type = mp::Append_<R, mp::Iota_<frank::value, skip::value>>;
};

template <class V, class T, class R=mp::MakeList_<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::MakeList_<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

// Get a list (per argument) of lists of live axes.
// The last frame match is not done; that relies on rest axis handling of each argument (ignoring axis spec beyond their own rank). @TODO Reexamine that.
// Case where V has rank.
template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "bad args");

    using T = std::tuple<Ti ...>;
    using R_ = std::tuple<Ri ...>;

// @TODO functions of arg rank, negative, inf.
// live = number of live axes on this frame, for each argument.
    using live = mp::int_list<(std::decay_t<Ti>::rank_s()-mp::len<Ri>-crank::value) ...>;
    static_assert(mp::Apply_<mp::And, mp::Map_<ValidRank, live>>::value, "bad ranks");

// select driver for this stage.
    constexpr static int driver = largest_i_tuple<live>::value;

// add actual axes to result.
    using skips = mp::MakeList_<sizeof...(Ti), mp::int_t<skip>>;
    using FM = Framematch<W, T, mp::Map_<AddFrameAxes, R_, skips, live>,
                          skip + mp::Ref_<live, driver>::value>;
    using R = typename FM::R;
    constexpr static int depth = mp::Ref_<live, driver>::value + FM::depth;

// drill down in V to get innermost Op (cf [ra31]).
    template <class VV> static decltype(auto) op(VV && v) { return FM::op(std::forward<VV>(v).op); }
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    using R_ = std::tuple<Ri ...>;

// @TODO -crank::value when the actual verb rank is used (e.g. to use ra_iterator<A, that_rank> instead of just begin()).
    using live = mp::int_list<(std::decay_t<Ti>::rank_s()-mp::len<Ri>) ...>;
    static_assert(mp::Apply_<mp::And, mp::Map_<ValidRank, live>>::value, "bad ranks");
    constexpr static int driver = largest_i_tuple<live>::value;

    using skips = mp::MakeList_<sizeof...(Ti), mp::int_t<skip>>;
    using R = mp::Map_<AddFrameAxes, R_, skips, live>;
    constexpr static int depth = mp::Ref_<live, driver>::value;

    template <class VV> static decltype(auto) op(VV && v) { return std::forward<VV>(v); }
};

template <class T>
struct zerostride
{
    constexpr static T f() { return T(0); }
};

template <class ... Ti>
struct zerostride<std::tuple<Ti ...>>
{
    constexpr static std::tuple<Ti ...> f() { return std::make_tuple(zerostride<Ti>::f() ...); }
};

// Wraps each argument of an expression using wrank.
// no shape(), size_s(), rank_s(), rank() -> this is above.
template <class LiveAxes, int depth, class A>
struct ApplyFrames
{
    A a;

    constexpr static int live(int k) { return mp::on_tuple<LiveAxes>::index(k); }

    template <class I>
    constexpr decltype(auto) at(I const & i)
    {
        return a.at(mp::map_indices<LiveAxes, std::array<dim_t, mp::len<LiveAxes>>>::f(i));
    }
    constexpr dim_t size(int k) const
    {
        int l = live(k);
        return l>=0 ? a.size(l) : DIM_BAD;
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        int l = live(k);
        if (l>=0) {
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
    constexpr decltype(auto) flat() const { return a.flat(); }
};

template <class LiveAxes, int depth, class Enable=void>
struct applyframes
{
    template <class A>
    static decltype(auto) f(A && a)
    {
        return ApplyFrames<LiveAxes, depth, A> { std::forward<A>(a) };
    }
};

// No-op case. @TODO Maybe apply to any Iota<n> where n<=depth.
// @TODO If A is ra_iterator, etc. beat LiveAxes directly on that... same for an eventual transpose_expr<>.
template <class LiveAxes, int depth>
struct applyframes<LiveAxes, depth, std::enable_if_t<std::is_same<LiveAxes, mp::Iota_<depth>>::value>>
{
    template <class A>
    static decltype(auto) f(A && a)
    {
        return std::forward<A>(a);
    }
};

// like Expr, except don't do driver selection here, but leave it to the args, as with Expr::adv(k, d). The args may need to be ApplyFrames... don't know yet.
// forward decl in atom.H.
template <class FM, class Op, class ... P, int ... I>
struct Ryn<FM, Op, std::tuple<P ...>, std::integer_sequence<int, I ...>>
{
    Op op;
    std::tuple<P ...> t;

    template <int iarg>
    bool check()
    {
        for (int k=0; k!=rank(); ++k) { // @TODO with static rank or sizes, can peval.
            dim_t s0 = size(k);
            dim_t sk = std::get<iarg>(t).size(k);
            if (sk!=s0 && sk!=DIM_BAD) { // @TODO See Expr::check(); maybe just sk>=0.
                return false;
            }
        }
        return true;
    }
    constexpr Ryn(Op op_, P ... t_): op(std::forward<Op>(op_)), t(std::forward<P>(t_) ...)
    {
        CHECK_BOUNDS(check<I>() && ... && "mismatched shapes");
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
    constexpr auto flat() const { return flat(); }

// Use the first arg that gives size(k)>=0; valid by ApplyFrame.
// @TODO if k were static, we could pick the driving arg from axisdrivers. Only need bool from that.
    template <int iarg=0>
    std::enable_if_t<(iarg<sizeof...(P)), dim_t>
    constexpr size(int k) const
    {
        dim_t s = std::get<iarg>(t).size(k);
        return s>=0 ? s : size<iarg+1>(k);
    }
    template <int iarg>
    std::enable_if_t<(iarg==sizeof...(P)), dim_t>
    constexpr size(int k) const
    {
        abort(); return DIM_BAD;
    }
    constexpr static dim_t size_s() { return DIM_ANY; } // @BUG
    constexpr static rank_t rank() { return FM::depth; } // @TODO Invalid for RANK_ANY
    constexpr static rank_t rank_s() { return FM::depth; } // @TODO Invalid for RANK_ANY
    constexpr auto shape() const
    {
        std::array<dim_t, FM::depth> s {};
        for (int k=0; k!=FM::depth; ++k) {
            s[k] = size(k);
            CHECK_BOUNDS(s[k]!=DIM_BAD);
        }
        return s;
    }

// forward to make sure value y is not misused as ref. Cf. test-ra-8.C.
#define DEF_RYN_ASSIGNOPS(OP)                                           \
    template <class X> void operator OP(X && x)                         \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_RYN_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_RYN_ASSIGNOPS
};

template <class FM, class Op, class ... P> inline
constexpr auto ryn(Op && op, P && ... t)
{
    return Ryn<FM, Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(t) ... }; // (note 1)
}

template <class K>
struct number_ryn;

template <int ... I>
struct number_ryn<std::integer_sequence<int, I ...>>
{
    template <class V, class ... P> constexpr static
    auto f(V && v, P && ... t)
    {
        using FM = Framematch<V, std::tuple<P ...>>;
        return ryn<FM>(FM::op(std::forward<V>(v)), applyframes<mp::Ref_<typename FM::R, I>, FM::depth>::f(std::forward<P>(t)) ...);
    }
};

// @TODO partial specialization means no universal ref :-/
#define DEF_EXPR_VERB(MOD)                                              \
    template <class cranks, class Op, class ... P> inline constexpr     \
    auto expr(Verb<cranks, Op> MOD v, P && ... t)                       \
    {                                                                   \
        return number_ryn<std::make_integer_sequence<int, sizeof...(P)>>::f(std::forward<decltype(v)>(v), std::forward<P>(t) ...); \
    }
FOR_EACH(DEF_EXPR_VERB, &&, &, const &)
#undef DEF_EXPR_VERB

// ---------------------------
// from, after APL, like (from) in guile-ploy
// @TODO integrate with is_beatable shortcuts, operator() in the various array types.
// ---------------------------

template <class I>
struct index_rank
{
    using type = mp::int_t<std::decay_t<I>::rank_s()>; // see ra_traits for ra::types (?)
    static_assert(type::value!=RANK_ANY, "dynamic rank unsupported");
    static_assert(std::decay_t<I>::size_s()!=DIM_BAD, "undelimited extent subscript unsupported");
};

template <class II, int drop, class Enable=void>
struct from_partial
{
    template <class Op>
    static decltype(auto) make(Op && op)
    {
        return wrank(mp::Append_<mp::MakeList_<drop, mp::int_t<0>>, mp::Drop_<II, drop>> {},
                     from_partial<II, drop+1>::make(std::forward<Op>(op)));
    }
};

template <class II, int drop>
struct from_partial<II, drop, std::enable_if_t<drop==mp::len<II>>>
{
    template <class Op>
    static decltype(auto) make(Op && op)
    {
        return std::forward<Op>(op);
    }
};

// Support dynamic rank for 1 arg only (see test in test-from.C).
template <class A, class I0> inline constexpr
auto from(A && a, I0 && i0)
{
    return expr(std::forward<A>(a), start(std::forward<I0>(i0)));
}

// @TODO we should be able to do better by slicing at each dimension, etc. But verb<> only supports rank-0 for the innermost op.
template <class A, class ... I> inline constexpr
auto from(A && a, I && ... i)
{
    using II = mp::Map_<index_rank, mp::tuple<decltype(start(std::forward<I>(i))) ...>>;
    return expr(from_partial<II, 1>::make(std::forward<A>(a)), start(std::forward<I>(i)) ...);
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_WRANK

#endif // RA_WRANK_H
