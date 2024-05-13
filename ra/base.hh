// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Before all other ra:: includes.

// (c) Daniel Llorens - 2013-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <ranges>
#include <array>
#include <vector>
#include <cstdint>
#include "tuples.hh"
#include <iosfwd> // for format, ss.
#include <sstream>
#include <version>
#include <source_location>

// FIMXE benchmark shows it's bad by default; probably requires optimizing also +=, etc.
#ifndef RA_DO_OPT_SMALLVECTOR
#define RA_DO_OPT_SMALLVECTOR 0
#endif

namespace ra {

constexpr int VERSION = 29;
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

// default storage for Big - see https://stackoverflow.com/a/21028912.
// allocator adaptor that interposes construct() calls to convert value initialization into default initialization.
template <class T, class A=std::allocator<T>>
struct default_init_allocator: public A
{
    using a_t = std::allocator_traits<A>;
    using A::A;

    template <class U>
    struct rebind
    {
        using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
    };

    template <class U>
    void construct(U * ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
    {
        ::new(static_cast<void *>(ptr)) U;
    }
    template <class U, class... Args>
    void construct(U * ptr, Args &&... args)
    {
        a_t::construct(static_cast<A &>(*this), ptr, RA_FWD(args)...);
    }
};

template <class T> using vector_default_init = std::vector<T, default_init_allocator<T>>;


// ---------------------
// concepts
// ---------------------

template <class A>
concept IteratorConcept = requires (A a, rank_t k, dim_t d, rank_t i, rank_t j)
{
    { a.rank() } -> std::same_as<rank_t>;
    { std::decay_t<A>::len_s(k) } -> std::same_as<dim_t>;
    { a.len(k) } -> std::same_as<dim_t>;
    { a.adv(k, d) } -> std::same_as<void>;
    { a.step(k) };
    { a.keep_step(d, i, j) } -> std::same_as<bool>;
    { a.save() };
    { a.load(std::declval<decltype(a.save())>()) } -> std::same_as<void>;
    { a.mov(d) } -> std::same_as<void>;
    { *a };
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
    template <class A> concept NAME = JOIN(NAME, _def)<std::decay_t< A >>;

RA_IS_DEF(is_scalar, (!std::is_pointer_v<A> && std::is_scalar_v<A> || ra::is_constant<A>))
template <> constexpr bool is_scalar_def<std::strong_ordering> = true;
template <> constexpr bool is_scalar_def<std::weak_ordering> = true;
template <> constexpr bool is_scalar_def<std::partial_ordering> = true;
// template <> constexpr bool is_scalar_def<std::string_view> = true; // [ra13]

RA_IS_DEF(is_iterator, IteratorConcept<A>)
template <class A> concept is_ra = is_iterator<A> || SliceConcept<A>;
template <class A> concept is_builtin_array = std::is_array_v<std::remove_cvref_t<A>>;
RA_IS_DEF(is_fov, (!is_scalar<A> && !is_ra<A> && !is_builtin_array<A> && std::ranges::bidirectional_range<A>))

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

template <class V> consteval rank_t rank_s(V const &) { return rank_s<V>(); }

constexpr rank_t
rank(auto const & v)
{
    if constexpr (ANY!=rank_s(v)) {
        return rank_s(v);
    } else if constexpr (requires { v.rank(); })  {
        return v.rank();
    } else {
        static_assert(false, "No rank() for this type.");
    }
}

RA_IS_DEF(is_pos, 0!=rank_s<A>())
template <class A> concept is_ra_pos = is_ra<A> && is_pos<A>;
template <class A> concept is_zero_or_scalar = (is_ra<A> && !is_pos<A>) || is_scalar<A>;
// all args rank 0 (immediate application), but at least one ra:: (don't collide with the scalar version).
RA_IS_DEF(is_special, false) // rank-0 types that we don't want reduced.
template <class ... A> constexpr bool toreduce = (!is_scalar<A> || ...) && ((is_zero_or_scalar<A> && !is_special<A>) && ...);
template <class ... A> constexpr bool tomap = ((is_ra_pos<A> || is_special<A>) || ...) && ((is_ra<A> || is_scalar<A> || is_fov<A> || is_builtin_array<A>) && ...);

template <class VV> requires (!std::is_void_v<VV>)
consteval dim_t
size_s()
{
    using V = std::remove_cvref_t<VV>;
    constexpr rank_t rs = rank_s<V>();
    if constexpr (0==rs) {
        return 1;
    } else if constexpr (is_builtin_array<V>) {
        return std::apply([] (auto ... i) { return (std::extent_v<V, i> * ... * 1); }, mp::iota<rs> {});
    } else if constexpr (is_fov<V> && requires { std::tuple_size<V>::value; }) {
        return std::tuple_size_v<V>;
    } else if constexpr (is_fov<V> || rs==ANY) {
        return ANY;
    } else if constexpr (requires { V::size_s(); }) {
        return V::size_s();
    } else {
        dim_t s = 1;
        for (int i=0; i<rs; ++i) {
            if (dim_t ss=V::len_s(i); ss>=0) { s *= ss; } else { return ss; } // ANY or BAD
        }
        return s;
    }
}

template <class V> consteval dim_t size_s(V const &) { return size_s<V>(); }

template <class V>
constexpr dim_t
size(V const & v)
{
    if constexpr (ANY!=size_s(v)) {
        return size_s(v);
    } else if constexpr (is_fov<V>) {
        return std::ssize(v);
    } else if constexpr (requires { v.size(); }) {
        return v.size();
    } else {
        dim_t s = 1;
        for (rank_t k=0; k<rank(v); ++k) { s *= v.len(k); }
        return s;
    }
}

// Returns concrete types, value or const &. FIXME return ra:: types, but only if it's in all cases.
template <class V>
constexpr decltype(auto)
shape(V const & v)
{
    constexpr rank_t rs = rank_s(v);
// FIXME __cpp_constexpr >= 202211L to return references to the constexpr cases
    if constexpr (is_builtin_array<V>) {
        return std::apply([] (auto ... i) { return std::array<dim_t, rs> { std::extent_v<V, i> ... }; }, mp::iota<rs> {});
    } else if constexpr (requires { v.shape(); }) {
        return v.shape();
    } else if constexpr (0==rs) {
        return std::array<dim_t, 0> {};
    } else if constexpr (1==rs) {
        return std::array<dim_t, 1> { ra::size(v) };
    } else if constexpr (1<rs) {
        return std::apply([&v](auto ... i) { return std::array<dim_t, rs> { v.len(i) ... }; }, mp::iota<rs> {});
    } else {
        static_assert(ANY==rs);
        return std::ranges::to<vector_default_init<dim_t>>(
            std::ranges::iota_view { 0, rank(v) } | std::views::transform([&v](auto k) { return v.len(k); }));
    }
}


// --------------
// format
// --------------

enum print_shape_t { defaultshape, withshape, noshape };

struct array_format
{
    print_shape_t shape = defaultshape;
    char const * open = "";
    char const * close = "";
    char const * sep0 = " ";
    char const * sepn = "\n";
    char const * rep = "\n";
    bool align = false;
};

constexpr array_format jstyle = {};
constexpr array_format cstyle = { .shape=noshape, .open="{", .close="}", .sep0=", ", .sepn=",\n", .rep="", .align=true};
constexpr array_format lstyle = { .shape=noshape, .open="(", .close=")", .sep0=" ", .sepn="\n", .rep="", .align=true};
constexpr array_format pstyle = { .shape=noshape, .open="[", .close="]", .sep0=", ", .sepn=",\n", .rep="\n", .align=true};

template <class A>
struct FormatArray
{
    A const & a;
    array_format fmt = {};
};

constexpr auto
format_array(auto const & a, array_format fmt = {})
{
    return FormatArray<decltype(a)> { a,  fmt };
}

struct shape_manip_t
{
    std::ostream & o;
    print_shape_t shape;
};

constexpr shape_manip_t
operator<<(std::ostream & o, print_shape_t shape) { return shape_manip_t { o, shape }; }

// exclude std::string_view so it still prints as a string [ra13].
template <class A> requires (is_ra<A> || (is_fov<A> && !std::is_convertible_v<A, std::string_view>))
constexpr std::ostream &
operator<<(std::ostream & o, A && a) { return o << format_array(a); }

template <class T>
constexpr std::ostream &
operator<<(std::ostream & o, std::initializer_list<T> const & a) { return o << format_array(a); }

template <class A>
constexpr std::ostream &
operator<<(shape_manip_t const & sm, FormatArray<A> fa) { return sm.o << (fa.fmt.shape=sm.shape, fa); }

template <class A>
constexpr std::ostream &
operator<<(shape_manip_t const & sm, A const & a) { return sm << format_array(a); }

constexpr std::ostream &
operator<<(std::ostream & o, std::source_location const & loc)
{
    return o << loc.file_name() << ":" << loc.line() << "," << loc.column();
}

constexpr std::string
format(auto && ... a)
{
    if constexpr (sizeof... (a)>0) {
        std::ostringstream o; (o << ... << RA_FWD(a)); return o.str();
    } else {
        return "";
    }
}

constexpr std::string const &
format(std::string const & s) { return s; }

} // namespace ra

#ifdef RA_AFTER_CHECK
#error Bad header include order! Do not include ra/base.hh after other ra:: headers.
#endif
