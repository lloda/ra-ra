
// (c) Daniel Llorens - 2011-2014, 2016-2017, 2019

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file expr.H
/// @brief Operator nodes for expression templates.

#pragma once
#include "ra/ply.H"

#ifndef RA_CHECK_BOUNDS_EXPR
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_EXPR 1
  #else
    #define RA_CHECK_BOUNDS_EXPR RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_EXPR==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

// Manipulate ET through flat (raw pointer-like) iterators P ...
template <class Op, class T, class I=mp::iota<mp::len<T>>>
struct Flat;

template <class Op, class ... P, int ... I>
struct Flat<Op, std::tuple<P ...>, mp::int_list<I ...>>
{
    Op & op;
    std::tuple<P ...> t;
    template <class S> constexpr void operator+=(S const & s) { ((std::get<I>(t) += std::get<I>(s)), ...); }
    constexpr decltype(auto) operator*() { return op(*std::get<I>(t) ...); }
};

template <class Op, class ... P> inline constexpr
auto flat(Op & op, P && ... p)
{
    return Flat<Op, std::tuple<P ...>> { op, std::tuple<P ...> { std::forward<P>(p) ... } };
}

template <class P> using var_rank = mp::bool_t<(std::decay_t<P>::size_s()==DIM_ANY)>;

// E is Expr or Pick.
template <int iarg, class E>
constexpr bool check_expr(int ranka, E const & e)
{
    using NotA = typename E::NotA;
    using PA = typename E::PA;
    using T = typename E::T;
    if constexpr (iarg<mp::len<NotA>) {
        constexpr int B = mp::ref<NotA, iarg>::value;
// provide safety where RANK_ANY was selected as driver in a leap of faith. TODO Dynamic driver selection.
// ct checks require ct rank. But for fixed rank / var size, size checks cannot be done at ct, so this will only do rank checks.
        using PB = std::decay_t<mp::ref<T, B>>;
        if constexpr (E::rank_s()>=0) {
            constexpr int rankb = PB::rank_s();
            constexpr auto f
                          = [](auto && f, auto k_)
                            {
                                constexpr int k = decltype(k_)::value;
                                static_assert(rankb<=E::rank_s() && "wrong driver selection");
                                if constexpr (k<rankb) {
                                    constexpr dim_t sa = PA::size_s(k);
                                    if constexpr (sa!=DIM_BAD) {
                                        constexpr dim_t sb = PB::size_s(k);
                                        if constexpr (sb!=DIM_BAD) {
                                            static_assert(sa==DIM_ANY || sb==DIM_ANY || sa==sb);
                                        }
                                    }
                                    f(f, mp::int_t<k+1> {});
                                }
                            };
            f(f, mp::int_t<0> {});
        }
// rt checks are needed if any sub is rt size, even if E's own size is ct.
        if constexpr (mp::apply<mp::orb, mp::map<var_rank, T>>::value) {
            rank_t rankb = std::get<B>(e.t).rank();
            assert(rankb<=ranka && "driver not max rank (could be RANK_ANY)");
            for (int k=0; k!=rankb; ++k) {
                if (dim_t sa=std::get<E::A>(e.t).size(k); sa!=DIM_BAD) { // maybe == in subexpressions
                    if (dim_t sb=std::get<B>(e.t).size(k); (sb!=sa) && (sb!=DIM_BAD)) {
                        std::cerr << "sa " << sa << ", sb " << sb << ": mismatched dimensions" << std::endl;
                        abort();
                    }
                }
            }
        }
        return check_expr<iarg+1>(ranka, e);
    } else {
        return true;
    }
}

// forward decl in atom.H
// TODO others:
// * 'static': Like expr, but the operator is compile time (e.g. ::apply()). Probably no need.
// * 'dynamic': Like expr, but driver is selected at run time (e.g. if args are RANK_ANY).
template <class Op, class ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>
{
// A-th argument decides rank and shape.
    constexpr static int A = driver_index<P ...>::value;
    using T = std::tuple<P ...>;
    using PA = std::decay_t<mp::ref<T, A>>;
    using NotA = mp::ComplementList_<mp::int_list<A>, mp::int_list<I ...>>;

    Op op;
    T t;

// If driver is RANK_ANY, driver selection should wait to run time, unless we can tell that RANK_ANY would be selected anyway.
    constexpr static bool CAN_DRIVE = PA::size_s()!=DIM_BAD ; //&& (PA::rank_s()!=RANK_ANY || sizeof...(P)==1);

// there's one size (by A), but each arg has its own strides.
// note: do not require driver. This is needed by check for all leaves.
    constexpr dim_t size(int i) const { return std::get<A>(t).size(i); }
    constexpr static dim_t size_s() { return PA::size_s(); }
    constexpr static dim_t size_s(int i) { return PA::size_s(i); }
    constexpr rank_t rank() const { return std::get<A>(t).rank(); }
    constexpr static rank_t rank_s() { return PA::rank_s(); }
    constexpr decltype(auto) shape() const { static_assert(CAN_DRIVE); return std::get<A>(t).shape(); }

// see test/ra-9.C [ra01] for forward() here.
    constexpr Expr(Op op_, P ... p_): op(std::forward<Op>(op_)), t(std::forward<P>(p_) ...)
    {
// TODO Should check only the driver: do this on ply.
        CHECK_BOUNDS(check_expr<0>(rank(), *this));
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
    constexpr decltype(auto) flat()
    {
        return ra::flat(op, std::get<I>(t).flat() ...);
    }

// needed for xpr with rank_s()==RANK_ANY, which don't decay to scalar when used as operator arguments.
    operator decltype(*(ra::flat(op, std::get<I>(t).flat() ...)))()
    {
        static_assert(rank_s()==0 || rank_s()==RANK_ANY || (rank_s()==1 && size_s()==1), // for coord types
                      "bad rank in conversion to scalar");
        assert(rank()==0 || (rank_s()==1 && size_s()==1)); // for coord types; so fixed only
        return *flat();
    }

// forward to make sure value y is not misused as ref [ra05].
#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class Op, class ... P> inline constexpr auto
expr(Op && op, P && ... p)
{
    return Expr<Op, std::tuple<P ...>> { std::forward<Op>(op), std::forward<P>(p) ... };
}

template <class Op, class ... A> inline constexpr auto
map(Op && op, A && ... a)
{
    return expr(std::forward<Op>(op), start(std::forward<A>(a)) ...);
}

template <class Op, class ... A> inline constexpr void
for_each(Op && op, A && ... a)
{
    ply(map(std::forward<Op>(op), std::forward<A>(a) ...));
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_EXPR
