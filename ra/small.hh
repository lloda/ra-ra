// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Views and containers with static lengths and steps, cf big.hh.

// (c) Daniel Llorens - 2013-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"

namespace ra {


// ---------------------
// replace Len in expr tree. VAL arguments that must be either is_constant or is_scalar.
// ---------------------

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

template <class I, class N, class S> requires (has_len<I> || has_len<N> || has_len<S>)
struct WLen<Ptr<I, N, S>>
{
    constexpr static auto
    f(auto ln, auto && e) { return Ptr(wlen(ln, e.cp), VAL(wlen(ln, e.n)), VAL(wlen(ln, e.s))); }
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
// outer product / slicing
// --------------------

template <int n> struct dots_t { constexpr static int N=n; static_assert(n>=0 || UNB==n); };
template <int n=UNB> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t { constexpr static int N=n; static_assert(n>=0); };
template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

template <int rank> constexpr auto
ii(dim_t (&&len)[rank], dim_t o, dim_t (&&step)[rank])
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

template <class A>
concept is_iota = requires (A a)
{
    []<class I, class N, class S>(Ptr<Seq<I>, N, S> const &){}(a);
    requires UNB!=a.nn; // exclude UNB from beating to let B=A(... i ...) use B's len. FIXME
};

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

template <class I> concept is_scalar_index = is_ra_0<I>;

// beaten, whole or piecewise. bv/ds are presized not to need push_back

template <class I> consteval bool
beatable(I && i)
{
    return ((requires { []<int N>(dots_t<N>){}(i); }) || (requires { []<int N>(insert_t<N>){}(i); })
            || is_scalar_index<I> || is_iota_any<I>);
}

template <class I> consteval auto fsrc(I const &) { return 1; }
template <class I> constexpr auto fdst(I const & i) { if constexpr (beatable(i)) { if consteval { return rank_s(i); } else { return rank(i); } } else { return 1; } }
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
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Waggressive-loop-optimizations" // gcc14.3/15.2 -DRA_CHECK=0 -O3
        for (; ak<ra::rank(a); ++ak, ++bk) {
#pragma GCC diagnostic pop
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Warray-bounds" // gcc14.3/15.2 --no-sanitize -O2 -O3
#pragma GCC diagnostic warning "-Wstringop-overflow" // gcc14.3/15.2 --no-sanitize -O2 -O3
            bv[bk] = a.dimv[ak];
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
    } else if constexpr (is_iota_any<I0>) {
        if constexpr (dobv || dopl) {
            auto const la = a.len(ak);
            for (int q=0; q<rank(i0); ++q) {
                if constexpr (dobv) { bv[bk] = Dim { .len=wlen(la, i0).len(q), .step=wlen(la, i0).step(q)*a.step(ak) }; ++bk; }
                if constexpr (dopl) {
// FIXME no wlen(view iota) yet so process .cp.i separately
                    auto const i = wlen(la, i0.cp.i);
                    RA_CK(([&, iz=wlen(la, i0)]{ return 0==iz.len(q) || (inside(i, la) && inside(i+(iz.len(q)-1)*iz.step(q), la)); }()),
                          "Bad iota[", q, "] in len[", ak, "]=", la, ".");
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
    return std::apply([&b](auto ... i){ return std::make_tuple(ra::iota(maybe_len(b, i)) ...); }, mp::iota<q-p, p>{});
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
        static_assert((0 + ... + (UNB==fdst(i)))<=1);
        constexpr int dsn = (0 + ... + int(!beatable(i)));
        if constexpr (constexpr int bn=frombrank_s(a, i ...); 0==bn) {
            return *frompl(a.cp, a, i ...);
        } else if constexpr (ANY!=bn) {
            auto beaten = [&]{
// FIXME more cases could be ViewSmall
                if constexpr (ANY!=ra::size_s(a) && ((!beatable(i) /* || has_len<decltype(i)> */ || ANY!=ra::size_s(i)) && ...)) {
                    return ViewSmall<decltype(a.data()), ic_t<frombv(a, i ...)>>(frompl(a.cp, a, i ...));
                } else {
                    ViewBig<decltype(a.data()), bn> b;
                    b.cp = fromb<1, 1, 0>(a.cp, 0, b.dimv, 0, a, 0, i ...);
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
            b.cp = 0==bn ? frompl(a.cp, a, i ...) : (b.dimv.resize(bn), fromb<1, 1, 0>(a.cp, 0, b.dimv, 0, a, 0, i ...));
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
            b.cp = fromb<1, 1, 0>(a.cp, 0, b.dimv, 0, a, 0, i ...);
            return fromu(std::move(b), ic<std::apply([](auto ... i) { return std::array { int(i) ... }; }, mp::iota<dsn> {})>,
                         ic<0>, std::tuple<> {}, RA_FW(i) ...);
        }
    } else if constexpr (0==sizeof...(i)) { // map(op) isn't defined
        return RA_FW(a)();
    } else {
        return map(fromu_loop<mp::tuple<ic_t<rank_s(i)> ...>, 1>(RA_FW(a)), RA_FW(i) ...);
    }
}

constexpr decltype(auto)
at_view(auto const & a, auto const & i)
{
// can't say 'frame rank 0' so -size wouldn't work. FIXME What about ra::len
    if constexpr (constexpr rank_t cr = rank_diff(rank_s(a), ra::size_s(i)); ANY==cr) {
        return a.template iter(rank(a)-ra::size(i)).at(i);
    } else {
        return a.template iter<cr>().at(i);
    }
}


// ---------------------
// Small view and container
// ---------------------

constexpr bool
is_c_order_dimv(auto const & dimv, bool step1=true)
{
    bool steps = true;
    dim_t s = 1;
    int k = ra::size(dimv);
    if (!step1) {
        while (--k>=0 && 1==dimv[k].len) {}
        if (k<=0) { return true; }
        s = dimv[k].step*dimv[k].len;
    }
    while (--k>=0) {
        steps = steps && (1==dimv[k].len || dimv[k].step==s);
        s *= dimv[k].len;
    }
    return s==0 || steps;
}

constexpr bool
is_c_order(auto const & v, bool step1=true) { return is_c_order_dimv(v.dimv, step1); }

// cf braces_def for in big.hh.

template <class T, class Dimv> struct nested_arg { using sub = noarg; };
template <class T, class Dimv> struct small_args { using nested = std::tuple<>; };
template <class T, class Dimv> requires (0<ssize(Dimv::value))
struct small_args<T, Dimv> { using nested = mp::makelist<Dimv::value[0].len, typename nested_arg<T, Dimv>::sub>; };

template <class T, class Dimv, class nested_args = small_args<T, Dimv>::nested>
struct SmallArray;

template <class T, class Dimv> requires (requires { T(); } && (0<ssize(Dimv::value)))
struct nested_arg<T, Dimv>
{
    constexpr static auto n = ssize(Dimv::value)-1;
    constexpr static auto s = std::apply([](auto ... i){ return std::array<dim_t, n> { Dimv::value[i].len ... }; }, mp::iota<n, 1> {});
    using sub = std::conditional_t<0==n, T, SmallArray<T, ic_t<default_dims(s)>>>;
};

template <class P> struct reconst_t { using type = void; };
template <class P> struct unconst_t { using type = void; };
template <class T> requires (!std::is_const_v<T>) struct reconst_t<T *> { using type = T const *; };
template <class T> struct unconst_t<T const *> { using type = T *; };
template <class P> using reconst = reconst_t<P>::type;
template <class P> using unconst = unconst_t<P>::type;

template <class P, class Dimv>
struct ViewSmall
{
    constexpr static auto dimv = Dimv::value;
    P cp;

    consteval static rank_t rank() { return dimv.size(); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    constexpr P data() const { return cp; }
    consteval static dim_t size() { return std::apply([](auto ... i){ return (i.len * ... * 1); }, dimv); }
    consteval bool empty() const { return any(0==map(&Dim::len, dimv)); }

    constexpr explicit ViewSmall(P cp_): cp(cp_) {}
    constexpr ViewSmall(ViewSmall const & s) = default;
    constexpr ViewSmall const & view() const { return *this; }
// exclude T and sub constructors by making T & sub noarg
    constexpr static bool have_braces = std::is_reference_v<decltype(*cp)>;
    using T = std::conditional_t<have_braces, std::remove_reference_t<decltype(*cp)>, noarg>;
    using sub = typename nested_arg<T, Dimv>::sub;
// row-major ravel braces
    constexpr ViewSmall const &
    operator=(T (&&x)[have_braces ? size() : 0]) const
        requires (have_braces && rank()>1 && size()>1)
    {
        std::ranges::copy(std::ranges::subrange(x), begin()); return *this;
    }
// nested braces
    constexpr ViewSmall const &
    operator=(sub (&&x)[have_braces ? (rank()>0 ? len(0) : 0) : 0]) const
        requires (have_braces && 0<rank() && 0<len(0) && (1!=rank() || 1!=len(0)))
    {
        ra::iter<-1>(*this) = x; return *this;
    }
// T not is_scalar [ra44]
    constexpr ViewSmall const & operator=(T const & t) const { start(*this)=ra::scalar(t); return *this; }
// cf RA_ASSIGNOPS_ITER [ra38] [ra34]
    ViewSmall const & operator=(ViewSmall const & x) const { start(*this) = x; return *this; }
#define ASSIGNOPS(OP)                                                   \
    constexpr ViewSmall const & operator OP(Iterator auto && x) const { start(*this) OP RA_FW(x); return *this; } \
    constexpr ViewSmall const & operator OP(auto const & x) const { start(*this) OP x; return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS
    template <int s, int o=0> constexpr auto as() const { return from(*this, ra::iota(ra::ic<s>, o)); }
    template <rank_t c=0> constexpr auto iter() const { return Cell<P, ic_t<dimv>, ic_t<c>>(cp); }
    constexpr auto iter(rank_t c) const { return Cell<P, decltype(dimv) const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { if constexpr (is_c_order_dimv(dimv)) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (is_c_order_dimv(dimv)) { return cp+size(); }
    constexpr static auto end() requires (!is_c_order_dimv(dimv)) { return std::default_sentinel; }
    constexpr decltype(auto) back() const { static_assert(size()>0, "Bad back()."); return cp[size()-1]; }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return at_view(*this, i); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
// conversion to const
    constexpr operator ViewSmall<reconst<P>, Dimv> () const requires (!std::is_void_v<reconst<P>>)
    {
        return ViewSmall<reconst<P>, Dimv>(cp);
    }
};

#if defined (__clang__)
template <class T, int N> using extvector __attribute__((ext_vector_type(N))) = T;
#else
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;
#endif

template <class T, size_t N> constexpr size_t align_req = alignof(T[N]);
template <class Z, class ... T> constexpr static bool equals_any = (std::is_same_v<Z, T> || ...);
template <class T, size_t N>
requires (equals_any<T, char, unsigned char, short, unsigned short, int, unsigned int, long, unsigned long,
          long long, unsigned long long, float, double> && 0<N && 0==(N & (N-1)))
constexpr size_t align_req<T, N> = alignof(extvector<T, N>);

template <class T, class Dimv, class ... nested_args>
struct
#if RA_OPT_SMALLVECTOR==1
alignas(align_req<T, std::apply([](auto ... i){ return (i.len * ... * 1); }, Dimv::value)>)
#endif
SmallArray<T, Dimv, std::tuple<nested_args ...>>
{
    constexpr static auto dimv = Dimv::value;
    consteval static rank_t rank() { return ssize(dimv); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static dim_t size() { return std::apply([](auto ... i){ return (i.len * ... * 1); }, dimv); }

    T cp[size()];
    [[no_unique_address]] struct {} prevent_zero_size; // or reuse std::array
    constexpr auto data(this auto && self) { return self.cp; }
    using View = ViewSmall<T *, Dimv>;
    using ViewConst = ViewSmall<T const *, Dimv>;
    constexpr View view() { return View(data()); }
    constexpr ViewConst view() const { return ViewConst(data()); }
    constexpr operator View () { return View(data()); }
    constexpr operator ViewConst () const { return ViewConst(data()); }

    constexpr SmallArray(ra::none_t) {}
    constexpr SmallArray() {}
// T not is_scalar [ra44]
    constexpr SmallArray(T const & t) { std::ranges::fill(cp, t); }
// row-major ravel braces
    constexpr SmallArray(T const & x0, std::convertible_to<T> auto const & ... x)
    requires ((rank()>1) && (size()>1) && ((1+sizeof...(x))==size()))
    {
        view() = { static_cast<T>(x0), static_cast<T>(x) ... };
    }
// nested braces FIXME p1219??
    constexpr SmallArray(nested_args const & ... x)
    requires ((0<rank() && 0!=len(0) && (1!=rank() || 1!=len(0))))
    {
        view() = { x ... };
    }
    constexpr SmallArray(auto const & x) { view() = x; }
    constexpr SmallArray(Iterator auto && x) { view() = RA_FW(x); }
#define ASSIGNOPS(OP)                                                   \
    constexpr SmallArray & operator OP(auto const & x) { view() OP x; return *this; } \
    constexpr SmallArray & operator OP(Iterator auto && x) { view() OP RA_FW(x); return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS

    template <int s, int o=0> constexpr decltype(auto) as(this auto && self) { return RA_FW(self).view().template as<s, o>(); }
    constexpr decltype(auto) back(this auto && self) { return RA_FW(self).view().back(); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) at(this auto && self, auto const & i) { return RA_FW(self).view().at(i); }
    constexpr auto begin(this auto && self) { return self.view().begin(); }
    constexpr auto end(this auto && self) { return self.view().end(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { return RA_FW(self).view().template iter<c>(); }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
};

template <class T, dim_t ... lens>
using Small = SmallArray<T, ic_t<default_dims(std::array<dim_t, sizeof...(lens)> {lens ...})>>;

template <class A0, class ... A> SmallArray(A0, A ...) -> Small<A0, 1+sizeof...(A)>;

// FIXME ravel constructor
template <class A>
constexpr auto
from_ravel(auto && b)
{
    A a;
    RA_CK(1==ra::rank(b) && ra::size(b)==ra::size(a),
          "Bad ravel argument [", fmt(nstyle, ra::shape(b)), "] expecting [", ra::size(a), "].");
    std::ranges::copy(RA_FW(b), a.begin());
    return a;
}

// Small view ops, see View ops in big.hh.

// FIXME Merge transpose & Reframe (beat reframe(view) into transpose(view)).
constexpr void
transpose_dims(auto const & s, auto const & src, auto & dst)
{
    std::ranges::fill(dst, Dim { UNB, 0 });
    for (int k=0; int sk: s) {
        dst[sk].step += src[k].step;
        dst[sk].len = dst[sk].len>=0 ? std::min(dst[sk].len, src[k].len) : src[k].len;
        ++k;
    }
}

RA_IS_DEF(cv_viewsmall, (std::is_convertible_v<A, ViewSmall<decltype(std::declval<A>().data()), ic_t<A::dimv>>>));

template <class K=ic_t<0>>
constexpr auto
reverse(cv_viewsmall auto && a_, K k = K {})
{
    decltype(auto) a = a_.view();
    using A = std::decay_t<decltype(a)>;
    constexpr auto rdimv = [&]{
        std::remove_const_t<decltype(A::dimv)> rdimv = A::dimv;
        RA_CK(inside(k, ssize(rdimv)), "Bad axis ", K::value, " for rank ", ssize(rdimv), ".");
        rdimv[k].step *= -1;
        return rdimv;
    }();
    return ViewSmall<decltype(a.cp), ic_t<rdimv>>(0==rdimv[k].len ? a.cp : a.cp + rdimv[k].step*(1-rdimv[k].len));
}

template <int ... Iarg>
constexpr auto
transpose(cv_viewsmall auto && a_, ilist_t<Iarg ...>)
{
    decltype(auto) a = a_.view();
    using A = std::decay_t<decltype(a)>;
    constexpr static std::array<dim_t, sizeof...(Iarg)> s = { Iarg ... };
    constexpr static auto src = A::dimv;
    static_assert(ra::size(src)==ra::size(s), "Bad size for transposed axes list.");
    constexpr static rank_t dstrank = (0==ra::size(s)) ? 0 : 1 + std::ranges::max(s);
    constexpr static auto dst = [&]{ std::array<Dim, dstrank> dst; transpose_dims(s, src, dst); return dst; }();
    return ViewSmall<decltype(a.cp), ic_t<dst>>(a.data());
}

template <class sup_t, class T, class A, class B>
constexpr void
explode_dims(A const & av, B & bv)
{
    rank_t rb = ssize(bv);
    constexpr rank_t rs = rank_s<sup_t>();
    dim_t s = 1;
    for (int i=rb+rs; i<ssize(av); ++i) {
        RA_CK(av[i].step==s, "Subtype axes are not compact.");
        s *= av[i].len;
    }
    RA_CK(s*sizeof(T)==sizeof(value_t<sup_t>), "Mismatched types.");
    if constexpr (rs>0) {
        for (int i=rb; i<rb+rs; ++i) {
            RA_CK(sup_t::dimv[i-rb].len==av[i].len && s*sup_t::dimv[i-rb].step==av[i].step, "Mismatched axes.");
        }
    }
    s *= size_s<sup_t>();
    for (int i=0; i<rb; ++i) {
        dim_t step = av[i].step;
        RA_CK(0==s ? 0==step : 0==step % s, "Step [", i, "] = ", step, " doesn't match ", s, ".");
        bv[i] = Dim { av[i].len, 0==s ? 0 : step/s };
    }
}

template <class sup_t>
constexpr auto
explode(cv_viewsmall auto && a)
{
    constexpr static rank_t ru = sizeof(value_t<sup_t>)==sizeof(value_t<decltype(a)>) ? 0 : 1;
    constexpr static auto bdimv = [&a]{
        std::array<Dim, ra::rank_s(a)-rank_s<sup_t>()-ru> bdimv;
        explode_dims<sup_t, value_t<decltype(a)>>(a.dimv, bdimv);
        return bdimv;
    }();
    return ViewSmall<sup_t *, ic_t<bdimv>>(reinterpret_cast<sup_t *>(a.data()));
}

constexpr auto
cat(cv_viewsmall auto && a1_, cv_viewsmall auto && a2_)
{
    decltype(auto) a1 = a1_.view();
    decltype(auto) a2 = a2_.view();
    static_assert(1==a1.rank() && 1==a2.rank(), "Bad ranks for cat.");
    Small<std::common_type_t<decltype(a1[0]), decltype(a2[0])>, ra::size(a1)+ra::size(a2)> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    std::copy(a2.begin(), a2.end(), val.begin()+ra::size(a1));
    return val;
}

constexpr auto
cat(cv_viewsmall auto && a1_, is_scalar auto && a2_)
{
    return cat(a1_, ViewSmall<decltype(&a2_), ic_t<std::array {Dim(1, 0)}>>(&a2_));
}

constexpr auto
cat(is_scalar auto && a1_, cv_viewsmall auto && a2_)
{
    return cat(ViewSmall<decltype(&a1_), ic_t<std::array {Dim(1, 0)}>>(&a1_), a2_);
}

} // namespace ra
