// -*- mode: c++; coding: utf-8 -*-
/// @file optimize.hh
/// @brief Naive optimization pass over ETs.

// (c) Daniel Llorens - 2015-2018
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/small.hh"

// no real downside to this.
#ifndef RA_DO_OPT_IOTA
#define RA_DO_OPT_IOTA 1
#endif

// benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_DO_OPT_SMALLVECTOR
#define RA_DO_OPT_SMALLVECTOR 0
#endif

namespace ra {

template <class E> inline decltype(auto) constexpr optimize(E && e) { return std::forward<E>(e); }

// These are named to match & transform Expr<OPNAME, ...> later on, and used by operator+ etc.
#define DEFINE_NAMED_BINARY_OP(OP, OPNAME)                              \
    struct OPNAME                                                       \
    {                                                                   \
        template <class A, class B>                                     \
            constexpr decltype(auto)                                    \
            operator()(A && a, B && b) const { return std::forward<A>(a) OP std::forward<B>(b); } \
    };
DEFINE_NAMED_BINARY_OP(+, plus)
DEFINE_NAMED_BINARY_OP(-, minus)
DEFINE_NAMED_BINARY_OP(*, times)
DEFINE_NAMED_BINARY_OP(/, slash)
#undef DEFINE_NAMED_BINARY_OP

// TODO need something to handle the & variants...
#define ITEM(i) std::get<(i)>(e.t)

#if RA_DO_OPT_IOTA==1
// TODO iota(int)*real is not opt to iota(real) since a+a+... != n*a.
template <class X> constexpr bool iota_op = ra::is_zero_or_scalar<X> && std::numeric_limits<value_t<X>>::is_integer;

// --------------
// plus
// --------------

template <class I, class J>
requires (is_iota<I> && iota_op<J>)
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J>> && e)
{
    return iota(ITEM(0).len_, ITEM(0).i_+ITEM(1), ITEM(0).step_);
}

template <class I, class J>
requires (iota_op<I> && is_iota<J>)
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J>> && e)
{
    return iota(ITEM(1).len_, ITEM(0)+ITEM(1).i_, ITEM(1).step_);
}

template <class I, class J>
requires (is_iota<I> && is_iota<J>)
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J>> && e)
{
    RA_CHECK(ITEM(0).len_==ITEM(1).len_ && "len mismatch");
    return iota(ITEM(0).len_, ITEM(0).i_+ITEM(1).i_, ITEM(0).step_+ITEM(1).step_);
}

// --------------
// minus
// --------------

template <class I, class J>
requires (is_iota<I> && iota_op<J>)
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J>> && e)
{
    return iota(ITEM(0).len_, ITEM(0).i_-ITEM(1), ITEM(0).step_);
}

template <class I, class J>
requires (iota_op<I> && is_iota<J>)
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J>> && e)
{
    return iota(ITEM(1).len_, ITEM(0)-ITEM(1).i_, -ITEM(1).step_);
}

template <class I, class J>
requires (is_iota<I> && is_iota<J>)
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J>> && e)
{
    RA_CHECK(ITEM(0).len_==ITEM(1).len_ && "len mismatch");
    return iota(ITEM(0).len_, ITEM(0).i_-ITEM(1).i_, ITEM(0).step_-ITEM(1).step_);
}

// --------------
// times
// --------------

template <class I, class J>
requires (is_iota<I> && iota_op<J>)
inline constexpr auto optimize(Expr<ra::times, std::tuple<I, J>> && e)
{
    return iota(ITEM(0).len_, ITEM(0).i_*ITEM(1), ITEM(0).step_*ITEM(1));
}

template <class I, class J>
requires (iota_op<I> && is_iota<J>)
inline constexpr auto optimize(Expr<ra::times, std::tuple<I, J>> && e)
{
    return iota(ITEM(1).len_, ITEM(0)*ITEM(1).i_, ITEM(0)*ITEM(1).step_);
}

#endif // RA_DO_OPT_IOTA

#if RA_DO_OPT_SMALLVECTOR==1

namespace {

#if defined (__clang__)
template <class T, int N> using extvector __attribute__((ext_vector_type(N))) = T;
#else
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;
#endif
// FIXME find a way to peel qualifiers from parameter type of start(), to ignore SmallBase<SmallArray> vs SmallBase<SmallView> or const vs nonconst.
template <class A, class T, dim_t N> constexpr bool match_smallvector =
    std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::template iterator<0>>
    || std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::template const_iterator<0>>;

static_assert(match_smallvector<ra::cell_iterator_small<ra::SmallBase<ra::SmallView, double, mp::int_list<4>, mp::int_list<1>>, 0>,
                                double, 4>);
} // namespace

#define RA_OPT_SMALLVECTOR_OP(OP, NAME, T, N)                           \
    template <class A, class B>                                         \
    requires (match_smallvector<A, T, N> && match_smallvector<B, T, N>) \
    inline auto                                                         \
    optimize(ra::Expr<NAME, std::tuple<A, B>> && e)                     \
    {                                                                   \
        alignas (alignof(extvector<T, N>)) ra::Small<T, N> val;         \
        *(extvector<T, N> *)(&val) = *(extvector<T, N> *)((ITEM(0).c.p)) OP *(extvector<T, N> *)((ITEM(1).c.p)); \
        return val;                                                     \
    }
#define RA_OPT_SMALLVECTOR_OP_FUNS(T, N)      \
    RA_OPT_SMALLVECTOR_OP(+, ra::plus, T, N)  \
    RA_OPT_SMALLVECTOR_OP(-, ra::minus, T, N) \
    RA_OPT_SMALLVECTOR_OP(/, ra::slash, T, N) \
    RA_OPT_SMALLVECTOR_OP(*, ra::times, T, N)
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
