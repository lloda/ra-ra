
// (c) Daniel Llorens - 2010, 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file format.H
/// @brief Array output formatting, with some generic ostream sugar.

#pragma once
#include <iterator>
#include <iosfwd>
#include <sstream>

namespace ra {

template <class ... A> inline std::string
format(A && ... a)
{
    std::ostringstream o; (o << ... << a); return o.str();
}

inline decltype(auto)
format(std::string && s)
{
    return std::forward<decltype(s)>(s);
}

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

template <class A> inline
FormatArray<A>
format_array(A const & a, char const * sep0=" ", char const * sep1="\n", char const * sep2="\n")
{
    return FormatArray<A> { a,  defaultshape, sep0, sep1, sep2 };
}

struct shape_manip_t
{
    std::ostream & o;
    print_shape_t shape;
};

inline shape_manip_t operator<<(std::ostream & o, print_shape_t shape)
{
    return shape_manip_t { o, shape };
}

template <class A>
inline std::ostream & operator<<(shape_manip_t const & sm, A const & a)
{
    FormatArray<A> fa = format_array(a);
    fa.shape = sm.shape;
    return sm.o << fa;
}

template <class A>
inline std::ostream & operator<<(shape_manip_t const & sm, FormatArray<A> fa)
{
    fa.shape = sm.shape;
    return sm.o << fa;
}

} // namespace ra
