// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - What to expect of sizeof... no checks.

// (c) Daniel Llorens - 2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"

using std::cout, std::endl;

struct noargx {}; // Small uses noarg for its own

struct sample1
{
    uint8_t a[16];
    [[no_unique_address]] ra::Small<int, 0> b; // adds actually 0, but I don't rely on this atm
};

struct sample2
{
    uint8_t a[16];
    [[no_unique_address]] ra::Small<noargx, 1> b; // adds a byte
};

struct sample3
{
    uint8_t a[16];
    [[no_unique_address]] int b[0]; // adds actually zero
};

struct sample4
{
    uint8_t a[16];
    [[no_unique_address]] noargx b[1]; // adds a byte
};

struct sample5
{
    uint8_t a[16];
    [[no_unique_address]] std::array<int, 0> b; // adds a byte (array<T, 0> doesn't use T[0])
};

struct sample6
{
    uint8_t a[16];
    [[no_unique_address]] std::array<noargx, 1> b; // adds a byte
};

int main()
{
    ra::TestRecorder tr;
    tr.section("self");
    {
        cout << sizeof(std::array<ra::none_t, 9> {}) << endl;
        cout << sizeof(ra::none_t [9]) << endl;
        cout << sizeof(ra::noarg [9]) << endl;
        cout << sizeof(std::array<ra::none_t, 0> {}) << endl;
        cout << sizeof(ra::none_t [0]) << endl;
        cout << sizeof(ra::noarg [0]) << endl;
        cout << sizeof(ra::Small<int, 0>) << endl;
        cout << sizeof(ra::Small<ra::none_t, 0>) << endl;
        cout << sizeof(ra::Small<noargx, 0>) << endl;
// the following tests were broken at some point because Small had a fixed empty base class, so nesting Smalls created multiples of these which added useless padding. Not a fan of the rule but w/e - the base class wasn't doing all that much.
        {
            using nest = ra::Small<ra::Small<int, 2>, 2>;
            cout << "nest " << sizeof(nest) << " " << alignof(nest) << endl;
            nest x = {{1, 2}, {3, 4}};
            cout << "(0, 0) " << &(x[0][0]) << endl;
            cout << "(0, 1) " << (&(x[0][1]) - &(x[0][0])) << endl;
            cout << "(1, 0) " << (&(x[1][0]) - &(x[0][0])) << endl;
            cout << "(1, 1) " << (&(x[1][1]) - &(x[0][0])) << endl;
            tr.test_eq(16u, sizeof(nest));
        }
        {
            using nest = ra::Small<ra::Small<ra::Small<int, 2>, 2>, 2>;
            cout << "nestnest " << sizeof(nest) << " " << alignof(nest) << endl;
            nest x = {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
            cout << "(0, 0, 0) " << &(x[0][0]) << endl;
            cout << "(0, 0, 1) " << (&(x[0][0][1]) - &(x[0][0][0])) << endl;
            cout << "(0, 1, 0) " << (&(x[0][1][0]) - &(x[0][0][0])) << endl;
            cout << "(0, 1, 1) " << (&(x[0][1][1]) - &(x[0][0][0])) << endl;
            cout << "(1, 0, 0) " << (&(x[1][0][0]) - &(x[0][0][0])) << endl;
            cout << "(1, 0, 1) " << (&(x[1][0][1]) - &(x[0][0][0])) << endl;
            cout << "(1, 1, 0) " << (&(x[1][1][0]) - &(x[0][0][0])) << endl;
            cout << "(1, 1, 1) " << (&(x[1][1][1]) - &(x[0][0][0])) << endl;
            tr.test_eq(32u, sizeof(nest));
        }
        {
            using nest = ra::Small<int, 2, 2, 2>;
            cout << "rank3 " << sizeof(nest) << endl;
        }
    }
    tr.section("base class");
    {
        cout << sizeof(sample1) << endl;
        cout << sizeof(sample2) << endl;
        cout << sizeof(sample3) << endl;
        cout << sizeof(sample4) << endl;
        cout << sizeof(sample5) << endl;
        cout << sizeof(sample6) << endl;
    }
    return tr.summary();
}
