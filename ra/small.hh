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

constexpr rank_t rank_sum(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a+b; }
constexpr rank_t rank_diff(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a-b; }
// cr>=0 is cell rank, -cr>0 is frame rank. TODO How to say frame rank 0. Maybe ra::end?
constexpr rank_t rank_cell(rank_t r, rank_t cr) { return cr>=0 ? cr : r==ANY ? ANY : (r+cr); }
constexpr rank_t rank_frame(rank_t r, rank_t cr) { return r==ANY ? ANY : cr>=0 ? (r-cr) : -cr; }

struct Dim { dim_t len, step; };

inline std::ostream &
operator<<(std::ostream & o, Dim const & dim) { return (o << "[Dim " << dim.len << " " << dim.step << "]"); }

template <auto v, int n>
constexpr auto vdrop = []()
{
    std::array<Dim, ssize(v)-n> r;
    for (int i=0; i<int(r.size()); ++i) { r[i] = v[n+i]; }
    return r;
}();

template <auto f, auto dimv, int n=ssize(dimv)>
using ctuple = decltype(std::apply([](auto ... i) { return ilist<std::invoke(f, dimv[i]) ...>; }, mp::iota<n> {}));

constexpr bool
is_c_order_dimv(auto const & dimv, bool step1=true)
{
    bool steps = true;
    dim_t s = 1;
    int k = dimv.size();
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
filldim(auto && shape, auto & dimv)
{
    map(&Dim::len, dimv) = shape;
    dim_t s = 1;
    for (int k=dimv.size(); --k>=0;) {
        dimv[k].step = s;
        RA_CHECK(dimv[k].len>=0, "Bad len[", k, "] ", dimv[k].len, ".");
// gcc 14.2, no warning with sanitizers
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"
        s *= dimv[k].len;
#pragma GCC diagnostic pop
    }
    return s;
}

template <auto lenv>
constexpr auto default_dims = []()
{
    std::array<Dim, ra::size(lenv)> dimv;
    filldim(lenv, dimv);
    return dimv;
}();

constexpr auto
shape(auto const & v, auto && e)
{
    if constexpr (is_scalar<decltype(e)>) {
        dim_t k = wlen(ra::rank(v), RA_FWD(e));
        RA_CHECK(inside(k, ra::rank(v)), "Bad axis ", k, " for rank ", ra::rank(v), ".");
        return v.len(k);
    } else {
        return map([&v](auto && e) { return shape(v, e); }, wlen(ra::rank(v), RA_FWD(e)));
    }
}

constexpr void
resize(auto & a, dim_t s)
{
    if constexpr (ANY==size_s(a)) {
        RA_CHECK(s>=0, "Bad resize ", s, ".");
        a.resize(s);
    } else {
        RA_CHECK(s==start(a).len(0) || UNB==s, "Bad resize ", s, ", need ", start(a).len(0), ".");
    }
}


// --------------------
// slicing/indexing helpers
// --------------------

template <int n=UNB> struct dots_t { static_assert(n>=0 || UNB==n); };
template <int n=UNB> constexpr dots_t<n> dots = dots_t<n>();
constexpr auto all = dots<1>;

template <int n> struct insert_t { static_assert(n>=0); };
template <int n=1> constexpr insert_t<n> insert = insert_t<n>();

template <class I> constexpr bool is_scalar_index = ra::is_zero_or_scalar<I>;

struct beatable_t
{
    bool rt, ct; // beatable at all and statically
    int src, dst, add; // axes on src, dst, and dst-src
};

template <class I> constexpr beatable_t beatable_def
    = { .rt=is_scalar_index<I>, .ct=is_scalar_index<I>, .src=1, .dst=0, .add=-1 };

template <int n> constexpr beatable_t beatable_def<dots_t<n>>
    = { .rt=true, .ct = true, .src=n, .dst=n, .add=0 };

template <int n> constexpr beatable_t beatable_def<insert_t<n>>
    = { .rt=true, .ct = true, .src=0, .dst=n, .add=n };

template <class I> requires (is_iota<I>) constexpr beatable_t beatable_def<I>
    = { .rt=(UNB!=I::nn), .ct=std::decay_t<decltype(wlen(ic<1>, std::declval<I>()))>::constant,
        .src=1, .dst=1, .add=0 };

template <class I> constexpr beatable_t beatable = beatable_def<std::decay_t<I>>;

template <class II, int drop, class Op>
constexpr decltype(auto)
from_partial(Op && op)
{
    if constexpr (drop==mp::len<II>) {
        return RA_FWD(op);
    } else {
        return wrank(mp::append<mp::makelist<drop, ic_t<0>>, mp::drop<II, drop>> {},
                     from_partial<II, drop+1>(RA_FWD(op)));
    }
}

// TODO should be able to do better by slicing at each dimension, etc. But verb<>'s innermost op must be rank 0.
template <class A, class ... I>
constexpr decltype(auto)
from(A && a, I && ... i)
{
    if constexpr (0==sizeof...(i)) {
        return RA_FWD(a)();
    } else if constexpr (1==sizeof...(i)) {
// support dynamic rank for 1 arg only (see test in test/from.cc).
        return map(RA_FWD(a), RA_FWD(i) ...);
    } else {
        return map(from_partial<mp::tuple<ic_t<rank_s(i)> ...>, 1>(RA_FWD(a)), RA_FWD(i) ...);
    }
}

template <int k=0, class V>
constexpr decltype(auto)
maybe_len(V && v)
{
    if constexpr (ANY!=std::decay_t<V>::len_s(k)) {
        return ic<std::decay_t<V>::len_s(k)>;
    } else {
        return v.len(k);
    }
}

template <int N, class KK=mp::iota<N>> struct unbeat;

template <int N, int ... k>
struct unbeat<N, ilist_t<k ...>>
{
    constexpr static decltype(auto)
    op(auto && v, auto && ... i)
    {
        return from(RA_FWD(v), wlen(maybe_len<k>(v), RA_FWD(i)) ...);
    }
};

template <class Q, class P>
constexpr dim_t
indexer(Q const & q, P const & pp)
{
    decltype(auto) p = start(pp);
    if constexpr (ANY==rank_s(p)) {
        RA_CHECK(1==rank(p), "Bad rank ", rank(p), " for subscript.");
    } else {
        static_assert(1==rank_s(p), "Bad rank for subscript.");
    }
    if constexpr (ANY==size_s(p) || ANY==rank_s(q)) {
        RA_CHECK(p.len(0) >= q.rank(), "Too few indices.");
    } else {
        static_assert(size_s(p) >= rank_s(q), "Too few indices.");
    }
    if constexpr (ANY==rank_s(q)) {
        dim_t c = 0;
        for (rank_t k=0; k<q.rank(); ++k, p.mov(p.step(0))) {
            auto pk = *p;
            RA_CHECK(inside(pk, q.len(k)) || (UNB==q.len(k) && 0==q.step(k)));
            c += q.step(k) * pk;
        }
        return c;
    } else {
        auto loop = [&](this auto && loop, auto && k, dim_t c)
        {
            if constexpr (k==rank_s(q)) {
                return c;
            } else {
                auto pk = *p;
                RA_CHECK(inside(pk, q.len(k)) || (UNB==q.len(k) && 0==q.step(k)));
                return p.mov(p.step(0)), loop(ic<k+1>, c + (q.step(k) * pk));
            }
        };
        return loop(ic<0>, 0);
    }
}


// --------------------
// view iterators. TODO Take iterator like Ptr does and Views should, not raw pointers
// --------------------

template <class T, class Dimv> struct ViewSmall;

template <class T, class Dimv, class Spec>
struct CellSmall
{
    constexpr static auto dimv = Dimv::value;
    constexpr static rank_t spec = maybe_any<Spec>;
    constexpr static rank_t fullr = size_s<decltype(dimv)>();
    constexpr static rank_t cellr = is_constant<Spec> ? rank_cell(fullr, spec) : ANY;
    constexpr static rank_t framer = is_constant<Spec> ? rank_frame(fullr, spec) : ANY;

    ViewSmall<T, ic_t<vdrop<dimv, framer>>> c;

    consteval static rank_t rank() { return framer; }
    constexpr static dim_t len(int k) { return dimv[k].len; } // len(0<=k<rank) or step(0<=k)
    constexpr static dim_t step(int k) { return k<rank() ? dimv[k].step : 0; }
    constexpr static bool keep(dim_t st, int z, int j) { return st*step(z)==step(j); }

    constexpr CellSmall(T * p): c { p } {}
};

template <class T, rank_t RANK=ANY> struct ViewBig;

template <class T, class Dimv, class Spec>
struct CellBig
{
    Dimv dimv;
    constexpr static rank_t spec = maybe_any<Spec>;
    constexpr static rank_t fullr = size_s<decltype(dimv)>();
    constexpr static rank_t cellr = is_constant<Spec> ? rank_cell(fullr, spec) : ANY;
    constexpr static rank_t framer = is_constant<Spec> ? rank_frame(fullr, spec) : ANY;

    ViewBig<T, cellr> c;
    [[no_unique_address]] Spec const dspec = {};

    consteval static rank_t rank() requires (ANY!=framer) { return framer; }
    constexpr rank_t rank() const requires (ANY==framer) { return rank_frame(dimv.size(), dspec); }
    constexpr dim_t len(int k) const { return dimv[k].len; } // len(0<=k<rank) or step(0<=k)
    constexpr dim_t step(int k) const { return k<rank() ? dimv[k].step : 0; }
    constexpr bool keep(dim_t st, int z, int j) const { return st*step(z)==step(j); }

    constexpr CellBig(T * cp, Dimv const & dimv_, Spec dspec_=Spec {}): dimv(dimv_), dspec(dspec_)
    {
        c.cp = cp;
        rank_t dcellr=rank_cell(dimv.size(), dspec), dframer=rank();
        RA_CHECK(0<=dframer && 0<=dcellr, "Bad cell rank ", dcellr, " for full rank ", ssize(dimv), ").");
        resize(c.dimv, dcellr);
        for (int k=0; k<dcellr; ++k) {
            c.dimv[k] = dimv[dframer+k];
        }
    }
};

template <class T, class Dimv, class Spec>
struct Cell: public std::conditional_t<is_constant<Dimv>, CellSmall<T, Dimv, Spec>, CellBig<T, Dimv, Spec>>
{
    using Base = std::conditional_t<is_constant<Dimv>, CellSmall<T, Dimv, Spec>, CellBig<T, Dimv, Spec>>;
    using Base::Base, Base::cellr, Base::framer, Base::c, Base::step, Base::len;
    using View = decltype(std::declval<Base>().c);

    static_assert(cellr>=0 || cellr==ANY || framer>=0 || framer==ANY, "Bad cell/frame rank.");

    RA_ASSIGNOPS_SELF(Cell)
    RA_ASSIGNOPS_DEFAULT_SET

    constexpr static dim_t len_s(int k) { if constexpr (is_constant<Dimv>) return len(k); else return ANY; }
    constexpr decltype(auto) at(auto const & i) const requires (0==cellr) { return c.cp[indexer(*this, i)]; }
    constexpr decltype(auto) at(auto const & i) const requires (0!=cellr) { View cc(c); cc.cp += indexer(*this, i); return cc; }
    constexpr void adv(rank_t k, dim_t d) { c.cp += step(k)*d; }
    constexpr decltype(auto) operator*() const requires (0==cellr) { return *(c.cp); }
    constexpr View const & operator*() const requires (0!=cellr) { return c; }
    constexpr auto save() const { return c.cp; }
    constexpr void load(decltype(c.cp) cp) { c.cp = cp; }
    constexpr void mov(dim_t d) { c.cp += d; }
};


// ---------------------
// nested braces for Small initializers. Cf braces_def for in big.hh.
// ---------------------

template <class T, class lens>
struct nested_arg { using sub = noarg; };

template <class T, class lens>
struct small_args { using nested = std::tuple<noarg>; };

template <class T, class lens> requires (0<mp::len<lens>)
struct small_args<T, lens>
{
    using nested = mp::makelist<mp::ref<lens, 0>::value, typename nested_arg<T, lens>::sub>;
};

template <class T, class Dimv, class nested_args = small_args<T, ctuple<&Dim::len, Dimv::value>>::nested>
struct SmallArray;

template <class T, dim_t ... lens>
using Small = SmallArray<T, ic_t<default_dims<std::array<dim_t, sizeof...(lens)> {lens ...}>>>;

template <class T, int S0, int ... S>
struct nested_arg<T, ilist_t<S0, S ...>>
{
    using sub = std::conditional_t<0==sizeof...(S), T, Small<T, S ...>>;
};


// ---------------------
// Small view & container
// ---------------------

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
    constexpr static auto dimv = []()
    {
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
    constexpr static auto dimv = []()
    {
        std::array<Dim, 1+ssize(next)> r;
        r[0] = Dim { I0::nn, prev[0].step * I0::gets() };
        for (int i=0; i<ssize(next); ++i) { r[1+i] = next[i]; }
        return r;
    }();
};

template <class T_, class Dimv>
struct SmallBase
{
    constexpr static auto dimv = Dimv::value;
    using T = T_;

    consteval static rank_t rank() { return ssize(dimv); }
    consteval static dim_t size() { dim_t s=1; for (auto dim: dimv) { s*=dim.len; } return s; }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    static_assert(every(0<=map(&Dim::len, dimv)), "Bad shape."); // TODO check steps
    constexpr static bool defsteps = is_c_order_dimv(dimv);
};

template <class T, class Dimv>
struct ViewSmall: public SmallBase<T, Dimv>
{
    using Base = SmallBase<T, Dimv>;
    using Base::rank, Base::size, Base::dimv, Base::len, Base::len_s, Base::step, Base::defsteps;
    using sub = typename nested_arg<T, ctuple<&Dim::len, dimv>>::sub;
    T * cp;

    using ViewConst = ViewSmall<T const, Dimv>;
    constexpr operator ViewConst () const requires (!std::is_const_v<T>) { return ViewConst(cp); }
    constexpr ViewSmall const & view() const { return *this; }
    constexpr T * data() const { return cp; }

    constexpr ViewSmall(T * cp_): cp(cp_) {}
// cf RA_ASSIGNOPS_SELF [ra38] [ra34]
    ViewSmall const & operator=(ViewSmall const & x) const { start(*this) = x; return *this; }
    constexpr ViewSmall(ViewSmall const & s) = default;

    template <class X> requires (!std::is_same_v<std::decay_t<X>, T>)
    constexpr ViewSmall const & operator=(X && x) const { start(*this) = x; return *this; }
#define ASSIGNOPS(OP)                                                   \
    constexpr ViewSmall const & operator OP(auto && x) const { start(*this) OP x; return *this; }
    FOR_EACH(ASSIGNOPS, *=, +=, -=, /=)
#undef ASSIGNOPS
// if T isn't is_scalar [ra44]
    constexpr ViewSmall const &
    operator=(T const & t) const
    {
        start(*this) = ra::scalar(t); return *this;
    }
// row-major ravel braces
    constexpr ViewSmall const &
    operator=(T (&&x)[size()]) const requires ((rank()>1) && (size()>1))
    {
        std::ranges::copy(std::ranges::subrange(x), begin()); return *this;
    }
// nested braces
    constexpr ViewSmall const &
    operator=(sub (&&x)[rank()>0 ? len(0) : 0]) const requires (0<rank() && 0!=len(0) && (1!=rank() || 1!=len(0)))
    {
        ra::iter<-1>(*this) = x; return *this;
    }

    template <int k>
    constexpr static dim_t
    select(dim_t i)
    {
        RA_CHECK(inside(i, len(k)), "Bad index ", i, " in len[", k, "]=", len(k), ".");
        return step(k)*i;
    }
    template <int k>
    constexpr static dim_t
    select(is_iota auto const & i)
    {
        static_assert((1>=i.n ? 1 : (i.s<0 ? -i.s : i.s)*(i.n-1)+1) <= len(k), "Bad index.");
        RA_CHECK(inside(i, len(k)), "Bad index iota [", i.n, " ", i.i, " ", i.s, "] in len[", k, "]=", len(k), ".");
        return 0==i.n ? 0 : step(k)*i.i;
    }
    template <int k, int n>
    constexpr static dim_t
    select(dots_t<n> i) { return 0; }

    template <int k, class I0, class ... I>
    constexpr static dim_t
    select_loop(I0 && i0, I && ... i)
    {
        constexpr int nn = (UNB==beatable<I0>.src) ? (rank() - k - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
        return select<k>(wlen(ic<len(k)>, RA_FWD(i0))) + select_loop<k + nn>(RA_FWD(i) ...);
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
            return ViewSmall<T, ic_t<filterdims<dimv, std::decay_t<I> ...>::dimv>> (self.cp + select_loop<0>(i ...));
// TODO partial beating
        } else {
// must fwd *this because we create temp views on every Small::view() call
            return unbeat<sizeof...(I)>::op(RA_FWD(self), RA_FWD(i) ...);
        }
    }
    constexpr decltype(auto)
    operator[](this auto && self, auto && ... i) { return RA_FWD(self)(RA_FWD(i) ...); }

    template <class I>
    constexpr decltype(auto)
    at(I && i) const
    {
// can't say 'frame rank 0' so -size wouldn't work. FIXME What about ra::len
        constexpr rank_t crank = rank_diff(rank(), ra::size_s(i));
        static_assert(crank>=0); // to make out the output type
        return iter<crank>().at(RA_FWD(i));
    }
// maybe remove if ic becomes easier to use
    template <int s, int o=0> constexpr auto as() const { return operator()(ra::iota(ra::ic<s>, o)); }
    template <rank_t c=0> constexpr auto iter() const { return Cell<T, ic_t<dimv>, ic_t<c>>(cp); }
    constexpr auto begin() const { if constexpr (defsteps) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (defsteps) { return cp+size(); }
    constexpr static auto end() requires (!defsteps) { return std::default_sentinel; }
    constexpr T & back() const { static_assert(size()>0, "Bad back()."); return cp[size()-1]; }
    constexpr operator T & () const { static_assert(1==size(), "Bad scalar conversion."); return cp[0]; }
};

#if defined (__clang__)
template <class T, int N> using extvector __attribute__((ext_vector_type(N))) = T;
#else
template <class T, int N> using extvector __attribute__((vector_size(N*sizeof(T)))) = T;
#endif

template <class Z, class ... T> constexpr static bool equal_to_any = (std::is_same_v<Z, T> || ...);

template <class T, size_t N>
consteval size_t
align_req()
{
    if constexpr (equal_to_any<T, char, unsigned char, short, unsigned short, int, unsigned int,
                  long, unsigned long, long long, unsigned long long, float, double
                  > && 0<N && 0==(N & (N-1))) {
        return alignof(extvector<T, N>);
    } else {
        return alignof(T[N]);
    }
}

template <class T, class Dimv, class ... nested_args>
struct
#if RA_OPT_SMALLVECTOR==1
alignas(align_req<T, std::apply([](auto ... i) { return (i.len * ... * 1); }, Dimv::value)>())
#else
#endif
SmallArray<T, Dimv, std::tuple<nested_args ...>>
    : public SmallBase<T, Dimv>
{
    using Base = SmallBase<T, Dimv>;
    using Base::rank, Base::size, Base::dimv;

    T cp[size()]; // cf what std::array does for zero size

    using View = ViewSmall<T, Dimv>;
    using ViewConst = ViewSmall<T const, Dimv>;
    constexpr View view() { return View(cp); }
    constexpr ViewConst view() const { return ViewConst(cp); }
    constexpr operator View () { return View(cp); }
    constexpr operator ViewConst () const { return ViewConst(cp); }

    constexpr SmallArray() {}
    constexpr SmallArray(ra::none_t) {}
// if T isn't is_scalar [ra44]
    constexpr SmallArray(T const & t) { std::ranges::fill(cp, t); }
// row-major ravel braces
    constexpr SmallArray(T const & x0, std::convertible_to<T> auto const & ... x)
    requires ((rank()>1) && (size()>1) && ((1+sizeof...(x))==size()))
    {
        view() = { static_cast<T>(x0), static_cast<T>(x) ... };
    }
// nested braces FIXME p1219??
    constexpr SmallArray(nested_args const & ... x)
    requires ((0<rank() && 0!=Base::len(0) && (1!=rank() || 1!=Base::len(0))))
    {
        view() = { x ... };
    }
// X && x makes this a better match than nested_args ... for 1 argument.
    template <class X> requires (!std::is_same_v<std::decay_t<X>, T>)
    constexpr SmallArray(X && x)
    {
        view() = RA_FWD(x);
    }
#define ASSIGNOPS(OP)                                                   \
    constexpr SmallArray & operator OP(auto && x) { view() OP RA_FWD(x); return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS

    constexpr decltype(auto) back(this auto && self) { return RA_FWD(self).view().back(); }
    constexpr auto data(this auto && self) { return self.view().data(); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... a) { return RA_FWD(self).view()(RA_FWD(a) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... a) { return RA_FWD(self).view()(RA_FWD(a) ...); }
    constexpr decltype(auto) at(this auto && self, auto && i) { return RA_FWD(self).view().at(RA_FWD(i)); }
    constexpr auto begin(this auto && self) { return self.view().begin(); }
    constexpr auto end(this auto && self) { return self.view().end(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { return RA_FWD(self).view().template iter<c>(); }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
// FIXME do (iota(ic<> ...)) instead
    template <int s, int o=0> constexpr decltype(auto) as(this auto && self) { return RA_FWD(self).view().template as<s, o>(); }
};

template <class A0, class ... A> SmallArray(A0, A ...) -> Small<A0, 1+sizeof...(A)>;

// FIXME tagged ravel constructor
template <class A>
constexpr auto
from_ravel(auto && b)
{
    A a;
    RA_CHECK(1==ra::rank(b) && ra::size(b)==ra::size(a),
             "Bad ravel argument [", fmt(nstyle, ra::shape(b)), "] expecting [", ra::size(a), "].");
    std::ranges::copy(RA_FWD(b), a.begin());
    return a;
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
    return ViewSmall<T, ic_t<default_dims<ra::shape(t)>>>(peel(t)).iter();
}


// --------------------
// Small view ops, see View ops in big.hh.
// FIXME Merge transpose(ViewBig), Reframe (eg beat(reframe(a)) -> transpose(a) ?)
// --------------------

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

RA_IS_DEF(cv_smallview, (std::is_convertible_v<A, ViewSmall<typename A::T, ic_t<A::dimv>>>));

template <int ... Iarg>
constexpr auto
transpose(cv_smallview auto && a_, ilist_t<Iarg ...>)
{
    decltype(auto) a = a_.view();
    using A = typename std::decay_t<decltype(a)>;
    constexpr static std::array<dim_t, sizeof...(Iarg)> s = { Iarg ... };
    constexpr static auto src = A::dimv;
    static_assert(src.size()==s.size(), "Bad size for transposed axes list.");
    constexpr static rank_t dstrank = (0==ra::size(s)) ? 0 : 1 + std::ranges::max(s);
    constexpr static auto dst = [&]() { std::array<Dim, dstrank> dst; transpose_dims(s, src, dst); return dst; }();
    return ViewSmall<typename A::T, ic_t<dst>>(a.data());
}

template <class sup_t, class T, class A, class B>
constexpr void
explode_dims(A const & av, B & bv)
{
    rank_t rb = ssize(bv);
    constexpr rank_t rs = rank_s<sup_t>();
    dim_t s = 1;
    for (int i=rb+rs; i<ssize(av); ++i) {
        RA_CHECK(av[i].step==s, "Subtype axes are not compact.");
        s *= av[i].len;
    }
    RA_CHECK(s*sizeof(T)==sizeof(value_t<sup_t>), "Mismatched types.");
    if constexpr (rs>0) {
        for (int i=rb; i<rb+rs; ++i) {
            RA_CHECK(sup_t::dimv[i-rb].len==av[i].len && s*sup_t::dimv[i-rb].step==av[i].step, "Mismatched axes.");
        }
    }
    s *= size_s<sup_t>();
    for (int i=0; i<rb; ++i) {
        dim_t step = av[i].step;
        RA_CHECK(0==step % s, "Step [", i, "] = ", step, " doesn't match ", s, ".");
        bv[i] = Dim { av[i].len, step/s };
    }
}

template <class sup_t>
constexpr auto
explode(cv_smallview auto && a)
{
    constexpr static rank_t ru = sizeof(value_t<sup_t>)==sizeof(value_t<decltype(a)>) ? 0 : 1;
    constexpr static auto bdimv = [&a]()
    {
        std::array<Dim, ra::rank_s(a)-rank_s<sup_t>()-ru> bdimv;
        explode_dims<sup_t, value_t<decltype(a)>>(a.dimv, bdimv);
        return bdimv;
    }();
    return ViewSmall<sup_t, ic_t<bdimv>>(reinterpret_cast<sup_t *>(a.data()));
}

// TODO generalize
constexpr auto
cat(cv_smallview auto && a1_, cv_smallview auto && a2_)
{
    decltype(auto) a1 = a1_.view();
    decltype(auto) a2 = a2_.view();
    static_assert(1==a1.rank() && 1==a2.rank(), "Bad ranks for cat.");
    Small<std::common_type_t<decltype(a1[0]), decltype(a2[0])>, a1.size()+a2.size()> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    std::copy(a2.begin(), a2.end(), val.begin()+a1.size());
    return val;
}

constexpr auto
cat(cv_smallview auto && a1_, is_scalar auto && a2_)
{
    decltype(auto) a1 = a1_.view();
    static_assert(1==a1.rank(), "Bad ranks for cat.");
    Small<std::common_type_t<decltype(a1[0]), decltype(a2_)>, a1.size()+1> val;
    std::copy(a1.begin(), a1.end(), val.begin());
    val[a1.size()] = a2_;
    return val;
}

constexpr auto
cat(is_scalar auto && a1_, cv_smallview auto && a2_)
{
    decltype(auto) a2 = a2_.view();
    static_assert(1==a2.rank(), "Bad ranks for cat.");
    Small<std::common_type_t<decltype(a1_), decltype(a2[0])>, 1+a2.size()> val;
    val[0] = a1_;
    std::copy(a2.begin(), a2.end(), val.begin()+1);
    return val;
}

} // namespace ra
