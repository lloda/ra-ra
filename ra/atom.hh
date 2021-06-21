// -*- mode: c++; coding: utf-8 -*-
/// @file atom.hh
/// @brief Terminal nodes for expression templates, and use-as-xpr wrapper.

// (c) Daniel Llorens - 2011-2016, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/type.hh"

namespace ra {

template <class V> inline constexpr auto size(V const & v);
template <class V> inline constexpr decltype(auto) shape(V const & v);

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

    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { return DIM_BAD; }
    constexpr static dim_t len(int k) { return DIM_BAD; } // used in shape checks with dyn rank.

    template <class I> constexpr decltype(auto) at(I const & i) { return c; }
    template <class I> constexpr decltype(auto) at(I const & i) const { return c; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t stride(int k) { return 0; }
    constexpr static bool keep_stride(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() { return static_cast<ScalarFlat<C> &>(*this); }
    constexpr decltype(auto) flat() const { return static_cast<ScalarFlat<C> const &>(*this); } // [ra39]

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class C> inline constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

// Wrap foreign vectors.
// FIXME This can handle temporaries and make_a().begin() can't, look out for that.
// FIXME Do we need this class? holding rvalue is the only thing it does over View, and it doesn't handle rank!=1.
template <class V>
requires (requires { ra_traits<V>::size_s; } &&
          requires (V v) { { v.begin() } -> std::random_access_iterator; })
struct Vector
{
    V v;
    decltype(v.begin()) p__;

    constexpr dim_t len(int k) const { RA_CHECK(k==0, " k ", k); return ra_traits<V>::size(v); }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return ra_traits<V>::size_s(); }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };

// see test/ra-9.cc [ra1] for forward() here.
    constexpr Vector(V && v_): v(std::forward<V>(v_)), p__(v.begin()) {}
// see [ra35] in test/ra-9.cc. FIXME How about I just hold a ref for any kind of V, like container -> iter.
    constexpr Vector(Vector<std::remove_reference_t<V>> const & a): v(std::move(a.v)), p__(v.begin()) { static_assert(!std::is_reference_v<V>); };
    constexpr Vector(Vector<std::remove_reference_t<V>> && a): v(std::move(a.v)), p__(v.begin()) { static_assert(!std::is_reference_v<V>); };

    template <class I>
    decltype(auto) at(I const & i)
    {
        RA_CHECK(inside(i[0], ra::size(v)), " i ", i[0], " size ", ra::size(v));
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
// k>0 happens on frame-matching when the axes k>0 can't be unrolled [ra03]
// k==0 && d!=1 happens on turning back at end of ply.
// we need this only on outer products and such, or in FIXME operator<<; which could be fixed I think.
        RA_CHECK(d==1 || d<=0, " k ", k, " d ", d, " (Vector)");
        p__ += (k==0) * d;
    }
    constexpr static dim_t stride(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p__; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class V> inline constexpr auto vector(V && v) { return Vector<V>(std::forward<V>(v)); }

template <std::random_access_iterator P>
struct Ptr
{
    P p__;

    constexpr static dim_t len(int k) { RA_CHECK(k==0, " k ", k); return DIM_BAD; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return DIM_BAD; }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };

    template <class I>
    constexpr decltype(auto) at(I && i)
    {
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        RA_CHECK(d==1 || d<=0, " k ", k, " d ", d, " (Ptr)");
        std::advance(p__, (k==0) * d);
    }
    constexpr static dim_t stride(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p__; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class I> inline auto ptr(I i) { return Ptr<I> { i }; }

// Same as Ptr, just with a size. For stuff like initializer_list that has size but no storage.
template <std::random_access_iterator P>
struct Span
{
    P p__;
    dim_t n__;

    constexpr dim_t len(int k) const { RA_CHECK(k==0, " k ", k); return n__; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return DIM_ANY; }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };

    template <class I>
    decltype(auto) at(I const & i)
    {
        RA_CHECK(inside(i[0], n__), " i ", i[0], " size ", n__);
        return p__[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        RA_CHECK(d==1 || d<=0, " k ", k, " d ", d, " (Span)");
        std::advance(p__, (k==0) * d);
    }
    constexpr static dim_t stride(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_stride(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p__; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class I> inline auto ptr(I i, dim_t n) { return Span<I> { i, n }; }

template <int w_, class value_type=ra::dim_t>
struct TensorIndexFlat
{
    dim_t i;
    constexpr void operator+=(dim_t const s) { i += s; }
    constexpr value_type operator*() { return i; }
};

template <int w, class value_type=ra::dim_t>
struct TensorIndex
{
    dim_t i = 0;
    static_assert(w>=0, "bad TensorIndex");
    constexpr static rank_t rank_s() { return w+1; }
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { return DIM_BAD; }
    constexpr static dim_t len(int k) { return DIM_BAD; } // used in shape checks with dyn rank.

    template <class I> constexpr value_type at(I const & ii) const { return value_type(ii[w]); }
    constexpr void adv(rank_t k, dim_t d) { RA_CHECK(d<=1, " d ", d); i += (k==w) * d; }
    constexpr static dim_t const stride(int k) { return (k==w); }
    constexpr static bool keep_stride(dim_t st, int z, int j) { return st*stride(z)==stride(j); }
    constexpr auto flat() const { return TensorIndexFlat<w, value_type> {i}; }
};

#define DEF_TENSORINDEX(i) TensorIndex<i> const JOIN(_, i) {};
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

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
    dim_t const len_;
    T i_;
    T const stride_;

    constexpr Iota(dim_t len, T org=0, T stride=1): len_(len), i_(org), stride_(stride)
    {
        RA_CHECK(len>=0, "Iota len ", len);
    }

    constexpr dim_t len(int k) const { RA_CHECK(k==0, " k ", k); return len_; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return DIM_ANY; }
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
    constexpr static bool keep_stride(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return IotaFlat<T> { i_, stride_ }; }
    decltype(auto) operator+=(T const & b) { i_ += b; return *this; };
    decltype(auto) operator-=(T const & b) { i_ -= b; return *this; };
};

template <class O=dim_t, class S=O> inline constexpr auto
iota(dim_t len, O org=0, S stride=1)
{
    using T = std::common_type_t<O, S>;
    return Iota<T> { len, T(org), T(stride) };
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
template <class Live, class A> struct Reframe;

// --------------
// Coerce potential RaIterator
// --------------

template <class T>
inline constexpr void start(T && t)
{
    static_assert(!std::same_as<T, T>, "Type cannot be start()ed.");
}

RA_IS_DEF(is_iota, (std::same_as<A, Iota<typename A::T>>))
RA_IS_DEF(is_ra_scalar, (std::same_as<A, Scalar<typename A::C>>))
RA_IS_DEF(is_ra_vector, (std::same_as<A, Vector<typename A::V>>))

template <class T> requires (is_foreign_vector<T>)
inline constexpr auto
start(T && t)
{
    return ra::vector(std::forward<T>(t));
}

template <class T> requires (is_scalar<T>)
inline constexpr auto
start(T && t)
{
    return ra::scalar(std::forward<T>(t));
}

// See [ra35] and Vector constructors above. RaIterators need to be restarted in case on every use (eg ra::cross()).
template <class T> requires (is_iterator<T> && !is_ra_scalar<T> && !is_ra_vector<T>)
inline constexpr auto
start(T && t)
{
    return std::forward<T>(t);
}

// Copy the iterator but not the data. This follows the behavior of iter(View); Vector is just an interface adaptor [ra35].
template <class T> requires (is_ra_vector<T>)
inline constexpr auto
start(T && t)
{
    return vector(t.v);
}

// For Scalar we forward since there's no iterator part.
template <class T> requires (is_ra_scalar<T>)
inline constexpr decltype(auto)
start(T && t)
{
    return std::forward<T>(t);
}

// Neither cell_iterator nor cell_iterator_small will retain rvalues [ra4].
template <class T> requires (is_slice<T>)
inline constexpr auto
start(T && t)
{
    return iter<0>(std::forward<T>(t));
}

template <class T>
inline constexpr auto
start(std::initializer_list<T> v)
{
    return ptr(v.begin(), v.size());
}

// forward declare for match.hh; implemented in small.hh.
template <class T> requires (is_builtin_array<T>)
inline constexpr auto
start(T && t);

// FIXME one of these is ET-generic and the other is slice only, so make up your mind.
// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using value_t = std::decay_t<decltype(*(ra::start(std::declval<A>()).flat()))>;

template <class V> inline constexpr dim_t
rank_s()
{
    if constexpr (requires { ra_traits<V>::rank_s(); }) {
        return ra_traits<V>::rank_s();
    } else if constexpr (requires { std::decay_t<V>::rank_s(); }) {
        return std::decay_t<V>::rank_s();
    } else {
        return 0;
    }
}

template <class V> inline  constexpr rank_t
rank_s(V const &)
{
    return rank_s<V>();
}

template <class V_> inline constexpr dim_t
size_s()
{
    using V = std::decay_t<V_>;
    if constexpr (requires { ra_traits<V>::size_s(); }) {
        return ra_traits<V>::size_s();
    } else if constexpr (requires { std::decay_t<V>::size_s(); }) {
        return std::decay_t<V>::size_s();
    } else {
        if constexpr (RANK_ANY==rank_s<V>()) {
            return DIM_ANY;
// make it work for non-registered types.
        } else if constexpr (0==rank_s<V>()) {
            return 1;
        } else {
            ra::dim_t s = 1;
            for (int i=0; i!=V::rank_s(); ++i) {
                if (dim_t ss=V::len_s(i); ss>=0) {
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
size_s(V const &)
{
    return size_s<V>();
}

template <class V> inline constexpr rank_t
rank(V const & v)
{
    if constexpr (requires { ra_traits<V>::rank(v); }) {
        return ra_traits<V>::rank(v);
    } else {
        return v.rank();
    }
}

template <class V> inline constexpr auto
size(V const & v)
{
    if constexpr (requires { ra_traits<V>::size(v); }) {
        return ra_traits<V>::size(v);
    } else if constexpr (requires { v.size(); }) {
        return v.size();
    } else {
        return prod(map([&v](auto && k) { return v.len(k); }, ra::iota(rank(v))));
    }
}

// To be used sparingly; prefer implicit matching.
template <class V> inline constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (requires { ra_traits<V>::shape(v); }) {
        return ra_traits<V>::shape(v);
    } else if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (constexpr rank_t rs=rank_s<V>(); rs>=0) {
// FIXME Would prefer to return the map directly
        return ra::Small<dim_t, rs>(map([&v](int k) { return v.len(k); }, ra::iota(rs)));
    } else {
        static_assert(RANK_ANY==rs);
        rank_t r = v.rank();
        std::vector<dim_t> s(r);
        for_each([&v, &s](int k) { s[k] = v.len(k); }, ra::iota(r));
        return s;
    }
}

// To handle arrays of static/dynamic size.
template <class A> void
resize(A & a, dim_t k)
{
    if constexpr (DIM_ANY==size_s<A>()) {
        a.resize(k);
    } else {
        RA_CHECK(k==dim_t(a.len_s(0)));
    }
}

} // namespace ra
