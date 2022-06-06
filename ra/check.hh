// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Error function.

// (c) Daniel Llorens - 2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cassert>
// Feel free to remove these if you define your own RA_ASSERT.
#include "format.hh"
#include <iostream>

// https://en.cppreference.com/w/cpp/preprocessor/replace
// See examples/throw.cc for the way to override this RA_ASSERT.

#ifndef RA_ASSERT
#define RA_ASSERT(cond, ...)                                    \
    {                                                           \
        if (std::is_constant_evaluated()) {                     \
            assert(cond /* FIXME maybe one day */);             \
        } else if (bool c = cond; !c) {                         \
            std::cerr << ra::format("**** ra: ", ##__VA_ARGS__, " ****") << std::endl; \
            assert(c);                                          \
        }                                                       \
    }
#endif

#if defined(RA_DO_CHECK) && RA_DO_CHECK==0
  #define RA_CHECK( ... )
#else
  #define RA_CHECK( ... ) RA_ASSERT( __VA_ARGS__ )
#endif

// This is NOT included by format.hh to allow format() to be used in pre-defining RA_ASSERT.

#define RA_AFTER_CHECK Yes
