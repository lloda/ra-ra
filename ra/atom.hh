// -*- mode: c++; coding: utf-8 -*-
/// ra-ra - Terminal nodes for expression templates.

// (c) Daniel Llorens - 2011-2016, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "type.hh"
#include <vector>

namespace ra {

template <class V> inline constexpr dim_t size(V const & v);
template <class V> inline constexpr decltype(auto) shape(V const & v);


// --------------------
// global introspection I
// --------------------

template <class V> inline constexpr dim_t
rank_s()
{
    if constexpr (requires { std::decay_t<V>::rank_s(); }) {
        return std::decay_t<V>::rank_s();
    } else if constexpr (requires { ra_traits<V>::rank_s(); }) {
        return ra_traits<V>::rank_s();
    } else {
        return 0;
    }
}

template <class V> inline  constexpr rank_t
rank_s(V const &)
{
    return rank_s<V>();
}

template <class V> inline constexpr dim_t
size_s()
{
    if constexpr (requires { std::decay_t<V>::size_s(); }) {
        return std::decay_t<V>::size_s();
    } else if constexpr (requires { ra_traits<V>::size_s(); }) {
        return ra_traits<V>::size_s();
    } else {
        if constexpr (RANK_ANY==rank_s<V>()) {
            return DIM_ANY;
// make it work for non-registered types.
        } else if constexpr (0==rank_s<V>()) {
            return 1;
        } else {
            using V_ = std::decay_t<V>;
            ra::dim_t s = 1;
            for (int i=0; i!=V_::rank_s(); ++i) {
                if (dim_t ss=V_::len_s(i); ss>=0) {
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

template <class V> inline constexpr dim_t
size(V const & v)
{
    if constexpr (requires { v.size(); }) {
        return v.size();
    } else if constexpr (requires { ra_traits<V>::size(v); }) {
        return ra_traits<V>::size(v);
    } else {
        dim_t s = 1;
        for (rank_t k=0; k<rank(v); ++k) {
            s *= v.len(k);
        }
        return s;
    }
}

// Try to avoid; prefer implicit matching.
template <class V> inline constexpr decltype(auto)
shape(V const & v)
{
    if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (requires { ra_traits<V>::shape(v); }) {
        return ra_traits<V>::shape(v);
    } else if constexpr (constexpr rank_t rs=rank_s<V>(); rs>=0) {
// FIXME Would prefer to return the map directly
        ra::Small<dim_t, rs> s;
        for (rank_t k=0; k<rs; ++k) {
            s[k] = v.len(k);
        }
        return s;
    } else {
        static_assert(RANK_ANY==rs);
        rank_t r = v.rank();
        std::vector<dim_t> s(r);
        for (rank_t k=0; k<r; ++k) {
            s[k] = v.len(k);
        }
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


// --------------------
// atom types
// --------------------

// Iterator for rank 0 object. This can be used on foreign objects, or as an alternative to the rank conjunction.
// We still want f(C) to be a specialization in most cases (ie avoid ply(f, C) when C is rank 0).
template <class C>
struct Scalar
{
    template <class C_>
    struct Flat: public Scalar<C_>
    {
        constexpr void operator+=(dim_t d) const {}
        constexpr C_ & operator*() { return this->c; }
        constexpr C_ const & operator*() const { return this->c; } // [ra39]
    };

    C c;

    constexpr static rank_t rank_s() { return 0; }
    constexpr static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { return DIM_BAD; }
    constexpr static dim_t len(int k) { return DIM_BAD; } // used in shape checks with dyn rank.

    template <class I> constexpr decltype(auto) at(I const & i) { return c; }
    template <class I> constexpr decltype(auto) at(I const & i) const { return c; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static dim_t step(int k) { return 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) flat() { return static_cast<Flat<C> &>(*this); }
    constexpr decltype(auto) flat() const { return static_cast<Flat<C> const &>(*this); } // [ra39]

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class C> inline constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

// Iterator for rank-1 foreign object. ra:: objects have their own Iterators.
template <class V>
requires (requires (V v) { { std::ssize(v) } -> std::signed_integral; } &&
          requires (V v) { { std::begin(v) } -> std::random_access_iterator; })
struct Vector
{
    V v;
// Using std::begin() and size_s together is inconsistent (FIXME). Limit to rank 1 types to prevent trouble.
    static_assert(1==rank_s<V>());
    decltype(std::begin(v)) p;
    constexpr static dim_t ct_size = size_s<V>();
    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k)
    {
        RA_CHECK(k==0, " k ", k);
        if constexpr (DIM_ANY==ct_size) { return DIM_ANY; } else { return ct_size; };
    }
    constexpr dim_t len(int k) const
    {
        RA_CHECK(k==0, " k ", k);
        if constexpr (DIM_ANY==ct_size) { return ra::size(v); } else { return ct_size; };
    }

// [ra1] test/ra-9.cc
    constexpr Vector(V && v_): v(std::forward<V>(v_)), p(std::begin(v)) {}
// [ra35] test/ra-9.cc
    constexpr Vector(Vector const & a) requires (!std::is_reference_v<V>): v(std::move(a.v)), p(std::begin(v)) {};
    constexpr Vector(Vector && a) requires (!std::is_reference_v<V>): v(std::move(a.v)), p(std::begin(v)) {};
    constexpr Vector(Vector const & a) requires (std::is_reference_v<V>) = default;
    constexpr Vector(Vector && a) requires (std::is_reference_v<V>) = default;
// cf RA_DEF_ASSIGNOPS_SELF
    Vector & operator=(Vector const & a) requires (!std::is_reference_v<V>) { v = std::move(a.v); p = std::begin(v); }
    Vector & operator=(Vector && a) requires (!std::is_reference_v<V>) { v = std::move(a.v); p = std::begin(v); }
    Vector & operator=(Vector const & a) requires (std::is_reference_v<V>) { v = a.v; p = std::begin(v); return *this; };
    Vector & operator=(Vector && a) requires (std::is_reference_v<V>) { v = a.v; p = std::begin(v); return *this; };

    template <class I>
    constexpr decltype(auto) at(I const & i)
    {
        RA_CHECK(inside(i[0], std::ssize(v)), " i ", i[0], " size ", std::ssize(v));
        return p[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
// k>0 happens on frame-matching when the axes k>0 can't be unrolled [ra3]
// k==0 && d!=1 happens on turning back at end of ply.
        RA_CHECK(d==1 || d<=0, " k ", k, " d ", d);
        p += (k==0) * d;
    }
    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class V> inline constexpr auto vector(V && v) { return Vector<V>(std::forward<V>(v)); }

template <std::random_access_iterator P>
struct Ptr
{
    P p;

    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return DIM_BAD; }
    constexpr static dim_t len(int k) { RA_CHECK(k==0, " k ", k); return DIM_BAD; }

    template <class I>
    constexpr decltype(auto) at(I && i)
    {
        return p[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        RA_CHECK(d==1 || d<=0, " k ", k, " d ", d);
        std::advance(p, (k==0) * d);
    }
    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class I> inline auto ptr(I i) { return Ptr<I> { i }; }

// Same as Ptr, just with a size. For stuff like initializer_list that has size but no storage.
template <std::random_access_iterator P>
struct Span
{
    P p;
    dim_t n_;

    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return DIM_ANY; }
    constexpr dim_t len(int k) const { RA_CHECK(k==0, " k ", k); return n_; }

    template <class I>
    decltype(auto) at(I const & i)
    {
        RA_CHECK(inside(i[0], n_), " i ", i[0], " size ", n_);
        return p[i[0]];
    }
    constexpr void adv(rank_t k, dim_t d)
    {
        RA_CHECK(d==1 || d<=0, " k ", k, " d ", d);
        std::advance(p, (k==0) * d);
    }
    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return p; }

    RA_DEF_ASSIGNOPS_DEFAULT_SET
};

template <class I> inline auto ptr(I i, dim_t n) { return Span<I> { i, n }; }

template <int w>
struct TensorIndex
{
    struct Flat
    {
        dim_t i;
        constexpr void operator+=(dim_t const s) { i += s; }
        constexpr dim_t operator*() { return i; }
    };

    dim_t i = 0;
    static_assert(w>=0, "bad TensorIndex");
    constexpr static rank_t rank_s() { return w+1; }
    constexpr static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { return DIM_BAD; }
    constexpr static dim_t len(int k) { return DIM_BAD; } // for shape checks with dyn rank.

    template <class I> constexpr dim_t at(I const & ii) const { return ii[w]; }
    constexpr void adv(rank_t k, dim_t d) { RA_CHECK(d<=1, " d ", d); i += (k==w) * d; }
    constexpr static dim_t const step(int k) { return (k==w); }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr auto flat() const { return Flat {i}; }
};

#define DEF_TENSORINDEX(i) TensorIndex<i> const JOIN(_, i) {};
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

template <class T>
struct Iota
{
    struct Flat
    {
        T i_;
        T const step_;
        T const & operator*() const { return i_; }
        void operator+=(dim_t d) { i_ += T(d)*step_; }
    };

    dim_t const len_;
    T i_;
    T const step_;

    constexpr Iota(dim_t len, T org=0, T step=1): len_(len), i_(org), step_(step)
    {
        RA_CHECK(len>=0, "Iota len ", len);
    }

    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { RA_CHECK(k==0, " k ", k); return DIM_ANY; }
    constexpr dim_t len(int k) const { RA_CHECK(k==0, " k ", k); return len_; }

    template <class I> constexpr T at(I const & i) { return i_ + T(i[0])*step_; }
    constexpr void adv(rank_t k, dim_t d) { i_ += T((k==0) * d) * step_; } // cf Vector::adv
    constexpr static dim_t step(rank_t i) { return i==0 ? 1 : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return (z==0) == (j==0); }
    constexpr auto flat() const { return Flat { i_, step_ }; }
    decltype(auto) operator+=(T const & b) { i_ += b; return *this; };
    decltype(auto) operator-=(T const & b) { i_ -= b; return *this; };
};

template <class O=dim_t, class S=O> inline constexpr auto
iota(dim_t len, O org=0, S step=1)
{
    using T = std::common_type_t<O, S>;
    return Iota<T> { len, T(org), T(step) };
}


// --------------------
// helpers for slicing
// --------------------

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


// --------------
// Coerce potential Iterator
// --------------

template <class T>
inline constexpr void start(T && t)
{
    static_assert(!std::same_as<T, T>, "Type cannot be start()ed.");
}

RA_IS_DEF(is_iota, (std::same_as<A, Iota<decltype(std::declval<A>().i_)>>))
RA_IS_DEF(is_ra_scalar, (std::same_as<A, Scalar<decltype(std::declval<A>().c)>>))
RA_IS_DEF(is_ra_vector, (std::same_as<A, Vector<decltype(std::declval<A>().v)>>))

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

template <class T>
inline constexpr auto
start(std::initializer_list<T> v)
{
    return ptr(v.begin(), v.size());
}

// forward declare for Match; implemented in small.hh.
template <class T> requires (is_builtin_array<T>)
inline constexpr auto
start(T && t);

// Neither cell_iterator_big nor cell_iterator_small will retain rvalues [ra4].
template <class T> requires (is_slice<T>)
inline constexpr auto
start(T && t)
{
    return iter<0>(std::forward<T>(t));
}

// Iterator (rather restart)

template <class T> requires (is_ra_scalar<T>)
inline constexpr decltype(auto)
start(T && t)
{
    return std::forward<T>(t);
}

// see [ra35] and Vector constructors above. Iterators need to be restarted on every use (eg ra::cross()).
template <class T> requires (is_iterator<T> && !is_ra_scalar<T> && !is_ra_vector<T>)
inline constexpr auto
start(T && t)
{
    return std::forward<T>(t);
}

// restart iterator but do not copy data. This follows iter(View); Vector is just an interface adaptor [ra35].
template <class T> requires (is_ra_vector<T>)
inline constexpr auto
start(T && t)
{
    return vector(t.v);
}


// --------------------
// global introspection II
// --------------------

// FIXME one of these is ET-generic and the other is slice only, so make up your mind.
// FIXME do we really want to drop const? See use in concrete_type.
template <class A> using value_t = std::decay_t<decltype(*(ra::start(std::declval<A>()).flat()))>;

} // namespace ra
