// -*- mode: c++; coding: utf-8 -*-
/// @file frame-new.C
/// @brief Test abilities of post v10 driverless frame matching Expr

// (c) Daniel Llorens - 2019
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/format.H"


// -------------------------------------
// bit from example/throw.C which FIXME should be easier. Maybe an option in ra/macros.H.

struct ra_error: public std::exception
{
    std::string s;
    template <class ... A> ra_error(A && ... a): s(ra::format(std::forward<A>(a) ...)) {}
    virtual char const * what() const throw ()
    {
        return s.c_str();
    }
};

#ifdef RA_ASSERT
#error RA_ASSERT is already defined!
#endif
#define RA_ASSERT( cond, ... )                                          \
    { if (!( cond )) throw ra_error("ra:: assert [" STRINGIZE(cond) "]", __VA_ARGS__); }
// -------------------------------------

#include "ra/test.H"
#include "ra/operators.H"
#include "ra/io.H"

using std::cout, std::endl, std::flush;

template <int i> using TI = ra::TensorIndex<i, int>;
template <int i> using UU = decltype(std::declval<ra::Unique<double, i>>().iter());
using mp::int_t;

int main()
{
    TestRecorder tr(std::cout);
    tr.section("view");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 2, 3, 4}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        cout << a << endl;
    }
    tr.section("II");
    {
        std::tuple<int_t<6>, int_t<3>, int_t<-4>> x;
        constexpr int ma = mp::fold_tuple(-99, x, [](auto && k, auto && a) { return max(k, std::decay_t<decltype(a)>::value); });
        constexpr int mi = mp::fold_tuple(+99, x, [](auto && k, auto && a) { return min(k, std::decay_t<decltype(a)>::value); });
        constexpr int su = mp::fold_tuple(0, x, [](auto && k, auto && a) { return k + std::decay_t<decltype(a)>::value; });
        cout << ma << endl;
        cout << mi << endl;
        cout << su << endl;
    }
    tr.section("static size - like Expr");
    {
        ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
        ra::Small<int, 2, 3, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
#define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, EXPR.rank());
        tr.test_eq(b.size(0), EXPR.size(0));
        tr.test_eq(b.size(1), EXPR.size(1));
        tr.test_eq(b.size(2), EXPR.size(2));
        tr.test_eq(b.size(3), EXPR.size(3));
        tr.test_eq(2*3*4*5, EXPR.size());
        static_assert(4==EXPR.rank_s());
        static_assert(b.size_s(0)==EXPR.size_s(0));
        static_assert(b.size_s(1)==EXPR.size_s(1));
        static_assert(b.size_s(2)==EXPR.size_s(2));
        static_assert(b.size_s(3)==EXPR.size_s(3));
        static_assert(2*3*4*5 == EXPR.size_s());
#undef EXPR
    }
    // properly fails to compile, which we cannot check at present [ra42]
//     tr.section("check mismatches - static");
//     {
//         ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
//         ra::Small<int, 2, 4, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
// #define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
//         tr.test_eq(2*3*4*5, EXPR.size_s());
//         tr.test_eq(3, EXPR.size_s(1));
// #undef EXPR
//     }
    tr.section("static rank, dynamic size - like Expr");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, EXPR.rank());
        tr.test_eq(b.size(0), EXPR.size(0));
        tr.test_eq(b.size(1), EXPR.size(1));
        tr.test_eq(b.size(2), EXPR.size(2));
        tr.test_eq(b.size(3), EXPR.size(3));
        tr.test_eq(2*3*4*5, EXPR.size());
// these _s are all static, but cannot Big cannot be constexpr yet.
// also decltype(EXPR) fails until p0315r3 (c++20).
// so check at runtime instead.
        tr.test_eq(4, EXPR.rank_s());
        tr.test_eq(ra::DIM_ANY, EXPR.size_s());
        tr.test_eq(ra::DIM_ANY, EXPR.size_s(0));
        tr.test_eq(ra::DIM_ANY, EXPR.size_s(1));
        tr.test_eq(ra::DIM_ANY, EXPR.size_s(2));
        tr.test_eq(ra::DIM_ANY, EXPR.size_s(3));
        tr.test_eq(ra::DIM_ANY, EXPR.size_s());
        cout << EXPR << endl;
#undef EXPR
    }
    tr.section("check mismatches - dynamic");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 4, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        int x = 0;
        try {
            tr.test_eq(ra::DIM_ANY, EXPR.size_s(1));
            x = 1;
        } catch (ra_error & e) {
        }
        tr.info("caught error").test_eq(0, x);
#undef EXPR
    }
    tr.section("dynamic rank - Expr driver selection is broken in this case.");
    {
        ra::Big<int, 3> as({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int> ad({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> bs({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        ra::Big<int> bd({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define EXPR(a, b) expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        auto test = [&tr](auto tag, auto && a, auto && b)
                    {
                        tr.section(tag);
                        tr.test_eq(4, EXPR(a, b).rank());
                        tr.info("0d").test_eq(b.size(0), EXPR(a, b).size(0));
                        tr.test_eq(b.size(1), EXPR(a, b).size(1));
                        tr.test_eq(b.size(2), EXPR(a, b).size(2));
                        tr.test_eq(b.size(3), EXPR(a, b).size(3));
                        tr.info("0-size()").test_eq(2*3*4*5, EXPR(a, b).size());
                        tr.test_eq(ra::RANK_ANY, EXPR(a, b).rank_s());
                        tr.info("0s").test_eq(ra::DIM_ANY, EXPR(a, b).size_s());
                        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(0));
                        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(1));
                        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(2));
                        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(3));
                        tr.info("0-size_s()").test_eq(ra::DIM_ANY, EXPR(a, b).size_s());
                    };
        test("sta-dyn", as, bd);
        test("dyn-sta", ad, bs);
        test("dyn-dyn", ad, bd);
#undef EXPR
    }
    tr.section("cases with periodic axes - dynamic (broken with Expr)");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        auto b = a(ra::all, ra::newaxis<1>, ra::iota(4, 0, 0));
#define EXPR(a, b) expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, EXPR(a, b).rank());
        tr.test_eq(b.size(0), EXPR(a, b).size(0));
        tr.test_eq(a.size(1), EXPR(a, b).size(1));
        tr.test_eq(b.size(2), EXPR(a, b).size(2));
        tr.test_eq(b.size(3), EXPR(a, b).size(3));
        tr.test_eq(2*3*4*4, EXPR(a, b).size());
// these _s are all static, but cannot Big cannot be constexpr yet.
// also decltype(EXPR(a, b)) fails until p0315r3 (c++20).
// so check at runtime instead.
        tr.test_eq(4, EXPR(a, b).rank_s());
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s());
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(0));
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(1));
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(2));
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(3));
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s());
        cout << EXPR(a, b) << endl;
// value test.
        ra::Big<int, 4> c({2, 3, 4, 4}, 0);
        c(ra::all, 0) = a(ra::all, ra::iota(4, 0, 0));
        c(ra::all, 1) = a(ra::all, ra::iota(4, 0, 0));
        c(ra::all, 2) = a(ra::all, ra::iota(4, 0, 0));
        tr.test_eq((a+c), EXPR(a, b));
// order doesn't affect prefix matching with Expr
        tr.test_eq((a+c), EXPR(b, a));
#undef EXPR
    }
    tr.section("broadcasting - like outer product");
    {
        ra::Big<int, 2> a({4, 3}, 10*ra::_1+100*ra::_0);
        ra::Big<int, 1> b({5}, ra::_0);
        cout << ra::start(ra::shape(from([](auto && a, auto && b) { return a-b; }, a, b))) << endl;
#define EXPR(a, b) expr([](auto && a, auto && b) { return a-b; }, start(a(ra::dots<2>, ra::newaxis<1>)), start(b(ra::newaxis<2>, ra::dots<1>)))
        tr.test_eq(3, EXPR(a, b).rank_s());
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(0));
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(1));
        tr.test_eq(ra::DIM_ANY, EXPR(a, b).size_s(2));
        tr.test_eq(3, EXPR(a, b).rank());
        tr.test_eq(4, EXPR(a, b).size(0));
        tr.test_eq(3, EXPR(a, b).size(1));
        tr.test_eq(5, EXPR(a, b).size(2));
        tr.test_eq(from([](auto && a, auto && b) { return a-b; }, a, b), EXPR(a, b));
#undef EXPR
    }
    tr.section("Expr has operatorX=");
    {
        ra::Big<int, 2> a({4, 3}, 10*ra::_1+100*ra::_0);
        expr([](auto & a) -> decltype(auto) { return a; }, start(a)) += 1;
        tr.test_eq(10*ra::_1 + 100*ra::_0 + 1, a);
    }
    tr.section("Compat with old Expr, from ra-0.C");
    {
        int p[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        int * pp = &p[0]; // force pointer decay in case we ever enforce p's shape
        ra::View<int> d(ra::pack<ra::Dim>(ra::Small<int, 3> {5, 1, 2}, ra::Small<int, 3> {1, 0, 5}), pp);
#define EXPR expr([](auto && a, auto && b) { return a==b; }, ra::_0*1 + ra::_1*0 + ra::_2*5 + 1, start(d))
        tr.test(every(EXPR));
        auto x = EXPR;
        static_assert(ra::DIM_ANY==decltype(x)::size_s());
        tr.test_eq(10, (EXPR).size());
    }
    tr.section("DIM_BAD on any size_s(k) means size_s() is DIM_BAD");
    {
        using order = std::tuple<mp::int_t<0>, mp::int_t<1> >;
        using T0 = ra::Expr<ra::times, std::tuple<ra::TensorIndex<0>, ra::Scalar<int>>, order>;
        ra::dim_t s0 = T0::size_s();
        using T1 = ra::Expr<ra::times, std::tuple<ra::TensorIndex<1>, ra::Scalar<int>>, order>;
        ra::dim_t s1 = T1::size_s();
        using T2 = ra::Expr<ra::times, std::tuple<ra::TensorIndex<2>, ra::Scalar<int>>, order>;
        ra::dim_t s2 = T2::size_s();
        tr.test_eq(ra::DIM_BAD, s0);
        tr.test_eq(ra::DIM_BAD, s1);
        tr.test_eq(ra::DIM_BAD, s2);
    }
    return tr.summary();
}
