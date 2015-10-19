
// (c) Daniel Llorens - 2014-2015

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

/// @file test-ra-operators.C
/// @brief Tests for operators on ra:: expr templates.

#include "ra/complex.H"
#include "ra/test.H"
#include "ra/mpdebug.H"
#include "ra/ra-operators.H"
#include "ra/ra-large.H"
#include "ra/wedge.H"

using std::cout; using std::endl;

int main()
{
    TestRecorder tr;

    section("unary ops");
    {
#define DEF_TEST_UNARY_OP(OP)                                           \
        auto test = [&tr](auto token, auto x, auto y, auto && vx, auto && vy) \
            {                                                           \
                using T = decltype(token);                              \
                using TY = decltype(OP(std::declval<T>()));             \
                tr.info("scalar-scalar").test_abs_error(OP(T(x)), TY(y), 0.); \
                tr.info("array(0)-scalar").test_abs_error(OP(ra::Unique<T, 0>(x)), TY(y), 0.); \
                tr.info("array(var)-scalar").test_abs_error(OP(ra::Unique<T>(x)), TY(y), 0.); \
                tr.info("array(1)-array(1)").test_abs_error(OP(vx), vy, 0.); \
            };
        {
            DEF_TEST_UNARY_OP(abs);
            test(int(), -3, 3, ra::Unique<int, 1>{1, -3, -2}, ra::Unique<int, 1>{1, 3, 2});
            test(real(), -3, 3, ra::Unique<real, 1>{1, -3, -2}, ra::Unique<real, 1>{1, 3, 2});
            test(float(), -3, 3, ra::Unique<float, 1>{1, -3, -2}, ra::Unique<float, 1>{1, 3, 2});
            test(complex(), -3, 3, ra::Unique<complex, 1>{1, -3, -2}, ra::Unique<complex, 1>{1, 3, 2});
        }
#define TEST_UNARY_OP_CR(OP, ri, ro, ci, co)                            \
        {                                                               \
            DEF_TEST_UNARY_OP(OP);                                      \
            test(real(), ri, ro, ra::Unique<real, 1>{ri, ri, ri}, ra::Unique<complex, 1>{ro, ro, ro}); \
            test(complex(), ci, co, ra::Unique<complex, 1>{ci, ci}, ra::Unique<complex, 1>{co, co}); \
        }
        TEST_UNARY_OP_CR(conj, 1., 1., complex(1., 2.), complex(1., -2));
        TEST_UNARY_OP_CR(cos, 0., 1., complex(0, 0), complex(1., 0.));
        TEST_UNARY_OP_CR(sin, 1.57079632679489661, 1., complex(1.57079632679489661, 0), complex(1., 0.));
        TEST_UNARY_OP_CR(exp, 0., 1., complex(0, 0), complex(1., 0.));
        TEST_UNARY_OP_CR(sqrt, 4., 2., complex(-1, 0), complex(0., 1.));
        TEST_UNARY_OP_CR(xI, 4., complex(0, 4.), complex(1., -2.), complex(2., 1.));
#undef TEST_UNARY_OP_CR
#undef DEF_TEST_UNARY_OP
    }

    section("establish meaning of selectors");
    {
// rank 0 containers/slices are not is_slice (and therefore not is_ra) so that their conversions to scalar are used instead.
        static_assert(ra::is_ra<ra::Small<int>>::value, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallSlice<int, mp::nil, mp::nil>>::value, "bad is_ra SmallSlice");
        static_assert(ra::is_ra<ra::Unique<int, 0>>::value, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::Raw<int, 0>>::value, "bad is_ra Raw");

        static_assert(ra::is_ra<ra::Small<int, 1>>::value, "bad is_ra Small");
        static_assert(ra::is_ra<ra::SmallSlice<int, mp::int_list<1>, mp::int_list<1>>>::value, "bad is_ra SmallSlice");
        static_assert(ra::is_ra<ra::Unique<int, 1>>::value, "bad is_ra Unique");
        static_assert(ra::is_ra<ra::Raw<int, 1>>::value, "bad is_ra Raw");
        static_assert(ra::is_ra<ra::Raw<int>>::value, "bad is_ra Raw");

        static_assert(ra::is_ra<decltype(ra::scalar(3))>::value, "bad is_ra Scalar");
        static_assert(ra::is_ra<decltype(ra::vector({1, 2, 3}))>::value, "bad is_ra Vector");
        static_assert(!ra::is_ra<int *>::value, "bad is_ra int *");

        static_assert(ra::is_scalar<real>::value, "bad is_scalar real");
        static_assert(ra::is_scalar<complex>::value, "bad is_scalar complex");
        static_assert(ra::is_scalar<int>::value, "bad is_scalar int");

        static_assert(!ra::is_scalar<decltype(ra::scalar(3))>::value, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<decltype(ra::vector({1, 2, 3}))>::value, "bad is_scalar Scalar");
        static_assert(!ra::is_scalar<decltype(ra::start(3))>::value, "bad is_scalar Scalar");
        int a = 3;
        static_assert(!ra::is_scalar<decltype(ra::start(a))>::value, "bad is_scalar Scalar");
    }
    section("check decay of rank 0 Containers/Slices w/ operators");
    {
        {
            auto test = [&tr](auto && a)
                {
                    tr.test_equal(12, a*4.);
                    auto b = a();
                    static_assert(std::is_same<int, decltype(b)>::value, "unexpected b non-decay to real");
                    static_assert(std::is_same<decltype(b*4.), real>::value, "expected b decay to real");
                    static_assert(std::is_same<decltype(4.*b), real>::value, "expected b decay to real");
                    tr.test_equal(12., b*4.);
                    tr.test_equal(12., 4.*b);
                    static_assert(std::is_same<decltype(a*4.), real>::value, "expected a decay to real");
                    static_assert(std::is_same<decltype(4.*a), real>::value, "expected a decay to real");
                    tr.test_equal(12., a*4.);
                    tr.test_equal(12., 4.*a);
                };
            test(ra::Small<int>(3));
            test(ra::Unique<int, 0>({}, 3));
        }
        {
            ra::Small<int, 3> a { 1, 2, 3 };
            ra::Small<int> b { 5 };
            a *= b;
            tr.test_equal(a[0], 5);
            tr.test_equal(a[1], 10);
            tr.test_equal(a[2], 15);
        }
        {
            ra::Small<int> a { 3 };
            ra::Small<int> b { 2 };
            auto c = a*b;
            static_assert(std::is_same<decltype(a*b), int>::value, "expected a, b decay to real"); \
            tr.test_equal(c, 6);
        }
    }

    section("operators with Unique");
    {
        ra::Unique<int, 2> a({3, 2}, { 1, 2, 3, 20, 5, 6 });
        ra::Unique<int, 1> b({3}, { 10, 20, 30 });
#define TESTSUM(expr)                                                   \
        tr.test_equal(expr, ra::Small<int, 3, 2> {11, 12, 23, 40, 35, 36});
        TESTSUM(ra::expr([](int a, int b) { return a + b; }, a.iter(), b.iter()));
        TESTSUM(a.iter() + b.iter());
        TESTSUM(a+b);
#undef TESTSUM
#define TESTEQ(expr)                                                   \
        tr.test_equal(expr, ra::Small<bool, 3, 2> {false, false, false, true, false, false});
        TESTEQ(a==b);
        TESTEQ(!(a!=b));
#undef TESTEQ
    }

    section("operators with Raw");
    {
        {
            ra::Unique<complex, 2> const a({2, 3}, {1, 2, 3, 4, 5, 6});
            {
                auto a0 = a(0);
                tr.test_equal(ra::Small<real, 3>{.5, 1., 1.5}, 0.5*a0);
            }
            {
                auto a0 = a.at(ra::Small<int, 1> { 0 }); // @BUG Not sure this is what I want
                tr.test_equal(ra::Small<real, 3>{.5, 1., 1.5}, 0.5*a0);
            }
        }
        {
            ra::Unique<complex, 1> const a({3}, {1, 2, 3});
            {
                auto a0 = a(0);
                tr.test_equal(0.5, 0.5*a0);
            }
            {
                auto a0 = a.at(ra::Small<int, 1> { 0 }); // @BUG Not sure this is what I want, see above
                tr.test_equal(2.1, 2.1*a0);
                tr.test_equal(0.5, 0.5*a0);
                tr.test_equal(0.5, complex(0.5)*a0);
            }
        }
    }

    section("operators with Small");
    {
        ra::Small<int, 3> a { 1, 2, 3 };
        ra::Small<int, 3> b { 1, 2, 4 };
        tr.test_equal(ra::Small<int, 3> {2, 4, 7}, ra::expr([](int a, int b) { return a + b; }, a.iter(), b.iter()));
        tr.test_equal(ra::Small<int, 3> {2, 4, 7}, (a.iter() + b.iter()));
        tr.test_equal(ra::Small<int, 3> {2, 4, 7}, a+b);
    }

    section("constructors from expr"); // @TODO For all other Container types.
    {
        {
// @TODO Systematic init-from-expr tests (every expr type vs every container type) with ra-operators.H included.
            ra::Unique<int, 1> a({3}, { 1, 2, 3 });
            ra::Unique<int, 1> b({3}, { 10, 20, 30 });
            ra::Unique<int, 1> c(a.iter() + b.iter());
            tr.test_equal(ra::Small<int, 3> {11, 22, 33}, c);
        }
        {
            ra::Unique<int, 2> a({3, 2}, 77);
            tr.test_equal(a, ra::Small<int, 3, 2> {77, 77, 77, 77, 77, 77});
        }
        {
            ra::Unique<int, 2> a({3, 2}, ra::cast<int>(ra::TensorIndex<0>()-ra::TensorIndex<1>()));
            tr.test_equal(ra::Small<int, 3, 2> {0, -1, 1, 0, 2, 1}, a);
        }
    }

    section("mixed ra-type / foreign-scalar operations");
    {
        ra::Unique<int, 2> a({3, 2}, { 1, 2, 3, 20, 5, 6 });
        ra::Small<int, 3, 2> ref {4, 5, 6, 23, 8, 9};
        tr.test_equal(ref, ra::expr([](int a, int b) { return a + b; }, ra::start(a), ra::start(3)));
        tr.test_equal(ref, ra::start(a) + ra::start(3));
        tr.test_equal(ref, a+3);
    }
// These are rather different because they have to be defined in-class.
    section("constructors & assignment operators with expr rhs"); // @TODO use TestRecorder::test_equal().
    {
        real check0[6] = { 0, -1, 1, 0, 2, 1 };
        real check1[6] = { 4, 3, 5, 4, 6, 5 };
        real check2[6] = { 8, 6, 10, 8, 12, 10 };
        auto test = [&](auto && a)
            {
                tr.test(std::equal(a.begin(), a.end(), check0));
                a += 4;
                tr.test(std::equal(a.begin(), a.end(), check1));
                a += a;
                tr.test(std::equal(a.begin(), a.end(), check2));
            };
        test(ra::Unique<int, 2>({3, 2}, ra::cast<int>(ra::TensorIndex<0>()-ra::TensorIndex<1>())));
        test(ra::Small<int, 3, 2>(ra::cast<int>(ra::TensorIndex<0>()-ra::TensorIndex<1>())));
    }
    section("operator= for Raw, WithStorage. Also see test-ra-ownership.C"); // @TODO use TestRecorder::test_equal().
    {
        real check5[6] = { 5, 5, 5, 5, 5, 5 };
        real check9[6] = { 9, 9, 9, 9, 9, 9 };
        ra::Unique<int, 2> a({3, 2}, 7);
        ra::Unique<int, 2> b({3, 2}, 5);
        ra::Raw<int, 2> c = a();
        ra::Raw<int, 2> d = b();
        c = d;
        tr.test(std::equal(a.begin(), a.end(), check5));
        ra::Unique<int, 2> t({2, 3}, 9);
        c = transpose(t, {1, 0});
        tr.test(std::equal(a.begin(), a.end(), check9));
        a = d;
        tr.test(std::equal(a.begin(), a.end(), check5));
        {
            ra::Unique<int, 2> e = d;
            tr.test(std::equal(e.begin(), e.end(), check5));
        }
    }
    section("operator= for Dynamic");
    {
        ra::Unique<int, 1> a({7}, 7);
        ra::Small<ra::dim_t, 3> i { 2, 3, 5 };
        ra::Small<int, 3> b { 22, 33, 55 };
        ra::expr([&a](ra::dim_t i) -> decltype(auto) { return a(i); }, ra::start(i)) = b;
        int checka[] = { 7, 7, 22, 33, 7, 55, 7 };
        tr.test(std::equal(checka, checka+7, a.begin()));
    }
    section("wedge");
    {
        {
            ra::Small<real, 3> a {1, 2, 3};
            ra::Small<real, 3> b {4, 5, 7};
            ra::Small<real, 3> c;
            fun::Wedge<3, 1, 1>::product(a, b, c);
            tr.test_equal(ra::Small<real, 3> {-1, 5, -3}, c);
        }
        {
            ra::Small<real, 1> a {2};
            ra::Small<real, 1> b {3};
            ra::Small<real, 1> r;
            fun::Wedge<1, 0, 0>::product(a, b, r);
            tr.test_equal(6, r[0]);
            tr.test_equal(6, wedge<1, 0, 0>(ra::Small<real, 1>{2}, ra::Small<real, 1>{3}));
            tr.test_equal(6, wedge<1, 0, 0>(ra::Small<real, 1>{2}, 3.));
            tr.test_equal(6, wedge<1, 0, 0>(2., ra::Small<real, 1>{3}));
            tr.test_equal(6, wedge<1, 0, 0>(2., 3));
        }
    }
    section("hodge / hodgex");
    {
        ra::Small<real, 3> a {1, 2, 3};
        ra::Small<real, 3> c;
        fun::hodgex<3, 1>(a, c);
        tr.test_equal(a, c);
        auto d = fun::hodge<3, 1>(a);
        tr.test_equal(a, d);
    }

    return tr.summary();
}
