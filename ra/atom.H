
// (c) Daniel Llorens - 2011-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file atom.H
/// @brief Terminal nodes for expression templates, and use-as-xpr wrapper.

#pragma once
#include "ra/opcheck.H"
#include "ra/type.H"

#ifndef RA_CHECK_BOUNDS_ATOM
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_ATOM 1
  #else
    #define RA_CHECK_BOUNDS_ATOM RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_ATOM==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

// value_type may be needed to avoid conversion issues.
template <int w_, class value_type=ra::dim_t>
struct TensorIndex
{
    static_assert(w_>=0, "bad TensorIndex");
    constexpr static int w = w_;
    constexpr static dim_t size(int k) { return DIM_BAD; } // used in shape checks with dyn. rank.
    constexpr static dim_t size_s() { return DIM_BAD; }
    constexpr static rank_t rank_s() { return w+1; }
    constexpr static rank_t rank() { return w+1; }

    template <class I> constexpr value_type at(I const & i) const { return value_type(i[w]); }
    constexpr void adv(rank_t k, dim_t d) {}
    constexpr dim_t stride(int i) const { assert(w<0); return 0; } // used by Expr::stride_t.
    constexpr value_type * flat() const { assert(w<0); return nullptr; } // used by type signatures.
};

#define DEF_TENSORINDEX(i) constexpr TensorIndex<i, int> JOIN(_, i) {};
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

template <class C> struct Scalar;

// Separate from Scalar so that operator+=, etc. has the array meaning there.
// We can reuse the Scalar object b/c operator+= is a no-op.
template <class C>
struct ScalarFlat: public Scalar<C>
{
    constexpr void operator+=(dim_t d) const {}
    constexpr C const & operator*() const { return this->c; }
    constexpr C & operator*() { return this->c; }
};

// Wrap constant for traversal. We still want f(C) to be a specialization in most cases.
template <class C_>
struct Scalar
{
    using C = C_;
    C c;

// Used in shape checks with dynamic rank. (Never called because rank is 0).
    constexpr static dim_t size(int k) { assert(0); return 0; }
    constexpr static dim_t size_s() { return 1; }
    constexpr static rank_t rank() { return 0; }
    constexpr static rank_t rank_s() { return 0; }
    using shape_type = std::array<dim_t, 0>;
    constexpr static shape_type shape() { return shape_type {}; }

    template <class I> constexpr C const & at(I const & i) const { return c; }
    template <class I> constexpr C & at(I const & i) { return c; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t stride(int i) { return 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return true; }
    constexpr decltype(auto) flat() const { return static_cast<ScalarFlat<C> const &>(*this); }
    constexpr decltype(auto) flat() { return static_cast<ScalarFlat<C> &>(*this); }

#define DEF_ASSIGNOPS(OP) template <class X> void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

// For the use of std::forward<>, see eg http://www.justsoftwaresolutions.co.uk/cplusplus/rvalue_references_and_perfect_forwarding.html
template <class C> inline constexpr auto
scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

// Wrap something with {size, begin} as 1-D vector. Sort of reduced ra_iterator.
// ra::ra_traits_def<V> must be defined with ::size, ::size_s.
// FIXME This can handle temporaries and make_a().begin() can't, look out for that.
// FIXME Do we need this class? holding rvalue is the only thing it does over View, and it doesn't handle rank!=1.
template <class V_>
struct Vector
{
    using V = V_;
    static_assert(has_traits<V>, "bad type for Vector"); // FIXME the next line should bomb by itself
    using traits = ra_traits<V>;

    V v;
    decltype(v.begin()) p__;
    static_assert(!std::is_reference_v<decltype(p__)>, "bad iterator type");

    constexpr dim_t size() const { return traits::size(v); }
    constexpr dim_t size(int i) const { CHECK_BOUNDS(i==0); return traits::size(v); }
    constexpr static dim_t size_s() { return traits::size_s(); }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };
    using shape_type = std::array<dim_t, 1>;
    constexpr auto shape() const { return shape_type { dim_t(traits::size(v)) }; }

// start() doesn't do this, but FIXME? Expr, etc. still does. See [ra35] in test-ra-9.C.
// FIXME these should never be called to convert Vector<V> to Vector<V&> etc.
    Vector(Vector<std::remove_reference_t<V>> const & a): v(std::move(a.v)), p__(v.begin()) {}; // looks like shouldn't happen
    Vector(Vector<std::remove_reference_t<V>> && a): v(std::move(a.v)), p__(v.begin()) {};
    Vector(Vector<std::remove_reference_t<V> &> const & a): v(a.v), p__(v.begin()) {};
    Vector(Vector<std::remove_reference_t<V> &> && a): v(a.v), p__(v.begin()) {};

// see test-compatibility.C [a1] for forward() here.
    Vector(V && v_): v(std::forward<V>(v_)), p__(v.begin()) {}

    template <class I>
    decltype(auto) at(I const & i)
    {
        CHECK_BOUNDS(inside(i[0], this->size()));
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
// k>0 happens on frame-matching when the axes k>0 can't be unrolled; see [trc-01] in test-compatibility.C.
// k==0 && d!=1 happens on turning back at end of ply; TODO we need this only on outer products and such.
        CHECK_BOUNDS(d==1 || d<0);
        p__ += (k==0) * d;
    }
    constexpr static dim_t stride(int i) { return i==0 ? 1 : 0; }
// reduced from cell_iterator::keep_stride()
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() { return p__; }
    constexpr auto flat() const { return p__; }

#define DEF_ASSIGNOPS(OP) template <class X> void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class V> inline constexpr auto
vector(V && v) { return Vector<V>(std::forward<V>(v)); }

// Like Vector, but on the iterator itself, so no size, P only needs to have +=, *, [].
// ra::ra_traits_def<P> doesn't need to be defined.
template <class P>
struct Ptr
{
    P p__;

    constexpr static dim_t size() { return DIM_BAD; }
    constexpr static dim_t size(int i) { CHECK_BOUNDS(i==0); return DIM_BAD; }
    constexpr static dim_t size_s() { return DIM_BAD; }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };
    using shape_type = std::array<dim_t, 1>;
    constexpr static auto shape() { return shape_type { dim_t(DIM_BAD) }; }

    template <class I>
    constexpr decltype(auto) at(I && i)
    {
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        CHECK_BOUNDS(d==1 || d<0); // cf Vector::adv
        p__ += (k==0) * d;
    }
    constexpr static dim_t stride(int i) { return i==0 ? 1 : 0; }
// reduced from cell_iterator::keep_stride()
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() { return p__; }
    constexpr auto flat() const { return p__; }

#define DEF_ASSIGNOPS(OP) template <class X> void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class I,
          std::enable_if_t<std::is_same_v<typename std::iterator_traits<std::decay_t<I>>::iterator_category,
                                          std::random_access_iterator_tag>, int> =0>
inline auto ptr(I i) { return Ptr<I> { i }; } // FIXME forwarder that moves if value, copies if ref ?

template <class T>
struct IotaFlat
{
    T i_;
    T const stride_;
    T const & operator*() const { return i_; } // TODO if not for this, I could use plain T. Maybe ra::eval_expr...
    void operator+=(dim_t d) { i_ += T(d)*stride_; }
};

template <class T_>
struct Iota
{
    using T = T_;
    dim_t const size_;
    T const org_;
    T const stride_;
    T i_;

    constexpr Iota(dim_t size, T org=0, T stride=1): size_(size), org_(org), stride_(stride), i_(org)
    {
        CHECK_BOUNDS(size>=0);
    }

    constexpr dim_t size() const { return size_; } // this is a Slice function...
    constexpr dim_t size(int i) const { CHECK_BOUNDS(i==0); return size_; }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr rank_t rank() const { return 1; }
    constexpr static rank_t rank_s() { return 1; };
    using shape_type = std::array<dim_t, 1>;
    constexpr auto shape() const { return shape_type { size_ }; }

    template <class I>
    constexpr decltype(auto) at(I const & i)
    {
        return org_ + T(i[0])*stride_;
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        i_ += T((k==0) * d) * stride_; // cf Vector::adv
    }
    constexpr static dim_t stride(rank_t i) { return i==0 ? 1 : 0; }
// reduced from cell_iterator::keep_stride()
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return IotaFlat<T> { i_, stride_ }; }
};

template <class O=dim_t, class S=O> inline constexpr auto
iota(dim_t size, O org=0, S stride=1)
{
    using T = std::common_type_t<O, S>;
    return Iota<T> { size, T(org), T(stride) };
}

template <class I> struct is_beatable_def
{
    constexpr static bool value = std::is_integral_v<I>;
    constexpr static int skip_src = 1;
    constexpr static int skip = 0;
    constexpr static bool static_p = value; // can the beating be resolved statically?
};

template <class II> struct is_beatable_def<Iota<II>>
{
    constexpr static bool value = std::numeric_limits<II>::is_integer;
    constexpr static int skip_src = 1;
    constexpr static int skip = 1;
    constexpr static bool static_p = false; // it cannot for Iota
};

template <int n> struct is_beatable_def<dots_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = n;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <int n> struct is_beatable_def<newaxis_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = 0;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <class I> using is_beatable = is_beatable_def<std::decay_t<I>>;

template <class X> constexpr bool has_tensorindex_def = false;
template <class T> constexpr bool has_tensorindex = has_tensorindex_def<std::decay_t<T>>;
template <int w, class value_type> constexpr bool has_tensorindex_def<TensorIndex<w, value_type>> = true;

template <class Op, class T, class K=std::make_integer_sequence<int, mp::len<T>>> struct Expr;
template <class T, class K=std::make_integer_sequence<int, mp::len<T>>> struct Pick;
template <class FM, class Op, class T, class K=std::make_integer_sequence<int, mp::len<T>>> struct Ryn;
template <class LiveAxes, int depth, class A> struct ApplyFrames;

template <class Op, class ... Ti, class K>
constexpr bool has_tensorindex_def<Expr<Op, std::tuple<Ti ...>, K>> = (has_tensorindex<Ti> || ...);
template <class ... Ti, class K>
constexpr bool has_tensorindex_def<Pick<std::tuple<Ti ...>, K>> = (has_tensorindex<Ti> || ...);
template <class LiveAxes, int depth, class A>
constexpr bool has_tensorindex_def<ApplyFrames<LiveAxes, depth, A>> = has_tensorindex<A>;
template <class FM, class Op, class ... Ti, class K>
constexpr bool has_tensorindex_def<Ryn<FM, Op, std::tuple<Ti ...>, K>> = (has_tensorindex<Ti> || ...);

// --------------
// Coerce potential ArrayIterators
// --------------

template <class T, int a>
inline constexpr auto start(T && t)
{
    static_assert(!mp::exists<T>, "bad type for ra:: operator");
}

RA_IS_DEF(is_iota, (std::is_same_v<A, Iota<typename A::T>>))
RA_IS_DEF(is_ra_scalar, (std::is_same_v<A, Scalar<typename A::C>>))
RA_IS_DEF(is_ra_vector, (std::is_same_v<A, Vector<typename A::V>>))

// it matters that this copies when T is lvalue. But FIXME shouldn't be necessary. See [ra35] and Vector constructors above.
// FIXME start should always copy the iterator part but never the content part for Vector or Scalar (or eventually .iter()). Copying the iterator part is necessary if e.g. ra::cross() has to work with ArrayIterator args, since the arguments are start()ed twice.
template <class T, std::enable_if_t<is_iterator<T> && !is_ra_scalar<T> && !is_ra_vector<T>, int> =0>
inline constexpr auto start(T && t)
{
    return std::forward<T>(t);
}

// don't copy these, always forward.
template <class T, std::enable_if_t<is_ra_vector<T> || is_ra_scalar<T>, int> =0>
inline constexpr decltype(auto) start(T && t)
{
    return std::forward<T>(t);
}

template <class T, std::enable_if_t<is_slice<T>, int> =0>
inline constexpr auto start(T && t)
{
    return iter<0>(t); // BUG neither cell_iterator nor cell_iterator_small will retain rvalues. See test-ra-7.C [opt-small].
}

template <class T, std::enable_if_t<is_scalar<T>, int> =0>
inline constexpr auto start(T && t)
{
    return ra::scalar(std::forward<T>(t));
}

template <class T, std::enable_if_t<is_foreign_vector<T>, int> =0>
inline constexpr auto start(T && t)
{
    return ra::vector(std::forward<T>(t));
}

template <class T>
inline constexpr auto start(std::initializer_list<T> v)
{
    return Vector<std::initializer_list<T>> { std::move(v) };
}

// forward declare for expr.H; implemented in small.H.
template <class T, std::enable_if_t<is_builtin_array<T>, int> =0>
inline constexpr auto start(T && t);

// FIXME one of these is ET-generic and the other is slice only, so make up your mind.
// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using start_t = decltype(ra::start(std::declval<A>()));
template <class A> using flat_t = decltype(*(ra::start(std::declval<A>()).flat()));
template <class A> using value_t = std::decay_t<flat_t<A>>;
template <class V> inline auto size(V const & v) { return ra_traits<V>::size(v); }

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_ATOM
