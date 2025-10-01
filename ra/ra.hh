// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Operator overloads for expression templates. Root header.

// (c) Daniel Llorens - 2014-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "arrays.hh"
#include <cmath>
#include <complex>

#ifndef RA_OPT
  #define RA_OPT opt
#endif

#if defined(RA_FMA)
#elif defined(FP_FAST_FMA)
  #define RA_FMA FP_FAST_FMA
#else
  #define RA_FMA 0
#endif

// ADL with explicit template args. See http://stackoverflow.com/questions/9838862.
template <int A> constexpr void iter(ra::noarg);
template <class T> constexpr void cast(ra::noarg);


// ---------------------------
// scalar overloads
// ---------------------------

// abs() needs no qualifying for ra:: types (ADL), shouldn't on pods either. std::max/min are special, see DEF_NAME.
// FIXME let user decide?
using std::max, std::min, std::abs, std::fma, std::sqrt, std::pow, std::exp, std::swap,
      std::isfinite, std::isinf, std::isnan, std::clamp, std::lerp, std::conj, std::expm1;

#define FOR_FLOAT(T)                            \
    constexpr T conj(T x) { return x; }         \
    FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

#define FOR_FLOAT(R)                                                    \
    constexpr std::complex<R>                                           \
    fma(std::complex<R> const & a, std::complex<R> const & b, std::complex<R> const & c) \
    {                                                                   \
        return std::complex<R>(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())), \
                               fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    constexpr R                                                         \
    fma_sqrm(std::complex<R> const & a, R const & c)                    \
    {                                                                   \
        return fma(a.real(), a.real(), fma(a.imag(), a.imag(), c));     \
    }                                                                   \
    constexpr bool isfinite(std::complex<R> z) { return isfinite(z.real()) && isfinite(z.imag()); } \
    constexpr bool isnan(std::complex<R> z)    { return isnan(z.real()) || isnan(z.imag()); } \
    constexpr bool isinf(std::complex<R> z)    { return (isinf(z.real()) || isinf(z.imag())) && !isnan(z); }
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

namespace ra {

constexpr bool odd(unsigned int N) { return N & 1; }

// As array op; special definitions for rank 0.
template <class T> constexpr bool ra_is_real = std::numeric_limits<T>::is_integer || std::is_floating_point_v<T>;
template <class T> requires (ra_is_real<T>) constexpr T amax(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T amin(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T sqr(T const & x)  { return x*x; }

#define RA_FOR_TYPES(T)                                                 \
    constexpr T arg(T x)                { return T(0); }                \
    constexpr T conj(T x)               { return x; }                   \
    constexpr T mul_conj(T x, T y)      { return x*y; }                 \
    constexpr T sqrm(T x)               { return sqr(x); }              \
    constexpr T sqrm(T x, T y)          { return sqr(x-y); }            \
    constexpr T dot(T x, T y)           { return x*y; }                 \
    constexpr T fma_conj(T a, T b, T c) { return fma(a, b, c); }        \
    constexpr T norm2(T x)              { return std::abs(x); }         \
    constexpr T norm2(T x, T y)         { return std::abs(x-y); }       \
    constexpr T rel_error(T a, T b)     { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    constexpr T const & real_part(T const & x) { return x; }            \
    constexpr T & real_part(T & x)      { return x; }                   \
    constexpr T imag_part(T x)          { return T(0); }
FOR_EACH(RA_FOR_TYPES, float, double)
#undef RA_FOR_TYPES

// FIXME few still inline should eventually be constexpr.
#define RA_FOR_TYPES(R, C)                                              \
    inline R arg(C x)                  { return std::arg(x); }          \
    constexpr C sqr(C x)               { return x*x; }                  \
    constexpr C dot(C x, C y)          { return x*y; }                  \
    constexpr C xi(R x)                { return C(0, x); }              \
    constexpr C xi(C z)                { return C(-z.imag(), z.real()); } \
    constexpr R real_part(C const & z) { return z.real(); }             \
    constexpr R imag_part(C const & z) { return z.imag(); }             \
    constexpr R & real_part(C & z)     { return std::bit_cast<R *>(&z)[0]; } \
    constexpr R & imag_part(C & z)     { return std::bit_cast<R *>(&z)[1]; } \
    constexpr R sqrm(C x)              { return sqr(x.real())+sqr(x.imag()); } \
    constexpr R sqrm(C x, C y)         { return sqr(x.real()-y.real())+sqr(x.imag()-y.imag()); } \
    constexpr R norm2(C x)             { return hypot(x.real(), x.imag()); } \
    constexpr R norm2(C x, C y)        { return sqrt(sqrm(x, y)); }     \
    constexpr R rel_error(C a, C b)    { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    constexpr C fma_conj(C const & a, C const & b, C const & c)         \
    { /* conj(a) * b + c */                                             \
        return C(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())), \
                 fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    constexpr C mul_conj(C const & a, C const & b)                      \
    { /* conj(a) * b */                                                 \
        return C(a.real()*b.real()+a.imag()*b.imag(),                   \
                 a.real()*b.imag()-a.imag()*b.real());                  \
    }
RA_FOR_TYPES(float, std::complex<float>)
RA_FOR_TYPES(double, std::complex<double>)
#undef RA_FOR_TYPES

template <class T> constexpr bool is_scalar_def<std::complex<T>> = true;


// --------------------------------
// optimization pass over expression templates.
// --------------------------------

constexpr decltype(auto) opt(auto && e) { return RA_FW(e); }

// FIXME only reduces iota exprs as operated on in ra.hh (operators), not a tree like wlen() does.
template <class X> concept iota_op = ra::is_ra_0<X> && std::is_integral_v<ncvalue_t<X>>;

// qualified ra::iota is necessary to avoid ADLing to std::iota (test/headers.cc).
// FIXME p2781r2, handle & variants, handle view iota.

#define RA0 std::get<(0)>(e.t)
#define RA1 std::get<(1)>(e.t)
#define DEF_IOTA_BINOP(TI, TJ, OP, VAL) \
    template <TI I, TJ J> constexpr auto opt(Map<OP, std::tuple<I, J>> && e) { return VAL; }

DEF_IOTA_BINOP(is_iota, iota_op, std::plus<>, ra::iota(RA0.n, RA0.cp.i+RA1, RA0.s))
DEF_IOTA_BINOP(iota_op, is_iota, std::plus<>, ra::iota(RA1.n, RA0+RA1.cp.i, RA1.s))
DEF_IOTA_BINOP(is_iota, is_iota, std::plus<>, ra::iota(clen(e, ic<0>), RA0.cp.i+RA1.cp.i, cadd(RA0.s, RA1.s)))
DEF_IOTA_BINOP(is_iota, iota_op, std::minus<>, ra::iota(RA0.n, RA0.cp.i-RA1, RA0.s))
DEF_IOTA_BINOP(iota_op, is_iota, std::minus<>, ra::iota(RA1.n, RA0-RA1.cp.i, csub(ic<0>, RA1.s)))
DEF_IOTA_BINOP(is_iota, is_iota, std::minus<>, ra::iota(clen(e, ic<0>), RA0.cp.i-RA1.cp.i, csub(RA0.s, RA1.s)))
DEF_IOTA_BINOP(is_iota, iota_op, std::multiplies<>, ra::iota(RA0.n, RA0.cp.i*RA1, RA0.s*RA1))
DEF_IOTA_BINOP(iota_op, is_iota, std::multiplies<>, ra::iota(RA1.n, RA0*RA1.cp.i, RA0*RA1.s))
#undef DEF_IOTA_BINOP

template <is_iota I> constexpr auto
opt(Map<std::negate<>, std::tuple<I>> && e) { return ra::iota(RA0.n, -RA0.cp.i, csub(ic<0>, RA0.s)); }

#if RA_OPT_SMALLVECTOR==1
template <class T, dim_t N, class A> constexpr bool match_small =
    std::is_same_v<std::decay_t<A>, Cell<T *, ic_t<std::array {Dim(N, 1)}>, ic_t<0>>>
    || std::is_same_v<std::decay_t<A>, Cell<T const *, ic_t<std::array {Dim(N, 1)}>, ic_t<0>>>;

#define RA_OPT_SMALLVECTOR_OP(OP, NAME, T, N)                           \
    template <class A, class B> requires (match_small<T, N, A> && match_small<T, N, B>) \
    constexpr auto opt(Map<NAME, std::tuple<A, B>> && e)           \
    {                                                                   \
        alignas (alignof(extvector<T, N>)) ra::Small<T, N> val;         \
        *(extvector<T, N> *)(&val) = *(extvector<T, N> *)((RA0.c.cp)) OP *(extvector<T, N> *)((RA1.c.cp)); \
        return val;                                                     \
    }
#define RA_OPT_SMALLVECTOR_OP_FUNS(T, N)                                \
    static_assert(0==alignof(ra::Small<T, N>) % alignof(extvector<T, N>)); \
    RA_OPT_SMALLVECTOR_OP(+, std::plus<>, T, N)                         \
    RA_OPT_SMALLVECTOR_OP(-, std::minus<>, T, N)                        \
    RA_OPT_SMALLVECTOR_OP(/, std::divides<>, T, N)                      \
    RA_OPT_SMALLVECTOR_OP(*, std::multiplies<>, T, N)
#define RA_OPT_SMALLVECTOR_OP_SIZES(T)        \
    RA_OPT_SMALLVECTOR_OP_FUNS(T, 2)          \
    RA_OPT_SMALLVECTOR_OP_FUNS(T, 4)          \
    RA_OPT_SMALLVECTOR_OP_FUNS(T, 8)
FOR_EACH(RA_OPT_SMALLVECTOR_OP_SIZES, float, double)

#undef RA_OPT_SMALLVECTOR_OP_SIZES
#undef RA_OPT_SMALLVECTOR_OP_FUNS
#undef RA_OPT_SMALLVECTOR_OP_OP
#endif // RA_OPT_SMALLVECTOR
#undef RA1
#undef RA0


// --------------------------------
// array versions of operators and functions
// --------------------------------

// We need zero/scalar specializations because the scalar/scalar operators maybe be templated (e.g. complex<>), so they won't be found when an implicit scalar conversion is also needed, and e.g. ra::View<complex, 0> * complex would fail.
#define DEF_BINOP(OP, OPNAME)                                 \
    template <class A, class B> requires (tomap<A, B>) constexpr auto   \
        operator OP(A && a, B && b) { return RA_OPT(map(OPNAME(), RA_FW(a), RA_FW(b))); } \
    template <class A, class B> requires (toreduce<A, B>) constexpr auto \
        operator OP(A && a, B && b) { return VAL(RA_FW(a)) OP VAL(RA_FW(b)); }

DEF_BINOP(+, std::plus<>)     DEF_BINOP(-, std::minus<>)      DEF_BINOP(*, std::multiplies<>)
DEF_BINOP(/, std::divides<>)  DEF_BINOP(==, std::equal_to<>)  DEF_BINOP(<=, std::less_equal<>)
DEF_BINOP(<, std::less<>)     DEF_BINOP(>, std::greater<>)    DEF_BINOP(>=, std::greater_equal<>)
DEF_BINOP(|, std::bit_or<>)   DEF_BINOP(&, std::bit_and<>)    DEF_BINOP(!=, std::not_equal_to<>)
DEF_BINOP(^, std::bit_xor<>)  DEF_BINOP(%, std::modulus<>)    DEF_BINOP(<=>, std::compare_three_way)
#undef DEF_BINOP

// FIXME address sanitizer complains in bench-optimize.cc if we use std::identity. Maybe false positive
struct unaryplus
{
    template <class T> constexpr static auto operator()(T && t) noexcept { return RA_FW(t); }
};

#define DEF_UNOP(OP, OPNAME)                                      \
    template <class A> requires (tomap<A>) constexpr auto               \
        operator OP(A && a) { return RA_OPT(map(OPNAME(), RA_FW(a))); } \
    template <class A> requires (toreduce<A>) constexpr auto            \
        operator OP(A && a) { return OP VAL(RA_FW(a)); }

DEF_UNOP(+, unaryplus)
DEF_UNOP(-, std::negate<>)
DEF_UNOP(!, std::logical_not<>)
#undef DEF_UNOP

// if OP(a) isn't found in ra::, deduction rank(0) -> scalar doesn't work. TODO Cf useret.cc, reexported.cc
#define DEF_NAME(OP)                                                    \
    template <class ... A> requires (tomap<A ...>) constexpr auto       \
        OP(A && ... a) { return map([](auto && ... a) -> decltype(auto) { return OP(RA_FW(a) ...); }, RA_FW(a) ...); } \
    template <class ... A> requires (toreduce<A ...>) constexpr decltype(auto) \
        OP(A && ... a) { return OP(VAL(RA_FW(a)) ...); }
#define DEF_FWD(QUALIFIED_OP, OP)                                       \
    template <class ... A> /* requires neither */ constexpr decltype(auto) \
        OP(A && ... a) { return QUALIFIED_OP(RA_FW(a) ...); }          \
    DEF_NAME(OP)
#define DEF_USING(QUALIFIED_OP, OP)             \
    using QUALIFIED_OP;                         \
    DEF_NAME(OP)

FOR_EACH(DEF_NAME, odd, arg, sqr, sqrm, real_part, imag_part, xi, rel_error)

// can't DEF_USING bc std::max will gobble ra:: objects if passed by const & (!)
// FIXME define own global max/min overloads for basic types. std::max seems too much of a special case to be usinged.
#define DEF_GLOBAL(f) DEF_FWD(::f, f)
FOR_EACH(DEF_GLOBAL, max, min)
#undef DEF_GLOBAL

// don't use DEF_FWD for these bc we want to allow ADL, e.g. for exp(dual).
#define DEF_GLOBAL(f) DEF_USING(::f, f)
FOR_EACH(DEF_GLOBAL, pow, conj, sqrt, exp, expm1, log, log1p, log10, isfinite, isnan, isinf, atan2)
FOR_EACH(DEF_GLOBAL, abs, sin, cos, tan, sinh, cosh, tanh, asin, acos, atan, clamp, lerp)
FOR_EACH(DEF_GLOBAL, fma)
#undef DEF_GLOBAL

#undef DEF_USING
#undef DEF_FWD
#undef DEF_NAME

template <class T> constexpr auto
cast(auto && a)
{
    return map([](auto && b) -> decltype(auto) { return T(b); }, RA_FW(a));
}

template <class T> constexpr auto
pack(auto && ... a)
{
    return map([](auto && ... a){ return T { RA_FW(a) ... }; }, RA_FW(a) ...);
}

template <class A> constexpr decltype(auto)
at(A && a, auto const & i) requires (Slice<std::decay_t<A>> || Iterator<std::decay_t<A>>)
{
    if constexpr (0==rank_s<decltype(VAL(i))>()) {
        return a.at(i);
    } else {
        return map([a=std::tuple<A>(RA_FW(a))](auto && i) -> decltype(auto) { return at(std::get<0>(a), i); }, RA_FW(i));
    }
}

template <Slice A> constexpr decltype(auto)
at_view(A && a, auto && i)
{
    if constexpr (0==rank_s<decltype(VAL(i))>()) {
// can't say 'frame rank 0' so -size wouldn't work. FIXME What about ra::len
        if constexpr (constexpr rank_t cr = rank_diff(rank_s(a), ra::size_s(i)); ANY==cr) {
            return a.template iter(rank(a)-ra::size(i)).at(i);
        } else {
            return a.template iter<cr>().at(i);
        }
    } else {
        return map([a=std::tuple<A>(RA_FW(a))](auto && i) -> decltype(auto) { return at_view(std::get<0>(a), i); }, RA_FW(i));
    }
}


// --------------------------------
// selection / shortcutting
// --------------------------------

template <class T, class F> requires (toreduce<T, F>)
constexpr decltype(auto)
where(bool const w, T && t, F && f) { return w ? VAL(t) : VAL(f); }

template <class W, class T, class F> requires (tomap<W, T, F>)
constexpr auto
where(W && w, T && t, F && f) { return pick(cast<bool>(RA_FW(w)), RA_FW(f), RA_FW(t)); }

// catch all for non-ra types.
template <class T, class F> requires (!(tomap<T, F>) && !(toreduce<T, F>))
constexpr decltype(auto)
where(bool const w, T && t, F && f) { return w ? t : f; }

template <class A, class B> requires (tomap<A, B>)
constexpr auto
operator &&(A && a, B && b) { return where(RA_FW(a), cast<bool>(RA_FW(b)), false); }

template <class A, class B> requires (tomap<A, B>)
constexpr auto
operator ||(A && a, B && b) { return where(RA_FW(a), true, cast<bool>(RA_FW(b))); }

#define DEF_SHORTCIRCUIT_BINOP(OP)                                  \
    template <class A, class B> requires (toreduce<A, B>)               \
    constexpr auto operator OP(A && a, B && b) { return VAL(a) OP VAL(b);  }
FOR_EACH(DEF_SHORTCIRCUIT_BINOP, &&, ||)
#undef DEF_SHORTCIRCUIT_BINOP


// --------------------------------
// whole-array ops. TODO First/variable rank reductions? FIXME C++23 and_then/or_else/etc
// --------------------------------

constexpr bool
any(auto && a)
{
    return early(map([](bool x){ return x ? std::make_optional(true) : std::nullopt; }, RA_FW(a)), false);
}

constexpr bool
every(auto && a)
{
    return early(map([](bool x){ return !x ? std::make_optional(false) : std::nullopt; }, RA_FW(a)), true);
}

// FIXME variable rank? see J 'index of' (x i. y), etc.
constexpr dim_t
index(auto && a)
{
    return early(map([](auto && a, auto && i){ return bool(a) ? std::make_optional(i) : std::nullopt; },
                     RA_FW(a), ra::iota(ra::start(a).len(0))),
                 ra::dim_t(-1));
}

constexpr bool
lexical_compare(auto && a, auto && b)
{
    return early(map([](auto && a, auto && b){ return a==b ? std::nullopt : std::make_optional(a<b); },
                     RA_FW(a), RA_FW(b)),
                 false);
}

constexpr auto
amin(auto && a)
{
    using std::min, std::numeric_limits;
    using T = ncvalue_t<decltype(a)>;
    T c = numeric_limits<T>::has_infinity ? numeric_limits<T>::infinity() : numeric_limits<T>::max();
    for_each([&c](auto && a){ if (a<c) { c=a; } }, a);
    return c;
}

constexpr auto
amax(auto && a)
{
    using std::max, std::numeric_limits;
    using T = ncvalue_t<decltype(a)>;
    T c = numeric_limits<T>::has_infinity ? -numeric_limits<T>::infinity() : numeric_limits<T>::lowest();
    for_each([&c](auto && a){ if (c<a) { c=a; } }, a);
    return c;
}

// FIXME encapsulate this kind of reference-reduction.
// FIXME ply mechanism doesn't allow partial iteration (adv then continue).
template <class A, class Less = std::less<ncvalue_t<A>>>
constexpr decltype(auto)
refmin(A && a, Less && less = {})
{
    RA_CK(a.size()>0, "refmin requires nonempty argument.");
    decltype(auto) s = ra::start(a);
    auto p = &(*s);
    for_each([&less, &p](auto & a){ if (less(a, *p)) { p=&a; } }, s);
    return *p;
}

template <class A, class Less = std::less<ncvalue_t<A>>>
constexpr decltype(auto)
refmax(A && a, Less && less = {})
{
    RA_CK(a.size()>0, "refmax requires nonempty argument.");
    decltype(auto) s = ra::start(a);
    auto p = &(*s);
    for_each([&less, &p](auto & a){ if (less(*p, a)) { p=&a; } }, s);
    return *p;
}

constexpr auto
sum(auto && a)
{
    auto c = concrete_type<ncvalue_t<decltype(a)>>(0);
    for_each([&c](auto && a){ c+=a; }, a);
    return c;
}

constexpr auto
prod(auto && a)
{
    auto c = concrete_type<ncvalue_t<decltype(a)>>(1);
    for_each([&c](auto && a){ c*=a; }, a);
    return c;
}

constexpr void maybe_fma(auto && a, auto && b, auto & c) { if constexpr (RA_FMA) c=fma(a, b, c); else c+=a*b; }
constexpr void maybe_fma_conj(auto && a, auto && b, auto & c) { if constexpr (RA_FMA) c=fma_conj(a, b, c); else c+=conj(a)*b; }
constexpr void maybe_fma_sqrm(auto && a, auto & c) { if constexpr (RA_FMA) c=fma_sqrm(a, c); else c+=sqrm(a); }

constexpr auto
dot(auto && a, auto && b)
{
    std::decay_t<decltype(VAL(a) * VAL(b))> c(0.);
    for_each([&c](auto && a, auto && b){ maybe_fma(a, b, c); }, RA_FW(a), RA_FW(b));
    return c;
}

constexpr auto
cdot(auto && a, auto && b)
{
    std::decay_t<decltype(conj(VAL(a)) * VAL(b))> c(0.);
    for_each([&c](auto && a, auto && b){ maybe_fma_conj(a, b, c); }, RA_FW(a), RA_FW(b));
    return c;
}

constexpr auto
reduce_sqrm(auto && a)
{
    std::decay_t<decltype(sqrm(VAL(a)))> c(0.);
    for_each([&c](auto && a){ maybe_fma_sqrm(a, c); }, RA_FW(a));
    return c;
}

constexpr auto
norm2(auto && a)
{
    return std::sqrt(reduce_sqrm(a));
}

constexpr auto
normv(auto const & a)
{
    auto b = concrete(a);
    return b /= norm2(b);
}

// FIXME benchmark w/o allocation and do Small/Big versions if it's worth it (see bench-gemm.cc)

constexpr void
gemm(auto const & a, auto const & b, auto & c)
{
    dim_t K=a.len(1);
    for (int k=0; k<K; ++k) {
        c += from(std::multiplies<>(), a(all, k), b(k)); // FIXME fma
    }
}

constexpr auto
gemm(auto const & a, auto const & b)
{
    dim_t M=a.len(0), N=b.len(1);
    using T = decltype(VAL(a)*VAL(b));
    using MMTYPE = decltype(from(std::multiplies<>(), a(all, 0), b(0)));
    auto c = with_shape<MMTYPE>({M, N}, T());
    gemm(a, b, c);
    return c;
}

constexpr auto
gevm(auto const & a, auto const & b)
{
    dim_t M=b.len(0), N=b.len(1);
    using T = decltype(VAL(a)*VAL(b));
    auto c = with_shape<decltype(a[0]*b(0))>({N}, T());
    for (int i=0; i<M; ++i) {
        maybe_fma(a[i], b(i), c);
    }
    return c;
}

// FIXME a must be a view, so it doesn't work with e.g. gemv(conj(a), b).
constexpr auto
gemv(auto const & a, auto const & b)
{
    dim_t M=a.len(0), N=a.len(1);
    using T = decltype(VAL(a)*VAL(b));
    auto c = with_shape<decltype(a(all, 0)*b[0])>({M}, T());
    for (int j=0; j<N; ++j) {
        maybe_fma(a(all, j), b[j], c);
    }
    return c;
}


// --------------------
// wedge product and cross product. FIXME
// --------------------

namespace mp {

template <class P, class Plist>
struct findcomb
{
    template <class A> using match = ic_t<0 != permsign<P, A>::value>;
    using ii = indexif<Plist, match>;
    constexpr static int where = ii::value;
    constexpr static int sign = (where>=0) ? permsign<P, typename ii::type>::value : 0;
};

// Combination aC complementary to C wrt [0, 1, ... Dim-1], permuted so [C, aC] has the sign of [0, 1, ... Dim-1].
template <class C, int D>
struct anticomb
{
    using EC = complement<C, D>;
    static_assert(2<=len<EC>, "can't correct this complement");
    constexpr static int sign = permsign<append<C, EC>, iota<D>>::value;
// produce permutation of opposite sign if sign<0.
    using type = cons<ref<EC, (sign<0) ? 1 : 0>,
                      cons<ref<EC, (sign<0) ? 0 : 1>,
                           drop<EC, 2>>>;
};

template <class C, int D> struct mapanticomb;
template <int D, class ... C>
struct mapanticomb<std::tuple<C ...>, D>
{
    using type = std::tuple<typename anticomb<C, D>::type ...>;
};

template <int D, int O>
struct choose_
{
    static_assert(D>=O, "Bad dimension or form order.");
    using type = combs<iota<D>, O>;
};

template <int D, int O> using choose = typename choose_<D, O>::type;

template <int D, int O> requires ((D>1) && (2*O>D))
struct choose_<D, O>
{
    static_assert(D>=O, "Bad dimension or form order.");
    using type = typename mapanticomb<choose<D, D-O>, D>::type;
};

} // namespace ra::mp

// Up to (62 x) or (63 28) ~ 2^59 on 64 bit size_t.
constexpr std::size_t
binom(std::size_t n, std::size_t p)
{
    if (p>n) { return 0; }
    if (p>(n-p)) { p=n-p; }
    std::size_t v = 1;
    for (std::size_t i=0; i<p; ++i) { v = v*(n-i)/(i+1); }
    return v;
}

// We form the basis for the result (Cr) and split it in pieces for Oa and Ob; there are (D over Oa) ways. Then we see where and with which signs these pieces are in the bases for Oa (Ca) and Ob (Cb), and form the product.
template <int D, int Oa, int Ob>
struct Wedge
{
    constexpr static int Or = Oa+Ob;
    static_assert(Oa<=D && Ob<=D && Or<=D, "bad orders");
    constexpr static int Na = binom(D, Oa);
    constexpr static int Nb = binom(D, Ob);
    constexpr static int Nr = binom(D, Or);
// in lexical order. Can be used to sort Ca below with FindPermutation.
    using LexOrCa = mp::combs<mp::iota<D>, Oa>;
// the actual components used, which are in lex. order only in some cases.
    using Ca = mp::choose<D, Oa>;
    using Cb = mp::choose<D, Ob>;
    using Cr = mp::choose<D, Or>;
// optimizations.
    constexpr static bool yields_expr = (Na>1) != (Nb>1);
    constexpr static bool yields_expr_a1 = yields_expr && Na==1;
    constexpr static bool yields_expr_b1 = yields_expr && Nb==1;
    constexpr static bool both_scalars = (Na==1 && Nb==1);
    constexpr static bool dot_plus = Na>1 && Nb>1 && Or==D && (Oa<Ob || (Oa>Ob && !ra::odd(Oa*Ob)));
    constexpr static bool dot_minus = Na>1 && Nb>1 && Or==D && (Oa>Ob && ra::odd(Oa*Ob));
    constexpr static bool other = (Na>1 && Nb>1) && ((Oa+Ob!=D) || (Oa==Ob));

    template <class Xr, class Fa, class Va, class Vb>
    constexpr static auto
    term(Va const & a, Vb const & b) -> decltype(VAL(a) * VAL(b))
    {
        if constexpr (0==mp::len<Fa>) {
            return 0;
        } else {
            using Fa0 = mp::first<Fa>;
            using Fb = mp::complement_list<Fa0, Xr>;
            using Sa = mp::findcomb<Fa0, Ca>;
            using Sb = mp::findcomb<Fb, Cb>;
            constexpr int sign = Sa::sign * Sb::sign * mp::permsign<mp::append<Fa0, Fb>, Xr>::value;
            static_assert(sign==+1 || sign==-1, "Bad sign in wedge term.");
            auto aw = [&a]{ if constexpr (is_scalar<Va>) { static_assert(0==Sa::where); return a; } else { return a[Sa::where]; } }();
            auto bw = [&b]{ if constexpr (is_scalar<Vb>) { static_assert(0==Sb::where); return b; } else { return b[Sb::where]; } }();
            return (decltype(aw)(sign))*aw*bw + term<Xr, mp::drop1<Fa>>(a, b);
        }
    }
    constexpr static void
    prod(auto const & a, auto const & b, auto & r)
    {
        static_assert(ra::size(a)==Na && ra::size(b)==Nb && ra::size(r)==Nr, "Bad dims.");
        [&]<class ... Xr>(std::tuple<Xr ...>) { r = { term<Xr, mp::combs<Xr, Oa>>(a, b) ... }; }(Cr{});
    }
};

// Euclidean space, only component shuffling.
template <int D, int O>
struct Hodge
{
    using W = Wedge<D, O, D-O>;
    using Ca = typename W::Ca;
    using Cb = typename W::Cb;
    using Cr = typename W::Cr;
    using LexOrCa = typename W::LexOrCa;
    constexpr static int Na = W::Na;
    constexpr static int Nb = W::Nb;
// If 2*O=D, it is not possible to differentiate the bases by order and hodgex() must be used.
// Likewise, when O(N-O) is odd, Hodge from (2*O>D) to (2*O<D) change sign, since **w= -w,
// and the basis in the (2*O>D) case is selected to make Hodge(<)->Hodge(>) trivial; but can't do both!
    constexpr static bool trivial = 2*O!=D && ((2*O<D) || !ra::odd(O*(D-O)));

    template <int i=0>
    constexpr static void
    hodge_aux(auto const & a, auto & b)
    {
        static_assert(i<=Na, "Bad argument to hodge_aux");
        static_assert(ra::size(a)==Na && ra::size(b)==Nb);
        if constexpr (i<Na) {
            using Cai = mp::ref<Ca, i>;
            static_assert(O==mp::len<Cai>);
// sort Cai, because complement only accepts sorted combs.
// ref<Cb, i> should be complementary to Cai, but I don't want to rely on that.
            using SCai = mp::ref<LexOrCa, mp::findcomb<Cai, LexOrCa>::where>;
            using CompCai = mp::complement<SCai, D>;
            static_assert(D-O==mp::len<CompCai>);
            using fpw = mp::findcomb<CompCai, Cb>;
// for the sign see e.g. DoCarmo1991 I.Ex 10.
            using fps = mp::findcomb<mp::append<Cai, mp::ref<Cb, fpw::where>>, Cr>;
            static_assert(0!=fps::sign);
            b[fpw::where] = decltype(a[i])(fps::sign)*a[i];
            hodge_aux<i+1>(a, b);
        }
    }
};

// The order of components is taken from Wedge<D, O, D-O>; this works for whatever order is defined there.
// With lexical order, component order is reversed, but signs vary.
// With the order given by choose<>, fpw::where==i and fps::sign==+1 in hodge_aux(), always. Then hodge() becomes a free operation, (with one exception) and the next function hodge() can be used.
template <int D, int O, class Va, class Vb>
constexpr void
hodgex(Va const & a, Vb & b) { Hodge<D, O>::hodge_aux(a, b); }

template <int D, int O, class Va, class Vb> requires (Hodge<D, O>::trivial)
constexpr void
hodge(Va const & a, Vb & b) { static_assert(ra::size(a)==Hodge<D, O>::Na && ra::size(b)==Hodge<D, O>::Nb); b = a; }

template <int D, int O, class Va, class Vb> requires (!Hodge<D, O>::trivial)
constexpr void
hodge(Va const & a, Vb & b) { Hodge<D, O>::hodge_aux(a, b); }

template <int D, int O, class Va> requires (Hodge<D, O>::trivial)
constexpr Va const &
hodge(Va const & a) { static_assert(ra::size(a)==Hodge<D, O>::Na, "error"); return a; }

template <int D, int O, class Va> requires (!Hodge<D, O>::trivial)
constexpr Va &
hodge(Va & a) { Va b(a); hodgex<D, O>(b, a); return a; }

// FIXME concrete() isn't needed if a/b are already views.
template <int D, int Oa, int Ob, class Va, class Vb, class Vr>
constexpr void
wedge(Va const & a, Vb const & b, Vr & r) { Wedge<D, Oa, Ob>::prod(concrete(a), concrete(b), r); }

template <int D, int Oa, int Ob, class Va, class Vb>
constexpr auto
wedge(Va const & a, Vb const & b)
{
    if constexpr (is_scalar<Va> && is_scalar<Vb>) {
        return a*b;
    } else {
        constexpr int Nr = Wedge<D, Oa, Ob>::Nr;
        using valtype = decltype(VAL(a) * VAL(b));
        std::conditional_t<Nr==1, valtype, Small<valtype, Nr>> r;
        wedge<D, Oa, Ob>(a, b, r);
        return r;
    }
}

constexpr auto
cross(auto const & a, auto const & b) { return wedge<size_s(a), 1, 1>(a, b); }

template <class V> constexpr auto
perp(V const & v)
{
    static_assert(2==v.size(), "Dimension error.");
    return Small<std::decay_t<decltype(VAL(v))>, 2> {v[1], -v[0]};
}

template <class V, class U> constexpr auto
perp(V const & v, U const & n)
{
    if constexpr (is_scalar<U>) {
        static_assert(2==v.size(), "Dimension error.");
        return Small<std::decay_t<decltype(VAL(v) * n)>, 2> {v[1]*n, -v[0]*n};
    } else {
        static_assert(3==v.size(), "Dimension error.");
        return cross(v, n);
    }
}

} // namespace ra

#undef RA_OPT
