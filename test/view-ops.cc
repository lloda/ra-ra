// -*- mode: c++; coding: utf-8 -*-
/// @file view-ops.cc
/// @brief Checks for view operations

// (c) Daniel Llorens - 2013-2015, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include <numeric>
#include "ra/test.hh"
#include "ra/ra.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
template <int i> using TI = ra::TensorIndex<i, int>;

template <class A>
void CheckReverse(TestRecorder & tr, A && a)
{
    std::iota(a.begin(), a.end(), 1);
    cout << "a: " << a << endl;
    auto b0 = reverse(a, 0);
    cout << "b: " << b0 << endl;
    double check0[24] = { 17, 18, 19, 20,   21, 22, 23, 24,
                          9, 10, 11, 12,  13, 14, 15, 16,
                          1, 2, 3, 4,     5, 6, 7, 8 };
    tr.test(std::equal(check0, check0+24, b0.begin()));

    auto b1 = reverse(a, 1);
    cout << "b: " << b1 << endl;
    double check1[24] = { 5, 6, 7, 8,      1, 2, 3, 4,
                          13, 14, 15, 16,  9, 10, 11, 12,
                          21, 22, 23, 24,  17, 18, 19, 20 };
    tr.test(std::equal(check1, check1+24, b1.begin()));

    auto b2 = reverse(a, 2);
    cout << "b: " << b2 << endl;
    double check2[24] = { 4, 3, 2, 1,      8, 7, 6, 5,
                          12, 11, 10, 9,   16, 15, 14, 13,
                          20, 19, 18, 17,  24, 23, 22, 21 };
    tr.test(std::equal(check2, check2+24, b2.begin()));
}

template <class A>
void CheckTranspose1(TestRecorder & tr, A && a)
{
    {
        std::iota(a.begin(), a.end(), 1);
        cout << "a: " << a << endl;
        auto b = transpose(ra::Small<int, 2>{1, 0}, a);
        cout << "b: " << b << endl;
        double check[6] = {1, 3, 5, 2, 4, 6};
        tr.test(std::equal(b.begin(), b.end(), check));
    }
    {
        std::iota(a.begin(), a.end(), 1);
        cout << "a: " << a << endl;
        auto b = transpose<1, 0>(a);
        cout << "b: " << b << endl;
        double check[6] = {1, 3, 5, 2, 4, 6};
        tr.test(std::equal(b.begin(), b.end(), check));
    }
}

int main()
{
    TestRecorder tr(std::cout);
    tr.section("reverse array types");
    {
        CheckReverse(tr, ra::Unique<double>({ 3, 2, 4 }, ra::none));
        CheckReverse(tr, ra::Unique<double, 3>({ 3, 2, 4 }, ra::none));
    }
    tr.section("transpose of 0 rank");
    {
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose(ra::Small<int, 0> {}, a);
            tr.test_eq(0, b.rank());
            tr.test_eq(99, b());
        }
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose(a);
            tr.test_eq(0, b.rank());
            tr.test_eq(99, b());
        }
// FIXME this doesn't work because init_list {} competes with mp::int_list<>.
        // {
        //     ra::Unique<double, 0> a({}, 99);
        //     auto b = transpose({}, a);
        //     tr.test_eq(0, b.rank());
        //     tr.test_eq(99, b());
        // }
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose<>(a);
            tr.test_eq(0, b.rank());
            tr.test_eq(99, b());
        }
    }
    tr.section("transpose A");
    {
        CheckTranspose1(tr, ra::Unique<double>({ 3, 2 }, ra::none));
        CheckTranspose1(tr, ra::Unique<double, 2>({ 3, 2 }, ra::none));
    }
    tr.section("transpose B");
    {
        auto transpose_test = [&tr](auto && b)
            {
                tr.test_eq(1, b.rank());
                tr.test_eq(2, b.size());
                tr.test_eq(1, b[0]);
                tr.test_eq(4, b[1]);
            };
        ra::Unique<double> a({3, 2}, ra::_0*2 + ra::_1 + 1);
        cout << "A: " << a << endl;
        transpose_test(transpose(ra::Small<int, 2> { 0, 0 }, a)); // dyn rank to dyn rank
        transpose_test(transpose<0, 0>(a));                       // dyn rank to static rank
        ra::Unique<double, 2> b({3, 2}, ra::_0*2 + ra::_1*1 + 1);
        transpose_test(transpose(ra::Small<int, 2> { 0, 0 }, b)); // static rank to dyn rank
        transpose_test(transpose<0, 0>(b));                       // static rank to static rank
    }
    tr.section("transpose C");
    {
        auto transpose_test = [&tr](auto && b)
            {
                tr.test_eq(1, b.rank());
                tr.test_eq(2, b.size());
                tr.test_eq(1, b[0]);
                tr.test_eq(5, b[1]);
            };
        ra::Unique<double> a({2, 3}, ra::_0*3 + ra::_1 + 1);
        transpose_test(transpose(ra::Small<int, 2> { 0, 0 }, a)); // dyn rank to dyn rank
        transpose_test(transpose<0, 0>(a));                       // dyn rank to static rank
        ra::Unique<double, 2> b({2, 3}, ra::_0*3 + ra::_1 + 1);
        transpose_test(transpose(ra::Small<int, 2> { 0, 0 }, b)); // static rank to dyn rank
        transpose_test(transpose<0, 0>(b));                       // static rank to static rank
    }
    tr.section("transpose D");
    {
        auto transpose_test = [&tr](auto && b)
            {
                tr.test_eq(1, b.rank());
                tr.test_eq(2, b.size());
                tr.test_eq(1, b[0]);
                tr.test_eq(5, b[1]);
            };
        ra::Unique<double> a({2, 3}, ra::_0*3 + ra::_1 + 1);
        transpose_test(transpose({ 0, 0 }, a)); // dyn rank to dyn rank
    }
    tr.section("transpose E");
    {
        ra::Unique<double> a({3}, {1, 2, 3});
        tr.test_eq(a-a(ra::insert<1>), a-transpose({1}, a));
    }
    tr.section("transpose F");
    {
        ra::Unique<double> a({3}, {1, 2, 3});
        tr.test_eq(a-a(ra::insert<1>), a-transpose<1>(a));
    }
    tr.section("transpose G");
    {
        ra::Unique<double, 1> a({3}, {1, 2, 3});
        tr.test_eq(a-a(ra::insert<1>), a-transpose<1>(a));
    }
// FIXME Small cannot have DIM_BAD sizes yet.
    // tr.section("transpose K");
    // {
    //     ra::Small<double, 3> a = {1, 2, 3};
    //     cout << transpose<1>(a) << endl;
    // }
    tr.section("transpose Q [ma117]");
    {
        ra::Small<int, 2> axes = {0, 1};
        ra::Unique<double, 2> a = {{1, 2, 3}, {4, 5, 6}};
        cout << "A: " << transpose(axes, a) << endl;
        axes = {1, 0};
        cout << "B: " << transpose(axes, a) << endl;
    }
    tr.section("is_ravel_free");
    {
        ra::Unique<double, 2> a = {{1, 2, 3}, {4, 5, 6}};
        tr.test(is_ravel_free(a));
        auto b = transpose<1, 0>(a);
        tr.test(!is_ravel_free(b));
    }
// trailing singleton dimensions
    {
        ra::Unique<double, 2> a = {{1, 2, 3}, {4, 5, 6}};
        ra::View<double, 2> c{{{2, 1}, {1, 3}}, a.data()};
        tr.test(is_ravel_free(c));
        tr.test_eq(ra::Small<int, 2>{1, 2}, ravel_free(c));
    }
// singleton dimensions elsewhere
    {
        ra::Unique<double, 1> a = ra::iota(30);
        ra::View<double, 3> c{{{2, 10}, {1, 8}, {5, 2}}, a.data()};
        tr.test(is_ravel_free(c));
        tr.test_eq(ra::iota(10, 0, 2), ravel_free(c));
    }
    return tr.summary();
}
