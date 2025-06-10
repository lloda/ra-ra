// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - View operations

// (c) Daniel Llorens - 2013-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder, ra::ilist;

template <class A>
void CheckReverse(TestRecorder & tr, A && a)
{
    std::iota(a.begin(), a.end(), 1);
    auto bd = reverse(a);
    auto b0 = reverse(a, ra::ic<0>);
    double check0[24] = { 17, 18, 19, 20,   21, 22, 23, 24,
                          9, 10, 11, 12,  13, 14, 15, 16,
                          1, 2, 3, 4,     5, 6, 7, 8 };
    tr.test(std::ranges::equal(check0, check0+24, b0.begin(), b0.end()));
    tr.test(std::ranges::equal(check0, check0+24, bd.begin(), bd.end()));
// FIXME ViewSmall doesn't support these subscripts
    if constexpr (ra::ANY==size_s(a)) {
        auto c0 = a(ra::iota(ra::len, ra::len-1, -1));
        tr.test_eq(b0, c0);
    }

    auto b1 = reverse(a, ra::ic<1>);
    double check1[24] = { 5, 6, 7, 8,      1, 2, 3, 4,
                          13, 14, 15, 16,  9, 10, 11, 12,
                          21, 22, 23, 24,  17, 18, 19, 20 };
    tr.test(std::ranges::equal(check1, check1+24, b1.begin(), b1.end()));
// FIXME ViewSmall doesn't support these subscripts
    if constexpr (ra::ANY==size_s(a)) {
        auto c1 = a(ra::dots<1>, ra::iota(ra::len, ra::len-1, -1));
        tr.test_eq(b1, c1);
    }

    auto b2 = reverse(a, ra::ic<2>);
    double check2[24] = { 4, 3, 2, 1,      8, 7, 6, 5,
                          12, 11, 10, 9,   16, 15, 14, 13,
                          20, 19, 18, 17,  24, 23, 22, 21 };
    tr.test(std::ranges::equal(check2, check2+24, b2.begin(), b2.end()));
// FIXME ViewSmall doesn't support these subscripts
    if constexpr (ra::ANY==size_s(a)) {
        auto c2 = a(ra::dots<2>, ra::iota(ra::len, ra::len-1, -1));
        tr.test_eq(b2, c2);
    }
}

template <class A>
void CheckTranspose1(TestRecorder & tr, A && a)
{
    {
        std::iota(a.begin(), a.end(), 1);
        auto b = transpose(a, ra::Small<int, 2>{1, 0});
        double check[6] = {1, 3, 5, 2, 4, 6};
        tr.test(std::ranges::equal(b.begin(), b.end(), check, check+6));
    }
    {
        std::iota(a.begin(), a.end(), 1);
        auto b = transpose(a, ilist<1, 0>);
        double check[6] = {1, 3, 5, 2, 4, 6};
        tr.test(std::ranges::equal(b.begin(), b.end(), check, check+6));
    }
    {
        std::iota(a.begin(), a.end(), 1);
        auto b = transpose(a); // meaning (... , ilist<1, 0>)
        double check[6] = {1, 3, 5, 2, 4, 6};
        tr.test(std::ranges::equal(b.begin(), b.end(), check, check+6));
    }
}

int main()
{
    TestRecorder tr(std::cout);
    tr.section("reverse array types");
    {
        CheckReverse(tr, ra::Unique<double>({ 3, 2, 4 }, ra::none));
        CheckReverse(tr, ra::Unique<double, 3>({ 3, 2, 4 }, ra::none));
        CheckReverse(tr, ra::Small<double, 3, 2, 4>(ra::none));
    }
    tr.section("transpose of 0 rank");
    {
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose(a, ra::Small<int, 0> {});
            tr.test_eq(0, b.rank());
            tr.test_eq(99, b());
            tr.test(is_c_order(a));
            tr.test(is_c_order(b));
        }
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose(a, ilist<>);
            tr.test_eq(0, b.rank());
            tr.test_eq(99, b());
        }
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose(a, {});
            tr.test_eq(0, b.rank());
            tr.test_eq(0, rank_s(b)); // even
            tr.test_eq(99, b());
        }
        {
            ra::Unique<double, 0> a({}, 99);
            auto b = transpose(a, ilist<>);
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
        tr.test(is_c_order(a));
        transpose_test(transpose(a, ra::Small<int, 2> { 0, 0 })); // dyn rank to dyn rank
        transpose_test(transpose(a, ilist<0, 0>));                // dyn rank to static rank
        ra::Unique<double, 2> b({3, 2}, ra::_0*2 + ra::_1*1 + 1);
        transpose_test(transpose(b, ra::Small<int, 2> { 0, 0 })); // static rank to dyn rank
        transpose_test(transpose(b, ilist<0, 0>));                // static rank to static rank
        auto bt = transpose(b, ilist<0, 0>);
        tr.info("dimv ", bt.dimv, " step ", bt.step(0), " len ", bt.len(0)).test(!is_c_order(bt));
        tr.info("dimv ", bt.dimv, " step ", bt.step(0), " len ", bt.len(0)).test(!is_c_order_dimv(bt.dimv));
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
        transpose_test(transpose(a, ra::Small<int, 2> { 0, 0 })); // dyn rank to dyn rank
        transpose_test(transpose(a, ilist<0, 0>));                // dyn rank to static rank
        ra::Unique<double, 2> b({2, 3}, ra::_0*3 + ra::_1 + 1);
        transpose_test(transpose(b, ra::Small<int, 2> { 0, 0 })); // static rank to dyn rank
        transpose_test(transpose(b, ilist<0, 0>));                // static rank to static rank
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
        transpose_test(transpose(a, { 0, 0 })); // dyn rank to dyn rank
    }
    tr.section("transpose E");
    {
        ra::Unique<double> a({3}, {1, 2, 3});
        tr.test_eq(a-a(ra::insert<1>), a-transpose(a, {1}));
    }
    tr.section("transpose F");
    {
        ra::Unique<double> a({3}, {1, 2, 3});
        tr.test_eq(a-a(ra::insert<1>), a-transpose(a, ilist<1>));
    }
    tr.section("transpose G");
    {
        ra::Unique<double, 1> a({3}, {1, 2, 3});
        tr.test_eq(a-a(ra::insert<1>), a-transpose(a, ilist<1>));
    }
// FIXME Small cannot have UNB sizes yet.
    // tr.section("transpose K");
    // {
    //     ra::Small<double, 3> a = {1, 2, 3};
    //     cout << transpose<1>(a) << endl;
    // }
    tr.section("transpose Q [ma117]");
    {
        ra::Small<int, 2> axes = {0, 1};
        ra::Unique<double, 2> a = {{1, 2, 3}, {4, 5, 6}};
        cout << "A: " << transpose(a, axes) << endl;
        axes = {1, 0};
        cout << "B: " << transpose(a, axes) << endl;
    }
    tr.section("free ravel?");
    {
        ra::Unique<double, 2> a = {{1, 2, 3}, {4, 5, 6}};
        tr.test(is_c_order(a, false));
        auto b = transpose(a, ilist<1, 0>);
        tr.test(!is_c_order(b, false));
    }
// trailing singleton dimensions
    {
        ra::Unique<double, 2> a = {{1, 2, 3}, {4, 5, 6}};
        ra::ViewBig<double *, 2> c{{{2, 1}, {1, 3}}, a.data()};
        tr.test(is_c_order(c, false));
        tr.test_eq(ra::Small<int, 2>{1, 2}, ravel_free(c));
    }
// singleton dimensions elsewhere
    {
        ra::Unique<double, 1> a = ra::iota(30);
        ra::ViewBig<double *, 3> c{{{2, 10}, {1, 8}, {5, 2}}, a.data()};
        tr.test(is_c_order(c, false));
        tr.test_eq(ra::iota(10, 0, 2), ravel_free(c));
    }
    return tr.summary();
}
