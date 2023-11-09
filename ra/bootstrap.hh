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
#include <iosfwd> // for format, ss.
#include <sstream>
#include <version>
#include <source_location>


// ---------------------
// Default #defines.
// ---------------------

// benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_DO_OPT_SMALLVECTOR
#define RA_DO_OPT_SMALLVECTOR 0
#endif

// no real downside.
#ifndef RA_DO_OPT_IOTA
#define RA_DO_OPT_IOTA 1
#endif

namespace ra {

constexpr int VERSION = 26;
// rank<0 is used as 'frame rank' in contrast to 'cell rank'. This limits the rank that ra:: can handle.
constexpr int ANY = -1944444444; // only at ct, meaning tbd at rt
constexpr int BAD = -1988888888; // undefined, eg dead axes

using rank_t = int;
using dim_t = std::ptrdiff_t;
static_assert(sizeof(rank_t)>=4 && sizeof(dim_t)>=4);
static_assert(sizeof(rank_t)>=sizeof(int) && sizeof(dim_t)>=sizeof(rank_t));
static_assert(std::is_signed_v<rank_t> && std::is_signed_v<dim_t>);

template <dim_t V> using dim_c = std::integral_constant<dim_t, V>;
template <rank_t V> using rank_c = std::integral_constant<rank_t, V>;
enum none_t { none }; // in constructors to mean: don't initialize
struct noarg { noarg() = delete; }; // in constructors to mean: don't instantiate

// forward decl, extended in ra.hh
constexpr bool any(bool const x) { return x; }
constexpr bool every(bool const x) { return x; }


// ---------------------
// concepts. Not sure i want duck typing, tbr.
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
    { a.rank() } -> std::same_as<rank_t>;
// FIXME still have ply(&) in places. Cf test/types.cc.
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
    { a.rank() } -> std::same_as<rank_t>;
    { a.iter() } -> IteratorConcept;
};


// --------------
// type classification / introspection
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
template <class A> constexpr bool is_ra = is_iterator<A> || SliceConcept<A>;
template <class A> constexpr bool is_builtin_array = std::is_array_v<std::remove_cvref_t<A>>;
RA_IS_DEF(is_fov, (!is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::random_access_range<A>))

template <class VV> requires (!std::is_void_v<VV>)
consteval rank_t
rank_s()
{
    using V = std::remove_cvref_t<VV>;
    if constexpr (is_builtin_array<V>) {
        return std::rank_v<V>;
    } else if constexpr (is_fov<V>) {
        return 1;
    } else if constexpr (requires { V::rank(); }) {
        return V::rank();
    } else if constexpr (requires (V v) { v.rank(); }) {
        return ANY;
    } else {
        return 0;
    }
}

template <class V> consteval rank_t rank_s(V const &) { return rank_s<V>(); } // waiting for c++23 p2280r4

template <class V>
constexpr rank_t
rank(V const & v)
{
    if constexpr (ANY!=rank_s<V>()) {
        return rank_s<V>();
    } else if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else {
        static_assert(always_false<V>, "No rank() for this type.");
    }
}

RA_IS_DEF(is_pos, 0!=rank_s<A>())
template <class A> constexpr bool is_ra_pos = is_ra<A> && is_pos<A>;
template <class A> constexpr bool is_zero_or_scalar = (is_ra<A> && !is_pos<A>) || is_scalar<A>;
// all args rank 0 (immediate application), but at least one ra:: (don't collide with the scalar version).
RA_IS_DEF(is_special, false) // rank-0 types that we don't want reduced.
template <class ... A> constexpr bool toreduce = (!is_scalar<A> || ...) && ((is_zero_or_scalar<A> && !is_special<A>) && ...);
template <class ... A> constexpr bool tomap = ((is_ra_pos<A> || is_special<A>) || ...) && ((is_ra<A> || is_scalar<A> || is_fov<A> || is_builtin_array<A>) && ...);


// --------------
// format
// --------------

enum print_shape_t { defaultshape, withshape, noshape };

template <class A>
struct FormatArray
{
    A const & a;
    print_shape_t shape;
    using pchar = char const *;
    pchar sep0, sep1, sep2;
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
operator<<(std::ostream & o, print_shape_t shape) { return shape_manip_t { o, shape }; }

// include is_fov bc shape() may be std::vector or std::array.
// exclude std::string_view to let it be is_fov and still print as a string [ra13].
template <class A> requires (is_ra<A> || (is_fov<A> && !std::is_convertible_v<A, std::string_view>))
constexpr std::ostream &
operator<<(std::ostream & o, A && a) { return o << format_array(a); }

template <class T>
constexpr std::ostream &
operator<<(std::ostream & o, std::initializer_list<T> const & a) { return o << format_array(a); }

template <class A>
constexpr std::ostream &
operator<<(shape_manip_t const & sm, FormatArray<A> fa) { return sm.o << (fa.shape=sm.shape, fa); }

template <class A>
constexpr std::ostream &
operator<<(shape_manip_t const & sm, A const & a) { return sm << format_array(a); }

/* constexpr */ inline std::ostream &
operator<<(std::ostream & o, std::source_location const & loc)
{
    return o << loc.file_name() << ":" << loc.line() << "," << loc.column();
}

template <class ... A>
constexpr std::string
format(A && ... a)
{
    if constexpr (sizeof ... (A)>0) {
        std::ostringstream o; (o << ... << RA_FWD(a)); return o.str();
    } else {
        return "";
    }
}

constexpr std::string const & format(std::string const & s) { return s; }

} // namespace ra

#ifdef RA_AFTER_CHECK
#error Bad header include order! Do not include ra/bootstrap.hh after other ra:: headers.
#endif
