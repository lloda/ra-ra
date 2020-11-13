// -*- mode: c++; coding: utf-8 -*-
/// @file operators.hh
/// @brief Sugar for ra:: expression templates.

// (c) Daniel Llorens - 2014-2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
// FIXME Dependence on specific ra:: types should maybe be elsewhere.
#include "ra/global.hh"
#include "ra/complex.hh"
#include "ra/wrank.hh"
#include "ra/pick.hh"
#include "ra/view-ops.hh"
#include "ra/optimize.hh"

#ifndef RA_DO_OPT
  #define RA_DO_OPT 1 // enabled by default
#endif
#if RA_DO_OPT==1
  #define RA_OPT optimize
#else
  #define RA_OPT
#endif

namespace ra {

// These ra::start are needed b/c rank 0 converts to and from scalar, so ? can't pick the right (-> scalar) conversion.
template <class T, class F>
requires (ra::is_zero_or_scalar<T> && ra::is_zero_or_scalar<F>)
inline constexpr decltype(auto) where(bool const w, T && t, F && f)
{
    return w ? *(ra::start(t).flat()) : *(ra::start(f).flat());
}

template <int D, int Oa, int Ob, class A, class B>
requires (ra::is_scalar<A> && ra::is_scalar<B>)
inline constexpr auto wedge(A const & a, B const & b) { return a*b; }

template <int ... Iarg, class A> inline constexpr
decltype(auto) transpose(mp::int_list<Iarg ...>, A && a)
{
    return transpose<Iarg ...>(std::forward<A>(a));
}

namespace {

template <class A> inline constexpr
decltype(auto) FLAT(A && a)
{
    return *(ra::start(std::forward<A>(a)).flat());
}

} // namespace


// ---------------------------
// from, after APL, like (from) in guile-ploy
// TODO integrate with is_beatable shortcuts, operator() in the various array types.
// ---------------------------

template <class I>
struct index_rank_
{
    using type = mp::int_t<rank_s<I>()>;
    static_assert(type::value!=RANK_ANY, "dynamic rank unsupported");
    static_assert(size_s<I>()!=DIM_BAD, "undelimited extent subscript unsupported");
};

template <class I> using index_rank = typename index_rank_<I>::type;

template <class II, int drop, class Op> inline constexpr
decltype(auto) from_partial(Op && op)
{
    if constexpr (drop==mp::len<II>) {
        return std::forward<Op>(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, mp::int_t<0>>, mp::drop<II, drop>> {},
                     from_partial<II, drop+1>(std::forward<Op>(op)));
    }
}

// TODO we should be able to do better by slicing at each dimension, etc. But verb<> only supports rank-0 for the innermost op.
template <class A, class ... I> inline constexpr
auto from(A && a, I && ... i)
{
    if constexpr (0==sizeof...(i)) {
        return a();
    } else if constexpr (1==sizeof...(i)) {
// support dynamic rank for 1 arg only (see test in test/from.cc).
        return expr(std::forward<A>(a), start(std::forward<I>(i) ...));
    } else {
        using II = mp::map<index_rank, mp::tuple<decltype(start(std::forward<I>(i))) ...>>;
        return expr(from_partial<II, 1>(std::forward<A>(a)), start(std::forward<I>(i)) ...);
    }
}


// I considered three options for lookup.
// 1. define these in a class that ArrayIterator or Container or Slice types derive from. This was done for an old library I had (vector-ops.hh). It results in the smallest scope, but since those types are used in the definition (ra::Expr is an ArrayIterator), it requires lots of forwarding and traits:: .
// 2. raw ADL doesn't work because some ra:: types use ! != etc for different things (e.g. Flat). Possible solution: don't ever use + != == for Flat.
// 3. requires-constrained ADL is what you see here.

// --------------------------------
// Array versions of operators and functions
// --------------------------------

// We need the zero/scalar specializations because the scalar/scalar operators
// maybe be templated (e.g. complex<>), so they won't be found when an implicit
// conversion from zero->scalar is also needed. That is, without those
// specializations, ra::View<complex, 0> * complex will fail.

// These depend on OPNAME defined in optimize.hh and used there to match ET patterns.
#define DEF_NAMED_BINARY_OP(OP, OPNAME)                                 \
    template <class A, class B>                                         \
    requires (ra_pos_and_any<A, B>)                                     \
    inline constexpr auto operator OP(A && a, B && b)                   \
    {                                                                   \
        return RA_OPT(map(OPNAME(), std::forward<A>(a), std::forward<B>(b))); \
    }                                                                   \
    template <class A, class B>                                         \
    requires (ra_zero<A, B>)                                            \
    inline constexpr auto operator OP(A && a, B && b)                   \
    {                                                                   \
        return FLAT(a) OP FLAT(b);                                      \
    }
DEF_NAMED_BINARY_OP(+, plus)
DEF_NAMED_BINARY_OP(-, minus)
DEF_NAMED_BINARY_OP(*, times)
DEF_NAMED_BINARY_OP(/, slash)
#undef DEF_NAMED_BINARY_OP

#define DEF_BINARY_OP(OP)                                               \
    template <class A, class B>                                         \
    requires (ra_pos_and_any<A, B>)                                     \
    inline auto operator OP(A && a, B && b)                             \
    {                                                                   \
        return map([](auto && a, auto && b) { return a OP b; },         \
                   std::forward<A>(a), std::forward<B>(b));             \
    }                                                                   \
    template <class A, class B>                                         \
    requires (ra_zero<A, B>)                                            \
    inline auto operator OP(A && a, B && b)                             \
    {                                                                   \
        return FLAT(a) OP FLAT(b);                                      \
    }
FOR_EACH(DEF_BINARY_OP, >, <, >=, <=, <=>, ==, !=, |, &, ^)
#undef DEF_BINARY_OP

#define DEF_UNARY_OP(OP)                                                \
    template <class A>                                                  \
    requires (is_ra_pos_rank<A>)                                        \
    inline auto operator OP(A && a)                                     \
    {                                                                   \
        return map([](auto && a) { return OP a; }, std::forward<A>(a)); \
    }
FOR_EACH(DEF_UNARY_OP, !, +, -) // TODO Make + into nop.
#undef DEF_UNARY_OP

// When OP(a) isn't found from ra::, the deduction from rank(0) -> scalar doesn't work.
// TODO Cf [ref:examples/useret.cc:0].
#define DEF_NAME_OP(OP)                                                 \
    using ::OP;                                                         \
    template <class ... A>                                              \
    requires (ra_pos_and_any<A ...>)                                    \
    inline auto OP(A && ... a)                                          \
    {                                                                   \
        return map([](auto && ... a) { return OP(a ...); }, std::forward<A>(a) ...); \
    }                                                                   \
    template <class ... A>                                              \
    requires (ra_zero<A ...>)                                           \
    inline auto OP(A && ... a)                                          \
    {                                                                   \
        return OP(FLAT(a) ...);                                         \
    }
FOR_EACH(DEF_NAME_OP, rel_error, pow, xI, conj, sqr, sqrm, sqrt, cos, sin)
FOR_EACH(DEF_NAME_OP, exp, expm1, log, log1p, log10, isfinite, isnan, isinf)
FOR_EACH(DEF_NAME_OP, max, min, abs, odd, asin, acos, atan, atan2, clamp)
FOR_EACH(DEF_NAME_OP, cosh, sinh, tanh, arg)
#undef DEF_NAME_OP

#define DEF_NAME_OP(OP)                                                 \
    using ::OP;                                                         \
    template <class ... A>                                              \
    requires (ra_pos_and_any<A ...>)                                    \
    inline auto OP(A && ... a)                                          \
    {                                                                   \
        return map([](auto && ... a) -> decltype(auto) { return OP(a ...); }, std::forward<A>(a) ...); \
    }                                                                   \
    template <class ... A>                                              \
    requires (ra_zero<A ...>)                                           \
    inline decltype(auto) OP(A && ... a)                                \
    {                                                                   \
        return OP(FLAT(a) ...);                                         \
    }
FOR_EACH(DEF_NAME_OP, real_part, imag_part)
#undef DEF_NAME_OP

template <class T, class A>
inline auto cast(A && a)
{
    return map([](auto && a) { return T(a); }, std::forward<A>(a));
}

// TODO could be useful to deduce T as tuple of value_types (&).
template <class T, class ... A>
inline auto pack(A && ... a)
{
    return map([](auto && ... a) { return T { a ... }; }, std::forward<A>(a) ...);
}

// FIXME Inelegant story wrt plain array / nested array :-/
template <class A, class I>
inline auto at(A && a, I && i)
{
    return map([&a](auto && i) -> decltype(auto) { return a.at(i); }, i);
}

template <class W, class T, class F>
requires (is_ra_pos_rank<W> || is_ra_pos_rank<T> || is_ra_pos_rank<F>)
inline auto where(W && w, T && t, F && f)
{
    return pick(cast<bool>(start(std::forward<W>(w))), start(std::forward<F>(f)), start(std::forward<T>(t)));
}

template <class A, class B>
requires (ra_pos_and_any<A, B>)
inline auto operator &&(A && a, B && b)
{
    return where(std::forward<A>(a), cast<bool>(std::forward<B>(b)), false);
}
template <class A, class B>
requires (ra_pos_and_any<A, B>)
inline auto operator ||(A && a, B && b)
{
    return where(std::forward<A>(a), true, cast<bool>(std::forward<B>(b)));
}
#define DEF_SHORTCIRCUIT_BINARY_OP(OP)                                  \
    template <class A, class B>                                         \
    requires (ra_zero<A, B>)                                            \
    inline auto operator OP(A && a, B && b)                             \
    {                                                                   \
        return FLAT(a) OP FLAT(b);                                      \
    }
FOR_EACH(DEF_SHORTCIRCUIT_BINARY_OP, &&, ||);
#undef DEF_SHORTCIRCUIT_BINARY_OP

// --------------------------------
// Some whole-array reductions.
// TODO First rank reductions? Variable rank reductions?
// --------------------------------

template <class A> inline bool
any(A && a)
{
    return early(map([](bool x) { return std::make_tuple(x, x); }, std::forward<A>(a)), false);
}

template <class A> inline bool
every(A && a)
{
    return early(map([](bool x) { return std::make_tuple(!x, x); }, std::forward<A>(a)), true);
}

// FIXME variable rank? see J 'index of' (x i. y), etc.
template <class A>
inline auto index(A && a)
{
    return early(map([](auto && a, auto && i) { return std::make_tuple(bool(a), i); },
                     std::forward<A>(a), ra::iota(start(a).size(0))),
                 ra::dim_t(-1));
}

// [ma108]
template <class A, class B>
inline bool lexicographical_compare(A && a, B && b)
{
    return early(map([](auto && a, auto && b)
                     { return a==b ? std::make_tuple(false, true) : std::make_tuple(true, a<b); },
                     a, b),
                 false);
}

// FIXME only works with numeric types.
using std::min;
template <class A>
inline auto amin(A && a)
{
    using T = value_t<A>;
    T c = std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max();
    for_each([&c](auto && a) { if (a<c) { c = a; } }, a);
    return c;
}

using std::max;
template <class A>
inline auto amax(A && a)
{
    using T = value_t<A>;
    T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
    for_each([&c](auto && a) { if (c<a) { c = a; } }, a);
    return c;
}

// FIXME encapsulate this kind of reference-reduction.
// FIXME expr/ply mechanism doesn't allow partial iteration (adv then continue).
template <class A, class Less = std::less<value_t<A>>>
inline decltype(auto) refmin(A && a, Less && less = std::less<value_t<A>>())
{
    RA_CHECK(a.size()>0);
    decltype(auto) s = ra::start(a);
    auto p = &(*s.flat());
    for_each([&less, &p](auto & a) { if (less(a, *p)) { p = &a; } }, s);
    return *p;
}

template <class A, class Less = std::less<value_t<A>>>
inline decltype(auto) refmax(A && a, Less && less = std::less<value_t<A>>())
{
    RA_CHECK(a.size()>0);
    decltype(auto) s = ra::start(a);
    auto p = &(*s.flat());
    for_each([&less, &p](auto & a) { if (less(*p, a)) { p = &a; } }, s);
    return *p;
}

template <class A>
inline constexpr auto sum(A && a)
{
    value_t<A> c {};
    for_each([&c](auto && a) { c += a; }, a);
    return c;
}

template <class A>
inline constexpr auto prod(A && a)
{
    value_t<A> c(1.);
    for_each([&c](auto && a) { c *= a; }, a);
    return c;
}

template <class A> inline auto reduce_sqrm(A && a) { return sum(sqrm(a)); }

template <class A> inline auto norm2(A && a) { return std::sqrt(reduce_sqrm(a)); }

template <class A, class B>
inline auto dot(A && a, B && b)
{
    std::decay_t<decltype(FLAT(a) * FLAT(b))> c(0.);
    for_each([&c](auto && a, auto && b) { c = fma(a, b, c); }, a, b);
    return c;
}

template <class A, class B>
inline auto cdot(A && a, B && b)
{
    std::decay_t<decltype(conj(FLAT(a)) * FLAT(b))> c(0.);
    for_each([&c](auto && a, auto && b) { c = fma_conj(a, b, c); }, a, b);
    return c;
}

// --------------------
// Wedge product
// TODO Handle the simplifications dot_plus, yields_scalar, etc. just as vec::wedge does.
// --------------------

template <class A>
struct torank1
{
    using type = std::conditional_t<is_scalar<A>, Small<std::decay_t<A>, 1>, A>;
};

template <class Wedge, class Va, class Vb>
struct fromrank1
{
    using valtype = typename Wedge::template valtype<Va, Vb>;
    using type = std::conditional_t<Wedge::Nr==1, valtype, Small<valtype, Wedge::Nr>>;
};

#define DECL_WEDGE(condition)                                           \
    template <int D, int Oa, int Ob, class Va, class Vb>                \
    requires (!(is_scalar<Va> && is_scalar<Vb>))                        \
    decltype(auto) wedge(Va const & a, Vb const & b)
DECL_WEDGE(general_case)
{
    Small<std::decay_t<value_t<Va>>, size_s<Va>()> aa = a;
    Small<std::decay_t<value_t<Vb>>, size_s<Vb>()> bb = b;

    using Ua = decltype(aa);
    using Ub = decltype(bb);

    typename fromrank1<fun::Wedge<D, Oa, Ob>, Ua, Ub>::type r;

    auto & r1 = reinterpret_cast<typename torank1<decltype(r)>::type &>(r);
    auto & a1 = reinterpret_cast<typename torank1<Ua>::type const &>(aa);
    auto & b1 = reinterpret_cast<typename torank1<Ub>::type const &>(bb);
    fun::Wedge<D, Oa, Ob>::product(a1, b1, r1);

    return r;
}
#undef DECL_WEDGE

#define DECL_WEDGE(condition)                                           \
    template <int D, int Oa, int Ob, class Va, class Vb, class Vr>      \
    requires (!(is_scalar<Va> && is_scalar<Vb>))                        \
    void wedge(Va const & a, Vb const & b, Vr & r)
DECL_WEDGE(general_case)
{
    Small<std::decay_t<value_t<Va>>, size_s<Va>()> aa = a;
    Small<std::decay_t<value_t<Vb>>, size_s<Vb>()> bb = b;

    using Ua = decltype(aa);
    using Ub = decltype(bb);

    auto & r1 = reinterpret_cast<typename torank1<decltype(r)>::type &>(r);
    auto & a1 = reinterpret_cast<typename torank1<Ua>::type const &>(aa);
    auto & b1 = reinterpret_cast<typename torank1<Ub>::type const &>(bb);
    fun::Wedge<D, Oa, Ob>::product(a1, b1, r1);
}
#undef DECL_WEDGE

template <class A, class B>
requires (size_s<A>()==2 && size_s<B>()==2)
inline auto cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(FLAT(a_))>, 2> a = a_;
    Small<std::decay_t<decltype(FLAT(b_))>, 2> b = b_;
    Small<std::decay_t<decltype(FLAT(a_) * FLAT(b_))>, 1> r;
    fun::Wedge<2, 1, 1>::product(a, b, r);
    return r[0];
}

template <class A, class B>
requires (size_s<A>()==3 && size_s<B>()==3)
inline auto cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(FLAT(a_))>, 3> a = a_;
    Small<std::decay_t<decltype(FLAT(b_))>, 3> b = b_;
    Small<std::decay_t<decltype(FLAT(a_) * FLAT(b_))>, 3> r;
    fun::Wedge<3, 1, 1>::product(a, b, r);
    return r;
}

template <class V>
inline auto perp(V const & v)
{
    static_assert(v.size()==2, "dimension error");
    return Small<std::decay_t<decltype(FLAT(v))>, 2> {v[1], -v[0]};
}

template <class V, class U>
inline auto perp(V const & v, U const & n)
{
    if constexpr (is_scalar<U>) {
        static_assert(v.size()==2, "dimension error");
        return Small<std::decay_t<decltype(FLAT(v) * n)>, 2> {v[1]*n, -v[0]*n};
    } else {
        static_assert(v.size()==3, "dimension error");
        return cross(v, n);
    }
}

// --------------------
// Other whole-array ops.
// --------------------

template <class A>
requires (is_slice<A>)
inline auto normv(A const & a)
{
    return concrete(a/norm2(a));
}

template <class A>
requires (!is_slice<A> && is_ra<A>)
inline auto normv(A const & a)
{
    auto b = concrete(a);
    b /= norm2(b);
    return b;
}

// FIXME benchmark w/o allocation and do Small/Big versions if it's worth it.
template <class A, class B, class C>
inline void
gemm(A const & a, B const & b, C & c)
{
    for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })), c, a, b);
}

// FIXME branch gemm on Ryn::size_s(), but that's bugged.
#define MMTYPE decltype(from(times(), a(ra::all, 0), b(0, ra::all)))

// default for row-major x row-major. See bench-gemm.cc for variants.
template <class S, class T>
inline auto
gemm(ra::View<S, 2> const & a, ra::View<T, 2> const & b)
{
    int const M = a.size(0);
    int const N = b.size(1);
    int const K = a.size(1);
// no with_same_shape b/c cannot index 0 for type if A/B are empty
    auto c = with_shape<MMTYPE>({M, N}, decltype(a(0, 0)*b(0, 0))());
    for (int k=0; k<K; ++k) {
        c += from(times(), a(ra::all, k), b(k, ra::all));
    }
    return c;
}

// we still want the Small version to be different.
template <class A, class B>
inline ra::Small<std::decay_t<decltype(FLAT(std::declval<A>()) * FLAT(std::declval<B>()))>, A::size(0), B::size(1)>
gemm(A const & a, B const & b)
{
    constexpr int M = a.size(0);
    constexpr int N = b.size(1);
// no with_same_shape b/c cannot index 0 for type if A/B are empty
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
inline auto
gevm(A const & a, B const & b)
{
    int const M = b.size(0);
    int const N = b.size(1);
// no with_same_shape b/c cannot index 0 for type if A/B are empty
    auto c = with_shape<decltype(a[0]*b(0, ra::all))>({N}, 0);
    for (int i=0; i<M; ++i) {
        c += a[i]*b(i);
    }
    return c;
}

// FIXME a must be a view, so it doesn't work with e.g. gemv(conj(a), b).
template <class A, class B>
inline auto
gemv(A const & a, B const & b)
{
    int const M = a.size(0);
    int const N = a.size(1);
// no with_same_shape b/c cannot index 0 for type if A/B are empty
    auto c = with_shape<decltype(a(ra::all, 0)*b[0])>({M}, 0);
    for (int j=0; j<N; ++j) {
        c += a(ra::all, j) * b[j];
    }
    return c;
}

} // namespace ra

#undef RA_OPT
