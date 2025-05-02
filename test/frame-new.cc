// -*- mode: c++; coding: utf-8 -*-
// ra-ra/test - Test abilities of post v10 driverless frame matching Map.

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

template <class K, class T, class F>
constexpr auto
fold_tuple(K && k, T && t, F && f)
{
    return std::apply([&](auto ... i) { auto r=k; ((r=f(r, i)), ...); return r; }, t);
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
        ra::int_list<6, 3, -4> x;
        static_assert(6==ra::mp::fold_tuple(-99, x, [](auto && k, auto && a) { return max(k, a.value); }));
        static_assert(-4==ra::mp::fold_tuple(+99, x, [](auto && k, auto && a) { return min(k, a.value); }));
        static_assert(5==ra::mp::fold_tuple(0, x, [](auto && k, auto && a) { return k + a.value; }));
    }
    tr.section("static size - like Map");
    {
        ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
        ra::Small<int, 2, 3, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
#define MAP map_([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, MAP.rank());
        tr.test_eq(b.len(0), MAP.len(0));
        tr.test_eq(b.len(1), MAP.len(1));
        tr.test_eq(b.len(2), MAP.len(2));
        tr.test_eq(b.len(3), MAP.len(3));
        tr.test_eq(2*3*4*5, size(MAP));
        static_assert(4==ra::rank_s<decltype(MAP)>());
        static_assert(b.len_s(0)==MAP.len_s(0));
        static_assert(b.len_s(1)==MAP.len_s(1));
        static_assert(b.len_s(2)==MAP.len_s(2));
        static_assert(b.len_s(3)==MAP.len_s(3));
        static_assert(2*3*4*5 == ra::size_s<decltype(MAP)>());
#undef MAP
    }
    tr.section("check mismatches - static");
    {
        ra::Small<int, 2, 3, 4> a = (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1);
        ra::Small<int, 2, 4, 4, 5> b = (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1);
// properly fails to compile, which we cannot check at present [ra42]
// #define MAP map_([](auto && a, auto && b) { return a+b; }, start(a), start(b))
//         tr.test_eq(2*3*4*5, ra::size_s<decltype(MAP)>());
//         tr.test_eq(3, MAP.len_s(1));
// #undef MAP
// we can use non-static Match::check() as constexpr however.
        static_assert(!agree(a, b));
    }
    tr.section("static rank, dynamic size - like Map");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> b({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define MAP map_([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, MAP.rank());
        tr.test_eq(b.len(0), MAP.len(0));
        tr.test_eq(b.len(1), MAP.len(1));
        tr.test_eq(b.len(2), MAP.len(2));
        tr.test_eq(b.len(3), MAP.len(3));
        tr.test_eq(2*3*4*5, size(MAP));
// could check all statically through decltype, although Big cannot be constexpr yet.
        static_assert(4==ra::rank_s<decltype(MAP)>());
        tr.test_eq(ra::ANY, MAP.len_s(0));
        tr.test_eq(ra::ANY, MAP.len_s(1));
        tr.test_eq(ra::ANY, MAP.len_s(2));
        tr.test_eq(ra::ANY, MAP.len_s(3));
        tr.test_eq(ra::ANY, ra::size_s<decltype(MAP)>());
        cout << MAP << endl;
#undef MAP
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
    tr.section("dynamic rank - Map driver selection is broken in this case.");
    {
        ra::Big<int, 3> as({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int> ad({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        ra::Big<int, 4> bs({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
        ra::Big<int> bd({2, 3, 4, 5}, (ra::_0+1)*1000 + (ra::_1+1)*100 + (ra::_2+1)*10 + (ra::_3+1));
#define MAP(a, b) map_([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        auto test = [&tr](auto tag, auto && a, auto && b)
                    {
                        tr.section(tag);
                        tr.test_eq(4, MAP(a, b).rank());
                        tr.info("0d").test_eq(b.len(0), MAP(a, b).len(0));
                        tr.test_eq(b.len(1), MAP(a, b).len(1));
                        tr.test_eq(b.len(2), MAP(a, b).len(2));
                        tr.test_eq(b.len(3), MAP(a, b).len(3));
                        tr.info("0-size()").test_eq(2*3*4*5, size(MAP(a, b)));
                        tr.test_eq(ra::ANY, ra::rank_s<decltype(MAP(a, b))>());
                        tr.test_eq(ra::ANY, ra::size_s<decltype(MAP(a, b))>());
                        tr.test_eq(ra::ANY, MAP(a, b).len_s(0));
                        tr.test_eq(ra::ANY, MAP(a, b).len_s(1));
                        tr.test_eq(ra::ANY, MAP(a, b).len_s(2));
                        tr.test_eq(ra::ANY, MAP(a, b).len_s(3));
                        tr.info("0-size_s()").test_eq(ra::ANY, ra::size_s<decltype(MAP(a, b))>());
                    };
        test("sta-dyn", as, bd);
        test("dyn-sta", ad, bs);
        test("dyn-dyn", ad, bd);
#undef MAP
    }
    tr.section("cases with periodic axes - dynamic (broken with Map)");
    {
        ra::Big<int, 3> a({2, 3, 4}, (ra::_0+1)*100 + (ra::_1+1)*10 + (ra::_2+1));
        auto b = a(ra::all, ra::insert<1>, ra::iota(4, 0, 0));
#define MAP(a, b) map_([](auto && a, auto && b) { return a+b; }, start(a), start(b))
        tr.test_eq(4, MAP(a, b).rank());
        tr.test_eq(b.len(0), MAP(a, b).len(0));
        tr.test_eq(a.len(1), MAP(a, b).len(1));
        tr.test_eq(b.len(2), MAP(a, b).len(2));
        tr.test_eq(b.len(3), MAP(a, b).len(3));
        tr.test_eq(2*3*4*4, size(MAP(a, b)));
// could check all statically through decltype, although Big cannot be constexpr yet.
        static_assert(4==ra::rank_s<decltype(MAP(a, b))>());
        tr.test_eq(ra::ANY, MAP(a, b).len_s(0));
        tr.test_eq(ra::ANY, MAP(a, b).len_s(1));
        tr.test_eq(ra::ANY, MAP(a, b).len_s(2));
        tr.test_eq(ra::ANY, MAP(a, b).len_s(3));
        tr.test_eq(ra::ANY, ra::size_s<decltype(MAP(a, b))>());
        cout << MAP(a, b) << endl;
// value test.
        ra::Big<int, 4> c({2, 3, 4, 4}, 0);
        c(ra::all, 0) = a(ra::all, ra::iota(4, 0, 0));
        c(ra::all, 1) = a(ra::all, ra::iota(4, 0, 0));
        c(ra::all, 2) = a(ra::all, ra::iota(4, 0, 0));
        tr.test_eq((a+c), MAP(a, b));
// order doesn't affect prefix matching with Map
        tr.test_eq((a+c), MAP(b, a));
#undef MAP
    }
    tr.section("broadcasting - like outer product");
    {
        ra::Big<int, 2> a({4, 3}, 10*ra::_1+100*ra::_0);
        ra::Big<int, 1> b({5}, ra::_0);
        cout << ra::start(ra::shape(from([](auto && a, auto && b) { return a-b; }, a, b))) << endl;
#define MAP(a, b) map_([](auto && a, auto && b) { return a-b; }, start(a(ra::dots<2>, ra::insert<1>)), start(b(ra::insert<2>, ra::dots<1>)))
        tr.test_eq(3, ra::rank_s<decltype(MAP(a, b))>());
        tr.test_eq(ra::ANY, MAP(a, b).len_s(0));
        tr.test_eq(ra::ANY, MAP(a, b).len_s(1));
        tr.test_eq(ra::ANY, MAP(a, b).len_s(2));
        tr.test_eq(3, MAP(a, b).rank());
        tr.test_eq(4, MAP(a, b).len(0));
        tr.test_eq(3, MAP(a, b).len(1));
        tr.test_eq(5, MAP(a, b).len(2));
        tr.test_eq(from([](auto && a, auto && b) { return a-b; }, a, b), MAP(a, b));
#undef MAP
    }
    tr.section("Map has operatorX=");
    {
        ra::Big<int, 2> a({4, 3}, 10*ra::_1+100*ra::_0);
        map_([](auto & a) -> decltype(auto) { return a; }, start(a)) += 1;
        tr.test_eq(10*ra::_1 + 100*ra::_0 + 1, a);
    }
    tr.section("Compat with old Map, from ra-0.cc");
    {
        int p[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        int * pp = &p[0]; // force pointer decay in case we ever enforce p's shape
        ra::ViewBig<int> d(ra::pack<ra::Dim>(ra::Small<int, 3> {5, 1, 2}, ra::Small<int, 3> {1, 0, 5}), pp);
#define MAP map_([](auto && a, auto && b) { return a==b; }, ra::_0*1 + ra::_1*0 + ra::_2*5 + 1, start(d))
        tr.test(every(MAP));
        auto x = MAP;
        static_assert(ra::ANY==ra::size_s<decltype(x)>());
        static_assert(ra::ANY==ra::size_s<decltype(x)>());
        tr.test_eq(10, size(MAP));
    }
#undef MAP
    tr.section("BAD on any len_s(k) means size_s() is BAD");
    {
        using order = ra::int_list<0, 1>;
        using T0 = ra::Map<std::multiplies<void>, std::tuple<decltype(ra::iota<0>()), ra::Scalar<int>>, order>;
        ra::dim_t s0 = ra::size_s<T0>();
        using T1 = ra::Map<std::multiplies<void>, std::tuple<decltype(ra::iota<1>()), ra::Scalar<int>>, order>;
        ra::dim_t s1 = ra::size_s<T1>();
        using T2 = ra::Map<std::multiplies<void>, std::tuple<decltype(ra::iota<2>()), ra::Scalar<int>>, order>;
        ra::dim_t s2 = ra::size_s<T2>();
        tr.test_eq(ra::BAD, s0);
        tr.test_eq(ra::BAD, s1);
        tr.test_eq(ra::BAD, s2);
    }
    return tr.summary();
}
