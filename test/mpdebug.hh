// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Metaprogramming debug utilities.

// (c) Daniel Llorens - 2011, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ra/base.hh"
#include <iosfwd>
#include <string>
#include <typeinfo>
#include <cxxabi.h>

namespace ra::mp {

template <class type_, bool condition=false>
struct show
{
    using type = type_;
    static bool const value = condition;
    static_assert(condition, "bad type");
};

// Prints value recursively, e.g. for int_c trees.

template <class A> struct print_ilist_t {};

template <class A> std::ostream &
operator<<(std::ostream & o, print_ilist_t<A> const & a)
{
    if constexpr (is_tuple<A>) {
        std::apply([&o](auto ... a) { ((o << "[") << ... << print_ilist_t<decltype(a)> {}) << "]"; }, A {});
        return o;
    } else {
        return (o << A::value << " ");
    }
}

template <class T>
std::string
type_name()
{
    int status;
    auto s = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status);
    std::string out(s);
    free(s);
    return out;
}

template <class A, int ... I> struct check_idx { constexpr static bool value = false; };
template <> struct check_idx<nil> { constexpr static bool value = true; };

template <class A0, int I0, class ... A, int ... I>
struct check_idx<tuple<A0, A ...>, I0, I ...>
{
    constexpr static bool value = (A0::value==I0) && check_idx<tuple<A ...>, I ...>::value;
};

} // namespace ra::mp
