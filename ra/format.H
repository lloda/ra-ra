
// (c) Daniel Llorens - 2010, 2016

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_FORMAT_H
#define RA_FORMAT_H

/// @file format.H
/// @brief Sugar for in-place ostringstream use.

#include <iterator>
#include <iosfwd>
#include <sstream>

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

#endif // RA_FORMAT_H
