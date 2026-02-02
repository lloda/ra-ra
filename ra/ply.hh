// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression traversal and slicer.

// (c) Daniel Llorens - 2013-2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

// TODO Parametrize traversal order, some ops (e.g. output, ravel) require specific orders.
// TODO Better traversal. Tiling, etc. (see eval.cc in Blitz++). Unit step case?
// TODO std::execution::xxx-policy TODO Validate output argument strides.

#pragma once
#include "expr.hh"

namespace ra {

struct Nop {};

// run time order/rank.

template <Iterator A, class Early = Nop>
constexpr auto
ply_ravel(A && a, Early && early = Nop {})
{
    validate(a);
    rank_t rank = ra::rank(a);
    if (0>=rank) {
        assert(0==rank);
        if constexpr (requires {early.def;}) {
            return (*a).value_or(early.def);
        } else {
            *a; return;
        }
    }
// inside first. FIXME better heuristic - but first need a way to force row-major
    struct z_t { rank_t order; dim_t sha, ind=0; };
    thread_local std::vector<z_t> z(4);
    z.resize(rank);
    for (rank_t i=0; i<rank; ++i) {
        z[i].order = rank-1-i;
    }
// find outermost compact dim.
    auto ocd = z.begin();
    dim_t ss = a.len(ocd->order);
    for (--rank, ++ocd; rank>0 && a.keep(ss, z[0].order, ocd->order); --rank, ++ocd) {
        ss *= a.len(ocd->order);
    }
    for (int k=0; k<rank; ++k) {
// ss takes care of the raveled dimensions ss.
        if (0>=(z[k].sha=a.len(ocd[k].order))) {
            assert(0==z[k].sha);
            if constexpr (requires {early.def;}) {
                return early.def;
            } else {
                return;
            }
        }
    }
    auto ss0 = a.step(z[0].order);
    for (;;) {
        auto place = a.save();
        for (dim_t s=ss; --s>=0; a.mov(ss0)) {
            if constexpr (requires {early.def;}) {
                if (auto stop = *a) {
                    return stop.value();
                }
            } else {
                *a;
            }
        }
        a.load(place); // FIXME wasted if k=0. Cf test/iota.cc
        for (int k=0; ; ++k) {
            if (k>=rank) {
                if constexpr (requires {early.def;}) {
                    return early.def;
                } else {
                    return;
                }
            } else if (++z[k].ind<z[k].sha) {
                a.adv(ocd[k].order, 1);
                break;
            } else {
                z[k].ind = 0;
                a.adv(ocd[k].order, 1-z[k].sha);
            }
        }
    }
}

// compile time order/rank.

template <auto order, int k, int urank, class S, class Early>
constexpr auto
subply(Iterator auto & a, dim_t s, S const & ss0, Early & early)
{
    if constexpr (k < urank) {
        auto place = a.save();
        for (; --s>=0; a.mov(ss0)) {
            if constexpr (requires {early.def;}) {
                if (auto stop = *a) {
                    return stop;
                }
            } else {
                *a;
            }
        }
        a.load(place); // FIXME wasted if k was 0 at the top
    } else {
        dim_t size = a.len(order[k]); // TODO precompute above
        for (dim_t i=0; i<size; ++i) {
            if constexpr (requires {early.def;}) {
                if (auto stop = subply<order, k-1, urank>(a, s, ss0, early)) {
                    return stop;
                }
            } else {
                subply<order, k-1, urank>(a, s, ss0, early);
            }
            a.adv(order[k], 1);
        }
        a.adv(order[k], -size);
    }
    if constexpr (requires {early.def;}) {
        return static_cast<decltype(*a)>(std::nullopt);
    } else {
        return;
    }
}

template <class Early = Nop>
constexpr decltype(auto)
ply_fixed(Iterator auto && a, Early && early = Nop {})
{
    validate(a);
    constexpr rank_t rank = rank_s(a);
    static_assert(0<=rank, "ply_fixed requires static rank");
// inside first. FIXME better heuristic - but first need a way to force row-major
    constexpr auto order = std::apply([](auto ... i){ return std::array<int, rank>{(rank-1-i) ...}; }, mp::iota<rank>{});
    if constexpr (0==rank) {
        if constexpr (requires {early.def;}) {
            return (*a).value_or(early.def);
        } else {
            *a; return;
        }
    } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Warray-bounds"
    auto ss0 = a.step(order[0]); // gcc 14.1 with RA_CHECK=0 and sanitizer on
#pragma GCC diagnostic pop
        if constexpr (requires {early.def;}) {
            return (subply<order, rank-1, 1>(a, a.len(order[0]), ss0, early)).value_or(early.def);
        } else {
            subply<order, rank-1, 1>(a, a.len(order[0]), ss0, early);
        }
    }
}

// defaults.

template <class Early = Nop>
constexpr decltype(auto)
ply(Iterator auto && a, Early && early = Nop {})
{
    if constexpr (ANY==size_s(a)) {
        return ply_ravel(a, early);
    } else {
        return ply_fixed(a, early);
    }
}

constexpr void for_each(auto && op, auto && ... a) { ply(map(op, a ...)); }

template <class T> struct Default { T & def; };
constexpr decltype(auto) early(Iterator auto && a, auto && def) { return ply(a, Default { def }); }


// --------------------
// Input/'output' iterator adapter. FIXME maybe random for rank 1?
// --------------------

template <Iterator A>
struct STLIterator
{
    using difference_type = dim_t;
    using value_type = value_t<A>;

    A a;
    std::decay_t<decltype(ra::shape(a))> ind; // concrete type
    bool over;

    STLIterator(A a_): a(a_), ind(ra::shape(a_)), over(0==ra::size(a)) { validate(a, ic<true>); }
    constexpr STLIterator(STLIterator &&) = default;
    constexpr STLIterator(STLIterator const &) = delete;
    constexpr STLIterator & operator=(STLIterator &&) = default;
    constexpr STLIterator & operator=(STLIterator const &) = delete;
    constexpr bool operator==(std::default_sentinel_t end) const { return over; }
    decltype(auto) operator*() const { return *a; }

    constexpr void
    next()
    {
        for (rank_t k=rank(a)-1; k>=0; --k) {
            if (--ind[k]>0) {
                a.adv(k, 1);
                return;
            } else {
                ind[k] = a.len(k);
                a.adv(k, 1-a.len(k));
            }
        }
        over = true;
    }
    template <int k=rank_s<A>()-1>
    constexpr void
    nexts(ic_t<k> = {})
    {
        if constexpr (k>=0) {
            if (--ind[k]>0) {
                a.adv(k, 1);
                return;
            } else {
                ind[k] = a.len(k);
                a.adv(k, 1-a.len(k));
                return nexts(ic<k-1>);
            }
        }
        over = true;
    }
    constexpr STLIterator & operator++() { if constexpr (ANY==rank_s(a)) next(); else nexts(); return *this; }
    constexpr void operator++(int) { ++(*this); }
};

template <class A> STLIterator(A &&) -> STLIterator<A>;

constexpr auto begin(is_ra auto && a) { return STLIterator(ra::iter(RA_FW(a))); }
constexpr auto end(is_ra auto && a) { return std::default_sentinel; }
constexpr auto range(is_ra auto && a) { return std::ranges::subrange(ra::begin(RA_FW(a)), std::default_sentinel); }

// unqualified might find .begin() anyway through std::begin etc (!)
constexpr auto begin(is_ra auto && a) requires (requires { a.begin(); }) { static_assert(std::is_lvalue_reference_v<decltype(a)>); return a.begin(); }
constexpr auto end(is_ra auto && a) requires (requires { a.end(); }) { static_assert(std::is_lvalue_reference_v<decltype(a)>); return a.end(); }
constexpr auto range(is_ra auto && a) requires (requires { a.begin(); }) { static_assert(std::is_lvalue_reference_v<decltype(a)>); return std::ranges::subrange(a.begin(), a.end()); }


// ---------------------
// Replace Len in expr tree. VAL arguments that must be either is_ctype or is_scalar.
// ---------------------

template <class Ln, class E>
constexpr decltype(auto)
wlen(Ln ln, E && e)
{
    static_assert(std::is_integral_v<Ln> || is_ctype<Ln>);
    if constexpr (has_len<E>) {
        return WLen<std::decay_t<E>>::f(ln, RA_FW(e));
    } else {
        return RA_FW(e);
    }
}

template <>
struct WLen<Len>
{
    constexpr static auto
    f(auto ln, auto && e) { return Scalar {ln}; }
};

template <class Op, Iterator ... P, int ... I> requires (has_len<P> || ...)
struct WLen<Map<Op, std::tuple<P ...>, ilist_t<I ...>>>
{
    constexpr static auto
    f(auto ln, auto && e) { return map_(RA_FW(e).op, wlen(ln, std::get<I>(RA_FW(e).t)) ...); }
};

template <Iterator ... P, int ... I> requires (has_len<P> || ...)
struct WLen<Pick<std::tuple<P ...>, ilist_t<I ...>>>
{
    constexpr static auto
    f(auto ln, auto && e) { return pick(wlen(ln, std::get<I>(RA_FW(e).t)) ...); }
};

template <class I> requires (has_len<I>)
struct WLen<Seq<I>>
{
    constexpr static auto
    f(auto ln, auto && e) { return Seq { VAL(wlen(ln, e.i)) }; }
};

template <class P, class N, class S> requires (has_len<P> || has_len<N> || has_len<S>)
struct WLen<Ptr<P, N, S>>
{
    constexpr static auto
    f(auto ln, auto && e) { return ptr(wlen(ln, e.data()), VAL(wlen(ln, e.dimv[0].len)), VAL(wlen(ln, e.dimv[0].step))); }
};

constexpr auto
shape(auto const & v, auto && e)
{
    if constexpr (is_scalar<decltype(e)>) {
        dim_t k = wlen(ra::rank(v), RA_FW(e));
        RA_CK(inside(k, ra::rank(v)), "Bad axis ", k, " for rank ", ra::rank(v), ".");
        return v.len(k);
    } else {
        return map([&v](auto && e){ return shape(v, e); }, wlen(ra::rank(v), RA_FW(e)));
    }
}


// --------------------
// Slicing and outer product.
// --------------------

template <int n> struct dots_t { constexpr static int N=n; static_assert(n>=0 || UNB==n); };
template <int n=UNB> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t { constexpr static int N=n; static_assert(n>=0); };
template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

template <int w=0, class I=dim_t, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
iota(N && n=N {}, I && i=dim_t(0), S && s=S(maybe_step<S>))
{
    // return ViewSmall<Seq<sarg<I>>, ic_t<std::array {Dim(n, s)}>>(Seq {RA_FW(i)})(insert<w>); // FIXME optimize view<seq>
    return reframe(ptr(Seq<sarg<I>>(RA_FW(i)), RA_FW(n), RA_FW(s)), ilist_t<w> {});
}

template <int rank, class T=dim_t> constexpr auto
ii(dim_t (&&len)[rank], T o, dim_t (&&step)[rank])
{
    return ViewBig<Seq<dim_t>, rank>(map([](dim_t len, dim_t step){ return Dim {len, step}; }, len, step), Seq<dim_t>{o});
}

template <int rank, class T=dim_t> constexpr auto
ii(dim_t (&&len)[rank], T o=0)
{
    return ViewBig<Seq<T>, rank>(len, Seq<T>{o});
}

template <std::integral auto ... i, class T=dim_t> constexpr auto
ii(ra::ilist_t<i ...>, T o=0)
{
    return ViewSmall<Seq<T>, ra::ic_t<ra::default_dims(std::array<ra::dim_t, sizeof...(i)>{i...})>>(Seq<T>{o});
}

template <class A> concept is_ptr = requires (A a) { []<class I, class N, class S>(Ptr<Seq<I>, N, S> const &){}(a); };
template <class A> concept is_iota_static = requires (A a) { []<class I, class Dimv>(ViewSmall<Seq<I>, Dimv> const &){}(a); };
template <class A> concept is_iota_dynamic = requires (A a) { []<class I, rank_t RANK>(ViewBig<Seq<I>, RANK> const &){}(a); };
template <class A> concept is_iota = (is_ptr<A> || is_iota_dynamic<A> || is_iota_static<A>);
template <class A> concept is_scalar_index = is_ra_0<A>;

// beaten, whole or piecewise. Presize bv/ds not to need push_back

template <class I> consteval bool
beatable(I && i)
{
    return ((requires { []<int N>(dots_t<N>){}(i); }) || (requires { []<int N>(insert_t<N>){}(i); }) || is_scalar_index<I>
// exclude to let B=A(... i ...) use B's len. FIXME
            || (is_iota<I> && requires { requires UNB!=ra::size_s<I>(); }));
}

consteval auto fsrc(auto const &) { return 1; }
constexpr auto fdst(auto const & i) { if constexpr (beatable(i)) { if consteval { return rank_s(i); } else { return rank(i); } } else { return 1; } }
template <int n> consteval auto fsrc(dots_t<n> const &) { return n; }
template <int n> consteval auto fdst(dots_t<n> const &) { return n; }
template <int n> consteval auto fsrc(insert_t<n> const &) { return 0; }
template <int n> consteval auto fdst(insert_t<n> const &) { return n; }

consteval int
frombrank_s(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    constexpr int stretch = rank_s(a) - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i)));
    return ANY==rank_s(a) || ((ANY==fdst(i)) || ...) ? ANY : stretch + (0 + ... + (UNB==fdst(i) ? 0 : fdst(i)));
}

constexpr int
frombrank(auto && a, auto && ...  i)
{
    static_assert((0 + ... + (UNB==fdst(i)))<=1);
    int stretch = rank(a) - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i)));
    return stretch + (0 + ... + (UNB==fdst(i) ? 0 : fdst(i)));
}

template <bool dopl=true, bool dobv=true, bool dods=true>
constexpr auto
fromb(auto pl, auto ds, auto && bv, int bk, auto && a, int ak)
{
    if constexpr (dobv) {
#pragma GCC diagnostic push // gcc14/15 -DRA_CHECK=0 --no-sanitize -O3 [ra02]
#pragma GCC diagnostic warning "-Waggressive-loop-optimizations"
        for (; ak<ra::rank(a); ++ak, ++bk) {
#pragma GCC diagnostic pop
#pragma GCC diagnostic push // gcc14/15 -DRA_CHECK=0 --no-sanitize -O2 -O3 [ra03]
#pragma GCC diagnostic warning "-Warray-bounds"
#pragma GCC diagnostic warning "-Wstringop-overflow"
            bv[bk] = { a.dimv[ak].len, a.dimv[ak].step };
#pragma GCC diagnostic pop
        }
    }
    return pl;
}

template <bool dopl=true, bool dobv=true, bool dods=true, class I0>
constexpr auto
fromb(auto pl, auto ds, auto && bv, int bk, auto && a, int ak, I0 const & i0, auto const & ...  i)
{
    if constexpr (is_scalar_index<I0>) {
        if constexpr (dopl) {
// FIXME need constant ak for la to maybe be constant. p2564? See also below
            auto const la = a.len(ak);
            dim_t const i = wlen(la, i0);
            RA_CK(inside(i, la) || (UNB==la && 0==a.step(ak)), "Bad index ", i, " in len[", ak, "]=", la, ".");
            pl += i*a.step(ak);
        }
        ++ak;
    } else if constexpr (is_iota<I0> && beatable(i0)) {
        if constexpr (dobv || dopl) {
            auto const la = a.len(ak);
            for (int q=0; q<rank(i0); ++q) {
                if constexpr (dobv) { bv[bk] = Dim {.len=wlen(la, i0).len(q), .step=wlen(la, i0).step(q)*a.step(ak)}; ++bk; }
                if constexpr (dopl) {
// FIXME no wlen(view iota) yet so process .data().i separately
                    auto const i = wlen(la, i0.data().i);
                    RA_CK(([&, iz=wlen(la, i0)]{ return 0==iz.len(q) || (inside(i, la) && inside(i+(iz.len(q)-1)*iz.step(q), la)); }()),
                          "Bad iota[", q, "] len ", wlen(la, i0).len(q), " step ", wlen(la, i0).step(q), " in len[", ak, "]=", la, ".");
                    pl += i*a.step(ak);
                }
            }
        } else if constexpr (dods) {
            bk += rank(i0);
        }
        ++ak;
    } else if constexpr (requires { []<int N>(dots_t<N>){}(i0); }) {
        int const n = UNB!=i0.N ? i0.N : (ra::rank(a) - ak - (0 + ... + (UNB==fsrc(i) ? 0 : fsrc(i))));
        for (int q=0; q<n; ++q) {
            if constexpr (dobv) { bv[bk] = a.dimv[ak]; }
            if constexpr (dobv || dods) { ++bk; }
            ++ak;
        }
    } else if constexpr (requires { []<int N>(insert_t<N>){}(i0); }) {
        for (int q=0; q<i0.N; ++q) {
            if constexpr (dobv) { bv[bk] = Dim {.len=UNB, .step=0}; }
            if constexpr (dobv || dods) { ++bk; }
        }
    } else {
        if constexpr (dods) { *ds = bk; ++ds; }
        if constexpr (dobv) { bv[bk] = a.dimv[ak]; }
        if constexpr (dobv || dods) { ++bk; }
        ++ak;
    }
    return fromb<dopl, dobv, dods>(pl, ds, bv, bk, a, ak, i ...);
}

constexpr auto
frombv(auto const & a, auto const & ...  i)
{
    std::array<Dim, frombrank_s(a, i ...)> bv {};
    fromb<0, 1, 0>(0, 0, bv, 0, a, 0, i ...);
    return bv;
}

consteval auto
fromds(auto const & a, auto const & ...  i)
{
    std::array<int, (0 + ... + int(!beatable(i)))> ds {};
    fromb<0, 0, 1>(0, ds.data(), 0, 0, a, 0, i ...);
    return ds;
}

constexpr auto
frompl(auto pl, auto && a, auto const & ...  i)
{
    return fromb<1, 0, 0>(pl, 0, 0, 0, a, 0, i ...);
}

// unbeaten

constexpr auto
spacer(auto && b, auto && p, auto && q)
{
    return std::apply([&b](auto ... i){ return std::make_tuple(ra::iota(clen(b, i)) ...); }, mp::iota<q-p, p>{});
}

template <class I, int drop>
constexpr decltype(auto)
fromu_loop(auto && op)
{
    if constexpr (drop==mp::len<I>) {
        return RA_FW(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, ic_t<0>>, mp::drop<I, drop>> {},
                     fromu_loop<I, drop+1>(RA_FW(op)));
    }
}

template <class B>
constexpr decltype(auto)
fromu(B && b, auto && ds, auto && c, auto && ti)
{
    return std::apply([&b](auto && ... i){
        return map(fromu_loop<mp::tuple<ic_t<rank_s(i)> ...>, 1>(std::forward<B>(b)), RA_FW(i) ...);
    }, std::tuple_cat(RA_FW(ti), spacer(b, ic<(0==c ? 0 : 1+ds()[c-1])>, ic<ra::size(b.dimv)>)));
}

constexpr decltype(auto)
fromu(auto && b, auto && ds, auto && c, auto && ti, auto && i0, auto && ... i)
{
    if constexpr (beatable(i0)) {
        return fromu(RA_FW(b), ds, c, RA_FW(ti), RA_FW(i) ...);
    } else {
        return fromu(RA_FW(b), ds, ic<c+1>,
                     std::tuple_cat(RA_FW(ti), spacer(b, ic<(0==c ? 0 : 1+ds()[c-1])>, ic<ds()[c]>),
                                    std::forward_as_tuple(wlen(b.len(ds()[c]), RA_FW(i0)))),
                     RA_FW(i) ...);
    }
}

// driver. Only forward to unbeaten part. Not all var rank cases are handled.

template <class A>
constexpr decltype(auto)
from(A && a, auto && ...  i)
{
    if constexpr (Slice<decltype(a)>) {
        constexpr int dsn = (0 + ... + int(!beatable(i)));
        if constexpr (constexpr int bn=frombrank_s(a, i ...); 0==bn) {
            return *frompl(a.data(), a, i ...);
        } else if constexpr (ANY!=bn) {
            auto beaten = [&]{
// FIXME more cases could be ViewSmall
                if constexpr (ANY!=ra::size_s(a) && ((!beatable(i) /* || has_len<decltype(i)> */ || ANY!=ra::size_s(i)) && ...)) {
                    return ViewSmall<decltype(a.data()), ic_t<frombv(a, i ...)>>(frompl(a.data(), a, i ...));
                } else {
                    ViewBig<decltype(a.data()), bn> b;
                    b.cp = fromb<1, 1, 0>(a.data(), 0, b.dimv, 0, a, 0, i ...);
                    return b;
                }
            };
            if constexpr (0==dsn) {
                return beaten();
            } else {
                return fromu(beaten(), ic<fromds(a, i ...)>, ic<0>, std::tuple<> {}, RA_FW(i) ...);
            }
        } else if constexpr (int bn=frombrank(a, i ...); 0==dsn) {
            ViewBig<decltype(a.data()), ANY> b;
            b.cp = 0==bn ? frompl(a.data(), a, i ...) : (b.dimv.resize(bn), fromb<1, 1, 0>(a.data(), 0, b.dimv, 0, a, 0, i ...));
            return b;
// unbeaten on original rank
        } else if constexpr (sizeof...(i)==dsn && sizeof...(i)==(0 + ... + (ANY!=rank_s(i)))) {
            RA_CK(rank(a)==sizeof...(i), "Run time reframe rank(a) ", rank(a), " args ", sizeof...(i), ".");
            return map(fromu_loop<mp::tuple<ic_t<rank_s(i)> ...>, 1>(
                         [a=std::tuple<A>(RA_FW(a))](auto && ... i) -> decltype(auto)
                         { return *frompl(std::get<0>(a).cp, std::get<0>(a), i ...); }),
                       RA_FW(i) ...);
// unbeaten on beaten rank
        } else {
            RA_CK(dsn==bn, "Run time reframe dsn ", dsn, " bn ", bn, ".");
            ViewBig<decltype(a.data()), dsn> b;
            b.cp = fromb<1, 1, 0>(a.data(), 0, b.dimv, 0, a, 0, i ...);
            return fromu(std::move(b), ic<std::apply([](auto ... i) { return std::array { int(i) ... }; }, mp::iota<dsn> {})>,
                         ic<0>, std::tuple<> {}, RA_FW(i) ...);
        }
    } else if constexpr (0==sizeof...(i)) { // map(op) isn't defined
        return RA_FW(a)();
    } else {
        return map(fromu_loop<mp::tuple<ic_t<rank_s(i)> ...>, 1>(RA_FW(a)), RA_FW(i) ...);
    }
}


// ---------------------------
// I/O
// ---------------------------

// fmt/ostream.h or https://stackoverflow.com/a/75738462
struct ostream_formatter: std::formatter<std::basic_string_view<char>>
{
    constexpr auto
    format(auto const & value, auto & ctx) const
    {
        std::basic_stringstream<char> ss;
        ss << value;
        return std::formatter<std::basic_string_view<char>, char>::format(ss.view(), ctx);
    }
};

template <class A>
constexpr std::ostream &
operator<<(std::ostream & o, Fmt<A> const & fa)
{
    std::print(o, "{}", fa);
    return o;
}

template <class C> requires (ANY!=size_s<C>() && (is_ra<C> || is_fov<C>))
inline std::istream &
operator>>(std::istream & i, C & c)
{
    for (auto & ci: c) { i >> ci; }
    return i;
}

template <class T, class A>
inline std::istream &
operator>>(std::istream & i, std::vector<T, A> & c)
{
    if (dim_t n; i >> n) {
        RA_CK(n>=0, "Negative length in input [", n, "].");
        std::vector<T, A> cc(n);
        swap(c, cc);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

template <class C> requires (ANY==size_s<C>() && !std::is_convertible_v<C, std::string_view>)
inline std::istream &
operator>>(std::istream & i, C & c)
{
    if (decltype(shape(c)) s; i >> s) {
        RA_CK(every(iter(s)>=0), "Negative length in input [", ra::fmt(nstyle, s), "].");
        C cc(s, ra::none);
        swap(c, cc);
        for (auto & ci: c) { i >> ci; }
    }
    return i;
}

} // namespace ra

template <ra::is_array_formattable A>
struct std::formatter<A>
{
    ra::format_t fmt;
    std::conditional_t<std::formattable<ra::ncvalue_t<A>, char>,
                       std::formatter<ra::ncvalue_t<A>>, ra::ostream_formatter> under;

    constexpr auto
    parse(auto & ctx)
    {
        auto i = ctx.begin();
        for (; i!=ctx.end() && ':'!=*i && '}'!=*i; ++i) {
            switch (*i) {
            case 'j': fmt = ra::jstyle; break;
            case 'c': fmt = ra::cstyle; break;
            case 'l': fmt = ra::lstyle; break;
            case 'p': fmt = ra::pstyle; break;
            case 'a': fmt.align = true; break;
            case 'e': fmt.align = false; break;
            case 's': fmt.shape = ra::withshape; break;
            case 'n': fmt.shape = ra::noshape; break;
            case 'd': fmt.shape = ra::defaultshape; break;
            default: throw std::format_error("Bad format for ra:: object.");
            }
        }
        if (i!=ctx.end() && ':'==*i) {
            ctx.advance_to(i+1);
            i = under.parse(ctx);
        }
        if (i!=ctx.end() && '}'!=*i) {
            throw std::format_error("Bad input while parsing format for ra:: object.");
        }
        return i;
    }
    constexpr auto
    format(A const & a_, auto & ctx, ra::format_t const & fmt) const
    {
        auto a = ra::iter(a_);
        validate(a);
        auto sha = ra::shape(a);
        for (int k=0; k<ra::size(sha); ++k) { assert(sha[k]>=0); } // no ops yet
        auto out = ctx.out();
// always print shape with defaultshape to avoid recursion on shape(shape(...)) = [1].
        if (fmt.shape==ra::withshape || (fmt.shape==ra::defaultshape && size_s(a)==ra::ANY)) {
            out = std::format_to(out, "{:d}\n", ra::iter(sha));
        }
        ra::rank_t const rank = ra::rank(a);
        auto goin = [&](int k, auto & goin) -> void
        {
            using std::ranges::copy;
            if (k==rank) {
                ctx.advance_to(under.format(*a, ctx));
                out = ctx.out();
            } else {
                out = copy(fmt.open, out).out;
                for (int i=0; i<sha[k]; ++i) {
                    goin(k+1, goin);
                    if (i+1<sha[k]) {
                        a.adv(k, 1);
                        out = copy((k==rank-1 ? fmt.sep0 : fmt.sepn), out).out;
                        for (int i=0; i<std::max(0, rank-2-k); ++i) {
                            out = copy(fmt.rep, out).out;
                        }
                        if (fmt.align && k<rank-1) {
                            for (int i=0; i<(k+1)*ra::size(fmt.open); ++i) {
                                *out++ = ' ';
                            }
                        }
                    } else {
                        a.adv(k, 1-sha[k]);
                        break;
                    }
                }
                out = copy(fmt.close, out).out;
            }
        };
        goin(0, goin);
        return out;
    }
    constexpr auto format(A const & a_, auto & ctx) const { return format(a_, ctx, fmt); }
};

template <class A>
struct std::formatter<ra::Fmt<A>>: std::formatter<std::basic_string_view<char>>
{
    constexpr auto
    format(ra::Fmt<A> const & f, auto & ctx) const
    {
        return std::formatter<decltype(ra::iter(f.a))>().format(ra::iter(f.a), ctx, f.f);
    }
};
