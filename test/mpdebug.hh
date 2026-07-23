// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Metaprogramming debug utilities.

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

namespace ra {

template <int V> using int_c = std::integral_constant<int, V>;
template <int V> using dim_c = std::integral_constant<dim_t, V>;

} // namespace ra

namespace ra::mp {

template <class type_, bool condition=false>
struct show
{
    using type = type_;
    static bool const value = condition;
    static_assert(condition, "bad type");
};

// Prints value recursively, like for int_c trees.

template <class A> struct print_ilist_t {};

template <class L> std::ostream &
operator<<(std::ostream & o, print_ilist_t<L> const & a)
{
    if constexpr (is_list<L>) {
        [&o]<class ... I>(list<I ...>){ ((o << "[") << ... << print_ilist_t<I> {}) << "]"; }(L {});
        return o;
    } else {
        return (o << L::value << " ");
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
template <> struct check_idx<list<>> { constexpr static bool value = true; };

template <class A0, int I0, class ... A, int ... I>
struct check_idx<list<A0, A ...>, I0, I ...>
{
    constexpr static bool value = (A0::value==I0) && check_idx<list<A ...>, I ...>::value;
};

} // namespace ra::mp
