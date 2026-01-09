// -*- mode: c++; coding: utf-8 -*-
// ra/test - Godbolted version of [ra8] (cf operators.cc)

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// $CXX -std=c++20 -o tc tc.cc

#include <iostream>

template <class C>
struct Scalar
{
    C c;
    constexpr C & operator*() { return this->c; }
    constexpr C const & operator*() const { return this->c; }
};

template <class C> constexpr auto scalar(C && c) { return Scalar<C> { std::forward<C>(c) }; }

template <class T> requires (std::is_scalar_v<std::decay_t<T>>)
constexpr decltype(auto)
iter(T && t) { return scalar(std::forward<T>(t)); }

template <class T>
constexpr decltype(auto)
iter(Scalar<T> && t) { return std::forward<decltype(t)>(t); }

template <class A>
constexpr decltype(auto)
VAL(A && a)
{
    return *(iter(std::forward<A>(a)));
}

template <class A, class B>
constexpr auto operator +(A && a, B && b)
{
    return VAL(std::forward<A>(a)) + VAL(std::forward<B>(b));
}

int main()
{
    // int x0 = 1 + scalar(2); // not ok for -O1 or higher
    // std::cout << x0 << std::endl;

    constexpr int q = 1;
    constexpr int x1 = q + scalar(2); // ok
    std::cout << x1 << std::endl;

    // constexpr int x2 = 1 + scalar(2); // not ok
    // std::cout << x2 << std::endl;
}
