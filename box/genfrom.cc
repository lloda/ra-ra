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
* [X] op is view or not (and don't try to beat)
* [ ] be good enough to replace indexer
  - [ ] index checks
* [X] unbeaten part
  - [X] use as lvalue
  - [ ] forwarding
* [ ] output should be as static as possible
  - [X] static spacer len where possible
  - [X] beaten SmallView for typical cases
  - [ ] beaten SmallView for every possible case
* [X] len in is_scalar_index
* [X] len in unbeaten subscript
* [X] len in ptr (not static, but that didn't work before either)
* [ ] len in general iota

*/

#include "ra/test.hh"
#include "test/mpdebug.hh"

using std::println, std::cout, std::endl, std::flush;

namespace ra {

template <int rank>
constexpr auto ii(dim_t (&&len)[rank], dim_t o, dim_t (&&step)[rank])
{
    return ViewBig<Seq<dim_t>, rank>(map([](dim_t len, dim_t step){ return Dim {len, step}; }, len, step), Seq<dim_t>{o});
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

template <class A>
concept is_iota_static = requires (A a)
{
    []<class I, class Dimv>(ViewSmall<Seq<I>, Dimv> const &){}(a);
    requires UNB!=ra::size(a); // exclude UNB from beating to let B=A(... i ...) use B's len. FIXME
};

template <class A>
concept is_iota_dynamic = requires (A a)
{
    []<class I, rank_t RANK>(ViewBig<Seq<I>, RANK> const &){}(a);
};
template <class A> concept is_iota_any = is_iota_dynamic<A> || is_iota_static<A> || is_iota<A>;

template<class Lambda, int=(Lambda{}(), 0)> constexpr bool is_constexpr(Lambda) { return true; }
constexpr bool is_constexpr(...) { return false; }

// ----------------------
// helpers to size bv/dst which cannot push_backed for constant eval'd froma().
// ----------------------

template <class I>
consteval bool
frombeatable(I && i)
{
    return ((requires { []<int N>(dots_t<N>){}(i); }) || (requires { []<int N>(insert_t<N>){}(i); })
            || is_scalar_index<I> || is_iota_any<I>);
}

template <class I> consteval auto fsrc(I const &) { return 1; }
template <class I> constexpr auto fdst(I const & i) { if constexpr (frombeatable(i)) { if consteval { return rank_s(i); } else { return rank(i); } } else { return 1; } }
template <int n> consteval auto fsrc(dots_t<n> const &) { return n; }
template <int n> consteval auto fdst(dots_t<n> const &) { return n; }
template <int n> consteval auto fsrc(insert_t<n> const &) { return 0; }
template <int n> consteval auto fdst(insert_t<n> const &) { return n; }

consteval int
frombrank_s(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    constexpr int stretch = rank_s(a) - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i)));
    return ANY==rank_s(a) || ((ANY==fdst(i)) || ...) ?  ANY : stretch + (0 + ... + (UNB==fdst(i) ? 0 : fdst(i)));
}

constexpr int
frombrank(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    int stretch = rank(a) - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i)));
    return stretch + (0 + ... + (UNB==fdst(i) ? 0 : fdst(i)));
}

// ----------------------
// beaten
// ----------------------

template <bool dopl=true, bool dobv=true>
constexpr auto
fromb(dim_t pl, auto && dst, int dk, auto && bv, int bk, auto && a, int ak)
{
    if constexpr  (dobv) {
        for (; ak<ra::rank(a); ++ak, ++bk) {
            bv[bk] = a.dimv[ak];
        }
    }
    return pl;
}

template <bool dopl=true, bool dobv=true, class I0>
constexpr auto
fromb(dim_t pl, auto && dst, int dk, auto && bv, int bk, auto && a, int ak, I0 const & i0, auto const & ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        if constexpr (dopl) { pl += wlen(a.len(ak), i0)*a.step(ak); }
        ++ak;
    } else if constexpr (is_iota_any<I0>) {
        for (int q=0; q<rank(i0); ++q) {
            if constexpr (dobv) { bv[bk] = Dim { .len=wlen(a.len(ak), i0).len(q), .step=wlen(a.len(ak), i0).step(q)*a.step(ak) }; }
            if constexpr (dopl) { pl += wlen(a.len(ak), i0).cp.i * a.step(ak); }
            ++bk;
        }
        ++ak;
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        int n = UNB!=i0.N ? i0.N : (ra::rank(a) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
        for (int q=0; q<n; ++q) {
            if constexpr (dobv) { bv[bk] = a.dimv[ak]; }
            ++ak; ++bk;
        }
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
        for (int q=0; q<i0.N; ++q) {
            if constexpr (dobv) { bv[bk] = Dim {.len=UNB, .step=0}; }
            ++bk;
        }
    } else {
        dst[dk] = bk;
        if constexpr (dobv) { bv[bk] = a.dimv[ak]; }
        ++ak; ++bk; ++dk;
    }
    return fromb<dopl, dobv>(pl, dst, dk, bv, bk, a, ak, i ...);
}

// ----------------------
// potentially constexpr fromb pieces
// ----------------------

constexpr auto
frombv(auto const & a, auto const & ...  i)
{
    std::array<Dim, frombrank_s(a, i ...)> bv {};
    fromb<false>(0, std::array<int, (0 + ... + int(!frombeatable(i)))> {}, 0, bv, 0, a, 0, i ...);
    return bv;
}

constexpr auto
fromdst(auto const & a, auto const & ...  i)
{
    std::array<int, (0 + ... + int(!frombeatable(i)))> dst {};
    std::array<Dim, frombrank_s(a, i ...)> bv;
    fromb<false, false>(0, dst, 0, bv, 0, a, 0, i ...);
    return dst;
}

constexpr auto
frompl(dim_t pl, auto && a, int ak) { return pl; }

template <class I0>
constexpr auto
frompl(dim_t pl, auto && a, int ak, I0 const & i0, auto const & ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        pl += wlen(a.len(ak), i0)*a.step(ak);
        ++ak;
    } else if constexpr (is_iota_any<I0>) {
        for (int q=0; q<rank(i0); ++q) {
            pl += wlen(a.len(ak), i0).cp.i * a.step(ak);
        }
        ++ak;
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        ak += UNB!=i0.N ? i0.N : (ra::rank(a) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
    } else {
        ++ak;
    }
    return frompl(pl, a, ak, i ...);
}

// ----------------------
// unbeaten
// ----------------------

constexpr auto
spacer(auto && b, auto && p, auto && q)
{
    return std::apply([&b](auto ... i){ return std::make_tuple(ra::iota(maybe_len(b, i)) ...); }, mp::iota<q-p, p>{});
}

template <class II, int drop>
constexpr decltype(auto)
fromu_loop(auto && op) // same as old from_partial
{
    if constexpr (drop==mp::len<II>) {
        return RA_FW(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, ic_t<0>>, mp::drop<II, drop>> {},
                     fromu_loop<II, drop+1>(RA_FW(op)));
    }
}

template <class B>
constexpr decltype(auto)
fromu(B && b, auto && dst, auto && c, auto && ti)
{
    return std::apply([&b](auto && ... i){
        return map(fromu_loop<mp::tuple<ic_t<rank_s(i)> ...>, 1>(std::forward<B>(b)), RA_FW(i) ...);
    }, std::tuple_cat(RA_FW(ti), spacer(b, ic<(0==c ? 0 : 1+dst()[c-1])>, ic<ra::size(b.dimv)>)));
}

template <class B>
constexpr decltype(auto)
fromu(B && b, auto && dst, auto && c, auto && ti, auto && i0, auto && ... i)
{
    if constexpr (frombeatable(i0)) {
        return fromu(RA_FW(b), dst, c, RA_FW(ti), RA_FW(i) ...);
    } else {
        return fromu(RA_FW(b), dst, ic<c+1>,
                     std::tuple_cat(RA_FW(ti), spacer(b, ic<(0==c ? 0 : 1+dst()[c-1])>, ic<dst()[c]>),
                                    std::forward_as_tuple(wlen(b.len(dst()[c]), RA_FW(i0)))),
                     RA_FW(i) ...);
    }
}

// ----------------------
// driver
// ----------------------

// only forward to the unbeaten part (fromu)

constexpr decltype(auto)
froma(auto && a, auto && ...  i)
{
    constexpr int nscalars = (0 + ... + is_scalar_index<decltype(i)>);
    if constexpr (Slice<decltype(a)>) {
        static_assert((0 + ... + (UNB==fdst(i)))<=1);
        constexpr int brs = frombrank_s(a, i ...);
// scalar instead of rank 0 view
        if constexpr (rank_s(a)==nscalars) {
            return *(a.cp + frompl(0, a, 0, i ...));
        } else if constexpr (ANY!=brs) {
            constexpr auto dst = fromdst(a, i ...);
            auto beaten = [&]{
// FIXME sufficient but not necessary, e.g. consider big to small cases
                if constexpr (ANY!=ra::size_s(a) && ((!frombeatable(i) /* || has_len<decltype(i)> */ || ANY!=ra::size_s(i)) && ...)) {
                    constexpr auto bv = ic<frombv(a, i ...)>;
                    return ViewSmall<decltype(a.data()), decltype(bv)>(a.cp + frompl(0, a, 0, i ...));
                } else {
                    ViewBig<decltype(a.data()), brs> b;
                    std::array<int, (0 + ... + int(!frombeatable(i)))> dst_ {}; // FIXME wasted
                    b.cp = a.cp + fromb(0, dst_, 0, b.dimv, 0, a, 0, i ...);
                    return b;
                }
            };
            if constexpr (0==dst.size()) {
                return beaten();
            } else {
                return fromu(beaten(), ic<dst>, ic<0>, std::tuple<> {}, RA_FW(i) ...);
            }
        } else {
            std::vector<Dim> bv(frombrank(a, i ...));
            std::array<int, (0 + ... + int(!frombeatable(i)))> dst {};
            auto pl = fromb(0, dst, 0, bv, 0, a, 0, i ...);
            if constexpr (0==dst.size()) {
                return ViewBig<decltype(a.data()), brs>(std::move(bv), a.cp + pl);
            } else {
                RA_CK(1==dst.size() && dst[0]==0 && 1==bv.size(), "Run time reframe dst ", fmt(lstyle, dst), " bv ", fmt(lstyle, bv));
                ViewBig<decltype(a.data()), 1> b1(bv, a.cp + pl);
                return fromu(std::move(b1), ic<std::array<int, 1>{0}>, ic<0>, std::tuple<> {}, RA_FW(i) ...);
            }
        }
    } else {
        if constexpr (0==sizeof...(i)) { // FIXME worth it?
            return RA_FW(a)();
        } else if constexpr (1==sizeof...(i)) { // FIXME worth it?
            return map(RA_FW(a), RA_FW(i) ...);
        } else {
            return map(fromu_loop<mp::tuple<ic_t<rank_s(i)> ...>, 1>(RA_FW(a)), RA_FW(i) ...);
        }
    }
}

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
        tr.strict().info("a0").test_eq(a0, froma(a));
        tr.strict().test_eq(a1, froma(a, ra::ii({2}), ra::ii({3})));
        tr.strict().test_eq(a2, froma(a, 1, ra::ii({3})));
        tr.strict().test_eq(a3, froma(a, ra::ii({2}), 1));
        tr.strict().test_eq(a4, froma(a, 1, 1));
        tr.strict().test_eq(a5, froma(a, 1));
        tr.strict().test_eq(a6, froma(a, ra::ii({2})));
        tr.strict().test_eq(a7, froma(a, ra::all, ra::ii({2, 2})));
        tr.strict().test_eq(a8, froma(a, ra::dots<1>, 1));
        tr.strict().test_eq(a9, froma(a, 1, ra::dots<1>));
        tr.strict().test_eq(aA, shape(froma(a, ra::dots<1>, ra::insert<1>)));
        tr.strict().test_eq(11, froma(a, 2, 3));
        tr.test(ra::ANY==rank_s(a) || std::is_integral_v<decltype(froma(a, 2, 3))>);
        tr.test(ra::ANY==rank_s(a) || std::is_integral_v<decltype(froma(a, ra::len-1, 3))>);
    };
    auto testa1 = [&](auto && b)
    {
        tr.strict().info("a1").test_eq(b0, froma(b));
        tr.strict().test_eq(b1, froma(b, 1, ra::dots<>));
        tr.strict().test_eq(b2, froma(b, ra::dots<>, 1));
        tr.strict().test_eq(b3, froma(b, 1, ra::dots<>, 1));
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
        auto b0 = froma(a, ra::len-1, ra::iota(ra::ic<3>), ra::len/2);
        auto b1 = froma(a, 2, ra::iota(ra::ic<3>), 2);
        tr.test_eq(ra::start({50, 54, 58}), b0);
        tr.test_eq(ra::start({50, 54, 58}), b1);
        tr.test_eq(3, b0.len_s(0));
        tr.test_eq(3, b1.len_s(0));
    }
    tr.section("len in unbeaten subscript");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 4, 3>);
// FIXME forwarding :-/
        // auto b0 = froma(a, 2, ra::Small<int, 2>{0, 1} + ra::len/2, 2);
        auto b1 = froma(a, 2, ra::iota(ra::ic<2>, 2), 2);
        tr.test_eq(ra::start({32, 35}), froma(a, 2, ra::Small<int, 2>{0, 1} + ra::len/2, 2));
        tr.test_eq(ra::start({32, 35}), b1);
        tr.test_eq(2, froma(a, 2, ra::Small<int, 2>{0, 1} + ra::len/2, 2).len_s(0));
        tr.test_eq(2, b1.len_s(0));
    }
    tr.section("len in iota");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 4, 3>);
        auto b0 = froma(a, 2, ra::iota(ra::len, 0), 2);
        auto b1 = froma(a, 2, ra::iota(ra::ic<4>, 0), 2);
        tr.test_eq(ra::start({26, 29, 32, 35}), b0);
        tr.test_eq(ra::start({26, 29, 32, 35}), b1);
        tr.test_eq(4, b0.len_s(0)); // FIXME
        tr.test_eq(4, b1.len_s(0));
        auto z = a(2, ra::iota(ra::len, 0), 2); // it also failed before
        tr.test_eq(4, z.len_s(0));
    }
    tr.section("Ptr works as rank 1 iota");
    {
        constexpr auto a = ra::ii(ra::ilist<3, 6, 4>);
        {
            auto b0 = froma(a, 1, ra::iota(3), 2);
            auto b1 = froma(a, 1, ra::ii({3}), 2);
            tr.test_eq(ra::start({26, 30, 34}), b0);
            tr.test_eq(ra::start({26, 30, 34}), b1);
            tr.test_eq(b0.dimv[0].len, b0.dimv[0].len);
            tr.test_eq(b0.dimv[0].step, b0.dimv[0].step);
        }
        {
            auto b0 = froma(a, 1, ra::iota(ra::ic<3>), ra::ii(ra::ilist<2>));
            auto b1 = froma(a, 1, ra::ii(ra::ilist<3>), ra::iota(ra::ic<2>));
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
        constexpr auto dst = fromdst(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        constexpr auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
// FIXME maybe forwarding issue (apparent with sanitizers on)
        // auto b = froma(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = froma(a, ra::all, i1, i2, ra::all);
        println(cout, "dst {:c} bv {:c} bv* {:c} rank(b) {}", dst, bv, b.op.dimv, rank(b));
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        froma(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("unbeaten ViewBig<... RANK> (nc)");
    {
        auto a = ra::ii({2, 3, 3, 2});
        constexpr auto dst = fromdst(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        println(cout, "dst {:c} bv {:c}", dst, bv);
// FIXME maybe forwarding issue (apparent with sanitizers on)
        // auto b = froma(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = froma(a, ra::all, i1, i2, ra::all);
        println(cout, "dst {:c} bv {:c} bv* {:c} rank(b) {}", dst, bv, b.op.dimv, rank(b));
        tr.strict().test_eq(u0, b);

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        froma(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("unbeaten ViewSmall<>");
    {
        auto a = ra::ii(ra::ilist<2, 3, 3, 2>);
        constexpr auto dst = fromdst(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        constexpr auto bv = frombv(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        println(cout, "dst {:c} bv {:c}", dst, bv);
        tr.strict().test_eq(u0, froma(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all));
// FIXME maybe forwarding issue (apparent with -O2 or higher)
        // auto b = froma(a, ra::all, ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::all);
        auto i1 = ra::Small<int, 2> {0, 2};
        auto i2 = ra::Small<int, 2> {0, 2};
        auto b = froma(a, ra::all, i1, i2, ra::all);
        println(cout, "dst {:c} bv {:c} bv* {:c} rank(b) {}", dst, bv, b.op.dimv, rank(b));
        tr.strict().test_eq(u0, b);
        tr.test_eq(2, b.len_s(0)); // spacer
        tr.test_eq(2, b.len_s(1)); // unbeaten index
        tr.test_eq(2, b.len_s(2)); // unbeaten index
        tr.test_eq(2, b.len_s(3)); // spacer

        auto c = concrete(a); // seq is read only
        c(0, 0, 0, 0) = 99;
        froma(c, ra::all, i1, i2, ra::all) = 0;
        tr.strict().test_eq(u1, c);
    }
    tr.section("small from big");
    {
        auto a = ra::ii({2, 3, 3, 2});
        constexpr auto dst = fromdst(a, ra::ii(ra::ilist<2>), ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::ii(ra::ilist<2>));
// FIXME ev. constexpr
        auto bv = frombv(a, ra::ii(ra::ilist<2>), ra::Small<int, 2> {0, 2}, ra::Small<int, 2> {0, 2}, ra::ii(ra::ilist<2>));
        println(cout, "dst {:c} bv {:c}", dst, bv);
    }
    tr.section("sanity");
    {
        auto i0 = ra::ic<2>;
        auto i1 = ra::ic<3>;
        static_assert(ra::is_constant<decltype(ra::cadd(i0, i1))> && 5==ra::cadd(i0, i1));
    }
    tr.section("not a view / generic outer product");
    {
        auto op = [](int a, int b) { return a-b; };
        ra::Big<int, 2> c = ra::froma(op, ra::iota(3), ra::iota(4));
        tr.strict().test_eq(c, ra::Small<int, 3, 4> {{0, -1, -2, -3}, {1, 0, -1, -2}, {2, 1, 0, -1}});
    }
    tr.section("unbeaten ViewSmall<> (over rank)");
    {
        auto a = ra::ii(ra::ilist<10>);
        ra::Small<int, 2, 3> b = froma(a, ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}});
        tr.strict().test_eq(ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}}, b);
    }
    tr.section("unbeaten ViewBig<... RANK> (over rank)");
    {
        auto a = ra::ViewBig<ra::Seq<ra::dim_t>, 1>(ra::ii({10}));
        ra::Small<int, 2, 3> b = froma(a, ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}});
        tr.strict().test_eq(ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}}, b);
    }
// general cases don't work, but those that don't need wrank do.
    tr.section("unbeaten ViewBig<... ANY>");
    {
        auto a = ra::ViewBig<ra::Seq<ra::dim_t>, ra::ANY>(ra::ii({10}));
        ra::Small<int, 2, 3> b = froma(a, ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}});
        tr.strict().test_eq(ra::Small<int, 2, 3> {{3, 2, 1}, {4, 5, 6}}, b);
    }
    return tr.summary();
}
