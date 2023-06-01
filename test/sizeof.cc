// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - What to expect of sizeof... no checks.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"

struct no_argx {}; // Small uses no_arg for its own

struct sample1
{
    uint8_t a[16];
    [[no_unique_address]] ra::Small<int, 0> b; // adds space for an int anyway :-/
};

struct sample2
{
    uint8_t a[16];
    [[no_unique_address]] ra::Small<no_argx, 1> b;
};

struct sample3
{
    uint8_t a[16];
    [[no_unique_address]] int b[0]; // adds actually zero
};

struct sample4
{
    uint8_t a[16];
    [[no_unique_address]] no_argx b[1];
};

struct sample5
{
    uint8_t a[16];
    [[no_unique_address]] std::array<int, 0> b; // adds one byte (array<T, 0> doesn't use T[0])
};

struct sample6
{
    uint8_t a[16];
    [[no_unique_address]] std::array<no_argx, 1> b;
};

int main()
{
    ra::TestRecorder tr;
    tr.section("self");
    {
        std::cout << sizeof(std::array<ra::none_t, 9> {}) << std::endl;
        std::cout << sizeof(std::array<ra::no_arg, 9> {}) << std::endl;
        std::cout << sizeof(ra::none_t [9]) << std::endl;
        std::cout << sizeof(ra::no_arg [9]) << std::endl;
        std::cout << sizeof(std::array<ra::none_t, 0> {}) << std::endl;
        std::cout << sizeof(std::array<ra::no_arg, 0> {}) << std::endl;
        std::cout << sizeof(ra::none_t [0]) << std::endl;
        std::cout << sizeof(ra::no_arg [0]) << std::endl;
        std::cout << sizeof(ra::Small<int, 0>) << std::endl;
        std::cout << sizeof(ra::Small<ra::none_t, 0>) << std::endl;
        std::cout << sizeof(ra::Small<no_argx, 0>) << std::endl;
    }
    tr.section("base class");
    {
        std::cout << sizeof(sample1) << std::endl;
        std::cout << sizeof(sample2) << std::endl;
        std::cout << sizeof(sample3) << std::endl;
        std::cout << sizeof(sample4) << std::endl;
        std::cout << sizeof(sample5) << std::endl;
        std::cout << sizeof(sample6) << std::endl;
    }
}
