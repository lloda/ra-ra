
// (c) Daniel Llorens - 2014

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-small.C
/// @brief Making ra::Small and its iterator work with expressions/traversal.

#include <iostream>
#include <iterator>
#include "ra/complex.H"
#include "ra/ra-small.H"
#include "ra/ra-iterator.H"
#include "ra/ra-operators.H"
#include "ra/ra-large.H"
#include "ra/format.H"
#include "ra/test.H"

using std::cout; using std::endl; using std::flush;

template <template <class, class, class> class Child_, class T, class sizes_, class strides_>
using small_iterator = ra::ra_iterator<ra::SmallBase<ra::SmallSlice, T, sizes_, strides_>, 0>;

int main()
{
    TestRecorder tr;
    section("constructors");
    {
        {
            ra::Small<int, 1> a(9);
            tr.test_equal(1, a.rank());
            tr.test_equal(1, a.size());
            tr.test_equal(9, a[0]);
        }
        {
            ra::Small<complex, 1> a = 9.;
            tr.test_equal(1, a.rank());
            tr.test_equal(1, a.size());
            tr.test_equal(9., a[0]);
        }
    }
    section("operator=");
    {
        ra::Small<complex, 2> a { 3, 4 };
        a = complex(99.);
        tr.test_equal(99., a[0]);
        tr.test_equal(99., a[1]);
        a = 88.;
        tr.test_equal(88., a[0]);
        tr.test_equal(88., a[1]);
        a += 1.;
        tr.test_equal(89., a[0]);
        tr.test_equal(89., a[1]);
    }
    section("sizeof");
    {
// These all static, but show the numbers if there's an error.
        tr.info("sizeof(ra::Small<real>)")
            .test_equal(sizeof(real), sizeof(ra::Small<real>));
        tr.info("sizeof(ra::Small<real, 0>)")
            .test(sizeof(real)==sizeof(ra::Small<real, 0>) || 0==sizeof(ra::Small<real, 0>)); // don't rely on either.
        tr.info("sizeof(ra::Small<real, 1>)")
            .test_equal(sizeof(real), sizeof(ra::Small<real, 1>));
        tr.info("sizeof(ra::Small<real, 2>)")
            .test_equal(2*sizeof(real), sizeof(ra::Small<real, 2>));
    }
    section("internal fields");
    {
        {
            using A = ra::Small<real, 10, 10>;
            alignas(A) real storage[sizeof(A)/sizeof(real)];
            A * a = new (&storage) A();
            std::fill(a->data(), a->data()+100, 0.);
            storage[99] = 1.3;
            std::cout << (*a) << std::endl;
            tr.test_equal(1.3, a->data()[99]);
            tr.test_equal(1.3, (*a)(9, 9));
        }
        {
            ra::Small<real, 2, 3> a {1, 2, 3, 4, 5, 6};
            tr.test_equal(2*3*sizeof(real), sizeof(a));
            tr.test_equal(1, a.data()[0]);
        }
    }
    section("iterators' shape_type is not Small, so it can be used by Small");
    {
        auto z = ra::ra_traits<std::array<real, 3>>::make(3);
        cout << "z: " << rawp(z) << endl;
    }
    section("static stride computation");
    {
        using d = mp::int_list<3, 4, 5>;
        using s = typename ra::default_strides<d>::type;
        tr.info("stride 0").test_equal(20, mp::Ref_<s, 0>::value);
        tr.info("stride 1").test_equal(5, mp::Ref_<s, 1>::value);
        tr.info("stride 2").test_equal(1, mp::Ref_<s, 2>::value);
    }
    section("subscripts");
    {
        section("with scalar indices");
        {
            ra::Small<real, 3, 2> s { 1, 4, 2, 5, 3, 6 };

            auto s0 = s();
            real check0[6] = { 1, 4, 2, 5, 3, 6 };
            tr.test(std::equal(s0.begin(), s0.end(), check0));

            auto s1 = s(1);
            real check1[3] = { 2, 5 };
            cout << "s1: " << s1(0) << ", " << s1(1) << endl;
            tr.test(s1(0)==2 && s1(1)==5);
            tr.test(std::equal(s1.begin(), s1.end(), check1));

// only valid if operator() -> rank 0 returns rank 0 array and not scalar
            // auto s2 = s(1, 1);
            // real check2[1] = { 5 };
            // tr.test(std::equal(s2.begin(), s2.end(), check2));
            tr.test_equal(5, s(1, 1));
        }
        section("using SmallSlice as rvalue");
        {
            ra::Small<real, 3, 2> s { 1, 4, 2, 5, 3, 6 };
// use as rvalue.
            s(0) = { 3, 2 };
            s(1) = { 5, 4 };
            s(2) = { 7, 6 };
            cout << s << endl;
            tr.test_equal(ra::Small<real, 3, 2> { 3, 2, 5, 4, 7, 6 }, s);

            cout << "S00" << endl;
            ra::Small<real, 3, 2> z = s;
            cout << "S01" << endl;
            z *= -1;
            cout << "S02" << endl;

// check that SmallSlice = SmallSlice copies contents, just as Raw = Raw.
            s(0) = z(2);
            s(1) = z(1);
            s(2) = z(0);
            cout << "z: " << z << endl;
            tr.test_equal(ra::Small<real, 3, 2> { -3, -2, -5, -4, -7, -6 }, z);
            cout << "s: " << s << endl;
            tr.test_equal(ra::Small<real, 3, 2> { -7, -6, -5, -4, -3, -2 }, s);
        }
        section("with tuples");
        {
            ra::Small<real, 3, 2> s { 1, 4, 2, 5, 3, 6 };
            ra::Small<int, 2> i2 { 1, 1 };
            ra::Small<int, 1> i1 { 1 };
            ra::Small<int, 0> i0 { };
            cout << "s(i2): " << s.at(i2) << endl;
            cout << "s(i1): " << s.at(i1) << endl;
            cout << "s(i0): " << s.at(i0) << endl;
            real check2[1] = { 5 };
            real check1[2] = { 2, 5 };
            real check0[6] = { 1, 4, 2, 5, 3, 6 };
            auto k2 = s.at(i2).begin(); tr.test(std::equal(check2, check2+1, k2));
            auto k1 = s.at(i1).begin(); tr.test(std::equal(check1, check1+2, k1));
            auto k0 = s.at(i0).begin(); tr.test(std::equal(check0, check0+6, k0));
        }
        section("with rank 1 subscripts");
        {
            ra::Small<real, 3, 2> s { 1, 4, 2, 5, 3, 6 };
            cout << "s(1, :): " << s(0) << endl;
            cout << "s(2, :): " << s(1) << endl;
            cout << "s(3, :): " << s(2) << endl;
            tr.test_equal(ra::Small<int, 3> { 1, 2, 3 }, s(ra::all, 0));
            tr.test_equal(ra::Small<int, 3> { 4, 5, 6 }, s(ra::all, 1));
            tr.test_equal(1, s(ra::all, 1).rank());
// check STL iterator.
            {
                int check0[] = { 1, 2, 3 };
                int check1[] = { 4, 5, 6 };
                tr.test(std::equal(check0, check0+3, s(ra::all, 0).begin()));
                tr.test(std::equal(check1, check1+3, s(ra::all, 1).begin()));
                tr.test(std::equal(s(ra::all, 0).begin(), s(ra::all, 0).end(), check0));
                tr.test(std::equal(s(ra::all, 1).begin(), s(ra::all, 1).end(), check1));
            }
            tr.test_equal(1, s(ra::all, 0)[0]);
            tr.test_equal(2, s(ra::all, 0)[1]);
            tr.test_equal(3, s(ra::all, 0)[2]);
            tr.test_equal(4, s(ra::all, 1)(0));
            tr.test_equal(5, s(ra::all, 1)(1));
            tr.test_equal(6, s(ra::all, 1)(2));
            using I0 = ra::Small<ra::dim_t, 1>;
            tr.test_equal(1, s(ra::all, 0).at(I0 {0}));
            tr.test_equal(2, s(ra::all, 0).at(I0 {1}));
            tr.test_equal(3, s(ra::all, 0).at(I0 {2}));
            tr.test_equal(4, s(ra::all, 1).at(I0 {0}));
            tr.test_equal(5, s(ra::all, 1).at(I0 {1}));
            tr.test_equal(6, s(ra::all, 1).at(I0 {2}));
        }
        section("with rank 1 subscripts, result rank > 1");
        {
            ra::Small<real, 3, 2, 2> s  = 100*ra::_0 + 10*ra::_1 + 1*ra::_2;
            cout << s << endl;
            auto t = s(ra::all, 1, ra::all);
            tr.test_equal(2, t.rank());
            tr.test_equal(3, t.size(0));
            tr.test_equal(2, t.size(1));
            tr.test_equal(10, t(0, 0));
            tr.test_equal(11, t(0, 1));
            tr.test_equal(110, t(1, 0));
            tr.test_equal(111, t(1, 1));
            tr.test_equal(210, t(2, 0));
            tr.test_equal(211, t(2, 1));
            tr.test_equal(ra::Small<int, 3, 2> { 10, 11, 110, 111, 210, 211 }, t);
            tr.test_equal(4, t.stride(0));
            tr.test_equal(1, t.stride(1));
// check STL iterator.
            {
                int check[] = { 10, 11, 110, 111, 210, 211 };
                tr.test(std::equal(t.begin(), t.end(), check));
                tr.test(std::equal(check, check+6, t.begin()));
            }
        }
        // section("with unbeatable rank 1 subscripts"); // @TODO
        // {
        //     ra::Small<complex, 4> a = { 1, 2, 3, 4 };
        //     ra::Small<int, 4> i = { 3, 2, 1, 0 };
        //     cout << a(i) << endl;
        // }
    }
    section("custom strides. List init is always row-major.");
    {
        ra::SmallArray<real, mp::int_list<2, 3>, mp::int_list<1, 2>> a { 1, 2, 3, 4, 5, 6 };
        tr.test_equal(1, a(0, 0));
        tr.test_equal(2, a(0, 1));
        tr.test_equal(3, a(0, 2));
        tr.test_equal(4, a(1, 0));
        tr.test_equal(5, a(1, 1));
        tr.test_equal(6, a(1, 2));
        using dim1 = std::array<ra::dim_t, 1>;
        cout << "sizes of a(0): " << rawp(mp::tuple_copy<decltype(a(0))::sizes, dim1>::f()) << endl;
        cout << "strides of a(0): " << rawp(mp::tuple_copy<decltype(a(0))::strides, dim1>::f()) << endl;
        tr.test_equal(1, a(0)(0));
        tr.test_equal(2, a(0)(1));
        tr.test_equal(3, a(0)(2));
        tr.test_equal(4, a(1)(0));
        tr.test_equal(5, a(1)(1));
        tr.test_equal(6, a(1)(2));
    }

    section("SmallArray converted to SmallSlice");
    {
        ra::Small<real, 2, 3> a { 1, 2, 3, 4, 5, 6 };
        ra::SmallSlice<real, mp::int_list<2, 3>, mp::int_list<3, 1>> b = a();
        tr.test_equal(a, b);
// non-default strides
        ra::SmallArray<real, mp::int_list<2, 3>, mp::int_list<1, 2>> ax { 1, 2, 3, 4, 5, 6 };
        ra::SmallSlice<real, mp::int_list<2, 3>, mp::int_list<1, 2>> bx = ax();
        tr.test_equal(a, ax);
        tr.test_equal(a, bx);
        bx = 77.;
        tr.test_equal(77., ax);
        b = 99.;
        tr.test_equal(99., a);
    }

    section("using ra_iterator with SmallBase");
    {
        cout << "@TODO" << endl;
    }

    section("expr with Small, rank 1, ply_index");
    {
        ra::Small<real, 3> a { 1, 4, 2 };
        tr.test_equal(3, a.iter().size(0));
#define TEST(plier)                                                     \
        {                                                               \
            real s = 0;                                                 \
            plier(ra::expr([&s](real & a) { s += a; }, a.iter()));   \
            tr.test_equal(7, s);                                       \
        }
        TEST(ply_ravel)
        TEST(ply_index)
#undef TEST
    }

    section("expr with Small, rank 2");
    {
        ra::Small<real, 3, 2> a { 1, 4, 2, 5, 3, 6 };
        tr.test_equal(3, a.iter().size(0));
        tr.test_equal(2, a.iter().size(1));
#define TEST(plier)                                                     \
        {                                                               \
            real s = 0;                                                 \
            plier(ra::expr([&s](real & a) { s += a; }, a.iter()));   \
            tr.test_equal(21, s);                                      \
        }
        TEST(ply_ravel);
        TEST(ply_index);
#undef TEST
#define TEST(plier)                                                     \
        {                                                               \
            ra::Small<real, 3, 2> b;                                    \
            plier(ra::expr([](real & a, real & b) { b = -a; }, a.iter(), b.iter())); \
            tr.test_equal(-1, b(0, 0));                                \
            tr.test_equal(-4, b(0, 1));                                \
            tr.test_equal(-2, b(1, 0));                                \
            tr.test_equal(-5, b(1, 1));                                \
            tr.test_equal(-3, b(2, 0));                                \
            tr.test_equal(-6, b(2, 1));                                \
        }
        TEST(ply_ravel);
        TEST(ply_index);
#undef TEST
    }

    section("Small as value type in var-size array");
    {
        {
// This pain with rank 0 arrays and ra::scalar can be avoided with ply; see e.g. grid_interp_n() in src/grid.C.
            ra::Unique<ra::Small<real, 2>, 1> b({4}, ra::scalar(ra::Small<real, 2> { 3., 1. }));
            tr.test_equal(3., b(0)(0));
            tr.test_equal(1., b(0)(1));

// if () returns rank 0 instead of scalar, otherwise ct error.
            // b(1) = ra::scalar(ra::Small<real, 2> { 7., 9. });
            // cout << b << endl;
// if () returns scalar instead of rank 0, otherwise bug. (This is what happens).
            b(1) = ra::Small<real, 2> { 7., 9. };
            tr.test_equal(3., b(0)(0));
            tr.test_equal(1., b(0)(1));
            tr.test_equal(7., b(1)(0));
            tr.test_equal(9., b(1)(1));
        }
        {
            ra::Unique<real, 1> b({2}, { 3., 1. });
            tr.test_equal(3., b(0));
            tr.test_equal(1., b(1));
            b = ra::Small<real, 2> { 7., 9. };
            cout << b << endl;
            tr.test_equal(7., b(0));
            tr.test_equal(9., b(1));
        }
        {
            ra::Unique<real, 2> b({2, 2}, { 3., 1., 3., 1. });
            b(1) = ra::Small<real, 2> { 7., 9. };
            tr.test_equal(3., b(0, 0));
            tr.test_equal(1., b(0, 1));
            tr.test_equal(7., b(1, 0));
            tr.test_equal(9., b(1, 1));
        }
        {
            ra::Unique<ra::Small<real, 2>, 0> b(ra::scalar(ra::Small<real, 2>{3., 1.}));
            b = ra::scalar(ra::Small<real, 2> { 7., 9. });
            tr.test_equal(7., b()(0));
            tr.test_equal(9., b()(1));
        }
        {
            ra::Unique<ra::Small<real, 2>, 1> b({4}, ra::scalar(ra::Small<real, 2> { 3., 1. }));
            ra::Small<real, 2> u = b(1);
            tr.test_equal(3, u[0]);
            tr.test_equal(1, u[1]);
            ra::Small<real, 2> v(b(1));
            tr.test_equal(3, v[0]);
            tr.test_equal(1, v[1]);
        }
    }

    section("transpose");
    {
        ra::Small<real, 2, 3> a { 1, 2, 3, 4, 5, 6 };
        tr.test_equal(ra::Small<real, 3, 2> { 1, 4, 2, 5, 3, 6 }, transpose(a));
        transpose(a) = { 1, 2, 3, 4, 5, 6 };
        tr.test_equal(ra::Small<real, 2, 3> { 1, 3, 5, 2, 4, 6 }, a);
    }

    section("diag"); // @TODO merge with transpose.
    {
        ra::Small<real, 3, 3> a = ra::_0*3 + ra::_1;
        tr.test_equal(ra::Small<real, 3> { 0, 4, 8 }, diag(a));
        diag(a) = { 11, 22, 33 };
        tr.test_equal(ra::Small<real, 3, 3> { 11, 1, 2, 3, 22, 5, 6, 7, 33 }, a);
    }

    section(".back()");
    {
        ra::Small<real, 3> a = ra::_0*3;
        tr.test_equal(0, a[0]);
        tr.test_equal(3, a[1]);
        tr.test_equal(6, a[2]);
        tr.test_equal(6, a.back());
    }

// @TODO Replace with uniform subscripting (ra::jvec).
    section("compile time subscripting of ra::Small (as)");
    {
        auto test_as = [&tr](auto && a, auto && b)
        {
            tr.test_equal(2, b.size());
            tr.test_equal(1, b[0]);
            tr.test_equal(2, b[1]);
            b = { 7, 8 };
            tr.test_equal(7, a[0]);
            tr.test_equal(8, a[1]);
            tr.test_equal(3, a[2]);
        };
        {
            ra::Small<real, 3> a = { 1, 2, 3 };
            test_as(a, a.as<2>());
            ra::Small<real, 6> b = { 1, 99, 2, 99, 3, 99 };
            ra::SmallSlice<real, mp::int_list<3>, mp::int_list<2> > c(b.data()); // @TODO no syntax yet.
            test_as(c, c.as<2>());
        }
        auto test_fra = [&tr](auto && a, auto && b)
        {
            tr.test_equal(2, b.size());
            tr.test_equal(2, b[0]);
            tr.test_equal(3, b[1]);
            b = { 7, 8 };
            tr.test_equal(1, a[0]);
            tr.test_equal(7, a[1]);
            tr.test_equal(8, a[2]);
        };
        {
            ra::Small<real, 3> a = { 1, 2, 3 };
            test_fra(a, a.as<2, 1>());
            ra::Small<real, 6> b = { 1, 99, 2, 99, 3, 99 };
            ra::SmallSlice<real, mp::int_list<3>, mp::int_list<2> > c(b.data()); // @TODO no syntax yet.
            test_fra(c, c.as<2, 1>());
        }
        auto test_fra_rank_2 = [&tr](auto && a, auto && b)
        {
            tr.test_equal(2, b.size(0));
            tr.test_equal(2, b.size(1));
            tr.test_equal(ra::Small<real, 2, 2> { 3, 4, 5, 6 }, b);
            b = ra::Small<real, 2, 2> { 13, 14, 15, 16 };
            tr.test_equal(ra::Small<real, 3, 2> { 1, 2, 13, 14, 15, 16 }, a);
        };
        {
            ra::Small<real, 3, 2> a = { 1, 2, 3, 4, 5, 6 };
            test_fra_rank_2(a, a.as<2, 1>());
            ra::Small<real, 6, 2> b = { 1, 2, 99, 99, 3, 4, 99, 99, 5, 6, 99, 99 };
            ra::SmallSlice<real, mp::int_list<3, 2>, mp::int_list<4, 1> > c(b.data()); // @TODO no syntax yet.
            test_fra_rank_2(c, c.as<2, 1>());
        }
    }

    section("a demo on rank1of1 vs rank2 [ref01]");
    {
// by prefix matching, first dim is 2 for both so they get matched. Then {1 2}
// (a 'scalar') gets matched to 10 & 20 in succesion. This is forbidden
// expressly in Small::Small(X && x).
        ra::Small<ra::Small<real, 2>, 2> a = { {1, 2}, {3, 4} };
        ra::Small<real, 2, 2> b = { 10, 20, 30, 40 };
        cout << "a: " << a << endl;
        cout << "b: " << b << endl;
        // a = b; // @TODO Check that this static fails
        cout << "a = b, a: " << a << endl;
    }

    return tr.summary();
}
