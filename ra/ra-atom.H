
// (c) Daniel Llorens - 2011-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_ATOM_H
#define RA_ATOM_H

/// @file ra-atom.H
/// @brief Expression templates for arrays with frame matching: terminal nodes.

#include "ra/ra-traits.H"
#include "ra/ra-opcheck.H"

#ifdef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_RA_ATOM RA_CHECK_BOUNDS
#else
    #ifndef RA_CHECK_BOUNDS_RA_ATOM
        #define RA_CHECK_BOUNDS_RA_ATOM 1
    #endif
#endif
#if RA_CHECK_BOUNDS_RA_ATOM==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

#define BAD_INTERFACE(X) static_assert(X, "bad use of FlatIterator interface"); return 0;

// value_type may be needed to avoid conversion issues.
template <int w_, class value_type_=ra::dim_t>
struct TensorIndex
{
    using value_type = value_type_;

    static constexpr int w = w_;
    static_assert(w>=0, "bad TensorIndex");
    constexpr static rank_t rank_s() { return w+1; }
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t size_s() { return DIM_BAD; }

    void adv(rank_t const k, dim_t const d) {}
    template <class I> value_type at(I const & i) const
    {
        return value_type(i[w]);  // @CONVERSION
    }

// Used in shape checks with dynamic rank. Needs special marker...
    constexpr static dim_t size(int const k) { return DIM_BAD; }

// Used by ply_ravel(), which doesn't support TensorIndex.
    dim_t stride(int const i) const { BAD_INTERFACE(w<0) }
    bool compact_stride(dim_t const step, int const z, int const j) const { BAD_INTERFACE(w<0) }
    value_type * flat() const { BAD_INTERFACE(w<0) } // type signature used by Expr::atom_type
};

#undef BAD_INTERFACE

constexpr TensorIndex<0, int> _0 {};
constexpr TensorIndex<1, int> _1 {};
constexpr TensorIndex<2, int> _2 {};
constexpr TensorIndex<3, int> _3 {};

// Wrap a scalar for traversal.
// flat() returns self, so implements both the Expr and the Leaves interface. But it's never a driver, so it doesn't need e.g. done() (in Expr) or preferred_stride() (in Leaves).
template <class S>
struct Scalar
{
    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t size_s() { return 1; }

    using shape_type = std::array<dim_t, 0>;
    constexpr static shape_type shape() { return shape_type {}; }

    S const s;
    Scalar(S && s_): s(std::forward<S>(s_)) {}

    constexpr static void adv(rank_t const k, dim_t const d) {}
    template <class I> constexpr S const & at(I const & i) const { return s; }

// Used in shape checks with dynamic rank. (Never called because rank is 0).
    constexpr static dim_t size(int const k) { assert(0); return 0; }

// @TODO The result is never used, so maybe there's a way to remove this. @TODO All constexpr.
    constexpr static dim_t stride(int const i) { return 0; }
    constexpr static bool compact_stride(dim_t const step, int const z, int const j) { return true; }
    constexpr Scalar const & flat() const { return *this; }

// Scalar acts as driver in a few places where I didn't bother to specialize (e.g. ra::from). Elsewhere (e.g. in +, *, operator=, etc.) there's always another expr to be driver, or a specialization (e.g. array constructors). Use in ra::from should be removed since a scalar subscript can be beaten; this is probably true of all other uses, and @TODO then we could remove this.
    constexpr static dim_t preferred_stride(int const i) { return 0; }

    constexpr void operator+=(dim_t const d) const {}
    constexpr S const & operator*() const { return s; }
};

// For the use of std::forward<>, see eg http://www.justsoftwaresolutions.co.uk/cplusplus/rvalue_references_and_perfect_forwarding.html
template <class S>
Scalar<S> scalar(S && s)
{
    return Scalar<S> { std::forward<S>(s) };
}

// Wrap an external compact 1-D vector type for traversal. Sort of reduced ra_iterator.
// @TODO This can handle temporaries and make_a().begin() can't, look out for that.
template <class V>
struct Vector
{
    using traits = ra_traits<V>;
    static_assert(traits::rank_s()==1, "unsupported arg for Vector");

    V v;
    decltype(v.begin()) p__;
    static_assert(!std::is_reference<decltype(p__)>::value, "bad iterator type");

    using shape_type = std::array<dim_t, 1>;
    auto shape() const { return shape_type { { dim_t(v.size()) } }; }

    dim_t size() const { return v.size(); }
    dim_t size(int const i) const { CHECK_BOUNDS(i==0); return v.size(); }
    rank_t constexpr rank() const { return 1; }
    constexpr static rank_t rank_s() { return 1; };
    constexpr static dim_t size_s() { return traits::size_s(); }

// see test-ra-compatibility.C [a1] for forward() here.
    Vector(V && v_): v(std::forward<V>(v_)), p__(v.begin()) {}

    void adv(rank_t const k, dim_t const d)
    {
// k>0 happens on frame-matching when the axes k>0 can't be unrolled; see [trc-01] in test-ra-compatibility.C.
// k==0 && d!=1 happens on turning back at end of ply; @TODO we need this only on outer products and such.
        CHECK_BOUNDS(d==1 || d<0);
        p__ += (k==0) * d;
    }
    template <class I>
    decltype(auto) at(I const & i)
    {
        CHECK_BOUNDS(i[0]>=0 && i[0]<this->size());
        return p__[i[0]];
    }

    dim_t stride(int const i) const { return i==0 ? 1 : 0; }
    dim_t preferred_stride(int const i) const { return stride(i); }
    bool compact_stride(dim_t const step, int const z, int const j) const
    {
// reduced from cell_iterator::compact_stride()
        return (z==0) == (j==0);
    }

    auto flat() { return p__; }
    auto flat() const { return p__; }
    bool done() const { return p__==v.end(); }

// depend on ra::Expr, see ra-expr.H
#define DEF_VECTOR_EQOPS(OP)                        \
    template <class X> void operator OP(X && x);
    FOR_EACH(DEF_VECTOR_EQOPS, =, *=, +=, -=, /=)
#undef DEF_VECTOR_EQOPS
};

template <class V>
Vector<V> vector(V && v)
{
    return Vector<V> { std::forward<V>(v) };
}
template <class T>
Vector<std::initializer_list<T>> vector(std::initializer_list<T> && v)
{
    return Vector<std::initializer_list<T>> { std::move(v) };
}

template <class T>
struct IotaLeaf
{
    T i_;
    T const & operator*() const { return i_; } // @TODO if not for this, I could use plain T. Maybe ra::eval_expr...
    void operator+=(dim_t const d) { i_ += d; }

    auto operator+(dim_t const s) const { return i_ + s; }
    template <class I> bool operator!=(I const & i0) const { return i_!=i0; }
    template <class I> bool operator==(I const & i0) const { return i_==i0; }
};

template <class T_>
struct Iota
{
    using T = T_;
    T const size_, org_, stride_;
    T i_;

    using shape_type = std::array<dim_t, 1>;
    auto shape() const { return shape_type { { dim_t(size_) } }; }

    dim_t size() const { return size_; }
    dim_t size(int const i) const { CHECK_BOUNDS(i==0); return size_; }

    rank_t constexpr rank() const { return 1; }
    constexpr static rank_t rank_s() { return 1; };
    constexpr static dim_t size_s() { return DIM_ANY; }

    Iota(dim_t size, dim_t org=0, dim_t stride=1): size_(size), org_(org), stride_(stride), i_(org) { CHECK_BOUNDS(size>=0); }
    bool done() const { return i_==org_+stride_*size_; }

    void adv(rank_t const k, dim_t const d)
    {
        i_ += (k==0) * stride_ * d; // cf Vector::adv below.
    }
    template <class I>
    decltype(auto) at(I const & i)
    {
        return org_ + i[0]*stride_;
    }

    dim_t stride(int const i) const { return i==0 ? stride_ : 0; }
    dim_t preferred_stride(int const i) const { return stride(i); }
    bool compact_stride(dim_t const step, int const z, int const j) const
    {
// reduced from cell_iterator::compact_stride()
        return (z==0) == (j==0);
    }
    auto flat() const { return IotaLeaf<T> { i_ }; }
};

inline auto iota(int size, int org=0, int stride=1) { return Iota<int> { size, org, stride }; }
inline auto jvec(dim_t size, dim_t org=0, dim_t stride=1) { return Iota<dim_t> { size, org, stride }; }

struct all_t {};
constexpr all_t all = all_t();

template <class I> struct is_beatable_def
{
    constexpr static bool value = mp::Or<std::is_integral<I>, std::is_same<I, all_t> >::value;
};

template <class II> struct is_beatable_def<Iota<II>>
{
    constexpr static bool value = std::numeric_limits<II>::is_integer;
};

template <class I> using is_beatable = is_beatable_def<std::decay_t<I> >;

// others:
// * 'static': Like expr, but the operator is compile time (e.g. ::apply()).
// * 'dynamic': Like expr, but driver is selected at run time (e.g. if args are RANK_ANY).
// * ...

template <class X>
struct has_tensorindex_def
{
    static bool const value = false;
};

template <class T> using has_tensorindex = has_tensorindex_def<std::decay_t<T> >;

template <int w_, class value_type_>
struct has_tensorindex_def<TensorIndex<w_, value_type_> >
{
    static bool const value = true;
};

template <class Op, class T, class K=mp::index_tuple_<T>> struct Expr;

template <class Op, class ... Ti, class K>
struct has_tensorindex_def<Expr<Op, std::tuple<Ti ...>, K>>
{
    static bool const value = mp::Or<has_tensorindex<Ti> ...>::value;
};

template <class LiveAxes, int depth, class A> struct ApplyFrames;

template <class LiveAxes, int depth, class A>
struct has_tensorindex_def<ApplyFrames<LiveAxes, depth, A>>
{
    static bool const value = has_tensorindex<A>::value;
};

template <class FM, class Op, class T, class K=mp::index_tuple_<T>> struct Ryn;

template <class FM, class Op, class ... Ti, class K>
struct has_tensorindex_def<Ryn<FM, Op, std::tuple<Ti ...>, K>>
{
    static bool const value = mp::Or<has_tensorindex<Ti> ...>::value;
};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_RA_ATOM

#endif // RA_ATOM_H
