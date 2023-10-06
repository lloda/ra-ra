// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Comparison of ra::vector(std::vector) vs ra::vector(std::array)

// (c) Daniel Llorens - 2021
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// This checks an alternate version of ra::Vector that doesn't work. Cf [ra35] in ra-9.cc.
/*
<PJBoy> lloda, ok I'm convinced it's because Vec is trivially copyable for
        std::array<int, n>                                              [22:00]
<PJBoy> after some help from the C++ discord :D                         [22:01]
<PJBoy> there's some standardese wording for it
        https://eel.is/c++draft/class.temporary#3                       [22:02]
<ville> well that's fun                                                 [22:05]
<PJBoy> tldr being something like, trivially copyable+moveable types can be
        arbitrarily copied without your consent                         [22:07]
<lloda> thx PJBoy
<PJBoy> which means the iterator can never be assumed to be valid after
        passing the array up or down the call stack                     [22:08]
<PJBoy> explicitly deleting or defining the copy/move ctors is the most direct
        workaround
*/

#include <iostream>
#include <array>
#include <vector>

using std::cout, std::endl;

template <class V>
struct Vec
{
    V v;
    decltype(v.begin()) p = v.begin();
};

template <class V>
constexpr auto
vec(V && v)
{
    return Vec<V> { std::forward<V>(v) };
}

int main()
{
    auto f1 = [] { return std::array {7, 2}; };
    auto f2 = [] { return std::vector {5, 2}; };
    auto v1 = vec(f1());
    auto v2 = vec(f2());

    cout << (v1.v.begin()==v1.p) << endl; // unfortunate
    cout << (v2.v.begin()==v2.p) << endl;

    return 0;
}
