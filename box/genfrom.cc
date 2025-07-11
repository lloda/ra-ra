// -*- mode: c++; coding: utf-8 -*-
// ek/box - A properly general version of view(...)

// (c) Daniel Llorens - 2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// Atm view(i ...) only beats i of rank ≤1 and only when all i are beatable. This is a sandbox for a version that beats any combination of scalar or view<Seq> of any rank, and only returns an expr for the unbeatable subscripts.
// There is an additional issue that eg A2(unbeatable-i1) (indicating ranks) returns a nested expression, that is, a rank-1 expr where each element is a rank-1 view. It should instead be rank 2 and not nested, iow, the value type of the result should always be the same as that of A.

// TODO
// [ ] unbeaten part
// [ ] output should be as static as possible
// [ ] len in cp
// [ ] len in axes like in Ptr len/step

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

template<class Lambda, int=(Lambda{}(), 0)>
constexpr bool is_constexpr(Lambda) { return true; }
constexpr bool is_constexpr(...) { return false; }

// ----------------------
// just static rank. When rank is dynamic we don't need to compute it separately.
// ----------------------

template <class I> consteval auto fsrc(I const &) { return 1; }
template <class I> constexpr auto fdst(I const & i) { if consteval { return rank_s(i); } else { return rank(i); } }
template <int n> consteval auto fsrc(dots_t<n> const &) { return n; }
template <int n> consteval auto fdst(dots_t<n> const &) { return n; }
template <int n> consteval auto fsrc(insert_t<n> const &) { return 0; }
template <int n> consteval auto fdst(insert_t<n> const &) { return n; }

constexpr int
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


// ----------------------
// dst is var rank if A or I are var rank, else it's fixed rank.
// FIXME unbeaten args don't work
// ----------------------

constexpr auto
froma(auto && dst, dim_t place, auto && bv, int bk, auto && av, int ak)
{
    for (; ak<ra::size(av); ++ak, ++bk) {
        bv[bk] = av[ak];
    }
    return std::make_tuple(RA_FW(dst), place);
}

template <class I0, class ... I>
constexpr auto
froma(auto && dst, dim_t place, auto && bv, int bk, auto && av, int ak,
      I0 && i0, I && ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        place += wlen(av[ak].len, RA_FW(i0))*av[ak].step;
        ++ak;
    } else if constexpr (is_iota_any<I0>) {
        for (int q=0; q<rank(i0); ++q) {
            bv[bk] = Dim {.len=i0.dimv[q].len, .step=i0.dimv[q].step*av[ak].step};
            place += wlen(av[ak].len, RA_FW(i0.cp)).i * av[ak].step;
            ++bk;
        }
        ++ak;
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        int n = UNB!=i0.N ? i0.N : (ra::size(av) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
        for (int q=0; q<n; ++q) {
            bv[bk] = av[ak];
            ++bk;
            ++ak;
        }
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
        for (int q=0; q<i0.N; ++q) {
            bv[bk] = Dim {.len=UNB, .step=0};
            ++bk;
        }
    } else {
        dst.push_back(bk);
        bv[bk] = av[ak];
        ++ak;
        ++bk;
    }
    return froma(RA_FW(dst), place, bv, bk, av, ak, RA_FW(i) ...);
}

constexpr auto
froma0(auto const & a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    constexpr int brs = fromrank_s(a, i ...);
    auto bv = [&]{ if constexpr (ANY!=brs) return std::array<Dim, brs> {}; else return std::vector<Dim>(fromrank(a, i ...)); }();
    auto [dst, place] = froma(std::vector<int> {}, 0, bv, 0, a.dimv, 0, RA_FW(i) ...);
    if (0==dst.size()) {
        return ViewBig<decltype(a.cp), brs>(std::move(bv), a.cp + place);
    } else {
        println(cout, "\ndst {:c} bv {:c}", dst, bv);
        return ViewBig<decltype(a.cp), brs>(std::move(bv), a.cp + place); // FIXME anything while I work it out
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

    tr.section("ViewSmall<>");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 4>);
        static_assert(every(a0==a));
        testa0(a);
        testa1(ra::ii(ra::ilist<3, 2, 3>));
        testrank0(true, a);
        testrank1(true, ra::ii(ra::ilist<3, 2, 3>));
    }
    tr.section("ViewBig<... RANK>");
    {
        constexpr auto a = ra::ii({3, 4});
        // static_assert(every(a0==a)); // FIXME
        testa0(a);
        testa1(ra::ii({3, 2, 3}));
        testrank0(true, a);
        testrank1(true, ra::ii({3, 2, 3}));
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
    tr.section("unbeaten");
    {
        constexpr auto a = ra::ii({2, 3, 3, 2});
        froma0(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
    }
    return tr.summary();
}
