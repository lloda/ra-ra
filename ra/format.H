
// (c) Daniel Llorens - 2010

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_FORMAT_H
#define RA_FORMAT_H

/// @file format.H
/// @brief Utilities to format output.

#include <iterator>
#include <iosfwd>
#include <sstream>

// Print vectors as a raw sequence, w/o size information.
template <class V>
struct Rawp
{
    V const & v;
    char const * sep;
    Rawp(V const & v_, char const * sep_=" "): v(v_), sep(sep_) {}
};

template <class V> Rawp<V> rawp(V const & v_, char const * sep_=" ")
{
    return Rawp<V>(v_, sep_);
}

template <class V>
std::ostream & operator<<(std::ostream & o, Rawp<V> const & r)
{
    std::copy(r.v.begin(), r.v.end(), std::ostream_iterator<decltype(*(r.v.begin()))>(o, r.sep));
    return o;
}

// Sugar for in-place ostringstream use.
inline void format(std::ostringstream & o) {}

template <class A0, class ... A>
inline void format(std::ostringstream & o, A0 const & a0, A const & ... a)
{
    o << a0;
    format(o, a ...);
}

template <class A0, class ... A>
inline std::string format(A0 const & a0, A const & ... a)
{
    std::ostringstream o;
    o << a0;
    format(o, a ...);
    return o.str();
}

template <class A0, class ... A>
inline decltype(auto) format(std::string && s)
{
    return std::forward<decltype(s)>(s);
}

#endif // RA_FORMAT_H
