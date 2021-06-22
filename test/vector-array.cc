// -*- mode: c++; coding: utf-8 -*-
/// @file vector-array.cc
/// @brief Comparison of ra::vector(std::vector) vs ra::vector(std::array)

// (c) Daniel Llorens - 2013-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// This checks an alternate version of ra::Vector that doesn't work (but it should?). Cf [ra35] in ra-9.cc.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

namespace ra {

template <class V>
requires ((requires (V v) { { ssize(v) } -> std::signed_integral; } ||
           requires { std::tuple_size<std::decay_t<V>>::value; } ) &&
          requires (V v) { { v.begin() } -> std::random_access_iterator; })
struct Wector
{
    V v;
    decltype(v.begin()) p__ = v.begin();
    constexpr static bool ct_size = requires { std::tuple_size<std::decay_t<V>>::value; };

    constexpr dim_t len(int k) const
    {
        RA_CHECK(k==0, " k ", k);
        if constexpr (ct_size) { return std::tuple_size_v<std::decay_t<V>>; } else { return ssize(v); };
    }
    constexpr static dim_t len_s(int k)
    {
        RA_CHECK(k==0, " k ", k);
        if constexpr (ct_size) { return std::tuple_size_v<std::decay_t<V>>; } else { return DIM_ANY; };
    }
    constexpr static rank_t rank() { return 1; }
    constexpr static rank_t rank_s() { return 1; };
};

template <class V> inline constexpr auto wector(V && v) { return Wector<V>(std::forward<V>(v)); }

} // namespace ra

int main()
{
    TestRecorder tr(std::cout);
    tr.section("reference");
    {
        std::array<int, 2> a1 = {1, 2};
        std::vector<int> a2 = {1, 2};
        auto v1 = ra::wector(a1);
        auto v2 = ra::wector(a2);

        tr.test(std::is_reference_v<decltype(v1.v)>);
        tr.test(std::is_reference_v<decltype(v2.v)>);

        cout << "&(v1.v[0])   " << &(v1.v[0]) << endl;
        cout << "&(v1.p__[0]) " << &(v1.p__[0]) << endl;
        cout << "&v1          " << &v1 << endl;
        tr.test_eq(ra::scalar(&(v1.v[0])), ra::scalar(&(v1.p__[0])));
        tr.test_eq(ra::scalar(&(v1.v[0])), ra::scalar(&(a1[0])));

        cout << "&(v2.v[0])   " << &(v2.v[0]) << endl;
        cout << "&(v2.p__[0]) " << &(v2.p__[0]) << endl;
        cout << "&v2          " << &v2 << endl;
        tr.test_eq(ra::scalar(&(v2.v[0])), ra::scalar(&(v2.p__[0])));
        tr.test_eq(ra::scalar(&(v2.v[0])), ra::scalar(&(a2[0])));
    }
    tr.section("value");
    {
        auto fun1 = []() { return std::array<int, 2> {7, 2}; };
        auto fun2 = []() { return std::vector<int> {5, 2}; };
        auto v1 = ra::wector(fun1());
        auto v2 = ra::wector(fun2());

        tr.test(!std::is_reference_v<decltype(v1.v)>);
        tr.test(!std::is_reference_v<decltype(v2.v)>);

        tr.test_eq(7, v1.v[0]);
        tr.test_eq(7, v1.p__[0]);
        cout << "&(v1.v[0])   " << &(v1.v[0]) << endl;
        cout << "&(v1.p__[0]) " << &(v1.p__[0]) << endl;
        cout << "&v1          " << &v1 << endl;
        tr.skip().info("FIXME").test_eq(ra::scalar(&(v1.v[0])), ra::scalar(&(v1.p__[0])));

        tr.test_eq(5, v2.v[0]);
        tr.test_eq(5, v2.p__[0]);
        cout << "&(v2.v[0])   " << &(v2.v[0]) << endl;
        cout << "&(v2.p__[0]) " << &(v2.p__[0]) << endl;
        cout << "&v2          " << &v2 << endl;
        tr.test_eq(ra::scalar(&(v2.v[0])), ra::scalar(&(v2.p__[0])));
    }
    return tr.summary();
}
