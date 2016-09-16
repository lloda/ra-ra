
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_WRANK_H
#define RA_WRANK_H

/// @file wrank.H
/// @brief Rank conjunction for arrays and expr templates.

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

// In order of urgency.
// @TODO Make it work with fixed size types.
// @TODO Make InnerA/OuterA choice robust wrt ply order.
// @TODO Make it work with var rank types.

namespace ra {

template <class cranks, class Op>
struct Verb
{
    using R = cranks;
    Op op;
};

template <rank_t ... crank>
struct verb
{
    using cranks = mp::int_list<crank ...>;

    template <class Op>
    static auto make(Op && op)
    {
        return Verb<cranks, Op> { std::forward<Op>(op) };
    }
};
template <rank_t ... crank, class Op> auto wrank(Op && op)
{
    return verb<crank ...>::make(std::forward<Op>(op));
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
struct Framematch;

// Get a list (per argument) of lists of live axes.
// The last frame match is not done; that relies on rest axis handling of each argument (ignoring axis spec beyond their own rank). @TODO Reexamine that.
// 1. case where V has rank.
template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "bad args");

    using T = std::tuple<Ti ...>;
    using R_ = std::tuple<Ri ...>;

// @TODO functions of arg rank, negative, inf.
// live = number of live axes on this frame, for each argument.
    using live = mp::int_list<(std::decay_t<Ti>::rank_s()-mp::len<Ri>-crank::value) ...>;
    static_assert(mp::Apply_<mp::And, mp::Map_<ValidRank, live>>::value, "bad ranks");

// select driver for this stage.
    static int const driver = largest_i_tuple<live>::value;

// add actual axes to result.
    using skips = mp::MakeList_<sizeof...(Ti), mp::int_t<skip>>;
    using FM = Framematch<std::decay_t<W>, T, mp::Map_<AddFrameAxes, R_, skips, live>,
                          skip + mp::Ref_<live, driver>::value>;
    using R = typename FM::R;
    static int const depth = mp::Ref_<live, driver>::value + FM::depth;
    using framedrivers = mp::Cons_<mp::int_t<driver>, typename FM::framedrivers>;
    using axisdrivers = mp::Append_<mp::MakeList_<mp::Ref_<live, driver>::value, mp::int_t<driver>>,
                                    typename FM::axisdrivers>;
    using axisaxes = mp::Append_<mp::Iota_<mp::Ref_<live, driver>::value, mp::len<mp::Ref_<R_, driver>>>,
                                 typename FM::axisaxes>;
    using argindices = mp::Zip_<axisdrivers, axisaxes>;

// Drill down in V to get innermost Op.
    template <class VV>
    static decltype(auto) op(VV && v)
    {
        return FM::op(v.op);
    }
};

template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch<Verb<std::tuple<crank ...>, W> &, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
    : public Framematch<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip> {};

template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch<Verb<std::tuple<crank ...>, W> const &, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
    : public Framematch<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip> {};

// 2. terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    using R_ = std::tuple<Ri ...>;

// @TODO -crank::value when the actual verb rank is used (e.g. to use ra_iterator<A, that_rank> instead of just begin()).
    using live = mp::int_list<(std::decay_t<Ti>::rank_s()-mp::len<Ri>) ...>;
    static_assert(mp::Apply_<mp::And, mp::Map_<ValidRank, live>>::value, "no dynamic ranks allowed");
    static int const driver = largest_i_tuple<live>::value;

    using skips = mp::MakeList_<sizeof...(Ti), mp::int_t<skip>>;
    using R = mp::Map_<AddFrameAxes, R_, skips, live>;
    static int const depth = mp::Ref_<live, driver>::value;
    using framedrivers = mp::int_list<driver>;
    using axisdrivers = mp::MakeList_<mp::Ref_<live, driver>::value, mp::int_t<driver>>;
    using axisaxes = mp::Iota_<mp::Ref_<live, driver>::value, mp::len<mp::Ref_<R_, driver>>>;
    using argindices = mp::Zip_<axisdrivers, axisaxes>;

    template <class VV>
    static decltype(auto) op(VV && v)
    {
        return std::forward<VV>(v);
    }
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

// no shape() -> this is above.
// no size_s(), rank_s(), rank() -> this is above.
// flat() -> forwarded to a.
template <class LiveAxes, int depth, class A>
struct ApplyFrames
{
    using RestAxes = mp::Complement_<LiveAxes, depth>;

    A a;
    explicit ApplyFrames(A a_): a(a_) {}

    constexpr static int live(int k) { return mp::on_tuple<LiveAxes>::index(k); }

    template <class I>
    constexpr decltype(auto) at(I const & i)
    {
        return a.at(mp::map_indices<LiveAxes, std::array<dim_t, mp::len<LiveAxes> > >::f(i));
    }
    constexpr dim_t size(int const k) const
    {
        int l = live(k);
        return l>=0 ? a.size(l) : DIM_BAD;
    }
    void adv(rank_t const k, dim_t const d)
    {
        int l = live(k);
        if (l>=0) {
            a.adv(l, d);
        }
    }
    auto stride(int const k) const
    {
        int l = live(k);
        return l>=0 ? a.stride(l) : zerostride<decltype(a.stride(l))>::f();
    }
    constexpr bool compact_stride(dim_t const step, int const z, int const j) const
    {
        int wz = live(z);
        int wj = live(j);
        return wz>=0 && wj>=0 && a.compact_stride(step, wz, wj); // @BUG may not be false.
    }
    auto flat() { return a.flat(); }
    auto flat() const { return a.flat(); }
};

template <class LiveAxes, int depth, class Enable=void>
struct applyframes
{
    template <class A>
    static ApplyFrames<LiveAxes, depth, A> f(A && a)
    {
        return ApplyFrames<LiveAxes, depth, A> { std::forward<A>(a) };
    }
};

// // No-op case. @TODO Maybe apply to any Iota<n> where n<=depth. @TODO Need to offer a default live().
// template <class LiveAxes, int depth>
// struct applyframes<LiveAxes, depth, std::enable_if_t<std::is_same<LiveAxes, mp::Iota_<depth>>::value>>
// {
//     template <class A>
//     static decltype(auto) f(A && a)
//     {
//         return std::forward<A>(a);
//     }
// };

// like Expr, except don't do driver selection here, but leave it to the args, as with Expr::adv(k, d). The args may need to be ApplyFrames... don't know yet.
// forward decl in atom.H.
template <class FM, class Op, class ... Ti, class ... Ki>
struct Ryn<FM, Op, std::tuple<Ti ...>, std::tuple<Ki ...>>
{
    using T = std::tuple<Ti ...>;
    using K = std::tuple<Ki ...>;

// @BUG Use of these depends on traversal order.
    static int const OuterA = mp::First_<typename FM::axisdrivers>::value;
    static int const InnerA = mp::Last_<typename FM::axisdrivers>::value;

// @TODO Prove that the last frame covers all the raveled last axes.
    using leaves_t = Leaves<Op, InnerA, std::tuple<decltype(std::declval<Ti>().flat()) ...>>;

    Op op;
    T t;

    template <int iarg>
    std::enable_if_t<(iarg==mp::len<K>), bool>
    check(dim_t const s0, int const k)
    {
        return true;
    }
    template <int iarg>
    std::enable_if_t<(iarg<mp::len<K>), bool>
    check(dim_t const s0, int const k)
    {
        dim_t sk = std::get<mp::Ref_<K, iarg>::value>(t).size(k);
        if (sk!=s0 && sk!=DIM_BAD) { // @TODO See Expr::check(); maybe just sk>=0.
            return false;
        }
        return check<iarg+1>(s0, k);
    }
    bool check()
    {
        for (int k=0; k!=rank(); ++k) { // @TODO with static rank or sizes, can peval.
            if (!check<0>(size(k), k)) {
                return false;
            }
        }
        return true;
    }

    Ryn(Op op_, Ti ... t_): op(op_), t(t_ ...)
    {
        CHECK_BOUNDS(check() && "mismatched shapes");
    }
    template <class J>
    constexpr decltype(auto) at(J const & i)
    {
        return op(std::get<Ki::value>(t).at(i) ...);
    }

// Find the driver for given axis. This pattern is used below.
    template <int iarg=0>
    std::enable_if_t<(iarg<mp::len<K>), int>
    constexpr driver(int const k)
    {
        dim_t s = std::get<iarg>(t).size(k);
        return s>=0 ? iarg : driver<iarg+1>(k);
    }
    template <int iarg>
    std::enable_if_t<(iarg==mp::len<K>), int>
    constexpr driver(int const k)
    {
        assert(0 && "there was no driver"); abort();
    }

// Use the first arg that gives size(k)>=0; valid by ApplyFrame.
    template <int iarg=0>
    std::enable_if_t<(iarg<mp::len<K>), dim_t>
    constexpr size(int const k) const
    {
        dim_t s = std::get<iarg>(t).size(k);
        return s>=0 ? s : size<iarg+1>(k);
    }
    template <int iarg>
    std::enable_if_t<(iarg==mp::len<K>), dim_t>
    constexpr size(int const k) const
    {
        assert(0 && "internal error"); abort();
    }

    constexpr auto shape() const
    {
        std::array<dim_t, FM::depth> s;
        for (int k=0; k!=FM::depth; ++k) {
            s[k] = size(k);
        }
        return s;
    }
    constexpr static dim_t size_s() { return DIM_ANY; } // @BUG
    constexpr static rank_t rank_s() { return FM::depth; } // @TODO Invalid for RANK_ANY
    constexpr static rank_t rank() { return FM::depth; } // @TODO Invalid for RANK_ANY

// @TODO if k were static, we could pick the driving arg from axisdrivers. Only need bool from that.
    void adv(rank_t const k, dim_t const d)
    {
        (std::get<Ki::value>(t).adv(k, d), ...);
    }
    template <int iarg=0>
    std::enable_if_t<(iarg<mp::len<K>), bool>
    constexpr compact_stride(dim_t const step, int const z, int const j) const
    {
        return (std::get<Ki::value>(t).compact_stride(step, z, j) && ...);
    }
    auto stride(int const i) const
    {
        return std::make_tuple(std::get<Ki::value>(t).stride(i) ...);
    }
    constexpr auto flat() const -> leaves_t
    {
        return leaves_t(op, std::get<Ki::value>(t).flat() ...);
    }

// forward to make sure value y is not misused as ref. Cf. test-ra-8.C.
#define DEF_RYN_EQOPS(OP)                                               \
    template <class X> void operator OP(X && x)                         \
    {                                                                   \
        ply_either(expr([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, start(x))); \
    }
    FOR_EACH(DEF_RYN_EQOPS, =, *=, +=, -=, /=)
#undef DEF_RYN_EQOPS
};

template <class FM, class Op, class ... Ti> inline
Ryn<FM, Op, std::tuple<Ti ...>>
ryn_(Op op, Ti && ... t)
{
    return Ryn<FM, Op, std::tuple<Ti ...>> { op, std::forward<Ti>(t) ... }; // (note 1)
}

template <class K>
struct number_ryn;

template <class ... Ki>
struct number_ryn<std::tuple<Ki ...>>
{
    template <class V, class ... Ti>
    static auto f(V && v, Ti && ... t)
    {
        using FM = Framematch<V, std::tuple<Ti ...>>;
// ryn_ doesn't forward because v is already wrapped as ref or not by FM::op.
        return ryn_<FM>(FM::op(std::forward<V>(v)), applyframes<mp::Ref_<typename FM::R, Ki::value>, FM::depth>::template f(std::forward<Ti>(t)) ...);
    }
};

template <class V, class ... Ti> inline
constexpr auto ryn(V && v, Ti && ... t)
{
    return number_ryn<mp::index_tuple<std::tuple<Ti ...>>>::f(std::forward<V>(v), std::forward<Ti>(t) ...);
}

template <class cranks, class Op, class ... Ti> inline
constexpr auto expr(Verb<cranks, Op> && v, Ti && ... t) // @TODO not universal ref anymore
{
    using V = decltype(v);
    return number_ryn<mp::index_tuple<std::tuple<Ti ...>>>::f(std::forward<V>(v), std::forward<Ti>(t) ...);
}

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

template <class II, int take>
struct from_partial_wrank
{
    template <class ... I> struct wrank_int_t { using type = verb<I::value ...>; };
    constexpr static int drop = mp::len<II>-take;
    using type = mp::Apply_<wrank_int_t,
                            mp::Append_<mp::MakeList_<drop, mp::int_t<0>>,
                                        mp::Drop_<II, drop> > >;
};

template <class II, int take>
struct from_partial
{
    template <class Op>
    static auto make(Op && op)
    {
        return from_partial_wrank<II, take>::type::make(from_partial<II, take-1>::make(std::forward<Op>(op)));
    }
};

template <class II>
struct from_partial<II, 0>
{
    template <class Op>
    static auto make(Op && op)
    {
        return std::forward<Op>(op);
    }
};

// @TODO should provide support for dynamic rank (see test in test-from.C).
template <class A, class I0> inline
auto from(A && a, I0 && i0)
{
    return expr([&a](auto && i0) -> decltype(auto) { return a(i0); }, start(std::forward<I0>(i0)));
}

// @TODO we should be able to do better by slicing at each dimension, etc. But verb<> only supports rank-0 for the innermost op.
template <class A, class ... I> inline
auto from(A && a, I && ... i)
{
    using II = mp::Map_<index_rank, mp::tuple<decltype(start(i)) ...> >;
    return expr(from_partial<II, mp::len<II>-1>::make([&a](auto && ... i) -> decltype(auto) { return a(i ...); } ),
                start(std::forward<I>(i)) ...);
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_WRANK

#endif // RA_WRANK_H
