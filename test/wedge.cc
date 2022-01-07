// -*- mode: c++; coding: utf-8 -*-
/// @file wedge-product.cc
/// @brief Test generic wedge product with compile-time dimensions.

// (c) Daniel Llorens - 2008-2010, 2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;
using ra::mp::Wedge, ra::mp::hodgex, ra::mp::int_list;

using real = double;
using complex = std::complex<double>;

real const GARBAGE(99);
template <class T, ra::dim_t N> using vec = ra::Small<T, N>;
using real1 = vec<real, 1>;
using real2 = vec<real, 2>;
using real3 = vec<real, 3>;
using real4 = vec<real, 4>;
using real6 = vec<real, 6>;
using complex1 = vec<complex, 1>;
using complex2 = vec<complex, 2>;
using complex3 = vec<complex, 3>;

template <class P, class Plist, int w, int s>
struct FindCombinationTester
{
    using finder = ra::mp::FindCombination<P, Plist>;
    static_assert(finder::where==w && finder::sign==s, "bad");
    static void check() {};
};

template <int N, int O>
void test_optimized_hodge_aux(TestRecorder & tr)
{
    if constexpr (O<=N) {
        tr.section(ra::format("hodge() vs hodgex() with N=", N, " O=", O));
        static_assert(N>=O, "bad_N_or_bad_O");
        using Va = vec<real, Wedge<N, O, N-O>::Na>;
        using Vb = vec<real, Wedge<N, O, N-O>::Nb>;
        Va u = ra::iota(u.size(), 1);
        Vb w(GARBAGE);
        hodge<N, O>(u, w);
        cout << "-> " << u << " hodge " << w << endl;
// this is the property that u^(*u) = dot(u, u)*vol form.
        if (O==1) {
            real S = sum(sqr(u));
// since the volume form and the 1-forms are always ordered lexicographically (0 1 2...) vs (0) (1) (2) ...
            tr.info("with O=1, S: ", S, " vs wedge(u, w): ", ra::wedge<N, O, N-O>(u, w))
                .test_eq(S, ra::wedge<N, O, N-O>(u, w));
        } else if (O+1==N) {
            real S = sum(sqr(w));
// compare with the case above, this is the sign of the (anti)commutativity of the exterior product.
            S *= odd(O*(N-O)) ? -1 : +1;
            tr.info("with O=N-1, S: ", S, " vs wedge(u, w): ", ra::wedge<N, N-O, O>(u, w))
                .test_eq(S, ra::wedge<N, N-O, O>(u, w));
        }
// test that it does the same as hodgex().
        Vb x(GARBAGE);
        hodgex<N, O>(u, x);
        if (2*O==N) {
            tr.info("-> ", u, " hodgex ", x).test_eq(ra::wedge<N, O, N-O>(u, w), ra::wedge<N, O, N-O>(u, x));
        }
// test basic duality property, **w = (-1)^{o(n-o)} w.
        {
            Va b(GARBAGE);
            hodgex<N, N-O>(x, b);
            tr.info("duality test with hodgex() (N ", N, " O ", O, ") -> ", u, " hodge ", x, " hodge(hodge) ", b)
                .test_eq((odd(O*(N-O)) ? -1 : +1)*u, b);
        }
        {
            Va a(GARBAGE);
            hodge<N, N-O>(w, a);
            tr.info("duality test with hodge()  (N ", N, " O ", O, ") -> ", u, " hodge ", w, " hodge(hodge) ", a)
                .test_eq((odd(O*(N-O)) ? -1 : +1)*u, a);
        }
        test_optimized_hodge_aux<N, O+1>(tr);
    }
}

template <int N>
void test_optimized_hodge(TestRecorder & tr)
{
    static_assert(N>=0, "bad_N");
    test_optimized_hodge_aux<N, 0>(tr);
    test_optimized_hodge<N-1>(tr);
}

template <>
void test_optimized_hodge<-1>(TestRecorder & tr)
{
}

template <int D, class R, class A, class B>
R test_scalar_case(A const & a, B const & b)
{
    R r = ra::wedge<D, 0, 0>(a, b);
    cout << "[" << D << "/0/0] " << a << " ^ " << b << " -> " << r << endl;
    return r;
}

template <int D, int OA, int OB, class R, class A, class B>
R test_one_one_case(TestRecorder & tr, A const & a, B const & b)
{
    R r1(GARBAGE);
    Wedge<D, OA, OB>::product(a, b, r1);
    cout << "[" << D << "/" << OA << "/" << OB << "] " << a << " ^ " << b << " -> " << r1 << endl;
    R r2(ra::wedge<D, OA, OB>(a, b));
    cout << "[" << D << "/" << OA << "/" << OB << "] " << a << " ^ " << b << " -> " << r2 << endl;
    tr.test_eq(r1, r2);
    return r1;
}

template <int D, int OA, int OB, class R, class A, class B>
R test_one_scalar_case(A const & a, B const & b)
{
    R r2(ra::wedge<D, OA, OB>(a, b));
    cout << "[" << D << "/" << OA << "/" << OB << "] " << a << " ^ " << b << " -> " << r2 << endl;
    return r2;
}

int main()
{
    TestRecorder tr(std::cout);
    static_assert(ra::mp::n_over_p(0, 0)==1, "");
    tr.section("Testing FindCombination");
    {
        using la = ra::mp::iota<3>;
        using ca = ra::mp::combinations<la, 2>;
        FindCombinationTester<int_list<0, 1>, ca, 0, +1>::check();
        FindCombinationTester<int_list<1, 0>, ca, 0, -1>::check();
        FindCombinationTester<int_list<0, 2>, ca, 1, +1>::check();
        FindCombinationTester<int_list<2, 0>, ca, 1, -1>::check();
        FindCombinationTester<int_list<1, 2>, ca, 2, +1>::check();
        FindCombinationTester<int_list<2, 1>, ca, 2, -1>::check();
        FindCombinationTester<int_list<0, 0>, ca, -1,  0>::check();
        FindCombinationTester<int_list<1, 1>, ca, -1,  0>::check();
        FindCombinationTester<int_list<2, 2>, ca, -1,  0>::check();
        FindCombinationTester<int_list<3, 0>, ca, -1,  0>::check();
    }

    tr.section("Testing AntiCombination");
    {
        using la = ra::mp::iota<3>;
        using ca = ra::mp::combinations<la, 1>;
        using cc0 = ra::mp::AntiCombination<ra::mp::ref<ca, 0>, 3>::type;
        static_assert(ra::mp::check_idx<cc0, 1, 2>::value, "bad");
        using cc1 = ra::mp::AntiCombination<ra::mp::ref<ca, 1>, 3>::type;
        static_assert(ra::mp::check_idx<cc1, 2, 0>::value, "bad");
        using cc2 = ra::mp::AntiCombination<ra::mp::ref<ca, 2>, 3>::type;
        static_assert(ra::mp::check_idx<cc2, 0, 1>::value, "bad");
    }

    tr.section("Testing ChooseComponents");
    {
        using c1 = ra::mp::ChooseComponents<3, 1>::type;
        static_assert(ra::mp::len<c1> == 3, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c1, 0>, 0>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c1, 1>, 1>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c1, 2>, 2>::value, "bad");
        using c2 = ra::mp::ChooseComponents<3, 2>::type;
        static_assert(ra::mp::len<c2> == 3, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c2, 0>, 1, 2>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c2, 1>, 2, 0>::value, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c2, 2>, 0, 1>::value, "bad");
        using c3 = ra::mp::ChooseComponents<3, 3>::type;
        static_assert(ra::mp::len<c3> == 1, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c3, 0>, 0, 1, 2>::value, "bad");
    }
    {
        using c0 = ra::mp::ChooseComponents<1, 0>::type;
        static_assert(ra::mp::len<c0> == 1, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c0, 0>>::value, "bad");
        using c1 = ra::mp::ChooseComponents<1, 1>::type;
        static_assert(ra::mp::len<c1> == 1, "bad");
        static_assert(ra::mp::check_idx<ra::mp::ref<c1, 0>, 0>::value, "bad");
    }

    tr.section("Testing Wedge<>::product()");
    {
        real1 a(1);
        real1 b(3);
        real1 r(GARBAGE);
        Wedge<1, 0, 0>::product(a, b, r);
        tr.info("[1/0/0] ", a, " ^ ", b, " -> ", r).test_eq(3, r[0]);
        real1 h(GARBAGE);
        hodgex<1, 0>(r, h);
        tr.info("thodge-star: ", h).test_eq(3, h[0]);
    }
    tr.section("change order changes sign");
    {
        real3 a{1, 0, 0};
        real3 b{0, 1, 0};
        real3 r(GARBAGE);
        Wedge<3, 1, 1>::product(a, b, r);
        tr.info("[3/1/1] ", a, " ^ ", b, " -> ", r).test_eq(real3{0, 0, +1}, r); // +1, 0, 0 in lex. order.
        real3 h(GARBAGE);
        hodgex<3, 2>(r, h);
        tr.info("hodge-star: ", h).test_eq(real3{0, 0, 1}, h);
    }
    {
        real3 a{0, 1, 0};
        real3 b{1, 0, 0};
        real3 r(GARBAGE);
        Wedge<3, 1, 1>::product(a, b, r);
        tr.info("[3/1/1] ", a, " ^ ", b, " -> ", r).test_eq(real3{0, 0, -1}, r); // -1, 0, 0 in lex order.
        real3 h(GARBAGE);
        hodgex<3, 2>(r, h);
        tr.info("hodge-star: ", h).test_eq(real3{0, 0, -1}, h);
    }
    tr.section("check type promotion");
    {
        complex3 a{complex(0, 1), 0, 0};
        real3 b{0, 1, 0};
        complex3 r(GARBAGE);
        Wedge<3, 1, 1>::product(a, b, r);
        tr.info("[3/1/1] ", a, " ^ ", b, " -> ", r).test_eq(complex3{0, 0, complex(0, 1)}, r); // +j, 0, 0 in lex. o.
        complex3 h(GARBAGE);
        hodgex<3, 2>(r, h);
        tr.info("hodge-star: ", h).test_eq(complex3{0, 0, complex(0, 1)}, h);
    }
    tr.section("sign change in going from lexicographic -> our peculiar order");
    {
        real3 a{1, 0, 0};
        real3 b{0, 0, 2};
        real3 r(GARBAGE);
        Wedge<3, 1, 1>::product(a, b, r);
        tr.info("[3/1/1] ", a, " ^ ", b, " -> ", r).test_eq(real3{0, -2, 0}, r); // 0, 2, 0 in lex order.
        real3 h(GARBAGE);
        hodgex<3, 2>(r, h);
        tr.info("hodge-star: ", h).test_eq(real3{0, -2, 0}, h);
    }
    {
        real3 a{1, 0, 2};
        real3 b{1, 0, 2};
        real3 r(GARBAGE);
        Wedge<3, 1, 1>::product(a, b, r);
        tr.info("[3/1/1] ", a, " ^ ", b, " -> ", r).test_eq(0., r);
        real3 h(GARBAGE);
        hodgex<3, 2>(r, h);
        tr.info("hodge-star: ", h).test_eq(0., h);
    }
    {
        real3 a{0, 1, 0};
        real3 b{0, -1, 0};  // 0, 1, 0 in lex order.
        real1 r(GARBAGE);
        Wedge<3, 1, 2>::product(a, b, r);
        tr.info("[3/1/2] ", a, " ^ ", b, " -> ", r).test_eq(-1, r[0]);
        real1 h(GARBAGE);
        hodgex<3, 3>(r, h);
        tr.info("\thodge-star: ", h).test_eq(-1, h[0]);
// this is not forced for hodgex (depends on vec::ChooseComponents<> as used in Wedge<>) so if you change that, change this too.
        real3 c;
        hodgex<3, 1>(b, c);
        tr.info("hodge<3, 1>(", b, "): ", c).test_eq(real3{0, -1, 0}, b);
        hodgex<3, 2>(b, c);
        tr.info("hodge<3, 2>(", b, "): ", c).test_eq(real3{0, -1, 0}, b);
    }
    {
        real4 a{1, 0, 0, 0};
        real4 b{0, 0, 1, 0};
        real6 r(GARBAGE);
        Wedge<4, 1, 1>::product(a, b, r);
        tr.info("[4/1/1] ", a, " ^ ", b, " -> ", r).test_eq(real6{0, 1, 0, 0, 0, 0}, r);
        real6 h(GARBAGE);
        hodgex<4, 2>(r, h);
        tr.info("hodge-star: ", h).test_eq(real6{0, 0, 0, 0, -1, 0}, h);
        r = GARBAGE;
        hodgex<4, 2>(h, r);
        tr.info("hodge-star(hodge-star()): ", r).test_eq(real6{0, 1, 0, 0, 0, 0}, r);
    }
    {
        real4 a{0, 0, 1, 0};
        real4 b{1, 0, 0, 0};
        real6 r(GARBAGE);
        Wedge<4, 1, 1>::product(a, b, r);
        tr.info("[4/1/1] ", a, " ^ ", b, " -> ", r).test_eq(real6{0, -1, 0, 0, 0, 0}, r);
    }
    {
        real6 r{1, 0, 0, 0, 0, 0};
        real6 h(GARBAGE);
        hodgex<4, 2>(r, h);
        tr.info("r: ", r, " -> hodge-star: ", h).test_eq(real6{0, 0, 0, 0, 0, 1}, h);
    }
    tr.section("important as a case where a^b==b^a");
    {
        real6 a{1, 0, 0, 0, 0, 0};
        real6 b{0, 0, 0, 0, 0, 1};
        real1 r(GARBAGE);
        Wedge<4, 2, 2>::product(a, b, r);
        tr.info("[4/2/2] ", a, " ^ ", b, " -> ", r).test_eq(1, r[0]);
        Wedge<4, 2, 2>::product(b, a, r);
        tr.info("[4/2/2] ", a, " ^ ", b, " -> ", r).test_eq(1, r[0]);
    }
    tr.section("important as a case where a^a!=0, see DoCarmo1994, Ch. 1 after Prop. 2.");
    {
        real6 a{1, 0, 0, 0, 0, 1};
        real6 b{1, 0, 0, 0, 0, 1};
        real1 r(GARBAGE);
        Wedge<4, 2, 2>::product(a, b, r);
        tr.info("[4/2/2] ", a, " ^ ", b, " -> ", r).test_eq(2, r[0]);
    }
    tr.section("important as a case where a^b is not dot(a, b) even though O(a)=D-O(b). This happens when O(a)==O(b), i.e. they have the same components");
    {
        real2 a{1, 0};
        real2 b{0, 1};
        real1 r(GARBAGE);
        Wedge<2, 1, 1>::product(a, b, r);
        tr.info("[2/1/1] ", a, " ^ ", b, " -> ", r).test_eq(1, r[0]);
        real2 p{1, 2};
        real2 q(GARBAGE);
        hodgex<2, 1>(p, q);
        tr.info("p: ", p, " -> hodge-star: ", q).test_eq(real2{-2, 1}, q);
    }
    tr.section("test the specializations in cross(), wedge<>()");
    {
        real2 a{1, 0};
        real2 b{0, 1};
        real c(cross(a, b));
        tr.info("a cross b: ", c).test_eq(1, c);
        c = cross(b, a);
        tr.test_eq(-1, c);
// accepts expr arguments.
        c = cross(a, b+1.);
        tr.test_eq(2, c);
    }
    tr.section("test the cross product some more");
    {
        real3 x3{1., 0. ,0.};
        real3 y3{0., 1., 0.};
        real3 z3{0., 0., 1.};
        tr.test_eq(z3, cross(x3, y3));
        tr.test_eq(x3, cross(y3, z3));
        tr.test_eq(y3, cross(z3, x3));
        tr.test_eq(-z3, cross(y3, x3));
        tr.test_eq(-x3, cross(z3, y3));
        tr.test_eq(-y3, cross(x3, z3));
        real2 x2{1., 0.};
        real2 y2{0., 1.};
        tr.test_eq(1., cross(x2, y2));
        tr.test_eq(-1., cross(y2, x2));
        complex2 cy2{0., 1.};
        tr.test_eq(complex(1., 0.), cross(x2, cy2));
    }
    tr.section("verify that wedge<>() returns an expression where appropriate");
    {
        real3 u{1., 2., 3.};
        real3 v{3., 2., 1.};
        tr.test_eq(10., ra::wedge<3, 1, 2>(u, v));
        tr.test_eq(cross(u, v), ra::wedge<3, 1, 1>(u, v));
        tr.test_eq(10., ra::wedge<3, 1, 2>(u, v));
    }
    tr.section("verify that we are allowed to choose our return type to wedge<>(a, b, r)");
    {
        real a(GARBAGE);
        real1 b(GARBAGE);
        ra::wedge<2, 1, 1>(real2{1, 0}, real2{0, 1}, a);
        ra::wedge<2, 1, 1>(real2{1, 0}, real2{0, 1}, b);
        tr.test_eq(1, a);
        tr.test_eq(1, b[0]);
    }
    tr.section("check the optimization of hodgex() that relies on a complementary order of bases in the 2*O>D forms");
    {
        test_optimized_hodge<6>(tr);
    }
    tr.section("Test scalar arg cases");
    {
        tr.test_eq(6, test_scalar_case<0, real>(real1(2), real(3)));
        tr.test_eq(6, test_scalar_case<1, real>(real1(2), real(3)));
        tr.test_eq(6, test_scalar_case<0, real>(real(2), real(3)));
        tr.test_eq(6, test_scalar_case<1, real>(real(2), real(3)));
        tr.test_eq(6, test_scalar_case<0, real>(real(2), real1(3)));
        tr.test_eq(6, test_scalar_case<1, real>(real(2), real1(3)));
        tr.test_eq(6, test_scalar_case<0, real>(real1(2), real1(3)));
        tr.test_eq(6, test_scalar_case<1, real>(real1(2), real1(3)));
        tr.test_eq(6, test_scalar_case<0, real1>(real(2), real(3)));
        tr.test_eq(6, test_scalar_case<1, real1>(real(2), real(3)));
        tr.test_eq(6, test_scalar_case<0, real1>(real1(2), real(3)));
        tr.test_eq(6, test_scalar_case<1, real1>(real1(2), real(3)));
        tr.test_eq(6, test_scalar_case<0, real1>(real(2), real1(3)));
        tr.test_eq(6, test_scalar_case<1, real1>(real(2), real1(3)));
        tr.test_eq(6, test_scalar_case<0, real1>(real1(2), real1(3)));
        tr.test_eq(6, test_scalar_case<1, real1>(real1(2), real1(3)));
    }
    tr.section("Test scalar x nonscalar arg cases.");
    {
        tr.test_eq(real2{6, 10}, test_one_one_case<2, 0, 1, real2>(tr, real1(2), real2{3, 5}));
        tr.test_eq(real2{6, 10}, test_one_one_case<2, 1, 0, real2>(tr, real2{3, 5}, real1(2)));
        tr.test_eq(real3{2, 6, 10}, test_one_one_case<3, 0, 1, real3>(tr, real1(2), real3{1, 3, 5}));
        tr.test_eq(real3{2, 6, 10}, test_one_one_case<3, 1, 0, real3>(tr, real3{1, 3, 5}, real1(2)));
    }
    {
        tr.test_eq(real2{6, 10}, test_one_scalar_case<2, 0, 1, real2>(real1(2), real2{3, 5}));
        tr.test_eq(real2{6, 10}, test_one_scalar_case<2, 1, 0, real2>(real2{3, 5}, real1(2)));
        tr.test_eq(real3{2, 6, 10}, test_one_scalar_case<3, 0, 1, real3>(real1(2), real3{1, 3, 5}));
        tr.test_eq(real3{2, 6, 10}, test_one_scalar_case<3, 1, 0, real3>(real3{1, 3, 5}, real1(2)));
    }
    tr.section("Test scalar x ~scalar arg cases.");
    {
        tr.test_eq(6., ra::wedge<1, 0, 1>(3., complex1(2.)));
    }
    return tr.summary();
}
