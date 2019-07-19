// -*- mode: c++; coding: utf-8 -*-
/// @file bootstrap.H
/// @brief Before all other ra:: includes.

// (c) Daniel Llorens - 2013-2015, 2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <sys/types.h> // ssize_t
#include "ra/tuple-list.H"
#include "ra/tuple-dynamic.H"

#if defined(RA_CHECK_BOUNDS) && RA_CHECK_BOUNDS==0
    #define CHECK_BOUNDS( cond )
#else
    #define CHECK_BOUNDS( cond ) RA_ASSERT( cond )
#endif

namespace ra {

constexpr int VERSION = 10;    // to force or prevent upgrades on dependents
constexpr int LAST_ISSUE = 46; // numbered as [ra··] t/o the code. 08-10 are free.
constexpr int LAST_MAN = 116;  // numbered as [ma··] t/o the manual and examples/tests

static_assert(sizeof(int)>=4, "bad assumption on int");
using rank_t = int;
using dim_t = ssize_t;
constexpr dim_t DIM_ANY = -1099999444;
constexpr dim_t DIM_BAD = -1099999888;
constexpr rank_t RANK_ANY = -1099999444;
constexpr rank_t RANK_BAD = -1099999888;

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
    static int const stride0 = t1::value * mp::first<rest>::value;
    using type = mp::cons<mp::int_t<stride0>, rest>;
};

template <class S> using default_strides = typename default_strides_<S>::type;

template <int n> struct dots_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};
template <int n> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};

template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

// Used by cell_iterator / cell_iterator_small.
template <class C>
struct CellFlat
{
    C c;
    constexpr void operator+=(dim_t const s) { c.p += s; }
    constexpr C & operator*() { return c; }
};

// Common to View / SmallBase.
template <int cell_rank, class A> inline constexpr auto
iter(A && a) { return std::forward<A>(a).template iter<cell_rank>(); }

// Defined in wrank.H but used in big.H (selectors, etc).
template <class A, class ... I> inline constexpr auto from(A && a, I && ... i);

// Extended by operators.H and repeated in global.H.
// TODO All users be int, then this take int.
constexpr inline bool any(bool const x) { return x; }
constexpr inline bool every(bool const x) { return x; }
constexpr inline bool odd(unsigned int N) { return N & 1; }

// This logically belongs in ra/small.H, but it's here so that we can return ra:: types from shape().


// ---------------------
// nested braces for Small initializers
// ---------------------

// The general SmallArray has 4 constructors,
// 1. The empty constructor.
// 2. The scalar constructor. This is needed when T isn't registered as ra::scalar, which I don't think should be required purely for container use.
// 3. The ravel constructor.
// 4. The nested constructor.
// When SmallArray has rank 1, or the first dimension is empty, or the shape is [1] or [], several of the constructors above become ambiguous. We solve this by defining the constructor arguments to variants of no_arg.

template <class T, class sizes, class Enable=void>
struct nested_tuple;

// ambiguity with empty constructor and scalar constructor.
// if size(0) is 0, then prefer the empty constructor.
// if size(0) is 1...
template <class sizes> constexpr bool no_nested = (mp::first<sizes>::value<1);
template <> constexpr bool no_nested<mp::nil> = true;
template <> constexpr bool no_nested<mp::int_list<1>> = true;
template <class T, class sizes>
using nested_arg = std::conditional_t<no_nested<sizes>,
                                      std::tuple<no_arg>, // match the template for SmallArray.
                                      typename nested_tuple<T, sizes>::list>;

// ambiguity with scalar constructors (for rank 0) and nested_tuple (for rank 1).
template <class sizes> constexpr bool no_ravel = ((mp::len<sizes> <=1) || (mp::apply<mp::prod, sizes>::value <= 1));
template <class T, class sizes>
using ravel_arg = std::conditional_t<no_ravel<sizes>,
                                     std::tuple<no_arg, no_arg>, // match the template for SmallArray.
                                     mp::makelist<mp::apply<mp::prod, sizes>::value, T>>;

template <class T, class sizes, class strides = default_strides<sizes>> struct SmallView; // for cell_iterator_small
template <class T, class sizes, class strides = default_strides<sizes>,
          class nested_arg_ = nested_arg<T, sizes>, class ravel_arg_ = ravel_arg<T, sizes>>
struct SmallArray;
template <class T, dim_t ... sizes> using Small = SmallArray<T, mp::int_list<sizes ...>>;

template <class T>
struct nested_tuple<T, mp::nil>
{
    using sub = no_arg;
    using list = std::tuple<no_arg>; // match the template for SmallArray.
};

template <class T, int S0>
struct nested_tuple<T, mp::int_list<S0>>
{
    using sub = T;
    using list = mp::makelist<S0, T>;
};

template <class T, int S0, int S1, int ... S>
struct nested_tuple<T, mp::int_list<S0, S1, S ...>>
{
    using sub = Small<T, S1, S ...>;
    using list = mp::makelist<S0, sub>;
};

} // namespace ra

#undef CHECK_BOUNDS
