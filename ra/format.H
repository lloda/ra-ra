// -*- mode: c++; coding: utf-8 -*-
/// @file format.H
/// @brief Array output formatting, with some generic ostream sugar.

// (c) Daniel Llorens - 2010, 2016-2018
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <iterator>
#include <iosfwd>
#include <sstream>

namespace ra {

constexpr char const * esc_bold = "\x1b[01m";
constexpr char const * esc_unbold = "\x1b[0m";
constexpr char const * esc_red = "\x1b[31m";
constexpr char const * esc_green = "\x1b[32m";
constexpr char const * esc_cyan = "\x1b[36m";
constexpr char const * esc_yellow = "\x1b[33m";
constexpr char const * esc_blue = "\x1b[34m";
constexpr char const * esc_white = "\x1b[97m"; // an AIXTERM sequence
constexpr char const * esc_plain = "\x1b[39m";
constexpr char const * esc_reset = "\x1b[39m\x1b[0m"; // plain + unbold
constexpr char const * esc_pink = "\x1b[38;5;225m";
constexpr char const * esc_invert = "\x1b[07m";
constexpr char const * esc_underline = "\x1b[04m";

template <class ... A> inline std::string
format(A && ... a)
{
    std::ostringstream o; (o << ... << std::forward<A>(a)); return o.str();
}

inline std::string const & format(std::string const & s) { return s; }

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
