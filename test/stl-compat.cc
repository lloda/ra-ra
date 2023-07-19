// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Using STL algos & types together with ra::.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// ra:: iterators are only partially STL compatible, because of copiability,
// lack of random access (which for the STL also means linear, but at least for
// 1D expressions it should be available), etc. Check some cases here.

#include <ranges>
#include <iostream>
#include <iterator>
#include <span>
#include <version>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr;
    tr.section("random access iterators");
    {
        // TODO rank-0 begin()/end() in ra::Small
        // TODO others?
    }
    tr.section("copyable iterators, but not random access");
    {
        {
            ra::Big<int, 1> a = { 1, 2, 3 };
            ra::Big<int, 1> b = { 0, 0, 0 };
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
        {
            ra::Big<int, 2> a({2, 3}, ra::_0 - 2*ra::_1);
            ra::Big<int, 2> b({2, 3}, 99);
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
        {
            ra::Small<int, 2, 3> a(ra::_0 - 2*ra::_1);
            ra::Small<int, 2, 3> b(99);
            std::transform(a.begin(), a.end(), b.begin(), [](int a) { return -a; });
            tr.test_eq(a, -b);
        }
    }
    tr.section("raw pointers");
    {
        ra::Big<int, 1> a = {1, 2, 3};
        int b[] = { +1, -1, +1 };
        tr.test_eq(ra::Small<int, 3> {2, 1, 4}, a + ra::ptr(b));
        ra::ptr(b) = ra::Small<int, 3> {7, 4, 5};
        tr.test_eq(ra::Small<int, 3> {7, 4, 5}, ra::ptr(b));

        int cp[3] = {1, 2, 3};
        // ra::Big<int, 1> c({3}, &cp[0]); // forbidden, confusing for higher rank c (pointer matches as rank 1).
        ra::Big<int, 1> c({3}, ra::ptr(cp));
        tr.test_eq(ra::Small<int, 3> {1, 2, 3}, c);
        ra::Big<int, 1> d(3, ra::ptr(cp)); // alt shape
        tr.test_eq(ra::Small<int, 3> {1, 2, 3}, d);
    }
    tr.section("raw pointers");
    {
        ra::Big<int, 1> a = {1, 2, 3};
        ra::ptr(a.data()) = map([](auto const & a) { return -a; }, ra::iota(3, 1, 9));
        tr.test_eq(ra::start({-1, -10, -19}), a);
    }
    tr.section("ptr with other iterators");
    {
        std::vector a = {1, 2, 3};
        ra::Small<int, 3> b = ra::ptr(a.begin());
        tr.test_eq(ra::Small<int, 3> {1, 2, 3}, b);
    }
    tr.section("[ra12] check that begin() and end() match for empty views");
    {
        ra::Big<int, 3> aa({0, 2, 3}, 0.);
        auto a = aa(ra::all, 1);
        tr.info("begin ", a.begin().ii.c.data(), " end ", a.end().ii.c.data()).test(a.begin()==a.end());
    }
    tr.section("foreign vectors from std::");
    {
        tr.info("adapted std::array has static size").test_eq(3, size_s(ra::start(std::array {1, 2, 0})));
        tr.info("adapted std::vector has dynamic size").test_eq(ra::DIM_ANY, size_s(ra::start(std::vector {1, 2, 0})));
    }
    tr.section("std::string");
    {
        tr.info("std::string is is_foreign_vector unless registered as is_scalar")
            .test_eq(ra::is_scalar<std::string> ? 0 : 1, ra::rank_s(std::string("hello")));
        tr.info("explicit adaption to rank 1 is possible").test_eq(5, size(ra::vector(std::string("hello"))));
        tr.info("note the difference with a char array").test_eq(6, ra::size("hello"));
    }
    tr.section("other std::ranges");
    {
        tr.test_eq(15, size(ra::start(std::ranges::iota_view(-5, 10))));
        tr.info("adapted std::ranges::iota_view has dynamic size")
            .test_eq(ra::DIM_ANY, size_s(ra::start(std::ranges::iota_view(-5, 10))));
        tr.test_eq(ra::iota(15, -5), std::ranges::iota_view(-5, 10));
    }
#if __cpp_lib_span >= 202002L
    tr.section("std::span");
    {
        std::vector a = {1, 2, 3, 4};
        auto b = std::span(a);
        tr.test_eq(1, ra::rank(b));
        tr.test_eq(ra::iota(4, 1), b);
    }
#endif
    return tr.summary();
}
