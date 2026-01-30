// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression templates with prefix matching.

// (c) Daniel Llorens - 2011-2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cassert>
#include <functional>
#include "base.hh"

// See examples/throw.cc to customize error handling.

#if !defined(RA_CHECK)
  #define RA_CHECK 1 // tell users
#endif
#if RA_CHECK==0
  #define RA_CK(...)
#else
  #ifdef RA_ASSERT
    #define RA_CK(...) RA_ASSERT(__VA_ARGS__)
  #elif RA_CHECK==1
    #define RA_CK(cond, ...) { assert(cond); }
  #elif RA_CHECK==2
    #include <iostream>
    #define RA_CK(cond, ...)                                            \
    {                                                                   \
        if consteval {                                                  \
            assert(cond /* FIXME show args */);                         \
        } else {                                                        \
            if (!(cond)) [[unlikely]] {                                 \
                ra::print(std::cerr, "*** ra::", std::source_location::current(), " (" RA_STRINGIZE(cond) ") " __VA_OPT__(,) __VA_ARGS__, " ***") << std::endl; \
                std::abort();                                           \
            }                                                           \
        }                                                               \
    }
  #else
    #error Bad RA_CHECK
  #endif
#endif
#define RA_AFTER_CHECK Yes

namespace ra {

// Assign ops for Iterators, might be different for Views.

#define RA_ASSIGNOPS_LINE(OP)                                           \
    for_each([](auto && y, auto && x){ /* [ra5] */ RA_FW(y) OP RA_FW(x); }, *this, RA_FW(x))
#define RA_ASSIGNOPS_DEFAULT(OP)                                        \
    constexpr void operator OP(auto && x) { RA_ASSIGNOPS_LINE(OP); }
// Restate for expression classes since a template doesn't replace the copy assignment op.
#define RA_ASSIGNOPS_ITER(TYPE)                                         \
    constexpr TYPE & operator=(TYPE && x) { RA_ASSIGNOPS_LINE(=); return *this; } \
    constexpr TYPE & operator=(TYPE const & x) { RA_ASSIGNOPS_LINE(=); return *this; } \
    constexpr TYPE(TYPE && x) = default;                                \
    constexpr TYPE(TYPE const & x) = default;                           \
    RA_FE(RA_ASSIGNOPS_DEFAULT, =, *=, +=, -=, /=)

// Contextual len, a unique object. See wlen in arrays.hh

constexpr struct Len
{
    consteval static rank_t rank() { return 0; }
    [[noreturn]] consteval static void len_out_of_context() { std::abort(); }
    consteval static dim_t len_s(int k) { len_out_of_context(); }
    consteval static dim_t len(int k) { len_out_of_context(); }
    consteval static dim_t step(int k) { len_out_of_context(); }
    consteval static void adv(rank_t k, dim_t d) { len_out_of_context(); }
    consteval static bool keep(dim_t st, int z, int j) { len_out_of_context(); }
    consteval dim_t operator*() const { len_out_of_context(); }
    consteval static int save() { len_out_of_context(); }
    consteval static void load(int) { len_out_of_context(); }
    consteval static void mov(dim_t d) { len_out_of_context(); }
} len;

template <class E> struct WLen; // defined in ply.hh. FIXME C++ p2481
template <class E> concept has_len = requires(int ln, E && e) { WLen<std::decay_t<E>>::f(ln, RA_FW(e)); };
template <has_len E> constexpr bool is_special_def<E> = true; // protect exprs with Len from reduction.

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

template <class N> constexpr auto
maybe_any = []{ if constexpr (is_ctype<N>) { return N::value; } else { return ANY; } }();

template <class S> constexpr S
maybe_step = []{ if constexpr (is_ctype<S>) { static_assert(1==S::value); return S{}; } else { return 1; } }();

template <class K> constexpr auto
clen(auto const & v, K k) { if constexpr (is_ctype<K> && (ANY!=v.len_s(k))) return ic<v.len_s(k)>; else return v.len(k); }

template <class A, class B> constexpr auto
cadd(A a, B b) { if constexpr (is_ctype<A> && is_ctype<B>) return ic<a+b>; else return a+b; }

template <class A, class B> constexpr auto
csub(A a, B b) { if constexpr (is_ctype<A> && is_ctype<B>) return ic<a-b>; else return a-b; }

// Sequence stl-like iterator

template <class I=dim_t>
struct Seq
{
    I i;
    static_assert(has_len<I> || std::is_arithmetic_v<I>);
    using difference_type = dim_t;
    using value_type = I;
    constexpr I operator*() const { return i; }
    constexpr I operator[](dim_t d) const { return i+I(d); }
    constexpr decltype(auto) operator++() { ++i; return *this; }
    constexpr decltype(auto) operator--() { --i; return *this; }
    constexpr auto operator-(Seq const & j) const { return i-j.i; }
    constexpr auto operator+(dim_t d) const { return Seq{i+I(d)}; }
    constexpr auto operator-(dim_t d) const { return Seq{i-I(d)}; }
    constexpr friend auto operator+(dim_t d, Seq const & j) { return Seq{I(d)+j.i}; }
    constexpr auto operator++(int) { return Seq{i++}; }
    constexpr auto operator--(int) { return Seq{i--}; }
    constexpr decltype(auto) operator+=(dim_t d) { i+=I(d); return *this; }
    constexpr decltype(auto) operator-=(dim_t d) { i-=I(d); return *this; }
    constexpr auto operator<=>(Seq const & j) const { return i<=>j.i; }
    constexpr bool operator==(Seq const & j) const = default;
};

// Rank-0 Iterator, to wrap foreign objects, or to manipulate prefix matching.
// f(scalar(C)) should be f(C) and not map(f, C), this is controlled by tomap/toreduce.

template <class C>
struct Scalar final
{
    C c;
    RA_FE(RA_ASSIGNOPS_DEFAULT, =, *=, +=, -=, /=)
    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); } // FIXME consteval cf Match::check_s
    constexpr static dim_t len(int k) { std::abort(); } // FIXME idem
    constexpr static dim_t step(int k) { return 0; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static bool keep(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) at(auto && i) const { return c; }
    constexpr std::conditional_t<std::is_lvalue_reference_v<C>, C, C const &> operator*() const { return c; } // [ra24] [ra37] [ra39]
    consteval static int save() { return 0; }
    constexpr static void load(int) {}
    constexpr static void mov(dim_t d) {}
};

template <class C> constexpr auto
scalar(C && c) { return Scalar<C> { RA_FW(c) }; }

// Making Iterators

constexpr auto iter(is_scalar auto && a) { return ra::scalar(RA_FW(a)); }
// iterators need resetting on each use [ra35].
constexpr auto iter(is_iterator auto & a) requires (!(requires { []<class C>(Scalar<C> &){}(a); })) { return a; }
constexpr decltype(auto) iter(is_iterator auto && a) { return RA_FW(a); }
// Cell doesn't retain rvalues [ra4]. If both Slice and Iterator (Ptr), go as Iterator.
// TODO any exprs? runtime cr? ra::len in cr?
template <int cr=0> constexpr auto iter(Slice auto && a) requires (!is_iterator<decltype(a)>) { return RA_FW(a).template iter<cr>(); }

// forward decl.
constexpr auto iter(is_fov auto && a);
constexpr auto iter(is_builtin auto && a);
template <class T> constexpr auto iter(std::initializer_list<T> a);

template <class A>
constexpr decltype(auto)
VAL(A && a)
{
    if constexpr (is_scalar<A>) { return RA_FW(a); } // [ra8]
    else if constexpr (is_iterator<A>) { return *a; } // no need to iter()
    else if constexpr (requires { *ra::iter(RA_FW(a)); }) { return *ra::iter(RA_FW(a)); }
    // else void
}

template <class A> using value_t = std::remove_volatile_t<std::remove_reference_t<decltype(VAL(std::declval<A>()))>>;
template <class A> using ncvalue_t = std::remove_const_t<value_t<A>>;

constexpr decltype(auto)
to_scalar(auto && e)
{
    if constexpr (constexpr dim_t s=size_s(e); 1!=s) {
        static_assert(ANY==s, "Bad scalar conversion.");
        RA_CK(1==ra::size(e), "Bad scalar conversion from shape [", fmt(lstyle, ra::shape(e)), "].");
    }
    return VAL(e);
}


// --------------------
// View Iterators.
// --------------------

constexpr auto
indexer(auto & a, auto cp, Iterator auto && p)
{
    if constexpr (ANY==rank_s(p)) {
        RA_CK(1==ra::rank(p), "Bad rank ", ra::rank(p), " for subscript.");
    } else {
        static_assert(1==rank_s(p), "Bad rank for subscript.");
    }
    if constexpr (ANY==size_s(p) || ANY==rank_s(a)) {
        RA_CK(p.len(0) >= a.rank(), "Too few indices.");
    } else {
        static_assert(size_s(p) >= rank_s(a), "Too few indices.");
    }
    for (rank_t k=0; k<a.rank(); ++k, p.mov(p.step(0))) {
        auto i = *p;
        RA_CK(inside(i, a.len(k)) || UNB==a.len(k), "Bad i[", k, "]=", i, " for len", a.len(k));
        std::ranges::advance(cp, i*a.step(k));
    }
    return cp;
}

struct Dim { dim_t len, step; };

inline std::ostream &
operator<<(std::ostream & o, Dim const & dim) { std::print(o, "[Dim {} {}]", dim.len, dim.step); return o; }

constexpr void
resize(auto & a, dim_t s)
{
    if constexpr (ANY==size_s(a)) {
        RA_CK(s>=0, "Bad resize ", s, ".");
        a.resize(s);
    } else {
        RA_CK(s==ra::iter(a).len(0) || UNB==s, "Bad resize ", s, ", need ", ra::iter(a).len(0), ".");
    }
}

constexpr dim_t
filldimv(Iterator auto && p, auto & dimv)
{
    ra::resize(dimv, 0==rank_s(p) ? 1 : ra::size(p)); // [ra37]
    for (rank_t k=0; k<ra::size(dimv); ++k, p.mov(p.step(0))) { dimv[k].len = *p; }
    dim_t s = 1;
    for (int k=ra::size(dimv); --k>=0;) {
        dimv[k].step = s;
        RA_CK(dimv[k].len>=0, "Bad len[", k, "] ", dimv[k].len, ".");
        s *= dimv[k].len;
    }
    return s;
}

consteval auto default_dims(auto lv) { std::array<Dim, ra::size(lv)> dv; filldimv(ra::iter(lv), dv); return dv; };

constexpr rank_t rank_sum(rank_t a, rank_t b) { return ANY==a || ANY==b ? ANY : a+b; }
constexpr rank_t rank_diff(rank_t a, rank_t b) { return ANY==a || ANY==b ? ANY : a-b; }
// cr>=0 is cell rank, -cr>0 is frame rank. TODO frame rank 0? maybe ra::len
constexpr rank_t rank_cell(rank_t r, rank_t cr) { return cr>=0 ? cr : r==ANY ? ANY : (r+cr); }
constexpr rank_t rank_frame(rank_t r, rank_t cr) { return r==ANY ? ANY : cr>=0 ? (r-cr) : -cr; }

template <class T, class Dimv> struct ViewSmall;
template <class T, rank_t RANK=ANY> struct ViewBig;

template <class P, class Dimv, class Cr>
struct CellBase
{
    constexpr static Dimv const simv = {};
    [[no_unique_address]] Dimv const dimv;
    constexpr static rank_t cr = maybe_any<Cr>;
    constexpr static rank_t cellr = is_ctype<Cr> ? rank_cell(size_s<Dimv>(), cr) : ANY;
    constexpr static rank_t framer = is_ctype<Cr> ? rank_frame(size_s<Dimv>(), cr) : ANY;
    using View = std::conditional_t<0==cellr, ViewSmall<P, ic_t<std::array<Dim, 0> {}>>, ViewBig<P, cellr>>; // FIXME
};

template <class P, is_ctype Dimv, class Cr>
struct CellBase<P, Dimv, Cr>
{
    constexpr static auto simv = Dimv::value;
    constexpr static auto dimv = simv;
    constexpr static rank_t cr = maybe_any<Cr>;
    constexpr static rank_t cellr = is_ctype<Cr> ? rank_cell(size_s(simv), cr) : ANY;
    constexpr static rank_t framer = is_ctype<Cr> ? rank_frame(size_s(simv), cr) : ANY;
    using View = ViewSmall<P, ic_t<std::apply([](auto ... i){ return std::array<Dim, cellr> {dimv[i+framer] ...}; }, mp::iota<cellr> {})>>;
};

template <class P, class Dimv, class Cr>
struct Cell: public CellBase<P, Dimv, Cr>
{
    static_assert(has_len<P> || std::bidirectional_iterator<P>);
    using Base = CellBase<P, Dimv, Cr>;
    using Base::cellr, Base::framer, Base::simv, Base::dimv, typename Base::View;
    static_assert((cellr>=0 || cellr==ANY) && (framer>=0 || framer==ANY), "Bad cell/frame ranks.");
    View c;
    constexpr explicit Cell(P p) requires (is_ctype<Dimv>): c {p} {}
    constexpr explicit Cell(P cp, Dimv dimv_, Cr dcr=Cr {}) requires (!is_ctype<Dimv>): Base { dimv_ }, c(cp)
    {
        rank_t dcell = rank_cell(ra::size(dimv), dcr);
        if constexpr (ANY==cellr) { c.dimv.resize(dcell); }
        rank_t dframe = rank(); // after sizing c.dimv
        RA_CK(0<=dframe && 0<=dcell, "Bad rank for cell ", dcell, " or frame ", dframe, ".");
        if constexpr (0!=cellr) { for (int k=0; k<dcell; ++k) { c.dimv[k] = dimv[dframe+k]; } }
    }
    RA_ASSIGNOPS_ITER(Cell)
    consteval static rank_t rank() requires (ANY!=framer) { return framer; }
    constexpr rank_t rank() const requires (ANY==framer) { return ra::size(dimv)-ra::size(c.dimv); }
#pragma GCC diagnostic push // test/bug83.cc gcc-14 RA_CHECK=0 --no-sanitize
#pragma GCC diagnostic warning "-Warray-bounds"
#define RAC(k, f) is_ctype<decltype(simv[k].f)>
    constexpr static dim_t len_s(auto k) { if constexpr (is_ctype<Dimv> || RAC(k, len)) return len(k); else return ANY; }
    constexpr static dim_t len(auto k) requires (is_ctype<Dimv> || RAC(k, len)) { return simv[k].len; }
    constexpr dim_t len(int k) const requires (!(is_ctype<Dimv> || RAC(k, len))) { return dimv[k].len; }
    constexpr static dim_t step(auto k) requires (is_ctype<Dimv> || RAC(k, step)) { return k<rank() ? simv[k].step : 0; }
    constexpr dim_t step(int k) const requires (!(is_ctype<Dimv> || RAC(k, step))) { return k<rank() ? dimv[k].step : 0; }
    constexpr static bool keep(dim_t st, auto z, auto j) requires (is_ctype<Dimv> || (RAC(z, step) && RAC(j, step))) { return st*step(z)==step(j); }
    constexpr bool keep(dim_t st, int z, int j) const requires (!(is_ctype<Dimv> || (RAC(z, step) && RAC(j, step)))) { return st*step(z)==step(j); }
#undef RAC
#pragma GCC diagnostic pop
    constexpr void adv(rank_t k, dim_t d) { mov(step(k)*d); }
    constexpr decltype(*c.cp) at(auto const & i) const requires (0==cellr) { return *indexer(*this, c.cp, ra::iter(i)); }
    constexpr View at(auto const & i) const requires (0!=cellr) { View d(c); d.cp=indexer(*this, d.cp, ra::iter(i)); return d; }
    constexpr decltype(*c.cp) operator*() const requires (0==cellr) { return *(c.cp); }
    constexpr View const & operator*() const requires (0!=cellr) { return c; }
    constexpr operator decltype(*c.cp) () const { return to_scalar(*this); } /* [ra45] */
    constexpr auto save() const { return c.cp; }
    constexpr void load(P p) { c.cp=p; }
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Waggressive-loop-optimizations" // Seq<!=dim_t> in gcc14/15 -O3
    constexpr void mov(dim_t d) { std::ranges::advance(c.cp, d); }
#pragma GCC diagnostic pop
// for the sake of iota(), which needs to be both iterator and slice. FIXME make iota() View not Cell.
    template <rank_t c=0> constexpr auto iter() const requires (0==cellr && 1==framer) { static_assert(0==c); return *this; }
    constexpr auto data() const { return c.cp; }
};

template <class X> using sarg = std::conditional_t<is_ctype<std::decay_t<X>> || is_scalar<X>, std::decay_t<X>, X>;

template <class N, class S> struct SDim { [[no_unique_address]] N const len = {}; [[no_unique_address]] S const step = {}; };
template <class P, class N, class S> using Ptr = Cell<P, std::array<SDim<N, S>, 1>, ic_t<0>>;

template <class P, class N=ic_t<dim_t(UNB)>, class S=ic_t<dim_t(1)>>
constexpr auto
ptr(P && p, N && n=N {}, S && s=S(maybe_step<S>))
{
    if constexpr (std::ranges::bidirectional_range<std::remove_reference_t<P>>) {
        static_assert(std::is_same_v<ic_t<dim_t(UNB)>, N>, "Conflict with own length.");
        static_assert(std::is_same_v<ic_t<dim_t(1)>, S>, "Deduced size means unit step.");
        return ptr(std::begin(RA_FW(p)), [&p](){ if constexpr (ANY==size_s(p)) return ra::size(p); else return ic<size_s(p)>; }(), RA_FW(s));
    } else {
        if constexpr (std::is_integral_v<N>) { RA_CK(n>=0, "Bad Ptr length ", n, "."); }
        return Ptr<std::decay_t<P>, sarg<N>, sarg<S>> { p, {ra::SDim<sarg<N>, sarg<S>> {.len=RA_FW(n), .step=RA_FW(s) }}};
    }
}

template <class I, class N, class S, class K=ic_t<dim_t(0)>>
constexpr auto
reverse(Ptr<Seq<I>, N, S> const & i, K k = {})
{
    static_assert(UNB!=i.len_s(0), "Bad arguments to reverse.");
    return ptr(Seq { i.data().i+(i.dimv[0].len-1)*i.dimv[0].step }, i.dimv[0].len, csub(ic<0>, i.dimv[0].step));
}

constexpr auto iter(is_fov auto && a) { return ra::ptr(RA_FW(a)); }
template <class T> constexpr auto iter(std::initializer_list<T> a) { return ra::ptr(a.begin(), a.size()); }

constexpr auto
iter(is_builtin auto && a)
{
    using T = std::remove_all_extents_t<std::remove_reference_t<decltype(a)>>; // preserve const
    return Cell<T *, ic_t<default_dims(ra::shape(a))>, ic_t<0>>(
        [](this auto const & self, auto && a){
            using T = std::remove_cvref_t<decltype(a)>;
            if constexpr (1 < std::rank_v<T>) {
                static_assert(0 < std::extent_v<T, 0>);
                return self(*std::data(a));
            } else {
                return std::data(a);
            }
        }(a));
}


// ---------------------------
// Reframe and rank conjunction.
// ---------------------------

// Transpose for general expressions. Like in transpose, give destination axis for each original axis.
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// Else axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO Handle repeated axes. Handle ANY rank [ra7].

template <dim_t N, class T> constexpr T samestep = N;
template <dim_t N, class ... T> constexpr std::tuple<T ...> samestep<N, std::tuple<T ...>> = { samestep<N, T> ... };

template <Iterator A, class Dest, class I=mp::iota<mp::len<Dest>>> struct Reframe;
template <Iterator A, int ... di, int ... i>
struct Reframe<A, ilist_t<di ...>, ilist_t<i ...>>
{
    A a;
    consteval static rank_t rank() { return 0==sizeof...(i) ? 0 : 1+std::ranges::max(std::array<int, sizeof...(i)> { di ... }); }
    constexpr static int orig(int k) { int r=-1; (void)((di==k && (r=i, 1)) || ...); return r; }
    constexpr static dim_t len_s(int k)
    {
        int l=orig(k);
        return l>=0 ? std::decay_t<A>::len_s(l) : UNB;
    }
    constexpr static dim_t
    len(int k) requires (requires { std::decay_t<A>::len(k); })
    {
        return len_s(k);
    }
    constexpr dim_t
    len(int k) const requires (!(requires { std::decay_t<A>::len(k); }))
    {
        int l=orig(k);
        return l>=0 ? a.len(l) : UNB;
    }
    constexpr static bool
    keep(dim_t st, int z, int j) requires (requires { std::decay_t<A>::keep(st, z, j); })
    {
        int wz=orig(z), wj=orig(j);
        return wz>=0 && wj>=0 && std::decay_t<A>::keep(st, wz, wj);
    }
    constexpr bool
    keep(dim_t st, int z, int j) const requires (!(requires { std::decay_t<A>::keep(st, z, j); }))
    {
        int wz=orig(z), wj=orig(j);
        return wz>=0 && wj>=0 && a.keep(st, wz, wj);
    }
    constexpr static auto
    step(int k) requires (requires { std::decay_t<A>::step(k); })
    {
        int l=orig(k);
        return l>=0 ? std::decay_t<A>::step(l) : samestep<0, decltype(std::decay_t<A>::step(l))>;
    }
    constexpr auto
    step(int k) const requires (!(requires { std::decay_t<A>::step(k); }))
    {
        int l=orig(k);
        return l>=0 ? a.step(l) : samestep<0, decltype(a.step(l))>;
    }
    constexpr void adv(rank_t k, dim_t d) { if (int l=orig(k); l>=0) { a.adv(l, d); } }
    constexpr decltype(auto) at(auto const & j) const { return a.at(std::array<dim_t, sizeof...(i)> { j[di] ... }); }
    constexpr decltype(auto) operator*() const { return *a; }
    constexpr auto save() const { return a.save(); }
    constexpr void load(auto const & p) { a.load(p); }
    constexpr void mov(auto const & s) { a.mov(s); }
};

// Optimize nop case. TODO If A is View/Cell, etc. beat Dest on it.
template <Iterator A, class Dest>
constexpr decltype(auto)
reframe(A && a, Dest)
{
    if constexpr (std::is_same_v<Dest, mp::iota<mp::len<Dest>>>) {
        return RA_FW(a);
    } else {
        return Reframe<A, Dest> { RA_FW(a) };
    }
}

template <class cranks, class Op> struct Verb final { Op op; };
template <class A> concept is_verb = requires (A & a) { []<class cranks, class Op>(Verb<cranks, Op> &){}(a); };

template <class cranks, class Op> constexpr auto
wrank(cranks, Op && op) { return Verb<cranks, Op> { RA_FW(op) }; }

template <rank_t ... crank, class Op> constexpr auto
wrank(Op && op) { return Verb<ilist_t<crank ...>, Op> { RA_FW(op) }; }

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, int skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, int skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

// Get list (per argument) of lists of live axes. The last frame match is handled by standard prefix matching.
template <class ... crank, class W, class ... Ti, class ... Ri, int skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
// TODO crank negative, inf.
    constexpr static std::array<int, sizeof...(Ti)> live { (rank_s<Ti>() - mp::len<Ri> - crank::value) ... };
    using frameaxes = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri> - crank::value), skip>> ...>;
    using FM = Framematch<W, std::tuple<Ti ...>, frameaxes, skip + std::ranges::max(live)>;
    using R = typename FM::R;
    template <class U> constexpr static decltype(auto) op(U && v) { return FM::op(RA_FW(v).op); } // cf [ra31]
};

template <class V, class ... Ti, class ... Ri, int skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
// TODO -crank::value when the actual verb rank is used
    using R = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri>), skip>> ...>;
    template <class U> constexpr static decltype(auto) op(U && v) { return RA_FW(v); }
};


// --------------------
// Prefix match.
// --------------------

// finite before ANY before UNB, assumes neither is MIS.
constexpr dim_t common_len(dim_t a, dim_t b) { return a>=0 ? (a==b ? a : b>=0 ? MIS : a) : UNB==a ? b : UNB==b ? a : b; }
constexpr rank_t common_rank(rank_t a, rank_t b) { return ANY==a ? a : ANY==b ? b : a>=0 ? (b>=0 ? std::max(a, b) : a) : b; }

template <class T, class K=mp::iota<mp::len<T>>> struct Match;
template <class A> concept is_match = requires (A a) { []<class T>(Match<T> const &){}(a); };

// need runtime check if there's more than one leaf with ANY size.
template <class TOP, class A=TOP>
consteval int
tbc(int sofar)
{
    if constexpr (is_match<A>) {
        using T = decltype(std::declval<A>().t);
        [&sofar]<int ... i>(ilist_t<i ...>){
            (void)(((sofar = tbc<TOP, std::decay_t<mp::ref<T, i>>>(sofar)) >= 0) && ...);
        } (mp::iota<mp::len<T>> {});
        return sofar;
    } else if (int rt=rank_s<TOP>(), ra=rank_s<A>(); 0==rt || 0==ra) {
        return sofar;
    } else if (ANY==sofar || ANY==ra) {
        return 1+sofar;
    } else {
        for (int k=0; k<ra; ++k) {
            if (dim_t lt=TOP::len_s(k, true), la=A::len_s(k); MIS==lt) {
                return -1;
            } else if (UNB!=la && UNB!=lt && (ANY==la || ANY==lt)) {
                return 1+sofar;
            }
        }
        return sofar;
    }
}

template <class A, class U = ic_t<false>>
constexpr void
validate(A const & a, U allow_unb = {})
{
    static_assert(!has_len<A>, "Stray ra::len.");
    static_assert(allow_unb || ra::UNB!=ra::size_s<A>(), "Undefined size.");
    static_assert(0<=rank_s(a) || ANY==rank_s(a), "Undefined rank.");
    if constexpr (is_match<A>) {
        static_assert(0!=a.check_s(), "Bad shapes."); // FIXME c++26
        a.check(false);
    }
}

template <Iterator ... P, int ... I>
struct Match<std::tuple<P ...>, ilist_t<I ...>>
{
    std::tuple<P ...> t;
    constexpr static rank_t rs = []{ rank_t r=UNB; return ((r=common_rank(rank_s<P>(), r)), ...); }();
    constexpr Match(P ... p_): t(p_ ...) {} // [ra1]

// 0: fail, 1: rt check, 2: pass
    consteval static int
    check_s()
    {
        int sofar = tbc<Match>(0);
        return 0>sofar ? 0 : 2>sofar ? 2 : 1;
    }
    constexpr bool
    check(bool allow=true) const
    {
        constexpr int c = check_s();
        if constexpr (1==c) {
            for (int k=0; k<rank(); ++k) {
                if (len(k)<0) {
                    RA_CK(allow, "Bad shapes ", fmt(lstyle, ra::shape(get<I>(t))) ..., ".");
                    return false;
                }
            }
        }
        return !(0==c);
    }
    consteval static rank_t
    rank() requires (ANY!=rs)
    {
        return rs;
    }
    constexpr rank_t
    rank() const requires (ANY==rs)
    {
        rank_t r = UNB; return ((r = common_rank(ra::rank(get<I>(t)), r)), ...);
    }
    constexpr static dim_t
    len_s(int k, bool check=false)
    {
        auto f = [&k, &check]<class A>(dim_t s){
            if (constexpr rank_t r=rank_s<A>(); r<0 || k<r) {
                dim_t sk = [&]{ if constexpr (is_match<A>) return A::len_s(k, check); else return A::len_s(k); }();
                return (MIS==sk) ? MIS : check && (ANY==sk || ANY==s) ? ANY : common_len(sk, s);
            }
            return s;
        };
        dim_t s = UNB; (void)(((s = f.template operator()<std::decay_t<P>>(s)) != MIS) && ...);
        return s;
    }
    constexpr static dim_t
    len(int k) requires (requires { std::decay_t<P>::len(k); } && ...)
    {
        return len_s(k);
    }
    constexpr dim_t
    len(int k) const requires (!(requires { std::decay_t<P>::len(k); } && ...))
    {
        auto f = [&k](auto const & a, dim_t s){
            if (k<ra::rank(a)) {
                dim_t sk = a.len(k);
                return (MIS==sk) ? MIS : common_len(sk, s);
            }
            return s;
        };
        dim_t s = UNB; (void)((MIS!=(s = f(get<I>(t), s))) && ...); return s;
    }
    constexpr bool
    keep(dim_t st, int z, int j) const requires (!(requires { std::decay_t<P>::keep(st, z, j); }  && ...))
    {
        return (get<I>(t).keep(st, z, j) && ...);
    }
    constexpr static bool
    keep(dim_t st, int z, int j) requires (requires { std::decay_t<P>::keep(st, z, j); } && ...)
    {
        return (std::decay_t<P>::keep(st, z, j) && ...);
    }
// step/adv may call sub Iterators with k>= their rank, in that case they must return 0 to allow for frame matching.
    constexpr auto
    step(int k) const requires (!(requires { std::decay_t<P>::step(k); } && ...))
    {
        return std::make_tuple(get<I>(t).step(k) ...);
    }
    constexpr static auto
    step(int k) requires (requires { std::decay_t<P>::step(k); } && ...)
    {
        return std::make_tuple(std::decay_t<P>::step(k) ...);
    }
    constexpr void adv(rank_t k, dim_t d) { (get<I>(t).adv(k, d), ...); }
    constexpr auto save() const { return std::make_tuple(get<I>(t).save() ...); }
    constexpr void load(auto const & pp) { ((get<I>(t).load(get<I>(pp))), ...); }
    constexpr void mov(auto const & s) { ((get<I>(t).mov(get<I>(s))), ...); }
};

template <class ... P>
Match(P && ... p) -> Match<std::tuple<P ...>>;

constexpr bool
agree(auto const & ... p) { return Match(ra::iter(p) ...).check(); }

consteval int
agree_s(auto const & ... p) { return decltype(Match(ra::iter(p) ...))::check_s(); }

constexpr bool
agree_op(is_verb auto const & op, auto const & ... p) { return agree_verb(mp::iota<sizeof...(p)> {}, op, p ...); }

constexpr bool
agree_op(auto const & op, auto const & ... p) { return agree(p ...); }

template <class V, class ... T, int ... i>
constexpr bool
agree_verb(ilist_t<i ...>, V const & v, T const & ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return agree_op(FM::op(v), reframe(ra::iter(t), mp::ref<typename FM::R, i> {}) ...);
}

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Map;
template <class Op, Iterator ... P, int ... I>
struct Map<Op, std::tuple<P ...>, ilist_t<I ...>>: public Match<std::tuple<P ...>>
{
    using Match<std::tuple<P ...>>::t;
    Op op;
    constexpr Map(Op op_, P ... p_): Match<std::tuple<P ...>>(p_ ...), op(op_) {} // [ra1]
    RA_ASSIGNOPS_ITER(Map)
    constexpr decltype(auto) at(auto const & j) const { return std::invoke(op, get<I>(t).at(j) ...); }
    constexpr decltype(auto) operator*() const { return std::invoke(op, *get<I>(t) ...); }
    constexpr operator decltype(std::invoke(op, *get<I>(t) ...)) () const { return to_scalar(*this); }
};

template <class Op, class ... P>
Map(Op && op, P && ... p) -> Map<Op, std::tuple<P ...>>;

template <class Op, class ... P, int ... i>
constexpr auto
map_verb(ilist_t<i ...>, Op && op, P && ... p)
{
    using FM = Framematch<Op, std::tuple<P ...>>;
    return map_(FM::op(RA_FW(op)), reframe(RA_FW(p), mp::ref<typename FM::R, i> {}) ...);
}

constexpr auto
map_(is_verb auto && op, auto && ... p) { return map_verb(mp::iota<sizeof...(p)> {}, RA_FW(op), RA_FW(p) ...); }

constexpr auto
map_(auto && op, auto && ... p) { return Map(RA_FW(op), RA_FW(p) ...); }

constexpr auto
map(auto && op, auto && ... a) { return map_(RA_FW(op), ra::iter(RA_FW(a)) ...); }

template <class J> struct type_at { template <class P> using type = decltype(std::declval<P>().at(std::declval<J>())); };

template <std::size_t I=0, class T, class J>
constexpr mp::apply<std::common_reference_t, mp::map<type_at<J>::template type, mp::drop1<std::decay_t<T>>>>
pick_at(std::size_t p0, T && t, J const & j)
{
    if constexpr (constexpr std::size_t N = mp::len<std::decay_t<T>> - 1; I<N) {
        return (p0==I) ? get<I+1>(t).at(j) : pick_at<I+1>(p0, t, j);
    } else {
        RA_CK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class P> using type_star = decltype(*std::declval<P>());

template <std::size_t I=0, class T>
constexpr mp::apply<std::common_reference_t, mp::map<type_star, mp::drop1<std::decay_t<T>>>>
pick_star(std::size_t p0, T && t)
{
    if constexpr (constexpr std::size_t N = mp::len<std::decay_t<T>> - 1; I<N) {
        return (p0==I) ? *(get<I+1>(t)) : pick_star<I+1>(p0, t);
    } else {
        RA_CK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class T, class K=mp::iota<mp::len<T>>> struct Pick;
template <Iterator ... P, int ... I> requires (sizeof...(P)>1)
struct Pick<std::tuple<P ...>, ilist_t<I ...>>: public Match<std::tuple<P ...>>
{
    using Match<std::tuple<P ...>>::t;
    constexpr Pick(P ... p_): Match<std::tuple<P ...>>(p_ ...) {} // [ra1]
    RA_ASSIGNOPS_ITER(Pick)
    constexpr decltype(auto) at(auto const & j) const { return pick_at(get<0>(t).at(j), t, j); }
    constexpr decltype(auto) operator*() const { return pick_star(*get<0>(t), t); }
    constexpr operator decltype(pick_star(*get<0>(t), t)) () const { return to_scalar(*this); }
};

template <class ... P>
Pick(P && ... p) -> Pick<std::tuple<P ...>>;

constexpr auto
pick(auto && ... p) { return Pick { ra::iter(RA_FW(p)) ... }; }

} // namespace ra
