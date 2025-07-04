// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests specific to Container, constructors.

// (c) Daniel Llorens - 2017-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

template <class T> constexpr bool ctest1 = requires { ra::Big<T, 2> ({2, 3, 1}, 99); }; // bad shape for rank
template <class T> constexpr bool ctest2 = requires { ra::Big<T, 2> ({2, 3, 1}, {1, 2, 3, 4, 5, 6}); }; // bad shape for rank
template <class T> constexpr bool ctest3 = requires { ra::Big<T, 2> ({2, 3, 1}, ra::none); }; // bad shape for rank
template <class T> constexpr bool ctest4 = requires { ra::Big<T, 0> ({3, 4}, 3.); }; // bad shape for rank

int main(int argc, char * * argv)
{
    TestRecorder tr;
    tr.section("predicates");
    {
        ra::ViewBig<int *, 2> a;
        static_assert(ra::rank_s<decltype(a().iter<0>())>()==ra::rank_s<decltype(a().iter())>());
    }
    tr.section("constructors");
    {
        tr.section("null View constructor");
        {
            ra::ViewBig<int *, 1> a;
            tr.test(nullptr==a.data());
        }
        tr.section("regression with some shape arguments (fixed rank)");
        {
            ra::Big<int, 1> sizes = {5};
            ra::Big<double, 1> a(sizes, ra::none);
            a = 33.;
            tr.test_eq(5, a.size());
            tr.test_eq(33, a);
        }
        {
            ra::Big<int> sizes = {5};
            ra::Big<double, 1> a(sizes, ra::none);
            a = 33.;
            tr.test_eq(5, a.size());
            tr.test_eq(33, a);
        }
        tr.section("regression with implicitly declared View constructors [ra38]. Reduced from examples/maxwell.cc");
        {
            ra::Big<int, 1> A = {1, 2};
            ra::Big<int, 1> X = {0, 0};
            X(ra::all) = A();
            tr.test_eq(ra::start({1, 2}), X);
        }
        tr.section("need for non-default View::operator=(View const & x) [ra34]");
        {
            ra::Big<int, 1> a {0, 1, 2, 3, 4, 5};
            ra::ViewBig<int *, 1> const va = a();
            ra::Big<int, 1> x(va); // replacing default operator= by copy-to-view.
            tr.test_eq(ra::iota(6), x);
        }
        tr.section("init list constructors handle implicit conversions");
        {
            ra::Big<int, 2> a({int(3), ra::dim_t(2)}, {0, 1, 2, 3, 4, 5});
            tr.test_eq(ra::_0 * 2 + ra::_1, a);
            tr.test_eq(3, a.len(0));
            tr.test_eq(2, a.len(1));
        }
    }
    tr.section("should-fail constructors");
    {
        static_assert(!ctest1<int>);
        static_assert(!ctest2<int>);
        static_assert(!ctest3<int>);
        static_assert(!ctest4<int>);

        // FIXME these errors depend on  static_assert so cannot be checked with requires.
        // ra::Big<T, 2> {3, 4}; // Invalid shape for rank
        // ra::Big<int, 2> (2, ra::none); // shape arg must have rank 1 for array rank>1
        // ra::Big<T, 2> {1, 2, 3, 4, 5, 6}; // bad deduced shape from content arg
        // ra::Big<T, 2> (ra::Small<int, 3>{2, 3, 4}, 99.); // bad shape for rank
    }
    tr.section("any rank 1 expression for the shape argument");
    {
        ra::Big<int, 2> a (2+ra::iota(2), {0, 1, 2, 3, 4, 5});
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
    }
    tr.section("even non-drivable expressions if the rank is fixed");
    {
        ra::Big<int, 2> a(ra::_0 + 2, {0, 1, 2, 3, 4, 5});
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
    }
    tr.section("also on raw views");
    {
        int ap[6] = {0, 1, 2, 3, 4, 5};
        ra::ViewBig<int *, 2> a(2+ra::iota(2), ap);
        tr.test_eq(2, a.len(0));
        tr.test_eq(3, a.len(1));
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
        tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
    }
    tr.section("also on raw views with var rank");
    {
        int ap[6] = {0, 1, 2, 3, 4, 5};
        ra::ViewBig<int *> a(2+ra::iota(2), ap);
        tr.test_eq(2, a.len(0));
        tr.test_eq(3, a.len(1));
        tr.test_eq(ra::Small<int, 2, 3> {{0, 1, 2}, {3, 4, 5}}, a);
        tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
    }
    tr.section("nested braces operator=");
    {
        ra::Big<int, 2> a({2, 3}, {0, 1, 2, 3, 4, 5});
        auto ap = a.data();
        {
// this uses operator=(nested_braces_r)
            a() = {{4, 5, 6}, {7, 8, 9}};
            tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
            tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        }
        {
// uses operator=(nested_braces_r)
            a = {{5, 6, 7}, {8, 9, 10}};
            tr.test_eq(ra::scalar(ap), ra::scalar(a.data()));
            tr.test_eq(ra::iota(6, 5), ra::ptr(a.data()));
// uses nested_braces_r constructor, so a's storage is NOT preserved. Don't rely on this either way
            a = {{{4, 5, 6}, {7, 8, 9}}};
            tr.skip().test_eq(ra::scalar(ap), ra::scalar(a.data()));
            tr.test_eq(2, a.len(0));
            tr.test_eq(3, a.len(1));
            tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
        }
    }
    tr.section("nested braces constructor");
    {
        ra::Big<int, 2> a = {{4, 5, 6}, {7, 8, 9}};
        tr.test_eq(2, a.len(0));
        tr.test_eq(3, a.len(1));
        tr.test_eq(ra::iota(6, 4), ra::ptr(a.data()));
    }
    tr.section("nested braces for nested type I");
    {
        using int2 = ra::Small<int, 2>;
// FIXME removed (shape, nested) constructors so this wouldn't be ambiguous (bc 1 converts to int2). But maybe int shouldn't convert to int2 [ra16]
        ra::Big<int2, 2> a({2, 2}, { {1, 2}, {2, 3}, {4, 5}, {6, 7} });
        ra::Big<int2, 2> b({{{1, 2},  {2, 3}}, {{4, 5},  {6, 7}}});
        ra::Big<int2, 2> c {{{1, 2},  {2, 3}}, {{4, 5},  {6, 7}}};
        ra::Big<int2, 2> d = {{{1, 2},  {2, 3}}, {{4, 5},  {6, 7}}};
        tr.test_eq(a, b);
        tr.test_eq(a, c);
        tr.test_eq(a, d);
    }
    tr.section("nested braces for nested type II");
    {
        int x[2][3] = {{1, 2, 3}, {4, 5, 6}};
        int y[2][3] = {{10, 20, 30}, {40, 50, 60}};
        ra::Big<ra::Small<int, 2, 3>, 1> a = {x, y};
        tr.test_eq(ra::_0*3+ra::_1 + 1, a(0));
        tr.test_eq(10*(ra::_0*3+ra::_1 + 1), a(1));
    }
    tr.section("nested braces for nested type II");
    {
        int x[2][3] = {{1, 2, 3}, {4, 5, 6}};
        int y[2][3] = {{10, 20, 30}, {40, 50, 60}};
        ra::Big<ra::Small<int, 2, 3>, 1> a = {x, y};
        tr.test_eq(ra::_0*3+ra::_1 + 1, a(0));
        tr.test_eq(10*(ra::_0*3+ra::_1 + 1), a(1));
    }
    tr.section("nested braces for nested type III");
    {
        int x[4] = {1, 2, 3, 4};
        int y[6] = {10, 20, 30, 40, 50, 60};
        ra::Big<ra::Big<int, 1>, 1> a = {x, y};
        tr.test_eq(ra::iota(4, 1), a(0));
        tr.test_eq(ra::iota(6, 10, 10), a(1));
    }
    tr.section("nested braces for nested type IV");
    {
        ra::Big<ra::Big<int, 1>, 1> a = {{1, 2, 3, 4}, {10, 20, 30, 40, 50, 60}};
        tr.test_eq(ra::iota(4, 1), a(0));
        tr.test_eq(ra::iota(6, 10, 10), a(1));
    }
    tr.section("nested braces for nested type V [ra45]");
    {
        int u[3] = { 1, 2, 3 };
        ra::Big<int, 1> v = u;

        ra::Small<ra::Big<int, 1>, 1> b = { {u} }; // ok; broken with { u }
        tr.test_eq(ra::iota(3, 1), b(0));

        ra::Small<ra::Big<int, 1>, 1> c = { v }; // ok
        tr.test_eq(ra::iota(3, 1), c(0));

        ra::Small<ra::Big<int, 1>, 1> d = { {1, 2, 3} }; // ok
        tr.test_eq(ra::iota(3, 1), d(0));

        auto x = ra::iota(3, 1);
        ra::Small<ra::Big<int, 1>, 1> f = { {x} }; // ok; broken with { x }
        tr.test_eq(ra::iota(3, 1), f(0));

        // ra::Small<int, 3> w = { 1, 2, 3 };
        // ra::Small<ra::Big<int, 1>, 1> e = { w }; // broken with { w }, ct error with { {w} }
        // tr.test_eq(ra::iota(3, 1), e(0));
    }
    tr.section("nested braces for nested type VI");
    {
        ra::Small<ra::Big<double, 1>, 2> g = { { 1 }, { 1, 2 } };
        tr.test_eq(ra::start({1}), g[0]);
        tr.test_eq(ra::start({1, 2}), g[1]);
    }
    tr.section("more nested braces");
    {
        tr.section("with dynamic rank");
        ra::Small<float, 2, 4> ref = { {1, 2, 3, 4}, {5, 6, 7, 8} };
        ra::Big<float> A = { {1, 2, 3, 4}, {5, 6, 7, 8} };
        ra::Big<float> B({2, 4}, {1, 2, 3, 4, 5, 6, 7, 8});
        // ra::Big<float> C({2, 4}, { {1, 2, 3, 4}, {5, 6, 7, 8} }); // not allowed bc ambiguity with empty braces
        ra::Big<float, 2> A2 = { {1, 2, 3, 4}, {5, 6, 7, 8} };
        ra::Big<float, 2> B2({2, 4}, {1, 2, 3, 4, 5, 6, 7, 8});
        // ra::Big<float, 2> C2({2, 4}, { {1, 2, 3, 4}, {5, 6, 7, 8} }); // not allowed to avoid ambiguity in [ra16] :-/
        tr.test_eq(ref, A);
        tr.test_eq(ref, B);
        tr.test_eq(ref, A2);
        tr.test_eq(ref, B2);
    }
// FIXME This works for Small, that has multi-arg constructors. Right now this calls the 2-elem constructor shape, content instead of the braces constructor, since initializer_list<T> doesn't match.
    // tr.section("item constructor");
    // {
    //     ra::Big<int, 2> a {{1, 2}, ra::iota(2, 33)};
    //     tr.test_eq(1, a(0, 0));
    //     tr.test_eq(2, a(0, 1));
    //     tr.test_eq(33, a(1, 0));
    //     tr.test_eq(34, a(1, 1));
    // }
    tr.section("at() takes foreign vector");
    {
        ra::Big<double, 2> a({3, 3}, ra::_0 + 10*ra::_1);
        std::array<int, 2> b = {2, 2};
        tr.test_eq(22, a.at(b));
    }
    tr.section("default constructor of var rank");
    {
        ra::Big<int> a {};
        ra::Big<int, 1> b {};
        tr.test_eq(b.rank(), a.rank());
        tr.test_eq(b.len(0), a.len(0));
        tr.test_eq(1, a.rank());
        tr.test_eq(0, a.len(0));
    }
    tr.section("allow scalar shape for rank 1");
    {
        ra::Big<int> a(3, ra::_0);
        tr.test_eq(1, rank(a));
        tr.test_eq(ra::iota(3), a);
        ra::Big<int, 1> b(4, ra::_0);
        tr.test_eq(ra::iota(4), b);
    }
    tr.section("default constructor for ct rank 0 Container");
    {
        ra::Big<int, 0> a = {}; // uninitialized
        a = 3;
        tr.test_eq(3, a);
    }
    tr.section("scalar constructor for rt rank 0 Container");
    {
        ra::Big<int> a(4);
        tr.test_eq(4, a);
    }
    tr.section("index with rank 0 exprs");
    {
        ra::Big<int, 0> a = 1;
        ra::Big<int, 1> b = { 1, 2, 3 };
        tr.test(std::is_same_v<int &, decltype(b(a))>);
        tr.test_eq(2, b(a));
    }
    tr.section("scalar conversion");
    {
        ra::Big<int, 0> xa({}, 1);
        ra::Big<int, 1> xb({1}, 2);
        ra::Big<int, 2> xc({1, 1}, 3);
        int a = xa;
        int b = xb;
        int c = xc;
        tr.test_eq(1, a);
        tr.test_eq(2, b);
        tr.test_eq(3, c);
    }
    return tr.summary();
}
