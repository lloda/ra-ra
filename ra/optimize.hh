// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Naive optimization pass over expression templates.

// (c) Daniel Llorens - 2015-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "small.hh"

namespace ra {

template <class E> constexpr decltype(auto) optimize(E && e) { return std::forward<E>(e); }

// FIXME only reduces iota exprs as op'ed on in operators.hh, not tree that is built directly, as in WithLen.
#if RA_DO_OPT_IOTA==1
// TODO iota(int)*real is not opt to iota(real) since a+a+... != n*a.
template <class X> constexpr bool iota_op = ra::is_zero_or_scalar<X> && std::numeric_limits<value_t<X>>::is_integer;

// TODO need something to handle the & variants...
#define ITEM(i) std::get<(i)>(e.t)

// --------------
// plus
// --------------

template <class I, class J> requires (is_iota<I> && iota_op<J>)
constexpr auto
optimize(Expr<std::plus<>, std::tuple<I, J>> && e)
{
    return ITEM(0).set(ITEM(0).i + ITEM(1));
}

template <class I, class J> requires (iota_op<I> && is_iota<J>)
constexpr auto
optimize(Expr<std::plus<>, std::tuple<I, J>> && e)
{
    return ITEM(1).set(ITEM(1).i + ITEM(0));
}

template <class I, class J> requires (is_iota<I> && is_iota<J>)
constexpr auto
optimize(Expr<std::plus<>, std::tuple<I, J>> && e)
{
    return iota(e.len(0), ITEM(0).i+ITEM(1).i, ITEM(0).gets()+ITEM(1).gets());
}

// --------------
// minus
// --------------

template <class I, class J> requires (is_iota<I> && iota_op<J>)
constexpr auto
optimize(Expr<std::minus<>, std::tuple<I, J>> && e)
{
    return ITEM(0).set(ITEM(0).i - ITEM(1));
}

template <class I, class J> requires (iota_op<I> && is_iota<J>)
constexpr auto
optimize(Expr<std::minus<>, std::tuple<I, J>> && e)
{
    return iota(e.len(0), ITEM(0)-ITEM(1).i, -ITEM(1).gets());
}

template <class I, class J> requires (is_iota<I> && is_iota<J>)
constexpr auto
optimize(Expr<std::minus<>, std::tuple<I, J>> && e)
{
    return iota(e.len(0), ITEM(0).i-ITEM(1).i, ITEM(0).gets()-ITEM(1).gets());
}

// --------------
// times
// --------------

template <class I, class J> requires (is_iota<I> && iota_op<J>)
constexpr auto
optimize(Expr<std::multiplies<>, std::tuple<I, J>> && e)
{
    return iota(e.len(0), ITEM(0).i*ITEM(1), ITEM(0).gets()*ITEM(1));
}

template <class I, class J> requires (iota_op<I> && is_iota<J>)
constexpr auto
optimize(Expr<std::multiplies<>, std::tuple<I, J>> && e)
{
    return iota(e.len(0), ITEM(0)*ITEM(1).i, ITEM(0)*ITEM(1).gets());
}

// --------------
// negate
// --------------

template <class I> requires (is_iota<I>)
constexpr auto
optimize(Expr<std::negate<>, std::tuple<I>> && e)
{
    return iota(e.len(0), -ITEM(0).i, -ITEM(0).gets());
}

#endif // RA_DO_OPT_IOTA

#if RA_DO_OPT_SMALLVECTOR==1

// FIXME find a way to peel qualifiers from parameter type of start(), to ignore SmallBase<SmallArray> vs SmallBase<SmallView> or const vs nonconst.
template <class A, class T, dim_t N> constexpr bool match_smallvector =
    std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::template iterator<0>>
    || std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::template const_iterator<0>>;

static_assert(match_smallvector<ra::CellSmall<ra::SmallBase<ra::SmallView, double, mp::int_list<4>, mp::int_list<1>>, 0>,
                                double, 4>);

#define RA_OPT_SMALLVECTOR_OP(OP, NAME, T, N)                           \
    template <class A, class B>                                         \
    requires (match_smallvector<A, T, N> && match_smallvector<B, T, N>) \
    constexpr auto                                                      \
    optimize(ra::Expr<NAME, std::tuple<A, B>> && e)                     \
    {                                                                   \
        alignas (alignof(extvector<T, N>)) ra::Small<T, N> val;         \
        *(extvector<T, N> *)(&val) = *(extvector<T, N> *)((ITEM(0).c.p)) OP *(extvector<T, N> *)((ITEM(1).c.p)); \
        return val;                                                     \
    }
#define RA_OPT_SMALLVECTOR_OP_FUNS(T, N)      \
    static_assert(0==alignof(ra::Small<T, N>) % alignof(extvector<T, N>)); \
    RA_OPT_SMALLVECTOR_OP(+, std::plus<>, T, N)  \
    RA_OPT_SMALLVECTOR_OP(-, std::minus<>, T, N) \
    RA_OPT_SMALLVECTOR_OP(/, std::divides<>, T, N) \
    RA_OPT_SMALLVECTOR_OP(*, std::multiplies<>, T, N)
#define RA_OPT_SMALLVECTOR_OP_SIZES(T)        \
    RA_OPT_SMALLVECTOR_OP_FUNS(T, 2)          \
    RA_OPT_SMALLVECTOR_OP_FUNS(T, 4)          \
    RA_OPT_SMALLVECTOR_OP_FUNS(T, 8)

RA_OPT_SMALLVECTOR_OP_SIZES(double)
RA_OPT_SMALLVECTOR_OP_SIZES(float)

#undef RA_OPT_SMALLVECTOR_OP_SIZES
#undef RA_OPT_SMALLVECTOR_OP_FUNS
#undef RA_OPT_SMALLVECTOR_OP_OP

#endif // RA_DO_OPT_SMALLVECTOR

#undef ITEM

} // namespace ra
