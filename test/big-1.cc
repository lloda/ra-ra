// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests specific to Container.

// (c) Daniel Llorens - 2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

template <class T, ra::rank_t RANK=ra::ANY> using BigValueInit = ra::Container<std::vector<T>, RANK>;
using int2 = ra::Small<int, 2>;

int main()
{
    TestRecorder tr;
    tr.section("push_back");
    {
        std::vector<int2> a;
        a.push_back({1, 2});
        ra::Big<int2, 1> b;
        b.push_back({1, 2});

        int2 check[1] = {{1, 2}};
        tr.test_eq(check, ra::start(a));
        tr.test_eq(check, b);
        tr.test_eq(check[0], b.back());
    }
    tr.section(".back() is last element not last item");
    {
        ra::Big<int2, 0> b({}, ra::scalar(int2 {1, 3})); // cf [ma116]
        tr.test_eq(int2 {1, 3}, b.back());
    }
    tr.section("behavior of resize with default Container");
    {
        {
            ra::Big<int, 1> a = {1, 2, 3, 4, 5, 6};
            a.resize(3);
            a.resize(6);
            tr.test_eq(ra::iota(6, 1), a);
        }
        {
            BigValueInit<int, 1> a = {1, 2, 3, 4, 5, 6};
            a.resize(3);
            a.resize(6);
            tr.test_eq(ra::start({1, 2, 3, 0, 0, 0}), a);
        }
    }
    tr.section("resize works on first dimension");
    {
        {
            ra::Big<int, 2> a({3, 2}, {1, 2, 3, 4, 5, 6});
            a.resize(2);
            tr.test_eq(1+ra::_1 + 2*ra::_0, a);
        }
        {
            ra::Big<int, 2> a({3, 2}, {1, 2, 3, 4, 5, 6});
            resize(a, 2);
            tr.test_eq(1+ra::_1 + 2*ra::_0, a);
        }
    }
    tr.section("operator=");
    {
        ra::Big<int, 2> a = {{0, 1, 2}, {3, 4, 5}};
        a = {{4, 5, 6}, {7, 8, 9}}; // unbraced
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        a = {{{4, 5, 6}, {7, 8, 9}}}; // braced :-/
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        // a = {{4, 5}, {7, 8}}; // operator= works as view (so this fails), but cannot verify [ra42].
        // tr.test_eq(a, ra::Small<int, 2, 2> {{4, 5}, {7, 8}});
    }
    tr.section("behavior of forced Fortran array");
    {
        ra::Big<int, 2> a ({2, 3}, {0, 1, 2, 3, 4, 5});
        ra::Big<int, 2> b ({2, 3}, {0, 1, 2, 3, 4, 5});
        auto c = transpose({1, 0}, ra::ViewBig<int, 2>({3, 2}, a.data()));
        a.dimv = c.dimv;
        for (int k=0; k!=c.rank(); ++k) {
            std::cout << "CSTRIDE " << k << " " << c.step(k) << std::endl;
            std::cout << "CLEN " << k << " " << c.len(k) << std::endl;
        }
        cout << endl;
        for (int k=0; k!=a.rank(); ++k) {
            std::cout << "ASTRIDE " << k << " " << a.step(k) << std::endl;
            std::cout << "ALEN " << k << " " << a.len(k) << std::endl;
        }
        cout << endl;
        c = b;
// FIXME this clobbers the steps of a, which is surprising -> Container should behave as View. Or, what happens to a shouldn't depend on the container vs view-ness of b.
        a = b;
        for (int k=0; k!=c.rank(); ++k) {
            std::cout << "CSTRIDE " << k << " " << c.step(k) << std::endl;
            std::cout << "CLEN " << k << " " << c.len(k) << std::endl;
        }
        cout << endl;
        for (int k=0; k!=a.rank(); ++k) {
            std::cout << "ASTRIDE " << k << " " << a.step(k) << std::endl;
            std::cout << "ALEN " << k << " " << a.len(k) << std::endl;
        }
        cout << endl;
        std::cout << "a: " << a << std::endl;
        std::cout << "b: " << b << std::endl;
        std::cout << "c: " << c << std::endl;
    }
    tr.section("casts from fixed rank View");
    {
        ra::Unique<double, 3> a({3, 2, 4}, ra::_0 + 15*ra::_1);
        ra::ViewBig<double> b(a);
        tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data())); // FIXME? pointers are not ra::scalars.
        tr.test_eq(a.rank(), b.rank());
        tr.test_eq(a.len(0), b.len(0));
        tr.test_eq(a.len(1), b.len(1));
        tr.test_eq(a.len(2), b.len(2));
        tr.test(every(a==b));

        auto test = [&tr](ra::ViewBig<double, 3> a, double * p)
                    {
                        tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
                        tr.test(p==a.data());
                    };
        auto test_const = [&tr](ra::ViewBig<double const, 3> a, double * p)
                    {
                        tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
                        tr.test(p==a.data());
                    };
        auto test_const_ref = [&tr](ra::ViewBig<double const, 3> const & a, double * p)
                    {
                        tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
                        tr.test(p==a.data());
                    };
        test(b, b.data()); // var rank to fixed rank
        test_const(b, b.data()); // non-const to const, var rank to fixed rank
        test_const_ref(a(), a.data()); // non-const to const, keeping fixed rank
    }
    tr.section("casts from var rank View");
    {
        ra::Unique<double> a({3, 2, 4}, ra::none);
        ra::ViewBig<double, 3> b(a);
        tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data())); // FIXME? pointers are not ra::scalars.
        tr.test_eq(a.rank(), b.rank());
        tr.test_eq(a.len(0), b.len(0));
        tr.test_eq(a.len(1), b.len(1));
        tr.test_eq(a.len(2), b.len(2));
        tr.test(every(a==b));

        auto test = [&tr](ra::ViewBig<double> a, double * p)
                    {
                        tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
                        tr.test(p==a.data());
                    };
        auto test_const = [&tr](ra::ViewBig<double const> a, double * p)
                    {
                        tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
                        tr.test(p==a.data());
                    };
        auto test_const_ref = [&tr](ra::ViewBig<double const> const & a, double * p)
                    {
                        tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
                        tr.test(p==a.data());
                    };
        test(b, b.data()); // fixed rank to var rank
        test_const(b, b.data()); // non-const to const, fixed rank to var rank
        test_const_ref(a, a.data()); // non-const to const, keeping var rank
    }
    tr.section("multidimensional []");
    {
        ra::Unique<int> a({3, 2, 4}, ra::_0 + ra::_1 - ra::_2);
        tr.test_eq(a(ra::all, 0), a[ra::all, 0]);
    }
    return tr.summary();
}
