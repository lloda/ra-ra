// -*- mode: c++; coding: utf-8 -*-
/// @file vector-array.cc
/// @brief Comparison of ra::vector(std::vector) vs ra::vector(std::array)

// (c) Daniel Llorens - 2021
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
struct Vec
{
    V v;
    // decltype(size(v)) s = size(v); // (1) changes (2)
    decltype(v.begin()) p = v.begin();
};

template <class V> inline constexpr auto vec(V && v)
{
    return Vec<V> { std::forward<V>(v) };
}

int main()
{
    auto f1 = []() { return std::array<int, 2> {7, 2}; };
    auto f2 = []() { return std::vector<int> {5, 2}; };
    auto v1 = vec(f1());
    auto v2 = vec(f2());

    cout << (v1.v.begin()==v1.p) << endl; // (2)
    cout << (v2.v.begin()==v2.p) << endl;

    return 0;
}
