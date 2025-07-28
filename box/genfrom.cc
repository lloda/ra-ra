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

  TODO
* [X] unbeaten part
    [X] use as lvalue
    [ ] forwarding
* [ ] output should be as static as possible
* [ ] len in cp
* [ ] len in axes like in Ptr len/step

*/

#include "ra/test.hh"

using std::println, std::cout, std::endl, std::flush;

namespace ra {

template <int rank>
constexpr auto ii(dim_t (&&len)[rank], dim_t o, dim_t (&&step)[rank])
{
    return ViewBig<Seq<dim_t>, rank>(map([](dim_t len, dim_t step) { return Dim {len, step}; }, len, step), Seq<dim_t>{o});
}

template <int rank>
constexpr auto ii(dim_t (&&len)[rank], dim_t o=0)
{
    return ViewBig<Seq<dim_t>, rank>(len, Seq<dim_t>{o});
}

template <std::integral auto ... i>
constexpr auto ii(ra::ilist_t<i ...>)
{
    return ViewSmall<Seq<dim_t>, ra::ic_t<ra::default_dims(std::array<ra::dim_t, sizeof...(i)>{i...})>>(Seq<dim_t>{0});
}

template <class A> concept is_iota_static = requires (A a)
{
    []<class I, class Dimv>(ViewSmall<Seq<I>, Dimv> const &){}(a);
    requires UNB!=ra::size(a); // exclude UNB from beating to let B=A(... i ...) use B's len. FIXME
};

template <class A> concept is_iota_dynamic = requires (A a)
{
    []<class I, rank_t RANK>(ViewBig<Seq<I>, RANK> const &){}(a);
};

template <class A> concept is_iota_any = is_iota_dynamic<A> || is_iota_static<A>;

template<class Lambda, int=(Lambda{}(), 0)> constexpr bool is_constexpr(Lambda) { return true; }
constexpr bool is_constexpr(...) { return false; }

// ----------------------
// helpers to size bv/dst which cannot push_backed for constant eval'd froma().
// ----------------------

template <class I> consteval auto fsrc(I const &) { return 1; }
template <class I> constexpr auto fdst(I const & i) { if consteval { return rank_s(i); } else { return rank(i); } }
template <int n> consteval auto fsrc(dots_t<n> const &) { return n; }
template <int n> consteval auto fdst(dots_t<n> const &) { return n; }
template <int n> consteval auto fsrc(insert_t<n> const &) { return 0; }
template <int n> consteval auto fdst(insert_t<n> const &) { return n; }

consteval int
fromrank_s(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    constexpr int stretch = rank_s(a) - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i)));
    return ANY==rank_s(a) || ((ANY==fdst(i)) || ...)
        ?  ANY : stretch + (0 + ... + (UNB==fdst(i) ? 0 : fdst(i)));
}

constexpr int
fromrank(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    int stretch = rank(a) - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i)));
    return stretch + (0 + ... + (UNB==fdst(i) ? 0 : fdst(i)));
}

template <class I>
consteval bool
frombeatable(I && i)
{
    return (is_scalar_index<I> || is_iota_any<I>
            || (requires { []<int N>(dots_t<N>){}(i); })
            || (requires { []<int N>(insert_t<N>){}(i); }));
}

// ----------------------
// generic template
// ----------------------

constexpr auto
froma(dim_t place, auto && dst, int dk, auto && bv, int bk, auto && av, int ak)
{
    for (; ak<ra::size(av); ++ak, ++bk) {
        bv[bk] = av[ak];
    }
    return place;
}

template <class I0>
constexpr auto
froma(dim_t place, auto && dst, int dk, auto && bv, int bk, auto && av, int ak, I0 const & i0, auto const & ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        place += wlen(av[ak].len, i0)*av[ak].step;
        ++ak;
    } else if constexpr (is_iota_any<I0>) {
        for (int q=0; q<rank(i0); ++q) {
            bv[bk] = Dim {.len=i0.dimv[q].len, .step=i0.dimv[q].step*av[ak].step};
            place += wlen(av[ak].len, i0.cp).i * av[ak].step;
            ++bk;
        }
        ++ak;
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        int n = UNB!=i0.N ? i0.N : (ra::size(av) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
        for (int q=0; q<n; ++q) {
            bv[bk] = av[ak];
            ++ak; ++bk;
        }
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
        for (int q=0; q<i0.N; ++q) {
            bv[bk] = Dim {.len=UNB, .step=0};
            ++bk;
        }
    } else {
        dst[dk] = bk;
        bv[bk] = av[ak];
        ++ak; ++bk; ++dk;
    }
    return froma(place, dst, dk, bv, bk, av, ak, i ...);
}

// ----------------------
// pieces that are potentially constexpr or not
// ----------------------

constexpr auto
frombv(auto const & a, auto const & ...  i)
{
    std::array<Dim, fromrank_s(a, i ...)> bv {};
    froma(0, std::array<int, (0 + ... + int(!frombeatable(i)))> {}, 0, bv, 0, a.dimv, 0, i ...);
    return bv;
}

constexpr void
fromdstx(auto && dst, int dk, int bk, auto && av, int ak) {}

template <class I0>
constexpr void
fromdstx(auto && dst, int dk, int bk, auto && av, int ak, I0 const & i0, auto const & ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        ++ak;
    } else if constexpr (is_iota_any<I0>) {
        ++ak; bk += rank(i0);
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        int n = UNB!=i0.N ? i0.N : (ra::size(av) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
        ak += n; bk += n;
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
        bk += i0.N;
    } else {
        dst[dk] = bk;
        ++ak; ++bk; ++dk;
    }
    return fromdstx(dst, dk, bk, av, ak, i ...);
}

// FIXME why can't I reuse froma
constexpr auto
fromdst(auto const & a, auto const & ...  i)
{
    std::array<int, (0 + ... + int(!frombeatable(i)))> dst {};
    fromdstx(dst, 0, 0, a.dimv, 0, i ...);
    return dst;
}

constexpr auto
fromplace(dim_t place, auto && av, int ak) { return place; }

template <class I0>
constexpr auto
fromplace(dim_t place, auto && av, int ak, I0 const & i0, auto const & ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        place += wlen(av[ak].len, i0)*av[ak].step;
        ++ak;
    } else if constexpr (is_iota_any<I0>) {
        for (int q=0; q<rank(i0); ++q) {
            place += wlen(av[ak].len, i0.cp).i * av[ak].step;
        }
        ++ak;
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        ak += UNB!=i0.N ? i0.N : (ra::size(av) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
    } else {
        ++ak;
    }
    return fromplace(place, av, ak, i ...);
}

// ----------------------
// unbeaten
// ----------------------

template <class I, int drop>
constexpr decltype(auto)
from_partialx(auto && op)
{
    if constexpr (drop==mp::len<I>) {
        return RA_FW(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, ic_t<0>>, mp::drop<I, drop>> {},
                     from_partialx<I, drop+1>(RA_FW(op)));
    }
}

constexpr auto
spacer(auto && bv, auto && p, auto && q)
{
    // static_assert(!is_constant<decltype(bv)>); // FIXME use ic<len> here if bc is constant
    return std::apply([&bv](auto ... i) { return std::make_tuple(ra::iota(bv[i].len) ...); }, mp::iota<q-p, p>{});
}

template <class B>
constexpr decltype(auto)
fromux(B && b, auto && dst, auto && c, auto && ti)
{
    return std::apply([&b](auto && ... i) {
        return map(from_partialx<mp::tuple<ic_t<rank_s(i)> ...>, 1>(std::forward<B>(b)), RA_FW(i) ...);
    }, RA_FW(ti));
}

template <class B>
constexpr decltype(auto)
fromu(B && b, auto && dst, auto && c, auto && ti)
{
    return fromux(RA_FW(b), dst, c,
                  std::tuple_cat(RA_FW(ti),
                                 spacer(b.dimv, ic<(0==c ? 0 : 1+dst()[c-1])>, ic<ra::size(b.dimv)>)));
}

template <class B>
constexpr decltype(auto)
fromu(B && b, auto && dst, auto && c, auto && ti, auto && i0, auto && ... i)
{
    if constexpr (frombeatable(i0)) {
        return fromu(RA_FW(b), dst, c, RA_FW(ti), RA_FW(i) ...);
    } else {
        return fromu(RA_FW(b), dst, ic<c+1>,
                     std::tuple_cat(RA_FW(ti),
                                    spacer(b.dimv, ic<(0==c ? 0 : 1+dst()[c-1])>, ic<dst()[c]>),
                                    forward_as_tuple(RA_FW(i0))),
                     RA_FW(i) ...);
    }
}

// driver

constexpr decltype(auto)
froma0(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    constexpr int brs = fromrank_s(a, i ...);
    auto bv = [&]{ if constexpr (ANY!=brs) return std::array<Dim, brs> {}; else return std::vector<Dim>(fromrank(a, i ...)); }();
    std::array<int, (0 + ... + int(!frombeatable(i)))> dst {};
// don't forward since args aren't retained
    auto place = froma(0, dst, 0, bv, 0, a.dimv, 0, i ...);
    assert(place==fromplace(0, a.dimv, 0, i ...)); // remove eventually
    ViewBig<decltype(a.data()), brs> beaten(std::move(bv), a.cp + place);
    if constexpr (0==dst.size()) {
        return beaten;
// FIXME do forward, since args are retained in map
    } else if constexpr (brs==ANY){
        println(cout, "Unbeaten subscripts not supported with var rank result");
        return beaten; // FIXME error, reframe can't handle var rank
    } else {
        constexpr auto dst = fromdst(a, i ...);
        return fromu(std::move(beaten), ic<dst>, ic<0>, std::tuple<> {}, RA_FW(i) ...);
    }
}


// ----------------------
// output is fixed size. TODO
// ----------------------

}; // namespace ra

int main()
{
    ra::TestRecorder tr(cout);
    {
        println(cout, "\nii([3, 4])\n{:c}", ra::ii({3, 4}));
        println(cout, "\nii([3, 4], -3, [1, 10])\n{:c}", ra::ii({3, 4}, -3, {1, 10}));
        println(cout, "\nii(<3, 4>)\n{:c}", ra::ii(ra::ilist<3, 4>));
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
    constexpr ra::Small<int, 2, 3, 3, 2> u1 ={{{{0, 0}, {2, 3}, {0, 0}},
                                               {{6, 7}, {8, 9}, {10, 11}},
                                               {{0, 0}, {14, 15}, {0, 0}}},
                                              {{{0, 0}, {20, 21}, {0, 0}},
                                               {{24, 25}, {26, 27}, {28, 29}},
                                               {{0, 0}, {32, 33}, {0, 0}}}};

    auto rank_maybe = [](bool s, auto && a, auto && ... i) { return s ? fromrank_s(a, i ...) : fromrank(a, i...); };
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
        tr.strict().info("a0").test_eq(a0, froma0(a));
        tr.strict().test_eq(a1, froma0(a, ra::ii({2}), ra::ii({3})));
        tr.strict().test_eq(a2, froma0(a, 1, ra::ii({3})));
        tr.strict().test_eq(a3, froma0(a, ra::ii({2}), 1));
        tr.strict().test_eq(a4, froma0(a, 1, 1));
        tr.strict().test_eq(a5, froma0(a, 1));
        tr.strict().test_eq(a6, froma0(a, ra::ii({2})));
        tr.strict().test_eq(a7, froma0(a, ra::all, ra::ii({2, 2})));
        tr.strict().test_eq(a8, froma0(a, ra::dots<1>, 1));
        tr.strict().test_eq(a9, froma0(a, 1, ra::dots<1>));
        tr.strict().test_eq(aA, shape(froma0(a, ra::dots<1>, ra::insert<1>)));
    };
    auto testa1 = [&](auto && b)
    {
        tr.strict().info("a1").test_eq(b0, froma0(b));
        tr.strict().test_eq(b1, froma0(b, 1, ra::dots<>));
        tr.strict().test_eq(b2, froma0(b, ra::dots<>, 1));
        tr.strict().test_eq(b3, froma0(b, 1, ra::dots<>, 1));
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
        // static_assert(every(a0==a)); // FIXME
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
    tr.section("unbeaten ViewBig<... RANK> (c)");
    {
        constexpr auto a = ra::ii({2, 3, 3, 2});
        constexpr auto dst = fromdst(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        constexpr auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
// FIXME maybe forwarding issue (apparent with sanitizers on)
        // auto b = froma0(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = froma0(a, ra::all, i1, i2, ra::all);
        println(cout, "dst {:c} bv {:c} bv* {:c} rank(b) {}", dst, bv, b.op.dimv, rank(b));
        println(cout, "a(...)\n{:c}", b);
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        froma0(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("unbeaten ViewBig<... RANK> (nc)");
    {
        auto a = ra::ii({2, 3, 3, 2});
        constexpr auto dst = fromdst(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        println(cout, "dst {:c} bv {:c}", dst, bv);
// FIXME maybe forwarding issue (apparent with sanitizers on)
        // auto b = froma0(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = froma0(a, ra::all, i1, i2, ra::all);
        println(cout, "dst {:c} bv {:c} bv* {:c} rank(b) {}", dst, bv, b.op.dimv, rank(b));
        println(cout, "a(...)\n{:c}", b);
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        froma0(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("unbeaten ViewSmall<>");
    {
        auto a = ra::ii(ra::ilist<2, 3, 3, 2>);
        constexpr auto dst = fromdst(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        constexpr auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        println(cout, "dst {:c} bv {:c}", dst, bv);
        println(cout, "a(...)\n{:c}", froma0(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all));
// FIXME maybe forwarding issue (apparent with -O2 or higher)
        // auto b = froma0(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = froma0(a, ra::all, i1, i2, ra::all);
        println(cout, "dst {:c} bv {:c} bv* {:c} rank(b) {}", dst, bv, b.op.dimv, rank(b));
        println(cout, "a(...)\n{:c}", b);
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        froma0(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    return tr.summary();
}
