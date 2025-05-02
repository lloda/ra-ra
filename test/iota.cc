// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Arrays, iterators.

// (c) Daniel Llorens - 2013-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

int main()
{
    TestRecorder tr(std::cout);
    static_assert(ra::IteratorConcept<decltype(ra::iota(10))>, "bad type pred for iota");
    tr.section("straight cases");
    {
        ra::Big<int, 1> a = ra::iota(4, 1);
        tr.test_eq(1, a[0]);
        tr.test_eq(2, a[1]);
        tr.test_eq(3, a[2]);
        tr.test_eq(4, a[3]);
    }
    tr.section("work with operators");
    {
        tr.test(every(ra::iota(4)==ra::Big<int, 1> {0, 1, 2, 3}));
        tr.test(every(ra::iota(4, 1)==ra::Big<int, 1> {1, 2, 3, 4}));
        tr.test(every(ra::iota(4, 1, 2)==ra::Big<int, 1> {1, 3, 5, 7}));
    }
    // TODO actually whether unroll is avoided depends on ply(), have a way to require it [ra3]
    tr.section("frame-matching, forbidding unroll");
    {
        ra::Big<int, 3> b ({3, 4, 2}, ra::none);
        transpose(b, {0, 2, 1}) = ra::iota(3, 1);
        cout << b << endl;
        tr.test(every(b(0)==1));
        tr.test(every(b(1)==2));
        tr.test(every(b(2)==3));
    }
    {
        ra::Big<int, 3> b ({3, 4, 2}, ra::none);
        transpose(b, ra::int_list<0, 2, 1>{}) = ra::iota(3, 1);
        cout << b << endl;
        tr.test(every(b(0)==1));
        tr.test(every(b(1)==2));
        tr.test(every(b(2)==3));
    }
    tr.section("indefinite length I");
    {
        tr.test_eq(4, (ra::iota() + ra::iota(4)).len(0));
        ra::Big<int, 1> a = ra::iota() + ra::iota(4);
        tr.test_eq(0, a[0]);
        tr.test_eq(2, a[1]);
        tr.test_eq(4, a[2]);
        tr.test_eq(6, a[3]);
    }
    tr.section("indefinite length II");
    {
        ra::Big<int, 1> b = ra::Small<int, 4> { 3, 5, 0, -1 } + ra::iota();
        tr.test_eq(3, b[0]);
        tr.test_eq(6, b[1]);
        tr.test_eq(2, b[2]);
        tr.test_eq(2, b[3]);
        tr.test_eq(99, ra::iota().at(std::array {99}));
    }
    tr.section("deduced types");
    {
        tr.test(std::is_same_v<double, decltype(ra::iota(5, double(4)).i)>);
    }
    tr.section("ra::iota with static members");
    {
        tr.test_eq(sizeof(ra::iota().i), sizeof(ra::dim_t));
        tr.test_eq(sizeof(ra::iota(4, 0, 2).i), sizeof(0));
        tr.test_eq(sizeof(ra::iota().i), sizeof(ra::iota()));
        tr.test_eq(sizeof(ra::iota().i), sizeof(ra::iota(ra::dim_c<4> {})));
// sizeof might still be > sizeof(i) + sizeof(n) because of alignment
        tr.test_eq(1, decltype(ra::iota(4).s)::value);
        tr.test_eq(sizeof(ra::iota(4, 0, 2)), sizeof(ra::iota(4, 0, 2).i) + sizeof(ra::iota(4, 0, 2).n) + sizeof(ra::iota(4, 0, 2).s));
    }
    tr.section("iota simulation with ptr(iota_view)");
    {
        tr.strictshape().test_eq(ra::iota(10, 0, 3), ra::ptr(std::ranges::iota_view(0, 10))*3);
    }
    tr.section("reverse(iota)");
    {
        auto i = ra::iota(ra::dim_c<5> {}, 3, ra::dim_c<-2> {});
        auto ri = reverse(i);
        static_assert(5==size(ri));
        static_assert(2==ri.s);
        tr.strictshape().test_eq(reverse(ra::Big<int, 1>(i)), reverse(i));
    }
    {
        auto i = ra::iota(ra::dim_c<5> {}, 3, -2);
        tr.strictshape().test_eq(reverse(ra::Big<int, 1>(i)), reverse(i));
    }
    {
        auto i = ra::iota(5, 3, -2);
        tr.strictshape().test_eq(reverse(ra::Big<int, 1>(i)), reverse(i));
    }
    return tr.summary();
}
