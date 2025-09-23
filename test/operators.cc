// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Tests for operators.

// (c) Daniel Llorens - 2014-2015
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include "ra/test.hh"
#include "mpdebug.hh"

using std::cout, std::endl, ra::TestRecorder;
using real = double;
using complex = std::complex<double>;

int main()
{
    TestRecorder tr;
    tr.section("[ra8]");
    {
        constexpr int z = 1 + ra::scalar(2);
        tr.test_eq(3, z);
    }
    tr.section("unary ops");
    {
#define DEF_TEST_UNARY_OP(OP)                                           \
        auto test = [&tr](auto token, auto x, auto y, auto && vx, auto && vy, real err) \
        {                                                               \
            using T = decltype(token);                                  \
            using TY = decltype(OP(std::declval<T>()));                 \
            tr.info("scalar-scalar").test_abs(OP(T(x)), TY(y), err);    \
            tr.info("array(0)-scalar").test_abs(OP(ra::Unique<T, 0>(x)), TY(y), err); \
            tr.info("array(var)-scalar").test_abs(OP(ra::Unique<T>(x)), TY(y), err); \
            tr.info("array(1)-array(1)").test_abs(OP(vx), vy, err);     \
        };
        {
            DEF_TEST_UNARY_OP(abs);
            test(int(), -3, 3, ra::Unique<int, 1>{1, -3, -2}, ra::Unique<int, 1>{1, 3, 2}, 0.);
            test(real(), -3, 3, ra::Unique<real, 1>{1, -3, -2}, ra::Unique<real, 1>{1, 3, 2}, 0.);
            test(float(), -3, 3, ra::Unique<float, 1>{1, -3, -2}, ra::Unique<float, 1>{1, 3, 2}, 0.);
            test(complex(), -3, 3, ra::Unique<complex, 1>{1, -3, -2}, ra::Unique<complex, 1>{1, 3, 2}, 0.);
        }
#define TEST_UNARY_OP_CR(OP, ri, ro, ci, co, err)                       \
        {                                                               \
            DEF_TEST_UNARY_OP(OP);                                      \
            test(real(), ri, ro, ra::Unique<real, 1>{ri, ri, ri}, ra::Unique<complex, 1>{ro, ro, ro}, err); \
            test(complex(), ci, co, ra::Unique<complex, 1>{ci, ci}, ra::Unique<complex, 1>{co, co}, err); \
        }
        TEST_UNARY_OP_CR(conj, 1., 1., complex(1., 2.), complex(1., -2), 0.);
        TEST_UNARY_OP_CR(cos, 0., 1., complex(0, 0), complex(1., 0.), 0.);
        TEST_UNARY_OP_CR(sin, 1.57079632679489661, 1., complex(1.57079632679489661, 0), complex(1., 0.), 0.);
        TEST_UNARY_OP_CR(exp, 0., 1., complex(0, 0), complex(1., 0.), 0.);
        TEST_UNARY_OP_CR(sqrt, 4., 2., complex(-1, 0), complex(0., 1.), 1e-16);
        TEST_UNARY_OP_CR(ra::xi, 4., complex(0, 4.), complex(1., -2.), complex(2., 1.), 0.);
#undef TEST_UNARY_OP_CR
#undef DEF_TEST_UNARY_OP
// TODO merge with DEF_TEST_UNARY_OP
        tr.info("odd").test_eq(ra::Unique<bool, 1> {true, false, true, true}, odd(ra::Unique<int, 1> {1, 2, 3, -1}));
    }
    tr.section("binary ops");
    {
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96278
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wzero-as-null-pointer-constant"
        tr.info("<=> a").test_eq(true, (2<=>1)>0);
        tr.info("<=> b").test_eq(ra::ptr((char const *)"+0-"),
                                 map([](auto z){ return z>0 ? '+' : z<0 ? '-' : '0'; },
                                     ra::Small<int, 3>{3, 4, 5} <=> ra::Small<double, 3>{2., 4., 6.}));
#pragma GCC diagnostic pop
    }

    tr.section("check decay of rank 0 Containers/Slices w/ operators");
    {
        {
            auto test = [&tr](auto && a){
                tr.test_eq(12, a*4.);
                auto b = a();
                static_assert(std::is_same_v<int, decltype(b)>, "unexpected b non-decay to real");
                static_assert(std::is_same_v<real, decltype(b*4.)>, "expected b decay to real");
                static_assert(std::is_same_v<real, decltype(4.*b)>, "expected b decay to real");
                tr.test_eq(12., b*4.);
                tr.test_eq(12., 4.*b);
                static_assert(std::is_same_v<real, decltype(a*4.)>, "expected a decay to real");
                static_assert(std::is_same_v<real, decltype(4.*a)>, "expected a decay to real");
                tr.test_eq(12., a*4.);
                tr.test_eq(12., 4.*a);
            };
            test(ra::Small<int>(3));
            test(ra::Unique<int, 0>({}, 3));
        }
        {
            ra::Small<int, 3> a { 1, 2, 3 };
            ra::Small<int> b { 5 };
            a *= b;
            tr.test_eq(a[0], 5);
            tr.test_eq(a[1], 10);
            tr.test_eq(a[2], 15);
        }
        {
            ra::Small<int> a { 3 };
            ra::Small<int> b { 2 };
            auto c = a*b;
            static_assert(std::is_same_v<int, decltype(a*b)>, "expected a, b decay to real"); \
            tr.test_eq(c, 6);
        }
    }

    tr.section("lvalue-rvalue operators I");
    {
        ra::Unique<complex, 1> a({3}, 0.);
        imag_part(a) = ra::Unique<real, 1> { 7., 2., 3. }; // TODO operator=(initializer_list) ?
        real_part(a) = -imag_part(ra::Unique<complex, 1> { ra::xi(7.), ra::xi(2.), ra::xi(3.) })+1;
        tr.test_eq(ra::Unique<complex, 1> {{-6., 7.}, {-1., 2.}, {-2., 3.}}, a);
    }
    tr.section("lvalue-rvalue operators II [ma115]");
    {
        ra::Small<std::complex<double>, 2, 2> A = {{1., 2.}, {3., 4.}};
        imag_part(A) = -2*real_part(A);
        cout << A << endl;
        tr.test_eq(ra::Small<std::complex<double>, 2, 2> {{{1., -2.}, {2., -4.}}, {{3., -6.}, {4, -8.}}}, A);
    }
    tr.section("operators with Unique");
    {
        ra::Unique<int, 2> a({3, 2}, { 1, 2, 3, 20, 5, 6 });
        ra::Unique<int, 1> b({3}, { 10, 20, 30 });
#define TESTSUM(arg)                                                   \
        tr.test_eq(arg, ra::Small<int, 3, 2> {11, 12, 23, 40, 35, 36});
        TESTSUM(ra::map_([](int a, int b){ return a + b; }, a.iter(), b.iter()));
        TESTSUM(a.iter() + b.iter());
        TESTSUM(a+b);
#undef TESTSUM
#define TESTEQ(arg)                                                   \
        tr.test_eq(arg, ra::Small<bool, 3, 2> {false, false, false, true, false, false});
        TESTEQ(a==b);
        TESTEQ(!(a!=b));
#undef TESTEQ
    }

    tr.section("operators with View");
    {
        {
            ra::Unique<complex, 2> const a({2, 3}, {1, 2, 3, 4, 5, 6});
            {
                auto a0 = a(0);
                tr.test_eq(ra::Small<real, 3>{.5, 1., 1.5}, 0.5*a0);
            }
            {
                auto a0 = at_view(a, ra::Small<int, 1> { 0 }); // BUG Not sure this is what I want
                tr.test_eq(ra::Small<real, 3>{.5, 1., 1.5}, 0.5*a0);
            }
        }
        {
            ra::Unique<complex, 1> const a({3}, {1, 2, 3});
            {
                auto a0 = a(0);
                tr.test_eq(0.5, 0.5*a0);
            }
            {
                auto a0 = a.at(ra::Small<int, 1> { 0 }); // BUG Not sure this is what I want, see above
                tr.test_eq(2.1, 2.1*a0);
                tr.test_eq(0.5, 0.5*a0);
                tr.test_eq(0.5, complex(0.5)*a0);
            }
        }
    }

    tr.section("operators with Small");
    {
        ra::Small<int, 3> a { 1, 2, 3 };
        ra::Small<int, 3> b { 1, 2, 4 };
        tr.test_eq(ra::Small<int, 3> {2, 4, 7}, ra::map_([](int a, int b){ return a + b; }, a.iter(), b.iter()));
        tr.test_eq(ra::Small<int, 3> {2, 4, 7}, (a.iter() + b.iter()));
        tr.test_eq(ra::Small<int, 3> {2, 4, 7}, a+b);
    }

    tr.section("constructors from Map"); // TODO For all other Container types.
    {
        {
// TODO Systematic init-from-expr tests (every expr type vs every container type)
            ra::Unique<int, 1> a({3}, { 1, 2, 3 });
            ra::Unique<int, 1> b({3}, { 10, 20, 30 });
            ra::Unique<int, 1> c(a.iter() + b.iter());
            tr.test_eq(ra::Small<int, 3> {11, 22, 33}, c);
        }
        {
            ra::Unique<int, 2> a({3, 2}, 77);
            tr.test_eq(a, ra::Small<int, 3, 2> {77, 77, 77, 77, 77, 77});
        }
        {
            ra::Unique<int, 2> a({3, 2}, ra::cast<int>(ra::_0-ra::_1));
            tr.test_eq(ra::Small<int, 3, 2> {0, -1, 1, 0, 2, 1}, a);
        }
    }

    tr.section("mixed ra-type / foreign-scalar operations");
    {
        ra::Unique<int, 2> a({3, 2}, { 1, 2, 3, 20, 5, 6 });
        ra::Small<int, 3, 2> ref {4, 5, 6, 23, 8, 9};
        tr.test_eq(ref, ra::map_([](int a, int b){ return a + b; }, ra::start(a), ra::start(3)));
        tr.test_eq(ref, ra::start(a) + ra::start(3));
        tr.test_eq(ref, a+3);
    }
// These are rather different because they have to be defined in-class.
    tr.section("constructors & assignment operators with Map rhs"); // TODO use TestRecorder::test_eq().
    {
        real check0[6] = { 0, -1, 1, 0, 2, 1 };
        real check1[6] = { 4, 3, 5, 4, 6, 5 };
        real check2[6] = { 8, 6, 10, 8, 12, 10 };
        auto test = [&](auto && a){
            tr.test(std::equal(a.begin(), a.end(), check0));
            a += 4;
            tr.test(std::equal(a.begin(), a.end(), check1));
            a += a;
            tr.test(std::equal(a.begin(), a.end(), check2));
        };
        test(ra::Unique<int, 2>({3, 2}, ra::cast<int>(ra::_0-ra::_1)));
        test(ra::Small<int, 3, 2>(ra::cast<int>(ra::_0-ra::_1)));
    }
    tr.section("assignment ops with ra::scalar [ra21]");
    {
        ra::Small<real, 2> a { 0, 0 };
        ra::Big<ra::Small<real, 2>, 1> b { {1, 10}, {2, 20}, {3, 30} };
// use scalar to match 1 (a) vs 3 (b) instead of 2 vs 3.
        ra::scalar(a) += b;
        tr.test_eq(ra::Small<real, 2> { 6, 60 }, a);
    }
    tr.section("pack operator");
    {
        ra::Small<real, 6> a = { 0, -1, 1, 0, 2, 1 };
        ra::Small<int, 6> b = { 4, 3, 5, 4, 6, 5 };
        ra::Big<std::tuple<real, int>, 1> x = ra::pack<std::tuple<real, int>>(a, b); // TODO kinda redundant...
        tr.test_eq(a, map([](auto && x) -> decltype(auto) { return std::get<0>(x); }, x));
        tr.test_eq(b, map([](auto && x) -> decltype(auto) { return std::get<1>(x); }, x));
    }
    tr.section("pack operator as ref");
    {
        using T = std::tuple<real, int>;
        ra::Big<T> x { T(0., 1), T(2., 3), T(4., 5) };
        ra::Small<real, 3> a = -99.;
        ra::Small<int, 3> b = -77;
        ra::pack<std::tuple<real &, int &>>(a, b) = x;
        tr.test_eq(ra::Small<real, 3> {0., 2., 4.}, a);
        tr.test_eq(ra::Small<int, 3> {1, 3, 5}, b);
    }
    tr.section("operator= for View, Container. Cf test/ownership.cc");
    {
        real check5[6] = { 5, 5, 5, 5, 5, 5 };
        real check9[6] = { 9, 9, 9, 9, 9, 9 };
        ra::Unique<int, 2> a({3, 2}, 7);
        ra::Unique<int, 2> b({3, 2}, 5);
        ra::ViewBig<int *, 2> c = a();
        ra::ViewBig<int *, 2> d = b();
        c = d;
        tr.test(std::equal(a.begin(), a.end(), check5));
        ra::Unique<int, 2> t({2, 3}, 9);
        c = transpose(t, {1, 0});
        tr.test(std::equal(a.begin(), a.end(), check9));
        a = d;
        tr.test(std::equal(a.begin(), a.end(), check5));
        ra::Unique<int, 2> e = d;
        tr.test(std::equal(e.begin(), e.end(), check5));
    }
    tr.section("operator= for Dynamic");
    {
        ra::Unique<int, 1> a({7}, 7);
        ra::Small<ra::dim_t, 3> i { 2, 3, 5 };
        ra::Small<int, 3> b { 22, 33, 55 };
        ra::map_([&a](ra::dim_t i) -> decltype(auto) { return a(i); }, ra::start(i)) = b;
        int checka[] = { 7, 7, 22, 33, 7, 55, 7 };
        tr.test(std::equal(checka, checka+7, a.begin()));
    }
    tr.section("wedge");
    {
        {
            ra::Small<real, 3> a {1, 2, 3};
            ra::Small<real, 3> b {4, 5, 7};
            ra::Small<real, 3> c;
            ra::Wedge<3, 1, 1>::prod(a, b, c);
            tr.test_eq(ra::Small<real, 3> {-1, 5, -3}, c);
        }
        {
            ra::Small<real, 1> a {2};
            ra::Small<real, 1> b {3};
            ra::Small<real, 1> r;
            ra::Wedge<1, 0, 0>::prod(a, b, r);
            tr.test_eq(6, r[0]);
            tr.test_eq(6, ra::wedge<1, 0, 0>(ra::Small<real, 1>{2}, ra::Small<real, 1>{3}));
            tr.test_eq(6, ra::wedge<1, 0, 0>(ra::Small<real, 1>{2}, 3.));
            tr.test_eq(6, ra::wedge<1, 0, 0>(2., ra::Small<real, 1>{3}));
            tr.test_eq(6, ra::wedge<1, 0, 0>(2., 3));
        }
    }
    tr.section("hodge / hodgex");
    {
        ra::Small<real, 3> a {1, 2, 3};
        ra::Small<real, 3> c;
        ra::hodgex<3, 1>(a, c);
        tr.test_eq(a, c);
        auto d = ra::hodge<3, 1>(a);
        tr.test_eq(a, d);
    }
    tr.section("index");
    {
        {
            ra::Big<real, 1> a {1, 2, 3, -4, 9, 9, 8};
            tr.test_eq(3, index(a<0));
            tr.test_eq(-1, index(a>100));
        }
        {
            ra::Big<real> a {1, 2, 3, -4, 9, 9, 8};
            tr.test_eq(4, index(abs(a)>4));
        }
    }
    tr.section("lexical_compare");
    {
        ra::Big<int, 3> a({10, 2, 2}, {0, 0, 1, 3, 0, 1, 3, 3, 0, 2, 3, 0, 3, 1, 2, 1, 1, 1, 3, 1, 0, 3, 2, 2, 2, 3, 1, 2, 2, 0, 0, 1, 0, 1, 1, 1, 3, 0, 2, 1});
        ra::Big<int, 1> i = ra::iota(a.len(0));
        std::sort(i.data(), i.data()+i.size(), [&a](int i, int j){ return lexical_compare(a(i), a(j)); });
        tr.test_eq(ra::start({0, 8, 1, 2, 5, 4, 7, 6, 9, 3}), i);
    }
    return tr.summary();
}
