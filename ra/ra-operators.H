
// (c) Daniel Llorens - 2014-2015

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

// See ::wedge() in ra/wedge.H, need ra::is_scalar<> to define.
// These ra::start are needed b/c rank 0 converts to and from scalar, so ? can't pick the right (-> scalar) conversion.
template <class T, class F,
          enableif_<mp::And<mp::Or<ra::is_scalar<T>, ra::is_ra_zero_rank<T> >,
                            mp::Or<ra::is_scalar<F>, ra::is_ra_zero_rank<F> > >, int> =0>
inline decltype(auto) where(bool const w, T && t, F && f) { return w ? *(ra::start(t).flat()) : *(ra::start(f).flat()); }

namespace ra {

// Wrappers over expr & ply_either.

template <class F, class ... A>
auto map(F && f, A && ... a) -> decltype(expr(std::forward<F>(f), start(std::forward<A>(a)) ...))
{
    return expr(std::forward<F>(f), start(std::forward<A>(a)) ...);
}

template <class F, class ... A>
void for_each(F && f, A && ... a)
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
// @TODO Probably can simplify a bit...
#define DEFINE_BINARY_OP(OPNAME, OP)                                    \
    template <class A, class B, enableif_<ra_pos_and_any<A, B>, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return expr([](auto && a, auto && b) { return a OP b; },        \
                    start(std::forward<A>(a)), start(std::forward<B>(b))); \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero_or_scalar<A, B>, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return *(start(a).flat()) OP *(start(b).flat());                \
    }
DEFINE_BINARY_OP(operator+, +)
DEFINE_BINARY_OP(operator-, -)
DEFINE_BINARY_OP(operator*, *)
DEFINE_BINARY_OP(operator/, /)
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
    auto OPNAME(A && a)                                                 \
    {                                                                   \
        return expr([](auto && a) { return OP a; }, start(std::forward<A>(a))); \
    }
DEFINE_UNARY_OP(operator!, !)
DEFINE_UNARY_OP(operator+, +) // @TODO Make into nop.
DEFINE_UNARY_OP(operator-, -)
#undef DEFINE_UNARY_OP

// When OP(a) isn't found from ra::, the deduction from rank(0) -> scalar doesn't work.
// @TODO Cf [ref:examples/useret.C:0] for examples.
#define DEFINE_NAME_OP(OPNAME, OP)                                      \
    using ::OP;                                                         \
    template <class ... A, enableif_<ra_pos_and_any<A ...>, int> =0>    \
    auto OPNAME(A && ... a)                                             \
    {                                                                   \
        return expr([](auto && ... a) { return OP(a ...); }, start(std::forward<A>(a)) ...); \
    }                                                                   \
    template <class ... A, enableif_<ra_zero_or_scalar<A ...>, int> =0> \
    auto OPNAME(A && ... a)                                             \
    {                                                                   \
        return OP(*(start(a).flat()) ...);                              \
    }
DEFINE_NAME_OP(rel_error, rel_error)
DEFINE_NAME_OP(pow, pow)
DEFINE_NAME_OP(xI, xI)
DEFINE_NAME_OP(conj, conj)
DEFINE_NAME_OP(sqr, sqr)
DEFINE_NAME_OP(sqrm, sqrm)
DEFINE_NAME_OP(abs, abs)
DEFINE_NAME_OP(real_part, real_part)
DEFINE_NAME_OP(imag_part, imag_part)
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

template <class T, class A>
auto cast(A && a)
{
    return expr([](auto && a) { return T(a); }, start(std::forward<A>(a)));
}

// --------------------------------
// Ternary op
// --------------------------------

// needed when ? returns rvalue reference.
template <class T>
auto safe_rvalue(T & t) -> decltype(auto)
{
    return t;
}

template <class T>
auto safe_rvalue(T && t) -> T
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
auto where(W && w, T && t, F && f)
{
    return expr([](auto && w_, auto && t_, auto && f_) -> decltype(auto)
                {
                    return safe_rvalue(w_ ? std::forward<decltype(t_)>(t_) : std::forward<decltype(f_)>(f_));
                },
                start(std::forward<W>(w)), start(std::forward<T>(t)), start(std::forward<F>(f)));
}

// --------------------------------
// Some whole-array reductions.
// --------------------------------

// @TODO First rank reductions? Variable rank reductions?
// @TODO Give return type once. gcc gives trouble.
template <class A>
auto reduce_sqrm(A && a) -> std::decay_t<decltype(sqrm(*(start(a).flat())))>
{
    std::decay_t<decltype(sqrm(*(start(a).flat())))> c(0.);
    ply_either(expr([&c](auto a) { c += sqrm(a); }, start(a)));
    return c;
}

template <class A>
auto norm2(A && a)
{
    return sqrt(reduce_sqrm(a));
}

using std::min;
using std::max;

#define START(a) *(start(a).flat())
template <class A>
auto amin(A && a) -> std::decay_t<decltype(START(a))>
{
    using T = std::decay_t<decltype(START(a))>;
    T c(std::numeric_limits<T>::max());
    ply_either(expr([&c](auto && a) { c = min(c, a); }, start(a)));
    return c;
}

template <class A>
auto amax(A && a) -> std::decay_t<decltype(START(a))>
{
    using T = std::decay_t<decltype(START(a))>;
    T c(std::numeric_limits<T>::lowest());
    ply_either(expr([&c](auto && a) { c = max(c, a); }, start(a)));
    return c;
}

template <class A>
auto sum(A && a) -> std::decay_t<decltype(START(a))>
{
    std::decay_t<decltype(START(a))> c(0.);
    ply_either(expr([&c](auto && a) { c += a; }, start(a)));
    return c;
}

template <class A>
auto prod(A && a) -> std::decay_t<decltype(START(a))>
{
    std::decay_t<decltype(START(a))> c(1.);
    ply_either(expr([&c](auto && a) { c *= a; }, start(a)));
    return c;
}

template <class A, class B>
auto dot(A && a, B && b) -> std::decay_t<decltype(START(a) * START(b))>
{
    std::decay_t<decltype(START(a) * START(b))> c(0.);
    ply_either(expr([&c](auto && a, auto && b) { c = fma(a, b, c); }, start(a), start(b)));
    return c;
}

template <class A, class B>
auto cdot(A && a, B && b) -> std::decay_t<decltype(conj(START(a)) * START(b))>
{
    std::decay_t<decltype(conj(START(a)) * START(b))> c(0.);
    ply_either(expr([&c](auto && a, auto && b) { c = fma_conj(a, b, c); }, start(a), start(b)));
    return c;
}
#undef START

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
              disableif_<mp::And<is_scalar<Va>, is_scalar<Vb> >, int> =0> \
    decltype(auto) wedge(Va const & a, Vb const & b)
DECL_WEDGE(general_case)
{
    Small<std::decay_t<decltype(*(start(a).flat()))>, decltype(start(a))::size_s()> aa = a;
    Small<std::decay_t<decltype(*(start(b).flat()))>, decltype(start(b))::size_s()> bb = b;

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
              disableif_<mp::And<is_scalar<Va>, is_scalar<Vb> >, int> =0> \
    void wedge(Va const & a, Vb const & b, Vr & r)
DECL_WEDGE(general_case)
{
    Small<std::decay_t<decltype(*(start(a).flat()))>, decltype(start(a))::size_s()> aa = a;
    Small<std::decay_t<decltype(*(start(b).flat()))>, decltype(start(b))::size_s()> bb = b;

    using Ua = decltype(aa);
    using Ub = decltype(bb);

    auto & r1 = reinterpret_cast<typename torank1<decltype(r)>::type &>(r);
    auto & a1 = reinterpret_cast<typename torank1<Ua>::type const &>(aa);
    auto & b1 = reinterpret_cast<typename torank1<Ub>::type const &>(bb);
    fun::Wedge<D, Oa, Ob>::product(a1, b1, r1);
}
#undef DECL_WEDGE

template <class A, class B, enableifc_<std::decay_t<A>::size_s()==2 && std::decay_t<B>::size_s()==2, int> =0>
auto cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(*(start(a_).flat()))>, 2> a = a_;
    Small<std::decay_t<decltype(*(start(b_).flat()))>, 2> b = b_;
    Small<std::decay_t<decltype(*(start(a_).flat()) * *(start(b_).flat()))>, 1> r;
    fun::Wedge<2, 1, 1>::product(a, b, r);
    return r[0];
}

template <class A, class B, enableifc_<std::decay_t<A>::size_s()==3 && std::decay_t<B>::size_s()==3, int> =0>
auto cross(A const & a_, B const & b_)
{
    Small<std::decay_t<decltype(*(start(a_).flat()))>, 3> a = a_;
    Small<std::decay_t<decltype(*(start(b_).flat()))>, 3> b = b_;
    Small<std::decay_t<decltype(*(start(a_).flat()) * *(start(b_).flat()))>, 3> r;
    fun::Wedge<3, 1, 1>::product(a, b, r);
    return r;
}

template <class V>
auto perp(V const & v)
{
    static_assert(v.size()==2, "dimension error");
    return Small<std::decay_t<decltype(*(start(v).flat()))>, 2> {v[1], -v[0]};
}
template <class V, class U, enableif_<is_scalar<U>, int> =0>
auto perp(V const & v, U const & n)
{
    static_assert(v.size()==2, "dimension error");
    return Small<std::decay_t<decltype(*(start(v).flat()) * n)>, 2> {v[1]*n, -v[0]*n};
}
template <class V, class U, enableifc_<!is_scalar<U>::value, int> =0>
auto perp(V const & v, U const & n)
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
    using V = Small<std::decay_t<decltype(*(start(a).flat()))>, A::size_s()>;
    return V(a/norm2(a));
}

template <class A, enableifc_<!is_slice<A>::value && is_ra<A>::value, int> =0>
inline auto normv(A const & a)
{
    static_assert(A::size_s()!=DIM_ANY, "waiting for ra::resolve or some such");  // gcc accepts a.size_s()
    using V = Small<std::decay_t<decltype(*(start(a).flat()))>, A::size_s()>;
    V b = a;
    b /= norm2(b);
    return b;
}

// @TODO Define within generalization...
// @TODO Plain auto causes problems witn transform() in mat-fun.H:transform().
template <class A, class B>
ra::Small<decltype(std::declval<A>()(0, 0)*std::declval<B>()(0, 0)), A::size(0), B::size(1)>
mm_mul(A const & a, B const & b)
{
    constexpr int M = a.size(0);
    constexpr int N = b.size(1);
    ra::Small<decltype(a(0, 0)*b(0, 0)), M, N> c;
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(ra::all, j));
        }
    }
    return c;
}

// A temporary for smatrix:: functions, @TODO refactor. @TODO loop over rank 1 pieces.
template <class S, class T>
inline auto
mm_mul(ra::Raw<S, 2> const & a, ra::Raw<T, 2> const & b)
{
    int const M = a.size(0);
    int const N = b.size(1);
    ra::Owned<decltype(a(0, 0)*b(0, 0)), 2> c({M, N}, ra::default_init);
    for (int i=0; i<M; ++i) {
        for (int j=0; j<N; ++j) {
            c(i, j) = dot(a(i), b(ra::all, j));
        }
    }
    return c;
}

template <class A, class B>
inline ra::Small<decltype(std::declval<A>()(0)*std::declval<B>()(0, 0)), B::size(1)>
vm_mul(A const & a, B const & b)
{
    constexpr int N = b.size(1);
    ra::Small<decltype(a(0)*b(0, 0)), N> c;
    for (int j=0; j<N; ++j) {
        c(j) = dot(a, b(ra::all, j));
    }
    return c;
}

template <class S, class T>
inline auto
vm_mul(ra::Raw<S, 1> const & a, ra::Raw<T, 2> const & b)
{
    int N = b.size(1);
    ra::Owned<decltype(a(0)*b(0, 0))> c({N}, ra::default_init);
    for (int j=0; j<N; ++j) {
        c(j) = dot(a, b(ra::all, j));
    }
    return c;
}

template <class A, class B>
inline ra::Small<decltype(std::declval<A>()(0, 0)*std::declval<B>()(0)), A::size(0)>
mv_mul(A const & a, B const & b)
{
    constexpr int M = a.size(0);
    ra::Small<decltype(a(0, 0)*b(0)), M> c;
    for (int i=0; i<M; ++i) {
        c(i) = dot(a(i), b);
    }
    return c;
}

template <class S, class T>
inline auto
mv_mul(ra::Raw<S, 2> const & a, ra::Raw<T, 1> const & b)
{
    int M = a.size(0);
    ra::Owned<decltype(a(0, 0)*b(0)), 1> c({M}, ra::default_init);
    for (int i=0; i<M; ++i) {
        c(i) = dot(a(i), b);
    }
    return c;
}

// --------------------
// I/O operators.
// --------------------

// a version of ply_index(). @TODO Eventually try to merge with that.
// is_foreign_vector is included b/c std::vector or std::array may be used as shape_type.
template <class A> inline
enableif_<mp::Or<is_ra<A>, is_foreign_vector<A> >, std::ostream &>
operator<<(std::ostream & o, A && a_)
{
// work with ArrayIterator
    auto a = ra::start(std::forward<A>(a_));
    rank_t const rank = a.rank();
    auto sha(a.shape());
    if (a.size_s()==DIM_ANY) {
        o << sha << "\n";
    }
// @TODO Scalar doesn't have done(), so avoid that. Concepts aren't well specified...
    if (prod(sha)==0) {
        return o;
    }
    auto ind = ra_traits<decltype(sha)>::make(rank, 0);
// unlike in ply_index(), order here is row-major on purpose.
    for (;;) {
    next: ;
        o << a.at(ind) << " ";
        for (int k=0; k<rank; ++k) {
            if (++ind[rank-1-k]<sha[rank-1-k]) {
                std::fill_n(std::ostream_iterator<char>(o, ""), k, '\n');
                goto next;
            } else {
                ind[rank-1-k] = 0;
            }
        }
        return o;
    }
}

// Static size.
template <class C> inline
enableifc_<ra_traits<C>::size_s()!=DIM_ANY, std::istream &>
operator>>(std::istream & i, C & c)
{
    using T = typename ra_traits<C>::value_type;
    std::copy_n(std::istream_iterator<T>(i), c.size(), c.begin());
    return i;
}

// Special case for std::vector, to handle create-new / resize() difference.
template <class T, class A> inline
std::istream &
operator>>(std::istream & i, std::vector<T, A> & c)
{
    dim_t n;
    i >> n;
    assert(n>=0);
    if (i) {
        c.resize(n);
        std::copy_n(std::istream_iterator<T>(i), c.size(), c.begin());
    }
    return i;
}

// Expr size, so read shape and possibly allocate (@TODO try to avoid).
template <class C> inline
enableifc_<ra_traits<C>::size_s()==DIM_ANY, std::istream &>
operator>>(std::istream & i, C & c)
{
    using T = typename ra_traits<C>::value_type;
    typename ra_traits<C>::shape_type s;
    i >> s;
    if (i) {
        c = std::decay_t<C>(s, ra::default_init);
        if (c.size()>0) { // I'd say a bug in copy_n; see failing case in test-ra-nested.C.
            std::copy_n(std::istream_iterator<T>(i), c.size(), c.begin());
        }
    }
    return i;
}

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_OPERATORS

#endif // RA_OPERATORS_H
