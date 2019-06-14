// -*- mode: c++; coding: utf-8 -*-
/// @file traits.H
/// @brief Array traits: dimension, rank, extent, etc.

// (c) Daniel Llorens - 2013-2016
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

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

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_TRAITS
