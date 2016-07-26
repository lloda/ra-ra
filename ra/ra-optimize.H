
// (c) Daniel Llorens - 2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_OPTIMIZE_H
#define RA_OPTIMIZE_H

/// @file ra-optimize.H
/// @brief Naive optimization pass over ETs.

#include "ra/ra-expr.H"
#include "ra/ra-small.H"

// no real downside to this.
#ifndef RA_OPTIMIZE_IOTA
#define RA_OPTIMIZE_IOTA 1
#endif

// benchmark shows it's not good by default; probably requires optimizing also +=, etc.
#ifndef RA_OPTIMIZE_SMALLVECTOR
#define RA_OPTIMIZE_SMALLVECTOR 0
#endif

namespace ra {

template <class E, int a=0> inline decltype(auto) optimize(E && e) { return std::forward<E>(e); }

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

// @TODO need something to handle the & variants...
#define ITEM(i) std::get<(i)>(e.t)

#if RA_OPTIMIZE_IOTA==1
#define IS_IOTA(I) std::is_same<std::decay_t<I>, Iota<typename I::T> >::value

// The second condition is to disallow Iota(3)*float, since Iota doesn't support float yet. @TODO Eventually allow.
template <class X>
constexpr bool iota_operand = ra::is_zero_or_scalar<X>
    && std::numeric_limits<std::decay_t<decltype(*(start(std::declval<X>()).flat()))> >::is_integer;

// --------------
// plus
// --------------

template <class I, class J, std::enable_if_t<IS_IOTA(I) && iota_operand<J>, int> =0>
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J> > && e)
{
    return iota(ITEM(0).size_, ITEM(0).org_+ITEM(1), ITEM(0).stride_);
}

template <class I, class J, std::enable_if_t<iota_operand<I> && IS_IOTA(J), int> =0>
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J> > && e)
{
    return iota(ITEM(1).size_, ITEM(0)+ITEM(1).org_, ITEM(1).stride_);
}

template <class I, class J, std::enable_if_t<IS_IOTA(I) && IS_IOTA(J), int> =0>
inline constexpr auto optimize(Expr<ra::plus, std::tuple<I, J> > && e)
{
    assert(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
    return iota(ITEM(0).size_, ITEM(0).org_+ITEM(1).org_, ITEM(0).stride_+ITEM(1).stride_);
}

// --------------
// minus
// --------------

template <class I, class J, std::enable_if_t<IS_IOTA(I) && iota_operand<J>, int> =0>
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J> > && e)
{
    return iota(ITEM(0).size_, ITEM(0).org_-ITEM(1), ITEM(0).stride_);
}

template <class I, class J, std::enable_if_t<iota_operand<I> && IS_IOTA(J), int> =0>
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J> > && e)
{
    return iota(ITEM(1).size_, ITEM(0)-ITEM(1).org_, -ITEM(1).stride_);
}

template <class I, class J, std::enable_if_t<IS_IOTA(I) && IS_IOTA(J), int> =0>
inline constexpr auto optimize(Expr<ra::minus, std::tuple<I, J> > && e)
{
    assert(ITEM(0).size_==ITEM(1).size_ && "size mismatch");
    return iota(ITEM(0).size_, ITEM(0).org_-ITEM(1).org_, ITEM(0).stride_-ITEM(1).stride_);
}

// --------------
// times
// --------------

template <class I, class J, std::enable_if_t<IS_IOTA(I) && iota_operand<J>, int> =0>
inline constexpr auto optimize(Expr<ra::times, std::tuple<I, J> > && e)
{
    return iota(ITEM(0).size_, ITEM(0).org_*ITEM(1), ITEM(0).stride_*ITEM(1));
}

template <class I, class J, std::enable_if_t<iota_operand<I> && IS_IOTA(J), int> =0>
inline constexpr auto optimize(Expr<ra::times, std::tuple<I, J> > && e)
{
    return iota(ITEM(1).size_, ITEM(0)*ITEM(1).org_, ITEM(0)*ITEM(1).stride_);
}

#undef IS_IOTA
#endif // RA_OPTIMIZE_IOTA

#if RA_OPTIMIZE_SMALLVECTOR==1
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;

inline auto
optimize(ra::Expr<ra::plus,
                  std::tuple<decltype(start(std::declval<ra::Small<double, 4> >())),
                             decltype(start(std::declval<ra::Small<double, 4> >()))> > && e)
{
    ra::Small<double, 4> val;
    (extvector<double, 4> &)(val) = ((extvector<double, 4> &)(*(ITEM(0).p)) + (extvector<double, 4> &)(*(ITEM(1).p)));
    return val;
}
inline auto
optimize(ra::Expr<ra::times,
                  std::tuple<decltype(start(std::declval<ra::Small<double, 4> >())),
                             decltype(start(std::declval<ra::Small<double, 4> >()))> > && e)
{
    ra::Small<double, 4> val;
    (extvector<double, 4> &)(val) = ((extvector<double, 4> &)(*(ITEM(0).p)) * (extvector<double, 4> &)(*(ITEM(1).p)));
    return val;
}
#endif // RA_OPTIMIZE_SMALLVECTOR

#undef ITEM

} // namespace ra

#endif // RA_OPTIMIZE_H
