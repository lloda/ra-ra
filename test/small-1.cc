// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Making ra::Small and its iterator work with expressions/traversal.

// (c) Daniel Llorens - 2014-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// See also small-0.cc.

#include <iostream>
#include <iterator>
#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using ra::ilist_t, ra::int_c, ra::ic_t, ra::Dim, ra::mp::print_ilist_t, ra::mp::ref, ra::transpose;
using int2 = ra::Small<int, 2>;

int main()
{
    TestRecorder tr;
    tr.section("transpose(ra::Small)");
    {
        ra::Small<double, 2, 3> const a(ra::_0 + 10*ra::_1);
        tr.info("<0 1>").test_eq(a, transpose(a, ilist_t<0, 1>{}));
        tr.info("<1 0>").test_eq(ra::Small<double, 3, 2>(10*ra::_0 + ra::_1), transpose(a, ilist_t<1, 0>{}));
        tr.info("<1 0> by default").test_eq(ra::Small<double, 3, 2>(10*ra::_0 + ra::_1), transpose(a));
        tr.info("<0 0>").test_eq(ra::Small<double, 2> {0, 11}, transpose(a, ilist_t<0, 0>{}));

        ra::Small<double, 2, 3> b(ra::_0 + 10*ra::_1);
        tr.info("<0 1>").test_eq(a, transpose(a, ilist_t<0, 1>{}));
        tr.info("<1 0>").test_eq(ra::Small<double, 3, 2>(10*ra::_0 + ra::_1), transpose(a, ilist_t<1, 0>{}));
        tr.info("<1 0> by default").test_eq(ra::Small<double, 3, 2>(10*ra::_0 + ra::_1), transpose(a));
        transpose(b, ilist_t<0, 0>{}) = {7, 9};
        tr.info("<0 0>").test_eq(ra::Small<double, 2, 3>{7, 10, 20, 1, 9, 21}, b);

        ra::Small<double> x {99};
        auto xt = transpose(x, ilist_t<>{});
        tr.info("<> rank").test_eq(0, xt.rank());
        tr.info("<>").test_eq(99, xt);

        ra::Small<double, 3, 3> x3 = ra::_0 - ra::_1;
        ra::Small<double, 3, 3> y3 = transpose(x3, ilist_t<1, 0>{});
        tr.info("transpose copy").test_eq(y3, ra::_1 - ra::_0);
        x3() = transpose(y3(), ilist_t<1, 0>{});
        tr.info("transpose copy").test_eq(x3, ra::_0 - ra::_1);
    }
    tr.section("internal fields");
    {
        {
            using A = ra::Small<double, 10, 10>;
            alignas(A) double storage[sizeof(A)/sizeof(double)];
            A * a = new (&storage) A();
            std::fill(a->data(), a->data()+100, 0.);
            storage[99] = 1.3;
            std::cout << (*a) << std::endl;
            tr.test_eq(1.3, a->data()[99]);
            tr.test_eq(1.3, (*a)(9, 9));
        }
        {
            ra::Small<double, 2, 3> a {1, 2, 3, 4, 5, 6};
            tr.test_eq(2*3*sizeof(double), sizeof(a));
            tr.test_eq(1, a.data()[0]);
        }
    }
    tr.section("top level generics");
    {
        ra::Small<double, 2, 3> a {1, 2, 3, 4, 5, 6};
        tr.test_eq(ra::Small<ra::dim_t, 2> {2, 3}, shape(a));
        tr.test_eq(3u, ra::size(std::array<double, 3>()));
    }
    tr.section("static step computation");
    {
        auto dims = ra::default_dims(std::array<ra::dim_t, 3> {3, 4, 5});
        tr.info("step 0").test_eq(20, dims[0].step);
        tr.info("step 1").test_eq(5, dims[1].step);
        tr.info("step 2").test_eq(1, dims[2].step);
    }
    tr.section("subscripts");
    {
        tr.section("with scalar indices");
        {
            ra::Small<double, 3, 2> s { 1, 4, 2, 5, 3, 6 };

            auto s0 = s();
            double check0[6] = { 1, 4, 2, 5, 3, 6 };
            tr.test(std::equal(s0.begin(), s0.end(), check0));

            auto s1 = s(1);
            double check1[3] = { 2, 5 };
            cout << "s1: " << s1(0) << ", " << s1(1) << endl;
            tr.test(s1(0)==2 && s1(1)==5);
            tr.test(std::equal(s1.begin(), s1.end(), check1));
            tr.test_eq(5, s(1, 1));
        }
        tr.section("using ViewSmall as rvalue");
        {
            ra::Small<double, 3, 2> s { 1, 4, 2, 5, 3, 6 };
// use as rvalue.
            s(0) = { 3, 2 };
            s(1) = { 5, 4 };
            s(2) = { 7, 6 };
            cout << s << endl;
            tr.test_eq(ra::Small<double, 3, 2> { 3, 2, 5, 4, 7, 6 }, s);

            ra::Small<double, 3, 2> z = s;
            z *= -1;

// check that ViewSmall = ViewSmall copies contents, just as View = View.
            s(0) = z(2);
            s(1) = z(1);
            s(2) = z(0);
            tr.test_eq(ra::Small<double, 3, 2> { -3, -2, -5, -4, -7, -6 }, z);
            tr.test_eq(ra::Small<double, 3, 2> { -7, -6, -5, -4, -3, -2 }, s);
        }
        tr.section("with tuples");
        {
            ra::Small<double, 3, 2> s { 1, 4, 2, 5, 3, 6 };
            ra::Small<int, 2> i2 { 1, 1 };
            ra::Small<int, 1> i1 { 1 };
            ra::Small<int, 0> i0 { };
            double check2[1] = { 5 };
            double check1[2] = { 2, 5 };
            double check0[6] = { 1, 4, 2, 5, 3, 6 };
            auto k2 = s.at(i2);
            tr.test_eq(0, ra::rank(k2));
            tr.test_eq(check2[0], k2);
            auto k1 = s.at(i1).begin(); tr.test(std::equal(check1, check1+2, k1));
            auto k0 = s.at(i0).begin(); tr.test(std::equal(check0, check0+6, k0));
        }
        tr.section("with rank 1 subscripts");
        {
            ra::Small<double, 3, 2> s { 1, 4, 2, 5, 3, 6 };
            tr.test_eq(ra::Small<int, 2> { 1, 4 }, s(0));
            tr.test_eq(ra::Small<int, 2> { 2, 5 }, s(1));
            tr.test_eq(ra::Small<int, 2> { 3, 6 }, s(2));
            tr.test_eq(ra::Small<int, 3> { 1, 2, 3 }, s(ra::all, 0));
            tr.test_eq(ra::Small<int, 3> { 4, 5, 6 }, s(ra::all, 1));
            tr.test_eq(1, s(ra::all, 1).rank());
// check STL iterator.
            {
                int check0[] = { 1, 2, 3 };
                int check1[] = { 4, 5, 6 };
                tr.test(std::ranges::equal(check0, check0+3, s(ra::all, 0).begin(), s(ra::all, 0).end()));
                tr.test(std::ranges::equal(check1, check1+3, s(ra::all, 1).begin(), s(ra::all, 1).end()));
                tr.test(std::ranges::equal(s(ra::all, 0).begin(), s(ra::all, 0).end(), check0, check0+3));
                tr.test(std::ranges::equal(s(ra::all, 1).begin(), s(ra::all, 1).end(), check1, check1+3));
            }
            tr.test_eq(1, s(ra::all, 0)[0]);
            tr.test_eq(2, s(ra::all, 0)[1]);
            tr.test_eq(3, s(ra::all, 0)[2]);
            tr.test_eq(4, s(ra::all, 1)(0));
            tr.test_eq(5, s(ra::all, 1)(1));
            tr.test_eq(6, s(ra::all, 1)(2));
            using I0 = ra::Small<ra::dim_t, 1>;
            tr.test_eq(1, s(ra::all, 0).at(I0 {0}));
            tr.test_eq(2, s(ra::all, 0).at(I0 {1}));
            tr.test_eq(3, s(ra::all, 0).at(I0 {2}));
            tr.test_eq(4, s(ra::all, 1).at(I0 {0}));
            tr.test_eq(5, s(ra::all, 1).at(I0 {1}));
            tr.test_eq(6, s(ra::all, 1).at(I0 {2}));
        }
        tr.section("with rank 1 subscripts, result rank > 1");
        {
            ra::Small<double, 3, 2, 2> s  = 100*ra::_0 + 10*ra::_1 + 1*ra::_2;
            cout << s << endl;
            auto t = s(ra::all, 1, ra::all);
            tr.test_eq(2, t.rank());
            tr.test_eq(3, t.len(0));
            tr.test_eq(2, t.len(1));
            tr.test_eq(10, t(0, 0));
            tr.test_eq(11, t(0, 1));
            tr.test_eq(110, t(1, 0));
            tr.test_eq(111, t(1, 1));
            tr.test_eq(210, t(2, 0));
            tr.test_eq(211, t(2, 1));
            tr.test_eq(ra::Small<int, 3, 2> { 10, 11, 110, 111, 210, 211 }, t);
            tr.test_eq(4, t.step(0));
            tr.test_eq(1, t.step(1));
// check STL iterator.
            {
                int check[] = { 10, 11, 110, 111, 210, 211 };
                tr.test(std::ranges::equal(t.begin(), t.end(), check, check+6));
                tr.test(std::ranges::equal(check, check+6, t.begin(), t.end()));
            }
        }
    }
    tr.section("Small<> can be constexpr");
    {
        constexpr ra::Small<int, 2, 2> a = {1, 2, 3, 4};
        using Va = int_c<int(a(1, 0))>;
        tr.test_eq(3, Va::value);
        using Vc = int_c<sum(a)>; // constexpr reduction!
        tr.test_eq(10, Vc::value);
        constexpr ra::Small<int> b = { 9 }; // needs std::fill
        using Vb = int_c<int(b)>;
        tr.test_eq(9, Vb::value);
    }
    tr.section("custom steps. List init is row-major regardless.");
    {
        auto test = [&tr](auto && a)
                    {
                        tr.test_eq(1, a(0, 0));
                        tr.test_eq(2, a(0, 1));
                        tr.test_eq(3, a(0, 2));
                        tr.test_eq(4, a(1, 0));
                        tr.test_eq(5, a(1, 1));

                        tr.test_eq(6, a(1, 2));
                        tr.test_eq(1, a(0)(0));
                        tr.test_eq(2, a(0)(1));
                        tr.test_eq(3, a(0)(2));
                        tr.test_eq(4, a(1)(0));
                        tr.test_eq(5, a(1)(1));
                        tr.test_eq(6, a(1)(2));

                        using A = std::decay_t<decltype(a(0))>;
                        tr.test_eq(1, ra::size(A::dimv));
                        tr.test_eq(3, A::dimv[0].len);
                        tr.test_eq(2, A::dimv[0].step);
                    };

        ra::SmallArray<double, ic_t<std::array {Dim {2, 1}, Dim {3, 2}}>> a { 1, 2, 3, 4, 5, 6 };
        ra::SmallArray<double, ic_t<std::array {Dim {2, 1}, Dim {3, 2}}>> b { {1, 2, 3}, {4, 5, 6} };
        test(a);
        test(b);
    }
    tr.section("SmallArray converted to ViewSmall");
    {
        ra::Small<double, 2, 3> a { 1, 2, 3, 4, 5, 6 };
        ra::ViewSmall<double *, ic_t<std::array {Dim {2, 3}, Dim {3, 1}}>> b = a();
        tr.test_eq(a, b);
// non-default steps (fortran / column major order).
        ra::SmallArray<double, ic_t<std::array {Dim {2, 1}, Dim {3, 2}}>> ax { 1, 2, 3, 4, 5, 6 };
        ra::ViewSmall<double *, ic_t<std::array {Dim {2, 1}, Dim {3, 2}}>> bx = ax();
        tr.test_eq(a, ax);
        tr.test_eq(a, bx);
// check iterators.
        tr.test(std::ranges::equal(a.begin(), a.end(), ax.begin(), ax.end()));
        tr.test(std::ranges::equal(ax.begin(), ax.end(), a.begin(), a.end()));
        tr.test(std::ranges::equal(b.begin(), b.end(), bx.begin(), bx.end()));
        tr.test(std::ranges::equal(bx.begin(), bx.end(), b.begin(), b.end()));
// check memory order.
        double fcheck[6] = { 1, 4, 2, 5, 3, 6 };
        tr.test(std::equal(fcheck, fcheck+6, ax.data()));
        tr.test(std::equal(fcheck, fcheck+6, bx.data()));
// views work as views.
        bx = 77.;
        tr.test_eq(77., ax);
        b = 99.;
        tr.test_eq(99., a);
    }
    tr.section("map with Small, rank 1");
    {
        ra::Small<double, 3> a { 1, 4, 2 };
        tr.test_eq(3, a.iter().len(0));
#define TEST(plier)                                                     \
        {                                                               \
            double s = 0;                                               \
            plier(ra::map_([&s](double & a) { s += a; }, a.iter()));    \
            tr.test_eq(7, s);                                           \
        }
        TEST(ply_ravel);
        TEST(ply);
#undef TEST
            }
    tr.section("map with Small, rank 2");
    {
        ra::Small<double, 3, 2> a { 1, 4, 2, 5, 3, 6 };
        tr.test_eq(3, a.iter().len(0));
        tr.test_eq(2, a.iter().len(1));
#define TEST(plier)                                                     \
        {                                                               \
            double s = 0;                                               \
            plier(ra::map_([&s](double & a) { s += a; }, a.iter()));    \
            tr.test_eq(21, s);                                          \
        }
        TEST(ply_ravel);
        TEST(ply);
#undef TEST
#define TEST(plier)                                                     \
        {                                                               \
            ra::Small<double, 3, 2> b;                                  \
            plier(ra::map_([](double & a, double & b) { b = -a; }, a.iter(), b.iter())); \
            tr.test_eq(-1, b(0, 0));                                    \
            tr.test_eq(-4, b(0, 1));                                    \
            tr.test_eq(-2, b(1, 0));                                    \
            tr.test_eq(-5, b(1, 1));                                    \
            tr.test_eq(-3, b(2, 0));                                    \
            tr.test_eq(-6, b(2, 1));                                    \
        }
        TEST(ply_ravel);
        TEST(ply);
#undef TEST
    }
    tr.section("Small as value type in var-size array");
    {
        {
// This pain with rank 0 arrays and ra::scalar can be avoided with ply; see e.g. grid_interp_n() in src/grid.cc.
            ra::Unique<ra::Small<double, 2>, 1> b({4}, ra::scalar(ra::Small<double, 2> { 3., 1. }));
            tr.test_eq(3., b(0)(0));
            tr.test_eq(1., b(0)(1));

// if () returns rank 0 instead of scalar, otherwise ct error.
            // b(1) = ra::scalar(ra::Small<double, 2> { 7., 9. });
            // cout << b << endl;
// if () returns scalar instead of rank 0, otherwise bug. (This is what happens).
            b(1) = ra::Small<double, 2> { 7., 9. };
            tr.test_eq(3., b(0)(0));
            tr.test_eq(1., b(0)(1));
            tr.test_eq(7., b(1)(0));
            tr.test_eq(9., b(1)(1));
        }
        {
            ra::Unique<double, 1> b({2}, { 3., 1. });
            tr.test_eq(3., b(0));
            tr.test_eq(1., b(1));
            b = ra::Small<double, 2> { 7., 9. };
            cout << b << endl;
            tr.test_eq(7., b(0));
            tr.test_eq(9., b(1));
        }
        {
            ra::Unique<double, 2> b({2, 2}, { 3., 1., 3., 1. });
            b(1) = ra::Small<double, 2> { 7., 9. };
            tr.test_eq(3., b(0, 0));
            tr.test_eq(1., b(0, 1));
            tr.test_eq(7., b(1, 0));
            tr.test_eq(9., b(1, 1));
        }
        {
            ra::Unique<ra::Small<double, 2>, 0> b(ra::scalar(ra::Small<double, 2>{3., 1.}));
            b = ra::scalar(ra::Small<double, 2> { 7., 9. });
            tr.test_eq(7., b()(0));
            tr.test_eq(9., b()(1));
        }
        {
            ra::Unique<ra::Small<double, 2>, 1> b({4}, ra::scalar(ra::Small<double, 2> { 3., 1. }));
            ra::Small<double, 2> u = b(1);
            tr.test_eq(3, u[0]);
            tr.test_eq(1, u[1]);
            ra::Small<double, 2> v(b(1));
            tr.test_eq(3, v[0]);
            tr.test_eq(1, v[1]);
        }
    }
    tr.section("transpose");
    {
        ra::Small<double, 2, 3> a { 1, 2, 3, 4, 5, 6 };
        tr.test_eq(ra::Small<double, 3, 2> { 1, 4, 2, 5, 3, 6 }, transpose(a, ilist_t<1, 0>{}));
        transpose(a, ilist_t<1, 0>{}) = { 1, 2, 3, 4, 5, 6 };
        tr.test_eq(ra::Small<double, 2, 3> { 1, 3, 5, 2, 4, 6 }, a);
    }
    tr.section("diag");
    {
        ra::Small<double, 3, 3> a = ra::_0*3 + ra::_1;
        tr.test_eq(ra::Small<double, 3> { 0, 4, 8 }, diag(a));
        diag(a) = { 11, 22, 33 };
        tr.test_eq(ra::Small<double, 3, 3> { 11, 1, 2, 3, 22, 5, 6, 7, 33 }, a);
    }
    tr.section(".back()");
    {
        ra::Small<double, 3> a = ra::_0*3;
        tr.test_eq(0, a[0]);
        tr.test_eq(3, a[1]);
        tr.test_eq(6, a[2]);
        tr.test_eq(6, a.back());
    }
    tr.section(".back() is last element not last item");
    {
        ra::Small<int2> b(ra::scalar(int2 {1, 3})); // cf [ma116]
        tr.test_eq(int2 {1, 3}, b.back());
    }
// TODO Replace with uniform subscripting (ra::iota).
    tr.section("compile time subscripting of ra::Small (as)");
    {
        auto test_as = [&tr](auto && a, auto && b)
            {
                tr.test_eq(2, b.size());
                tr.test_eq(1, b[0]);
                tr.test_eq(2, b[1]);
                b = { 7, 8 };
                tr.test_eq(7, a[0]);
                tr.test_eq(8, a[1]);
                tr.test_eq(3, a[2]);
            };
        {
            ra::Small<double, 3> a = { 1, 2, 3 };
            test_as(a, a.as<2>());
            ra::Small<double, 6> b = { 1, 99, 2, 99, 3, 99 };
            ra::ViewSmall<double *, ic_t<std::array {Dim {3, 2}}>> c(b.data()); // TODO no syntax yet.
            test_as(c, c.as<2>());
        }
        auto test_fra = [&tr](auto && a, auto && b)
            {
                tr.test_eq(2, b.size());
                tr.test_eq(2, b[0]);
                tr.test_eq(3, b[1]);
                b = { 7, 8 };
                tr.test_eq(1, a[0]);
                tr.test_eq(7, a[1]);
                tr.test_eq(8, a[2]);
            };
        {
            ra::Small<double, 3> a = { 1, 2, 3 };
            test_fra(a, a.as<2, 1>());
            ra::Small<double, 6> b = { 1, 99, 2, 99, 3, 99 };
            ra::ViewSmall<double *, ic_t<std::array {Dim {3, 2}}>> c(b.data()); // TODO no syntax yet.
            test_fra(c, c.as<2, 1>());
        }
        auto test_fra_rank_2 = [&tr](auto && a, auto && b)
            {
                tr.test_eq(2, b.len(0));
                tr.test_eq(2, b.len(1));
                tr.test_eq(ra::Small<double, 2, 2> { 3, 4, 5, 6 }, b);
                b = ra::Small<double, 2, 2> { 13, 14, 15, 16 };
                tr.test_eq(ra::Small<double, 3, 2> { 1, 2, 13, 14, 15, 16 }, a);
            };
        {
            ra::Small<double, 3, 2> a = { 1, 2, 3, 4, 5, 6 };
            test_fra_rank_2(a, a.as<2, 1>());
            ra::Small<double, 6, 2> b = { 1, 2, 99, 99, 3, 4, 99, 99, 5, 6, 99, 99 };
            ra::ViewSmall<double *, ic_t<std::array {Dim {3, 4}, Dim {2, 1}}>> c(b.data()); // TODO no syntax yet.
            test_fra_rank_2(c, c.as<2, 1>());
        }
    }
    tr.section("cat");
    {
        tr.test_eq(ra::Small<int, 4> {1, 2, 3, 4}, cat(ra::Small<int, 3> {1, 2, 3}, 4));
        tr.test_eq(ra::Small<int, 4> {4, 1, 2, 3}, cat(4, ra::Small<int, 3> {1, 2, 3}));
        tr.test_eq(ra::Small<int, 5> {1, 2, 3, 4, 5}, cat(ra::Small<int, 2> {1, 2}, ra::Small<int, 3> {3, 4, 5}));
    }
    tr.section("a demo on rank1of1 vs rank2");
    {
// by prefix matching, first dim is 2 for both so they get matched. Then {1 2}
// (a 'scalar') gets matched to 10 & 20 in succesion. This used to be forbidden in Small::Small(X && x), but now I value consistency more.
        ra::Small<ra::Small<double, 2>, 2> a = { {1, 2}, {3, 4} };
        ra::Small<double, 2, 2> b = { 10, 20, 30, 40 };
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
        // a = b; // TODO Check that this static fails
        cout << "a = b, a: " << a << endl;
    }
// ASSIGNOPS for ViewSmall
    {
        ra::Small<int, 3> s {1, 2, 3};
        s.iter() += 9;
        tr.test_eq(ra::start({10, 11, 12}), s);
    }
    tr.section("deduction guides");
    {
        ra::SmallArray a {1, 2, 3}; // FIXME the deduction guide can't work for ra::Small
        tr.test_eq(ra::start({1, 2, 3}), a);
    }
    tr.section("multidimensional []");
    {
        ra::Small<int, 3, 2, 4> a = ra::_0 + ra::_1 - ra::_2;
        tr.test_eq(a(ra::all, 0), a[ra::all, 0]);
    }
    tr.section("ViewSmall of Seq");
    {
        ra::ViewSmall<ra::Seq<int>, ra::ic_t<std::array {ra::Dim {3, 2}, ra::Dim {2, 1}}>> a(ra::Seq {1});
        std::println(cout, "{:c:2}\n", transpose(a));
        tr.test_eq(a, 1+ra::Small<int, 3, 2> {{0, 1}, {2, 3}, {4, 5}});
    }
    tr.section("ViewSmall as iota<w>");
// in order to replace Ptr<>, we must support Len in View::P and View::Dimv.
    {
        constexpr ra::ViewSmall<ra::Seq<ra::dim_t>, ra::ic_t<std::array {ra::Dim {ra::UNB, 1}}>>
            i0(ra::Seq<ra::dim_t> {0});
        constexpr ra::ViewSmall<ra::Seq<ra::dim_t>, ra::ic_t<std::array {ra::Dim {ra::UNB, 0}, ra::Dim {ra::UNB, 1}}>>
            i1(ra::Seq<ra::dim_t> {0});
        tr.strict().test_eq(ra::Small<int, 3> {1, 3, 5}, i0 + ra::Small<int, 3> {1, 2, 3});
        ra::Big<ra::dim_t> p({3, 4}, i0 - i1);
        tr.strict().test_eq(ra::Big<ra::dim_t, 2> {{0, -1, -2, -3}, {1, 0, -1, -2}, {2, 1, 0, -1}}, p);
    }
    return tr.summary();
}
