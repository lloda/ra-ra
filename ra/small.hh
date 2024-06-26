// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with static lengths/strides, cf big.hh.

// (c) Daniel Llorens - 2013-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"

namespace ra {

constexpr rank_t
rank_sum(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a+b; }

constexpr rank_t
rank_diff(rank_t a, rank_t b) { return (ANY==a || ANY==b) ? ANY : a-b; }

// cr>=0 is cell rank. -cr>0 is frame rank. TODO How to say frame rank 0.
constexpr rank_t
rank_cell(rank_t r, rank_t cr) { return cr>=0 ? cr /* indep */ : r==ANY ? ANY /* defer */ : (r+cr); }

constexpr rank_t
rank_frame(rank_t r, rank_t cr) { return r==ANY ? ANY /* defer */ : cr>=0 ? (r-cr) /* indep */ : -cr; }

struct Dim { dim_t len, step; };

inline std::ostream &
operator<<(std::ostream & o, Dim const & dim) { return (o << "[Dim " << dim.len << " " << dim.step << "]"); }

constexpr bool
is_c_order_dimv(auto const & dimv, bool unitstep=true)
{
    bool steps = true;
    dim_t s = 1;
    int k = dimv.size();
    if (!unitstep) {
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
is_c_order(auto const & v, bool unitstep=true) { return is_c_order_dimv(v.dimv, unitstep); }

constexpr dim_t
filldim(auto & dimv, auto && shape)
{
    map(&Dim::len, dimv) = shape;
    dim_t s = 1;
    for (int k=dimv.size(); --k>=0;) {
        dimv[k].step = s;
        RA_CHECK(dimv[k].len>=0, "Bad len[", k, "] ", dimv[k].len, ".");
        s *= dimv[k].len;
    }
    return s;
}

// FIXME parameterize Small on dimv, then simplify.
template <class lens>
struct default_steps_
{
    constexpr static int rank = mp::len<lens>;
    constexpr static auto dimv = [] { std::array<Dim, rank> dimv; filldim(dimv, mp::tuple2array<dim_t, lens>()); return dimv; } ();
    using type = decltype([] { return std::apply([](auto ... k) { return mp::int_list<dimv[k].step ...> {}; }, mp::iota<rank> {}); } ());
};
template <class lens> using default_steps = typename default_steps_<lens>::type;

constexpr dim_t
shape(auto const & v, auto && e)
{
    dim_t k = wlen(ra::rank(v), RA_FWD(e));
    RA_CHECK(inside(k, ra::rank(v)), "Bad axis ", k, " for rank ", ra::rank(v), ".");
    return v.len(k);
}

template <class A>
constexpr void
resize(A & a, dim_t s)
{
    if constexpr (ANY==size_s<A>()) {
        RA_CHECK(s>=0, "Bad resize ", s, ".");
        a.resize(s);
    } else {
        RA_CHECK(s==start(a).len(0) || BAD==s, "Bad resize ", s, " needing ", start(a).len(0), ".");
    }
}


// --------------------
// slicing helpers
// --------------------

template <int n=BAD> struct dots_t { static_assert(n>=0 || BAD==n); };
template <int n=BAD> constexpr dots_t<n> dots = dots_t<n>();
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
    = { .rt=(BAD!=I::nn), .ct=std::decay_t<decltype(wlen(ic<1>, std::declval<I>()))>::constant,
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
        return map(from_partial<mp::tuple<ic_t<rank_s<I>()> ...>, 1>(RA_FWD(a)), RA_FWD(i) ...);
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
struct unbeat<N, mp::int_list<k ...>>
{
    constexpr static decltype(auto)
    op(auto && v, auto && ... i)
    {
        return from(RA_FWD(v), wlen(maybe_len<k>(v), RA_FWD(i)) ...);
    }
};


// --------------------
// develop indices
// --------------------

template <rank_t k, rank_t end>
constexpr dim_t
indexer(auto const & q, auto && pp, auto const & ss0, dim_t c)
{
    if constexpr (k==end) {
        return c;
    } else {
        auto pk = *pp;
        RA_CHECK(inside(pk, q.len(k)) || (BAD==q.len(k) && 0==q.step(k)));
        return pp.mov(ss0), indexer<k+1, end>(q, pp, ss0, c + (q.step(k) * pk));
    }
}

constexpr dim_t
indexer(rank_t end, auto const & q, auto && pp, auto const & ss0)
{
    dim_t c = 0;
    for (rank_t k=0; k<end; ++k, pp.mov(ss0)) {
        auto pk = *pp;
        RA_CHECK(inside(pk, q.len(k)) || (BAD==q.len(k) && 0==q.step(k)));
        c += q.step(k) * pk;
    }
    return c;
}

template <class Q, class P>
constexpr dim_t
longer(Q const & q, P const & pp)
{
    decltype(auto) p = start(pp);
    if constexpr (ANY==rank_s<P>()) {
        RA_CHECK(1==rank(p), "Bad rank ", rank(p), " for subscript.");
    } else {
        static_assert(1==rank_s<P>(), "Bad rank for subscript.");
    }
    if constexpr (ANY==size_s<P>() || ANY==rank_s<Q>()) {
        RA_CHECK(p.len(0) >= q.rank(), "Too few indices.");
    } else {
        static_assert(size_s<P>() >= rank_s<Q>(), "Too few indices.");
    }
    if constexpr (ANY==rank_s<Q>()) {
        return indexer(q.rank(), q, p, p.step(0));
    } else {
        return indexer<0, rank_s<Q>()>(q, p, p.step(0), 0);
    }
}


// --------------------
// view iterators. TODO Take iterator like Ptr does and Views should, not raw pointers
// --------------------

template <auto f, auto dimv, int cellr, int framer=0>
using ctuple = decltype(std::apply([](auto ... i) { return mp::int_list<std::invoke(f, dimv[i]) ...> {}; }, mp::iota<cellr, framer> {}));

template <class lens, class steps>
constexpr static auto cdimv = mp::tuple2array<Dim, mp::zip<lens, steps>, [](auto i) { return std::make_from_tuple<Dim>(i); }>();

template <class T, class lens, class steps> struct ViewSmall;

template <class T, class Dimv, class Spec>
struct CellSmall
{
    constexpr static auto dimv = Dimv::value;

    constexpr static rank_t spec = Spec::value;
    constexpr static rank_t fullr = ssize(dimv);
    constexpr static rank_t cellr = rank_cell(fullr, spec);
    constexpr static rank_t framer = rank_frame(fullr, spec);
    static_assert(spec!=ANY && spec!=BAD && choose_rank(fullr, cellr)==fullr, "Bad cell rank.");
// FIXME Small take dimv instead of lens/steps
    using ctype = ViewSmall<T, ctuple<&Dim::len, dimv, cellr, framer>, ctuple<&Dim::step, dimv, cellr, framer>>;

    ctype c;

    consteval static rank_t rank() { return framer; }
    constexpr static dim_t len(int k) { return dimv[k].len; } // len(0<=k<rank) or step(0<=k)
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return k<rank() ? dimv[k].step : 0; }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }

    constexpr CellSmall(T * p): c { p } {}
};

template <class T, rank_t RANK=ANY> struct ViewBig;

template <class T, class Dimv, class Spec>
struct CellBig
{
    constexpr static rank_t spec = maybe_any<Spec>;
    constexpr static rank_t fullr = size_s<Dimv>();
    constexpr static rank_t cellr = is_constant<Spec> ? rank_cell(fullr, spec) : ANY;
    constexpr static rank_t framer = is_constant<Spec> ? rank_frame(fullr, spec) : ANY;
    using ctype = ViewBig<T, cellr>;

    Dimv dimv;
    ctype c;
    [[no_unique_address]] Spec const dspec = {};

    consteval static rank_t rank() requires (ANY!=framer) { return framer; }
    constexpr rank_t rank() const requires (ANY==framer) { return rank_frame(dimv.size(), dspec); }
    constexpr dim_t len(int k) const { return dimv[k].len; } // len(0<=k<rank) or step(0<=k)
    constexpr static dim_t len_s(int k) { return ANY; }
    constexpr dim_t step(int k) const { return k<rank() ? dimv[k].step : 0; }
    constexpr bool keep_step(dim_t st, int z, int j) const { return st*step(z)==step(j); }

    constexpr CellBig(T * cp, Dimv const & dimv_, Spec dspec_ = Spec {})
        : dimv(dimv_), dspec(dspec_)
    {
        c.cp = cp;
        rank_t dcellr = rank_cell(dimv.size(), dspec);
        rank_t dframer = rank();
        RA_CHECK(0<=dframer && 0<=dcellr, "Bad cell rank ", dcellr, " for array rank ", ssize(dimv), ").");
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
    using Base::Base, Base::cellr, Base::framer, Base::c, Base::step;
    using ctype = Base::ctype;

    static_assert(cellr>=0 || cellr==ANY, "Bad cell rank.");
    static_assert(framer>=0 || framer==ANY, "Bad frame rank.");

    RA_ASSIGNOPS_SELF(Cell)
    RA_ASSIGNOPS_DEFAULT_SET

    constexpr decltype(auto) at(auto const & i) const requires (0==cellr) { return c.cp[longer(*this, i)]; }
    constexpr decltype(auto) at(auto const & i) const requires (0!=cellr) { ctype cc(c); cc.cp += longer(*this, i); return cc; }
    constexpr void adv(rank_t k, dim_t d) { c.cp += step(k)*d; }
    constexpr decltype(auto) operator*() const requires (0==cellr) { return *(c.cp); }
    constexpr ctype const & operator*() const requires (0!=cellr) { return c; }
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

template <class T, class lens, class steps, class nested_args = small_args<T, lens>::nested>
struct SmallArray;

template <class T, dim_t ... lens>
using Small = SmallArray<T, mp::int_list<lens ...>, default_steps<mp::int_list<lens ...>>>;

template <class T, int S0, int ... S>
struct nested_arg<T, mp::int_list<S0, S ...>>
{
    using sub = std::conditional_t<0==sizeof...(S), T, Small<T, S ...>>;
};


// ---------------------
// Small view & container
// ---------------------

template <class lens_, class steps_, class ... I>
struct FilterDims
{
    using lens = lens_;
    using steps = steps_;
};

template <class lens_, class steps_, class I0, class ... I> requires (!is_iota<I0>)
struct FilterDims<lens_, steps_, I0, I ...>
{
    constexpr static bool stretch = (beatable<I0>.dst==BAD);
    static_assert(!stretch || ((beatable<I>.dst!=BAD) && ...), "Cannot repeat stretch index.");
    constexpr static int dst = stretch ? (mp::len<lens_> - (0 + ... + beatable<I>.src)) : beatable<I0>.dst;
    constexpr static int src = stretch ? (mp::len<lens_> - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
    using next = FilterDims<mp::drop<lens_, src>, mp::drop<steps_, src>, I ...>;
    using lens = mp::append<mp::take<lens_, dst>, typename next::lens>;
    using steps = mp::append<mp::take<steps_, dst>, typename next::steps>;
};

template <class lens_, class steps_, class I0, class ... I> requires (is_iota<I0>)
struct FilterDims<lens_, steps_, I0, I ...>
{
    constexpr static int dst = beatable<I0>.dst;
    constexpr static int src = beatable<I0>.src;
    using next = FilterDims<mp::drop<lens_, src>, mp::drop<steps_, src>, I ...>;
    using lens = mp::append<mp::int_list<I0::nn>, typename next::lens>;
    using steps = mp::append<mp::int_list<(mp::ref<steps_, 0>::value * I0::gets())>, typename next::steps>;
};

template <class T_, class lens_, class steps_>
struct SmallBase
{
    using lens = lens_;
    using steps = steps_;
    using T = T_;

    static_assert(mp::len<lens> == mp::len<steps>, "Mismatched lengths & steps.");
    consteval static rank_t rank() { return mp::len<lens>; }
    constexpr static auto dimv = cdimv<lens, steps>;
    constexpr static auto theshape = mp::tuple2array<dim_t, lens>();
    consteval static dim_t size() { return std::apply([](auto ... s) { return (s * ... * 1); }, theshape); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    consteval static dim_t size_s() { return size(); }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static decltype(auto) shape() { return theshape; }
// TODO check steps
    static_assert(std::apply([](auto ... s) { return ((0<=s) && ...); }, theshape), "Bad shape.");
    constexpr static dim_t len0 = rank()>0 ? len(0) : 0;
    constexpr static bool defsteps = is_c_order_dimv(dimv);
};

template <class T, class lens, class steps>
struct ViewSmall: public SmallBase<T, lens, steps>
{
    using Base = SmallBase<T, lens, steps>;
    using Base::rank, Base::size, Base::dimv;
    using Base::len, Base::len_s, Base::step, Base::len0, Base::defsteps;
    using sub = typename nested_arg<T, lens>::sub;

    T * cp;

    template <rank_t c=0> using iterator = Cell<T, ic_t<dimv>, ic_t<c>>;
    using ViewConst = ViewSmall<T const, lens, steps>;
    constexpr operator ViewConst () const requires (!std::is_const_v<T>) { return ViewConst(cp); }
    constexpr ViewSmall const & view() const { return *this; }

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
// needed if T isn't registered as scalar [ra44]
    constexpr ViewSmall const &
    operator=(T const & t) const
    {
        start(*this) = ra::scalar(t); return *this;
    }
// nested braces
    constexpr ViewSmall const &
    operator=(sub (&&x)[len0]) const requires (0<rank() && 0!=len0 && (1!=rank() || 1!=len0))
    {
        ra::iter<-1>(*this) = x; return *this;
    }
// row-major ravel braces
    constexpr ViewSmall const &
    operator=(T (&&x)[size()]) const requires ((rank()>1) && (size()>1))
    {
        std::ranges::copy(std::ranges::subrange(x), begin()); return *this;
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
    select(is_iota auto i)
    {
        if constexpr ((1>=i.n ? 1 : (i.s<0 ? -i.s : i.s)*(i.n-1)+1) > len(k)) { // FIXME c++23 std::abs
            static_assert(always_false<decltype(k)>, "Bad index.");
        } else {
            RA_CHECK(inside(i, len(k)), "Bad index iota [", i.n, " ", i.i, " ", i.s, "] in len[", k, "]=", len(k), ".");
        }
        return 0==i.n ? 0 : step(k)*i.i;
    }
    template <int k, int n>
    constexpr static dim_t
    select(dots_t<n> i)
    {
        return 0;
    }
    template <int k, class I0, class ... I>
    constexpr static dim_t
    select_loop(I0 && i0, I && ... i)
    {
        constexpr int nn = (BAD==beatable<I0>.src) ? (rank() - k - (0 + ... + beatable<I>.src)) : beatable<I0>.src;
        return select<k>(wlen(ic<len(k)>, RA_FWD(i0)))
            + select_loop<k + nn>(RA_FWD(i) ...);
    }
    template <int k>
    consteval static dim_t
    select_loop() { return 0; }

    template <class ... I>
    constexpr decltype(auto)
    operator()(I && ... i) const
    {
        constexpr int stretch = (0 + ... + (beatable<I>.dst==BAD));
        static_assert(stretch<=1, "Cannot repeat stretch index.");
        if constexpr ((0 + ... + is_scalar_index<I>)==rank()) {
            return cp[select_loop<0>(i ...)];
// FIXME wlen before this, cf is_constant_iota
        } else if constexpr ((beatable<I>.ct && ...)) {
            using FD = FilterDims<lens, steps, std::decay_t<I> ...>;
            return ViewSmall<T, typename FD::lens, typename FD::steps> (cp + select_loop<0>(i ...));
// TODO partial beating
        } else {
// FIXME must forward *this so that expr can hold to it (c++23 deducing this).
// Container's view is self so we get away with a ref, but here we create new temp views on every Small::view() call.
            return unbeat<sizeof...(I)>::op(ViewSmall(*this), RA_FWD(i) ...);
        }
    }
    constexpr decltype(auto)
    operator[](auto && ... i) const { return (*this)(RA_FWD(i) ...); } // see above about forwarding

    template <class I>
    constexpr decltype(auto)
    at(I && i) const
    {
// can't say 'frame rank 0' so -size wouldn't work. What about ra::len
        constexpr rank_t crank = rank_diff(rank(), ra::size_s<I>());
        static_assert(crank>=0); // to make out the output type
        return iter<crank>().at(RA_FWD(i));
    }
// maybe remove if ic becomes easier to use
    template <int ss, int oo=0> constexpr auto as() const { return operator()(ra::iota(ra::ic<ss>, oo)); }
    constexpr T * data() const { return cp; }
    template <rank_t c=0> constexpr iterator<c> iter() const { return cp; }
    constexpr auto begin() const { if constexpr (defsteps) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (defsteps) { return cp+size(); }
    constexpr static auto end() requires (!defsteps) { return std::default_sentinel; }
    constexpr T & back() const { static_assert(rank()>=1 && size()>0, "No back()."); return cp[size()-1]; }
    constexpr operator T & () const { static_assert(1==size(), "Bad conversion to scalar."); return cp[0]; }
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
    if constexpr (equal_to_any<T, char, unsigned char, short, unsigned short,
                  int, unsigned int, long, unsigned long, long long, unsigned long long,
                  float, double>
                  && 0<N && 0==(N & (N-1))) {
        return alignof(extvector<T, N>);
    } else {
        return alignof(T[N]);
    }
}

template <class T, class lens, class steps, class ... nested_args>
struct
#if RA_DO_OPT_SMALLVECTOR==1
alignas(align_req<T, std::apply([](auto && ... i) { return (i * ... * 1); }, lens {})>())
#else
#endif
SmallArray<T, lens, steps, std::tuple<nested_args ...>>
    : public SmallBase<T, lens, steps>
{
    using Base = SmallBase<T, lens, steps>;
    using Base::rank, Base::size, Base::len0;

    T cp[size()]; // cf what std::array does for zero size; wish zero size just worked :-/

    using View = ViewSmall<T, lens, steps>;
    using ViewConst = ViewSmall<T const, lens, steps>;
    constexpr View view() { return View(cp); }
    constexpr ViewConst view() const { return ViewConst(cp); }
    constexpr operator View () { return View(cp); }
    constexpr operator ViewConst () const { return ViewConst(cp); }

    constexpr SmallArray() {}
    constexpr SmallArray(ra::none_t) {}
// needed if T isn't registered as scalar [ra44]
    constexpr SmallArray(T const & t)
    {
        for (auto & x: cp) { x = t; }
    }
// nested braces FIXME p1219??
    constexpr SmallArray(nested_args const & ... x)
    requires ((0<rank() && 0!=Base::len(0) && (1!=rank() || 1!=Base::len(0))))
    {
        view() = { x ... };
    }
// row-major ravel braces
    constexpr SmallArray(T const & x0, std::convertible_to<T> auto const & ... x)
    requires ((rank()>1) && (size()>1) && ((1+sizeof...(x))==size()))
    {
        view() = { static_cast<T>(x0), static_cast<T>(x) ... };
    }
// X && x makes this a better match than nested_args ... for 1 argument.
    template <class X> requires (!std::is_same_v<std::decay_t<X>, T>)
    constexpr SmallArray(X && x)
    {
        view() = RA_FWD(x);
    }
#define ASSIGNOPS(OP)                                                   \
    constexpr decltype(auto) operator OP(auto && x) { view() OP RA_FWD(x); return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS

#define RA_CONST_OR_NOT(CONST)                                          \
    constexpr T CONST & back() CONST { return view().back(); }          \
    constexpr T CONST * data() CONST { return view().data(); }          \
    constexpr operator T CONST & () CONST { return view(); }            \
    constexpr decltype(auto) operator()(auto && ... a) CONST { return view()(RA_FWD(a) ...); } \
    constexpr decltype(auto) operator[](auto && ... a) CONST { return view()(RA_FWD(a) ...); } \
    constexpr decltype(auto) at(auto && i) CONST { return view().at(RA_FWD(i)); } \
    template <int ss, int oo=0> constexpr decltype(auto) as() CONST { return view().template as<ss, oo>(); } \
    template <rank_t c=0> constexpr auto iter() CONST { return view().template iter<c>(); } \
    constexpr auto begin() CONST { return view().begin(); }             \
    constexpr auto end() CONST { return view().end(); }
    FOR_EACH(RA_CONST_OR_NOT, /*not const*/, const)
#undef RA_CONST_OR_NOT
};

template <class A0, class ... A> SmallArray(A0, A ...) -> Small<A0, 1+sizeof...(A)>;

// FIXME tagged ravel constructor
template <class A>
constexpr auto
from_ravel(auto && b)
{
    A a;
    RA_CHECK(1==ra::rank(b) && ra::size(b)==ra::size(a),
             "Bad ravel argument [", ra::noshape, ra::shape(b), "] expecting [", ra::size(a), "].");
    std::ranges::copy(RA_FWD(b), a.begin());
    return a;
}


// ---------------------
// builtin arrays.
// ---------------------

template <class T>
constexpr auto
peel(T && t)
{
    static_assert(0 < std::extent_v<std::remove_cvref_t<T>, 0>);
    if constexpr (1 < std::rank_v<std::remove_cvref_t<T>>) {
        return peel(*std::data(RA_FWD(t)));
    } else {
        return std::data(t);
    }
}

constexpr auto
start(is_builtin_array auto && t)
{
    using A = std::remove_reference_t<decltype(t)>; // preserve const
    using lens = decltype(std::apply([](auto ... i) { return mp::int_list<std::extent_v<A, i> ...> {}; },
                                     mp::iota<std::rank_v<A>> {}));
    return ViewSmall<std::remove_all_extents_t<A>, lens, default_steps<lens>>(peel(t)).iter();
}


// --------------------
// Small view ops, see View ops in big.hh.
// FIXME Merge transpose(ViewBig), Reframe (eg beat(reframe(a)) -> transpose(a) ?)
// --------------------

constexpr void
transpose_filldim(auto const & s, auto const & src, auto & dst)
{
    std::ranges::fill(dst, Dim { BAD, 0 });
    for (int k=0; int sk: s) {
        dst[sk].step += src[k].step;
        dst[sk].len = dst[sk].len>=0 ? std::min(dst[sk].len, src[k].len) : src[k].len;
        ++k;
    }
}

RA_IS_DEF(cv_smallview, (std::is_convertible_v<A, ViewSmall<typename A::T, typename A::lens, typename A::steps>>));

template <int ... Iarg>
constexpr auto
transpose(cv_smallview auto && a_)
{
    decltype(auto) a = a_.view();
    using AA = typename std::decay_t<decltype(a)>;
    constexpr std::array<dim_t, sizeof...(Iarg)> s = { Iarg ... };
    constexpr auto src = cdimv<typename AA::lens, typename AA::steps>;
    static_assert(src.size()==s.size(), "Bad size for transposed axes list.");
    constexpr rank_t dstrank = (0==ra::size(s)) ? 0 : 1 + *std::ranges::max_element(s);
    constexpr auto dst = [&]() { std::array<Dim, dstrank> dst; transpose_filldim(s, src, dst); return dst; }();
    return ViewSmall<typename AA::T, ctuple<&Dim::len, dst, dstrank>, ctuple<&Dim::step, dst, dstrank>>(a.data());
}

constexpr auto
diag(cv_smallview auto && a)
{
    return transpose<0, 0>(a);
}

template <class super_t>
constexpr auto
explode(cv_smallview auto && a_)
{
// result has steps in super_t, but to support general steps we'd need steps in T. FIXME?
    decltype(auto) a = a_.view();
    using AA = std::decay_t<decltype(a)>;
    static_assert(super_t::defsteps);
    constexpr rank_t ra = ra::rank_s<AA>();
    constexpr rank_t rb = rank_s<super_t>();
    static_assert(std::is_same_v<mp::drop<typename AA::lens, ra-rb>, typename super_t::lens>);
    static_assert(std::is_same_v<mp::drop<typename AA::steps, ra-rb>, typename super_t::steps>);
    constexpr dim_t supers = ra::size_s<super_t>();
    using csteps = decltype(std::apply([](auto ... i)
                                       {
                                           static_assert(((i==(i/supers)*supers) && ...));
                                           return mp::int_list<(i/supers) ...> {};
                                       }, mp::take<typename AA::steps, ra-rb> {}));
    return ViewSmall<super_t, mp::take<typename AA::lens, ra-rb>, csteps>((super_t *) a.data());
}

// TODO generalize
template <class A1, class A2> requires (cv_smallview<A1> || cv_smallview<A2>)
constexpr auto
cat(A1 && a1_, A2 && a2_)
{
    if constexpr (cv_smallview<A1> && cv_smallview<A2>) {
        decltype(auto) a1 = a1_.view();
        decltype(auto) a2 = a2_.view();
        static_assert(1==a1.rank() && 1==a2.rank(), "Bad ranks for cat.");
        using T = std::common_type_t<std::decay_t<decltype(a1[0])>, std::decay_t<decltype(a2[0])>>;
        Small<T, a1.size()+a2.size()> val;
        std::copy(a1.begin(), a1.end(), val.begin());
        std::copy(a2.begin(), a2.end(), val.begin()+a1.size());
        return val;
    } else if constexpr (cv_smallview<A1> && is_scalar<A2>) {
        decltype(auto) a1 = a1_.view();
        static_assert(1==a1.rank(), "Bad ranks for cat.");
        using T = std::common_type_t<std::decay_t<decltype(a1[0])>, decltype(a2_)>;
        Small<T, a1.size()+1> val;
        std::copy(a1.begin(), a1.end(), val.begin());
        val[a1.size()] = a2_;
        return val;
    } else if constexpr (is_scalar<A1> && cv_smallview<A2>) {
        decltype(auto) a2 = a2_.view();
        static_assert(1==a2.rank(), "Bad ranks for cat.");
        using T = std::common_type_t<decltype(a1_), std::decay_t<decltype(a2[0])>>;
        Small<T, 1+a2.size()> val;
        val[0] = a1_;
        std::copy(a2.begin(), a2.end(), val.begin()+1);
        return val;
    } else {
        static_assert(always_false<A1, A2>);
    }
}

} // namespace ra
