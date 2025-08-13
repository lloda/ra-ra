// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with static lengths/strides, cf big.hh.

// (c) Daniel Llorens - 2013-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"

namespace ra {

struct Dim { dim_t len, step; };

inline std::ostream &
operator<<(std::ostream & o, Dim const & dim) { return (o << "[Dim " << dim.len << " " << dim.step << "]"); }

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

constexpr dim_t
filldimv(auto const & shape, auto & dimv)
{
    map(&Dim::len, dimv) = shape;
    dim_t s = 1;
    for (int k=ra::size(dimv); --k>=0;) {
        dimv[k].step = s;
        RA_CK(dimv[k].len>=0, "Bad len[", k, "] ", dimv[k].len, ".");
// gcc 14.2, no warning with sanitizers
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"
        s *= dimv[k].len;
#pragma GCC diagnostic pop
    }
    return s;
}

consteval auto
default_dims(auto const & lv) { std::array<Dim, ra::size(lv)> dv; filldimv(lv, dv); return dv; };

constexpr auto
shape(auto const & v, auto && e)
{
    if constexpr (is_scalar<decltype(e)>) {
        dim_t k = wlen(ra::rank(v), RA_FW(e));
        RA_CK(inside(k, ra::rank(v)), "Bad axis ", k, " for rank ", ra::rank(v), ".");
        return v.len(k);
    } else {
        return map([&v](auto && e) { return shape(v, e); }, wlen(ra::rank(v), RA_FW(e)));
    }
}

constexpr void
resize(auto & a, dim_t s)
{
    if constexpr (ANY==size_s(a)) {
        RA_CK(s>=0, "Bad resize ", s, ".");
        a.resize(s);
    } else {
        RA_CK(s==start(a).len(0) || UNB==s, "Bad resize ", s, ", need ", start(a).len(0), ".");
    }
}


// --------------------
// slicing/indexing helpers
// --------------------

template <int n> struct dots_t { constexpr static int N=n; static_assert(n>=0 || UNB==n); };
template <int n=UNB> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t { constexpr static int N=n; static_assert(n>=0); };
template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

template <class I> concept is_scalar_index = is_ra_0<I>;

struct beatable_t
{
    bool rt, ct; // beatable at all and statically
    int src, dst; // axes on src and dst
};

template <class I> constexpr beatable_t beatable_def
    = { .rt=is_scalar_index<I>, .ct=is_scalar_index<I>, .src=1, .dst=0 };

template <int n> constexpr beatable_t beatable_def<dots_t<n>>
    = { .rt=true, .ct=true, .src=n, .dst=n };

template <int n> constexpr beatable_t beatable_def<insert_t<n>>
    = { .rt=true, .ct=true, .src=0, .dst=n };

template <class I> requires (is_iota<I>) constexpr beatable_t beatable_def<I>
    = { .rt=(UNB!=I::nn), .ct=std::decay_t<decltype(wlen(ic<1>, std::declval<I>()))>::constant,
        .src=1, .dst=1 };

template <class I> constexpr beatable_t beatable = beatable_def<std::decay_t<I>>;

template <class II, int drop>
constexpr decltype(auto)
from_partial(auto && op)
{
    if constexpr (drop==mp::len<II>) {
        return RA_FW(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, ic_t<0>>, mp::drop<II, drop>> {},
                     from_partial<II, drop+1>(RA_FW(op)));
    }
}

// TODO should be able to do better by slicing at each dimension, etc. But verb<>'s innermost op must be rank 0.
constexpr decltype(auto)
from(auto && a, auto && ... i)
{
    if constexpr (0==sizeof...(i)) {
        return RA_FW(a)();
    } else if constexpr (1==sizeof...(i)) {
// support dynamic rank for 1 arg only (see test in test/from.cc).
        return map(RA_FW(a), RA_FW(i) ...);
    } else {
        return map(from_partial<mp::tuple<ic_t<rank_s(i)> ...>, 1>(RA_FW(a)), RA_FW(i) ...);
    }
}

template <int N, class KK=mp::iota<N>> struct unbeat;
template <int N, int ... k>
struct unbeat<N, ilist_t<k ...>>
{
    constexpr static decltype(auto)
    op(auto && v, auto && ... i) { return from(RA_FW(v), wlen(maybe_len(v, ic<k>), RA_FW(i)) ...); }
};

template <class Q, class P>
constexpr dim_t
indexer(Q const & q, P const & pp)
{
    decltype(auto) p = start(pp);
    if constexpr (ANY==rank_s(p)) {
        RA_CK(1==rank(p), "Bad rank ", rank(p), " for subscript.");
    } else {
        static_assert(1==rank_s(p), "Bad rank for subscript.");
    }
    if constexpr (ANY==size_s(p) || ANY==rank_s(q)) {
        RA_CK(p.len(0) >= q.rank(), "Too few indices.");
    } else {
        static_assert(size_s(p) >= rank_s(q), "Too few indices.");
    }
    if constexpr (ANY==rank_s(q)) {
        dim_t c = 0;
        for (rank_t k=0; k<q.rank(); ++k, p.mov(p.step(0))) {
            auto pk = *p;
            RA_CK(inside(pk, q.len(k)) || (UNB==q.len(k) && 0==q.step(k)));
            c += q.step(k) * pk;
        }
        return c;
    } else {
        auto loop = [&](this auto && loop, auto k, dim_t c)
        {
            if constexpr (k==rank_s(q)) {
                return c;
            } else {
                auto pk = *p;
                RA_CK(inside(pk, q.len(k)) || (UNB==q.len(k) && 0==q.step(k)));
                return p.mov(p.step(0)), loop(ic<k+1>, c + (q.step(k) * pk));
            }
        };
        return loop(ic<0>, 0);
    }
}


// --------------------
// view iterators. TODO Take iterator like Ptr does and Views should, not raw pointers
// --------------------

template <auto v, int n>
constexpr auto vdrop = []{
    std::array<Dim, ra::size(v)-n> r;
    for (int i=0; i<ra::size(r); ++i) { r[i] = v[n+i]; }
    return r;
}();

template <class T, class Dimv> struct ViewSmall;
template <class T, rank_t RANK=ANY> struct ViewBig;

constexpr rank_t rank_sum(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a+b; }
constexpr rank_t rank_diff(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a-b; }
// cr>=0 is cell rank, -cr>0 is frame rank. TODO frame rank 0? maybe ra::len
constexpr rank_t rank_cell(rank_t r, rank_t cr) { return cr>=0 ? cr : r==ANY ? ANY : (r+cr); }
constexpr rank_t rank_frame(rank_t r, rank_t cr) { return r==ANY ? ANY : cr>=0 ? (r-cr) : -cr; }

template <class P, class Dimv, class Spec>
struct CellSmall
{
    constexpr static rank_t spec = maybe_any<Spec>;
    constexpr static rank_t fullr = size_s(Dimv::value);
    constexpr static rank_t cellr = is_constant<Spec> ? rank_cell(fullr, spec) : ANY;
    constexpr static rank_t framer = is_constant<Spec> ? rank_frame(fullr, spec) : ANY;

    constexpr static auto dimv = Dimv::value;
    ViewSmall<P, ic_t<vdrop<dimv, framer>>> c;
    constexpr explicit CellSmall(P p): c { p } {}

    consteval static rank_t rank() { return framer; }
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Warray-bounds" // gcc14.3/15.1 -O3 (but not -O<3)
    constexpr static dim_t len(int k) { return dimv[k].len; }
#pragma GCC diagnostic pop
    constexpr static dim_t step(int k) { return k<rank() ? dimv[k].step : 0; }
    constexpr static bool keep(dim_t st, int z, int j) { return st*step(z)==step(j); }
};

template <class P, class Dimv, class Spec>
struct CellBig
{
    constexpr static rank_t spec = maybe_any<Spec>;
    constexpr static rank_t fullr = size_s<Dimv>();
    constexpr static rank_t cellr = is_constant<Spec> ? rank_cell(fullr, spec) : ANY;
    constexpr static rank_t framer = is_constant<Spec> ? rank_frame(fullr, spec) : ANY;

    Dimv dimv;
    ViewBig<P, cellr> c;
    [[no_unique_address]] Spec const dspec;
    constexpr CellBig(P cp, Dimv const & dimv_, Spec dspec_=Spec {}): dimv(dimv_), dspec(dspec_)
    {
        c.cp = cp;
        rank_t dcell=rank_cell(ra::size(dimv), dspec), dframe=rank();
        RA_CK(0<=dframe && 0<=dcell, "Bad cell rank ", dcell, " for full rank ", ssize(dimv), ").");
        resize(c.dimv, dcell);
        for (int k=0; k<dcell; ++k) {
            c.dimv[k] = dimv[dframe+k];
        }
    }

    consteval static rank_t rank() requires (ANY!=framer) { return framer; }
    constexpr rank_t rank() const requires (ANY==framer) { return rank_frame(ra::size(dimv), dspec); }
    constexpr dim_t len(int k) const { return dimv[k].len; }
    constexpr dim_t step(int k) const { return k<rank() ? dimv[k].step : 0; }
    constexpr bool keep(dim_t st, int z, int j) const { return st*step(z)==step(j); }
};

template <class P, class Dimv, class Spec>
struct Cell: public std::conditional_t<is_constant<Dimv>, CellSmall<P, Dimv, Spec>, CellBig<P, Dimv, Spec>>
{
    using Base = std::conditional_t<is_constant<Dimv>, CellSmall<P, Dimv, Spec>, CellBig<P, Dimv, Spec>>;
    using Base::Base, Base::cellr, Base::framer, Base::c, Base::step, Base::len;
    using View = decltype(std::declval<Base>().c);
    static_assert((cellr>=0 || cellr==ANY) && (framer>=0 || framer==ANY), "Bad cell/frame ranks.");

    RA_ASSIGNOPS_SELF(Cell)
    RA_ASSIGNOPS_DEFAULT_SET

    constexpr static dim_t len_s(int k) { if constexpr (is_constant<Dimv>) return len(k); else return ANY; }
    constexpr void adv(rank_t k, dim_t d) { mov(step(k)*d); }
    constexpr decltype(auto) at(auto const & i) const requires (0==cellr) { return c.cp[indexer(*this, i)]; }
    constexpr auto at(auto const & i) const requires (0!=cellr) { View cc(c); cc.cp += indexer(*this, i); return cc; }
    constexpr decltype(auto) operator*() const requires (0==cellr) { return *(c.cp); }
    constexpr View const & operator*() const requires (0!=cellr) { return c; }
    constexpr auto save() const { return c.cp; }
    constexpr void load(decltype(c.cp) cp) { c.cp = cp; }
    constexpr void mov(dim_t d) { c.cp += d; }
};


// ---------------------
// Small view and containers
// ---------------------

// helpers for operator()

template <auto prev, class ... I>
struct filterdims
{
    constexpr static auto dimv = prev;
};

template <auto prev, class I0, class ... I> requires (!is_iota<I0>)
struct filterdims<prev, I0, I ...>
{
    constexpr static bool stretch = (beatable<I0>.dst==UNB);
    static_assert(!stretch || ((beatable<I>.dst!=UNB) && ...), "Repeated stretch index.");
    constexpr static int dst = stretch ? (ssize(prev) - (0 + ... + beatable<I>.src)) : beatable<I0>.dst;
    constexpr static int src = stretch ? (ssize(prev) - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
    constexpr static auto next = filterdims<vdrop<prev, src>, I ...>::dimv;
    constexpr static auto dimv = []{
        std::array<Dim, dst+ssize(next)> r;
        for (int i=0; i<dst; ++i) { r[i] = prev[i]; }
        for (int i=0; i<ssize(next); ++i) { r[dst+i] = next[i]; }
        return r;
    }();
};

template <auto prev, class I0, class ... I> requires (is_iota<I0>)
struct filterdims<prev, I0, I ...>
{
    constexpr static int src = beatable<I0>.src;
    constexpr static auto next = filterdims<vdrop<prev, src>, I ...>::dimv;
    constexpr static auto dimv = []{
        std::array<Dim, 1+ssize(next)> r;
        r[0] = Dim { I0::nn, prev[0].step * I0::gets() };
        for (int i=0; i<ssize(next); ++i) { r[1+i] = next[i]; }
        return r;
    }();
};

// nested braces for Small initializers. Cf braces_def for in big.hh.

template <class T, class Dimv> struct nested_arg { using sub = noarg; };
template <class T, class Dimv> struct small_args { using nested = std::tuple<>; };
template <class T, class Dimv> requires (0<ssize(Dimv::value))
struct small_args<T, Dimv> { using nested = mp::makelist<Dimv::value[0].len, typename nested_arg<T, Dimv>::sub>; };

template <class T, class Dimv, class nested_args = small_args<T, Dimv>::nested>
struct SmallArray;

template <class T, class Dimv> requires (requires { T(); } && (0<ssize(Dimv::value)))
struct nested_arg<T, Dimv>
{
    constexpr static auto sn = ssize(Dimv::value)-1;
    constexpr static auto s = std::apply([](auto ... i) { return std::array<dim_t, sn> { Dimv::value[i].len ... }; }, mp::iota<sn, 1> {});
    using sub = std::conditional_t<0==sn, T, SmallArray<T, ic_t<default_dims(s)>>>;
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
    consteval static rank_t rank() { return ssize(dimv); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static dim_t size() { return std::apply([](auto ... i) { return (i.len * ... * 1); }, dimv); }

    P cp;
    constexpr ViewSmall const & view() const { return *this; }
    constexpr P data() const { return cp; }
    constexpr explicit ViewSmall(P cp_): cp(cp_) {}
// exclude T and sub constructors by making T & sub noarg
    constexpr static bool have_braces = std::is_reference_v<decltype(*cp)>;
    using T = std::conditional_t<have_braces, std::remove_reference_t<decltype(*cp)>, noarg>;
    using sub = typename nested_arg<T, Dimv>::sub;
// if T isn't is_scalar [ra44]
    constexpr ViewSmall const &
    operator=(T const & t) const
    {
        start(*this) = ra::scalar(t); return *this;
    }
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
    constexpr ViewSmall(ViewSmall const & s) = default;
// cf RA_ASSIGNOPS_SELF [ra38] [ra34]
    ViewSmall const & operator=(ViewSmall const & x) const { start(*this) = x; return *this; }
#define ASSIGNOPS(OP)                                                   \
    constexpr ViewSmall const & operator OP(Iterator auto && x) const { start(*this) OP RA_FW(x); return *this; } \
    constexpr ViewSmall const & operator OP(auto const & x) const { start(*this) OP x; return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS

    template <int k>
    constexpr static dim_t
    select(dim_t i)
    {
        RA_CK(inside(i, len(k)), "Bad index ", i, " in len[", k, "]=", len(k), ".");
        return step(k)*i;
    }
    template <int k>
    constexpr static dim_t
    select(is_iota auto const & i)
    {
        static_assert((1>=i.n ? 1 : (i.s<0 ? -i.s : i.s)*(i.n-1)+1) <= len(k), "Bad index.");
        RA_CK(inside(i, len(k)), "Bad index iota [", i.n, " ", i.cp.i, " ", i.s, "] in len[", k, "]=", len(k), ".");
        return 0==i.n ? 0 : step(k)*i.cp.i;
    }
    template <int k, int n>
    constexpr static dim_t
    select(dots_t<n> i) { return 0; }

    template <int k, class I0, class ... I>
    constexpr static dim_t
    select_loop(I0 && i0, I && ... i)
    {
        constexpr int nn = (UNB==beatable<I0>.src) ? (rank() - k - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
        return select<k>(wlen(ic<len(k)>, RA_FW(i0))) + select_loop<k + nn>(RA_FW(i) ...);
    }
    template <int k>
    consteval static dim_t
    select_loop() { return 0; }

    template <class ... I>
    constexpr decltype(auto)
    operator()(this auto && self, I && ... i)
    {
        constexpr int stretch = (0 + ... + (beatable<I>.dst==UNB));
        static_assert(stretch<=1, "Cannot repeat stretch index.");
        if constexpr ((0 + ... + is_scalar_index<I>)==rank()) {
            return self.cp[select_loop<0>(i ...)];
// FIXME wlen before this, cf is_constant_iota
        } else if constexpr ((beatable<I>.ct && ...)) {
            return ViewSmall<P, ic_t<filterdims<dimv, std::decay_t<I> ...>::dimv>> (self.cp + select_loop<0>(i ...));
// TODO partial beating
        } else {
// must fwd *this because we create temp views on every Small::view() call
            return unbeat<sizeof...(I)>::op(RA_FW(self), RA_FW(i) ...);
        }
    }
    constexpr decltype(auto)
    operator[](this auto && self, auto && ... i) { return RA_FW(self)(RA_FW(i) ...); }

    constexpr decltype(auto)
    at(auto const & i) const
    {
// can't say 'frame rank 0' so -size wouldn't work. FIXME What about ra::len
        constexpr rank_t crank = rank_diff(rank(), ra::size_s(i));
        static_assert(crank>=0); // to make out the output type
        return iter<crank>().at(i);
    }
    template <int s, int o=0> constexpr auto as() const { return operator()(ra::iota(ra::ic<s>, o)); }
    template <rank_t c=0> constexpr auto iter() const { return Cell<P, ic_t<dimv>, ic_t<c>>(cp); }
    constexpr auto begin() const { if constexpr (is_c_order_dimv(dimv)) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (is_c_order_dimv(dimv)) { return cp+size(); }
    constexpr static auto end() requires (!is_c_order_dimv(dimv)) { return std::default_sentinel; }
    constexpr decltype(auto) back() const { static_assert(size()>0, "Bad back()."); return cp[size()-1]; }
    constexpr operator ViewSmall<reconst<P>, Dimv> () const requires (!std::is_void_v<reconst<P>>)
    {
        return ViewSmall<reconst<P>, Dimv>(cp);
    }
    constexpr operator T & () const { return to_scalar(*this); }
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
alignas(align_req<T, std::apply([](auto ... i) { return (i.len * ... * 1); }, Dimv::value)>)
#endif
SmallArray<T, Dimv, std::tuple<nested_args ...>>
{
    constexpr static auto dimv = Dimv::value;
    consteval static rank_t rank() { return ssize(dimv); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static dim_t size() { return std::apply([](auto ... i) { return (i.len * ... * 1); }, dimv); }

    T cp[size()];
    [[no_unique_address]] struct {} prevent_zero_size; // or reuse std::array
    constexpr auto data(this auto && self) { return self.cp; }
    using View = ViewSmall<T *, Dimv>;
    using ViewConst = ViewSmall<T const *, Dimv>;
    constexpr View view() { return View(data()); }
    constexpr ViewConst view() const { return ViewConst(data()); }
    constexpr operator View () { return View(cp); }
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
    constexpr decltype(auto) operator()(this auto && self, auto && ... a) { return RA_FW(self).view()(RA_FW(a) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... a) { return RA_FW(self).view()(RA_FW(a) ...); }
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


// ---------------------
// builtin arrays.
// ---------------------

constexpr auto
peel(auto && t)
{
    using T = std::remove_cvref_t<decltype(t)>;
    if constexpr (1 < std::rank_v<T>) {
        static_assert(0 < std::extent_v<T, 0>);
        return peel(*std::data(t));
    } else {
        return std::data(t);
    }
}

constexpr auto
start(is_builtin_array auto && t)
{
    using T = std::remove_all_extents_t<std::remove_reference_t<decltype(t)>>; // preserve const
    return ViewSmall<T *, ic_t<default_dims(ra::shape(t))>>(peel(t)).iter();
}

} // namespace ra
