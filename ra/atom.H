// -*- mode: c++; coding: utf-8 -*-
/// @file atom.H
/// @brief Terminal nodes for expression templates, and use-as-xpr wrapper.

// (c) Daniel Llorens - 2011-2016, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/type.H"
#include <iostream>

namespace ra {

template <class V> inline constexpr auto size(V const & v);
template <class V> inline constexpr decltype(auto) shape(V const & v);

template <int w_, class value_type=ra::dim_t>
struct TensorIndexFlat
{
    dim_t i;
    constexpr void operator+=(dim_t const s) { i += s; }
    constexpr value_type operator*() { return i; }
};

// ra may be needed to avoid conversion issues.
template <int w_, class value_type=ra::dim_t>
struct TensorIndex
{
    dim_t i = 0;
    static_assert(w_>=0, "bad TensorIndex");
    constexpr static int w = w_;
    constexpr static dim_t size(int k) { return DIM_BAD; } // used in shape checks with dyn. rank.
    constexpr static rank_t rank_s() { return w+1; }
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t size_s(int k) { return DIM_BAD; }

    template <class I> constexpr value_type at(I const & ii) const { return value_type(ii[w]); }

    constexpr void adv(rank_t k, dim_t d) { RA_CHECK(d<=1); i += (k==w) * d; }
    constexpr decltype(auto) flat() const { return TensorIndexFlat<w, value_type> {i}; }
    constexpr static dim_t const stride(int k) { return (k==w); }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return step*stride(z)==stride(j); }
};

#define DEF_TENSORINDEX(i) TensorIndex<i> const JOIN(_, i) {};
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

template <class C> struct Scalar;

// Separate from Scalar so that operator+=, etc. has the array meaning there.
template <class C>
struct ScalarFlat: public Scalar<C>
{
    constexpr void operator+=(dim_t d) const {}
    constexpr C & operator*() { return this->c; }
    constexpr C const & operator*() const { return this->c; } // [ra39]
};

// Wrap constant for traversal. We still want f(C) to be a specialization in most cases.
template <class C_>
struct Scalar
{
    using C = C_;
    C c;

    constexpr static dim_t size(int k) { assert(0); return DIM_BAD; } // used in shape checks with dynamic rank.
    constexpr static dim_t size_s(int k) { return DIM_BAD; }
    constexpr static rank_t rank() { return 0; }
    constexpr static rank_t rank_s() { return 0; }

    template <class I> constexpr C & at(I const & i) { return c; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t stride(int k) { return 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return true; }
    constexpr decltype(auto) flat() { return static_cast<ScalarFlat<C> &>(*this); }
    constexpr decltype(auto) flat() const { return static_cast<ScalarFlat<C> const &>(*this); } // [ra39]

#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class C> inline constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

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

    constexpr dim_t size(int k) const { RA_CHECK(k==0); return traits::size(v); }
    constexpr static dim_t size_s(int k) { RA_CHECK(k==0); return traits::size_s(); }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };

// start() doesn't do this, but FIXME? Expr, etc. still does. See [ra35] in test/ra-9.C.
// FIXME these should never be called to convert Vector<V> to Vector<V&> etc.
    constexpr Vector(Vector<std::remove_reference_t<V>> const & a): v(std::move(a.v)), p__(v.begin()) {}; // looks like shouldn't happen
    constexpr Vector(Vector<std::remove_reference_t<V>> && a): v(std::move(a.v)), p__(v.begin()) {};
    constexpr Vector(Vector<std::remove_reference_t<V> &> const & a): v(a.v), p__(v.begin()) {};
    constexpr Vector(Vector<std::remove_reference_t<V> &> && a): v(a.v), p__(v.begin()) {};

// see test/ra-9.C [ra01] for forward() here.
    constexpr Vector(V && v_): v(std::forward<V>(v_)), p__(v.begin()) {}

    template <class I>
    decltype(auto) at(I const & i)
    {
        RA_CHECK(inside(i[0], ra::size(v)));
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
// k>0 happens on frame-matching when the axes k>0 can't be unrolled [ra03]
// k==0 && d!=1 happens on turning back at end of ply; TODO we need this only on outer products and such.
        RA_CHECK(d==1 || d<0);
        p__ += (k==0) * d;
    }
    constexpr static dim_t stride(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p__; }

#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class V> inline constexpr auto vector(V && v) { return Vector<V>(std::forward<V>(v)); }

// For STL iterators, no size. P needs to have advance(), *, and [] if used with by_index/ply_index.
// ra::ra_traits_def<P> doesn't need to be defined.
// FIXME a type that accepts (begin, end).
template <class P>
struct Ptr
{
    P p__;

    constexpr static dim_t size(int k) { RA_CHECK(k==0); return DIM_BAD; }
    constexpr static dim_t size_s(int k) { RA_CHECK(k==0); return DIM_BAD; }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };

    template <class I>
    constexpr decltype(auto) at(I && i)
    {
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        RA_CHECK(d==1 || d<0);
        std::advance(p__, (k==0) * d);
    }
    constexpr static dim_t stride(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p__; }

#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class I> inline auto ptr(I i) { return Ptr<I> { i }; }

// Same as Ptr, just with a size. For stuff like initializer_list that has size but no storage.
template <class P>
struct Span
{
    P p__;
    dim_t n__;

    constexpr dim_t size(int k) const { RA_CHECK(k==0); return n__; }
    constexpr static dim_t size_s(int k) { RA_CHECK(k==0); return DIM_ANY; }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };

    template <class I>
    decltype(auto) at(I const & i)
    {
        RA_CHECK(inside(i[0], n__));
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        RA_CHECK(d==1 || d<0);
        std::advance(p__, (k==0) * d);
    }
    constexpr static dim_t stride(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p__; }

#define DEF_ASSIGNOPS(OP) template <class X> constexpr void operator OP(X && x) \
    { for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x); }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS
};

template <class I> inline auto ptr(I i, dim_t n) { return Span<I> { i, n }; }

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
    T i_;
    T const stride_;

    constexpr Iota(dim_t size, T org=0, T stride=1): size_(size), i_(org), stride_(stride)
    {
        RA_CHECK(size>=0 && "Iota");
    }

    constexpr dim_t size(int k) const { RA_CHECK(k==0); return size_; }
    constexpr static dim_t size_s(int k) { RA_CHECK(k==0); return DIM_ANY; }
    constexpr rank_t rank() const { return 1; }
    constexpr static rank_t rank_s() { return 1; };

    template <class I>
    constexpr decltype(auto) at(I const & i)
    {
        return i_ + T(i[0])*stride_;
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        i_ += T((k==0) * d) * stride_; // cf Vector::adv
    }
    constexpr static dim_t stride(rank_t i) { return i==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t step, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return IotaFlat<T> { i_, stride_ }; }
    decltype(auto) operator+=(T const & b) { i_ += b; return *this; };
    decltype(auto) operator-=(T const & b) { i_ -= b; return *this; };
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

// FIXME have a 'filler' version (e.g. with default n = -1) or maybe a distinct type.
template <int n> struct is_beatable_def<dots_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = n;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <int n> struct is_beatable_def<insert_t<n>>
{
    static_assert(n>=0, "bad count for dots_n");
    constexpr static bool value = (n>=0);
    constexpr static int skip_src = 0;
    constexpr static int skip = n;
    constexpr static bool static_p = true;
};

template <class I> using is_beatable = is_beatable_def<std::decay_t<I>>;

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Expr;
template <class T, class K=mp::iota<mp::len<T>>> struct Pick;
template <class FM, class Op, class T, class K=mp::iota<mp::len<T>>> struct Ryn;
template <class LiveAxes, int depth, class A> struct ApplyFrames;

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

// don't restart these. About Scalar see [ra39]. For Vector FIXME this might not be correct.
template <class T, std::enable_if_t<is_ra_vector<T> || is_ra_scalar<T>, int> =0>
inline constexpr decltype(auto) start(T && t)
{
    return std::forward<T>(t);
}

template <class T, std::enable_if_t<is_slice<T>, int> =0>
inline constexpr auto start(T && t)
{
// BUG neither cell_iterator nor cell_iterator_small will retain rvalues [ra04]
// BUG iter() won't retain the View it's built from, either, which is why it needs to copy Dimv.
    return iter<0>(std::forward<T>(t));
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
    return ptr(v.begin(), v.size());
}

// forward declare for match.H; implemented in small.H.
template <class T, std::enable_if_t<is_builtin_array<T>, int> =0>
inline constexpr auto start(T && t);

// FIXME there should be default traits for all is_ra classes. E.g. Expr doesn't have it. Check ra::rank(), ra::shape() in atom.H.

// FIXME one of these is ET-generic and the other is slice only, so make up your mind.
// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using start_t = decltype(ra::start(std::declval<A>()));
template <class A> using flat_t = decltype(*(ra::start(std::declval<A>()).flat()));
template <class A> using value_t = std::decay_t<flat_t<A>>;

template <class V> inline constexpr dim_t
rank_s()
{
    if constexpr (has_traits<V>) {
        return ra_traits<V>::rank_s();
    } else {
        return std::decay_t<V>::rank_s();
    }
}

template <class V> inline  constexpr rank_t
rank_s(V const & v)
{
    return rank_s<V>();
}

template <class V_> inline constexpr dim_t
size_s()
{
    using V = std::decay_t<V_>;
    if constexpr (has_traits<V>) {
        return ra_traits<V>::size_s();
    } else {
        if constexpr (V::rank_s()==RANK_ANY) {
            return DIM_ANY;
        } else {
            ra::dim_t s = 1;
            for (int i=0; i!=V::rank_s(); ++i) {
                if (dim_t ss=V::size_s(i); ss>=0) {
                    s *= ss;
                } else {
                    return ss; // either DIM_ANY or DIM_BAD
                }
            }
            return s;
        }
    }
}

template <class V> constexpr dim_t
size_s(V const & v)
{
    return size_s<V>();
}

template <class V> inline constexpr rank_t
rank(V const & v)
{
    if constexpr (has_traits<V>) {
        return ra_traits<V>::rank(v);
    } else {
        return v.rank();
    }
}

template <class V> inline constexpr auto
size(V const & v)
{
    if constexpr (has_traits<V>) {
        return ra_traits<V>::size(v);
    } else {
        return prod(map([&v](auto && k) { return v.size(k); }, ra::iota(rank(v))));
    }
}

// To be used sparingly; prefer implicit matching.
template <class V> inline constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (has_traits<V>) {
        return ra_traits<V>::shape(v);
// FIXME version for static shape
// FIXME would prefer to return the map directly (except maybe for static shapes)
    } else if constexpr (constexpr rank_t rs=v.rank_s(); rs>=0) {
        return ra::Small<dim_t, rs>(map([&v](int k) { return v.size(k); }, ra::iota(rs)));
    } else {
        static_assert(RANK_ANY==rs);
        rank_t r = v.rank();
        std::vector<dim_t> s(r);
        for_each([&v, &s](int k) { s[k] = v.size(k); }, ra::iota(r));
        return s;
        // return map([this](int k) { return this->size(k); }, ra::iota(rank()));
    }
}

} // namespace ra
