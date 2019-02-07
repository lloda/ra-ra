
// (c) Daniel Llorens - 2013-2015, 2017, 2019

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file bootstrap.H
/// @brief Before all other ra:: includes.

#pragma once
#include <sys/types.h> // ssize_t
#include "ra/tuple-list.H"
#include "ra/tuple-dynamic.H"

#ifndef RA_CHECK_BOUNDS_BOOTSTRAP
  #ifndef RA_CHECK_BOUNDS
    #define RA_CHECK_BOUNDS_BOOTSTRAP 1
  #else
    #define RA_CHECK_BOUNDS_BOOTSTRAP RA_CHECK_BOUNDS
  #endif
#endif
#if RA_CHECK_BOUNDS_BOOTSTRAP==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) assert( cond )
#endif

namespace ra {

constexpr int VERSION = 7;     // to force or prevent upgrades on dependents
constexpr int LAST_ISSUE = 46; // numbered as [ra··] t/o the code. 08-10 are free.
constexpr int LAST_MAN = 113;  // numbered as [ma··] t/o the manual and examples/tests

static_assert(sizeof(int)>=4, "bad assumption on int");
using rank_t = int;
using dim_t = ssize_t;
dim_t const DIM_ANY = -1099999999;
dim_t const DIM_BAD = -1088888888;
rank_t const RANK_ANY = DIM_ANY;
rank_t const RANK_BAD = DIM_BAD;

constexpr dim_t dim_prod(dim_t const a, dim_t const b)
{
    return (a==DIM_ANY) ? DIM_ANY : ((b==DIM_ANY) ? DIM_ANY : a*b);
}
constexpr rank_t rank_sum(rank_t const a, rank_t const b)
{
    return (a==RANK_ANY) ? RANK_ANY : ((b==RANK_ANY) ? RANK_ANY : a+b);
}
constexpr rank_t rank_diff(rank_t const a, rank_t const b)
{
    return (a==RANK_ANY) ? RANK_ANY : ((b==RANK_ANY) ? RANK_ANY : a-b);
}
constexpr bool inside(dim_t const i, dim_t const b)
{
    return i>=0 && i<b;
}
constexpr bool inside(dim_t const i, dim_t const a, dim_t const b)
{
    return i>=a && i<b;
}

// used in array constructors to mean ‘don't initalize’.
enum none_t { none };

// used in array constructors to mean ‘don't instantiate’
struct no_arg {};

// used in stl_iterator / stl_iterator_small.
template <class S, class I, class P>
inline void next_in_cube(rank_t const framer, S const & dim, I & i, P & p)
{
    for (int k=framer-1; k>=0; --k) {
        ++i[k];
        if (i[k]<dim[k].size) {
            p += dim[k].stride;
            return;
        } else {
            i[k] = 0;
            p -= dim[k].stride*(dim[k].size-1);
        }
    }
    p = nullptr;
}

template <int k, class sizes, class strides, class I, class P> void
next_in_cube(I & i, P & p)
{
    if constexpr (k>=0) {
        ++i[k];
        if (i[k]<mp::Ref_<sizes, k>::value) {
            p += mp::Ref_<strides, k>::value;
        } else {
            i[k] = 0;
            p -= mp::Ref_<strides, k>::value*(mp::Ref_<sizes, k>::value-1);
            next_in_cube<k-1, sizes, strides>(i, p);
        }
    } else {
        p = nullptr;
    }
}

// Order of decl issues.
template <class C> struct Scalar; // for type predicates
template <class T, rank_t RANK=RANK_ANY> struct View; // for cell_iterator
template <class V, class Enable=void> struct ra_traits_def;

template <class S> struct default_strides_ {};
template <class tend> struct default_strides_<std::tuple<tend>> { using type = mp::int_list<1>; };
template <> struct default_strides_<std::tuple<>> { using type = mp::int_list<>; };

template <class t0, class t1, class ... ti>
struct default_strides_<std::tuple<t0, t1, ti ...>>
{
    using rest = typename default_strides_<std::tuple<t1, ti ...>>::type;
    static int const stride0 = t1::value * mp::First_<rest>::value;
    using type = mp::Cons_<mp::int_t<stride0>, rest>;
};

template <class S> using default_strides = typename default_strides_<S>::type;

template <int n> struct dots_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};
template <int n> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct newaxis_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};
template <int n=1> constexpr newaxis_t<n> newaxis = newaxis_t<n>();

// Used by cell_iterator / cell_iterator_small.
template <class C>
struct CellFlat
{
    C c;
    constexpr void operator+=(dim_t const s) { c.p += s; }
    constexpr C & operator*() { return c; }
};

// Common to View / SmallBase.
template <int cell_rank, class A> inline constexpr auto iter(A && a) { return a.template iter<cell_rank>(); }

// Defined in wrank.H but used in big.H (selectors, etc).
template <class A, class ... I> inline constexpr auto from(A && a, I && ... i);

} // namespace ra

#undef CHECK_BOUNDS
#undef RA_CHECK_BOUNDS_BOOTSTRAP
