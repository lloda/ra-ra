// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Operator overloads for expression templates, and root header.

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "big.hh"
#include "optimize.hh"
#include <cmath>
#include <complex>

#ifndef RA_DO_OPT
  #define RA_DO_OPT 1 // enabled by default
#endif
#if RA_DO_OPT==1
  #define RA_OPT optimize
#else
  #define RA_OPT
#endif

// Enable ADL with explicit template args. See http://stackoverflow.com/questions/9838862.
template <class A> constexpr void transpose(ra::noarg);
template <int A> constexpr void iter(ra::noarg);


// ---------------------------
// scalar overloads
// ---------------------------

// abs() needs no qualifying for ra:: types (ADL), shouldn't need it on pods either. FIXME maybe let user decide.
// std::max/min are special, see DEF_NAME in ra.hh.
using std::max, std::min, std::abs, std::fma, std::sqrt, std::pow, std::exp, std::swap,
      std::isfinite, std::isinf, std::isnan, std::clamp, std::lerp, std::conj, std::expm1;

#define FOR_FLOAT(T)                                      \
    constexpr T conj(T x) { return x; }                   \
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

#define FOR_FLOAT(R, C)                                                 \
    constexpr C                                                         \
    fma(C const & a, C const & b, C const & c)                          \
    {                                                                   \
        return C(fma(a.real(), b.real(), fma(-a.imag(), b.imag(), c.real())), \
                 fma(a.real(), b.imag(), fma(a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    constexpr bool isfinite(C z)       { return isfinite(z.real()) && isfinite(z.imag()); } \
    constexpr bool isnan(C z)          { return isnan(z.real()) || isnan(z.imag()); } \
    constexpr bool isinf(C z)          { return (isinf(z.real()) || isinf(z.imag())) && !isnan(z); }
FOR_FLOAT(float, std::complex<float>)
FOR_FLOAT(double, std::complex<double>)
#undef FOR_FLOAT

namespace ra {

// As an array op; special definitions for rank 0.
template <class T> constexpr bool ra_is_real = std::numeric_limits<T>::is_integer || std::is_floating_point_v<T>;
template <class T> requires (ra_is_real<T>) constexpr T amax(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T amin(T const & x) { return x; }
template <class T> requires (ra_is_real<T>) constexpr T sqr(T const & x)  { return x*x; }

#define FOR_FLOAT(T)                                                    \
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
    constexpr T const & real_part(T const & x)  { return x; }           \
    constexpr T & real_part(T & x)      { return x; }                   \
    constexpr T imag_part(T x)          { return T(0); }
FOR_EACH(FOR_FLOAT, float, double)
#undef FOR_FLOAT

// FIXME few still inline should eventually be constexpr.
#define FOR_FLOAT(R, C)                                                 \
    inline R arg(C x)                  { return std::arg(x); }          \
    constexpr C sqr(C x)               { return x*x; }                  \
    constexpr C dot(C x, C y)          { return x*y; }                  \
    constexpr C xI(R x)                { return C(0, x); }              \
    constexpr C xI(C z)                { return C(-z.imag(), z.real()); } \
    constexpr R real_part(C const & z) { return z.real(); }             \
    constexpr R imag_part(C const & z) { return z.imag(); }             \
    inline R & real_part(C & z)        { return reinterpret_cast<R *>(&z)[0]; } \
    inline R & imag_part(C & z)        { return reinterpret_cast<R *>(&z)[1]; } \
    constexpr R sqrm(C x)              { return sqr(x.real())+sqr(x.imag()); } \
    constexpr R sqrm(C x, C y)         { return sqr(x.real()-y.real())+sqr(x.imag()-y.imag()); } \
    constexpr R norm2(C x)             { return hypot(x.real(), x.imag()); } \
    constexpr R norm2(C x, C y)        { return sqrt(sqrm(x, y)); }     \
    inline R rel_error(C a, C b)       { auto den = (abs(a)+abs(b)); return den==0 ? 0. : 2.*norm2(a, b)/den; } \
    /* conj(a) * b + c */                                               \
    constexpr C                                                         \
    fma_conj(C const & a, C const & b, C const & c)                     \
    {                                                                   \
        return C(fma(a.real(), b.real(), fma(a.imag(), b.imag(), c.real())), \
                 fma(a.real(), b.imag(), fma(-a.imag(), b.real(), c.imag()))); \
    }                                                                   \
    /* conj(a) * b */                                                   \
    constexpr C                                                         \
    mul_conj(C const & a, C const & b)                                  \
    {                                                                   \
        return C(a.real()*b.real()+a.imag()*b.imag(),                   \
                 a.real()*b.imag()-a.imag()*b.real());                  \
    }
FOR_FLOAT(float, std::complex<float>)
FOR_FLOAT(double, std::complex<double>)
#undef FOR_FLOAT

template <class T> constexpr bool is_scalar_def<std::complex<T>> = true;

template <int ... Iarg, class A>
constexpr decltype(auto)
transpose(mp::int_list<Iarg ...>, A && a)
{
    return transpose<Iarg ...>(RA_FWD(a));
}

constexpr bool odd(unsigned int N) { return N & 1; }


// --------------------------------
// Array versions of operators and functions
// --------------------------------

// We need zero/scalar specializations because the scalar/scalar operators maybe be templated (e.g. complex<>), so they won't be found when an implicit conversion to scalar is also needed, and e.g. ra::View<complex, 0> * complex would fail.
// The function objects are matched in optimize.hh.
#define DEF_NAMED_BINARY_OP(OP, OPNAME)                                 \
    template <class A, class B> requires (tomap<A, B>) constexpr auto   \
    operator OP(A && a, B && b)                                         \
    { return RA_OPT(map(OPNAME(), RA_FWD(a), RA_FWD(b))); }             \
    template <class A, class B> requires (toreduce<A, B>) constexpr auto \
    operator OP(A && a, B && b)                                         \
    { return VALUE(RA_FWD(a)) OP VALUE(RA_FWD(b)); }

DEF_NAMED_BINARY_OP(+, std::plus<>)          DEF_NAMED_BINARY_OP(-, std::minus<>)
DEF_NAMED_BINARY_OP(*, std::multiplies<>)    DEF_NAMED_BINARY_OP(/, std::divides<>)
DEF_NAMED_BINARY_OP(==, std::equal_to<>)     DEF_NAMED_BINARY_OP(>, std::greater<>)
DEF_NAMED_BINARY_OP(<, std::less<>)          DEF_NAMED_BINARY_OP(>=, std::greater_equal<>)
DEF_NAMED_BINARY_OP(<=, std::less_equal<>)   DEF_NAMED_BINARY_OP(!=, std::not_equal_to<>)
DEF_NAMED_BINARY_OP(|, std::bit_or<>)        DEF_NAMED_BINARY_OP(&, std::bit_and<>)
DEF_NAMED_BINARY_OP(^, std::bit_xor<>)       DEF_NAMED_BINARY_OP(<=>, std::compare_three_way)
#undef DEF_NAMED_BINARY_OP

// FIXME address sanitizer complains in bench-optimize.cc if we use std::identity. Maybe false positive
struct unaryplus
{
    template <class T> constexpr /* static P1169 in gcc13 */ auto
    operator()(T && t) const noexcept { return RA_FWD(t); }
};

#define DEF_NAMED_UNARY_OP(OP, OPNAME)                          \
    template <class A> requires (tomap<A>) constexpr auto       \
    operator OP(A && a)                                         \
    { return map(OPNAME(), RA_FWD(a)); }                        \
    template <class A> requires (toreduce<A>) constexpr auto    \
    operator OP(A && a)                                         \
    { return OP VALUE(RA_FWD(a)); }

DEF_NAMED_UNARY_OP(+, unaryplus)
DEF_NAMED_UNARY_OP(-, std::negate<>)
DEF_NAMED_UNARY_OP(!, std::logical_not<>)
#undef DEF_NAMED_UNARY_OP

// if OP(a) isn't found in ra::, deduction rank(0) -> scalar doesn't work. TODO Cf useret.cc, reexported.cc
#define DEF_NAME(OP)                                                    \
    template <class ... A> requires (tomap<A ...>) constexpr auto       \
    OP(A && ... a)                                                      \
    { return map([](auto && ... a) -> decltype(auto) { return OP(RA_FWD(a) ...); }, RA_FWD(a) ...); } \
    template <class ... A> requires (toreduce<A ...>) constexpr decltype(auto) \
    OP(A && ... a)                                                      \
    { return OP(VALUE(RA_FWD(a)) ...); }
#define DEF_FWD(QUALIFIED_OP, OP)                                       \
    template <class ... A> requires (!tomap<A ...> && !toreduce<A ...>) constexpr decltype(auto) \
    OP(A && ... a)                                                      \
    { return QUALIFIED_OP(RA_FWD(a) ...); }                             \
    DEF_NAME(OP)
#define DEF_USING(QUALIFIED_OP, OP)             \
    using QUALIFIED_OP;                         \
    DEF_NAME(OP)

FOR_EACH(DEF_NAME, odd, arg, sqr, sqrm, real_part, imag_part, xI, rel_error)

// can't DEF_USING bc std::max will gobble ra:: objects if passed by const & (!)
// FIXME define own global max/min overloads for basic types. std::max seems too much of a special case to be usinged.
#define DEF_GLOBAL(f) DEF_FWD(::f, f)
FOR_EACH(DEF_GLOBAL, max, min)
#undef DEF_GLOBAL

// don't use DEF_FWD for these bc we want to allow ADL, e.g. for exp(dual).
#define DEF_GLOBAL(f) DEF_USING(::f, f)
FOR_EACH(DEF_GLOBAL, pow, conj, sqrt, exp, expm1, log, log1p, log10, isfinite, isnan, isinf, atan2)
FOR_EACH(DEF_GLOBAL, abs, sin, cos, tan, sinh, cosh, tanh, asin, acos, atan, clamp, lerp)
#undef DEF_GLOBAL

#undef DEF_USING
#undef DEF_FWD
#undef DEF_NAME

template <class T, class A>
constexpr auto
cast(A && a)
{
    return map([](auto && b) -> decltype(auto) { return T(b); }, RA_FWD(a));
}

// TODO std::forward_as_tuple?
template <class T, class ... A>
constexpr auto
pack(A && ... a)
{
    return map([](auto && ... a) { return T { a ... }; }, RA_FWD(a) ...);
}

// FIXME needs nested array for I
template <class A, class I>
constexpr auto
at(A && a, I && i)
{
    return map([a = std::tuple<A>(RA_FWD(a))] (auto && i) -> decltype(auto) { return std::get<0>(a).at(i); },
               RA_FWD(i));
}


// --------------------------------
// selection / shortcutting
// --------------------------------

// ra::start are needed bc rank 0 converts to and from scalar, so ? can't pick the right (-> scalar) conversion.
template <class T, class F> requires (toreduce<T, F>)
constexpr decltype(auto)
where(bool const w, T && t, F && f)
{
    return w ? VALUE(t) : VALUE(f);
}
template <class W, class T, class F> requires (tomap<W, T, F>)
constexpr auto
where(W && w, T && t, F && f)
{
    return pick(cast<bool>(RA_FWD(w)), RA_FWD(f), RA_FWD(t));
}
// catch all for non-ra types.
template <class T, class F> requires (!(tomap<T, F>) && !(toreduce<T, F>))
constexpr decltype(auto)
where(bool const w, T && t, F && f)
{
    return w ? t : f;
}

template <class A, class B> requires (tomap<A, B>)
constexpr auto
operator &&(A && a, B && b)
{
    return where(RA_FWD(a), cast<bool>(RA_FWD(b)), false);
}
template <class A, class B> requires (tomap<A, B>)
constexpr auto
operator ||(A && a, B && b)
{
    return where(RA_FWD(a), true, cast<bool>(RA_FWD(b)));
}
#define DEF_SHORTCIRCUIT_BINARY_OP(OP)                                  \
    template <class A, class B> requires (toreduce<A, B>)               \
    constexpr auto operator OP(A && a, B && b)                          \
    {                                                                   \
        return VALUE(a) OP VALUE(b);                                    \
    }
FOR_EACH(DEF_SHORTCIRCUIT_BINARY_OP, &&, ||);
#undef DEF_SHORTCIRCUIT_BINARY_OP


// --------------------------------
// Some whole-array reductions. TODO First rank reductions? Variable rank reductions?
// FIXME C++23 and_then/or_else/etc
// --------------------------------

constexpr bool
any(auto && a)
{
    return early(map([](bool x) { return x ? std::make_optional(true) : std::nullopt; }, RA_FWD(a)), false);
}

constexpr bool
every(auto && a)
{
    return early(map([](bool x) { return !x ? std::make_optional(false) : std::nullopt; }, RA_FWD(a)), true);
}

// FIXME variable rank? see J 'index of' (x i. y), etc.
constexpr auto
index(auto && a)
{
    return early(map([](auto && a, auto && i) { return bool(a) ? std::make_optional(i) : std::nullopt; },
                     RA_FWD(a), ra::iota(ra::start(a).len(0))),
                 ra::dim_t(-1));
}

// [ma108]
constexpr bool
lexicographical_compare(auto && a, auto && b)
{
    return early(map([](auto && a, auto && b) { return a==b ? std::nullopt : std::make_optional(a<b); },
                     RA_FWD(a), RA_FWD(b)),
                 false);
}

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
    auto p = &(*s);
    for_each([&less, &p](auto & a) { if (less(a, *p)) { p = &a; } }, s);
    return *p;
}

template <class A, class Less = std::less<value_t<A>>>
constexpr decltype(auto)
refmax(A && a, Less && less = std::less<value_t<A>>())
{
    RA_CHECK(a.size()>0);
    decltype(auto) s = ra::start(a);
    auto p = &(*s);
    for_each([&less, &p](auto & a) { if (less(*p, a)) { p = &a; } }, s);
    return *p;
}

template <class A>
constexpr auto
sum(A && a)
{
    auto c = concrete_type<value_t<A>>(0);
    for_each([&c](auto && a) { c += a; }, a);
    return c;
}

template <class A>
constexpr auto
prod(A && a)
{
    auto c = concrete_type<value_t<A>>(1);
    for_each([&c](auto && a) { c *= a; }, a);
    return c;
}

constexpr auto reduce_sqrm(auto && a) { return sum(sqrm(a)); }
constexpr auto norm2(auto && a) { return std::sqrt(reduce_sqrm(a)); }

constexpr auto
dot(auto && a, auto && b)
{
    std::decay_t<decltype(VALUE(a) * VALUE(b))> c(0.);
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

constexpr auto
cdot(auto && a, auto && b)
{
    std::decay_t<decltype(conj(VALUE(a)) * VALUE(b))> c(0.);
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

constexpr auto
normv(auto const & a)
{
    auto b = concrete(a);
    b /= norm2(b);
    return b;
}

// FIXME benchmark w/o allocation and do Small/Big versions if it's worth it.
constexpr void
gemm(auto const & a, auto const & b, auto & c)
{
    for_each(ra::wrank<1, 1, 2>(ra::wrank<1, 0, 1>([](auto && c, auto && a, auto && b) { c += a*b; })), c, a, b);
}

#define MMTYPE decltype(from(std::multiplies<>(), a(all, 0), b(0)))

// default for row-major x row-major. See bench-gemm.cc for variants.
template <class S, class T>
constexpr auto
gemm(ra::View<S, 2> const & a, ra::View<T, 2> const & b)
{
    dim_t M=a.len(0), N=b.len(1), K=a.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<MMTYPE>({M, N}, decltype(std::declval<S>()*std::declval<T>())());
    for (int k=0; k<K; ++k) {
        c += from(std::multiplies<>(), a(all, k), b(k));
    }
    return c;
}

// we still want the Small version to be different.
template <class A, class B>
constexpr ra::Small<std::decay_t<decltype(VALUE(std::declval<A>()) * VALUE(std::declval<B>()))>, A::len(0), B::len(1)>
gemm(A const & a, B const & b)
{
    dim_t M=a.len(0), N=b.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<MMTYPE>({M, N}, ra::none);
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(all, j));
        }
    }
    return c;
}

#undef MMTYPE

constexpr auto
gevm(auto const & a, auto const & b)
{
    dim_t M=b.len(0), N=b.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<decltype(a[0]*b(0))>({N}, 0);
    for (int i=0; i<M; ++i) {
        c += a[i]*b(i);
    }
    return c;
}

// FIXME a must be a view, so it doesn't work with e.g. gemv(conj(a), b).
constexpr auto
gemv(auto const & a, auto const & b)
{
    dim_t M=a.len(0), N=a.len(1);
// no with_same_shape bc cannot index 0 for type if A/B are empty
    auto c = with_shape<decltype(a(all, 0)*b[0])>({M}, 0);
    for (int j=0; j<N; ++j) {
        c += a(all, j) * b[j];
    }
    return c;
}


// --------------------
// Wedge product and cross product
// --------------------

namespace mp {

template <class P, class Plist>
struct FindCombination
{
    template <class A> using match = bool_c<0 != PermutationSign<P, A>::value>;
    using type = IndexIf<Plist, match>;
    constexpr static int where = type::value;
    constexpr static int sign = (where>=0) ? PermutationSign<P, typename type::type>::value : 0;
};

// Combination antiC complementary to C wrt [0, 1, ... Dim-1], permuted so [C, antiC] has the sign of [0, 1, ... Dim-1].
template <class C, int D>
struct AntiCombination
{
    using EC = complement<C, D>;
    static_assert((len<EC>)>=2, "can't correct this complement");
    constexpr static int sign = PermutationSign<append<C, EC>, iota<D>>::value;
// Produce permutation of opposite sign if sign<0.
    using type = mp::cons<std::tuple_element_t<(sign<0) ? 1 : 0, EC>,
                          mp::cons<std::tuple_element_t<(sign<0) ? 0 : 1, EC>,
                                   mp::drop<EC, 2>>>;
};

template <class C, int D> struct MapAntiCombination;
template <int D, class ... C>
struct MapAntiCombination<std::tuple<C ...>, D>
{
    using type = std::tuple<typename AntiCombination<C, D>::type ...>;
};

template <int D, int O>
struct ChooseComponents
{
    static_assert(D>=O, "Bad dimension or form order.");
    using type = mp::combinations<iota<D>, O>;
};

template <int D, int O> using ChooseComponents_ = typename ChooseComponents<D, O>::type;

template <int D, int O> requires ((D>1) && (2*O>D))
struct ChooseComponents<D, O>
{
    static_assert(D>=O, "Bad dimension or form order.");
    using type = typename MapAntiCombination<ChooseComponents_<D, D-O>, D>::type;
};

// Works almost to the range of std::size_t.
constexpr std::size_t
n_over_p(std::size_t const n, std::size_t p)
{
    if (p>n) {
        return 0;
    } else if (p>(n-p)) {
        p = n-p;
    }
    std::size_t v = 1;
    for (std::size_t i=0; i!=p; ++i) {
        v = v*(n-i)/(i+1);
    }
    return v;
}

// We form the basis for the result (Cr) and split it in pieces for Oa and Ob; there are (D over Oa) ways. Then we see where and with which signs these pieces are in the bases for Oa (Ca) and Ob (Cb), and form the product.
template <int D, int Oa, int Ob>
struct Wedge
{
    constexpr static int Or = Oa+Ob;
    static_assert(Oa<=D && Ob<=D && Or<=D, "bad orders");
    constexpr static int Na = n_over_p(D, Oa);
    constexpr static int Nb = n_over_p(D, Ob);
    constexpr static int Nr = n_over_p(D, Or);
// in lexicographic order. Can be used to sort Ca below with FindPermutation.
    using LexOrCa = mp::combinations<mp::iota<D>, Oa>;
// the actual components used, which are in lex. order only in some cases.
    using Ca = mp::ChooseComponents_<D, Oa>;
    using Cb = mp::ChooseComponents_<D, Ob>;
    using Cr = mp::ChooseComponents_<D, Or>;
// optimizations.
    constexpr static bool yields_expr = (Na>1) != (Nb>1);
    constexpr static bool yields_expr_a1 = yields_expr && Na==1;
    constexpr static bool yields_expr_b1 = yields_expr && Nb==1;
    constexpr static bool both_scalars = (Na==1 && Nb==1);
    constexpr static bool dot_plus = Na>1 && Nb>1 && Or==D && (Oa<Ob || (Oa>Ob && !ra::odd(Oa*Ob)));
    constexpr static bool dot_minus = Na>1 && Nb>1 && Or==D && (Oa>Ob && ra::odd(Oa*Ob));
    constexpr static bool general_case = (Na>1 && Nb>1) && ((Oa+Ob!=D) || (Oa==Ob));

    template <class Va, class Vb>
    using valtype = std::decay_t<decltype(std::declval<Va>()[0] * std::declval<Vb>()[0])>;

    template <class Xr, class Fa, class Va, class Vb>
    constexpr static valtype<Va, Vb>
    term(Va const & a, Vb const & b)
    {
        if constexpr (mp::len<Fa> > 0) {
            using Fa0 = mp::first<Fa>;
            using Fb = mp::complement_list<Fa0, Xr>;
            using Sa = mp::FindCombination<Fa0, Ca>;
            using Sb = mp::FindCombination<Fb, Cb>;
            constexpr int sign = Sa::sign * Sb::sign * mp::PermutationSign<mp::append<Fa0, Fb>, Xr>::value;
            static_assert(sign==+1 || sign==-1, "Bad sign in wedge term.");
            return valtype<Va, Vb>(sign)*a[Sa::where]*b[Sb::where] + term<Xr, mp::drop1<Fa>>(a, b);
        } else {
            return 0.;
        }
    }
    template <class Va, class Vb, class Vr, int wr>
    constexpr static void
    coeff(Va const & a, Vb const & b, Vr & r)
    {
        if constexpr (wr<Nr) {
            using Xr = mp::ref<Cr, wr>;
            using Fa = mp::combinations<Xr, Oa>;
            r[wr] = term<Xr, Fa>(a, b);
            coeff<Va, Vb, Vr, wr+1>(a, b, r);
        }
    }
    template <class Va, class Vb, class Vr>
    constexpr static void
    product(Va const & a, Vb const & b, Vr & r)
    {
        static_assert(Va::size()==Na, "Bad Va dim.");
        static_assert(Vb::size()==Nb, "Bad Vb dim.");
        static_assert(Vr::size()==Nr, "Bad Vr dim.");
        coeff<Va, Vb, Vr, 0>(a, b, r);
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

    template <int i, class Va, class Vb>
    constexpr static void
    hodge_aux(Va const & a, Vb & b)
    {
        static_assert(i<=W::Na, "Bad argument to hodge_aux");
        if constexpr (i<W::Na) {
            using Cai = mp::ref<Ca, i>;
            static_assert(mp::len<Cai> == O, "Bad.");
// sort Cai, because mp::complement only accepts sorted combinations.
// ref<Cb, i> should be complementary to Cai, but I don't want to rely on that.
            using SCai = mp::ref<LexOrCa, mp::FindCombination<Cai, LexOrCa>::where>;
            using CompCai = mp::complement<SCai, D>;
            static_assert(mp::len<CompCai> == D-O, "Bad.");
            using fpw = mp::FindCombination<CompCai, Cb>;
// for the sign see e.g. DoCarmo1991 I.Ex 10.
            using fps = mp::FindCombination<mp::append<Cai, mp::ref<Cb, fpw::where>>, Cr>;
            static_assert(fps::sign!=0, "Bad.");
            b[fpw::where] = decltype(a[i])(fps::sign)*a[i];
            hodge_aux<i+1>(a, b);
        }
    }
};

// The order of components is taken from Wedge<D, O, D-O>; this works for whatever order is defined there.
// With lexicographic order, component order is reversed, but signs vary.
// With the order given by ChooseComponents<>, fpw::where==i and fps::sign==+1 in hodge_aux(), always. Then hodge() becomes a free operation, (with one exception) and the next function hodge() can be used.
template <int D, int O, class Va, class Vb>
constexpr void
hodgex(Va const & a, Vb & b)
{
    static_assert(O<=D, "bad orders");
    static_assert(Va::size()==mp::Hodge<D, O>::Na, "error");
    static_assert(Vb::size()==mp::Hodge<D, O>::Nb, "error");
    mp::Hodge<D, O>::template hodge_aux<0>(a, b);
}

} // namespace ra::mp

// This depends on Wedge<>::Ca, Cb, Cr coming from ChooseCombinations. hodgex() should always work, but this is cheaper.
// However if 2*O=D, it is not possible to differentiate the bases by order and hodgex() must be used.
// Likewise, when O(N-O) is odd, Hodge from (2*O>D) to (2*O<D) change sign, since **w= -w in that case, and the basis in the (2*O>D) case is selected to make Hodge(<)->Hodge(>) trivial; but can't do both!
consteval bool trivial_hodge(int D, int O) { return 2*O!=D && ((2*O<D) || !ra::odd(O*(D-O))); }

template <int D, int O, class Va, class Vb>
constexpr void
hodge(Va const & a, Vb & b)
{
    if constexpr (trivial_hodge(D, O)) {
        static_assert(Va::size()==mp::Hodge<D, O>::Na, "error");
        static_assert(Vb::size()==mp::Hodge<D, O>::Nb, "error");
        b = a;
    } else {
        ra::mp::hodgex<D, O>(a, b);
    }
}

template <int D, int O, class Va> requires (trivial_hodge(D, O))
constexpr Va const &
hodge(Va const & a)
{
    static_assert(Va::size()==mp::Hodge<D, O>::Na, "error");
    return a;
}

template <int D, int O, class Va> requires (!trivial_hodge(D, O))
constexpr Va &
hodge(Va & a)
{
    Va b(a);
    ra::mp::hodgex<D, O>(b, a);
    return a;
}

template <int D, int Oa, int Ob, class A, class B> requires (is_scalar<A> && is_scalar<B>)
constexpr auto
wedge(A const & a, B const & b) { return a*b; }

template <class A>
using torank1 = std::conditional_t<is_scalar<A>, Small<std::decay_t<A>, 1>, A>;

template <int D, int Oa, int Ob, class Va, class Vb> requires (!(is_scalar<Va> && is_scalar<Vb>))
decltype(auto)
wedge(Va const & a, Vb const & b)
{
    Small<value_t<Va>, size_s<Va>()> aa = a;
    Small<value_t<Vb>, size_s<Vb>()> bb = b;
    using Ua = decltype(aa);
    using Ub = decltype(bb);
    using Wedge = mp::Wedge<D, Oa, Ob>;
    using valtype = typename Wedge::template valtype<Ua, Ub>;
    std::conditional_t<Wedge::Nr==1, valtype, Small<valtype, Wedge::Nr>> r;
    auto & a1 = reinterpret_cast<torank1<Ua> const &>(aa);
    auto & b1 = reinterpret_cast<torank1<Ub> const &>(bb);
    auto & r1 = reinterpret_cast<torank1<decltype(r)> &>(r);
    mp::Wedge<D, Oa, Ob>::product(a1, b1, r1);
    return r;
}

template <int D, int Oa, int Ob, class Va, class Vb, class Vr> requires (!(is_scalar<Va> && is_scalar<Vb>))
void
wedge(Va const & a, Vb const & b, Vr & r)
{
    Small<value_t<Va>, size_s<Va>()> aa = a;
    Small<value_t<Vb>, size_s<Vb>()> bb = b;
    using Ua = decltype(aa);
    using Ub = decltype(bb);
    auto & r1 = reinterpret_cast<torank1<decltype(r)> &>(r);
    auto & a1 = reinterpret_cast<torank1<Ua> const &>(aa);
    auto & b1 = reinterpret_cast<torank1<Ub> const &>(bb);
    mp::Wedge<D, Oa, Ob>::product(a1, b1, r1);
}

template <class A, class B>
constexpr auto
cross(A const & a_, B const & b_)
{
    constexpr int n = size_s<A>();
    static_assert(n==size_s<B>() && (2==n || 3==n));
    Small<std::decay_t<decltype(VALUE(a_))>, n> a = a_;
    Small<std::decay_t<decltype(VALUE(b_))>, n> b = b_;
    using W = mp::Wedge<n, 1, 1>;
    Small<std::decay_t<decltype(VALUE(a_) * VALUE(b_))>, W::Nr> r;
    W::product(a, b, r);
    if constexpr (1==W::Nr) {
        return r[0];
    } else {
        return r;
    }
}

template <class V>
constexpr auto
perp(V const & v)
{
    static_assert(2==v.size(), "Dimension error.");
    return Small<std::decay_t<decltype(VALUE(v))>, 2> {v[1], -v[0]};
}

template <class V, class U>
constexpr auto
perp(V const & v, U const & n)
{
    if constexpr (is_scalar<U>) {
        static_assert(2==v.size(), "Dimension error.");
        return Small<std::decay_t<decltype(VALUE(v) * n)>, 2> {v[1]*n, -v[0]*n};
    } else {
        static_assert(3==v.size(), "Dimension error.");
        return cross(v, n);
    }
}

} // namespace ra

#undef RA_OPT
