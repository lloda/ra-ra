// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Before all other ra:: includes.

// (c) Daniel Llorens - 2013-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <ranges>
#include <array>
#include <cstdint>
#include "tuples.hh"


// ---------------------
// Default #defines.
// ---------------------
// Since these are tested with #if, define them at the very top so accidental nodef isn't mistaken for 0.

// benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_DO_OPT_SMALLVECTOR
#define RA_DO_OPT_SMALLVECTOR 0
#endif

// no real downside to this.
#ifndef RA_DO_OPT_IOTA
#define RA_DO_OPT_IOTA 1
#endif

namespace ra {

constexpr int VERSION = 23;

using rank_t = int;
using dim_t = std::ptrdiff_t;
static_assert(sizeof(rank_t)>=4 && sizeof(dim_t)>=4);
static_assert(std::is_signed_v<rank_t> && std::is_signed_v<dim_t>);
template <dim_t V> using dim_c = std::integral_constant<dim_t, V>;
template <rank_t V> using rank_c = std::integral_constant<rank_t, V>;

// Negative numbers are used in some places as 'frame rank' in contrast to 'cell rank', so this limits the rank that ra:: can handle.

constexpr dim_t DIM_ANY = -1944444444; // only at ct, meaning tbd at rt
constexpr rank_t RANK_ANY = -1944444444;
constexpr dim_t DIM_BAD = -1988888888; // undefined, eg dead axes
constexpr rank_t RANK_BAD = -1988888888;


// ---------------------
// concepts (WIP) - not sure i want duck typing, tbr.
// ---------------------

template <class P, class S>
concept FlatConcept = requires (P p, S d)
{
    { *p };
    { p += d };
};

template <class A>
concept IteratorConcept = requires (A a, rank_t k, dim_t d, rank_t i, rank_t j)
{
// FIXME we still allow ply(&) in some places. Cf also test/types.cc.
    { std::decay_t<A>::rank_s() } -> std::convertible_to<rank_t>;
    { a.rank() } -> std::convertible_to<rank_t>;
    { std::decay_t<A>::len_s(k) } -> std::convertible_to<dim_t>;
    { a.len(k) } -> std::same_as<dim_t>;
    { a.adv(k, d) } -> std::same_as<void>;
    { a.step(k) };
    { a.keep_step(d, i, j) } -> std::same_as<bool>;
    { a.flat() } -> FlatConcept<decltype(a.step(k))>;
};

template <class A>
concept SliceConcept = requires (A a)
{
    { A::rank_s() } -> std::convertible_to<rank_t>;
    { a.rank() } -> std::convertible_to<rank_t>;
    { a.iter() } -> IteratorConcept;
};


// ---------------------
// other types, forward decl
// ---------------------

enum none_t { none }; // used in array constructors to mean ‘don't initialize’
struct no_arg {}; // used in array constructors to mean ‘don't instantiate’

template <class C> struct Scalar; // for type predicates
template <class V> struct ra_traits_def;

template <class S> struct default_steps_ {};
template <class tend> struct default_steps_<std::tuple<tend>> { using type = mp::int_list<1>; };
template <> struct default_steps_<std::tuple<>> { using type = mp::int_list<>; };

template <class t0, class t1, class ... ti>
struct default_steps_<std::tuple<t0, t1, ti ...>>
{
    using rest = typename default_steps_<std::tuple<t1, ti ...>>::type;
    constexpr static int step0 = t1::value * mp::first<rest>::value;
    using type = mp::cons<ic_t<step0>, rest>;
};

template <class S> using default_steps = typename default_steps_<S>::type;

template <int n=DIM_BAD> struct dots_t
{
    static_assert(n>=0 || DIM_BAD==n);
    constexpr static rank_t rank_s() { return n; }
};
template <int n=DIM_BAD> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t
{
    static_assert(n>=0);
    constexpr static rank_t rank_s() { return n; }
};

template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

// Common to View / SmallBase. TODO Shouldn't it work on ... foreign vectors? arbitrary exprs?
template <int cell_rank, class A> constexpr auto
iter(A && a) { return std::forward<A>(a).template iter<cell_rank>(); }

// Used in big.hh (selectors, etc).
template <class A, class ... I> constexpr auto from(A && a, I && ... i);

// Extended in ra.hh (reductions)
constexpr bool any(bool const x) { return x; }
constexpr bool every(bool const x) { return x; }
constexpr bool odd(unsigned int N) { return N & 1; }


// ---------------------
// nested braces for Small initializers
// ---------------------

// This logically belongs in ra/small.hh, but it's here so that shape() can return ra:: types.

// The general SmallArray has 4 constructors,
// 1. The empty constructor.
// 2. The scalar constructor. This is needed when T isn't registered as ra::scalar, which isn't required purely for container use.
// 3. The ravel constructor.
// 4. The nested constructor.
// When SmallArray has rank 1, or the first dimension is empty, or the shape is [1] or [], several of the constructors above become ambiguous. We solve this by defining the constructor arguments to variants of no_arg.

template <class T, class lens>
struct nested_tuple;

// ambiguity with empty constructor and scalar constructor.
// if len(0) is 0, then prefer the empty constructor.
// if len(0) is 1...
template <class lens> constexpr bool no_nested = (mp::first<lens>::value<1);
template <> constexpr bool no_nested<mp::nil> = true;
template <> constexpr bool no_nested<mp::int_list<1>> = true;
template <class T, class lens>
using nested_arg = std::conditional_t<no_nested<lens>,
                                      std::tuple<no_arg>, // match the template for SmallArray.
                                      typename nested_tuple<T, lens>::list>;

// ambiguity with scalar constructors (for rank 0) and nested_tuple (for rank 1).
template <class lens> constexpr bool no_ravel = ((mp::len<lens> <=1) || (mp::apply<mp::prod, lens>::value <= 1));
template <class T, class lens>
using ravel_arg = std::conditional_t<no_ravel<lens>,
                                     std::tuple<no_arg, no_arg>, // match the template for SmallArray.
                                     mp::makelist<mp::apply<mp::prod, lens>::value, T>>;

template <class T, class lens, class steps = default_steps<lens>> struct SmallView; // for CellSmall
template <class T, class lens, class steps = default_steps<lens>,
          class nested_arg_ = nested_arg<T, lens>, class ravel_arg_ = ravel_arg<T, lens>>
struct SmallArray;
template <class T, dim_t ... lens> using Small = SmallArray<T, mp::int_list<lens ...>>;

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


// --------------
// type classification
// --------------

// FIXME https://wg21.link/p2841r0 ?
#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A> constexpr bool JOIN(NAME, _def) = requires { requires PRED; }; \
    template <class A> constexpr bool NAME = JOIN(NAME, _def)<std::decay_t< A >>;

// ra_traits are for foreign types only. FIXME Not sure this is the interface I want.

RA_IS_DEF(is_scalar, (!std::is_pointer_v<A> && std::is_scalar_v<A> || ra::is_constant<A>))
template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;
// template <> constexpr bool is_scalar_def<std::string_view> = true; // [ra13]

template <class T> requires (is_scalar<T>)
struct ra_traits_def<T>
{
    using V = T;
    constexpr static auto shape(V const & v) { return std::array<dim_t, 0> {}; }
    constexpr static dim_t size(V const & v) { return 1; }
    constexpr static dim_t size_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 0; }
    constexpr static rank_t rank_s() { return 0; }
};

RA_IS_DEF(is_iterator, IteratorConcept<A>)
RA_IS_DEF(is_iterator_pos_rank, IteratorConcept<A> && A::rank_s()!=0)
RA_IS_DEF(is_slice, SliceConcept<A>)
RA_IS_DEF(is_slice_pos_rank, SliceConcept<A> && A::rank_s()!=0)

template <class A> constexpr bool is_ra = is_iterator<A> || is_slice<A>;
template <class A> constexpr bool is_ra_pos_rank = is_iterator_pos_rank<A> || is_slice_pos_rank<A>; // internal only FIXME
template <class A> constexpr bool is_zero_or_scalar = (is_ra<A> && !is_ra_pos_rank<A>) || is_scalar<A>;

// ra_traits in small.hh.
template <class A> constexpr bool is_builtin_array = std::is_array_v<std::remove_cvref_t<A>>;

// std::string is std::ranges::range, but if we have it as is_scalar, we can't have it as is_foreign_vector.
RA_IS_DEF(is_foreign_vector, (!is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::random_access_range<A>))

// not using decay_t bc of builtin arrays.
template <class A> using ra_traits = ra_traits_def<std::remove_cvref_t<A>>;

template <class V>
requires (is_foreign_vector<V> && requires { std::tuple_size<V>::value; })
struct ra_traits_def<V>
{
    constexpr static rank_t rank_s() { return 1; };
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static dim_t size_s() { return std::tuple_size_v<V>; }
    constexpr static dim_t size(V const & v) { return size_s(); }
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { size_s() }; }
};

template <class V>
requires (is_foreign_vector<V> && !(requires { std::tuple_size<V>::value; }))
struct ra_traits_def<V>
{
    constexpr static rank_t rank_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr static dim_t size(V const & v) { return std::ssize(v); }
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { size(v) }; }
};

template <class T>
struct ra_traits_def<std::initializer_list<T>>
{
    using V = std::initializer_list<T>;
    constexpr static rank_t rank_s() { return 1; }
    constexpr static rank_t rank(V const & v) { return 1; }
    constexpr static dim_t size_s() { return DIM_ANY; }
    constexpr static dim_t size(V const & v) { return std::ssize(v); }
    constexpr static auto shape(V const & v) { return std::array<dim_t, 1> { size(v) }; }
};

RA_IS_DEF(is_special, false) // these are rank-0 types that we don't want reduced.

// all args have rank 0 (so immediate application), but at least one is ra:: (don't collide with the scalar version).
template <class ... A> constexpr bool ra_reducible = (!is_scalar<A> || ...) && ((is_zero_or_scalar<A> && !is_special<A>) && ...);
template <class ... A> constexpr bool ra_irreducible = ((is_ra_pos_rank<A> || is_special<A>) || ...) && ((is_ra<A> || is_scalar<A> || is_foreign_vector<A> || is_builtin_array<A>) && ...);

} // namespace ra


// --------------
// array output formatting
// --------------

#include <iterator>
#include <iosfwd>
#include <sstream>
#include <version>
#include <source_location>

namespace ra {

constexpr char const * esc_bold = "\x1b[01m";
constexpr char const * esc_unbold = "\x1b[0m";
constexpr char const * esc_invert = "\x1b[07m";
constexpr char const * esc_underline = "\x1b[04m";
constexpr char const * esc_red = "\x1b[31m";
constexpr char const * esc_green = "\x1b[32m";
constexpr char const * esc_cyan = "\x1b[36m";
constexpr char const * esc_yellow = "\x1b[33m";
constexpr char const * esc_blue = "\x1b[34m";
constexpr char const * esc_white = "\x1b[97m"; // an AIXTERM sequence
constexpr char const * esc_plain = "\x1b[39m";
constexpr char const * esc_reset = "\x1b[39m\x1b[0m"; // plain + unbold
constexpr char const * esc_pink = "\x1b[38;5;225m";

enum print_shape_t { defaultshape, withshape, noshape };

template <class A>
struct FormatArray
{
    A const & a;
    print_shape_t shape;
    char const * sep0;
    char const * sep1;
    char const * sep2;
};

template <class A>
constexpr FormatArray<A>
format_array(A const & a, char const * sep0=" ", char const * sep1="\n", char const * sep2="\n")
{
    return FormatArray<A> { a,  defaultshape, sep0, sep1, sep2 };
}

struct shape_manip_t
{
    std::ostream & o;
    print_shape_t shape;
};

constexpr shape_manip_t
operator<<(std::ostream & o, print_shape_t shape)
{
    return shape_manip_t { o, shape };
}

// is_foreign_vector is included bc std::vector or std::array may be used as the type of shape().
// Excluding std::string_view allows it to be is_foreign_vector and still print as a string [ra13].

template <class A> requires (is_ra<A> || (is_foreign_vector<A> && !std::is_convertible_v<A, std::string_view>))
constexpr std::ostream &
operator<<(std::ostream & o, A && a)
{
    return o << format_array(a);
}

// initializer_list cannot match A && above.
template <class T>
constexpr std::ostream &
operator<<(std::ostream & o, std::initializer_list<T> const & a)
{
    return o << format_array(a);
}

template <class A>
constexpr std::ostream &
operator<<(shape_manip_t const & sm, A const & a)
{
    FormatArray<A> fa = format_array(a);
    fa.shape = sm.shape;
    return sm.o << fa;
}

template <class A>
constexpr std::ostream &
operator<<(shape_manip_t const & sm, FormatArray<A> fa)
{
    fa.shape = sm.shape;
    return sm.o << fa;
}

inline std::ostream &
operator<<(std::ostream & o, std::source_location const & loc)
{
    o << loc.file_name() << ":" << loc.line() << "," << loc.column();
    return o;
}

template <class ... A>
constexpr std::string
format(A && ... a)
{
    if constexpr (sizeof ... (A)>0) {
        std::ostringstream o; (o << ... << std::forward<A>(a)); return o.str();
    } else {
        return "";
    }
}

constexpr std::string const &
format(std::string const & s) { return s; }

} // namespace ra

#ifdef RA_AFTER_CHECK
#error Bad header include order! Do not include ra/bootstrap.hh after ra/ra.hh.
#endif
