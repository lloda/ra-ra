// -*- mode: c++; coding: utf-8 -*-
// ek/box - A properly general version of view(...)

// (c) Daniel Llorens - 2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Atm view(i ...) only beats i of rank ≤1 and only when all i are beatable. This is a sandbox for a version that beats any combination of scalar or view<Seq> of any rank, and only returns an expr for the unbeatable subscripts.
// There is an additional issue that eg A2(unbeatable-i1) (indicating ranks) returns a nested expression, that is, a rank-1 expr where each element is a rank-1 view. It should instead be rank 2 and not nested, iow, the value type of the result should always be the same as that of A.

/*

RA_CK(inside(i, len(k)), "Bad index iota [", i.n, " ", i.cp.i, " ", i.s, "] in len[", k, "]=", len(k), ".");

TODO
* [ ] optimize() applies to ops with general iota
* [ ] Ptr is a slice (but being Slice and Iterator at the same time is trouble)
* [ ] review '1-past is ok but 1-before is not' change in fromb.cc
* [ ] index checks
  - [X] for scalar
  - [X] for iota (test/checks.cc)
  - [ ] unuglify
* [X] unbeaten part
  - [X] use as lvalue
  - [ ] forwarding
* [ ] output should be as static as possible
  - [X] static spacer len where possible
  - [X] beaten SmallView for typical cases
  - [ ] beaten SmallView for every possible case
* [ ] len
  - [X] in is_scalar_index
  - [X] in unbeaten subscript
  - [X] in ptr (not static, but that didn't work before either)
  - [ ] in general iota

*/

#include "ra/test.hh"
#include "test/mpdebug.hh"

using std::println, std::cout, std::endl, std::flush;

namespace ra {

constexpr auto cadd(is_constant auto a, is_constant auto b) { return ic<a+b>; }
constexpr auto cadd(dim_t a, dim_t b) { return a+b; }

}; // namespace ra

int main()
{
    ra::TestRecorder tr(cout);
    {
        println(cout, "\nii([3, 4])\n{:c}", ra::ii({3, 4}));
        println(cout, "\nii([3, 4], -3, [1, 10])\n{:c}", ra::ii({3, 4}, -3, {1, 10}));
        println(cout, "\nii(<3, 4>)\n{:c}", ra::ii(ra::ilist<3, 4>));
        tr.test(ra::is_scalar_index<decltype(ra::Small<int>(1))>);
        tr.test(ra::is_scalar_index<decltype(ra::Small<int>(1).iter())>);
    }

    constexpr ra::Small<int, 3, 4> a0 = {{0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}};
    constexpr ra::Small<int, 2, 3> a1 = {{0, 1, 2}, {4, 5, 6}};
    constexpr ra::Small<int, 3> a2 = {4, 5, 6};
    constexpr ra::Small<int, 2> a3 = {1, 5};
    constexpr ra::Small<int> a4 = 5;
    constexpr ra::Small<int, 4> a5 = {4, 5, 6, 7};
    constexpr ra::Small<int, 2, 4> a6 = {{0, 1, 2, 3}, {4, 5, 6, 7}};
    constexpr ra::Small<int, 3, 2, 2> a7 = {{{0, 1}, {2, 3}}, {{4, 5}, {6, 7}}, {{8, 9}, {10, 11}}};
    constexpr ra::Small<int, 3> a8 = {1, 5, 9};
    constexpr ra::Small<int, 4> a9 = {4, 5, 6, 7};
    constexpr ra::Small<ra::dim_t, 3> aA = {3, ra::UNB, 4};

    constexpr ra::Small<int, 3, 2, 3> b0 = {{{0, 1, 2}, {3, 4, 5}},
                                            {{6, 7, 8}, {9, 10, 11}},
                                            {{12, 13, 14}, {15, 16, 17}}};
    constexpr ra::Small<int, 2, 3> b1 = {{6, 7, 8}, {9, 10, 11}};
    constexpr ra::Small<int, 3, 2> b2 = {{1, 4}, {7, 10}, {13, 16}};
    constexpr ra::Small<int, 2> b3 = {7, 10};
    constexpr ra::Small<int, 2, 2, 2, 2> u0 = {{{{0, 1}, {4, 5}}, {{12, 13}, {16, 17}}},
                                               {{{18, 19}, {22, 23}}, {{30, 31}, {34, 35}}}};
    constexpr ra::Small<int, 2, 3, 3, 2> u1 = {{{{0, 0}, {2, 3}, {0, 0}},
                                                {{6, 7}, {8, 9}, {10, 11}},
                                                {{0, 0}, {14, 15}, {0, 0}}},
                                               {{{0, 0}, {20, 21}, {0, 0}},
                                                {{24, 25}, {26, 27}, {28, 29}},
                                                {{0, 0}, {32, 33}, {0, 0}}}};
    constexpr ra::Small<int, 2, 2> u2 = {{5, 4}, {2, 1}};

    auto rank_maybe = [](bool s, auto && a, auto && ... i){ return s ? frombrank_s(a, i ...) : frombrank(a, i...); };
    auto testrank0 = [&](bool s, auto && a)
    {
        tr.info("rank0").test_eq(rank(a0), rank_maybe(s, a));
        tr.test_eq(rank(a1), rank_maybe(s, a, ra::ii({2}), ra::ii({3})));
        tr.test_eq(rank(a2), rank_maybe(s, a, 1, ra::ii({3})));
        tr.test_eq(rank(a3), rank_maybe(s, a, ra::ii({2}), 1));
        tr.test_eq(rank(a4), rank_maybe(s, a, 1, 1));
        tr.test_eq(rank(a5), rank_maybe(s, a, 1));
        tr.test_eq(rank(a6), rank_maybe(s, a, ra::ii({2})));
        tr.test_eq(rank(a7), rank_maybe(s, a, ra::all, ra::ii({2, 2})));
        tr.test_eq(rank(a8), rank_maybe(s, a, ra::dots<1>, 1));
        tr.test_eq(rank(a9), rank_maybe(s, a, 1, ra::dots<1>));
    };
    auto testrank1 = [&](bool s, auto && b)
    {
        tr.info("rank1").test_eq(rank(b0), rank_maybe(s, b));
        tr.test_eq(rank(b1), rank_maybe(s, b, 1, ra::dots<>));
        tr.test_eq(rank(b2), rank_maybe(s, b, ra::dots<>, 1));
        tr.test_eq(rank(b3), rank_maybe(s, b, 1, ra::dots<>, 1));
    };

    auto testa0 = [&](auto && a)
    {
        tr.strict().info("a0").test_eq(a0, from(a));
        tr.strict().test_eq(a1, from(a, ra::ii({2}), ra::ii({3})));
        tr.strict().test_eq(a2, from(a, 1, ra::ii({3})));
        tr.strict().test_eq(a3, from(a, ra::ii({2}), 1));
        tr.strict().test_eq(a4, from(a, 1, 1));
        tr.strict().test_eq(a5, from(a, 1));
        tr.strict().test_eq(a6, from(a, ra::ii({2})));
        tr.strict().test_eq(a7, from(a, ra::all, ra::ii({2, 2})));
        tr.strict().test_eq(a8, from(a, ra::dots<1>, 1));
        tr.strict().test_eq(a9, from(a, 1, ra::dots<1>));
        tr.strict().test_eq(aA, shape(from(a, ra::dots<1>, ra::insert<1>)));
        tr.strict().test_eq(11, from(a, 2, 3));
        tr.test(ra::ANY==rank_s(a) || std::is_integral_v<decltype(from(a, 2, 3))>);
        tr.test(ra::ANY==rank_s(a) || std::is_integral_v<decltype(from(a, ra::len-1, 3))>);
    };
    auto testa1 = [&](auto && b)
    {
        tr.strict().info("a1").test_eq(b0, from(b));
        tr.strict().test_eq(b1, from(b, 1, ra::dots<>));
        tr.strict().test_eq(b2, from(b, ra::dots<>, 1));
        tr.strict().test_eq(b3, from(b, 1, ra::dots<>, 1));
    };

    tr.section("ViewSmall<>");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 4>);
        static_assert(every(a0==a));
        testrank0(true, a);
        testrank1(true, ra::ii(ra::ilist<3, 2, 3>));
        testa0(a);
        testa1(ra::ii(ra::ilist<3, 2, 3>));
    }
    tr.section("ViewBig<... RANK>");
    {
        constexpr auto a = ra::ii({3, 4});
        static_assert(every(a0==a));
        testrank0(true, a);
        testrank1(true, ra::ii({3, 2, 3}));
        testa0(a);
        testa1(ra::ii({3, 2, 3}));
    }
    tr.section("ViewBig<... ANY>");
    {
        /* constexpr FIXME */ auto a = ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({3, 4}));
        // static_assert(every(a0==a)); // FIXME
        testa0(a);
        // testrank0(a); // FIXME only static
        testa1(ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({3, 2, 3})));
        testrank0(false, a);
        testrank1(false, ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({3, 2, 3})));
    }
    tr.section("len in scalar");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 6, 4>);
        auto b0 = from(a, ra::len-1, ra::iota(ra::ic<3>), ra::len/2);
        auto b1 = from(a, 2, ra::iota(ra::ic<3>), 2);
        tr.test_eq(ra::start({50, 54, 58}), b0);
        tr.test_eq(ra::start({50, 54, 58}), b1);
        tr.test_eq(3, b0.len_s(0));
        tr.test_eq(3, b1.len_s(0));
    }
    tr.section("len in unbeaten subscript");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 4, 3>);
// FIXME forwarding :-/
        // auto b0 = from(a, 2, ra::Small<int, 2>{0, 1} + ra::len/2, 2);
        auto b1 = from(a, 2, ra::iota(ra::ic<2>, 2), 2);
        tr.test_eq(ra::start({32, 35}), from(a, 2, ra::Small<int, 2>{0, 1} + ra::len/2, 2));
        tr.test_eq(ra::start({32, 35}), b1);
        tr.test_eq(2, from(a, 2, ra::Small<int, 2>{0, 1} + ra::len/2, 2).len_s(0));
        tr.test_eq(2, b1.len_s(0));
    }
    tr.section("len in ptr-iota");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 4, 3>);
        auto b0 = from(a, 2, ra::iota(ra::len, 0), 2);
        auto b1 = from(a, 2, ra::iota(ra::ic<4>, 0), 2);
        tr.test_eq(ra::start({26, 29, 32, 35}), b0);
        tr.test_eq(ra::start({26, 29, 32, 35}), b1);
        tr.info("FIXME").skip().test_eq(4, b0.len_s(0));
        tr.test_eq(4, b1.len_s(0));
    }
    tr.section("len in view-iota (only in cp)");
    {
        auto b = ra::ii({20});
        tr.strict().test_eq(ra::iota(10, 10), b(ra::ii({10}, ra::len-10)));
        auto a = ra::ii({10}, ra::len-10);
        tr.strict().test_eq(ra::iota(10, 10), b(a));
    }
    tr.section("Ptr works as rank 1 iota");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 6, 4>);
        {
            auto b0 = from(a, 1, ra::iota(3), 2);
            auto b1 = from(a, 1, ra::ii({3}), 2);
            tr.test_eq(ra::start({26, 30, 34}), b0);
            tr.test_eq(ra::start({26, 30, 34}), b1);
            tr.test_eq(b0.dimv[0].len, b0.dimv[0].len);
            tr.test_eq(b0.dimv[0].step, b0.dimv[0].step);
        }
        {
            auto b0 = from(a, 1, ra::iota(ra::ic<3>), ra::ii(ra::ilist<2>));
            auto b1 = from(a, 1, ra::ii(ra::ilist<3>), ra::iota(ra::ic<2>));
            tr.test_eq(ra::Small<int, 3, 2> {{24, 25}, {28, 29}, {32, 33}}, b0);
            tr.test_eq(ra::Small<int, 3, 2> {{24, 25}, {28, 29}, {32, 33}}, b1);
            constexpr ra::dim_t l0 = b0.len(0);
            constexpr ra::dim_t l1 = b0.len(1);
            constexpr ra::dim_t s0 = b0.step(0);
            constexpr ra::dim_t s1 = b0.step(1);
            tr.test_eq(l0, b0.dimv[0].len);
            tr.test_eq(s0, b0.dimv[0].step);
            tr.test_eq(l1, b0.dimv[1].len);
            tr.test_eq(s1, b0.dimv[1].step);
            tr.test_eq(l0, b1.dimv[0].len);
            tr.test_eq(s0, b1.dimv[0].step);
            tr.test_eq(l1, b1.dimv[1].len);
            tr.test_eq(s1, b1.dimv[1].step);
        }
    }
    tr.section("unbeaten ViewBig<... RANK> (c)");
    {
        constexpr auto a = ra::ii({2, 3, 3, 2});
        constexpr auto ds = fromds(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        constexpr auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
// FIXME maybe forwarding issue (apparent with sanitizers on)
        // auto b = from(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = from(a, ra::all, i1, i2, ra::all);
        println(cout, "ds {:c} bv {:c} bv* {:c} rank(b) {}", ds, bv, b.op.dimv, rank(b));
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        from(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("unbeaten ViewBig<... RANK> (c) pure");
    {
        constexpr auto a = ra::ii({2, 3});
        constexpr auto ds = fromds(a, ra::Small<int, 2> {1, 0}, ra::Small<int, 2> {2, 1});
        constexpr auto bv = frombv(a, ra::Small<int, 2> {1, 0}, ra::Small<int, 2> {2, 1});
        auto i0 = ra::Small<int, 2> {1, 0};
        auto i1 = ra::Small<int, 2> {2, 1};
        auto b = from(a, i0, i1);
        println(cout, "ds {:c} bv {:c} bv* {:c} rank(b) {} b {:c}", ds, bv, b.op.dimv, rank(b), b);
        tr.strict().test_eq(u2, b);
    }
    tr.section("unbeaten ViewBig<... RANK> (nc)");
    {
        auto a = ra::ii({2, 3, 3, 2});
        constexpr auto ds = fromds(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        println(cout, "ds {:c} bv {:c}", ds, bv);
// FIXME maybe forwarding issue (apparent with sanitizers on)
        // auto b = from(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = from(a, ra::all, i1, i2, ra::all);
        println(cout, "ds {:c} bv {:c} bv* {:c} rank(b) {}", ds, bv, b.op.dimv, rank(b));
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        from(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("unbeaten ViewSmall<>");
    {
        auto a = ra::ii(ra::ilist<2, 3, 3, 2>);
        constexpr auto ds = fromds(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        constexpr auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        println(cout, "ds {:c} bv {:c}", ds, bv);
        tr.strict().test_eq(u0, from(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all));
// FIXME maybe forwarding issue (apparent with -O2 or higher)
        // auto b = from(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = from(a, ra::all, i1, i2, ra::all);
        println(cout, "ds {:c} bv {:c} bv* {:c} rank(b) {}", ds, bv, b.op.dimv, rank(b));
        tr.strict().test_eq(u0, b);
        tr.test_eq(2, b.len_s(0)); // spacer
        tr.test_eq(2, b.len_s(1)); // unbeaten index
        tr.test_eq(2, b.len_s(2)); // unbeaten index
        tr.test_eq(2, b.len_s(3)); // spacer

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        from(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("small from big");
    {
        auto a = ra::ii({2, 3, 3, 2});
        constexpr auto ds = fromds(a, ra::ii(ra::ilist<2>), ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::ii(ra::ilist<2>));
// FIXME ev. constexpr
        auto bv = frombv(a, ra::ii(ra::ilist<2>), ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::ii(ra::ilist<2>));
        println(cout, "ds {:c} bv {:c}", ds, bv);
    }
    tr.section("sanity");
    {
        auto i0 = ra::ic<2>;
        auto i1 = ra::ic<3>;
        static_assert(ra::is_constant<decltype(ra::cadd(i0, i1))> && 5==ra::cadd(i0, i1));
    }
    tr.section("unbeaten ViewSmall<> (over rank)");
    {
        auto a = ra::ii(ra::ilist<10>);
        ra::Small<int, 2, 3> b = from(a, ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}});
        tr.strict().test_eq(ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}}, b);
    }
    tr.section("unbeaten ViewBig<... RANK> (over rank)");
    {
        auto a = ra::ViewBig<ra::Seq<ra::dim_t>, 1>(ra::ii({10}));
        ra::Small<int, 2, 3> b = from(a, ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}});
        tr.strict().test_eq(ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}}, b);
    }
// general ANY cases don't work but two that do are: 1) all unbeaten, match rank
    tr.section("unbeaten ViewBig<... ANY> I");
    {
        auto a = ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({2, 3}));
        auto i0 = ra::Small<int, 2> {1, 0};
        auto i1 = ra::Small<int, 2> {2, 1};
        std::vector<ra::Dim> bv(ra::frombrank(a, i0, i1));
        std::array<int, int(!ra::beatable(i0)) + int(!ra::beatable(i1))> ds {};
        auto pl = fromb(0, ds.data(), bv, 0, a, 0, i0, i1);
        println(cout, "ds {:c} bv {:c} pl {}", ds, bv, pl);
        auto b = from(a, i0, i1);
        println(cout, "ds {:c} bv {:c} bv* {:c} b {:c}", ds, bv, ra::rank(b), b);
        tr.strict().test_eq(u2, b);
    }
// and 2) unbeaten match rank after beating
    tr.section("unbeaten ViewBig<... ANY> IIa");
    {
        auto i = ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}};
        auto b = from(ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({10})), i);
        ra::Small<int, 2, 3> c = b;
        println(cout, "c {:c}", c);
        tr.strict().test_eq(ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}}, c);
    }
    tr.section("unbeaten ViewBig<... ANY> IIb");
    {
        auto i = ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}};
        auto b = from(ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({10, 2})), i, 0);
        ra::Small<int, 2, 3> c = b;
        println(cout, "c {:c}", c);
        tr.strict().test_eq(ra::Small<int, 2, 3> {{6, 4, 2}, {8, 10, 12}}, c);
    }
    tr.section("not a view / generic outer product");
    {
        auto op = [](int a, int b) { return a-b; };
        ra::Big<int, 2> c = ra::from(op, ra::iota(3), ra::iota(4));
        tr.strict().test_eq(c, ra::Small<int, 3, 4> {{0, -1, -2, -3}, {1, 0, -1, -2}, {2, 1, 0, -1}});
    }
    tr.section("not a view / generic outer product, arbitrary number or args");
    {
        auto op = [](auto ... a) { return (3 + ... + a); };
        int c0 = ra::from(op);
        tr.strict().test_eq(3, c0);
        ra::Big<int, 1> c1 = ra::from(op, ra::iota(3, -3));
        tr.strict().test_eq(ra::Small<int, 3> {0, 1, 2}, c1);
        ra::Big<int, 2> c2 = ra::from(op, ra::iota(3, -3), ra::iota(3, -2));
        tr.strict().test_eq(ra::Small<int, 3, 3> {{-2, -1, 0}, {-1, 0, 1}, {0, 1, 2}}, c2);
        ra::Big<int, 3> c3 = ra::from(op, ra::iota(3, -3), ra::iota(3, -2), ra::iota(3, -1));
        tr.strict().test_eq(c2-1, c3(ra::dots<2>, 0));
        tr.strict().test_eq(c2+0, c3(ra::dots<2>, 1));
        tr.strict().test_eq(c2+1, c3(ra::dots<2>, 2));
    }
    return tr.summary();
}
