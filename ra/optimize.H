
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
template <class I, class Enable=void> constexpr bool is_iota = false;
template <class I> constexpr bool is_iota<I, std::enable_if_t<std::is_same_v<std::decay_t<I>, Iota<typename std::decay_t<I>::T>>>> = true;
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
    assert(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
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
    assert(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
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
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;

#define OPTIMIZE_SMALLVECTOR_OP(OP, NAME)                               \
    inline auto                                                         \
    optimize(ra::Expr<NAME,                                             \
             std::tuple<decltype(start(std::declval<ra::Small<double, 4>>())), \
                        decltype(start(std::declval<ra::Small<double, 4>>()))>> && e) \
    {                                                                   \
        ra::Small<double, 4> val;                                       \
        (extvector<double, 4> &)(val) = (extvector<double, 4> &)(*(ITEM(0).c.p)) OP (extvector<double, 4> &)(*(ITEM(1).c.p)); \
        return val;                                                     \
    }
OPTIMIZE_SMALLVECTOR_OP(+, ra::plus)
OPTIMIZE_SMALLVECTOR_OP(*, ra::times)
#undef OPTIMIZE_SMALLVECTOR_OP

#endif // RA_OPTIMIZE_SMALLVECTOR

#undef ITEM

} // namespace ra
