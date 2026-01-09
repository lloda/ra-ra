// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Using STL algos & types together with ra::.

// (c) Daniel Llorens - 2014
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <map>
#include <span>
#include <version>
#include <iostream>
#include <iterator>
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
        tr.test_eq(ra::iter({-1, -10, -19}), a);
    }
    tr.section("ptr with other iterators");
    {
        std::vector a = {1, 2, 3};
        ra::Small<int, 3> b = ra::ptr(a.begin());
        tr.test_eq(ra::Small<int, 3> {1, 2, 3}, b);
    }
    tr.section("ptr with step");
    {
        std::vector a = {1, 2, 3, 4, 5, 6};
        tr.test_eq(std::vector {1, 3, 5}, ra::ptr(a.begin(), 3, 2));
        tr.test_eq(5, ra::ptr(a.begin(), 3, 2).at(std::array { 2 }));
    }
    tr.section("ptr with step");
    {
        char const * s = "hello";
        auto p = ra::ptr(s, std::integral_constant<int, 2> {});
        static_assert(2==ra::size(p)); // ok
        tr.test_eq(ra::iter({'h', 'e'}), p);
    }
    tr.section("check that begin() and end() match for empty views");
    {
        ra::Big<int, 3> aa({0, 2, 3}, 0.);
        auto a = aa(ra::all, 1);
        tr.test(aa.empty());
        tr.test(a.begin()==a.end());
    }
    tr.section("foreign vectors from std::");
    {
        tr.info("adapted std::array has static size").test_eq(3, size_s(ra::iter(std::array {1, 2, 0})));
        tr.info("adapted std::vector has dynamic size").test_eq(ra::ANY, ra::size_s<decltype(ra::iter(std::vector {1, 2, 0}))>());
    }
    tr.section("std::string");
    {
        tr.info("std::string is is_foreign_vector unless registered as is_scalar")
            .test_eq(ra::is_scalar<std::string> ? 0 : 1, ra::rank_s<decltype(std::string("hello"))>());
        tr.info("explicit adaption to rank 1 is possible").test_eq(5, size(ra::ptr(std::string("hello"))));
        tr.info("note the difference with a char array").test_eq(6, ra::size("hello"));
    }
    tr.section("other std::ranges");
    {
        tr.test_eq(15, size(ra::iter(std::ranges::iota_view(-5, 10))));
        tr.info("adapted std::ranges::iota_view has dynamic size")
            .test_eq(ra::ANY, size_s(ra::iter(std::ranges::iota_view(-5, 10))));
        tr.test_eq(ra::iota(15, -5), std::ranges::iota_view(-5, 10));
    }
    tr.section("STL predicates");
    {
        ra::ViewBig<int *, 2> a;
        tr.test(std::input_iterator<decltype(a.begin())>);
        // tr.test(std::weak_output_iterator<decltype(a.begin()), int>); // p2550 when ready c++
        tr.test(std::input_iterator<decltype(begin(a+1))>);
        tr.test(std::sentinel_for<decltype(end(a+1)), decltype(begin(a+1))>);
        ra::Big<int, 2> b;
        tr.test(std::random_access_iterator<decltype(ra::begin(b))>);
        ra::Small<int, 2, 3> c;
        tr.test(std::random_access_iterator<decltype(ra::begin(c))>);
    }
    tr.section("STLIterator works with arbitrary expr not just views");
    {
        ra::Big<int, 3> a({4, 2, 3}, ra::_0 - ra::_1 + ra::_2);
        ra::Big<int, 1> b(4*2*3, 0);
        std::ranges::copy(std::ranges::subrange(ra::STLIterator(a+1), std::default_sentinel), begin(b));
        tr.test_eq(ra::ravel_free(a) + 1, b);
        b =  0;
// FIXME broken bc of hairy ADL issues (https://stackoverflow.com/a/33576098). Use ra::range instead.
        // static_assert(std::ranges::input_range<decltype(a+1)>);
        std::ranges::copy(range(a+1), begin(b));
        tr.test_eq(ra::ravel_free(a) + 1, b);
    }
    tr.section("STLIterator as output");
    {
        using complex = std::complex<double>;
        ra::Big<complex, 3> a({4, 2, 3}, ra::_0 - ra::_1 + ra::_2);
        ra::Big<double, 1> b(4*2*3, real_part(ra::ravel_free(a)));
        std::ranges::copy(std::ranges::subrange(b), ra::STLIterator(imag_part(a)));
        tr.test_eq((ra::_0 - ra::_1 + ra::_2)*1.*complex(1, 1), a);
        a = 0;
        std::ranges::copy(std::ranges::subrange(b), begin(imag_part(a)));
        tr.test_eq((ra::_0 - ra::_1 + ra::_2)*1.*complex(0, 1), a);
        a = 0;
        std::ranges::copy(range(b*1.) | std::views::transform([](auto x) { return -x; }), begin(a));
        tr.test_eq((ra::_0 - ra::_1 + ra::_2)*(-1.), a);
    }
    tr.section("ra::begin / ra::end use members if they exist");
    {
        ra::Big<char, 1> A = {'x', 'z', 'y'};
        ra::Big<char, 1> const & B = A;
        tr.test_seq(ra::begin(A), ra::begin(B));
        std::sort(ra::begin(A), ra::end(A));
        tr.test_eq(ra::iter({'x', 'y', 'z'}), A);
    }
    {
        ra::Big<char, 1> A = {'x', 'z', 'y'};
        std::ranges::sort(ra::range(A));
        tr.test_eq(ra::iter({'x', 'y', 'z'}), A);
    }
    tr.section("std::span");
    {
        std::vector a = {1, 2, 3, 4};
        auto b = std::span(a);
        tr.test_eq(1, ra::rank(b));
        tr.test_eq(ra::iota(4, 1), b);
    }
    tr.section("ptr(bidirectional iterator)");
    {
        std::map<int, float> m;
        for (int i=0; i<9; ++i) { m[i] = -i; }
        tr.test_eq(9, ra::size(ra::ptr(m)));
        for_each([&](auto const & m) { tr.test_eq(-m.first, m.second); }, ra::ptr(m));
    }
    return tr.summary();
}
