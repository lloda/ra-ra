// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Overloads for expression templates, root header.

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
// Scalar overloads.
// ---------------------------

#if defined(__STDCPP_FLOAT128_T__)
    #include <stdfloat>
#define RA_FLOATS float, double, std::float128_t
#define RA_OP_QUAD_DOUBLE(OP, R, C)                                     \
    constexpr C operator OP(C const & x, double y) { return x OP R(y); } \
    constexpr C operator OP(double x, C const & y) { return R(x) OP y; } \
    constexpr C operator OP(C const & x, std::complex<double> const & y) { return x OP C(y); } \
    constexpr C operator OP(std::complex<double> const & x, C const & y) { return C(x) OP y; }
#define RA_QUAD_DOUBLE(OP) RA_OP_QUAD_DOUBLE(OP, std::float128_t, std::complex<std::float128_t>)
RA_FE(RA_QUAD_DOUBLE, +, -, /, *)
#undef RA_QUAD_DOUBLE_OP
#undef RA_OP_QUAD_DOUBLE_OP
#else
    #define RA_FLOATS float, double
#endif

// abs() works for ra:: types through ADL, should work on pods. std::max/min are special, see RA_NAME. FIXME let user decide?
using std::max, std::min, std::abs, std::fma, std::sqrt, std::pow, std::exp, std::swap,
      std::isfinite, std::isinf, std::isnan, std::clamp, std::lerp, std::conj, std::expm1;

#define RA_FOR_FLOAT(R)                                                 \
    constexpr R conj(R x) { return x; }                                 \
    constexpr std::complex<R>                                           \
    fma(std::complex<R> const & a, std::complex<R> const & b, std::complex<R> const & c) \
    {                                                                   \
        return std::complex<R>(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())), \
                               fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    constexpr bool isfinite(std::complex<R> z) { return isfinite(z.real()) && isfinite(z.imag()); } \
    constexpr bool isnan(std::complex<R> z) { return isnan(z.real()) || isnan(z.imag()); } \
    constexpr bool isinf(std::complex<R> z) { return (isinf(z.real()) || isinf(z.imag())) && !isnan(z); }
RA_FE(RA_FOR_FLOAT, RA_FLOATS)
#undef RA_FOR_FLOAT

namespace ra {

constexpr bool odd(unsigned int N) { return N & 1; }

// As array op; special definitions for rank 0.
template <class T> constexpr bool ra_is_real = std::numeric_limits<T>::is_integer || std::is_floating_point_v<T>;
template <class T> requires (ra_is_real<T>) constexpr T amax(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T amin(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T sqr(T const & x)  { return x*x; }

#define RA_FOR_TYPES(R, C)                                              \
    constexpr R arg(R x) { return R(0); }                               \
    inline /* FIXME constexpr */ R arg(C x) { return std::arg(x); }     \
    constexpr C sqr(C x) { return x*x; }                                \
    constexpr R sqrm(R x) { return sqr(x); }                            \
    constexpr R sqrm(R x, R y) { return sqr(x-y); }                     \
    constexpr R sqrm(C x) { return sqr(x.real())+sqr(x.imag()); }       \
    constexpr R sqrm(C x, C y) { return sqr(x.real()-y.real())+sqr(x.imag()-y.imag()); } \
    constexpr R norm2(R x) { return std::abs(x); }                      \
    constexpr R norm2(R x, R y) { return std::abs(x-y); }               \
    constexpr R norm2(C x) { return sqrt(sqrm(x)); }                    \
    constexpr R norm2(C x, C y) { return sqrt(sqrm(x, y)); }            \
    constexpr R dot(R x, R y) { return x*y; }                           \
    constexpr C dot(C x, C y) { return x*y; }                           \
    constexpr R mul_conj(R x, R y) { return x*y; } /* conj(a) * b */    \
    constexpr C mul_conj(C const & a, C const & b)                      \
    {                                                                   \
        return C(a.real()*b.real()+a.imag()*b.imag(),  a.real()*b.imag()-a.imag()*b.real()); \
    }                                                                   \
    constexpr R fma_conj(R a, R b, R c) { return fma(a, b, c); } /* conj(a) * b + c */ \
    constexpr C fma_conj(C const & a, C const & b, C const & c)         \
    {                                                                   \
        return C(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())), fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    constexpr R fma_sqrm(C const & a, R const & c) { return fma(a.real(), a.real(), fma(a.imag(), a.imag(), c)); } \
    constexpr R rel_error(R a, R b) { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    constexpr R rel_error(C a, C b) { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    constexpr R const & real_part(R const & x) { return x; }            \
    constexpr R & real_part(R & x) { return x; }                        \
    constexpr R real_part(C const & z) { return z.real(); }             \
    constexpr R & real_part(C & z) { return std::bit_cast<R *>(&z)[0]; } \
    constexpr R imag_part(R x) { return R(0); }                         \
    constexpr R imag_part(C const & z) { return z.imag(); }             \
    constexpr R & imag_part(C & z) { return std::bit_cast<R *>(&z)[1]; } \
    constexpr C xi(R x) { return C(0, x); }                             \
    constexpr C xi(C z) { return C(-z.imag(), z.real()); }
#define RA_FOR_FLOAT(R) RA_FOR_TYPES(R, std::complex<R>)
RA_FE(RA_FOR_FLOAT, RA_FLOATS)
#undef RA_FOR_FLOAT
#undef RA_FOR_TYPES
#undef RA_FLOATS

template <class T> constexpr bool is_scalar_def<std::complex<T>> = true;


// --------------------------------
// Optimization pass over expression templates.
// --------------------------------

constexpr decltype(auto) opt(auto && e) { return RA_FW(e); }

// FIXME only reduces iota exprs as operated on in ra.hh (operators), not a tree like wlen() does.
// It's simpler to iota+iota directly, but going through expr(+, iota, iota) handles agreement & wlen.
template <class X> concept iota_op = ra::is_ra_0<X> && std::is_integral_v<ncvalue_t<X>>;

// qualified ra::iota is necessary to avoid ADLing to std::iota (test/headers.cc).
// FIXME p2781r2, handle & variants, handle view iota.

#define RA0 std::get<(0)>(e.t)
#define RA1 std::get<(1)>(e.t)
#define RA_IOTA_BINOP(OP, A, B, VAL) \
    template <A I, B J> constexpr auto opt(Map<OP, std::tuple<I, J>> && e) { return VAL; }

RA_IOTA_BINOP(std::plus<>,       is_iota, iota_op, ra::iota(RA0.n, RA0.cp.i+RA1, RA0.s))
RA_IOTA_BINOP(std::plus<>,       iota_op, is_iota, ra::iota(RA1.n, RA0+RA1.cp.i, RA1.s))
RA_IOTA_BINOP(std::plus<>,       is_iota, is_iota, ra::iota(clen(e, ic<0>), RA0.cp.i+RA1.cp.i, cadd(RA0.s, RA1.s)))
RA_IOTA_BINOP(std::minus<>,      is_iota, iota_op, ra::iota(RA0.n, RA0.cp.i-RA1, RA0.s))
RA_IOTA_BINOP(std::minus<>,      iota_op, is_iota, ra::iota(RA1.n, RA0-RA1.cp.i, csub(ic<0>, RA1.s)))
RA_IOTA_BINOP(std::minus<>,      is_iota, is_iota, ra::iota(clen(e, ic<0>), RA0.cp.i-RA1.cp.i, csub(RA0.s, RA1.s)))
RA_IOTA_BINOP(std::multiplies<>, is_iota, iota_op, ra::iota(RA0.n, RA0.cp.i*RA1, RA0.s*RA1))
RA_IOTA_BINOP(std::multiplies<>, iota_op, is_iota, ra::iota(RA1.n, RA0*RA1.cp.i, RA0*RA1.s))
#undef RA_IOTA_BINOP

template <is_iota I> constexpr auto
opt(Map<std::negate<>, std::tuple<I>> && e) { return ra::iota(RA0.n, -RA0.cp.i, csub(ic<0>, RA0.s)); }

#if RA_OPT_SMALL==1
template <class T, dim_t N, class A> constexpr bool match_small =
    std::is_same_v<std::decay_t<A>, Cell<T *, ic_t<std::array {Dim(N, 1)}>, ic_t<0>>>
    || std::is_same_v<std::decay_t<A>, Cell<T const *, ic_t<std::array {Dim(N, 1)}>, ic_t<0>>>;

#define RA_OPT_SMALL_OP(OP, NAME, T, N)                                 \
    template <class A, class B> requires (match_small<T, N, A> && match_small<T, N, B>) \
    constexpr auto opt(Map<NAME, std::tuple<A, B>> && e)                \
    {                                                                   \
        alignas (alignof(extvector<T, N>)) ra::Small<T, N> val;         \
        *(extvector<T, N> *)(&val) = *(extvector<T, N> *)((RA0.c.cp)) OP *(extvector<T, N> *)((RA1.c.cp)); \
        return val;                                                     \
    }
#define RA_OPT_SMALL_OP_FUNS(T, N)                                \
    static_assert(0==alignof(ra::Small<T, N>) % alignof(extvector<T, N>)); \
    RA_OPT_SMALL_OP(+, std::plus<>, T, N)                         \
    RA_OPT_SMALL_OP(-, std::minus<>, T, N)                        \
    RA_OPT_SMALL_OP(/, std::divides<>, T, N)                      \
    RA_OPT_SMALL_OP(*, std::multiplies<>, T, N)
#define RA_OPT_SMALL_OP_TYPES(N)        \
    RA_OPT_SMALL_OP_FUNS(float, N)      \
    RA_OPT_SMALL_OP_FUNS(double, N)
RA_FE(RA_OPT_SMALL_OP_TYPES, 2, 4, 8)
#undef RA_OPT_SMALL_OP_TYPES
#undef RA_OPT_SMALL_OP_FUNS
#undef RA_OPT_SMALL_OP
#endif // RA_OPT_SMALL
#undef RA1
#undef RA0


// --------------------------------
// Array versions of operators and functions.
// --------------------------------

// We need zero/scalar specializations because the scalar/scalar operators maybe be templated (e.g. complex<>), so they won't be found when an implicit scalar conversion is also needed, and e.g. ra::View<complex, 0> * complex would fail.
#define RA_BINOP(OP, OPNAME)                                            \
    template <class A, class B> requires (tomap<A, B>) constexpr auto   \
        operator OP(A && a, B && b) { return RA_OPT(map(OPNAME(), RA_FW(a), RA_FW(b))); } \
    template <class A, class B> requires (toreduce<A, B>) constexpr auto \
        operator OP(A && a, B && b) { return VAL(RA_FW(a)) OP VAL(RA_FW(b)); }
RA_BINOP(+, std::plus<>)      RA_BINOP(-, std::minus<>)       RA_BINOP(*, std::multiplies<>)
RA_BINOP(/, std::divides<>)   RA_BINOP(==, std::equal_to<>)   RA_BINOP(<=, std::less_equal<>)
RA_BINOP(<, std::less<>)      RA_BINOP(>, std::greater<>)     RA_BINOP(>=, std::greater_equal<>)
RA_BINOP(|, std::bit_or<>)    RA_BINOP(&, std::bit_and<>)     RA_BINOP(!=, std::not_equal_to<>)
RA_BINOP(^, std::bit_xor<>)   RA_BINOP(%, std::modulus<>)     RA_BINOP(<=>, std::compare_three_way)
#undef RA_BINOP

// FIXME sanitizer complains in bench-optimize.cc with std::identity. Maybe false positive
struct unaryplus
{
    template <class T> constexpr static auto operator()(T && t) noexcept { return RA_FW(t); }
};

#define RA_UNOP(OP, OPNAME)                                             \
    template <class A> requires (tomap<A>) constexpr auto               \
        operator OP(A && a) { return RA_OPT(map(OPNAME(), RA_FW(a))); } \
    template <class A> requires (toreduce<A>) constexpr auto            \
        operator OP(A && a) { return OP VAL(RA_FW(a)); }
RA_UNOP(+, unaryplus)         RA_UNOP(-, std::negate<>)       RA_UNOP(!, std::logical_not<>)
#undef RA_UNOP

// if OP(a) isn't found in ra::, deduction rank(0) -> scalar doesn't work. TODO Cf useret.cc, reexported.cc
#define RA_NAME(OP)                                                    \
    template <class ... A> requires (tomap<A ...>) constexpr auto       \
        OP(A && ... a) { return map([](auto && ... a) -> decltype(auto) { return OP(RA_FW(a) ...); }, RA_FW(a) ...); } \
    template <class ... A> requires (toreduce<A ...>) constexpr decltype(auto) \
        OP(A && ... a) { return OP(VAL(RA_FW(a)) ...); }
#define RA_FWD(QUALIFIED_OP, OP)                                    \
    template <class ... A> /* requires neither */ constexpr decltype(auto) \
        OP(A && ... a) { return QUALIFIED_OP(RA_FW(a) ...); }           \
    RA_NAME(OP)
#define RA_USING(QUALIFIED_OP, OP)          \
    using QUALIFIED_OP; RA_NAME(OP)

RA_FE(RA_NAME, odd, arg, sqr, sqrm, real_part, imag_part, xi, rel_error)

// can't RA_USING bc std::max will gobble ra:: objects if passed by const & (!)
// FIXME define own global max/min overloads for basic types. std::max seems too much of a special case to be usinged.
#define RA_GLOBAL(f) RA_FWD(::f, f)
RA_FE(RA_GLOBAL, max, min)
#undef RA_GLOBAL

// don't use RA_FWD for these bc we want to allow ADL, e.g. for exp(dual).
#define RA_GLOBAL(f) RA_USING(::f, f)
RA_FE(RA_GLOBAL, pow, conj, sqrt, exp, expm1, log, log1p, log10, isfinite, isnan, isinf, atan2)
RA_FE(RA_GLOBAL, abs, sin, cos, tan, sinh, cosh, tanh, asin, acos, atan, clamp, lerp)
RA_FE(RA_GLOBAL, fma)
#undef RA_GLOBAL

#undef RA_USING
#undef RA_FWD
#undef RA_NAME

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
// Selection / shortcutting.
// --------------------------------

template <class T, class F> requires (toreduce<T, F>)
constexpr decltype(auto) where(bool const w, T && t, F && f) { return w ? VAL(t) : VAL(f); }

template <class W, class T, class F> requires (tomap<W, T, F>)
constexpr auto where(W && w, T && t, F && f) { return pick(cast<bool>(RA_FW(w)), RA_FW(f), RA_FW(t)); }

// catch all for non-ra types.
template <class T, class F> requires (!(tomap<T, F>) && !(toreduce<T, F>))
constexpr decltype(auto) where(bool const w, T && t, F && f) { return w ? t : f; }

template <class A, class B> requires (tomap<A, B>)
constexpr auto operator &&(A && a, B && b) { return where(RA_FW(a), cast<bool>(RA_FW(b)), false); }

template <class A, class B> requires (tomap<A, B>)
constexpr auto operator ||(A && a, B && b) { return where(RA_FW(a), true, cast<bool>(RA_FW(b))); }

#define RA_BINOP_SHORT(OP)                                              \
    template <class A, class B> requires (toreduce<A, B>)               \
    constexpr auto operator OP(A && a, B && b) { return VAL(a) OP VAL(b);  }

RA_FE(RA_BINOP_SHORT, &&, ||)
#undef RA_BINOP_SHORT


// --------------------------------
// Whole-array ops. TODO First/variable rank reductions? FIXME C++23 and_then/or_else/etc
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
    using T = ncvalue_t<decltype(a)>;
    T c = std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max();
    for_each([&c](auto && a){ if (a<c) { c=a; } }, a);
    return c;
}

constexpr auto
amax(auto && a)
{
    using T = ncvalue_t<decltype(a)>;
    T c = std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest();
    for_each([&c](auto && a){ if (c<a) { c=a; } }, a);
    return c;
}

// FIXME encapsulate this kind of reference-reduction.
// FIXME ply doesn't allow partial iteration (adv then continue).
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
// Wedge product and cross product.
// --------------------

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

// Form the basis for the result (Cr) and split it in pieces for Oa and Ob; there are (D over Oa) ways. Then see where and with which signs these pieces are in the bases for Oa (Ca) and Ob (Cb), and form the product.
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
            static_assert((!is_scalar<Va> || 0==Sa::where) && (!is_scalar<Vb> || 0==Sb::where));
            auto aw = [&a]{ if constexpr (is_scalar<Va>) return a; else return a[Sa::where]; }();
            auto bw = [&b]{ if constexpr (is_scalar<Vb>) return b; else return b[Sb::where]; }();
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
// if 2*O=D, it is not possible to differentiate the bases by order and hodgex() must be used. Likewise, when O(N-O) is odd, Hodge from (2*O>D) to (2*O<D) change sign, since **w= -w, and the basis in the (2*O>D) case is selected to make Hodge(<)->Hodge(>) trivial; but can't do both!
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
// sort Cai, because complement only accepts sorted combs. ref<Cb, i> should be complementary to Cai, but I don't want to rely on that.
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
// With lexical order, component order is reversed, but signs vary. With the order given by choose<>, fpw::where==i and fps::sign==+1 in hodge_aux(), always. Then hodge() becomes a free operation, (with one exception) and the next function hodge() can be used.
template <int D, int O>
constexpr void
hodgex(auto const & a, auto & b) { Hodge<D, O>::hodge_aux(a, b); }

template <int D, int O> requires (Hodge<D, O>::trivial)
constexpr void
hodge(auto const & a, auto & b) { static_assert(ra::size(a)==Hodge<D, O>::Na && ra::size(b)==Hodge<D, O>::Nb); b = a; }

template <int D, int O> requires (!Hodge<D, O>::trivial)
constexpr void
hodge(auto const & a, auto & b) { Hodge<D, O>::hodge_aux(a, b); }

template <int D, int O> requires (Hodge<D, O>::trivial)
constexpr auto const &
hodge(auto const & a) { static_assert(ra::size(a)==Hodge<D, O>::Na, "error"); return a; }

template <int D, int O> requires (!Hodge<D, O>::trivial)
constexpr auto &
hodge(auto & a) { auto b(a); hodgex<D, O>(b, a); return a; }

// FIXME concrete() isn't needed if a/b are already views.
template <int D, int Oa, int Ob>
constexpr void
wedge(auto const & a, auto const & b, auto & r) { Wedge<D, Oa, Ob>::prod(concrete(a), concrete(b), r); }

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


// --------------------
// Dual numbers for automatic differentiation. This section depends only on base.hh.
// --------------------

// From Taylor expansion of f(a), f(a, b) ... FIXME
// f(a+εa') = f(a)+εa'f_a(a); f(a+εa', b+εb') = f(a, b)+ε[a'f_a(a, b) b'f_b(a, b)]

template <class T>
struct Dual
{
    T re, du;

    constexpr static bool is_complex = requires (T & a) { []<class R>(std::complex<R> &){}(a); };
    template <class S> struct real_part { struct type {}; };
    template <class S> requires (is_complex) struct real_part<S> { using type = typename S::value_type; };
    using real_type = typename real_part<T>::type;

    constexpr Dual(T const & r, T const & d): re(r), du(d) {}
    constexpr Dual(T const & r): re(r), du(0.) {} // conversions are by default constants.
    constexpr Dual(real_type const & r) requires (is_complex): re(r), du(0.) {}
    constexpr Dual() {}
#define RA_ASSIGNOPS(OP)                                                \
    constexpr Dual & operator RA_JOIN(OP, =)(T const & r) { *this = *this OP r; return *this; } \
    constexpr Dual & operator RA_JOIN(OP, =)(Dual const & r) { *this = *this OP r; return *this; } \
    constexpr Dual & operator RA_JOIN(OP, =)(real_type const & r) requires (is_complex) { *this = *this OP r; return *this; }
    RA_FE(RA_ASSIGNOPS, +, -, /, *)
#undef RA_ASSIGNOPS
};

template <class A> concept is_dual = requires (A & a) { []<class T>(Dual<T> &){}(a); };

constexpr auto dual(is_dual auto const & r) { return r; }
template <class R> constexpr auto dual(R const & r) { return Dual<R> { r, 0. }; }
template <class R, class D> constexpr auto dual(R const & r, D const & d) { return Dual<std::common_type_t<R, D>> { r, d }; }

constexpr auto operator+(is_dual auto const & a) { return a; }
constexpr auto operator+(is_dual auto const & a, is_dual auto const & b) { return dual(a.re+b.re, a.du+b.du); }
constexpr auto operator+(is_dual auto const & a, auto const & b) { return dual(a.re+b, a.du); }
constexpr auto operator+(auto const & a, is_dual auto const & b) { return dual(a+b.re, b.du); }
constexpr auto operator-(is_dual auto const & a) { return dual(-a.re, -a.du); }
constexpr auto operator-(is_dual auto const & a, is_dual auto const & b) { return dual(a.re-b.re, a.du-b.du); }
constexpr auto operator-(is_dual auto const & a, auto const & b) { return dual(a.re-b, a.du); }
constexpr auto operator-(auto const & a, is_dual auto const & b) { return dual(a-b.re, -b.du); }
constexpr auto operator*(is_dual auto const & a, is_dual auto const & b) { return dual(a.re*b.re, a.re*b.du + a.du*b.re); }
constexpr auto operator*(is_dual auto const & a, auto const & b) { return dual(a.re*b, a.du*b); }
constexpr auto operator*(auto const & a, is_dual auto const & b) { return dual(a*b.re, a*b.du); }
constexpr auto operator/(is_dual auto const & a, is_dual auto const & b) { return a*inv(b); }
constexpr auto operator/(is_dual auto const & a, auto const & b) { return a*inv(dual(b)); }
constexpr auto operator/(auto const & a, is_dual auto const & b) { return dual(a)*inv(b); }

constexpr auto fma(is_dual auto const & a, is_dual auto const & b, is_dual auto const & c) { return dual(fma(a.re, b.re, c.re), fma(a.re, b.du, fma(a.du, b.re, c.du))); }
constexpr auto sqr(is_dual auto const & a) { return a*a; }
constexpr auto inv(is_dual auto const & a) { auto i = 1./a.re; return dual(i, -a.du*sqr(i)); }
constexpr auto cos(is_dual auto const & a) { return dual(cos(a.re), -sin(a.re)*a.du); }
constexpr auto sin(is_dual auto const & a) { return dual(sin(a.re), +cos(a.re)*a.du); }
constexpr auto cosh(is_dual auto const & a) { return dual(cosh(a.re), +sinh(a.re)*a.du); }
constexpr auto sinh(is_dual auto const & a) { return dual(sinh(a.re), +cosh(a.re)*a.du); }
constexpr auto tan(is_dual auto const & a) { auto c = cos(a.du); return dual(tan(a.re), a.du/(c*c)); }
constexpr auto exp(is_dual auto const & a) { return dual(exp(a.re), +exp(a.re)*a.du); }
constexpr auto pow(is_dual auto const & a, auto const & b) { return dual(pow(a.re, b), +b*pow(a.re, b-1)*a.du); }
constexpr auto log(is_dual auto const & a) {  return dual(log(a.re), +a.du/a.re); }
constexpr auto sqrt(is_dual auto const & a) { return dual(sqrt(a.re), +a.du/(2.*sqrt(a.re))); }
constexpr auto abs(is_dual auto const & a) { return abs(a.re); }
constexpr bool isfinite(is_dual auto const & a) { return isfinite(a.re) && isfinite(a.du); }
constexpr auto xi(is_dual auto const & a) { return dual(xi(a.re), xi(a.du)); }

inline std::ostream &
operator<<(std::ostream & o, is_dual auto const & a) { return o << "[" << a.re << " " << a.du << "]"; }

inline std::istream &
operator>>(std::istream & i, is_dual auto & a)
{
    if (char s; i >> s, s!='[') {
        i.setstate(std::ios::failbit);
    } else {
        i >> a.re >> a.du >> s;
        if (s!=']') { i.setstate(std::ios::failbit); }
    }
    return i;
}

} // namespace ra

#undef RA_OPT
