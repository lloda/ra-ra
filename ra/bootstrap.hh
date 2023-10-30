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
// Since these are tested with #if, give defaults so accidental nodef isn't mistaken for 0.

// benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_DO_OPT_SMALLVECTOR
#define RA_DO_OPT_SMALLVECTOR 0
#endif

// no real downside.
#ifndef RA_DO_OPT_IOTA
#define RA_DO_OPT_IOTA 1
#endif

namespace ra {

constexpr int VERSION = 25;

using rank_t = int;
using dim_t = std::ptrdiff_t;
static_assert(sizeof(rank_t)>=4 && sizeof(dim_t)>=4);
static_assert(sizeof(rank_t)>=sizeof(int) && sizeof(dim_t)>=sizeof(rank_t));
static_assert(std::is_signed_v<rank_t> && std::is_signed_v<dim_t>);
template <dim_t V> using dim_c = std::integral_constant<dim_t, V>;
template <rank_t V> using rank_c = std::integral_constant<rank_t, V>;

// rank<0 is used places as 'frame rank' in contrast to 'cell rank'. This limits the rank that ra:: can handle.
constexpr int ANY = -1944444444; // only at ct, meaning tbd at rt
constexpr int BAD = -1988888888; // undefined, eg dead axes


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
    { std::decay_t<A>::rank_s() } -> std::same_as<rank_t>;
    { a.rank() } -> std::same_as<rank_t>;
    { std::decay_t<A>::len_s(k) } -> std::same_as<dim_t>;
    { a.len(k) } -> std::same_as<dim_t>;
    { a.adv(k, d) } -> std::same_as<void>;
    { a.step(k) };
    { a.keep_step(d, i, j) } -> std::same_as<bool>;
    { a.flat() } -> FlatConcept<decltype(a.step(k))>;
};

template <class A>
concept SliceConcept = requires (A a)
{
    { A::rank_s() } -> std::same_as<rank_t>;
    { a.rank() } -> std::same_as<rank_t>;
    { a.iter() } -> IteratorConcept;
};


// ---------------------
// other types, forward decl
// ---------------------

enum none_t { none }; // in array constructors means ‘don't initialize’
struct noarg { noarg() = delete; }; // in array constructors means ‘don't instantiate’

template <class C> struct Scalar; // for type predicates

template <int n=BAD> struct dots_t
{
    static_assert(n>=0 || BAD==n);
    consteval static rank_t rank_s() { return n; }
};
template <int n=BAD> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t
{
    static_assert(n>=0);
    consteval static rank_t rank_s() { return n; }
};

template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

// For views. TODO on foreign vectors? arbitrary exprs?
template <int crank, class A> constexpr auto
iter(A && a) { return std::forward<A>(a).template iter<crank>(); }

// Used in big.hh (selectors, etc).
template <class A, class ... I> constexpr decltype(auto)
from(A && a, I && ... i);

// Extended in ra.hh (reductions)
constexpr bool any(bool const x) { return x; }
constexpr bool every(bool const x) { return x; }
constexpr bool odd(unsigned int N) { return N & 1; }


// --------------
// type classification
// --------------

// FIXME https://wg21.link/p2841r0 ?
#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A> constexpr bool JOIN(NAME, _def) = requires { requires PRED; }; \
    template <class A> constexpr bool NAME = JOIN(NAME, _def)<std::decay_t< A >>;

RA_IS_DEF(is_scalar, (!std::is_pointer_v<A> && std::is_scalar_v<A> || ra::is_constant<A>))
template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;
// template <> constexpr bool is_scalar_def<std::string_view> = true; // [ra13]

RA_IS_DEF(is_iterator, IteratorConcept<A>)
RA_IS_DEF(is_iterator_pos_rank, IteratorConcept<A> && 0!=A::rank_s())
RA_IS_DEF(is_slice, SliceConcept<A>)
RA_IS_DEF(is_slice_pos_rank, SliceConcept<A> && 0!=A::rank_s())

template <class A> constexpr bool is_ra = is_iterator<A> || is_slice<A>;
template <class A> constexpr bool is_ra_pos_rank = is_iterator_pos_rank<A> || is_slice_pos_rank<A>;
template <class A> constexpr bool is_zero_or_scalar = (is_ra<A> && !is_ra_pos_rank<A>) || is_scalar<A>;
template <class A> constexpr bool is_builtin_array = std::is_array_v<std::remove_cvref_t<A>>;

RA_IS_DEF(is_fov, (!is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::random_access_range<A>))
RA_IS_DEF(is_special, false) // these are rank-0 types that we don't want reduced.

// all args rank 0 (so immediate application), but at least one ra:: (don't collide with the scalar version).
template <class ... A> constexpr bool ra_reducible = (!is_scalar<A> || ...) && ((is_zero_or_scalar<A> && !is_special<A>) && ...);
template <class ... A> constexpr bool ra_irreducible = ((is_ra_pos_rank<A> || is_special<A>) || ...) && ((is_ra<A> || is_scalar<A> || is_fov<A> || is_builtin_array<A>) && ...);

} // namespace ra


// --------------
// formatting
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

// is_fov is included bc std::vector or std::array may be used as the type of shape().
// Excluding std::string_view allows it to be is_fov and still print as a string [ra13].

template <class A> requires (is_ra<A> || (is_fov<A> && !std::is_convertible_v<A, std::string_view>))
constexpr std::ostream &
operator<<(std::ostream & o, A && a)
{
    return o << format_array(a);
}

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
