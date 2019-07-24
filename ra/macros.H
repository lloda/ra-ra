// -*- mode: c++; coding: utf-8 -*-
/// @file macros.H
/// @brief Fundamental macros and types.

// (c) Daniel Llorens - 2005, 2012, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cstddef>
#include <cassert>
#include <type_traits>
#include <utility>

#define STRINGIZE1( x ) #x
#define STRINGIZE( x ) STRINGIZE1( x )
#define JOIN1( x, y ) x##y
#define JOIN( x, y ) JOIN1( x, y )

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
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define FOR_EACH_RSEQ_N() 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOR_EACH_(N, what, ...) JOIN(FOR_EACH_, N)(what, __VA_ARGS__)
#define FOR_EACH(what, ...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)

// FIXME use __VA_OPT__(,) or __VA_OPT__(&&) in c++20
// https://en.cppreference.com/w/cpp/preprocessor/replace
// See examples/throw.C for the way to override this RA_ASSERT.

#ifndef RA_ASSERT
  #define RA_ASSERT(cond, ...) assert(cond)
#endif

#if defined(RA_DO_CHECK) && RA_DO_CHECK==0
  #define RA_CHECK( ... )
#else
  #define RA_CHECK( ... ) RA_ASSERT( __VA_ARGS__ )
#endif

namespace mp {

template <class B> constexpr bool exists = true;

struct identity
{
    template<class T>
    constexpr decltype(auto) operator()(T && t) const noexcept
    {
        return std::forward<T>(t);
    }
};

#define RA_IS_DEF(NAME, PRED)                                           \
    template <class A, class Enable=void> constexpr bool JOIN(NAME, _def) = false; \
    template <class A> constexpr bool JOIN(NAME, _def) < A, std::enable_if_t< PRED >> = true; \
    template <class A> constexpr bool NAME = JOIN(NAME, _def)< std::decay_t< A >>;

} // namespace mp
