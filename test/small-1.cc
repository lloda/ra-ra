// -*- mode: c++; coding: utf-8 -*-
/// @file small-1.cc
/// @brief Making ra::Small and its iterator work with expressions/traversal.

// (c) Daniel Llorens - 2014, 2016-2017, 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// See also small-0.cc.

#include <iostream>
#include <iterator>
#include "ra/complex.hh"
#include "ra/ra.hh"
#include "ra/test.hh"
#include "ra/mpdebug.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr;
    tr.section("pieces of transpose(ra::Small)");
    {
        using sizes = mp::int_list<1, 2, 3, 4, 5>;
        using strides = mp::int_list<1, 10, 100, 1000, 10000>;

        using c0 = ra::axis_indices<mp::int_list<0, 1, 3, 2, 0>, mp::int_t<0>>::type;
        using e0 = mp::int_list<0, 4>;
        tr.info(mp::print_int_list<e0> {}, " vs ", mp::print_int_list<c0> {}).test(std::is_same_v<e0, c0>);

        using c1 = ra::axis_indices<mp::int_list<0, 1, 3, 2, 0>, mp::int_t<1>>::type;
        using e1 = mp::int_list<1>;
        tr.info(mp::print_int_list<e1> {}, " vs ", mp::print_int_list<c1> {}).test(std::is_same_v<e1, c1>);

        using call = ra::axes_list_indices<mp::int_list<0, 1, 3, 2, 0>, sizes, strides>::type;
        using eall = std::tuple<mp::int_list<0, 4>, mp::int_list<1>, mp::int_list<3>, mp::int_list<2>>;
        tr.info(mp::print_int_list<eall> {}, " vs ", mp::print_int_list<call> {}).test(std::is_same_v<eall, call>);
    }
    tr.section("transpose(ra::Small)");
    {
        ra::Small<double, 2, 3> const a(ra::_0 + 10*ra::_1);
        tr.info("<0 1>").test_eq(a, ra::transpose<0, 1>(a));
        tr.info("<1 0>").test_eq(ra::Small<double, 3, 2>(10*ra::_0 + ra::_1), ra::transpose<1, 0>(a));
        tr.info("<0 0>").test_eq(ra::Small<double, 2> {0, 11}, ra::transpose<0, 0>(a));

        ra::Small<double, 2, 3> b(ra::_0 + 10*ra::_1);
        tr.info("<0 1>").test_eq(a, ra::transpose<0, 1>(a));
        tr.info("<1 0>").test_eq(ra::Small<double, 3, 2>(10*ra::_0 + ra::_1), ra::transpose<1, 0>(a));
        ra::transpose<0, 0>(b) = {7, 9};
        tr.info("<0 0>").test_eq(ra::Small<double, 2, 3>{7, 10, 20, 1, 9, 21}, b);

        ra::Small<double> x {99};
        auto xt = transpose<>(x);
        tr.info("<> rank").test_eq(0, xt.rank());
        tr.info("<>").test_eq(99, xt);

        ra::Small<double, 3, 3> x3 = ra::_0 - ra::_1;
        ra::Small<double, 3, 3> y3 = transpose<1, 0>(x3);
        tr.info("transpose copy").test_eq(y3, ra::_1 - ra::_0);
        x3() = transpose<1, 0>(y3());
        tr.info("transpose copy").test_eq(x3, ra::_0 - ra::_1);
    }
    tr.section("sizeof");
    {
// These all static, but show the numbers if there's an error.
        tr.info("sizeof(ra::Small<double>)")
            .test_eq(sizeof(double), sizeof(ra::Small<double>));
        tr.info("sizeof(ra::Small<double, 0>)")
            .test(sizeof(double)==sizeof(ra::Small<double, 0>) || 0==sizeof(ra::Small<double, 0>)); // don't rely on either.
        tr.info("sizeof(ra::Small<double, 1>)")
            .test_eq(sizeof(double), sizeof(ra::Small<double, 1>));
        tr.info("sizeof(ra::Small<double, 2>)")
            .test_eq(2*sizeof(double), sizeof(ra::Small<double, 2>));
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
    tr.section("iterators' shape_type is not Small, so it can be used by Small");
    {
        auto z = ra::ra_traits<std::array<double, 3>>::make(3);
        tr.test_eq(3u, z.size());
    }
    tr.section("traits");
    {
        ra::Small<double, 2, 3> a {1, 2, 3, 4, 5, 6};
        tr.test_eq(ra::Small<ra::dim_t, 2> {2, 3}, ra::ra_traits<decltype(a)>::shape(a));
    }
    tr.section("static stride computation");
    {
        using d = mp::int_list<3, 4, 5>;
        using s = ra::default_strides<d>;
        tr.info("stride 0").test_eq(20, mp::ref<s, 0>::value);
        tr.info("stride 1").test_eq(5, mp::ref<s, 1>::value);
        tr.info("stride 2").test_eq(1, mp::ref<s, 2>::value);
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

// only valid if operator() -> rank 0 returns rank 0 array and not scalar
            // auto s2 = s(1, 1);
            // double check2[1] = { 5 };
            // tr.test(std::equal(s2.begin(), s2.end(), check2));
            tr.test_eq(5, s(1, 1));
        }
        tr.section("using SmallView as rvalue");
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

// check that SmallView = SmallView copies contents, just as View = View.
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
            auto k2 = s.at(i2).begin(); tr.test(std::equal(check2, check2+1, k2));
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
                tr.test(std::equal(check0, check0+3, s(ra::all, 0).begin()));
                tr.test(std::equal(check1, check1+3, s(ra::all, 1).begin()));
                tr.test(std::equal(s(ra::all, 0).begin(), s(ra::all, 0).end(), check0));
                tr.test(std::equal(s(ra::all, 1).begin(), s(ra::all, 1).end(), check1));
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
            tr.test_eq(3, t.size(0));
            tr.test_eq(2, t.size(1));
            tr.test_eq(10, t(0, 0));
            tr.test_eq(11, t(0, 1));
            tr.test_eq(110, t(1, 0));
            tr.test_eq(111, t(1, 1));
            tr.test_eq(210, t(2, 0));
            tr.test_eq(211, t(2, 1));
            tr.test_eq(ra::Small<int, 3, 2> { 10, 11, 110, 111, 210, 211 }, t);
            tr.test_eq(4, t.stride(0));
            tr.test_eq(1, t.stride(1));
// check STL iterator.
            {
                int check[] = { 10, 11, 110, 111, 210, 211 };
                tr.test(std::equal(t.begin(), t.end(), check));
                tr.test(std::equal(check, check+6, t.begin()));
            }
        }
    }
    tr.section("Small<> can be constexpr");
    {
        constexpr ra::Small<int, 2, 2> a = {1, 2, 3, 4};
        using Va = mp::int_t<int(a(1, 0))>;
        tr.test_eq(3, Va::value);
        using Vc = mp::int_t<sum(a)>; // constexpr reduction!
        tr.test_eq(10, Vc::value);
        constexpr ra::Small<int> b = { 9 }; // needs std::fill
        using Vb = mp::int_t<int(b)>;
        tr.test_eq(9, Vb::value);
    }
    tr.section("custom strides. List init is always row-major.");
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
                        using dim1 = std::array<ra::dim_t, 1>;
                        auto sizes = mp::tuple_values<dim1, typename A::sizes>();
                        auto strides = mp::tuple_values<dim1, typename A::strides>();
                        tr.test_eq(dim1 {3}, ra::start(sizes));
                        tr.test_eq(dim1 {2}, ra::start(strides));
                    };
        ra::SmallArray<double, mp::int_list<2, 3>, mp::int_list<1, 2>> a { 1, 2, 3, 4, 5, 6 };
        ra::SmallArray<double, mp::int_list<2, 3>, mp::int_list<1, 2>> b { {1, 2, 3}, {4, 5, 6} };
        test(a);
        test(b);
    }
    tr.section("SmallArray converted to SmallView");
    {
        ra::Small<double, 2, 3> a { 1, 2, 3, 4, 5, 6 };
        ra::SmallView<double, mp::int_list<2, 3>, mp::int_list<3, 1>> b = a();
        tr.test_eq(a, b);
// non-default strides (fortran / column major order).
        ra::SmallArray<double, mp::int_list<2, 3>, mp::int_list<1, 2>> ax { 1, 2, 3, 4, 5, 6 };
        ra::SmallView<double, mp::int_list<2, 3>, mp::int_list<1, 2>> bx = ax();
        tr.test_eq(a, ax);
        tr.test_eq(a, bx);
// check iterators.
        tr.test(std::equal(a.begin(), a.end(), ax.begin()));
        tr.test(std::equal(ax.begin(), ax.end(), a.begin()));
        tr.test(std::equal(b.begin(), b.end(), bx.begin()));
        tr.test(std::equal(bx.begin(), bx.end(), b.begin()));
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
    tr.section("using cell_iterator with SmallBase");
    {
        cout << "TODO" << endl;
    }
    tr.section("expr with Small, rank 1");
    {
        ra::Small<double, 3> a { 1, 4, 2 };
        tr.test_eq(3, a.iter().size(0));
#define TEST(plier)                                                     \
        {                                                               \
            double s = 0;                                               \
            plier(ra::expr([&s](double & a) { s += a; }, a.iter()));    \
            tr.test_eq(7, s);                                           \
        }
        TEST(ply_ravel);
        TEST(ply);
#undef TEST
            }
    tr.section("expr with Small, rank 2");
    {
        ra::Small<double, 3, 2> a { 1, 4, 2, 5, 3, 6 };
        tr.test_eq(3, a.iter().size(0));
        tr.test_eq(2, a.iter().size(1));
#define TEST(plier)                                                     \
        {                                                               \
            double s = 0;                                               \
            plier(ra::expr([&s](double & a) { s += a; }, a.iter()));    \
            tr.test_eq(21, s);                                          \
        }
        TEST(ply_ravel);
        TEST(ply);
#undef TEST
#define TEST(plier)                                                     \
        {                                                               \
            ra::Small<double, 3, 2> b;                                  \
            plier(ra::expr([](double & a, double & b) { b = -a; }, a.iter(), b.iter())); \
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
        tr.test_eq(ra::Small<double, 3, 2> { 1, 4, 2, 5, 3, 6 }, transpose<1, 0>(a));
        ra::transpose<1, 0>(a) = { 1, 2, 3, 4, 5, 6 };
        tr.test_eq(ra::Small<double, 2, 3> { 1, 3, 5, 2, 4, 6 }, a);
    }
    tr.section("diag");
    {
        ra::Small<double, 3, 3> a = ra::_0*3 + ra::_1;
        tr.test_eq(ra::Small<double, 3> { 0, 4, 8 }, diag(a));
        diag(a) = { 11, 22, 33 };
        tr.test_eq(ra::Small<double, 3, 3> { 11, 1, 2, 3, 22, 5, 6, 7, 33 }, a);
    }
    tr.section("renames");
    {
        ra::Small<double, 2, 2> a { 13, 8, 75, 19 };
        ra::mat_uv<double> b(a);
        assert(b.uu==13 && b.uv==8 && b.vu==75 && b.vv==19);
        ra::Small<double, 3> x { 13, 8, 75 };
        ra::vec_xyz<double> y(x);
        assert(y.x==13 && y.y==8 && y.z==75);
    }
    tr.section(".back()");
    {
        ra::Small<double, 3> a = ra::_0*3;
        tr.test_eq(0, a[0]);
        tr.test_eq(3, a[1]);
        tr.test_eq(6, a[2]);
        tr.test_eq(6, a.back());
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
            ra::SmallView<double, mp::int_list<3>, mp::int_list<2>> c(b.data()); // TODO no syntax yet.
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
            ra::SmallView<double, mp::int_list<3>, mp::int_list<2>> c(b.data()); // TODO no syntax yet.
            test_fra(c, c.as<2, 1>());
        }
        auto test_fra_rank_2 = [&tr](auto && a, auto && b)
            {
                tr.test_eq(2, b.size(0));
                tr.test_eq(2, b.size(1));
                tr.test_eq(ra::Small<double, 2, 2> { 3, 4, 5, 6 }, b);
                b = ra::Small<double, 2, 2> { 13, 14, 15, 16 };
                tr.test_eq(ra::Small<double, 3, 2> { 1, 2, 13, 14, 15, 16 }, a);
            };
        {
            ra::Small<double, 3, 2> a = { 1, 2, 3, 4, 5, 6 };
            test_fra_rank_2(a, a.as<2, 1>());
            ra::Small<double, 6, 2> b = { 1, 2, 99, 99, 3, 4, 99, 99, 5, 6, 99, 99 };
            ra::SmallView<double, mp::int_list<3, 2>, mp::int_list<4, 1>> c(b.data()); // TODO no syntax yet.
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
// ASSIGNOPS for SmallBase.iter()
    {
        ra::Small<int, 3> s {1, 2, 3};
        s.iter() += 9;
        tr.test_eq(ra::start({10, 11, 12}), s);
    }
    return tr.summary();
}
