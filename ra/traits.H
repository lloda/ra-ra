
// (c) Daniel Llorens - 2013-2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file traits.H
/// @brief Array traits: dimension, rank, extent, etc.

#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include "ra/bootstrap.H"

#ifndef RA_CHECK_BOUNDS_TRAITS
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_TRAITS 1
  #else
    #define RA_CHECK_BOUNDS_TRAITS RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_TRAITS==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) RA_ASSERT( cond )
#endif

namespace ra {

template <class A> using ra_traits = ra_traits_def<std::remove_cv_t<std::remove_reference_t<A>>>;

template <class T, class A>
struct ra_traits_def<std::vector<T, A>>
{
    using V = std::vector<T, A>;
    using value_type = T;
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { dim_t(v.size()) }; }
    static V make(dim_t const n)
    {
        return std::vector<T, A>(n);
    }
    template <class TT> static V make(dim_t n, TT const & t)
    {
        return V(n, t);
    }
    constexpr static dim_t size(V const & v) { return v.size(); }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; }
};

template <class T, std::size_t N>
struct ra_traits_def<std::array<T, N>>
{
    using V = std::array<T, N>;
    using value_type = T;
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { N }; }
    constexpr static V make(dim_t const n)
    {
        CHECK_BOUNDS(n==N);
        return V {};
    }
    template <class TT> constexpr static V make(dim_t n, TT const & t)
    {
        CHECK_BOUNDS(n==N);
        V r {};
        std::fill(r.data(), r.data()+n, t);
        return r;
    }
    constexpr static dim_t size(V const & v) { return v.size(); }
    constexpr static dim_t size_s() { return N; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; };
};

template <class T>
struct ra_traits_def<T *>
{
    using V = T *;
    using value_type = T;
    constexpr static dim_t size(V const & v) { return DIM_BAD; }
    constexpr static dim_t size_s() { return DIM_BAD; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; }
};

template <class T>
struct ra_traits_def<std::initializer_list<T>>
{
    using V = std::initializer_list<T>;
    using value_type = T;
    constexpr static rank_t size(V const & v) { return v.size(); }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static rank_t rank_s() { return 1; }
    constexpr static rank_t size_s() { return DIM_ANY; }
};

// To handle arrays of static/dynamic size.
template <class A> void
resize(A & a, dim_t k)
{
    if constexpr (ra_traits<A>::size_s()==DIM_ANY) {
        a.resize(k);
    } else {
        CHECK_BOUNDS(k==dim_t(a.size_s(0)));
    }
}

// ---------------------------
// Indexers TODO traits.H doesn't seem quite the logical place.
// ---------------------------

template <class sizes, class strides>
struct Indexer0
{
    static_assert(mp::len<sizes> == mp::len<strides>, "mismatched sizes & strides");

    template <rank_t end, rank_t k, class P>
    constexpr static std::enable_if_t<k==end, dim_t> index_p_(dim_t const c, P const & p)
    {
        return c;
    }
    template <rank_t end, rank_t k, class P>
    constexpr static std::enable_if_t<(k<end), dim_t> index_p_(dim_t const c, P const & p)
    {
        CHECK_BOUNDS(inside(p[k], mp::first<sizes>::value));
        return Indexer0<mp::drop1<sizes>, mp::drop1<strides>>::template index_p_<end, k+1>(c + p[k] * mp::first<strides>::value, p);
    }
    template <class P>
    constexpr static dim_t index_p(P const & p) // for Container::at().
    {
// gcc accepts p.size(), but I also need P = std::array to work. See also below.
        static_assert(mp::len<sizes> >= ra_traits<P>::size_s(), "too many indices");
        return index_p_<ra_traits<P>::size_s(), 0>(0, p);
    }
    template <class P, std::enable_if_t<ra_traits<P>::size_s()!=RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        static_assert(mp::len<sizes> <= ra_traits<P>::size_s(), "too few indices");
        return index_p_<mp::len<sizes>, 0>(0, p);
    }
    template <class P, std::enable_if_t<ra_traits<P>::size_s()==RANK_ANY, int> =0>
    constexpr static dim_t index_short(P const & p) // for ArrayIterator::at().
    {
        CHECK_BOUNDS(mp::len<sizes> <= p.size());
        return index_p_<mp::len<sizes>, 0>(0, p);
    }
};

struct Indexer1
{
    template <class Dim, class P>
    constexpr static dim_t index_p(Dim const & dim, P && p)
    {
        CHECK_BOUNDS(dim_t(dim.size())>=start(p).size(0) && "too many indices");
// use dim.data() to skip the size check.
        dim_t c = 0;
        for_each([&c](auto && d, auto && p) { CHECK_BOUNDS(inside(p, d.size)); c += d.stride*p; },
                 ptr(dim.data()), p);
        return c;
    }

// used by cell_iterator::at() for rank matching on rank<driving rank, no slicing. TODO Static check.
    template <class Dim, class P>
    constexpr static dim_t index_short(rank_t framer, Dim const & dim, P const & p)
    {
        dim_t c = 0;
        for (rank_t k=0; k<framer; ++k) {
            CHECK_BOUNDS(inside(p[k], dim[k].size));
            c += dim[k].stride * p[k];
        }
        return c;
    }
};

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_TRAITS
