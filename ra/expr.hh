// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Expression templates with prefix matching.

// (c) Daniel Llorens - 2011-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include <cassert>
#include <functional>
#include "base.hh"


// --------------------
// error handling. See examples/throw.cc for how to customize.
// --------------------

#if !defined(RA_DO_CHECK)
  #define RA_DO_CHECK 1 // tell users
#endif
#if RA_DO_CHECK==0
  #define RA_CHECK(...) // good luck
#else
  #ifdef RA_ASSERT
    #define RA_CHECK(...) RA_ASSERT(__VA_ARGS__)
  #elif RA_DO_CHECK==1
    #define RA_CHECK(cond, ...) { assert(cond); }
  #elif RA_DO_CHECK==2
    #include <iostream>
    #define RA_CHECK(cond, ...)                                         \
{                                                                       \
    if (std::is_constant_evaluated()) {                                 \
        assert(cond /* FIXME show args */);                             \
    } else {                                                            \
        if (!(cond)) [[unlikely]] {                                     \
            std::cerr << ra::format("*** ra::", std::source_location::current(), " (" STRINGIZE(cond) ") " __VA_OPT__(,) __VA_ARGS__, " ***") << std::endl; \
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

constexpr bool inside(dim_t i, dim_t b) { return 0<=i && i<b; }


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

// Rank-0 IteratorConcept. Can be used on foreign objects, or as alternative to the rank conjunction.
// We still want f(scalar(C)) to be f(C) and not map(f, C), this is controlled by tomap/toreduce.
template <class C>
struct Scalar
{
    C c;
    RA_ASSIGNOPS_DEFAULT_SET
    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static dim_t step(int k) { return 0; }
    constexpr static void adv(rank_t k, dim_t d) {}
    constexpr static bool keep_step(dim_t st, int z, int j) { return true; }
    constexpr decltype(auto) at(auto && j) const { return c; }
    constexpr C & operator*() requires (std::is_lvalue_reference_v<C>) { return c; } // [ra37]
    constexpr C const & operator*() requires (!std::is_lvalue_reference_v<C>) { return c; }
    constexpr C const & operator*() const { return c; } // [ra39]
    constexpr static int save() { return 0; }
    constexpr static void load(int) {}
    constexpr static void mov(dim_t d) {}
};

template <class C> constexpr auto
scalar(C && c) { return Scalar<C> { RA_FWD(c) }; }

template <class N> constexpr int
maybe_any = []{
    if constexpr (is_constant<N>) {
        return N::value;
    } else {
        static_assert(std::is_integral_v<N> || !std::is_same_v<N, bool>);
        return ANY;
    }
}();

// IteratorConcept for foreign rank 1 objects.
template <std::bidirectional_iterator I, class N, class S>
struct Ptr
{
    static_assert(is_constant<N> || 0==rank_s<N>());
    static_assert(is_constant<S> || 0==rank_s<S>());
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(nn==ANY || nn>=0 || nn==BAD);
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
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr decltype(auto) at(auto && j) const requires (std::random_access_iterator<I>)
    {
        RA_CHECK(BAD==nn || inside(j[0], n), "Bad index ", j[0], " for len[0]=", n, ".");
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

template <class X> using seq_arg = std::conditional_t<is_constant<std::decay_t<X>> || is_scalar<std::decay_t<X>>, std::decay_t<X>, X>;

template <class S>
constexpr auto
thestep()
{
    if constexpr (std::is_integral_v<S>) {
        return S(1);
    } else if constexpr (is_constant<S>) {
        static_assert(1==S::value);
        return S {};
    } else {
        static_assert(always_false<S>, "Bad step type for sequence.");
    }
}

template <class I, class N=dim_c<BAD>, class S=dim_c<1>>
constexpr auto
ptr(I && i, N && n = N {}, S && s = thestep<S>())
{
    if constexpr (std::ranges::bidirectional_range<std::remove_reference_t<I>>) {
        static_assert(std::is_same_v<dim_c<BAD>, N>, "Object has own length.");
        static_assert(std::is_same_v<dim_c<1>, S>, "No step with deduced size.");
        if constexpr (ANY==size_s<I>()) {
            return ptr(std::begin(RA_FWD(i)), std::ssize(i), RA_FWD(s));
        } else {
            return ptr(std::begin(RA_FWD(i)), ic<size_s<I>()>, RA_FWD(s));
        }
    } else if constexpr (std::bidirectional_iterator<std::decay_t<I>>) {
        if constexpr (std::is_integral_v<N>) {
            RA_CHECK(n>=0, "Bad ptr length ", n, ".");
        }
        return Ptr<std::decay_t<I>, seq_arg<N>, seq_arg<S>> { i, RA_FWD(n), RA_FWD(s) };
    } else {
        static_assert(always_false<I>, "Bad type for ptr().");
    }
}

// Sequence and IteratorConcept for same. Iota isn't really a terminal, but its exprs must all have rank 0.
// FIXME w is a custom Reframe mechanism inherited from TensorIndex. Generalize/unify
// FIXME Sequence should be its own type, we can't represent a ct origin bc IteratorConcept interface takes up i.
template <int w, class I, class N, class S>
struct Iota
{
    static_assert(w>=0);
    static_assert(is_constant<S> || 0==rank_s<S>());
    static_assert(is_constant<N> || 0==rank_s<N>());
    constexpr static dim_t nn = maybe_any<N>;
    static_assert(nn==ANY || nn>=0 || nn==BAD);
    constexpr static bool constant = is_constant<N> && is_constant<S>;

    I i = {};
    [[no_unique_address]] N const n = {};
    [[no_unique_address]] S const s = {};

    constexpr static S gets() requires (is_constant<S>) { return S {}; }
    constexpr I gets() const requires (!is_constant<S>) { return s; }

    consteval static rank_t rank() { return w+1; }
    constexpr static dim_t len_s(int k) { return k==w ? nn : BAD; } // len(0<=k<=w) or step(0<=k)
    constexpr static dim_t len(int k) requires (is_constant<N>) { return len_s(k); }
    constexpr dim_t len(int k) const requires (!is_constant<N>) { return k==w ? n : BAD; }
    constexpr static dim_t step(rank_t k) { return k==w ? 1 : 0; }
    constexpr void adv(rank_t k, dim_t d) { i += I(step(k) * d) * I(s); }
    constexpr static bool keep_step(dim_t st, int z, int j) { return st*step(z)==step(j); }
    constexpr auto at(auto && j) const
    {
        RA_CHECK(BAD==nn || inside(j[0], n), "Bad index ", j[0], " for len[0]=", n, ".");
        return i + I(j[w])*I(s);
    }
    constexpr I operator*() const { return i; }
    constexpr I save() const { return i; }
    constexpr void load(I ii) { i = ii; }
    constexpr void mov(dim_t d) { i += I(d)*I(s); }
};

template <int w=0, class I=dim_t, class N=dim_c<BAD>, class S=dim_c<1>>
constexpr auto
iota(N && n = N {}, I && i = 0, S && s = thestep<S>())
{
    if constexpr (std::is_integral_v<N>) {
        RA_CHECK(n>=0, "Bad iota length ", n, ".");
    }
    return Iota<w, seq_arg<I>, seq_arg<N>, seq_arg<S>> { RA_FWD(i), RA_FWD(n), RA_FWD(s) };
}

#define DEF_TENSORINDEX(w) constexpr auto JOIN(_, w) = iota<w>();
FOR_EACH(DEF_TENSORINDEX, 0, 1, 2, 3, 4);
#undef DEF_TENSORINDEX

RA_IS_DEF(is_iota, false)
// BAD is excluded from beating to allow B = A(... i ...) to use B's len. FIXME find a way?
template <class I, class N, class S>
constexpr bool is_iota_def<Iota<0, I, N, S>> = (BAD != Iota<0, I, N, S>::nn);

constexpr bool
inside(is_iota auto const & i, dim_t l)
{
    return (inside(i.i, l) && inside(i.i+(i.n-1)*i.s, l)) || (0==i.n /* don't bother */);
}

constexpr struct Len
{
    consteval static rank_t rank() { return 0; }
    constexpr static dim_t len_s(int k) { std::abort(); }
    constexpr static dim_t len(int k) { std::abort(); }
    constexpr static dim_t step(int k) { std::abort(); }
    constexpr static void adv(rank_t k, dim_t d) { std::abort(); }
    constexpr static bool keep_step(dim_t st, int z, int j) { std::abort(); }
    constexpr dim_t operator*() const { std::abort(); }
    constexpr static int save() { std::abort(); }
    constexpr static void load(int) { std::abort(); }
    constexpr static void mov(dim_t d) { std::abort(); }
} len;

// protect exprs with Len from reduction.
template <> constexpr bool is_special_def<Len> = true;
RA_IS_DEF(has_len, false);


// --------------
// making Iterators
// --------------

// TODO arbitrary exprs? runtime cr? ra::len in cr?
template <int cr>
constexpr auto
iter(SliceConcept auto && a) { return RA_FWD(a).template iter<cr>(); }

constexpr void
start(auto && t) { static_assert(always_false<decltype(t)>, "Cannot start() type."); }

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
start(SliceConcept auto && t) { return iter<0>(RA_FWD(t)); }

RA_IS_DEF(is_ra_scalar, (std::same_as<A, Scalar<decltype(std::declval<A>().c)>>))

// iterators need to be start()ed on each use [ra35].
template <class T> requires (is_iterator<T> && !is_ra_scalar<T>)
constexpr auto
start(T & t) { return t; }

// FIXME const Iterator would still be unusable after start().
constexpr decltype(auto)
start(is_iterator auto && t) { return RA_FWD(t); }


// --------------------
// prefix match
// --------------------

constexpr rank_t
choose_rank(rank_t ra, rank_t rb) { return BAD==rb ? ra : BAD==ra ? rb : ANY==ra ? ra : ANY==rb ? rb : std::max(ra, rb); }

// pick first if mismatch (see below). FIXME maybe return invalid.
constexpr dim_t
choose_len(dim_t sa, dim_t sb) { return BAD==sa ? sb : BAD==sb ? sa : ANY==sa ? sb : sa; }

template <bool checkp, class T, class K=mp::iota<mp::len<T>>> struct Match;
template <bool checkp, IteratorConcept ... P, int ... I>
struct Match<checkp, std::tuple<P ...>, mp::int_list<I ...>>
{
    std::tuple<P ...> t;
    constexpr static rank_t rs = [] { rank_t r=BAD; return ((r=choose_rank(r, ra::rank_s<P>())), ...); }();

// 0: fail, 1: rt, 2: pass
    consteval static int
    check_s()
    {
        if constexpr (sizeof...(P)<2) {
            return 2;
        } else if constexpr (ANY==rs) {
            return 1; // FIXME can be tightened to 2 if all args are rank 0 save one
        } else {
            bool tbc = false;
            for (int k=0; k<rs; ++k) {
                dim_t ls = len_s(k);
                if (((k<ra::rank_s<P>() && ls!=choose_len(std::decay_t<P>::len_s(k), ls)) || ...)) {
                    return 0;
                }
                int anyk = ((k<ra::rank_s<P>() && (ANY==std::decay_t<P>::len_s(k))) + ...);
                int fixk = ((k<ra::rank_s<P>() && (0<=std::decay_t<P>::len_s(k))) + ...);
                tbc = tbc || (anyk>0 && anyk+fixk>1);
            }
            return tbc ? 1 : 2;
        }
    }
    constexpr bool
    check() const
    {
        if constexpr (constexpr int c = check_s(); 2==c) {
            return true;
        } else if constexpr (0==c) {
            return false;
        } else if constexpr (1==c) {
            for (int k=0; k<rank(); ++k) {
#pragma GCC diagnostic push // gcc 12.2 and 13.2 with RA_DO_CHECK=0 and -fno-sanitize=all.
#pragma GCC diagnostic warning "-Warray-bounds"
                dim_t ls = len(k);
#pragma GCC diagnostic pop
                if (((k<ra::rank(std::get<I>(t)) && ls!=choose_len(std::get<I>(t).len(k), ls)) || ...)) {
                    return false;
                }
            }
        }
        return true;
    }

    constexpr
    Match(P ... p_): t(p_ ...) // [ra1]
    {
// TODO Maybe on ply would make checkp, agree_xxx() unnecessary.
        if constexpr (checkp && !(has_len<P> || ...)) {
            constexpr int c = check_s();
            static_assert(0!=c, "Mismatched shapes."); // FIXME c++26
            if constexpr (1==c) {
                RA_CHECK(check(), "Mismatched shapes", format_array(ra::shape(p_), {.shape=noshape, .open=" [", .close="]"}) ..., ".");
            }
        }
    }

    consteval static rank_t
    rank() requires (ANY!=rs)
    {
        return rs;
    }
    constexpr rank_t
    rank() const requires (ANY==rs)
    {
        rank_t r = BAD;
        ((r = choose_rank(r, ra::rank(std::get<I>(t)))), ...);
        assert(ANY!=r); // not at runtime
        return r;
    }
// first nonnegative size, if none first ANY, if none then BAD
    constexpr static dim_t
    len_s(int k)
    {
        auto f = [&k]<class A>(dim_t s) {
            constexpr rank_t ar = ra::rank_s<A>();
            return (ar<0 || k<ar) ? choose_len(s, A::len_s(k)) : s;
        };
        dim_t s = BAD; ((s>=0 ? s : s = f.template operator()<std::decay_t<P>>(s)), ...);
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
        auto f = [&k](dim_t s, auto const & a) {
            return k<ra::rank(a) ? choose_len(s, a.len(k)) : s;
        };
        dim_t s = BAD; ((s>=0 ? s : s = f(s, std::get<I>(t))), ...);
        assert(ANY!=s); // not at runtime
        return s;
    }
// could preserve static, but ply doesn't use it atm.
    constexpr auto
    step(int i) const
    {
        return std::make_tuple(std::get<I>(t).step(i) ...);
    }
    constexpr void
    adv(rank_t k, dim_t d)
    {
        (std::get<I>(t).adv(k, d), ...);
    }
    constexpr bool
    keep_step(dim_t st, int z, int j) const requires (!(requires { P::keep_step(st, z, j); }  && ...))
    {
        return (std::get<I>(t).keep_step(st, z, j) && ...);
    }
    constexpr static bool
    keep_step(dim_t st, int z, int j) requires (requires { P::keep_step(st, z, j); } && ...)
    {
        return (std::decay_t<P>::keep_step(st, z, j) && ...);
    }
    constexpr auto save() const { return std::make_tuple(std::get<I>(t).save() ...); }
    constexpr void load(auto const & pp) { ((std::get<I>(t).load(std::get<I>(pp))), ...); }
    constexpr void mov(auto const & s) { ((std::get<I>(t).mov(std::get<I>(s))), ...); }
};


// ---------------------------
// reframe
// ---------------------------

template <dim_t N, class T> constexpr T samestep = N;
template <dim_t N, class ... T> constexpr std::tuple<T ...> samestep<N, std::tuple<T ...>> = { samestep<N, T> ... };

// Transpose variant for IteratorConcepts. As in transpose(), one names the destination axis for
// each original axis. However, axes may not be repeated. Used in the rank conjunction below.
// Dest is a list of destination axes [l0 l1 ... li ... l(rank(A)-1)].
// The dimensions of the reframed A are numbered as [0 ... k ... max(l)-1].
// If li = k for some i, then axis k of the reframed A moves on axis i of the original iterator A.
// If not, then axis k of the reframed A is 'dead' and doesn't move the iterator.
// TODO invalid for ANY, since Dest is compile time. [ra7]

template <class Dest, IteratorConcept A>
struct Reframe
{
    A a;

    consteval static rank_t
    rank()
    {
        return 1 + std::apply([](auto ... i) { int r=-1; ((r=std::max(r, int(i))), ...); return r; }, Dest {});
    }
    constexpr static int orig(int k)
    {
        return mp::int_list_index<Dest>(k);
    }
    constexpr static dim_t len_s(int k)
    {
        int l=orig(k);
        return l>=0 ? std::decay_t<A>::len_s(l) : BAD;
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
        return l>=0 ? a.len(l) : BAD;
    }
    constexpr auto
    step(int k) const
    {
        int l=orig(k);
        return l>=0 ? a.step(l) : samestep<0, decltype(a.step(l))>;
    }
    constexpr void
    adv(rank_t k, dim_t d)
    {
        int l=orig(k);
        if (l>=0) { a.adv(l, d); }
    }
    constexpr static bool
    keep_step(dim_t st, int z, int j) requires (requires { std::decay_t<A>::keep_step(st, z, j); })
    {
        int wz=orig(z), wj=orig(j);
        return wz>=0 && wj>=0 && std::decay_t<A>::keep_step(st, wz, wj);
    }
    constexpr bool
    keep_step(dim_t st, int z, int j) const requires (!(requires { std::decay_t<A>::keep_step(st, z, j); }))
    {
        int wz=orig(z), wj=orig(j);
        return wz>=0 && wj>=0 && a.keep_step(st, wz, wj);
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        return a.at(std::apply([&i](auto ... t) { return std::array<dim_t, sizeof...(t)> { i[t] ... }; }, Dest {}));
    }
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

template <class cranks_, class Op_>
struct Verb
{
    using cranks = cranks_;
    using Op = Op_;
    Op op;
};

RA_IS_DEF(is_verb, (std::is_same_v<A, Verb<typename A::cranks, typename A::Op>>))

template <class cranks, class Op>
constexpr auto
wrank(cranks cranks_, Op && op) { return Verb<cranks, Op> { RA_FWD(op) }; }

template <rank_t ... crank, class Op>
constexpr auto
wrank(Op && op) { return Verb<mp::int_list<crank ...>, Op> { RA_FWD(op) }; }

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
struct Framematch_def;

template <class V, class T, class R=mp::makelist<mp::len<T>, mp::nil>, rank_t skip=0>
using Framematch = Framematch_def<std::decay_t<V>, T, R, skip>;

template <class A, class B>
struct max_i
{
    constexpr static int value = (A::value == choose_rank(A::value, B::value)) ? 0 : 1;
};

// Get a list (per argument) of lists of live axes. The last frame match is handled by standard prefix matching.
template <class ... crank, class W, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<Verb<std::tuple<crank ...>, W>, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(crank) && sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
// live = number of live axes on this frame, for each argument. // TODO crank negative, inf.
    using live = mp::int_list<(rank_s<Ti>() - mp::len<Ri> - crank::value) ...>;
    using frameaxes = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri> - crank::value), skip>> ...>;
    using FM = Framematch<W, std::tuple<Ti ...>, frameaxes, skip + mp::ref<live, mp::indexof<max_i, live>>::value>;
    using R = typename FM::R;
    template <class VV> constexpr static decltype(auto) op(VV && v) { return FM::op(RA_FWD(v).op); } // cf [ra31]
};

// Terminal case where V doesn't have rank (is a raw op()).
template <class V, class ... Ti, class ... Ri, rank_t skip>
struct Framematch_def<V, std::tuple<Ti ...>, std::tuple<Ri ...>, skip>
{
    static_assert(sizeof...(Ti)==sizeof...(Ri), "Bad arguments.");
// TODO -crank::value when the actual verb rank is used (eg to use CellBig<... that_rank> instead of just begin()).
    using R = std::tuple<mp::append<Ri, mp::iota<(rank_s<Ti>() - mp::len<Ri>), skip>> ...>;
    template <class VV> constexpr static decltype(auto) op(VV && v) { return RA_FWD(v); }
};


// ---------------
// explicit agreement checks
// ---------------

constexpr bool
agree(auto && ... p) { return agree_(ra::start(RA_FWD(p)) ...); }

// 0: fail, 1: rt, 2: pass
constexpr int
agree_s(auto && ... p) { return agree_s_(ra::start(RA_FWD(p)) ...); }

template <class Op, class ... P> requires (is_verb<Op>)
constexpr bool
agree_op(Op && op, P && ... p) { return agree_verb(mp::iota<sizeof...(P)> {}, RA_FWD(op), RA_FWD(p) ...); }

template <class Op, class ... P> requires (!is_verb<Op>)
constexpr bool
agree_op(Op && op, P && ... p) { return agree(RA_FWD(p) ...); }

template <class ... P>
constexpr bool
agree_(P && ... p) { return (Match<false, std::tuple<P ...>> { RA_FWD(p) ... }).check(); }

template <class ... P>
constexpr int
agree_s_(P && ... p) { return Match<false, std::tuple<P ...>>::check_s(); }

template <class V, class ... T, int ... i>
constexpr bool
agree_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return agree_op(FM::op(RA_FWD(v)), reframe<mp::ref<typename FM::R, i>>(ra::start(RA_FWD(t))) ...);
}


// ---------------------------
// operator expression
// ---------------------------

template <class Op, class T, class K=mp::iota<mp::len<T>>> struct Expr;
template <class Op, IteratorConcept ... P, int ... I>
struct Expr<Op, std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    using Match_ = Match<true, std::tuple<P ...>>;
    using Match_::t, Match_::rs, Match_::rank;
    Op op;

    constexpr Expr(Op op_, P ... p_): Match_(p_ ...), op(op_) {} // [ra1]
    RA_ASSIGNOPS_SELF(Expr)
    RA_ASSIGNOPS_DEFAULT_SET
    constexpr decltype(auto) at(auto const & j) const { return std::invoke(op, std::get<I>(t).at(j) ...); }
    constexpr decltype(auto) operator*() const { return std::invoke(op, *std::get<I>(t) ...); }
// needed for rs==ANY, which don't decay to scalar when used as operator arguments.
    constexpr
    operator decltype(std::invoke(op, *std::get<I>(t) ...)) () const
    {
        if constexpr (1!=size_s<Expr>()) {
            RA_CHECK(1==size(*this), "Bad conversion to scalar from shape [", ra::noshape, ra::shape(*this), "].");
        }
        return *(*this);
    }
};

template <class Op, IteratorConcept ... P>
constexpr bool is_special_def<Expr<Op, std::tuple<P ...>>> = (is_special<P> || ...);

template <class V, class ... T, int ... i>
constexpr auto
expr_verb(mp::int_list<i ...>, V && v, T && ... t)
{
    using FM = Framematch<V, std::tuple<T ...>>;
    return expr(FM::op(RA_FWD(v)), reframe<mp::ref<typename FM::R, i>>(RA_FWD(t)) ...);
}

template <class Op, class ... P>
constexpr auto
expr(Op && op, P && ... p)
{
    if constexpr (is_verb<Op>) {
        return expr_verb(mp::iota<sizeof...(P)> {}, RA_FWD(op), RA_FWD(p) ...);
    } else {
        return Expr<Op, std::tuple<P ...>> { RA_FWD(op), RA_FWD(p) ... };
    }
}

constexpr auto
map(auto && op, auto && ... a) { return expr(RA_FWD(op), start(RA_FWD(a)) ...); }


// ---------------------------
// pick expression
// ---------------------------

template <class T, class J> struct pick_at_type;
template <class ... P, class J> struct pick_at_type<std::tuple<P ...>, J>
{
    using type = std::common_reference_t<decltype(std::declval<P>().at(std::declval<J>())) ...>;
};

template <std::size_t I, class T, class J>
constexpr pick_at_type<mp::drop1<std::decay_t<T>>, J>::type
pick_at(std::size_t p0, T && t, J const & j)
{
    constexpr std::size_t N = mp::len<std::decay_t<T>> - 1;
    if constexpr (I < N) {
        return (p0==I) ? std::get<I+1>(t).at(j) : pick_at<I+1>(p0, t, j);
    } else {
        RA_CHECK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class T> struct pick_star_type;
template <class ... P> struct pick_star_type<std::tuple<P ...>>
{
    using type = std::common_reference_t<decltype(*std::declval<P>()) ...>;
};

template <std::size_t I, class T>
constexpr pick_star_type<mp::drop1<std::decay_t<T>>>::type
pick_star(std::size_t p0, T && t)
{
    constexpr std::size_t N = mp::len<std::decay_t<T>> - 1;
    if constexpr (I < N) {
        return (p0==I) ? *(std::get<I+1>(t)) : pick_star<I+1>(p0, t);
    } else {
        RA_CHECK(p0 < N, "Bad pick ", p0, " with ", N, " arguments."); std::abort();
    }
}

template <class T, class K=mp::iota<mp::len<T>>> struct Pick;
template <IteratorConcept ... P, int ... I>
struct Pick<std::tuple<P ...>, mp::int_list<I ...>>: public Match<true, std::tuple<P ...>>
{
    using Match_ = Match<true, std::tuple<P ...>>;
    using Match_::t, Match_::rs, Match_::rank;
    static_assert(sizeof...(P)>1);

    constexpr Pick(P ... p_): Match_(p_ ...) {} // [ra1]
    RA_ASSIGNOPS_SELF(Pick)
    RA_ASSIGNOPS_DEFAULT_SET
    constexpr decltype(auto) at(auto const & j) const { return pick_at<0>(std::get<0>(t).at(j), t, j); }
    constexpr decltype(auto) operator*() const { return pick_star<0>(*std::get<0>(t), t); }
// needed for rs==ANY, which don't decay to scalar when used as operator arguments.
    constexpr
    operator decltype(pick_star<0>(*std::get<0>(t), t)) () const
    {
        if constexpr (1!=size_s<Pick>()) {
            RA_CHECK(1==size(*this), "Bad conversion to scalar from shape [", ra::noshape, ra::shape(*this), "].");
        }
        return *(*this);
    }
};

template <IteratorConcept ... P>
constexpr bool is_special_def<Pick<std::tuple<P ...>>> = (is_special<P> || ...);

template <class ... P>
Pick(P && ... p) -> Pick<std::tuple<P ...>>;

constexpr auto
pick(auto && ... p) { return Pick { start(RA_FWD(p)) ... }; }

} // namespace ra
