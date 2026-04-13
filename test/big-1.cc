// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests specific to Array.

// (c) Daniel Llorens - 2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

template <class T, ra::rank_t R=ra::ANY> using BigValueInit = ra::Array<std::vector<T>, ra::BigDim1<R>>;
using int2 = ra::Small<int, 2>;

namespace ra {

bool operator==(Dim const & a, Dim const & b) { return a.len==b.len && a.step==b.step; }

} // namespace ra

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
        tr.test_eq(check, ra::iter(a));
        tr.test_eq(check, b);
        tr.test_eq(check[0], b.back());
        tr.test_eq(int2 {1, 2}, b().back()); // back on views
    }
    tr.section("behavior of resize with default Array (using vector_default_init)");
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
        tr.test_eq(ra::iter({1, 2, 3, 0, 0, 0}), a);
    }
    tr.section("resize works on first dimension");
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
    tr.section("operator=");
    {
        ra::Big<int, 2> a = {{0, 1, 2}, {3, 4, 5}};
        a = {{4, 5, 6}, {7, 8, 9}}; // unbraced
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        a = {{{4, 5, 6}, {7, 8, 9}}}; // braced :-/
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        // a = {{4, 5}, {7, 8}}; // error; see checks.cc
        // tr.test_eq(a, ra::Small<int, 2, 2> {{4, 5}, {7, 8}});
    }
    tr.section("forced Fortran array");
    {
        ra::Big<int, 2> a ({2, 3}, {0, 1, 2, 3, 4, 5}); // is Array
        ra::Big<int, 2> b ({2, 3}, {0, 1, 2, 3, 4, 5}); // is Array
        auto c = transpose(ra::ViewBig<int *, 2>({3, 2}, a.data()), {1, 0}); // is View
        ra::iter(a.dimv) = c.dimv;
        tr.strict().test_eq(a, c);
// beware that Array A; A = Array constructs A, but A = (not Array) doesn't and is like A() = (not Array).
// FIXME not happy with this quirk, but I don't have a better solution.
        a() = b;
        tr.test_eq(a.dimv, c.dimv);
        a = b;
        tr.test_eq(a.dimv, b.dimv);
    }
    tr.section("using shape as expr");
    {
        ra::Unique<double, 3> v({3, 2, 4}, ra::_0 + 15*ra::_1);
        tr.test_eq(ra::iter({3, 2}), ra::shape(v, ra::iota(2)));
        tr.test_eq(ra::iter({3, 2}), ra::shape(v, ra::iota(ra::len-1)));
        tr.test_eq(ra::iter({2, 4}), ra::shape(v, ra::iota(2, 1)));
        tr.test_eq(ra::iter({2, 4}), ra::shape(v, ra::iota(2, ra::len-2)));
        ra::Small<int, 4> i = {0, 2, 1, 0};
        tr.test_eq(ra::iter({3, 4}), ra::shape(v, i(ra::iota(ra::len-2)))); // len is i's
    }
    tr.section("casts from fixed rank View");
    {
        ra::Unique<double, 3> a({3, 2, 4}, ra::_0 + 15*ra::_1);
        ra::ViewBig<double *> b(a);
        tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data())); // FIXME? pointers are not ra::scalars.
        tr.test_eq(a.rank(), b.rank());
        tr.test_eq(a.len(0), b.len(0));
        tr.test_eq(a.len(1), b.len(1));
        tr.test_eq(a.len(2), b.len(2));
        tr.test(every(a==b));

        auto test = [&tr](ra::ViewBig<double *, 3> a, double * p){
            tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
            tr.test(p==a.data());
        };
        auto test_const = [&tr](ra::ViewBig<double const *, 3> a, double * p){
            tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
            tr.test(p==a.data());
        };
        auto test_const_ref = [&tr](ra::ViewBig<double const *, 3> const & a, double * p){
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
        ra::ViewBig<double *, 3> b(a);
        tr.test_eq(ra::scalar(a.data()), ra::scalar(b.data())); // FIXME? pointers are not ra::scalars.
        tr.test_eq(a.rank(), b.rank());
        tr.test_eq(a.len(0), b.len(0));
        tr.test_eq(a.len(1), b.len(1));
        tr.test_eq(a.len(2), b.len(2));
        tr.test(every(a==b));

        auto test = [&tr](ra::ViewBig<double *> a, double * p){
            tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
            tr.test(p==a.data());
        };
        auto test_const = [&tr](ra::ViewBig<double const *> a, double * p){
            tr.test_eq(ra::Small<int, 3> {3, 2, 4}, shape(a));
            tr.test(p==a.data());
        };
        auto test_const_ref = [&tr](ra::ViewBig<double const *> const & a, double * p){
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
    tr.section("ViewBig of Seq");
    {
        ra::ViewBig<ra::Seq<int>, 2> a({3, 2}, ra::Seq {1});
        std::println(cout, "{:c:2}\n", transpose(a));
        tr.test_eq(a, 1+ra::Small<int, 3, 2> {{0, 1}, {2, 3}, {4, 5}});
    }
    tr.section("ViewBig as iota<w>");
// in order to replace Ptr<>, we must support Len both in P and in Dimv.
    {
        constexpr ra::ViewBig<ra::Seq<ra::dim_t>, 1> i0({{ra::UNB, 1}}, ra::Seq<ra::dim_t> {0});
        constexpr ra::ViewBig<ra::Seq<ra::dim_t>, 2> i1({{ra::UNB, 0}, {ra::UNB, 1}}, ra::Seq<ra::dim_t> {0});
        tr.strict().test_eq(ra::Small<int, 3> {1, 3, 5}, i0 + ra::Small<int, 3> {1, 2, 3});
        ra::Big<ra::dim_t> p({3, 4}, i0 - i1);
        tr.strict().test_eq(ra::Big<ra::dim_t, 2> {{0, -1, -2, -3}, {1, 0, -1, -2}, {2, 1, 0, -1}}, p);
    }
    return tr.summary();
}
