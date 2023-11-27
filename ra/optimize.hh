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

template <class E> constexpr decltype(auto) optimize(E && e) { return RA_FWD(e); }

// FIXME only reduces iota exprs as operated on in ra.hh (operators), not a tree like WithLen does.
#if RA_DO_OPT_IOTA==1
// TODO maybe don't opt iota(int)*real -> iota(real) since a+a+... != n*a
template <class X> concept iota_op = ra::is_zero_or_scalar<X> && std::is_arithmetic_v<value_t<X>>;

// TODO something to handle the & variants...
#define ITEM(i) std::get<(i)>(e.t)

// FIXME gets() vs p2781r2
// qualified ra::iota is necessary not to pick std::iota through ADL (test/headers.cc).

template <is_iota I, iota_op J>
constexpr auto
optimize(Expr<std::plus<>, std::tuple<I, J>> && e)
{
    return ra::iota(ITEM(0).n, ITEM(0).i+ITEM(1), ITEM(0).s);
}
template <iota_op I, is_iota J>
constexpr auto
optimize(Expr<std::plus<>, std::tuple<I, J>> && e)
{
    return ra::iota(ITEM(1).n, ITEM(0)+ITEM(1).i, ITEM(1).s);
}
template <is_iota I, is_iota J>
constexpr auto
optimize(Expr<std::plus<>, std::tuple<I, J>> && e)
{
    return ra::iota(maybe_len(e), ITEM(0).i+ITEM(1).i, ITEM(0).gets()+ITEM(1).gets());
}

template <is_iota I, iota_op J>
constexpr auto
optimize(Expr<std::minus<>, std::tuple<I, J>> && e)
{
    return ra::iota(ITEM(0).n, ITEM(0).i-ITEM(1), ITEM(0).s);
}
template <iota_op I, is_iota J>
constexpr auto
optimize(Expr<std::minus<>, std::tuple<I, J>> && e)
{
    return ra::iota(ITEM(1).n, ITEM(0)-ITEM(1).i, -ITEM(1).s);
}
template <is_iota I, is_iota J>
constexpr auto
optimize(Expr<std::minus<>, std::tuple<I, J>> && e)
{
    return ra::iota(maybe_len(e), ITEM(0).i-ITEM(1).i, ITEM(0).gets()-ITEM(1).gets());
}

template <is_iota I, iota_op J>
constexpr auto
optimize(Expr<std::multiplies<>, std::tuple<I, J>> && e)
{
    return ra::iota(ITEM(0).n, ITEM(0).i*ITEM(1), ITEM(0).gets()*ITEM(1));
}
template <iota_op I, is_iota J>
constexpr auto
optimize(Expr<std::multiplies<>, std::tuple<I, J>> && e)
{
    return ra::iota(ITEM(1).n, ITEM(0)*ITEM(1).i, ITEM(0)*ITEM(1).gets());
}

template <is_iota I>
constexpr auto
optimize(Expr<std::negate<>, std::tuple<I>> && e)
{
    return ra::iota(ITEM(0).n, -ITEM(0).i, -ITEM(0).gets());
}

#endif // RA_DO_OPT_IOTA

#if RA_DO_OPT_SMALLVECTOR==1

// FIXME can't match CellSmall directly, maybe bc N is in std::array { Dim { N, 1 } }.
template <class T, dim_t N, class A> constexpr bool match_small =
    std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::View::iterator<0>>
    || std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::ViewConst::iterator<0>>;

static_assert(match_small<double, 4, ra::CellSmall<double, ic_t<std::array { Dim { 4, 1 } }>, 0>>);

#define RA_OPT_SMALLVECTOR_OP(OP, NAME, T, N)                           \
    template <class A, class B> requires (match_small<T, N, A> && match_small<T, N, B>) \
    constexpr auto                                                      \
    optimize(ra::Expr<NAME, std::tuple<A, B>> && e)                     \
    {                                                                   \
        alignas (alignof(extvector<T, N>)) ra::Small<T, N> val;         \
        *(extvector<T, N> *)(&val) = *(extvector<T, N> *)((ITEM(0).c.cp)) OP *(extvector<T, N> *)((ITEM(1).c.cp)); \
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
FOR_EACH(RA_OPT_SMALLVECTOR_OP_SIZES, float, double)

#undef RA_OPT_SMALLVECTOR_OP_SIZES
#undef RA_OPT_SMALLVECTOR_OP_FUNS
#undef RA_OPT_SMALLVECTOR_OP_OP

#endif // RA_DO_OPT_SMALLVECTOR

#undef ITEM

} // namespace ra
