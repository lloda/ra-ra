
// (c) Daniel Llorens - 2014-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_OPERATORS_H
#define RA_OPERATORS_H

/// @file ra-operators.H
/// @brief Sugar for ra:: expression templates.

#include "ra/ra-wrank.H"

// @TODO These (dependence on specific ra:: types) should be elsewhere.
#include "ra/ra-small.H"
#include "ra/ra-large.H"
#include "ra/complex.H"
#include "ra/wedge.H"

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_OPERATORS RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_OPERATORS
        #define RA_CHECK_BOUNDS_RA_OPERATORS 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_OPERATORS==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

#include "ra/ra-optimize.H"

#ifndef RA_OPTIMIZE
#define RA_OPTIMIZE 1 // enabled by default
#endif
#if RA_OPTIMIZE==1
#define OPTIMIZE optimize
#else
#define OPTIMIZE
#endif

#define START(a) *(ra::start(a).flat())

// This global version must be available so that ra::transpose<> may be searched by ADL even when giving explicit template args. See http://stackoverflow.com/questions/9838862 . See also wedge() in ra/wedge.H.
template <class ... Iarg, class A, enableif_<ra::is_scalar<A>, int> =0>
decltype(auto) transpose(A && a)
{
    static_assert(sizeof...(Iarg)==0, "bad transpose axes list for scalar");
    return START(a);
}

// See ::wedge() in ra/wedge.H, need ra::is_scalar<> to define.
// These ra::start are needed b/c rank 0 converts to and from scalar, so ? can't pick the right (-> scalar) conversion.
template <class T, class F,
          enableif_<mp::And<ra::is_zero_or_scalar<T>, ra::is_zero_or_scalar<F> >, int> =0>
inline decltype(auto) where(bool const w, T && t, F && f) { return w ? START(t) : START(f); }

namespace ra {

template <int ... Iarg, class A>
decltype(auto) transpose(A && a, mp::int_list<Iarg ...>)
{
    return transpose<Iarg ...>(std::forward<A>(a));
}

// Wrappers over expr & ply_either.

template <class F, class ... A>
inline auto map(F && f, A && ... a) -> decltype(expr(std::forward<F>(f), start(std::forward<A>(a)) ...))
{
    return expr(std::forward<F>(f), start(std::forward<A>(a)) ...);
}

template <class F, class ... A>
inline void for_each(F && f, A && ... a)
{
    ply_either(expr(std::forward<F>(f), start(std::forward<A>(a)) ...));
}

// I considered three options for lookup.
// 1. define these in a class that ArrayIterator or Container or Slice types derive from. This was done for an old library I had (vector-ops.H). It results in the narrowest scope, but since those types are used in the definition (ra::Expr is an ArrayIterator), it requires lots of forwarding and traits:: .
// 2. raw ADL doesn't work because some ra:: types use ! != etc for different things (e.g. FlatIterator). Possible solution: don't ever use + != == for FlatIterator.
// 3. enable_if'd ADL is what you see here.

// --------------------------------
// Array versions of operators and functions
// --------------------------------

// We need the zero/scalar specializations because the scalar/scalar operators
// maybe be templated (e.g. complex<>), so they won't be found when an implicit
// conversion from zero->scalar is also needed. That is, without those
// specializations, ra::Raw<complex, 0> * complex will fail.

// These depend on OPNAME defined in ra-optimize.H and used there to match ET patterns.
#define DEFINE_NAMED_BINARY_OP(OPERATOR, OP, OPNAME)                    \
    template <class A, class B, enableif_<ra_pos_and_any<A, B>, int> =0> \
    inline auto OPERATOR(A && a, B && b)                                \
    {                                                                   \
        return OPTIMIZE(map(OPNAME(), std::forward<A>(a), std::forward<B>(b))); \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero<A, B>, int> =0>       \
    inline auto OPERATOR(A && a, B && b)                                \
    {                                                                   \
        return START(a) OP START(b);                                    \
    }
DEFINE_NAMED_BINARY_OP(operator+, +, plus)
DEFINE_NAMED_BINARY_OP(operator-, -, minus)
DEFINE_NAMED_BINARY_OP(operator*, *, times)
DEFINE_NAMED_BINARY_OP(operator/, /, slash)
#undef DEFINE_NAMED_BINARY_OP

#define DEFINE_BINARY_OP(OPERATOR, OP)                                  \
    template <class A, class B, enableif_<ra_pos_and_any<A, B>, int> =0> \
    inline auto OPERATOR(A && a, B && b)                                \
    {                                                                   \
        return map([](auto && a, auto && b) { return a OP b; },         \
                   std::forward<A>(a), std::forward<B>(b));             \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero<A, B>, int> =0>       \
    inline auto OPERATOR(A && a, B && b)                                \
    {                                                                   \
        return START(a) OP START(b);                                    \
    }
DEFINE_BINARY_OP(operator&&, &&)
DEFINE_BINARY_OP(operator||, ||)
DEFINE_BINARY_OP(operator>, >)
DEFINE_BINARY_OP(operator<, <)
DEFINE_BINARY_OP(operator>=, >=)
DEFINE_BINARY_OP(operator<=, <=)
DEFINE_BINARY_OP(operator==, ==)
DEFINE_BINARY_OP(operator!=, !=)
#undef DEFINE_BINARY_OP

#define DEFINE_UNARY_OP(OPNAME, OP)                                     \
    template <class A, enableif_<is_ra_pos_rank<A>, int> = 0>           \
    inline auto OPNAME(A && a)                                          \
    {                                                                   \
        return map([](auto && a) { return OP a; }, std::forward<A>(a)); \
    }
DEFINE_UNARY_OP(operator!, !)
DEFINE_UNARY_OP(operator+, +) // @TODO Make into nop.
DEFINE_UNARY_OP(operator-, -)
#undef DEFINE_UNARY_OP

// When OP(a) isn't found from ra::, the deduction from rank(0) -> scalar doesn't work.
// @TODO Cf [ref:examples/useret.C:0].
#define DEFINE_NAME_OP(OPNAME, OP)                                      \
    using ::OP;                                                         \
    template <class ... A, enableif_<ra_pos_and_any<A ...>, int> =0>    \
    inline auto OPNAME(A && ... a)                                      \
    {                                                                   \
        return map([](auto && ... a) { return OP(a ...); }, std::forward<A>(a) ...); \
    }                                                                   \
    template <class ... A, enableif_<ra_zero<A ...>, int> =0>           \
    inline auto OPNAME(A && ... a)                                      \
    {                                                                   \
        return OP(START(a) ...);                                        \
    }
DEFINE_NAME_OP(rel_error, rel_error)
DEFINE_NAME_OP(pow, pow)
DEFINE_NAME_OP(xI, xI)
DEFINE_NAME_OP(conj, conj)
DEFINE_NAME_OP(sqr, sqr)
DEFINE_NAME_OP(sqrm, sqrm)
DEFINE_NAME_OP(abs, abs)
DEFINE_NAME_OP(cos, cos)
DEFINE_NAME_OP(sin, sin)
DEFINE_NAME_OP(exp, exp)
DEFINE_NAME_OP(sqrt, sqrt)
DEFINE_NAME_OP(log, log)
DEFINE_NAME_OP(log1p, log1p)
DEFINE_NAME_OP(isfinite, isfinite)
DEFINE_NAME_OP(isnan, isnan)
DEFINE_NAME_OP(isinf, isinf)
#undef DEFINE_NAME_OP

#define DEFINE_NAME_OP(OPNAME, OP)                                      \
    using ::OP;                                                         \
    template <class ... A, enableif_<ra_pos_and_any<A ...>, int> =0>    \
    inline auto OPNAME(A && ... a)                                      \
    {                                                                   \
        return map([](auto && ... a) -> decltype(auto) { return OP(a ...); }, std::forward<A>(a) ...); \
    }                                                                   \
    template <class ... A, enableif_<ra_zero<A ...>, int> =0>           \
    inline decltype(auto) OPNAME(A && ... a)                            \
    {                                                                   \
        return OP(START(a) ...);                                        \
    }
DEFINE_NAME_OP(real_part, real_part)
DEFINE_NAME_OP(imag_part, imag_part)

template <class T, class A>
inline auto cast(A && a)
{
    return map([](auto && a) { return T(a); }, std::forward<A>(a));
}

// --------------------------------
// Ternary op
// --------------------------------

// needed when ? returns rvalue reference.
template <class T>
inline auto safe_rvalue(T & t) -> decltype(auto)
{
    return t;
}

template <class T>
inline auto safe_rvalue(T && t) -> T
{
    return t;
}

// @TODO use instead
// typename safe_rvalue<decltype(t_), decltype(f_), decltype(w_ ? std::forward<decltype(t_)>(t_) : std::forward<decltype(f_)>(f_))>::type

// template <class T, class F, class R, class Enable=void>
// struct safe_rvalue
// {
//     using type = R;
// };

// template <class T, class F, class R>
// struct safe_rvalue<T, F, R, enableif_<mp::And<std::is_rvalue_reference<T>, std::is_rvalue_reference<F> > > >
// {
//     using type = std::decay_t<R>;
// };

// http://scottmeyers.blogspot.ch/2013/05/c14-lambdas-and-perfect-forwarding.html
// @TODO only one of t or f should be evaluated. But that probably requires where() to be a different type from Expr.
template <class W, class T, class F, enableif_<mp::Or<is_ra_pos_rank<W>, is_ra_pos_rank<T>, is_ra_pos_rank<F> >, int> = 0>
inline auto where(W && w, T && t, F && f)
{
    return map([](auto && w_, auto && t_, auto && f_) -> decltype(auto)
                {
                    return safe_rvalue(w_ ? std::forward<decltype(t_)>(t_) : std::forward<decltype(f_)>(f_));
                },
                std::forward<W>(w), std::forward<T>(t), std::forward<F>(f));
}

// --------------------------------
// Some whole-array reductions.
// --------------------------------

// @TODO First rank reductions? Variable rank reductions?
// @TODO Give return type once. gcc gives trouble.
template <class A>
inline auto reduce_sqrm(A && a) -> std::decay_t<decltype(sqrm(START(a)))>
{
    std::decay_t<decltype(sqrm(START(a)))> c(0.);
    for_each([&c](auto a) { c += sqrm(a); }, a);
    return c;
}

template <class A>
inline auto norm2(A && a)
{
    return sqrt(reduce_sqrm(a));
}

using std::min;
using std::max;

template <class A>
inline auto amin(A && a) -> std::decay_t<decltype(START(a))>
{
    using T = std::decay_t<decltype(START(a))>;
    T c(std::numeric_limits<T>::max());
    for_each([&c](auto && a) { c = min(c, a); }, a);
    return c;
}

template <class A>
inline auto amax(A && a) -> std::decay_t<decltype(START(a))>
{
    using T = std::decay_t<decltype(START(a))>;
    T c(std::numeric_limits<T>::lowest());
    for_each([&c](auto && a) { c = max(c, a); }, a);
    return c;
}

template <class A>
inline auto sum(A && a) -> std::decay_t<decltype(START(a))>
{
    std::decay_t<decltype(START(a))> c(0.);
    for_each([&c](auto && a) { c += a; }, a);
    return c;
}

template <class A>
inline auto prod(A && a) -> std::decay_t<decltype(START(a))>
{
    std::decay_t<decltype(START(a))> c(1.);
    for_each([&c](auto && a) { c *= a; }, a);
    return c;
}

template <class A, class B>
inline auto dot(A && a, B && b) -> std::decay_t<decltype(START(a) * START(b))>
{
    std::decay_t<decltype(START(a) * START(b))> c(0.);
    for_each([&c](auto && a, auto && b) { c = fma(a, b, c); }, a, b);
    return c;
}

template <class A, class B>
inline auto cdot(A && a, B && b) -> std::decay_t<decltype(conj(START(a)) * START(b))>
{
    std::decay_t<decltype(conj(START(a)) * START(b))> c(0.);
    for_each([&c](auto && a, auto && b) { c = fma_conj(a, b, c); }, a, b);
    return c;
}

// --------------------
// Wedge product
// @TODO Handle the simplifications dot_plus, yields_scalar, etc. just as vec::wedge does.
// --------------------

template <class A>
struct torank1
{
    using type = mp::If_<is_scalar<A>::value, Small<std::decay_t<A>, 1>, A>;
};

template <class Wedge, class Va, class Vb, class Enable=void>
struct fromrank1
{
    using valtype = typename Wedge::template valtype<Va, Vb>;
    using type = mp::If_<Wedge::Nr==1, valtype, Small<valtype, Wedge::Nr> >;
};

#define DECL_WEDGE(condition)                                           \
    template <int D, int Oa, int Ob, class Va, class Vb,                \
              enableif_<mp::Not<mp::And<is_scalar<Va>, is_scalar<Vb>>>, int> =0> \
    decltype(auto) wedge(Va const & a, Vb const & b)
DECL_WEDGE(general_case)
{
    Small<std::decay_t<decltype(START(a))>, decltype(start(a))::size_s()> aa = a;
    Small<std::decay_t<decltype(START(b))>, decltype(start(b))::size_s()> bb = b;

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

#define DECL_WEDGE(condition)                                      \
    template <int D, int Oa, int Ob, class Va, class Vb, class Vr, \
              enableif_<mp::Not<mp::And<is_scalar<Va>, is_scalar<Vb>>>, int> =0> \
    void wedge(Va const & a, Vb const & b, Vr & r)
DECL_WEDGE(general_case)
{
    Small<std::decay_t<decltype(START(a))>, decltype(start(a))::size_s()> aa = a;
    Small<std::decay_t<decltype(START(b))>, decltype(start(b))::size_s()> bb = b;

    using Ua = decltype(aa);
    using Ub = decltype(bb);

    auto & r1 = reinterpret_cast<typename torank1<decltype(r)>::type &>(r);
    auto & a1 = reinterpret_cast<typename torank1<Ua>::type const &>(aa);
    auto & b1 = reinterpret_cast<typename torank1<Ub>::type const &>(bb);
    fun::Wedge<D, Oa, Ob>::product(a1, b1, r1);
}
#undef DECL_WEDGE

template <class A, class B, std::enable_if_t<std::decay_t<A>::size_s()==2 && std::decay_t<B>::size_s()==2, int> =0>
inline auto cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(START(a_))>, 2> a = a_;
    Small<std::decay_t<decltype(START(b_))>, 2> b = b_;
    Small<std::decay_t<decltype(START(a_) * START(b_))>, 1> r;
    fun::Wedge<2, 1, 1>::product(a, b, r);
    return r[0];
}

template <class A, class B, std::enable_if_t<std::decay_t<A>::size_s()==3 && std::decay_t<B>::size_s()==3, int> =0>
inline auto cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(START(a_))>, 3> a = a_;
    Small<std::decay_t<decltype(START(b_))>, 3> b = b_;
    Small<std::decay_t<decltype(START(a_) * START(b_))>, 3> r;
    fun::Wedge<3, 1, 1>::product(a, b, r);
    return r;
}

template <class V>
inline auto perp(V const & v)
{
    static_assert(v.size()==2, "dimension error");
    return Small<std::decay_t<decltype(START(v))>, 2> {v[1], -v[0]};
}
template <class V, class U, enableif_<is_scalar<U>, int> =0>
inline auto perp(V const & v, U const & n)
{
    static_assert(v.size()==2, "dimension error");
    return Small<std::decay_t<decltype(START(v) * n)>, 2> {v[1]*n, -v[0]*n};
}
template <class V, class U, std::enable_if_t<!is_scalar<U>::value, int> =0>
inline auto perp(V const & v, U const & n)
{
    static_assert(v.size()==3, "dimension error");
    return cross(v, n);
}

// --------------------
// Other whole-array ops. @TODO Cleaner way to get concrete types from maybe-exprs. (expr:: had resolve()).
// --------------------

// @TODO Would like to be able to return just a/norm2(a) without temp-ref errors.
template <class A, enableif_<is_slice<A>, int> =0>
inline auto normv(A const & a)
{
    static_assert(A::size_s()!=DIM_ANY, "waiting for ra::resolve or some such"); // gcc accepts a.size_s()
    using V = Small<std::decay_t<decltype(START(a))>, A::size_s()>;
    return V(a/norm2(a));
}

template <class A, std::enable_if_t<!is_slice<A>::value && is_ra<A>::value, int> =0>
inline auto normv(A const & a)
{
    static_assert(A::size_s()!=DIM_ANY, "waiting for ra::resolve or some such");  // gcc accepts a.size_s()
    using V = Small<std::decay_t<decltype(START(a))>, A::size_s()>;
    V b = a;
    b /= norm2(b);
    return b;
}

// @TODO Define within generalization...
// @TODO Plain auto causes problems witn transform() in mat-fun.H:transform().
template <class A, class B>
inline ra::Small<std::decay_t<decltype(START(std::declval<A>()) * START(std::declval<B>()))>, A::size(0), B::size(1)>
gemm(A const & a, B const & b)
{
    constexpr int M = a.size(0);
    constexpr int N = b.size(1);
    ra::Small<std::decay_t<decltype(START(std::declval<A>()) * START(std::declval<B>()))>, A::size(0), B::size(1)> c;
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(ra::all, j));
        }
    }
    return c;
}

// the default for row-major x row-major. See bench-ra-mmul.C for variants.
template <class S, class T>
inline auto
gemm(ra::Raw<S, 2> const & a, ra::Raw<T, 2> const & b)
{
    int const M = a.size(0);
    int const N = b.size(1);
    int const K = a.size(1);
    ra::Owned<std::decay_t<decltype(START(a)*START(b))>, 2> c({M, N}, 0.);
    for (int k=0; k<K; ++k) {
        c += from(times(), a(ra::all, k), b(k, ra::all));
    }
    return c;
}

template <class A, class B>
inline ra::Small<std::decay_t<decltype(START(std::declval<A>()) * START(std::declval<B>()))>, B::size(1)>
gevm(A const & a, B const & b)
{
    constexpr int N = b.size(1);
    ra::Small<std::decay_t<decltype(START(std::declval<A>()) * START(std::declval<B>()))>, B::size(1)> c;
    for (int j=0; j<N; ++j) {
        c(j) = dot(a, b(ra::all, j));
    }
    return c;
}

template <class S, class T>
inline auto
gevm(ra::Raw<S, 1> const & a, ra::Raw<T, 2> const & b)
{
    int const N = b.size(1);
    ra::Owned<decltype(a(0)*b(0, 0))> c({N}, ra::unspecified);
    for (int j=0; j<N; ++j) {
        c(j) = dot(a, b(ra::all, j));
    }
    return c;
}

template <class A, class B>
inline ra::Small<std::decay_t<decltype(START(std::declval<A>()) * START(std::declval<B>()))>, A::size(0)>
gemv(A const & a, B const & b)
{
    constexpr int M = a.size(0);
    ra::Small<std::decay_t<decltype(START(std::declval<A>()) * START(std::declval<B>()))>, A::size(0)> c;
    for (int i=0; i<M; ++i) {
        c(i) = dot(a(i), b);
    }
    return c;
}

template <class S, class T>
inline auto
gemv(ra::Raw<S, 2> const & a, ra::Raw<T, 1> const & b)
{
    int const M = a.size(0);
    ra::Owned<std::decay_t<decltype(START(a)*START(b))>, 1> c({M}, ra::unspecified);
    for (int i=0; i<M; ++i) {
        c(i) = dot(a(i), b);
    }
    return c;
}

#undef START

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_OPERATORS

#endif // RA_OPERATORS_H
