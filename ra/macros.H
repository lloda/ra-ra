
// (c) Daniel Llorens - 2005, 2012

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_MACROS_H
#define RA_MACROS_H

/// @file macros.H
/// @brief Fundamental macros and types.

#define GCC_VERSION (__GNUC__*10000+__GNUC_MINOR__*100+__GNUC_PATCHLEVEL__)

#include <cstddef>
#include <sys/types.h> // ssize_t
#include <type_traits>
#include <utility>

#define X_STRINGIZE(s) STRINGIZE_1(s)
#define STRINGIZE_1(x) #x
#define STRINGIZE(x) STRINGIZE_1(x)

#define JOIN( X, Y ) DO_JOIN( X, Y )
#define DO_JOIN( X, Y ) X##Y

// by G. Pakosz @ http://stackoverflow.com/a/1872506
#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...)\
  what(x);\
  FOR_EACH_1(what,  __VA_ARGS__);
#define FOR_EACH_3(what, x, ...)\
  what(x);\
  FOR_EACH_2(what, __VA_ARGS__);
#define FOR_EACH_4(what, x, ...)\
  what(x);\
  FOR_EACH_3(what,  __VA_ARGS__);
#define FOR_EACH_5(what, x, ...)\
  what(x);\
 FOR_EACH_4(what,  __VA_ARGS__);
#define FOR_EACH_6(what, x, ...)\
  what(x);\
  FOR_EACH_5(what,  __VA_ARGS__);
#define FOR_EACH_7(what, x, ...)\
  what(x);\
  FOR_EACH_6(what,  __VA_ARGS__);
#define FOR_EACH_8(what, x, ...)\
  what(x);\
  FOR_EACH_7(what,  __VA_ARGS__);

#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__)
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define FOR_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0

#define FOR_EACH_(N, what, x, ...) JOIN(FOR_EACH_, N)(what, x, __VA_ARGS__)
#define FOR_EACH(what, x, ...) FOR_EACH_(FOR_EACH_NARG(x, __VA_ARGS__), what, x, __VA_ARGS__)

namespace mp {

template <class B> constexpr bool exists = true;

struct identity
{
    template<class T>
    constexpr auto operator()(T && t) const noexcept -> decltype(auto)
    {
        return std::forward<T>(t);
    }
};

} // namespace mp

#endif // RA_MACROS_H
