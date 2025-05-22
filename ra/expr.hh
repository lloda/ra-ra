// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression templates with prefix matching.

// (c) Daniel Llorens - 2011-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cassert>
#include <functional>
#include "base.hh"


// --------------------
// error handling. To customize see examples/throw.cc.
// --------------------

#if !defined(RA_DO_CHECK)
  #define RA_DO_CHECK 1 // tell users
#endif
#if RA_DO_CHECK==0
  #define RA_CHECK(...)
#else
  #ifdef RA_ASSERT
    #define RA_CHECK(...) RA_ASSERT(__VA_ARGS__)
  #elif RA_DO_CHECK==1
    #define RA_CHECK(cond, ...) { assert(cond); }
  #elif RA_DO_CHECK==2
    #include <iostream>
    #define RA_CHECK(cond, ...)                                         \
{                                                                       \
    if consteval {                                                      \
        assert(cond /* FIXME show args */);                             \
    } else {                                                            \
        if (!(cond)) [[unlikely]] {                                     \
            ra::print(std::cerr, "*** ra::", std::source_location::current(), " (" STRINGIZE(cond) ") " __VA_OPT__(,) __VA_ARGS__, " ***") << std::endl; \
            std::abort();                                               \
        }                                                               \
    }                                                                   \
}
  #else
    #error Bad value for RA_DO_CHECK
  #endif
#endif
#define RA_AFTER_CHECK Yes

namespace ra {

template <int ... I> using ilist_t = std::tuple<int_c<I> ...>;
template <int ... I> constexpr ilist_t<I ...> ilist {};
constexpr bool inside(dim_t i, dim_t b) { return 0<=i && i<b; }

template <class C, class T>
constexpr auto tuple2array = std::apply([](auto ... t) { return std::array<C, sizeof...(t)> { C(t) ... }; }, T {});


// --------------------
// assign ops for settable iterators. Might be different for e.g. Views.
// --------------------

// Forward to forbid misusing value y as ref [ra5].
#define RA_ASSIGNOPS_LINE(OP) \
    for_each([](auto && y, auto && x) { RA_FWD(y) OP x; }, *this, RA_FWD(x))
#define RA_ASSIGNOPS(OP) \
    constexpr void operator OP(auto && x) { RA_ASSIGNOPS_LINE(OP); }
// But see local ASSIGNOPS elsewhere.
#define RA_ASSIGNOPS_DEFAULT_SET \
    FOR_EACH(RA_ASSIGNOPS, =, *=, +=, -=, /=)
// Restate for expression classes since a template doesn't replace the copy assignment op.
#define RA_ASSIGNOPS_SELF(TYPE)                                         \
    TYPE & operator=(TYPE && x) { RA_ASSIGNOPS_LINE(=); return *this; } \
    TYPE & operator=(TYPE const & x) { RA_ASSIGNOPS_LINE(=); return *this; } \
    constexpr TYPE(TYPE && x) = default;                                \
    constexpr TYPE(TYPE const & x) = default;


// --------------------
// terminal types
// --------------------

constexpr struct Len
{
    consteval static rank_t rank() { return 0; }
    [[noreturn]] consteval static void len_outside_subscript_context() { std::abort(); }
    consteval static dim_t len_s(int k) { len_outside_subscript_context(); }
    consteval static dim_t len(int k) { len_outside_subscript_context(); }
    consteval static dim_t step(int k) { len_outside_subscript_context(); }
    consteval static void adv(rank_t k, dim_t d) { len_outside_subscript_context(); }
    consteval static bool keep(dim_t st, int z, int j) { len_outside_subscript_context(); }
    consteval dim_t operator*() const { len_outside_subscript_context(); }
    consteval static int save() { len_outside_subscript_context(); }
    consteval static void load(int) { len_outside_subscript_context(); }
    consteval static void mov(dim_t d) { len_outside_subscript_context(); }
} len;

template <> constexpr bool is_special_def<Len> = true;  // protect exprs with Len from reduction.
template <class E> struct WLen;                         // defined in ply.hh.
template <class E> concept has_len = requires(int ln, E && e) { WLen<std::decay_t<E>>::f(ln, RA_FWD(e)); };

template <class Ln, class E>
constexpr decltype(auto)
wlen(Ln ln, E && e)
{
    static_assert(std::is_integral_v<std::decay_t<Ln>> || is_constant<std::decay_t<Ln>>);
    if constexpr (has_len<E>) {
        return WLen<std::decay_t<E>>::f(ln, RA_FWD(e));
    } else {
        return RA_FWD(e);
    }
}

// Rank-0 Iterator. Can be used on foreign objects, or as alternative to the rank conjunction.
// We still want f(scalar(C)) to be f(C) and not map(f, C), this is controlled by tomap/toreduce.
template <class C>
struct Scalar final
{
    C c;
    RA_ASSIGNOPS_DEFAULT_SET
    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); } // FIXME consteval cf Match::check_s
    constexpr static dim_t len(int k) { std::abort(); } // FIXME idem
    constexpr static dim_t step(int k) { return 0; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static bool keep(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) at(auto && j) const { return c; }
    constexpr C & operator*() requires (std::is_lvalue_reference_v<C>) { return c; } // [ra37]
    constexpr C const & operator*() requires (!std::is_lvalue_reference_v<C>) { return c; }
    constexpr C const & operator*() const { return c; } // [ra39]
    consteval static int save() { return 0; }
    constexpr static void load(int) {}
    constexpr static void mov(dim_t d) {}
};

template <class C> constexpr auto
scalar(C && c) { return Scalar<C> { RA_FWD(c) }; }

template <class N> constexpr int
maybe_any = []{ if constexpr (is_constant<N>) { return N::value; } else { return ANY; } }();

// Iterator for foreign rank 1 objects.
template <std::bidirectional_iterator I, class N, class S>
struct Ptr final
{
    static_assert(is_constant<N> || 0==rank_s<N>());
    static_assert(is_constant<S> || 0==rank_s<S>());
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(nn==ANY || nn>=0 || nn==UNB);
    constexpr static bool constant = is_constant<N> && is_constant<S>;

    I i;
    [[no_unique_address]] N const n = {};
    [[no_unique_address]] S const s = {};

    constexpr Ptr(I i, N n, S s): i(i), n(n), s(s) {}
    RA_ASSIGNOPS_SELF(Ptr)
    RA_ASSIGNOPS_DEFAULT_SET
    consteval static rank_t rank() { return 1; }
    constexpr static dim_t len_s(int k) { return nn; } // len(k==0) or step(k>=0)
    constexpr static dim_t len(int k) requires (is_constant<N>) { return len_s(k); }
    constexpr dim_t len(int k) const requires (!is_constant<N>) { return n; }
    constexpr static dim_t step(int k) { return k==0 ? 1 : 0; }
    constexpr void adv(rank_t k, dim_t d) { mov(step(k) * d); }
    constexpr static bool keep(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr decltype(auto) at(auto && j) const requires (std::random_access_iterator<I>)
    {
        RA_CHECK(UNB==nn || inside(j[0], n), "Bad index ", j[0], " for len[0]=", n, ".");
        return i[j[0]*s];
    }
    constexpr decltype(auto) operator*() const { return *i; }
    constexpr auto save() const { return i; }
    constexpr void load(I ii) { i = ii; }
    constexpr void mov(dim_t d)
    {
        if constexpr (std::random_access_iterator<I>) {
            i += d*s;
        } else {
            if (dim_t j=d*s; j>0) while (j>0) { ++i; --j; } else while (j<0) { --i; ++j; }
        }
    }
};

template <class X> using sarg = std::conditional_t<is_constant<std::decay_t<X>> || is_scalar<X>, std::decay_t<X>, X>;

template <class S>
consteval auto maybe_step()
{
    if constexpr (std::is_integral_v<S>) {
        return S(1);
    } else {
        static_assert(is_constant<S> && 1==S::value);
        return S {};
    }
}

template <class I, class N=dim_c<UNB>, class S=dim_c<1>>
constexpr auto
ptr(I && i, N && n = N {}, S && s = maybe_step<S>())
{
    if constexpr (std::ranges::bidirectional_range<std::remove_reference_t<I>>) {
        static_assert(std::is_same_v<dim_c<UNB>, N>, "Object has own length.");
        static_assert(std::is_same_v<dim_c<1>, S>, "No step with deduced size.");
        if constexpr (ANY==size_s(i)) {
            return ptr(std::begin(RA_FWD(i)), std::ssize(i), RA_FWD(s));
        } else {
            return ptr(std::begin(RA_FWD(i)), ic<size_s(i)>, RA_FWD(s));
        }
    } else if constexpr (std::bidirectional_iterator<std::decay_t<I>>) {
        if constexpr (std::is_integral_v<N>) {
            RA_CHECK(n>=0, "Bad ptr length ", n, ".");
        }
        return Ptr<std::decay_t<I>, sarg<N>, sarg<S>> { i, RA_FWD(n), RA_FWD(s) };
    } else {
        static_assert(false, "Bad type for ptr().");
    }
}

// Sequence and its Iterator. Iota parameters can be expressions, but they must all have rank 0.
// FIXME w is a custom Reframe mechanism. Generalize/unify.
// FIXME Can't represent a ct origin bc Iterator interface takes up i. Sequence should be its own type.
template <int w, class I, class N, class S>
struct Iota final
{
    static_assert(w>=0);
    static_assert(is_constant<S> || 0==rank_s<S>());
    static_assert(is_constant<N> || 0==rank_s<N>());
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(nn==ANY || nn>=0 || nn==UNB);
    constexpr static bool constant = is_constant<N> && is_constant<S>;

    I i = {};
    [[no_unique_address]] N const n = {};
    [[no_unique_address]] S const s = {};

    constexpr static S gets() requires (is_constant<S>) { return S {}; }
    constexpr I gets() const requires (!is_constant<S>) { return s; }

    consteval static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { return k==w ? nn : UNB; } // len(0<=k<=w) or step(0<=k)
    constexpr static dim_t len(int k) requires (is_constant<N>) { return len_s(k); }
    constexpr dim_t len(int k) const requires (!is_constant<N>) { return k==w ? n : UNB; }
    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr void adv(rank_t k, dim_t d) { i += I(step(k) * d) * I(s); }
    constexpr static bool keep(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr auto at(auto && j) const
    {
        RA_CHECK(UNB==nn || inside(j[0], n), "Bad index ", j[0], " for len[0]=", n, ".");
        return i + I(j[w])*I(s);
    }
    constexpr I operator*() const { return i; }
    constexpr I save() const { return i; }
    constexpr void load(I ii) { i = ii; }
    constexpr void mov(dim_t d) { i += I(d)*I(s); }
};

template <int w=0, class I=dim_t, class N=dim_c<UNB>, class S=dim_c<1>>
constexpr auto
iota(N && n = N {}, I && i = 0, S && s = maybe_step<S>())
{
    if constexpr (std::is_integral_v<N>) {
        RA_CHECK(n>=0, "Bad iota length ", n, ".");
    }
    return Iota<w, sarg<I>, sarg<N>, sarg<S>> { RA_FWD(i), RA_FWD(n), RA_FWD(s) };
}

#define DEF_TENSORINDEX(w) constexpr auto JOIN(_, w) = iota<w>();
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

template <class A> concept is_iota = requires (A a)
{
    []<class I, class N, class S>(Iota<0, I, N, S> const &){}(a);
    requires UNB!=a.nn; // exclude UNB from beating to let B=A(... i ...) use B's len. FIXME
};

constexpr bool
inside(is_iota auto const & i, dim_t l)
{
    return (inside(i.i, l) && inside(i.i+(i.n-1)*i.s, l)) || (0==i.n /* don't bother */);
}

template <int w, class I, class N, class S, class K=dim_c<0>>
constexpr auto
reverse(Iota<w, I, N, S> const & i, K k = {})
{
    static_assert(i.nn!=UNB && k>=0 && k<=w, "Bad arguments to reverse(iota).");
    if constexpr (k==w) {
        return ra::iota<w>(i.n, i.i+(i.n-1)*i.s,
                           [&i]() { if constexpr (is_constant<S>) return dim_c<-S {}> {}; else return -i.s; }());
    } else {
        return i;
    }
}


// --------------
// making Iterators
// --------------

// TODO arbitrary exprs? runtime cr? ra::len in cr?
template <int cr>
constexpr auto
iter(Slice auto && a) { return RA_FWD(a).template iter<cr>(); }

constexpr void
start(auto && t) { static_assert(false, "Cannot start() type."); }

constexpr auto
start(is_fov auto && t) { return ra::ptr(RA_FWD(t)); }

template <class T>
constexpr auto
start(std::initializer_list<T> v) { return ra::ptr(v.begin(), v.size()); }

constexpr auto
start(is_scalar auto && t) { return ra::scalar(RA_FWD(t)); }

// forward declare for Match; implemented in small.hh.
constexpr auto
start(is_builtin_array auto && t);

// neither CellBig nor CellSmall will retain rvalues [ra4].
constexpr auto
start(Slice auto && t) { return iter<0>(RA_FWD(t)); }

// iterators need to be reset on each use [ra35].
template <class A> requires (is_iterator<A> && !(requires (A a) { []<class C>(Scalar<C> const &){}(a); }))
constexpr auto
start(A & a) { return a; }

constexpr decltype(auto)
start(is_iterator auto && a) { return RA_FWD(a); }


// --------------------
// prefix match
// --------------------

constexpr rank_t
choose_rank(rank_t a, rank_t b) { return ANY==a ? a : ANY==b ? b : a>=0 ? (b>=0 ? std::max(a, b) : a) : b; }

// finite before ANY before UNB, assumes checks pass.
constexpr dim_t
choose_len(dim_t a, dim_t b) { return a>=0 ? (a==b ? a : b>=0 ? MIS : a) : UNB==a ? b : UNB==b ? a : b; }

template <class T, class K=mp::iota<mp::len<T>>> struct Match;
template <class A> concept is_match = requires (A a) { []<class T>(Match<T> const &){}(a); };

// need runtime check if there's more than one leaf with ANY size.
template <class TOP, class A=TOP>
consteval int
tbc(int sofar)
{
    if constexpr (is_match<A>) {
        using T = decltype(std::declval<A>().t);
        [&sofar]<int ... i>(ilist_t<i ...>) {
            (void)(((sofar = tbc<TOP, std::decay_t<mp::ref<T, i>>>(sofar)) >= 0) && ...);
        } (mp::iota<mp::len<T>> {});
        return sofar;
    } else {
        if (int rt=rank_s<TOP>(), ra=rank_s<A>(); 0==rt || 0==ra) {
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
}

template <class A, class U = bool_c<false>>
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
    constexpr static rank_t rs = [] { rank_t r=UNB; return ((r=choose_rank(rank_s<P>(), r)), ...); }();

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
                    RA_CHECK(allow, "Bad shapes ", fmt(lstyle, shape(get<I>(t))) ..., ".");
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
        rank_t r = UNB; ((r = choose_rank(ra::rank(get<I>(t)), r)), ...); assert(ANY!=r); // not at rt
        return r;
    }
    constexpr static dim_t
    len_s(int k, bool check=false)
    {
        auto f = [&k, &check]<class A>(dim_t s) {
            if (constexpr rank_t r=rank_s<A>(); r<0 || k<r) {
                dim_t sk = [&]{ if constexpr (is_match<A>) return A::len_s(k, check); else return A::len_s(k); }();
                return (MIS==sk) ? MIS : check && (ANY==sk || ANY==s) ? ANY : choose_len(sk, s);
            }
            return s;
        };
        dim_t s = UNB; (void)(((s = f.template operator()<std::decay_t<P>>(s)) != MIS) && ...);
        return s;
    }
    constexpr static dim_t
    len(int k) requires (requires { P::len(k); } && ...)
    {
        return len_s(k);
    }
    constexpr dim_t
    len(int k) const requires (!(requires { P::len(k); } && ...))
    {
        auto f = [&k](auto const & a, dim_t s) {
            if (k<ra::rank(a)) {
                dim_t sk = a.len(k);
                return (MIS==sk) ? MIS : choose_len(sk, s);
            }
            return s;
        };
        dim_t s = UNB; (void)(((s = f(get<I>(t), s)) != MIS) && ...); assert(ANY!=s); // not at rt
        return s;
    }
    constexpr bool
    keep(dim_t st, int z, int j) const requires (!(requires { P::keep(st, z, j); }  && ...))
    {
        return (get<I>(t).keep(st, z, j) && ...);
    }
    constexpr static bool
    keep(dim_t st, int z, int j) requires (requires { P::keep(st, z, j); } && ...)
    {
        return (std::decay_t<P>::keep(st, z, j) && ...);
    }
// could preserve static, but ply doesn't use it atm.
    constexpr auto step(int i) const { return std::make_tuple(get<I>(t).step(i) ...); }
    constexpr void adv(rank_t k, dim_t d) { (get<I>(t).adv(k, d), ...); }
    constexpr auto save() const { return std::make_tuple(get<I>(t).save() ...); }
    constexpr void load(auto const & pp) { ((get<I>(t).load(get<I>(pp))), ...); }
    constexpr void mov(auto const & s) { ((get<I>(t).mov(get<I>(s))), ...); }
};


// ---------------------------
// reframe
// ---------------------------

template <dim_t N, class T> constexpr T samestep = N;
template <dim_t N, class ... T> constexpr std::tuple<T ...> samestep<N, std::tuple<T ...>> = { samestep<N, T> ... };

// Transpose for Iterators. As in transpose(), one names the destination axis for each original axis.
// However, axes may not be repeated. Used in the rank conjunction below.
// Dest is a list of destination axes [l0 l1 ... li ... l(rank(A)-1)].
// The dimensions of the reframed A are numbered as [0 ... k ... max(l)-1].
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// If not, then axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO invalid for ANY, since Dest is compile time. [ra7]

template <class Dest, Iterator A>
struct Reframe
{
    A a;
    constexpr static auto dest = tuple2array<int, Dest>;

    consteval static rank_t
    rank()
    {
        return dest.size()==0 ? 0 : 1 + std::ranges::max(dest);
    }
    constexpr static int orig(int k)
    {
        return [&k]<int ... i>(ilist_t<i ...>) { int r=-1; (void)((k==dest[i] && (r=i, 1)) || ...); return r; }
        (mp::iota<dest.size()> {});
    }
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
    constexpr decltype(auto)
    at(auto const & i) const
    {
        return a.at(std::apply([&i](auto ... t) { return std::array<dim_t, sizeof...(t)> { i[t] ... }; }, Dest {}));
    }
    constexpr auto step(int k) const { int l=orig(k); return l>=0 ? a.step(l) : samestep<0, decltype(a.step(l))>; }
    constexpr void adv(rank_t k, dim_t d) { if (int l=orig(k); l>=0) { a.adv(l, d); } }
    constexpr decltype(auto) operator*() const { return *a; }
    constexpr auto save() const { return a.save(); }
    constexpr void load(auto const & p) { a.load(p); }
// FIXME only if Dest preserves axis order (?) which is how wrank works
    constexpr void mov(auto const & s) { a.mov(s); }
};

// Optimize no-op case. TODO If A is CellBig, etc. beat Dest on it, same for eventual transpose_expr<>.

template <class Dest, class A>
constexpr decltype(auto)
reframe(A && a)
{
    if constexpr (std::is_same_v<Dest, mp::iota<Reframe<Dest, A>::rank()>>) {
        return RA_FWD(a);
    } else {
        return Reframe<Dest, A> { RA_FWD(a) };
    }
}


// ---------------------------
// verbs and rank conjunction
// ---------------------------

template <class cranks, class Op> struct Verb final { Op op; };
template <class A> concept is_verb = requires (A a) { []<class cranks, class Op>(Verb<cranks, Op> const &){}(a); };

template <class cranks, class Op>
constexpr auto
wrank(cranks, Op && op) { return Verb<cranks, Op> { RA_FWD(op) }; }

template <rank_t ... crank, class Op>
constexpr auto
wrank(Op && op) { return Verb<ilist_t<crank ...>, Op> { RA_FWD(op) }; }

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

// Get a list (per argument) of lists of live axes. The last frame match is handled by standard prefix matching.
template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
// live = number of live axes on this frame, for each argument. // TODO crank negative, inf.
    using live = ilist_t<(rank_s<Ti>() - mp::len<Ri> - crank::value) ...>;
    using frameaxes = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri> - crank::value), skip>> ...>;
    using FM = Framematch<W, std::tuple<Ti ...>, frameaxes, skip + std::ranges::max(tuple2array<int, live>)>;
    using R = typename FM::R;
    template <class U> constexpr static decltype(auto) op(U && v) { return FM::op(RA_FWD(v).op); } // cf [ra31]
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
// TODO -crank::value when the actual verb rank is used (eg to use CellBig<... that_rank> instead of just begin()).
    using R = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri>), skip>> ...>;
    template <class U> constexpr static decltype(auto) op(U && v) { return RA_FWD(v); }
};


// ---------------
// explicit agreement checks
// ---------------

template <class ... P>
Match(P && ... p) -> Match<std::tuple<P ...>>;

constexpr bool
agree(auto const & ... p) { return Match(ra::start(p) ...).check(); }

consteval int
agree_s(auto const & ... p) { return decltype(Match(ra::start(p) ...))::check_s(); }

constexpr bool
agree_op(is_verb auto const & op, auto const & ... p) { return agree_verb(mp::iota<sizeof...(p)> {}, op, p ...); }

constexpr bool
agree_op(auto const & op, auto const & ... p) { return agree(p ...); }

template <class V, class ... T, int ... i>
constexpr bool
agree_verb(ilist_t<i ...>, V const & v, T const & ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return agree_op(FM::op(v), reframe<mp::ref<typename FM::R, i>>(ra::start(t)) ...);
}


// ---------------------------
// operator expression
// ---------------------------

template <class E>
constexpr decltype(auto) to_scalar(E && e)
{
    if constexpr (constexpr dim_t s=size_s(e); 1!=s) {
        static_assert(ANY==s, "Bad scalar conversion.");
        RA_CHECK(1==size(e), "Bad scalar conversion from shape [", fmt(nstyle, ra::shape(e)), "].");
    }
    return *e;
}

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Map;
template <class Op, Iterator ... P, int ... I>
struct Map<Op, std::tuple<P ...>, ilist_t<I ...>>: public Match<std::tuple<P ...>>
{
    using Match<std::tuple<P ...>>::t;
    Op op;
    constexpr Map(Op op_, P ... p_): Match<std::tuple<P ...>>(p_ ...), op(op_) {} // [ra1]
    RA_ASSIGNOPS_SELF(Map)
    RA_ASSIGNOPS_DEFAULT_SET
    constexpr decltype(auto) at(auto const & j) const { return std::invoke(op, get<I>(t).at(j) ...); }
    constexpr decltype(auto) operator*() const { return std::invoke(op, *get<I>(t) ...); }
    constexpr operator decltype(std::invoke(op, *get<I>(t) ...)) () const { return to_scalar(*this); }
};

template <class Op, Iterator ... P>
constexpr bool is_special_def<Map<Op, std::tuple<P ...>>> = (is_special<P> || ...);

template <class Op, class ... P>
Map(Op && op, P && ... p) -> Map<Op, std::tuple<P ...>>;

template <class Op, class ... P, int ... i>
constexpr auto
map_verb(ilist_t<i ...>, Op && op, P && ... p)
{
    using FM = Framematch<Op, std::tuple<P ...>>;
    return map_(FM::op(RA_FWD(op)), reframe<mp::ref<typename FM::R, i>>(RA_FWD(p)) ...);
}

constexpr auto
map_(is_verb auto && op, auto && ... p) { return map_verb(mp::iota<sizeof...(p)> {}, RA_FWD(op), RA_FWD(p) ...); }

constexpr auto
map_(auto && op, auto && ... p) { return Map(RA_FWD(op), RA_FWD(p) ...); }

constexpr auto
map(auto && op, auto && ... a) { return map_(RA_FWD(op), start(RA_FWD(a)) ...); }


// ---------------------------
// pick expression
// ---------------------------

template <class J> struct type_at { template <class P> using type = decltype(std::declval<P>().at(std::declval<J>())); };

template <std::size_t I, class T, class J>
constexpr mp::apply<std::common_reference_t, mp::map<type_at<J>::template type, mp::drop1<std::decay_t<T>>>>
pick_at(std::size_t p0, T && t, J const & j)
{
    if constexpr (constexpr std::size_t N = mp::len<std::decay_t<T>> - 1; I<N) {
        return (p0==I) ? get<I+1>(t).at(j) : pick_at<I+1>(p0, t, j);
    } else {
        RA_CHECK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class P> using type_star = decltype(*std::declval<P>());

template <std::size_t I, class T>
constexpr mp::apply<std::common_reference_t, mp::map<type_star, mp::drop1<std::decay_t<T>>>>
pick_star(std::size_t p0, T && t)
{
    if constexpr (constexpr std::size_t N = mp::len<std::decay_t<T>> - 1; I<N) {
        return (p0==I) ? *(get<I+1>(t)) : pick_star<I+1>(p0, t);
    } else {
        RA_CHECK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class T, class K=mp::iota<mp::len<T>>> struct Pick;
template <Iterator ... P, int ... I>
struct Pick<std::tuple<P ...>, ilist_t<I ...>>: public Match<std::tuple<P ...>>
{
    using Match<std::tuple<P ...>>::t;
    static_assert(sizeof...(P)>1);
    constexpr Pick(P ... p_): Match<std::tuple<P ...>>(p_ ...) {} // [ra1]
    RA_ASSIGNOPS_SELF(Pick)
    RA_ASSIGNOPS_DEFAULT_SET
    constexpr decltype(auto) at(auto const & j) const { return pick_at<0>(get<0>(t).at(j), t, j); }
    constexpr decltype(auto) operator*() const { return pick_star<0>(*get<0>(t), t); }
    constexpr operator decltype(pick_star<0>(*get<0>(t), t)) () const { return to_scalar(*this); }
};

template <Iterator ... P>
constexpr bool is_special_def<Pick<std::tuple<P ...>>> = (is_special<P> || ...);

template <class ... P>
Pick(P && ... p) -> Pick<std::tuple<P ...>>;

constexpr auto
pick(auto && ... p) { return Pick { start(RA_FWD(p)) ... }; }

} // namespace ra
