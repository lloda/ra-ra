// -*- mode: c++; coding: utf-8 -*-
/// @file vector-array.cc
/// @brief Comparison of ra::vector(std::vector) vs ra::vector(std::array)

// (c) Daniel Llorens - 2013-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// This checks an alternate version of ra::Vector that doesn't work (but it should?). Cf [ra35] in ra-9.cc.

#include <iostream>
#include <array>
#include <vector>

using std::cout, std::endl;

template <class V>
struct Wector
{
    V v;
    decltype(v.begin()) p = v.begin();
};

template <class V> inline constexpr auto wector(V && v) { return Wector<V> { std::forward<V>(v) }; }

int main()
{
    auto fun1 = []() { return std::array<int, 2> {7, 2}; };
    auto fun2 = []() { return std::vector<int> {5, 2}; };
    auto v1 = wector(fun1());
    auto v2 = wector(fun2());

    cout << (!std::is_reference_v<decltype(v1.v)>) << endl;
    cout << (!std::is_reference_v<decltype(v2.v)>) << endl;

    cout << endl;

    cout << "&(v1.v[0])         " << &(v1.v[0]) << endl;
    cout << "&(*(v1.v.begin())) " << &(*(v1.v.begin())) << endl;
    cout << "&(v1.p[0])       " << &(v1.p[0]) << endl; // BAD
    cout << "&v1                " << &v1 << endl;

    cout << endl;

    cout << "&(v2.v[0])         " << &(v2.v[0]) << endl;
    cout << "&(*(v2.v.begin())) " << &(*(v2.v.begin())) << endl;
    cout << "&(v2.p[0])       " << &(v2.p[0]) << endl;
    cout << "&v2                " << &v2 << endl;

    return 0;
}
