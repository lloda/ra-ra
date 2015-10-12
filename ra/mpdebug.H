
// (c) Daniel Llorens - 2011

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_MPDEBUG_H
#define RA_MPDEBUG_H

#include <sys/types.h>
#include "ra/tuple-list.H"

/// @file mpdebug.H
/// @brief Metaprogramming debugging utilities.

namespace mp {

template <ssize_t value_, bool condition=false>
struct show_number
{
    static ssize_t const value = value_;
    static_assert(condition, "bad number");
};
template <class type_, bool condition=false>
struct show_type
{
    typedef type_ type;
    static bool const value = condition;
    static_assert(condition, "bad type");
};

// Also prints lists of lists.
template <class A>
struct print_int_list
{
    static std::ostream & f(std::ostream & o)
    {
        o << ".";
        return o;
    }
};
template <class A0, class ... A>
struct print_int_list<std::tuple<A0, A ...>>
{
    static std::ostream & f(std::ostream & o)
    {
        o << "[";
        print_int_list<A0>::f(o);
        print_int_list<std::tuple<A ...>>::f(o);
        return o;
    }
};
template <int i0, class ... A>
struct print_int_list<std::tuple<mp::int_t<i0>, A ...>>
{
    static std::ostream & f(std::ostream & o)
    {
        o << i0 << " ";
        print_int_list<std::tuple<A ...>>::f(o);
        return o;
    }
};
template <>
struct print_int_list<std::tuple<>>
{
    static std::ostream & f(std::ostream & o)
    {
        o << ";";
        return o;
    }
};

} // namespace MP

#endif // RA_MPDEBUG_H
