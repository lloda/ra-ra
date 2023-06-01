// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Operator overloads for expression templates.

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "pick.hh"
#include "view-ops.hh"
#include "optimize.hh"
#include "complex.hh" // optional

#ifndef RA_DO_OPT
  #define RA_DO_OPT 1 // enabled by default
#endif
#if RA_DO_OPT==1
  #define RA_OPT optimize
#else
  #define RA_OPT
#endif


// ---------------------------
// globals FIXME do we really need these?
// ---------------------------

// Cf using std::abs, etc. in complex.hh - functions that need to work with scalars as well as with ra objects.
using ra::odd, ra::every, ra::any;

// These global versions must be available so that e.g. ra::transpose<> may be searched by ADL even when giving explicit template args. See http://stackoverflow.com/questions/9838862 .
template <class A> constexpr void transpose(ra::no_arg);
template <int A> constexpr void iter(ra::no_arg);

namespace ra {

template <int ... Iarg, class A>
constexpr decltype(auto)
transpose(mp::int_list<Iarg ...>, A && a)
{
    return transpose<Iarg ...>(std::forward<A>(a));
}


// ---------------------------
// from, after APL, like (from) in guile-ploy
// TODO integrate with is_beatable shortcuts, operator() in the various array types.
// ---------------------------

template <class I>
struct index_rank_
{
    using type = mp::int_c<rank_s<I>()>;
    static_assert(type::value!=RANK_ANY, "dynamic rank unsupported");
    static_assert(size_s<I>()!=DIM_BAD, "undelimited extent subscript unsupported");
};

template <class I> using index_rank = typename index_rank_<I>::type;

template <class II, int drop, class Op>
constexpr decltype(auto)
from_partial(Op && op)
{
    if constexpr (drop==mp::len<II>) {
        return std::forward<Op>(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, mp::int_c<0>>, mp::drop<II, drop>> {},
                     from_partial<II, drop+1>(std::forward<Op>(op)));
    }
}

// TODO we should be able to do better by slicing at each dimension, etc. But verb<> only supports rank-0 for the innermost op.
template <class A, class ... I>
constexpr auto
from(A && a, I && ... i)
{
    if constexpr (0==sizeof...(i)) {
        return a();
    } else if constexpr (1==sizeof...(i)) {
// support dynamic rank for 1 arg only (see test in test/from.cc).
        return expr(std::forward<A>(a), ra::start(std::forward<I>(i) ...));
    } else {
        using II = mp::map<index_rank, mp::tuple<decltype(ra::start(std::forward<I>(i))) ...>>;
        return expr(from_partial<II, 1>(std::forward<A>(a)), ra::start(std::forward<I>(i)) ...);
    }
}


// --------------------------------
// Array versions of operators and functions
// --------------------------------

// I considered three options for lookup.
// 1. define these in a class that Iterator or Container or Slice types derive from. This was done for an old library I had (vector-ops.hh). It results in the smallest scope, but since those types are used in the definition (ra::Expr is an Iterator), it requires lots of forwarding and traits:: .
// 2. raw ADL doesn't work because some ra:: types use ! != etc for different things (e.g. Flat). Possible solution: don't ever use + != == for Flat.
// 3. requires-constrained ADL is what you see here.

// We need the zero/scalar specializations because the scalar/scalar operators maybe be templated (e.g. complex<>), so they won't be found when an implicit conversion from zero->scalar is also needed. That is, without those specializations, ra::View<complex, 0> * complex will fail.

// These depend on OPNAME defined in optimize.hh and used there to match ET patterns.
#define DEF_NAMED_BINARY_OP(OP, OPNAME)                                 \
    template <class A, class B> requires (ra_pos_and_any<A, B>)         \
    constexpr auto                                                      \
    operator OP(A && a, B && b)                                         \
    {                                                                   \
        return RA_OPT(map(OPNAME(), std::forward<A>(a), std::forward<B>(b))); \
    }                                                                   \
    template <class A, class B> requires (ra_zero<A, B>)                \
    constexpr auto operator OP(A && a, B && b)                          \
    {                                                                   \
        return FLAT(a) OP FLAT(b);                                      \
    }
DEF_NAMED_BINARY_OP(+, plus)
DEF_NAMED_BINARY_OP(-, minus)
DEF_NAMED_BINARY_OP(*, times)
DEF_NAMED_BINARY_OP(/, slash)
#undef DEF_NAMED_BINARY_OP

#define DEF_BINARY_OP(OP)                                               \
    template <class A, class B> requires (ra_pos_and_any<A, B>)         \
    constexpr auto                                                      \
    operator OP(A && a, B && b)                                         \
    {                                                                   \
        return map([](auto && a, auto && b) { return a OP b; },         \
                   std::forward<A>(a), std::forward<B>(b));             \
    }                                                                   \
    template <class A, class B> requires (ra_zero<A, B>)                \
    constexpr auto                                                      \
    operator OP(A && a, B && b)                                         \
    {                                                                   \
        return FLAT(a) OP FLAT(b); /*  forward bug test/early.cc */     \
    }
FOR_EACH(DEF_BINARY_OP, >, <, >=, <=, <=>, ==, !=, |, &, ^)
#undef DEF_BINARY_OP

#define DEF_UNARY_OP(OP)                                                \
    template <class A> requires (ra_pos_and_any<A>)                     \
    constexpr auto                                                      \
    operator OP(A && a)                                                 \
    {                                                                   \
        return map([](auto && a) { return OP a; }, std::forward<A>(a)); \
    }
FOR_EACH(DEF_UNARY_OP, !, +, -) // TODO Make + into nop.
#undef DEF_UNARY_OP

// When OP(a) isn't found from ra::, the deduction from rank(0) -> scalar doesn't work.
// TODO Cf examples/useret.cc, test/reexported.cc
#define DEF_NAME_OP(OP)                                                 \
    using ::OP;                                                         \
    template <class ... A> requires (ra_pos_and_any<A ...>)             \
    constexpr auto                                                      \
    OP(A && ... a)                                                      \
    {                                                                   \
        return map([](auto && ... a) { return OP(a ...); }, std::forward<A>(a) ...); \
    }                                                                   \
    template <class ... A> requires (ra_zero<A ...>)                    \
    constexpr auto                                                      \
    OP(A && ... a)                                                      \
    {                                                                   \
        return OP(FLAT(a) ...);                                         \
    }
FOR_EACH(DEF_NAME_OP, rel_error, pow, xI, conj, sqr, sqrm, sqrt, cos, sin)
FOR_EACH(DEF_NAME_OP, exp, expm1, log, log1p, log10, isfinite, isnan, isinf)
FOR_EACH(DEF_NAME_OP, max, min, abs, odd, asin, acos, atan, atan2, clamp)
FOR_EACH(DEF_NAME_OP, cosh, sinh, tanh, arg, lerp)
#undef DEF_NAME_OP

#define DEF_NAME_OP_REF(OP)                                             \
    using ::OP;                                                         \
    template <class ... A> requires (ra_pos_and_any<A ...>)             \
    constexpr auto OP(A && ... a)                                       \
    {                                                                   \
        return map([](auto && ... a) -> decltype(auto) { return OP(a ...); }, std::forward<A>(a) ...); \
    }                                                                   \
    template <class ... A> requires (ra_zero<A ...>)                    \
    constexpr decltype(auto) OP(A && ... a)                             \
    {                                                                   \
        return OP(FLAT(a) ...);                                         \
    }
FOR_EACH(DEF_NAME_OP_REF, real_part, imag_part)
#undef DEF_NAME_OP_REF

template <class T, class A>
constexpr auto cast(A && a)
{
    return map([](auto && b) { return T(b); }, std::forward<A>(a));
}

// TODO could be useful to deduce T as tuple of value_types (&).
template <class T, class ... A>
constexpr auto pack(A && ... a)
{
    return map([](auto && ... a) { return T { a ... }; }, std::forward<A>(a) ...);
}

// FIXME needs a nested array for I, which is ugly.
template <class A, class I>
constexpr auto at(A && a, I && i)
{
    return map([a = std::tuple<A>(std::forward<A>(a))]
               (auto && i) -> decltype(auto) { return std::get<0>(a).at(i); }, i);
}


// --------------------------------
// selection or shorcutting
// --------------------------------

// These ra::start are needed bc rank 0 converts to and from scalar, so ? can't pick the right (-> scalar) conversion.
template <class T, class F>
requires (ra::is_zero_or_scalar<T> && ra::is_zero_or_scalar<F>)
constexpr decltype(auto)
where(bool const w, T && t, F && f)
{
    return w ? *(ra::start(t).flat()) : *(ra::start(f).flat());
}

template <class W, class T, class F>
requires (ra_pos_and_any<W, T, F>)
constexpr auto
where(W && w, T && t, F && f)
{
    return pick(cast<bool>(ra::start(std::forward<W>(w))),
                ra::start(std::forward<F>(f)),
                ra::start(std::forward<T>(t)));
}

// catch all for non-ra types.
template <class T, class F>
requires (!(ra_pos_and_any<T, F>) && !(ra::is_zero_or_scalar<T> && ra::is_zero_or_scalar<F>))
constexpr decltype(auto)
where(bool const w, T && t, F && f)
{
    return w ? t : f;
}

template <class A, class B>
requires (ra_pos_and_any<A, B>)
constexpr auto operator &&(A && a, B && b)
{
    return where(std::forward<A>(a), cast<bool>(std::forward<B>(b)), false);
}
template <class A, class B>
requires (ra_pos_and_any<A, B>)
constexpr auto operator ||(A && a, B && b)
{
    return where(std::forward<A>(a), true, cast<bool>(std::forward<B>(b)));
}
#define DEF_SHORTCIRCUIT_BINARY_OP(OP)                                  \
    template <class A, class B>                                         \
    requires (ra_zero<A, B>)                                            \
    constexpr auto operator OP(A && a, B && b)                             \
    {                                                                   \
        return FLAT(a) OP FLAT(b);                                      \
    }
FOR_EACH(DEF_SHORTCIRCUIT_BINARY_OP, &&, ||);
#undef DEF_SHORTCIRCUIT_BINARY_OP


// --------------------------------
// Some whole-array reductions.
// TODO First rank reductions? Variable rank reductions?
// --------------------------------

template <class A> constexpr bool
any(A && a)
{
    return early(map([](bool x) { return std::make_tuple(x, x); }, std::forward<A>(a)), false);
}

template <class A>
constexpr bool
every(A && a)
{
    return early(map([](bool x) { return std::make_tuple(!x, x); }, std::forward<A>(a)), true);
}

// FIXME variable rank? see J 'index of' (x i. y), etc.
template <class A>
constexpr auto
index(A && a)
{
    return early(map([](auto && a, auto && i) { return std::make_tuple(bool(a), i); },
                     std::forward<A>(a), ra::iota(ra::start(a).len(0))),
                 ra::dim_t(-1));
}

// [ma108]
template <class A, class B>
constexpr bool
lexicographical_compare(A && a, B && b)
{
    return early(map([](auto && a, auto && b)
                     { return a==b ? std::make_tuple(false, true) : std::make_tuple(true, a<b); },
                     a, b),
                 false);
}

// FIXME only works with numeric types.
template <class A>
constexpr auto
amin(A && a)
{
    using std::min;
    using T = value_t<A>;
    T c = std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max();
    for_each([&c](auto && a) { if (a<c) { c = a; } }, a);
    return c;
}

template <class A>
constexpr auto
amax(A && a)
{
    using std::max;
    using T = value_t<A>;
    T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
    for_each([&c](auto && a) { if (c<a) { c = a; } }, a);
    return c;
}

// FIXME encapsulate this kind of reference-reduction.
// FIXME expr/ply mechanism doesn't allow partial iteration (adv then continue).
template <class A, class Less = std::less<value_t<A>>>
constexpr decltype(auto)
refmin(A && a, Less && less = std::less<value_t<A>>())
{
    RA_CHECK(a.size()>0);
    decltype(auto) s = ra::start(a);
    auto p = &(*s.flat());
    for_each([&less, &p](auto & a) { if (less(a, *p)) { p = &a; } }, s);
    return *p;
}

template <class A, class Less = std::less<value_t<A>>>
constexpr decltype(auto)
refmax(A && a, Less && less = std::less<value_t<A>>())
{
    RA_CHECK(a.size()>0);
    decltype(auto) s = ra::start(a);
    auto p = &(*s.flat());
    for_each([&less, &p](auto & a) { if (less(*p, a)) { p = &a; } }, s);
    return *p;
}

template <class A>
constexpr auto
sum(A && a)
{
    concrete_type<value_t<A>> c {};
    for_each([&c](auto && a) { c += a; }, a);
    return c;
}

template <class A>
constexpr auto
prod(A && a)
{
    concrete_type<value_t<A>> c(1.);
    for_each([&c](auto && a) { c *= a; }, a);
    return c;
}

template <class A> constexpr auto reduce_sqrm(A && a) { return sum(sqrm(a)); }
template <class A> constexpr auto norm2(A && a) { return std::sqrt(reduce_sqrm(a)); }

template <class A, class B>
constexpr auto
dot(A && a, B && b)
{
    std::decay_t<decltype(FLAT(a) * FLAT(b))> c(0.);
    for_each([&c](auto && a, auto && b)
             {
#ifdef FP_FAST_FMA
                 c = fma(a, b, c);
#else
                 c += a*b;
#endif
             }, a, b);
    return c;
}

template <class A, class B>
constexpr auto
cdot(A && a, B && b)
{
    std::decay_t<decltype(conj(FLAT(a)) * FLAT(b))> c(0.);
    for_each([&c](auto && a, auto && b)
             {
#ifdef FP_FAST_FMA
                 c = fma_conj(a, b, c);
#else
                 c += conj(a)*b;
#endif
             }, a, b);
    return c;
}


// --------------------
// Other whole-array ops.
// --------------------

template <class A>
constexpr auto
normv(A const & a)
{
    auto b = concrete(a);
    b /= norm2(b);
    return b;
}

// FIXME benchmark w/o allocation and do Small/Big versions if it's worth it.
template <class A, class B, class C>
constexpr void
gemm(A const & a, B const & b, C & c)
{
    for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })), c, a, b);
}

#define MMTYPE decltype(from(times(), a(ra::all, 0), b(0)))

// default for row-major x row-major. See bench-gemm.cc for variants.
template <class S, class T>
constexpr auto
gemm(ra::View<S, 2> const & a, ra::View<T, 2> const & b)
{
    int const M = a.len(0);
    int const N = b.len(1);
    int const K = a.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<MMTYPE>({M, N}, decltype(a(0, 0)*b(0, 0))());
    for (int k=0; k<K; ++k) {
        c += from(times(), a(ra::all, k), b(k));
    }
    return c;
}

// we still want the Small version to be different.
template <class A, class B>
constexpr ra::Small<std::decay_t<decltype(FLAT(std::declval<A>()) * FLAT(std::declval<B>()))>, A::len(0), B::len(1)>
gemm(A const & a, B const & b)
{
    constexpr int M = a.len(0);
    constexpr int N = b.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<MMTYPE>({M, N}, ra::none);
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(ra::all, j));
        }
    }
    return c;
}

#undef MMTYPE

template <class A, class B>
constexpr auto
gevm(A const & a, B const & b)
{
    int const M = b.len(0);
    int const N = b.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<decltype(a[0]*b(0))>({N}, 0);
    for (int i=0; i<M; ++i) {
        c += a[i]*b(i);
    }
    return c;
}

// FIXME a must be a view, so it doesn't work with e.g. gemv(conj(a), b).
template <class A, class B>
constexpr auto
gemv(A const & a, B const & b)
{
    int const M = a.len(0);
    int const N = a.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<decltype(a(ra::all, 0)*b[0])>({M}, 0);
    for (int j=0; j<N; ++j) {
        c += a(ra::all, j) * b[j];
    }
    return c;
}

} // namespace ra

#undef RA_OPT
