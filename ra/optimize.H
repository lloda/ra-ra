
// (c) Daniel Llorens - 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file optimize.H
/// @brief Naive optimization pass over ETs.

#pragma once
#include "ra/small.H"

#ifndef RA_CHECK_BOUNDS_OPTIMIZE
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_OPTIMIZE 1
  #else
    #define RA_CHECK_BOUNDS_OPTIMIZE RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_OPTIMIZE==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

// no real downside to this.
#ifndef RA_OPTIMIZE_IOTA
#define RA_OPTIMIZE_IOTA 1
#endif

// benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_OPTIMIZE_SMALLVECTOR
#define RA_OPTIMIZE_SMALLVECTOR 0
#endif

namespace ra {

template <class E> inline decltype(auto) constexpr optimize(E && e) { return std::forward<E>(e); }

// These are named to match & transform Expr<OPNAME, ...> later on, and used by operator+ etc.
#define DEFINE_NAMED_BINARY_OP(OP, OPNAME)                              \
    struct OPNAME                                                       \
    {                                                                   \
        template <class A, class B>                                     \
        decltype(auto) operator()(A && a, B && b) { return std::forward<A>(a) OP std::forward<B>(b); } \
    };
DEFINE_NAMED_BINARY_OP(+, plus)
DEFINE_NAMED_BINARY_OP(-, minus)
DEFINE_NAMED_BINARY_OP(*, times)
DEFINE_NAMED_BINARY_OP(/, slash)
#undef DEFINE_NAMED_BINARY_OP

// TODO need something to handle the & variants...
#define ITEM(i) std::get<(i)>(e.t)

#if RA_OPTIMIZE_IOTA==1
// TODO iota(int)*real is not opt to iota(real) since a+a+... != n*a.
template <class X> constexpr bool iota_op = ra::is_zero_or_scalar<X> && std::numeric_limits<value_t<X>>::is_integer;

// --------------
// plus
// --------------

template <class I, class J, std::enable_if_t<is_iota<I> && iota_op<J>, int> =0>
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J>> && e)
{
    return iota(ITEM(0).size_, ITEM(0).org_+ITEM(1), ITEM(0).stride_);
}

template <class I, class J, std::enable_if_t<iota_op<I> && is_iota<J>, int> =0>
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J>> && e)
{
    return iota(ITEM(1).size_, ITEM(0)+ITEM(1).org_, ITEM(1).stride_);
}

template <class I, class J, std::enable_if_t<is_iota<I> && is_iota<J>, int> =0>
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J>> && e)
{
    CHECK_BOUNDS(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
    return iota(ITEM(0).size_, ITEM(0).org_+ITEM(1).org_, ITEM(0).stride_+ITEM(1).stride_);
}

// --------------
// minus
// --------------

template <class I, class J, std::enable_if_t<is_iota<I> && iota_op<J>, int> =0>
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J>> && e)
{
    return iota(ITEM(0).size_, ITEM(0).org_-ITEM(1), ITEM(0).stride_);
}

template <class I, class J, std::enable_if_t<iota_op<I> && is_iota<J>, int> =0>
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J>> && e)
{
    return iota(ITEM(1).size_, ITEM(0)-ITEM(1).org_, -ITEM(1).stride_);
}

template <class I, class J, std::enable_if_t<is_iota<I> && is_iota<J>, int> =0>
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J>> && e)
{
    CHECK_BOUNDS(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
    return iota(ITEM(0).size_, ITEM(0).org_-ITEM(1).org_, ITEM(0).stride_-ITEM(1).stride_);
}

// --------------
// times
// --------------

template <class I, class J, std::enable_if_t<is_iota<I> && iota_op<J>, int> =0>
inline constexpr auto optimize(Expr<ra::times, std::tuple<I, J>> && e)
{
    return iota(ITEM(0).size_, ITEM(0).org_*ITEM(1), ITEM(0).stride_*ITEM(1));
}

template <class I, class J, std::enable_if_t<iota_op<I> && is_iota<J>, int> =0>
inline constexpr auto optimize(Expr<ra::times, std::tuple<I, J>> && e)
{
    return iota(ITEM(1).size_, ITEM(0)*ITEM(1).org_, ITEM(0)*ITEM(1).stride_);
}

#endif // RA_OPTIMIZE_IOTA

#if RA_OPTIMIZE_SMALLVECTOR==1

namespace {

#if defined (__clang__)
template <class T, int N> using extvector __attribute__((ext_vector_type(N))) = T;
#else
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;
#endif
// FIXME find a way to peel qualifiers from parameter type of start_t, to ignore SmallBase<SmallArray> vs SmallBase<SmallView> or const vs nonconst.
template <class A, class T, dim_t N> constexpr bool match_smallvector =
    std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::template iterator<0>>
    || std::is_same_v<std::decay_t<A>, typename ra::Small<T, N>::template const_iterator<0>>;

static_assert(match_smallvector<ra::cell_iterator_small<ra::SmallBase<ra::SmallView, double, mp::int_list<4>, mp::int_list<1>>, 0>,
                                double, 4>);
}; //namespace

#define OPTIMIZE_SMALLVECTOR_OP(OP, NAME, T, N)                         \
    template <class A, class B,                                         \
              std::enable_if_t<match_smallvector<A, T, N> && match_smallvector<B, T, N>, int> =0> \
    inline auto                                                         \
    optimize(ra::Expr<NAME, std::tuple<A, B>> && e)                     \
    {                                                                   \
        alignas (alignof(extvector<T, N>)) ra::Small<T, N> val;         \
        *(extvector<T, N> *)(&val) = *(extvector<T, N> *)((ITEM(0).c.p)) OP *(extvector<T, N> *)((ITEM(1).c.p)); \
        return val;                                                     \
    }
#define OPTIMIZE_SMALLVECTOR_OP_FUNS(T, N)      \
    OPTIMIZE_SMALLVECTOR_OP(+, ra::plus, T, N)  \
    OPTIMIZE_SMALLVECTOR_OP(-, ra::minus, T, N) \
    OPTIMIZE_SMALLVECTOR_OP(*, ra::times, T, N)
#define OPTIMIZE_SMALLVECTOR_OP_SIZES(T)        \
    OPTIMIZE_SMALLVECTOR_OP_FUNS(T, 2)          \
    OPTIMIZE_SMALLVECTOR_OP_FUNS(T, 4)          \
    OPTIMIZE_SMALLVECTOR_OP_FUNS(T, 8)

OPTIMIZE_SMALLVECTOR_OP_SIZES(double)
OPTIMIZE_SMALLVECTOR_OP_SIZES(float)

#undef OPTIMIZE_SMALLVECTOR_OP_SIZES
#undef OPTIMIZE_SMALLVECTOR_OP_FUNS
#undef OPTIMIZE_SMALLVECTOR_OP_OP

#endif // RA_OPTIMIZE_SMALLVECTOR

#undef ITEM

} // namespace ra
