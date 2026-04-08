// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Views and containers.

// (c) Daniel Llorens - 2013-2026
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "ply.hh"
#include <memory>

namespace ra {

constexpr bool
c_order(auto const & dimv, bool step1=true)
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

constexpr decltype(auto)
to_ravel(auto && x, auto && b)
{
    if constexpr (ANY==ra::size_s(x) || ANY==ra::size_s(b)) {
        RA_CK(ra::size(x)==ra::size(b), "Mismatched sizes ", ra::size(x), " [", fmt(nstyle, ra::shape(b)), "].");
    } else {
        static_assert(ra::size_s(x)==ra::size_s(b), "Mismatched sizes."); // c++26
    }
    std::ranges::copy(RA_FW(x), b.begin());
    return RA_FW(b);
}


// ---------------------
// Views
// ---------------------

template <class T, class Dimv> struct nested_arg_ { using type = noarg<>; };
template <class T, class Dimv> using nested_arg = typename nested_arg_<T, Dimv>::type;

template <class T, class Dimv, class nested_args = mp::makelist<(0==ssize(Dimv::value) ? 0 : Dimv::value[0].len), nested_arg<T, Dimv>>>
struct SmallArray;

template <class T, class Dimv> requires (requires { T(); } && 0<ssize(Dimv::value) && 0<=dimv_size(Dimv::value))
struct nested_arg_<T, Dimv>
{
    constexpr static auto n = ssize(Dimv::value)-1;
    constexpr static auto s = std::apply([](auto ... i){ return std::array<dim_t, n> { Dimv::value[i].len ... }; }, mp::iota<n, 1>{});
    using type = std::conditional_t<0==n, T, SmallArray<T, ic_t<c_dimv(s)>>>;
};

// FIXME Let any expr = braces.
template <class T, rank_t R> struct braces_ { using type = T; };
template <class T, rank_t R> using braces = braces_<T, R>::type;
template <class T, rank_t R> requires (R>0) struct braces_<T, R> { using type = std::initializer_list<braces<T, R-1>>; };

template <class T, rank_t R>
constexpr auto
braces_shape(braces<T, R> const & l)
{
    std::array<dim_t, R> s = {};
    [&s](this auto const & sf, auto RR, auto const & l){
        if constexpr (RR>0) {
            s[R-RR] = l.size();
            if (l.size()>0) sf(ic<RR-1>, *l.begin());
        }
    }(ic<R>, l);
    return s;
}

template <class P, class Dimv_>
struct View: public ViewBase<Dimv_>
{
    using Dimv = Dimv_;
    using ViewBase<Dimv_>::dimv, ViewBase<Dimv_>::simv;
    constexpr static bool CT = is_ctype<Dimv>;
    constexpr static rank_t R = ra::size_s<decltype(dimv)>();
#define RAC(k, f) (CT || 0==R || is_ctype<decltype(simv[k].f)>)
    consteval static rank_t rank() requires (R!=ANY) { return R; }
    constexpr rank_t rank() const requires (R==ANY) { return dimv.size(); }
    constexpr static dim_t len_s(auto k) { if constexpr (RAC(k, len)) return len(k); else return ANY; }
    constexpr static dim_t len(auto k) requires (RAC(k, len)) { return simv[k].len; }
    constexpr dim_t len(int k) const requires (!(RAC(k, len))) { return dimv[k].len; }
    constexpr static dim_t step(auto k) requires (RAC(k, step)) { return simv[k].step; }
    constexpr dim_t step(int k) const requires (!(RAC(k, step))) { return dimv[k].step; }
    consteval static dim_t size() requires (CT || 0==R) { return dimv_size(simv); }
    consteval static bool empty() requires (CT || 0==R) { return dimv_empty(simv); }
    constexpr dim_t size() const requires (!(CT || 0==R)) { return dimv_size(dimv); }
    constexpr bool empty() const requires (!(CT || 0==R)) { return dimv_empty(dimv); }
#undef RAC
    P cp;
    using T = std::remove_reference_t<decltype(*cp)>;
    constexpr auto data() const { return cp; }
// FIXME constructor checks (lens>=0, steps inside, etc.).
    constexpr View() requires (!CT) {} // used by Array constructors
    constexpr View(Dimv && dimv, P cp) requires (!CT && !std::is_reference_v<Dimv>): ViewBase<Dimv>{std::move(dimv)}, cp(cp) {} // cannot assign dimv later
    constexpr View(Dimv const & dimv, P cp) requires (!CT): ViewBase<Dimv>{dimv}, cp(cp) {} // cannot assign dimv later
    constexpr View(Slice auto && x) requires requires { View(RA_FW(x).dimv, x.data()); }: View(RA_FW(x).dimv, x.data()) {}
    constexpr View(auto const & s, P cp) requires (!CT) && requires { [](dim_t){}(VAL(s)); }: cp(cp) { filldimv(iter(s), dimv); }
    constexpr View(auto const & s, P cp) requires (!CT) && requires { [](Dim){}(VAL(s)); }: cp(cp) { resize(dimv, ra::size(s)); iter(dimv) = s; } // [ra37]
    template <int N> constexpr View(dim_t (&&s)[N], P cp) requires (!CT): View(iter(s), cp) {}
    template <int N> constexpr View(Dim (&&s)[N], P cp) requires (!CT): View(iter(s), cp) {}
    constexpr explicit View(P cp): cp(cp) {} // empty dimv, but also uninit by slicers, esp. has_len<P>
    constexpr View(View &&) noexcept = default; // must declare bc Dimv may be a reference.
    constexpr View(View const &) = default;
    constexpr View const & operator=(braces<T, R> x) const requires (!CT && 1<R) { iter<-1>(*this) = x; return *this; } // nested
#define RA_BRACES(N) constexpr View const & operator=(braces<T, N> x) const requires (!CT && R==ANY) { iter<-1>(*this) = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES
    template <int N> constexpr View const & operator=(nested_arg<T, Dimv> (&&x)[N]) const requires (CT && 1<R || 1!=len_s(0)) // nested
    {
        iter<-1>(*this) = x;
        if !consteval { asm volatile("" ::: "memory"); } // patch for [ra01]
        return *this;
    }
    constexpr View const & operator=(View const & x) const { iter(*this) = x; return *this; }
    template <int N> constexpr View const & operator=(T (&&x)[N]) const { return to_ravel(RA_FW(x), *this); } // row major
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & t) const { iter(*this) OP ra::scalar(t); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) const { iter(*this) OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) const { iter(*this) OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    constexpr auto begin() const { if constexpr ((CT && c_order(simv)) || (1==R && 1==simv[0].step)) return cp; else return STLIterator(iter(*this)); }
    constexpr auto end() const requires ((CT && c_order(simv)) || (1==R && 1==simv[0].step)) { return cp+size(); }
    constexpr static auto end() requires (!(CT && c_order(simv)) && !(1==R && 1==simv[0].step)) { return std::default_sentinel; }
    constexpr decltype(auto) back(this auto && sf) { static_assert(!CT || len_s(0)>0, "Bad back()."); return RA_FW(sf)(sf.len(0)-1); }
    constexpr decltype(auto) operator()(this auto && sf, auto && ... i) { return from(RA_FW(sf), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && sf, auto && ... i) { return from(RA_FW(sf), RA_FW(i) ...); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
    template <dim_t s, dim_t o=0> constexpr auto as() const requires (CT) { return from(*this, iota(ic<s>, o)); }
// conversion to const is done differently for (!CT) (conversion op -> &) vs CT (constructor > value, which is just *).
    using U = std::remove_const_t<T>;
    constexpr operator View<T const *, Dimv> const & () const requires (!CT && std::is_same_v<P, U *>) { return *reinterpret_cast<View<T const *, Dimv> const *>(this); }
    constexpr View(View<U *, Dimv> const & x) requires (CT && std::is_same_v<P, T const *>): View(x.cp) {}
};


// ---------------------
// Array
// ---------------------

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

template <class T, class Dimv_, class ... nested_args>
struct
#if RA_OPT_SMALL==1
alignas(align_req<T, dimv_size(Dimv_::value)>)
#endif
SmallArray<T, Dimv_, std::tuple<nested_args ...>>
{
    using Dimv = Dimv_;
    constexpr static auto dimv = Dimv::value;
    constexpr static auto simv = Dimv::value;
    consteval static rank_t rank() { return ssize(dimv); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static dim_t size() { return dimv_size(dimv); }
// from std::array
    struct T0
    {
        [[noreturn]] constexpr T & operator[](size_t) const noexcept { abort(); }
        constexpr explicit operator T*() const noexcept { return nullptr; }
    };
    [[no_unique_address]] std::conditional_t<0==size(), T0, T[size()]> cp;
    constexpr auto data() { return (T *)cp; }
    constexpr auto data() const { return (T const *)cp; }
    constexpr auto view() { return View<T *, Dimv>(data()); }
    constexpr auto view() const { return View<T const *, Dimv>(data()); }
    constexpr SmallArray() {}
    constexpr SmallArray(none_t) {}
    constexpr explicit (1!=size()) SmallArray(T const & t) { std::ranges::fill(cp, t); } // [ra12][ra44]
// p1219??
    constexpr SmallArray(std::convertible_to<T> auto const & ... x) requires (sizeof...(x)==size()) { view() = { static_cast<T>(x) ... }; } // row major
    constexpr SmallArray(nested_args const & ... x) requires (1!=rank() || 1!=len(0)) { view() = { x ... }; } // nested
    constexpr SmallArray(auto const & x) { view() = x; }
    constexpr SmallArray(Iterator auto && x) { view() = RA_FW(x); }
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & x) { iter(*this) OP ra::scalar(x); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) { iter(*this) OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) { iter(*this) OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <int s, int o=0> constexpr decltype(auto) as(this auto && sf) { return RA_FW(sf).view().template as<s, o>(); }
    constexpr auto begin(this auto && sf) { return RA_FW(sf).view().begin(); }
    constexpr auto end(this auto && sf) { return RA_FW(sf).view().end(); }
    constexpr decltype(auto) back(this auto && sf) { return RA_FW(sf).view().back(); }
    constexpr decltype(auto) operator()(this auto && sf, auto && ... i) { return from(RA_FW(sf), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && sf, auto && ... i) { return from(RA_FW(sf), RA_FW(i) ...); }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
};

template <class T, dim_t ... lens>
using Small = SmallArray<T, ic_t<c_dimv(std::array<dim_t, sizeof...(lens)>{lens ...})>>;

template <class A0, class ... A> SmallArray(A0, A ...) -> Small<A0, 1+sizeof...(A)>;

template <class V>
struct storage_traits
{
    using T = V::value_type;
    static_assert(std::is_same_v<T *, decltype(std::declval<V>().data())>, "Bad store.");
    constexpr static auto create(dim_t n) { RA_CK(0<=n, "Bad size ", n, "."); return V(n); }
    constexpr static auto data(auto & v) { return v.data(); }
};

template <class P>
struct storage_traits<std::unique_ptr<P>>
{
    using V = std::unique_ptr<P>;
    using T = std::remove_reference_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CK(0<=n, "Bad size ", n, "."); return V(new T[n]); }
    constexpr static auto data(auto & v) { return v.get(); }
};

template <class P>
struct storage_traits<std::shared_ptr<P>>
{
    using V = std::shared_ptr<P>;
    using T = std::remove_reference_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CK(0<=n, "Bad size ", n, "."); return V(new T[n], std::default_delete<T[]>()); }
    constexpr static auto data(auto & v) { return v.get(); }
};

// FIXME Requires copyable T. store(x) avoids it for Big, but not for Unique. Should construct in place like std::vector.
template <class Store, class Dimv_>
struct Array
{
    using Dimv = Dimv_;
    Dimv dimv;
    constexpr static Dimv const simv = {};
    constexpr static rank_t R = ra::size_s<Dimv>();
#define RAC(k, f) (0==R || is_ctype<decltype(simv[k].f)>)
    consteval static rank_t rank() requires (R!=ANY) { return R; }
    constexpr rank_t rank() const requires (R==ANY) { return dimv.size(); }
    constexpr static dim_t len_s(auto k) { if constexpr (RAC(k, len)) return len(k); else return ANY; }
    constexpr static dim_t len(auto k) requires (RAC(k, len)) { return simv[k].len; }
    constexpr dim_t len(int k) const requires (!(RAC(k, len))) { return dimv[k].len; }
    constexpr static dim_t step(auto k) requires (RAC(k, step)) { return simv[k].step; }
    constexpr dim_t step(int k) const requires (!(RAC(k, step))) { return dimv[k].step; }
    constexpr dim_t size() const { return dimv_size(dimv); }
    constexpr bool empty() const { return dimv_empty(dimv); }
#undef RAC
    Store store;
    using T = typename storage_traits<Store>::T;
    constexpr auto data(this auto && sf) { return storage_traits<Store>::data(sf.store); }
    constexpr auto view() { return View<T *, Dimv const &>(dimv, data()); }
    constexpr auto view() const { return View<T const *, Dimv const &>(dimv, data()); }
// unlike the defaulted operator=(Array), operator=(not Array) treat *this like View. See operator>>(std::istream &, Array &), test/ownership.cc [ra20].
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & x) { iter(*this) OP ra::scalar(x); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) { iter(*this) OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) { iter(*this) OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <int N> constexpr Array & operator=(T (&&x)[N]) requires (1!=R) { view() = RA_FW(x); return *this; }
    constexpr Array & operator=(braces<T, R> x) requires (R>=1) { view() = x; return *this; }
#define RA_BRACES(N) constexpr Array & operator=(braces<T, N> x) requires (ANY==R) { view() = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES
    constexpr static Dimv zdimv = []{ Dimv dv; for(auto & dim: dv) dim=Dim {0, 1}; return dv; }();
    constexpr Array() requires (R>0): dimv {zdimv} {}
    constexpr Array() requires (R==ANY): dimv {{0, 1}} {}
    constexpr Array() requires (0==R): Array({}, none) {}
    constexpr Array(auto const & x): Array(ra::shape(x), x) {}
    constexpr Array(braces<T, R> x) requires (R>=1): Array(braces_shape<T, R>(x), x) {}
#define RA_BRACES(N) constexpr Array(braces<T, N> x) requires (R==ANY): Array(braces_shape<T, N>(x), x) {}
    RA_FE(RA_BRACES, 1, 2, 3, 4)
#undef RA_BRACES
    constexpr Array(auto && s, none_t) { store = storage_traits<Store>::create(filldimv(iter(RA_FW(s)), dimv)); }
    constexpr Array(auto && s, auto const & x): Array(RA_FW(s), none) { view() = x; }
    constexpr Array(auto && s, std::initializer_list<T> x): Array(RA_FW(s), none) { to_ravel(x, *this); }
    constexpr Array(std::array<dim_t, 0> s, auto const & x): Array(iter(s), x) {};
    constexpr Array(std::array<dim_t, 0> s, std::initializer_list<T> x): Array(iter(s), x) {}
    template <int N> constexpr Array(dim_t (&&s)[N], auto const & x): Array(iter(s), x) {}
    template <int N> constexpr Array(dim_t (&&s)[N], std::initializer_list<T> x): Array(iter(s), x) {}
    template <int N> constexpr Array(dim_t (&&s)[N], auto * p): Array(iter(s), none) { std::ranges::copy_n(p, size(), begin()); }
#define RA_CR(left) if constexpr (ANY==R) { RA_CK(left rank()); } else { static_assert(left R); }
// resize first axis or full shape. Only for some kinds of store.
    constexpr void resize(dim_t const s) { RA_CR(0<); dimv[0].len = s; store.resize(size()); }
    constexpr void resize(dim_t const s, T const & t) { RA_CR(0<); dimv[0].len = s; store.resize(size(), t); }
    constexpr void resize(auto const & s) requires (1==rank_s(s)) { store.resize(filldimv(iter(s), dimv)); }
// auto && + RA_FW wouldn't work for push_back(brace-enclosed-list). p1219??
    constexpr void push_back(T && t) { RA_CR(1==); store.push_back(std::move(t)); ++dimv[0].len; }
    constexpr void push_back(T const & t) { RA_CR(1==); store.push_back(t); ++dimv[0].len; }
    constexpr void emplace_back(auto && ... a) { RA_CR(1==); store.emplace_back(RA_FW(a) ...); ++dimv[0].len; }
    constexpr void pop_back() { RA_CR(1==); RA_CK(0<dimv[0].len, "Empty array pop_back()."); --dimv[0].len; store.pop_back(); }
#undef RA_CR
    constexpr auto begin(this auto && sf) { return sf.data(); }
    constexpr auto end(this auto && sf) { return sf.data()+sf.size(); }
    constexpr decltype(auto) back(this auto && sf) { return RA_FW(sf).view().back(); }
    constexpr decltype(auto) operator()(this auto && sf, auto && ... i) { return from(RA_FW(sf), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && sf, auto && ... i) { return from(RA_FW(sf), RA_FW(i) ...); }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
};

template <class Store, rank_t R=ANY> using Container = Array<Store, std::conditional_t<1==R, std::array<SDim<dim_t, ic_t<1>>, 1>, BigDimv<R>>>;
template <class T, rank_t R=ANY> using Big = Container<vector_default_init<T>, R>;
template <class T, rank_t R=ANY> using Unique = Container<std::unique_ptr<T []>, R>;
template <class T, rank_t R=ANY> using Shared = Container<std::shared_ptr<T>, R>;

// rely on std::swap; else ambiguous
template <class Store, class DA, class DB> requires (!std::is_same_v<DA, DB>)
void
swap(Array<Store, DA> & a, Array<Store, DB> & b)
{
    if constexpr ((ANY==a.R)==(ANY==b.R)) {
        static_assert(a.R==b.R);
        std::swap(a.dimv, b.dimv);
    } else {
        RA_CK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " ", rank(b), ".");
        auto [xa, xb] = [&]{ if constexpr (ANY==a.R) return std::tie(a, b); else return std::tie(b, a); }();
        decltype(xb.dimv) c;
        iter(c) = xa.dimv;
        iter(xa.dimv) = xb.dimv;
        std::swap(xb.dimv, c);
    }
    std::swap(a.store, b.store);
}

// TODO Can use unique_ptr's deleter for this?
// TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args.

auto
shared_borrowing(Slice auto & s)
{
    using T = value_t<decltype(s)>;
    Shared<T, rank_s(s)> a;
    a.store = std::shared_ptr<T>(s.data(), [](T *){});
    if constexpr (1==rank_s(s)) { a.dimv[0] = s.dimv[0]; } else { a.dimv = s.dimv; } // [Dimv] = [SDim] etc.
    return a;
}

template <int w> constexpr auto tindex = reframe(iter(iota()), ilist_t<w> {});
#define RA_TINDEX(w) constexpr auto RA_JOIN(_, w) = tindex<w>;
RA_FE(RA_TINDEX, 0, 1, 2, 3, 4);
#undef RA_TINDEX


// --------------------
// Array if rank>0, else value type, even unregistered. FIXME concrete-or-view.
// --------------------

template <class E> struct concrete_type_;

template <class E> requires (0==rank_s<E>())
struct concrete_type_<E> { using type = ncvalue_t<E>; };

template <class E> requires (ANY==size_s<E>())
struct concrete_type_<E> { using type = Big<ncvalue_t<E>, rank_s<E>()>; };

template <class E> requires (0!=rank_s<E>() && ANY!=size_s<E>())
struct concrete_type_<E> { using type = SmallArray<ncvalue_t<E>, ic_t<c_dimv(shape_s<E>)>>; };

template <class E>
using concrete_type = std::conditional_t<(0==rank_s<E>() && !is_ra<E>), std::decay_t<E>, typename concrete_type_<E>::type>;

template <class E> constexpr auto
concrete(E && e) { return concrete_type<E>(RA_FW(e)); }

template <class E, class ... X> constexpr auto
with_same_shape(E && e, X && ... x) requires (ANY!=size_s<E>()) { return concrete_type<E>(RA_FW(x) ...); }

template <class E> constexpr auto
with_same_shape(E && e) requires (ANY==size_s<E>()) { return concrete_type<E>(ra::shape(e), none); }

template <class E, class X> constexpr auto
with_same_shape(E && e, X && x) requires (ANY==size_s<E>()) { return concrete_type<E>(ra::shape(e), RA_FW(x)); }

template <class E, class S, class X> constexpr auto
with_shape(S && s, X && x) requires (ANY!=size_s<E>()) { return concrete_type<E>(RA_FW(x)); }

template <class E, class S, class X> constexpr auto
with_shape(S && s, X && x) requires (ANY==size_s<E>()) { return concrete_type<E>(RA_FW(s), RA_FW(x)); }

template <class E, class S, class X> constexpr auto
with_shape(std::initializer_list<S> && s, X && x) { return with_shape<E>(iter(s), RA_FW(x)); }


// --------------------
// View ops. FIXME Slice && takes Array & / View & / View &&, but Array && is a risk.
// --------------------

template <class K=ic_t<0>>
constexpr auto
reverse(Slice auto && a, K k = K {})
{
    if constexpr (rank_s(a)>=0 && is_ctype<K>) {
        static_assert(inside(k, rank_s(a)), "Bad axis for rank.");
        static_assert(UNB!=a.len_s(k), "Cannot reverse unbounded axis.");
    } else {
        RA_CK(UNB!=a.len(k), "Cannot reverse unbounded axis ", k, ".");
        RA_CK(inside(k, rank(a)), "Bad axis ", k, " for rank ", rank(a), ".");
    }
    constexpr auto revp = [](auto const & ak){ return 0==ak.len ? 0 : ak.step*(ak.len-1); };
    constexpr auto revd = [](auto const & av, auto k){ auto dv=av; dv[k].step *= -1; return dv; };
    if constexpr (1==rank_s(a) && !std::is_same_v<std::decay_t<decltype(a.simv[0])>, Dim>) {
        return viewptr(a.data() + revp(a.dimv[0]), a.dimv[0].len, csub(ic<0>, a.dimv[0].step));
    } else if constexpr (is_ctype<typename std::decay_t<decltype(a)>::Dimv>) {
        return View<decltype(a.data()), ic_t<revd(a.dimv, k)>>(a.data() + ic<revp(a.dimv[k])>);
    } else {
        return ViewBig<decltype(a.data()), rank_s(a)>(revd(a.dimv, k), a.data() + revp(a.dimv[k]));
    }
}

// FIXME beat reframe(view) into transpose(view).
constexpr void
transpose_dims(auto const & s, auto const & src, auto & dst)
{
    static_assert(ANY==ra::size_s(src) || ra::size_s(src)==ra::size_s(s), "Bad rank for transposed axes."); // c++26
    RA_CK(ra::size(s)==ra::size(src), "Bad rank ", ra::size(src), " for ", ra::size(s), " transposed axes.");
    std::ranges::fill(dst, Dim {UNB, 0});
    for (int k=0; int sk: s) {
        dst[sk].step += src[k].step;
        dst[sk].len = dst[sk].len>=0 ? std::min(dst[sk].len, src[k].len) : src[k].len;
        ++k;
    }
}

constexpr auto
transpose(Slice auto && a, auto && s)
{
    constexpr auto trank = [](auto const & s){ return 0==ra::size(s) ? 0 : 1+std::ranges::max(s); };
    constexpr auto tdimv = [](auto const & s, auto const & a, auto && r){ transpose_dims(s, a.dimv, r); return std::move(r); };
    if constexpr (!is_ctype<decltype(s)>) {
        using V = ViewBig<decltype(a.data()), ANY>;
        return V(tdimv(s, a, decltype(V::dimv)(trank(s))), a.data());
    } else if constexpr (is_ctype<typename std::decay_t<decltype(a)>::Dimv>) {
        return View<decltype(a.data()), ic_t<tdimv(s.value, a, std::array<Dim, trank(s.value)> {})>>(a.data());
    } else {
        return ViewBig<decltype(a.data()), trank(s.value)>(tdimv(s.value, a, std::array<Dim, trank(s.value)> {}), a.data());
    }
}

template <int ... I> constexpr auto transpose(Slice auto && a, ilist_t<I ...>) { return transpose(RA_FW(a), ic<std::array<dim_t, sizeof...(I)> {I ...}>); }
template <class T, int N> constexpr auto transpose(Slice auto && a, T (&&s)[N]) { return transpose(RA_FW(a), iter(s)); }
constexpr auto transpose(Slice auto && a) { return transpose(RA_FW(a), ilist<1, 0>); }
constexpr auto diag(Slice auto && a) { return transpose(RA_FW(a), ilist<0, 0>); };

constexpr auto
cat(auto && a1, auto && a2) requires (ra::size_s(a1)>=0 && ra::size_s(a2)>=0)
{
    static_assert((1==rank(a1) || 0==rank(a1)) && (1==rank(a2) || 0==rank(a2)), "Bad ranks for cat.");
    Small<std::common_type_t<ncvalue_t<decltype(a1)>, ncvalue_t<decltype(a2)>>, ra::size(a1)+ra::size(a2)> val;
    val(iota(ic<ra::size(a1)>)) = a1;
    val(iota(ic<ra::size(a2)>, ra::size(a1))) = a2;
    return val;
}

constexpr auto
ravel_free(Slice auto const & a)
{
    RA_CK(c_order(a.dimv, false), "View cannot be raveled.");
    int r = a.rank()-1;
    for (; r>=0 && a.len(r)==1; --r) {}
    return ViewBig<decltype(a.data()), 1>({{ra::size(a), r<0 ? 1 : a.step(r)}}, a.data());
}

inline auto
reshape(Slice auto && a, auto && sb_)
{
    auto sb = concrete(RA_FW(sb_));
// FIXME when we need to copy, accept/return Shared
    dim_t la = ra::size(a);
    dim_t lb = 1;
    for (int i=0; i<ra::size(sb); ++i) {
        if (sb[i]==-1) {
            dim_t quot = lb;
            for (int j=i+1; j<ra::size(sb); ++j) {
                quot *= sb[j];
                RA_CK(quot>0, "Cannot deduce dimensions.");
            }
            auto pv = la/quot;
            RA_CK((la%quot==0 && pv>=0), "Bad placeholder.");
            sb[i] = pv;
            lb = la;
            break;
        } else {
            lb *= sb[i];
        }
    }
    auto sa = shape(a);
// FIXME should be able to reshape Scalar etc.
    ViewBig<decltype(a.data()), ra::size_s(sb)> b(map([](auto i){ return Dim { UNB, 0 }; }, iota(ra::size(sb))), a.data());
    rank_t i = 0;
    for (; i<a.rank() && i<b.rank(); ++i) {
        if (sa[a.rank()-i-1]!=sb[b.rank()-i-1]) {
            RA_CK(c_order(a.dimv, false) && la>=lb, "Reshape with copy not implemented.");
// FIXME ViewBig(SS const & s, T * p). Cf [ra37].
            filldimv(iter(sb), b.dimv);
            for (int j=0; j!=b.rank(); ++j) {
                b.dimv[j].step *= a.step(a.rank()-1);
            }
            return b;
        } else { // select
            b.dimv[b.rank()-i-1] = a.dimv[a.rank()-i-1];
        }
    }
    if (i==a.rank()) { // tile
        for (rank_t j=i; j<b.rank(); ++j) {
            b.dimv[b.rank()-j-1] = { sb[b.rank()-j-1], 0 };
        }
    }
    return b;
}

template <class T, int N> constexpr auto reshape(Slice auto && a, T (&&s)[N]) { return reshape(RA_FW(a), iter(s)); }

template <class sup_t, class T>
constexpr void
explode_dims(auto const & av, auto & bv)
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
        bv[i] = Dim {av[i].len, 0==s ? 0 : step/s};
    }
}

template <class sup_t>
constexpr auto
explode(Slice auto && a)
{
    static_assert(std::is_reference_v<decltype(*a.data())>, "Bad type for explode().");
    constexpr static rank_t ru = sizeof(value_t<sup_t>)==sizeof(value_t<decltype(a)>) ? 0 : 1;
    if constexpr (is_ctype<typename std::decay_t<decltype(a)>::Dimv>) {
        constexpr static auto bdimv = [&a]{
            std::array<Dim, ra::rank_s(a)-rank_s<sup_t>()-ru> bdimv;
            explode_dims<sup_t, value_t<decltype(a)>>(a.dimv, bdimv);
            return bdimv;
        }();
        return View<sup_t *, ic_t<bdimv>>(reinterpret_cast<sup_t *>(a.data()));
    } else {
        ViewBig<sup_t *, rank_sum(rank_s(a), -rank_s<sup_t>()-ru)> b;
        ra::resize(b.dimv, a.rank()-rank_s<sup_t>()-ru);
        explode_dims<sup_t, value_t<decltype(a)>>(a.dimv, b.dimv);
        b.cp = reinterpret_cast<sup_t *>(a.data());
        return b;
    }
}

// TODO Check that ranks below SUBR are compact. Version for Small.
template <class sub_t>
constexpr auto
collapse(Slice auto && a)
{
    static_assert(std::is_reference_v<decltype(*a.data())>, "Bad type for collapse().");
    using sup_t = value_t<decltype(a)>;
    using super_v = value_t<sup_t>;
    using sub_v = value_t<sub_t>;
    constexpr int subtype = sizeof(super_v)/sizeof(sub_t);
    constexpr int SUBR = rank_s<sup_t>() - rank_s<sub_t>();
    static auto gstep = [](int i){ if constexpr (is_scalar<sup_t>) return 1; else return sup_t::step(i); };
    static auto glen = [](int i){ if constexpr (is_scalar<sup_t>) return 1; else return sup_t::len(i); };
    ViewBig<sub_t *, rank_sum(rank_s(a), SUBR+int(subtype>1))> b(reinterpret_cast<sub_t *>(a.data()));
    resize(b.dimv, a.rank()+SUBR+int(subtype>1));
    constexpr dim_t r = sizeof(sup_t)/sizeof(sub_t);
    static_assert(sizeof(sup_t)==r*sizeof(sub_t), "Cannot make axis of super from sub.");
    for (int i=0; i<a.rank(); ++i) {
        b.dimv[i] = Dim { a.len(i), a.step(i)*r };
    }
    constexpr dim_t t = sizeof(super_v)/sizeof(sub_v);
    constexpr dim_t s = sizeof(sub_t)/sizeof(sub_v);
    static_assert(t*sizeof(sub_v)>=1, "Bad sub.");
    for (int i=0; i<SUBR; ++i) {
        dim_t step = gstep(i);
        RA_CK(0==step % s, "Bad steps.");
        b.dimv[a.rank()+i] = Dim { glen(i), step/s*t };
    }
    if (subtype>1) {
        b.dimv[a.rank()+SUBR] = Dim { t, 1 };
    }
    return b;
}

// lo: lower bounds, hi: upper bounds. The stencil indices are in [0 lo+1+hi] = [-lo +hi].
inline auto
stencil(Slice auto && a, auto && lo, auto && hi)
{
    ViewBig<decltype(a.data()), rank_sum(rank_s(a), rank_s(a))> s;
    s.cp = a.data();
    ra::resize(s.dimv, 2*a.rank());
    RA_CK(every(lo>=0) && every(hi>=0), "Bad stencil bounds lo ", fmt(nstyle, lo), " hi ", fmt(nstyle, hi), ".");
    for_each([](auto & dims, auto && dima, auto && lo, auto && hi){
        RA_CK(dima.len>=lo+hi, "Stencil is too large for array.");
        dims = { dima.len-lo-hi, dima.step };
    }, ptr(s.dimv.data()), a.dimv, lo, hi);
    for_each([](auto & dims, auto && dima, auto && lo, auto && hi) {
        dims = { lo+hi+1, dima.step };
    },  ptr(s.dimv.data()+a.rank()), a.dimv, lo, hi);
    return s;
}

} // namespace ra
