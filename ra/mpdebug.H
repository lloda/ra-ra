
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
    using type = type_;
    static bool const value = condition;
    static_assert(condition, "bad type");
};

// Prints recursively, i.e. int_t trees.

template <class A> struct print_int_list {};

template <class A> std::ostream &
operator<<(std::ostream & o, print_int_list<A> const & a)
{
    return print_int_list<A>::f(o);
}

template <class ... A>
struct print_int_list<std::tuple<A ...> >
{
    static std::ostream & f(std::ostream & o)
    {
        return ((o << "[") << ... << print_int_list<A> {}) << "]";
    }
};
template <int i0>
struct print_int_list<mp::int_t<i0> >
{
    static std::ostream & f(std::ostream & o)
    {
        return (o << i0 << " ");
    }
};

} // namespace MP

#endif // RA_MPDEBUG_H
