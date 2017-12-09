
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

namespace ra {

template <class A> using ra_traits = ra_traits_def<std::decay_t<A>>;

template <class T, class A>
struct ra_traits_def<std::vector<T, A>>
{
    using V = std::vector<T, A>;
    using value_type = T;
    using shape_type = std::array<dim_t, 1>;
    constexpr static shape_type shape(V const & v) { return shape_type { { dim_t(v.size()) } }; }
    static V make(dim_t const n)
    {
        return std::vector<T>(n);
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
    using shape_type = std::array<dim_t, 1>;
    constexpr static shape_type shape(V const & v) { return shape_type { { N } }; }
    constexpr static V make(dim_t const n)
    {
        assert(n==N);
        return V {};
    }
    template <class TT> constexpr static V make(dim_t n, TT const & t)
    {
        assert(n==N);
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

// To handle rank-1 arrays of static/dynamic size.
template <class Dim, std::enable_if_t<ra_traits<Dim>::size_s()!=DIM_ANY, int> =0>
void resize(Dim & dim, dim_t k)
{
    static_assert(ra_traits<Dim>::rank_s()==1, "bad rank");
    assert(k==dim_t(dim.size()));
}

template <class Dim, std::enable_if_t<ra_traits<Dim>::size_s()==DIM_ANY, int> =0>
void resize(Dim & dim, dim_t k)
{
    static_assert(ra_traits<Dim>::rank_s()==1 || ra_traits<Dim>::rank_s()==RANK_ANY, "bad rank");
    dim.resize(k);
}

// ---------------------
// nested_braces
// ---------------------

// Avoid clash of T with scalar constructors (for rank 0) and raveling init list constructor (for rank 1).
template <class T, rank_t rank, class Enable=void>
struct nested_braces
{
    struct not_used_as_argument {};
    using list = not_used_as_argument;
};

template <class T, rank_t rank>
struct nested_braces<T, rank, std::enable_if_t<(rank==2)>>
{
    using list = std::initializer_list<std::initializer_list<T>>;
    template <size_t N> constexpr static
    void shape(list l, std::array<dim_t, N> & s)
    {
        s[N-2] = l.size();
        s[N-1] = l.size()>0 ? l.begin()->size() : 0;
    }
};

template <class T, rank_t rank>
struct nested_braces<T, rank, std::enable_if_t<(rank>2)>>
{
    using sub = nested_braces<T, rank-1>;
    using list = std::initializer_list<typename sub::list>;
    template <size_t N> constexpr static
    void shape(list l, std::array<dim_t, N> & s)
    {
        s[N-rank] = l.size();
        if (l.size()>0) {
            sub::template shape(*(l.begin()), s);
        } else {
            std::fill(s.begin()+N-rank+1, s.end(), 0);
        }
    }
};

} // namespace ra
