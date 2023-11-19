// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Basic macros.

// (c) Daniel Llorens - 2005--2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <utility>

#define STRINGIZE_( x ) #x
#define STRINGIZE( x ) STRINGIZE_( x )
#define JOIN_( x, y ) x##y
#define JOIN( x, y ) JOIN_( x, y )

// see http://stackoverflow.com/a/1872506
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
#define FOR_EACH_11(what, x, ...) what(x) FOR_EACH_10(what, __VA_ARGS__)
#define FOR_EACH_12(what, x, ...) what(x) FOR_EACH_11(what, __VA_ARGS__)
#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) N
#define FOR_EACH_RSEQ_N() 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOR_EACH_(N, what, ...) JOIN(FOR_EACH_, N)(what, __VA_ARGS__)
#define FOR_EACH(what, ...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)

#define RA_FWD(a) std::forward<decltype(a)>(a)
