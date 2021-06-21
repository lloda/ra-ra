// -*- mode: c++; coding: utf-8 -*-
/// @file concrete.cc
/// @brief Tests for concrete_type.

// (c) Daniel Llorens - 2017
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <memory>
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl;

struct ZZ { int x = 0; };

int main()
{
    ra::TestRecorder tr(std::cout);

    tr.section("scalars");
    {
        int a = 3;
        int b = 4;
        using K = ra::concrete_type<decltype(a+b)>;
        cout << mp::type_name<K>() << endl;
        tr.info("scalars are their own concrete_types").test(std::is_same_v<K, int>);
        auto c = ra::concrete(a+b);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a+b, c);
        auto d = ra::concrete(a);
        d = 99;
        tr.info("concrete() makes copies (", d, ")").test_eq(a, 3);
        tr.test_eq(ra::Small<int, 0> {}, ra::shape(d));
    }

    tr.section("unregistered scalar types");
    {
        tr.test(std::is_same_v<ra::concrete_type<ZZ>, ZZ>);
    }
    tr.section("fixed size, rank==1");
    {
        ra::Small<int, 3> a = {1, 2, 3};
        ra::Small<int, 3> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same_v<K, ra::Small<int, 3>>);
        auto c = concrete(a+b);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a+b, c);
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(a+b));
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(c));
    }
    tr.section("fixed size, rank>1");
    {
        ra::Small<int, 2, 3> a = {{1, 2, 3}, {4, 5, 6}};
        ra::Small<int, 2, 3> b = {{10, 20, 30}, {40, 50, 60}};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same_v<K, ra::Small<int, 2, 3>>);
        auto c = concrete(a+b);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a+b, c);
        tr.test_eq(ra::Small<int, 2> {2, 3}, ra::shape(a+b));
        tr.test_eq(ra::Small<int, 2> {2, 3}, ra::shape(c));
    }
    tr.section("var size I");
    {
        ra::Big<int> a = {1, 2, 3};
        tr.info(a.len(0)).test_eq(ra::Small<int, 1> {3}, ra::shape(a));
        tr.info(a.len(0)).test_eq(ra::Small<int, 1> {3}, ra::shape(ra::Big<int> {1, 2, 3}));
    }
    tr.section("var size II");
    {
        ra::Big<int, 1> a = {1, 2, 3};
        ra::Big<int, 1> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same_v<K, ra::Big<int, 1>>);
        auto c = concrete(a+b);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a+b, c);
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(a+b));
        cout << ra::start(c) << endl;
        tr.info(c.len(0)).test_eq(ra::Small<int, 1> {3}, ra::shape(c));
    }
    tr.section("var size + fixed size");
    {
        ra::Small<int, 3, 2> a = {1, 2, 3, 4, 5, 6};
        ra::Big<int, 1> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
        tr.test(std::is_same_v<K, ra::Small<int, 3, 2>>);
        auto c = concrete(a+b);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a+b, c);
        tr.test_eq(ra::Small<int, 2> {3, 2}, ra::shape(a+b));
        tr.test_eq(ra::Small<int, 2> {3, 2}, ra::shape(c));
    }
    tr.section("var size + var rank");
    {
        ra::Big<int, 1> a = {1, 2, 3};
        ra::Big<int> b = {4, 5, 6};
        using K = ra::concrete_type<decltype(a+b)>;
// ra:: b could be higher rank and that decides the type.
        tr.test(std::is_same_v<K, ra::Big<int>>);
        auto c = concrete(a+b);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a+b, c);
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(a+b));
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(c));
    }
    tr.section("concrete on is_slice fixed size");
    {
        ra::Small<int, 3> a = {1, 2, 3};
        auto c = concrete(a);
        using K = decltype(c);
        tr.test(std::is_same_v<K, ra::Small<int, 3>>);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a, c);
        a = 99;
        tr.test_eq(99, a);
        tr.info("concrete() makes copies").test_eq(K {1, 2, 3}, c);
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(a));
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(c));
    }
    tr.section("concrete on is_slice var size");
    {
        ra::Big<int, 1> a = {1, 2, 3};
        auto c = concrete(a);
        using K = decltype(c);
        tr.test(std::is_same_v<K, ra::Big<int, 1>>);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a, c);
        a = 99;
        tr.test_eq(99, a);
        tr.info("concrete() makes copies").test_eq(K {1, 2, 3}, c);
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(a));
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(c));
    }
    tr.section("concrete on foreign vector");
    {
        std::vector<int> a = {1, 2, 3};
        auto c = ra::concrete(a);
        using K = decltype(c);
        tr.test(std::is_same_v<K, ra::Big<int, 1>>);
        tr.test(std::is_same_v<decltype(c), K>);
        tr.test_eq(a, c);
        ra::start(a) = 99;
        tr.test_eq(99, ra::start(a));
        tr.info("concrete() makes copies").test_eq(K {1, 2, 3}, c);
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(a));
        tr.test_eq(ra::Small<int, 1> {3}, ra::shape(c));
    }
    tr.section("concrete on scalar");
    {
        int a = 9;
        auto b = ra::with_same_shape(a, 8);
        tr.test_eq(8, b);
        tr.test_eq(9, a);
        b = 7;
        tr.test_eq(7, b);
        tr.test_eq(9, a);
    }
    tr.section("concrete on nested array");
    {
        ra::Big<ra::Small<int, 3>, 1> x({2}, 1);
        cout << x << endl;
        cout << concrete(x) << endl;
        cout << x*double(2.) << endl;
        // cout << concrete(x*double(2.)) << endl; // FIXME fails [ra41]
        tr.test_eq(ra::Small<int, 1> {2}, ra::shape(x));
    }
    return tr.summary();
}
