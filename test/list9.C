// -*- mode: c++; coding: utf-8 -*-
/// @file list9.C
/// @brief Demo new warning in 9.1

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/*
https://gcc.gnu.org/onlinedocs/gcc-9.1.0/gcc/C_002b_002b-Dialect-Options.html#index-Winit-list-lifetime

    When a list constructor stores the begin pointer from the initializer_list
    argument, this doesn’t extend the lifetime of the array, so if a class variable
    is constructed from a temporary initializer_list, the pointer is left dangling
    by the end of the variable declaration statement.

<lloda> I don't understand this warning in 9.1                          [13:06]
<lloda> https://wandbox.org/permlink/RTIL3P6nRanIx4Sd
<lloda> I've read
<lloda>
        https://gcc.gnu.org/onlinedocs/gcc-9.1.0/gcc/C_002b_002b-Dialect-Options.html#index-Winit-list-lifetime
<lloda> the last item
<lloda> but doesn't it make a difference that I use p(a.begin()) and not
        p(x.begin()) ?
<lloda> either gives the same warning
<redi> no, it makes no difference                                       [13:07]
<redi> a and x are both just pointers to the same underlying array
<redi> the array is not owned by the initializer_list
<redi> do not store an initializer_list like that. ever. it's not a container.
<lloda> fair :O                                                         [13:08]
<lloda> then the { p = a.begin(); } should warn as well?
<redi> it's for INITIALIZING, not storing data
<redi> yes, ideally that would warn too
<redi> your comment says "works" but it's wrong. It doesn't work any
       differently, it just doesn't warn.                               [13:09]
*/

// /opt/gcc-9.1/bin/g++ -o list9 -std=c++17 -Wall -Werror list9.C

#include <utility>

struct Bar
{
    std::initializer_list<int> a;
    decltype(a.begin()) p;
    // Bar(std::initializer_list<int> x): a(x), p(a.begin()) {} // warns
    Bar(std::initializer_list<int> x): a(x) { p = a.begin(); } // doesn't (but same thing)
};

template <class T>
struct Foo
{
    Foo(std::initializer_list<int> s): Foo(Bar(s)) {}
};

int main() {}
