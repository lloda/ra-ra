// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Metaprogramming debug utilities.

// (c) Daniel Llorens - 2011, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/tuples.hh"
#include <typeinfo>
#include <sys/types.h>

namespace ra::mp {

template <class type_, bool condition=false>
struct show
{
    using type = type_;
    static bool const value = condition;
    static_assert(condition, "bad type");
};

// Prints value recursively, e.g. for int_c trees.

template <class A> struct print_int_list {};

template <class A> std::ostream &
operator<<(std::ostream & o, print_int_list<A> const & a)
{
    if constexpr (is_tuple<A>) {
        std::apply([&o](auto ... a) { ((o << "[") << ... << print_int_list<decltype(a)> {}) << "]"; }, A {});
        return o;
    } else {
        return (o << A::value << " ");
    }
}

template <class T>
std::string
type_name()
{
    using TR = std::remove_cvref_t<T>;
    std::string r = typeid(TR).name();
    if (std::is_const_v<TR>)
        r += " const";
    if (std::is_volatile_v<TR>)
        r += " volatile";
    if (std::is_lvalue_reference_v<T>)
        r += " &";
    else if (std::is_rvalue_reference_v<T>)
        r += " &&";
    return r;
}

template <class A, int ... I> struct check_idx { constexpr static bool value = false; };
template <> struct check_idx<nil> { constexpr static bool value = true; };

template <class A0, int I0, class ... A, int ... I>
struct check_idx<tuple<A0, A ...>, I0, I ...>
{
    constexpr static bool value = (A0::value==I0) && check_idx<tuple<A ...>, I ...>::value;
};

} // namespace ra::mp
