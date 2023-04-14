// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Traversal.

// (c) Daniel Llorens - 2013-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <numeric>
#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "ra/complex.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
template <int i> using TI = ra::TensorIndex<i>;
using real = double;

struct Never
{
    int a;
    Never(): a(0) {}
    void operator=(int b) { a = 99; }
    bool used() { return a==99 ? true : false; }
};

int main()
{
    TestRecorder tr;
    tr.section("traversal - xpr types - Expr");
    {
        {
            real check[6] = {0-3, 1-3, 2-3, 3-3, 4-3, 5-3};
            ra::Unique<real, 2> a({3, 2}, ra::none);
            ra::Unique<real, 2> c({3, 2}, ra::none);
            std::iota(a.begin(), a.end(), 0);
#define TEST(plier)                                                     \
            {                                                           \
                std::fill(c.begin(), c.end(), 0);                       \
                plier(ra::expr([](real & c, real a, real b) { c = a-b; }, \
                               c.iter(), a.iter(), ra::scalar(3.0)));   \
                tr.info(STRINGIZE(plier)).test(std::equal(check, check+6, c.begin()));  \
            }
            TEST(ply_ravel);
            TEST(plyf);
#undef TEST
        }
#define TEST(plier)                                                     \
        {                                                               \
            ra::Small<int, 3> A {1, 2, 3};                              \
            ra::Small<int, 3> C {0, 0, 0};                              \
            plier(ra::expr([](int a, int & c) { c = -a; }, A.iter(), C.iter())); \
            tr.test_eq(-1, C[0]);                                       \
            tr.test_eq(-2, C[1]);                                       \
            tr.test_eq(-3, C[2]);                                       \
            ra::Small<int, 3> B {+1, -2, +3};                           \
            plier(ra::expr([](int a, int b, int & c) { c = a*b; }, A.iter(), B.iter(), C.iter())); \
            tr.test_eq(+1, C[0]);                                       \
            tr.test_eq(-4, C[1]);                                       \
            tr.test_eq(+9, C[2]);                                       \
        }
        TEST(ply_ravel);
        TEST(plyf);
#undef TEST
        {
            ra::Unique<int, 3> a(std::vector<ra::dim_t> {3, 2, 4}, ra::none);
            ra::Unique<int, 3> b(std::vector<ra::dim_t> {3, 2, 4}, ra::none);
            ra::Unique<int, 3> c(std::vector<ra::dim_t> {3, 2, 4}, ra::none);
            std::iota(a.begin(), a.end(), 0);
            std::iota(b.begin(), b.end(), 0);
#define TEST(plier)                                                     \
            {                                                           \
                std::iota(c.begin(), c.end(), 99);                      \
                plier(ra::expr([](int a, int b, int & c) { c = a-b; }, a.iter(), b.iter(), c.iter())); \
                for (int ci: c) { tr.test_eq(0, ci); }                  \
            }
            TEST(ply_ravel);
            TEST(plyf);
#undef TEST
        }
    }
    tr.section("traversal - xpr types - Expr - rank 0");
    {
        {
            real check[1] = {4};
#define TEST(plier)                                                     \
            [&tr, &check](auto && a, auto && c)                         \
            {                                                           \
                std::iota(a.begin(), a.end(), 7);                       \
                std::fill(c.begin(), c.end(), 0);                       \
                plier(ra::expr([](real & c, real a, real b) { c = a-b; }, \
                               c.iter(), a.iter(), ra::scalar(3.0)));   \
                tr.test(std::equal(check, check+1, c.begin()));        \
            }
#define TEST2(plier) \
            TEST(plier)(ra::Unique<real, 0>({}, ra::none), ra::Unique<real, 0>({}, ra::none)); \
            TEST(plier)(ra::Small<real> {}, ra::Small<real> {});        \
            TEST(plier)(ra::Small<real> {}, ra::Unique<real, 0>({}, ra::none));
            TEST2(ply_ravel);
            TEST2(plyf);
#undef TEST2
#undef TEST
        }
    }
    tr.section("traversal - xpr types - Expr - empty");
    {
        {
#define TEST(plier, id)                                                 \
            [&tr](auto && a, bool used)                                 \
            {                                                           \
                tr.info(STRINGIZE(plier) "/" id)                        \
                    .test((used || (a.begin()==a.end() && a.size()==0)) && STRINGIZE(plier) id " before"); \
                Never check;                                            \
                plier(ra::expr([&check](int a) { check = a; }, a.iter())); \
                tr.info(STRINGIZE(plier) id " after")                   \
                    .test(check.used()==used);                          \
            }

#define TEST2(plier)                                                    \
            TEST(plier, "00")(ra::Small<int, 0> {}, false);             \
            TEST(plier, "01")(ra::Unique<int, 1>({ 0 }, ra::none), false); \
            TEST(plier, "02")(ra::Unique<int, 2>({ 2, 0 }, ra::none), false); \
            TEST(plier, "03")(ra::Unique<int, 2>({ 0, 2 }, ra::none), false); \
            TEST(plier, "04")(ra::Small<int> {}, true);                 \
            TEST(plier, "05")(ra::Unique<int, 0>({}, ra::none), true);

            TEST2(ply_ravel);
            TEST2(plyf);
// this one cannot be done with plyf.
            TEST(ply_ravel, "06")(ra::Unique<int>({ 0 }, ra::none), false);
#undef TEST2
#undef TEST
// With ra::expr, non-slices.
#define TEST(plier, id)                                                 \
            [&tr](auto && a, bool used)                                 \
            {                                                           \
                cout << STRINGIZE(plier) "/" id << endl;                \
                tr.test((used || (a.len(0)==0 || a.len(1)==0)) && STRINGIZE(plier) id " before"); \
                Never check;                                            \
                plier(ra::expr([&check](int a) { check = a; }, a));     \
                tr.test(check.used()==used && STRINGIZE(plier) id " after"); \
            }
#define TEST2(plier)                                                    \
            TEST(plier, "10")(ra::Unique<int, 1>({ 0 }, ra::none)+ra::Small<int, 0>(), false); \
            TEST(plier, "11")(ra::Unique<int, 2>({ 2, 0 }, ra::none)+ra::Small<int, 2, 0>(), false); \
            TEST(plier, "12")(ra::Unique<int, 2>({ 0, 2 }, ra::none)+ra::Small<int, 0, 2>(), false); \
            TEST(plier, "13")(ra::Unique<int, 1>({ 0 }, ra::none)+ra::scalar(1), false); \
            TEST(plier, "14")(ra::Unique<int, 2>({ 2, 0 }, ra::none)+ra::scalar(1), false); \
            TEST(plier, "15")(ra::Unique<int, 2>({ 0, 2 }, ra::none)+ra::scalar(1), false);

            TEST2(plyf);
            TEST2(ply_ravel);
        }
#undef TEST2
#undef TEST
    }
    tr.section("traversal - does it compile?");
    {
// TODO Check.
        auto print = [](real a) { cout << a << " "; };
        {
            auto test = [&](auto && a)
                {
                    ra::ply_ravel(ra::expr(print, a.iter())); cout << endl;
                    ra::plyf(ra::expr(print, a.iter())); cout << endl;
                };
            ra::Unique<real, 3> a(std::vector<ra::dim_t> {1, 2, 3}, ra::none);
            std::iota(a.begin(), a.end(), 0);
            test(a);
            test(a()); // also View.
        }
// TODO See Expr::CAN_DRIVE in expr.hh. Doesn't generally work with Unique<RANK_ANY> because Expr needs to pick a driving argument statically. However, it does work when there's only one argument, since ply_ravel() is rank-dynamic.
        {
            auto test = [&](auto && a)
                {
                    ra::ply_ravel(ra::expr(print, a.iter())); cout << endl;
                };
            ra::Unique<real> a(std::vector<ra::dim_t> {1, 2, 3}, ra::none);
            std::iota(a.begin(), a.end(), 0);
            test(a);
            test(a()); // also View.
        }
    }
    tr.section("[ra06] constructor cases with scalar or RANK_ANY arguments");
    {
// TODO Move these to the constructor tests, and put assignment versions here.
        tr.section("construction of 0 rank <- scalar expr");
        {
            ra::Unique<real, 0> a ({}, ra::scalar(77));
            tr.test_eq(77, a());
        }
        tr.section("construction of var rank <- scalar expr");
        {
            ra::Unique<real> a ({3, 2}, ra::scalar(77));
            tr.test_eq(77, a(0, 0));
            tr.test_eq(77, a(0, 1));
            tr.test_eq(77, a(1, 0));
            tr.test_eq(77, a(1, 1));
            tr.test_eq(77, a(2, 0));
            tr.test_eq(77, a(2, 1));
        }
        tr.section("construction of var rank <- lower rank expr I");
        {
            ra::Unique<real, 1> b ({3}, {1, 2, 3});
            ra::Unique<real> a ({3, 2}, b.iter());
            tr.test_eq(1, a(0, 0));
            tr.test_eq(1, a(0, 1));
            tr.test_eq(2, a(1, 0));
            tr.test_eq(2, a(1, 1));
            tr.test_eq(3, a(2, 0));
            tr.test_eq(3, a(2, 1));
        }
        tr.section("construction of var rank <- lower rank expr II");
        {
            ra::Unique<real> b ({3, 2}, {1, 2, 3, 4, 5, 6});
            cout << "b: " << b << endl;
            ra::Unique<real> a ({3, 2, 4}, b.iter());
            cout << "a: " << a << endl;
            for (int i=0; i<3; ++i) {
                for (int j=0; j<2; ++j) {
                    for (int k=0; k<4; ++k) {
                        tr.test_eq(a(i, j, k), b(i, j));
                    }
                }
            }
        }
        // this succeeds because of the two var ranks, the top rank comes first (and so it's selected as driver). TODO Have run time driver selection so this is safe.
        tr.section("construction of var rank <- lower rank expr III (var rank)");
        {
            ra::Unique<real> b ({3}, {1, 2, 3});
            ra::Unique<real> a ({3, 2}, b.iter());
            tr.test_eq(1, a(0, 0));
            tr.test_eq(1, a(0, 1));
            tr.test_eq(2, a(1, 0));
            tr.test_eq(2, a(1, 1));
            tr.test_eq(3, a(2, 0));
            tr.test_eq(3, a(2, 1));
        }
// driver selection is done at compile time (see Expr::DRIVER). Here it'll be the var rank expr, which results in an error at run time. TODO Do run time driver selection to avoid this error.
        // tr.section("construction of var rank <- higher rank expr");
        // {
        //     ra::Unique<real> b ({3, 2}, {1, 2, 3, 4, 5, 6});
        //     cout << "b: " << b << endl;
        //     ra::Unique<real> a ({4}, b.iter());
        //     cout << "a: " << a << endl;
        // }
    }

    tr.section("cf plying with and without driver (error)");
    {
        ra::Unique<real, 1> a({3}, ra::none);
        ply_ravel(expr([](real & a, int b) { a = b; }, a.iter(), ra::scalar(7)));
        tr.test_eq(7, a[0]);
        tr.test_eq(7, a[1]);
        tr.test_eq(7, a[2]);
        ply(expr([](real & a, int b) { a = b; }, a.iter(), TI<0>()));
        tr.test_eq(0, a[0]);
        tr.test_eq(1, a[1]);
        tr.test_eq(2, a[2]);
// TODO Check that these give ct error. Not clear that the second one should...
        // ply(expr([](int b) { cout << b << endl; }, TI<0>()));
        // ply(expr([](int b) { cout << b << endl; }, ra::scalar(3)));
    }
    tr.section("traversal - rank matching - Unique/Unique 1");
    {
        ra::Unique<real, 3> a({ 3, 2, 4 }, ra::none);
        ra::Unique<real, 2> b({ 3, 2 }, ra::none);
        ra::Unique<real, 3> c({ 3, 2, 4 }, ra::none);
        real check[24] = { 0, 1, 2, 3,     3, 4, 5, 6,      6, 7, 8, 9,
                           9, 10, 11, 12,  12, 13, 14, 15,  15, 16, 17, 18 };
        std::iota(a.begin(), a.end(), 1);
        std::iota(b.begin(), b.end(), 1);
        {
            ra::Unique<real, 3> c0(expr([](real a, real b) { return a-b; }, a.iter(), b.iter()));
            tr.test(std::equal(check, check+24, c0.begin()));
            ra::Unique<real, 3> c1(expr([](real a, real b) { return b-a; }, b.iter(), a.iter()));
            tr.test(std::equal(check, check+24, c1.begin()));
        }
        {
#define TEST(plier)                                                 \
            std::fill(c.begin(), c.end(), 0);                       \
            plier(expr([&](real & c, real a, real b) { c=a-b; }, \
                          c.iter(), a.iter(), b.iter()));           \
            tr.info(STRINGIZE(plier) " a-b").test(std::equal(check, check+24, c.begin())); \
            std::fill(c.begin(), c.end(), 0);                       \
            plier(expr([](real & c, real a, real b) { c=b-a; },  \
                          c.iter(), b.iter(), a.iter()));           \
            tr.info(STRINGIZE(plier) " b-a").test(std::equal(check, check+24, c.begin()));
            TEST(ply_ravel);
            TEST(plyf);
#undef TEST
        }
    }
    tr.section("traversal - op uses from");
    {
        ra::Unique<int, 1> a({3}, ra::none);
        ra::Unique<int, 1> b({3}, ra::none);
        std::iota(a.begin(), a.end(), 1);
#define TEST(plier)                                                     \
        tr.section(STRINGIZE(plier));                                   \
        {                                                               \
            std::fill(b.begin(), b.end(), 0);                           \
            real check[3] = { 2, 3, 1 };                                \
            plier(expr([&a](int & b, int i) { b = a(i); }, b.iter(), ra::vector(std::array {1, 2, 0}))); \
            tr.info(STRINGIZE(plier) " std::array").test(std::equal(check, check+3, b.begin())); \
            plier(expr([&a](int & b, int & i) { b = a(i); }, b.iter(), ra::vector(std::vector {1, 2, 0}))); \
            tr.info(STRINGIZE(plier) " std::vector").test(std::equal(check, check+3, b.begin())); \
        }
        TEST(ply_ravel);
        TEST(plyf);
#undef TEST
    }
    tr.section("helpers for ply - map, for_each");
    {
// TODO Test need for map() -> decltype(...) in the declaration of map.
        ra::Unique<real, 1> b = map([](auto x) { return exp(x); }, ra::Unique<int, 1>({1, 2}));
        tr.test_eq(b, ra::Unique<real, 1>({exp(1), exp(2)}));
        real x = 0.;
        for_each([&x](auto y) { x += y; }, ra::Unique<int, 1>({13, 21}));
        tr.test_eq(34, x);
    }
    tr.section("the loop cannot be unrolled entirely and one of the outside dims is zero");
    {
        real aa = 100;
        ra::View<real, 3> a { {{0, 22}, {11, 2}, {2, 1}}, &aa };
        ra::View<real, 3> b { {{0, 1}, {11, 2}, {2, 1}}, &aa };
#define TEST(plier)                                             \
        {                                                       \
            real c = 99;                                        \
            plier(ra::expr([&c](real a, real b) { c = 77; },    \
                           a.iter(), b.iter()));                \
            tr.info(STRINGIZE(plier)).test(c==99);              \
        }
        TEST(ply_ravel);
        TEST(plyf);
#undef TEST
    }
    tr.section("more pliers on scalar");
    {
        tr.test_eq(-99, ra::map([](auto && x) { return -x; }, ra::scalar(99)));
        tr.test_eq(true, every(ra::expr([](auto && x) { return x>0; }, ra::start(99))));
    }
    return tr.summary();
}
