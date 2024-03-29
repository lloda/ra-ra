// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Test abilities of post v10 driverless frame matching Expr.

// (c) Daniel Llorens - 2019-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#include <iostream>
#include <iterator>
#include "ra/test.hh"

using std::cout, std::endl, std::flush, ra::TestRecorder;

template <int i> using UU = decltype(std::declval<ra::Unique<double, i>>().iter());
using ra::int_c;

namespace ra::mp {

// once we had fold expr this became less useful.
template <class K, class T, class F, class I = int_c<0>>
constexpr auto
fold_tuple(K && k, T && t, F && f, I && i = int_c<0> {})
{
    if constexpr (I::value==len<std::decay_t<T>>) {
        return k;
    } else {
        return fold_tuple(f(k, std::get<I::value>(t)), t, f, int_c<I::value+1> {});
    }
}

} // namespace ra::mp

int
main()
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
        ra::mp::int_list<6, 3, -4> x;
        constexpr int ma = ra::mp::fold_tuple(-99, x, [](auto && k, auto && a) { return max(k, a.value); });
        constexpr int mi = ra::mp::fold_tuple(+99, x, [](auto && k, auto && a) { return min(k, a.value); });
        constexpr int su = ra::mp::fold_tuple(0, x, [](auto && k, auto && a) { return k + a.value; });
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
        tr.test_eq(b.len(0), EXPR.len(0));
        tr.test_eq(b.len(1), EXPR.len(1));
        tr.test_eq(b.len(2), EXPR.len(2));
        tr.test_eq(b.len(3), EXPR.len(3));
        tr.test_eq(2*3*4*5, size(EXPR));
        static_assert(4==ra::rank_s<decltype(EXPR)>());
        static_assert(b.len_s(0)==EXPR.len_s(0));
        static_assert(b.len_s(1)==EXPR.len_s(1));
        static_assert(b.len_s(2)==EXPR.len_s(2));
        static_assert(b.len_s(3)==EXPR.len_s(3));
        static_assert(2*3*4*5 == ra::size_s<decltype(EXPR)>());
#undef EXPR
    }
    tr.section("check mismatches - static");
    {
        ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
        ra::Small<int, 2, 4, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
// properly fails to compile, which we cannot check at present [ra42]
// #define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
//         tr.test_eq(2*3*4*5, ra::size_s<decltype(EXPR)>());
//         tr.test_eq(3, EXPR.len_s(1));
// #undef EXPR
// we can use non-static Match::check() as constexpr however.
        static_assert(!agree(a, b));
    }
    tr.section("static rank, dynamic size - like Expr");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define EXPR expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, EXPR.rank());
        tr.test_eq(b.len(0), EXPR.len(0));
        tr.test_eq(b.len(1), EXPR.len(1));
        tr.test_eq(b.len(2), EXPR.len(2));
        tr.test_eq(b.len(3), EXPR.len(3));
        tr.test_eq(2*3*4*5, size(EXPR));
// could check all statically through decltype, although Big cannot be constexpr yet.
        static_assert(4==ra::rank_s<decltype(EXPR)>());
        tr.test_eq(ra::ANY, EXPR.len_s(0));
        tr.test_eq(ra::ANY, EXPR.len_s(1));
        tr.test_eq(ra::ANY, EXPR.len_s(2));
        tr.test_eq(ra::ANY, EXPR.len_s(3));
        tr.test_eq(ra::ANY, ra::size_s<decltype(EXPR)>());
        cout << EXPR << endl;
#undef EXPR
    }
    tr.section("check mismatches - dynamic (explicit)");
    {
        {
            ra::Big<int, 3> a({2, 3, 4}, 0);
            ra::Big<int, 4> b({2, 4, 4, 5}, 0);
            tr.test(!ra::agree(a, b));
// TestRecorder sees mismatches as another kind of error, it used to happen this would RA_ASSERT instead.
// FIXME This isn't true for static mismatches, which will fail to compile.
            tr.expectfail().test_eq(a, b);
        }
        {
            ra::Big<int, 3> a({2, 3, 4}, 0);
            ra::Big<int, 4> b({2, 3, 4, 5}, 0);
            tr.test(ra::agree(a, b));
            tr.test_eq(a, b);
        }
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
                        tr.info("0d").test_eq(b.len(0), EXPR(a, b).len(0));
                        tr.test_eq(b.len(1), EXPR(a, b).len(1));
                        tr.test_eq(b.len(2), EXPR(a, b).len(2));
                        tr.test_eq(b.len(3), EXPR(a, b).len(3));
                        tr.info("0-size()").test_eq(2*3*4*5, size(EXPR(a, b)));
                        tr.test_eq(ra::ANY, ra::rank_s<decltype(EXPR(a, b))>());
                        tr.test_eq(ra::ANY, ra::size_s<decltype(EXPR(a, b))>());
                        tr.test_eq(ra::ANY, EXPR(a, b).len_s(0));
                        tr.test_eq(ra::ANY, EXPR(a, b).len_s(1));
                        tr.test_eq(ra::ANY, EXPR(a, b).len_s(2));
                        tr.test_eq(ra::ANY, EXPR(a, b).len_s(3));
                        tr.info("0-size_s()").test_eq(ra::ANY, ra::size_s<decltype(EXPR(a, b))>());
                    };
        test("sta-dyn", as, bd);
        test("dyn-sta", ad, bs);
        test("dyn-dyn", ad, bd);
#undef EXPR
    }
    tr.section("cases with periodic axes - dynamic (broken with Expr)");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        auto b = a(ra::all, ra::insert<1>, ra::iota(4, 0, 0));
#define EXPR(a, b) expr([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, EXPR(a, b).rank());
        tr.test_eq(b.len(0), EXPR(a, b).len(0));
        tr.test_eq(a.len(1), EXPR(a, b).len(1));
        tr.test_eq(b.len(2), EXPR(a, b).len(2));
        tr.test_eq(b.len(3), EXPR(a, b).len(3));
        tr.test_eq(2*3*4*4, size(EXPR(a, b)));
// could check all statically through decltype, although Big cannot be constexpr yet.
        static_assert(4==ra::rank_s<decltype(EXPR(a, b))>());
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(0));
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(1));
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(2));
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(3));
        tr.test_eq(ra::ANY, ra::size_s<decltype(EXPR(a, b))>());
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
#define EXPR(a, b) expr([](auto && a, auto && b) { return a-b; }, start(a(ra::dots<2>, ra::insert<1>)), start(b(ra::insert<2>, ra::dots<1>)))
        tr.test_eq(3, ra::rank_s<decltype(EXPR(a, b))>());
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(0));
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(1));
        tr.test_eq(ra::ANY, EXPR(a, b).len_s(2));
        tr.test_eq(3, EXPR(a, b).rank());
        tr.test_eq(4, EXPR(a, b).len(0));
        tr.test_eq(3, EXPR(a, b).len(1));
        tr.test_eq(5, EXPR(a, b).len(2));
        tr.test_eq(from([](auto && a, auto && b) { return a-b; }, a, b), EXPR(a, b));
#undef EXPR
    }
    tr.section("Expr has operatorX=");
    {
        ra::Big<int, 2> a({4, 3}, 10*ra::_1+100*ra::_0);
        expr([](auto & a) -> decltype(auto) { return a; }, start(a)) += 1;
        tr.test_eq(10*ra::_1 + 100*ra::_0 + 1, a);
    }
    tr.section("Compat with old Expr, from ra-0.cc");
    {
        int p[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        int * pp = &p[0]; // force pointer decay in case we ever enforce p's shape
        ra::ViewBig<int> d(ra::pack<ra::Dim>(ra::Small<int, 3> {5, 1, 2}, ra::Small<int, 3> {1, 0, 5}), pp);
#define EXPR expr([](auto && a, auto && b) { return a==b; }, ra::_0*1 + ra::_1*0 + ra::_2*5 + 1, start(d))
        tr.test(every(EXPR));
        auto x = EXPR;
        static_assert(ra::ANY==ra::size_s<decltype(x)>());
        static_assert(ra::ANY==ra::size_s<decltype(x)>());
        tr.test_eq(10, size(EXPR));
    }
    tr.section("BAD on any len_s(k) means size_s() is BAD");
    {
        using order = ra::mp::int_list<0, 1>;
        using T0 = ra::Expr<std::multiplies<void>, std::tuple<decltype(ra::iota<0>()), ra::Scalar<int>>, order>;
        ra::dim_t s0 = ra::size_s<T0>();
        using T1 = ra::Expr<std::multiplies<void>, std::tuple<decltype(ra::iota<1>()), ra::Scalar<int>>, order>;
        ra::dim_t s1 = ra::size_s<T1>();
        using T2 = ra::Expr<std::multiplies<void>, std::tuple<decltype(ra::iota<2>()), ra::Scalar<int>>, order>;
        ra::dim_t s2 = ra::size_s<T2>();
        tr.test_eq(ra::BAD, s0);
        tr.test_eq(ra::BAD, s1);
        tr.test_eq(ra::BAD, s2);
    }
    return tr.summary();
}
