
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

#ifdef EK_CHECK_BOUNDS
    #define EK_CHECK_BOUNDS_RA_OPERATORS EK_CHECK_BOUNDS
#else
    #ifndef EK_CHECK_BOUNDS_RA_OPERATORS
        #define EK_CHECK_BOUNDS_RA_OPERATORS 1
    #endif
#endif
#if EK_CHECK_BOUNDS_RA_OPERATORS==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

// See ::wedge() in src/base/wedge.H. Used to be in src/base/int_t.H, but I don't have access to ra::is_scalar<> there. @TODO A solution would be a partial traits class (to be fully defined in ra::) at the level of src/base/int_t.H or src/real.H.
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
// 1. define these in a class that ArrayIterator or Container or Slice types derive from. This was done for the old expr:: operators (src/base/vector-ops.H). It results in the narrowest scope, but since those types are used in the definition (ra::expr is an ArrayIterator), it requires lots of forwarding and traits:: .
// 2. raw ADL doesn't work because some ra:: types use ! != etc for different things (e.g. FlatIterator). Possible solution: don't ever use + != == for FlatIterator.
// 3. enable_if'd ADL is what you see here.

template <class A, class B>
struct ra_pos_and_any
{
    static bool const value = mp::And<is_ra_pos_rank<A>, mp::Or<is_ra<B>, is_scalar<B> > >::value;
};

template <class A, class B>
struct ra_zero_and_scalar
{
    static bool const value = mp::And<is_ra_zero_rank<A>, is_scalar<B> >::value;
};

// --------------------------------
// Binary ops
// --------------------------------

// We need the zero/scalar specializations because the scalar/scalar operators
// maybe be templated (e.g. complex<>), so they won't be found when an implicit
// conversion from zero->scalar is also needed. That is, without those
// specializations, ra::Raw<complex, 0> * complex will fail.
// @TODO Probably can simplify a bit...
#define DEFINE_BINARY_OP(OPNAME, OP)                                    \
    template <class A, class B,                                         \
              enableif_<mp::Or<ra_pos_and_any<A, B>, ra_pos_and_any<B, A> >, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return expr([](auto && a, auto && b) { return a OP b; }, \
                    start(std::forward<A>(a)), start(std::forward<B>(b))); \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero_and_scalar<A, B>, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return *(start(a).flat()) OP b;                                 \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero_and_scalar<B, A>, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return a OP *(start(b).flat());                                 \
    }                                                                   \
    template <class A, class B, enableif_<mp::And<is_ra_zero_rank<A>, is_ra_zero_rank<B>>, int> =0> \
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

#define DEFINE_BINARY_NAME_OP(OPNAME, OP)                               \
    using ::OP;                                                         \
    template <class A, class B,                                         \
              enableif_<mp::Or<ra_pos_and_any<A, B>, ra_pos_and_any<B, A> >, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return expr([](auto && a, auto && b) { return OP(a, b); },      \
                    start(std::forward<A>(a)), start(std::forward<B>(b))); \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero_and_scalar<A, B>, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return OP(*(start(a).flat()), b);                               \
    }                                                                   \
    template <class A, class B, enableif_<ra_zero_and_scalar<B, A>, int> =0> \
    auto OPNAME(A && a, B && b)                                         \
    {                                                                   \
        return OP(a, *(start(b).flat()));                               \
    }                                                                   \
    template <class A, class B, enableif_<mp::And<is_ra_zero_rank<A>, is_ra_zero_rank<B>>, int> =0> \
                           auto OPNAME(A && a, B && b)                  \
    {                                                                   \
        return OP(*(start(a).flat()), *(start(b).flat()));              \
    }
DEFINE_BINARY_NAME_OP(rel_error, rel_error)
DEFINE_BINARY_NAME_OP(pow, pow)
#undef DEFINE_BINARY_NAME_OP

// --------------------------------
// Unary ops
// --------------------------------

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

#define DEFINE_UNARY_NAME_OP(OPNAME, OP)                                \
    template <class A, enableif_<is_ra_pos_rank<A>, int> = 0>           \
    auto OPNAME(A && a)                                                 \
    {                                                                   \
        return expr([](auto && a) { return OP(a); }, start(std::forward<A>(a)));  \
    }
DEFINE_UNARY_NAME_OP(to_int, int)
DEFINE_UNARY_NAME_OP(to_uint, uint) // @TODO for vec:: ra:: size type clashes, remove.
DEFINE_UNARY_NAME_OP(to_bool, bool)
DEFINE_UNARY_NAME_OP(to_double, double)
#undef DEFINE_UNARY_NAME_OP

// When OP(a) isn't found from ra::, the deduction from rank(0) -> scalar doesn't work.
#define DEFINE_UNARY_NAME_NAMESPACED_OP(OP)                             \
    using ::OP;                                                         \
    template <class A, enableif_<is_ra_pos_rank<A>, int> = 0>           \
    auto OP(A && a)                                                     \
    {                                                                   \
        return expr([](auto && a) { return OP(a); }, start(std::forward<A>(a))); \
    }                                                                   \
    template <class A, enableif_<is_ra_zero_rank<A>, int> = 0>          \
    auto OP(A && a)                                                     \
    {                                                                   \
        return OP(*(start(a).flat()));                                  \
    }
DEFINE_UNARY_NAME_NAMESPACED_OP(xI)
DEFINE_UNARY_NAME_NAMESPACED_OP(conj)
DEFINE_UNARY_NAME_NAMESPACED_OP(sqr)
DEFINE_UNARY_NAME_NAMESPACED_OP(sqrm)
DEFINE_UNARY_NAME_NAMESPACED_OP(abs)
DEFINE_UNARY_NAME_NAMESPACED_OP(real_part)
DEFINE_UNARY_NAME_NAMESPACED_OP(imag_part)
DEFINE_UNARY_NAME_NAMESPACED_OP(cos)
DEFINE_UNARY_NAME_NAMESPACED_OP(sin)
DEFINE_UNARY_NAME_NAMESPACED_OP(exp)
DEFINE_UNARY_NAME_NAMESPACED_OP(sqrt)
DEFINE_UNARY_NAME_NAMESPACED_OP(log)
DEFINE_UNARY_NAME_NAMESPACED_OP(log1p)
DEFINE_UNARY_NAME_NAMESPACED_OP(isfinite)
DEFINE_UNARY_NAME_NAMESPACED_OP(isnan)
DEFINE_UNARY_NAME_NAMESPACED_OP(isinf)
#undef DEFINE_UNARY_NAME_NAMESPACED_OP

// --------------------------------
// Ternary op
// --------------------------------

// http://scottmeyers.blogspot.ch/2013/05/c14-lambdas-and-perfect-forwarding.html
// @TODO only one of t or f should be evaluated. But that probably requires where() to be a different type from Expr.
template <class W, class T, class F, enableif_<mp::Or<is_ra_pos_rank<W>, is_ra_pos_rank<T>, is_ra_pos_rank<F> >, int> = 0>
auto where(W && w, T && t, F && f)
{
    return expr([](auto && w_, auto && t_, auto && f_) -> decltype(auto)
                {
                    // std::cout << "w_ is " << w_ << std::endl;
                    // std::cout << "returns: " << (w_ ? std::forward<decltype(t_)>(t_) : std::forward<decltype(f_)>(f_)) << std::endl;
                    return w_ ? std::forward<decltype(t_)>(t_) : std::forward<decltype(f_)>(f_);
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

// @TODO Have it work for ArrayIterator. Try reusing ply_either.
// is_foreign_vector b/c std::vector or std::array may be used as shape_type.
template <class C> inline
enableif_<mp::Or<is_slice<C>, is_foreign_vector<C>>, std::ostream &>
operator<<(std::ostream & o, C && c)
{
    auto s = ra_traits<C>::shape(c);
    using Shape = decltype(s);
    Shape i(ra_traits<Shape>::make(s.size(), 0));

    if (ra_traits<C>::size_s()==DIM_ANY) {
        o << s << "\n";
    }
    auto a = c.begin();
    if (a==c.end()) {
        return o;
    }
// @BUG carrying index twice. Iterator should suffice.
    rank_t const rank = ra_traits<C>::rank(c);
    while (1) {
        o << *a << " ";
        ++a;
        for (rank_t j=rank-1; j>=0; --j) {
            if (++i[j]<s[j]) {
                std::fill_n(std::ostream_iterator<char>(o, ""), rank-j-1, '\n');
                goto next;
            } else {
                i[j] = 0;
            }
        }
        return o;
next: ;
    }
}

// @TODO Fix formatting, should be as ostream & << Container or Slice
template <class A> inline
enableif_<is_array_iterator<A>, std::ostream &>
operator<<(std::ostream & o, A && a)
{
    if (std::decay_t<A>::size_s()==DIM_ANY) {
        o << a.shape() << "\n";
    }
// @BUG Rely on ply_either using row-major.
    ply_either(expr([&o](auto && a) { o << a << " "; }, a));
    return o;
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
    size_t n;
    i >> n;
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
#undef EK_CHECK_BOUNDS_RA_OPERATORS

#endif // RA_OPERATORS_H
