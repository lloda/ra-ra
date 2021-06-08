// -*- mode: c++; coding: utf-8 -*-
/// @file macros.hh
/// @brief Fundamental macros and types.

// (c) Daniel Llorens - 2005, 2012, 2019-2020
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cstddef>
#include <cassert>
#include <type_traits>
#include <utility>

#define STRINGIZE_( x ) #x
#define STRINGIZE( x ) STRINGIZE_( x )
#define JOIN_( x, y ) x##y
#define JOIN( x, y ) JOIN_( x, y )

// by G. Pakosz @ http://stackoverflow.com/a/1872506
#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...) what(x) FOR_EACH_1(what, __VA_ARGS__)
#define FOR_EACH_3(what, x, ...) what(x) FOR_EACH_2(what, __VA_ARGS__)
#define FOR_EACH_4(what, x, ...) what(x) FOR_EACH_3(what, __VA_ARGS__)
#define FOR_EACH_5(what, x, ...) what(x) FOR_EACH_4(what, __VA_ARGS__)
#define FOR_EACH_6(what, x, ...) what(x) FOR_EACH_5(what, __VA_ARGS__)
#define FOR_EACH_7(what, x, ...) what(x) FOR_EACH_6(what, __VA_ARGS__)
#define FOR_EACH_8(what, x, ...) what(x) FOR_EACH_7(what, __VA_ARGS__)
#define FOR_EACH_9(what, x, ...) what(x) FOR_EACH_8(what, __VA_ARGS__)
#define FOR_EACH_10(what, x, ...) what(x) FOR_EACH_9(what, __VA_ARGS__)
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define FOR_EACH_RSEQ_N() 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOR_EACH_(N, what, ...) JOIN(FOR_EACH_, N)(what, __VA_ARGS__)
#define FOR_EACH(what, ...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)

// https://en.cppreference.com/w/cpp/preprocessor/replace
// See examples/throw.cc for the way to override this RA_ASSERT.

#ifndef RA_ASSERT
  #define RA_ASSERT(cond, ...) assert(cond)
#endif

#if defined(RA_DO_CHECK) && RA_DO_CHECK==0
  #define RA_CHECK( ... )
#else
  #define RA_CHECK( ... ) RA_ASSERT( __VA_ARGS__ )
#endif

namespace mp {

#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A> constexpr bool JOIN(NAME, _def) = false;         \
    template <class A> requires (PRED) constexpr bool JOIN(NAME, _def) < A > = true; \
    template <class A> constexpr bool NAME = JOIN(NAME, _def)< std::decay_t< A >>;

// Assign ops for settable array iterators; these must be members.
// For containers & views this might be defined differently.
// forward to make sure value y is not misused as ref [ra05].
#define RA_DEF_ASSIGNOPS_LINE(OP)                                       \
    for_each([](auto && y, auto && x) { std::forward<decltype(y)>(y) OP x; }, *this, x)
#define RA_DEF_ASSIGNOPS(OP)                                            \
    template <class X> constexpr void operator OP(X && x) { RA_DEF_ASSIGNOPS_LINE(OP); }

// Restate RA_DEF_ASSIGNOPS for expression classes since the template doesn't replace the assignment ops.
#define RA_DEF_ASSIGNOPS_SELF(TYPE)                                     \
    TYPE & operator=(TYPE && x) { RA_DEF_ASSIGNOPS_LINE(=); return *this; } \
    TYPE & operator=(TYPE const & x) { RA_DEF_ASSIGNOPS_LINE(=); return *this; } \
    constexpr TYPE(TYPE && x) = default;                                \
    constexpr TYPE(TYPE const & x) = default;

} // namespace mp
