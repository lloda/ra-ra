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
template <class T, class Dimv> struct small_args { using type = mp::makelist<Dimv::value[0].len, nested_arg<T, Dimv>>; };
template <class T, class Dimv> requires (0==ssize(Dimv::value)) struct small_args<T, Dimv> { using type = std::tuple<>; };

template <class T, class Dimv, class nested_args = small_args<T, Dimv>::type>
struct SmallArray;

template <class T, class Dimv> requires (requires { T(); } && 0<ssize(Dimv::value) && 0<=dimv_size(Dimv::value))
struct nested_arg_<T, Dimv>
{
    constexpr static auto n = ssize(Dimv::value)-1;
    constexpr static auto s = std::apply([](auto ... i){ return std::array<dim_t, n> { Dimv::value[i].len ... }; }, mp::iota<n, 1>{});
    using type = std::conditional_t<0==n, T, SmallArray<T, ic_t<c_dimv(s)>>>;
};

template <class P, class Dimv_>
struct ViewSmall
{
    using Dimv = Dimv_;
    constexpr static auto dimv = Dimv::value;
    P cp;
    using T = std::remove_reference_t<decltype(*cp)>;

    consteval static rank_t rank() { return dimv.size(); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static dim_t size() { return dimv_size(dimv); }
    consteval static bool empty() { return any(0==map(&Dim::len, dimv)); }

    constexpr explicit ViewSmall(P cp_): cp(cp_) {}
    constexpr ViewSmall(ViewSmall const & s) = default;
    template <class Q> constexpr ViewSmall(ViewSmall<Q, Dimv> const & x) requires (requires { ViewSmall(x.cp); }): ViewSmall(x.cp) {} // FIXME Slice
    constexpr ViewSmall const & operator=(ViewSmall const & x) const { iter() = x; return *this; }
    template <int N> constexpr ViewSmall const & operator=(T (&&x)[N]) const { return to_ravel(RA_FW(x), *this); } // row major
    template <int N> constexpr ViewSmall const & operator=(nested_arg<T, Dimv> (&&x)[N]) const requires (1<rank() || 1!=len(0)) // nested
    {
        iter<-1>() = x;
        if !consteval { asm volatile("" ::: "memory"); } // patch for [ra01]
        return *this;
    }
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & t) const { iter() OP ra::scalar(t); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) const { iter() OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) const { iter() OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <dim_t s, dim_t o=0> constexpr auto as() const { return from(*this, ra::iota(ic<s>, o)); }
    template <rank_t c=0> constexpr auto iter() const { return Cell<P, Dimv, ic_t<c>>(cp); }
    constexpr auto iter(rank_t c) const { return Cell<P, decltype(dimv) const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { if constexpr (c_order(dimv)) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (c_order(dimv)) { return cp+size(); }
    constexpr static auto end() requires (!c_order(dimv)) { return std::default_sentinel; }
    constexpr decltype(auto) back() const { static_assert(size()>0, "Bad back()."); return cp[size()-1]; }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return *indexer(*this, cp, ra::iter(i)); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
    constexpr auto data() const { return cp; }
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
    [&s](this auto const & self, auto RR, auto const & l){
        if constexpr (RR>0) {
            s[R-RR] = l.size();
            if (l.size()>0) self(ic<RR-1>, *l.begin());
        }
    }(ic<R>, l);
    return s;
}

// FIXME avoid duplicating cp, dimv from Container without being parent. Parameterize on Dimv, like Cell.
// FIXME constructor checks (lens>=0, steps inside, etc.).
template <class P, rank_t R>
struct ViewBig
{
    using Dimv = std::conditional_t<ANY==R, vector_default_init<Dim>, std::array<Dim, ANY==R ? 0 : R>>;
    [[no_unique_address]] Dimv dimv;
    P cp;
    using T = std::remove_reference_t<decltype(*cp)>;

    consteval static rank_t rank() requires (R!=ANY) { return R; }
    constexpr rank_t rank() const requires (R==ANY) { return dimv.size(); }
    constexpr static dim_t len_s(int k) { return ANY; }
    constexpr dim_t len(int k) const { return dimv[k].len; }
    constexpr dim_t step(int k) const { return dimv[k].step; }
    constexpr dim_t size() const { return dimv_size(dimv); }
    constexpr bool empty() const { return any(0==map(&Dim::len, dimv)); }

    constexpr ViewBig() {} // used by Container constructors
    constexpr explicit ViewBig(P cp): cp(cp) {} // empty dimv, but also uninit by slicers, esp. has_len<P>
    constexpr ViewBig(ViewBig const &) = default;
    constexpr ViewBig(Slice auto const & x) requires (requires { ViewBig(x.dimv, x.cp); }): ViewBig(x.dimv, x.cp) {}
    constexpr ViewBig(auto const & s, P cp) requires (requires { [](dim_t){}(VAL(s)); }): cp(cp) { filldimv(ra::iter(s), dimv); }
    constexpr ViewBig(auto const & s, P cp) requires (requires { [](Dim){}(VAL(s)); }): cp(cp) { resize(dimv, ra::size(s)); ra::iter(dimv) = s; } // [ra37]
    template <int N> constexpr ViewBig(dim_t (&&s)[N], P cp): ViewBig(ra::iter(s), cp) {}
    template <int N> constexpr ViewBig(Dim (&&s)[N], P cp): ViewBig(ra::iter(s), cp) {}
    constexpr ViewBig const & operator=(ViewBig const & x) const { iter() = x; return *this; }
    template <int N> constexpr ViewBig const & operator=(T (&&x)[N]) const { return to_ravel(RA_FW(x), *this); } // row major
    constexpr ViewBig const & operator=(braces<T, R> x) const requires (1<R) { iter<-1>() = x; return *this; } // nested
#define RA_BRACES(N) constexpr ViewBig const & operator=(braces<T, N> x) const requires (R==ANY) { iter<-1>() = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & t) const { iter() OP ra::scalar(t); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) const { iter() OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) const { iter() OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <rank_t c=0> constexpr auto iter() const && { return Cell<P, Dimv, ic_t<c>>(cp, std::move(dimv)); }
    template <rank_t c=0> constexpr auto iter() const & { return Cell<P, Dimv const &, ic_t<c>>(cp, dimv); }
    constexpr auto iter(rank_t c) const && { return Cell<P, Dimv, dim_t>(cp, std::move(dimv), c); }
    constexpr auto iter(rank_t c) const & { return Cell<P, Dimv const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { return STLIterator(iter<0>()); }
    constexpr static auto end() { return std::default_sentinel; }
    constexpr decltype(auto) back() const { dim_t s=size(); RA_CK(s>0, "Bad back()."); return cp[s-1]; }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return *indexer(*this, cp, ra::iter(i)); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
    constexpr auto data() const { return cp; }
// conversion to const, used by Container::view(). FIXME cf Small
    constexpr operator ViewBig<T const *, R> const & () const requires (std::is_same_v<P, std::remove_const_t<T> *>)
    {
        return *reinterpret_cast<ViewBig<T const *, R> const *>(this);
    }
};


// ---------------------
// Containers
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
alignas(align_req<T, std::apply([](auto ... d){ return (d.len * ... * 1); }, Dimv_::value)>)
#endif
SmallArray<T, Dimv_, std::tuple<nested_args ...>>
{
    using Dimv = Dimv_;
    constexpr static auto dimv = Dimv::value;
    consteval static rank_t rank() { return ssize(dimv); }
    constexpr static dim_t len(int k) { return dimv[k].len; }
    constexpr static dim_t len_s(int k) { return len(k); }
    constexpr static dim_t step(int k) { return dimv[k].step; }
    consteval static dim_t size() { return std::apply([](auto ... i){ return (i.len * ... * 1); }, dimv); }

// from std::array
    struct T0
    {
        [[noreturn]] constexpr T & operator[](size_t) const noexcept { abort(); }
        constexpr explicit operator T*() const noexcept { return nullptr; }
    };
    [[no_unique_address]] std::conditional_t<0==size(), T0, T[size()]> cp;

    constexpr auto data() { return (T *)cp; }
    constexpr T const * data() const { return (T const *)cp; }
    using View = ViewSmall<T *, Dimv>;
    using ViewConst = ViewSmall<T const *, Dimv>;
    constexpr View view() { return View(data()); }
    constexpr ViewConst view() const { return ViewConst(data()); }
    constexpr operator View () { return View(data()); }
    constexpr operator ViewConst () const { return ViewConst(data()); }

    constexpr SmallArray() {}
    constexpr SmallArray(ra::none_t) {}
    constexpr explicit (1!=size()) SmallArray(T const & t) { std::ranges::fill(cp, t); } // [ra12][ra44]
// p1219??
    constexpr SmallArray(std::convertible_to<T> auto const & ... x) requires (sizeof...(x)==size()) { view() = { static_cast<T>(x) ... }; } // row major
    constexpr SmallArray(nested_args const & ... x) requires (1!=rank() || 1!=len(0)) { view() = { x ... }; } // nested
    constexpr SmallArray(auto const & x) { view() = x; }
    constexpr SmallArray(Iterator auto && x) { view() = RA_FW(x); }
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & x) { iter() OP ra::scalar(x); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) { iter() OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) { iter() OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <int s, int o=0> constexpr decltype(auto) as(this auto && self) { return RA_FW(self).view().template as<s, o>(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { return RA_FW(self).view().template iter<c>(); }
    constexpr auto iter(this auto && self, rank_t c) { return RA_FW(self).view().template iter(c); }
    constexpr auto begin(this auto && self) { return self.view().begin(); }
    constexpr auto end(this auto && self) { return self.view().end(); }
    constexpr decltype(auto) back(this auto && self) { return RA_FW(self).view().back(); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) at(this auto && self, auto const & i) { return RA_FW(self).view().at(i); }
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
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CK(0<=n, "Bad size ", n, "."); return V(new T[n]); }
    constexpr static auto data(auto & v) { return v.get(); }
};

template <class P>
struct storage_traits<std::shared_ptr<P>>
{
    using V = std::shared_ptr<P>;
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CK(0<=n, "Bad size ", n, "."); return V(new T[n], std::default_delete<T[]>()); }
    constexpr static auto data(auto & v) { return v.get(); }
};

// FIXME avoid duplicating ViewBig::p. Avoid overhead with rank 1.
// FIXME requires copyable T. store(x) avoids it for Big, but not for Unique. Should construct in place like std::vector.
template <class Store, rank_t R>
struct Container: public ViewBig<typename storage_traits<Store>::T *, R>
{
    Store store;
    using T = typename storage_traits<Store>::T;
    using View = ViewBig<T *, R>;
    using ViewConst = ViewBig<T const *, R>;
    constexpr ViewConst const & view() const { return *this; }
    constexpr View & view() { return *this; }
    using View::size, View::rank, View::dimv, View::cp;
    constexpr auto data(this auto && self) { return self.view().data(); }

// A(shape 2 3) = A-type [1 2 3] initializes, so it doesn't behave as A(shape 2 3) = not-A-type [1 2 3] which uses View::operator=. This is used by operator>>(std::istream &, Container &). See test/ownership.cc [ra20].
    constexpr Container & operator=(Container && w)
    {
        store = std::move(w.store);
        dimv = std::move(w.dimv);
        cp = storage_traits<Store>::data(store);
        return *this;
    }
    constexpr Container & operator=(Container const & w)
    {
        store = w.store;
        dimv = w.dimv;
        cp = storage_traits<Store>::data(store);
        return *this;
    }
    constexpr Container(Container && w): store(std::move(w.store))
    {
        dimv = std::move(w.dimv);
        cp = storage_traits<Store>::data(store);
    }
    constexpr Container(Container const & w): store(w.store)
    {
        dimv = w.dimv;
        cp = storage_traits<Store>::data(store);
    }
#define RA_ASSIGNOPS(OP)                                                \
    constexpr decltype(auto) operator OP(T const & x) { iter() OP ra::scalar(x); return *this; } /* [ra44] */ \
    constexpr decltype(auto) operator OP(auto const & x) { iter() OP x; return *this; } \
    constexpr decltype(auto) operator OP(Iterator auto && x) { iter() OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <int N> constexpr Container & operator=(T (&&x)[N]) requires (1!=R) { view() = RA_FW(x); return *this; }
    constexpr Container & operator=(braces<T, R> x) requires (R>=1) { view() = x; return *this; }
#define RA_BRACES(N) constexpr Container & operator=(braces<T, N> x) requires (ANY==R) { view() = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES
// FIXME skip ravel-through-iterator if rank is 1.
    constexpr static auto zdims = std::apply([](auto ... i){ return std::array<Dim, (R==ANY)?1:R>{Dim{i*0, 1} ... }; }, mp::iota<(R==ANY)?1:R>{});
    constexpr Container(): View(zdims, nullptr) {}
    constexpr Container() requires (R==0): Container({}, none) {}
    constexpr Container(auto const & x): Container(ra::shape(x), ra::none) { view() = x; }
    constexpr Container(braces<T, R> x) requires (R>=1): Container(braces_shape<T, R>(x), ra::none) { view() = x; }
#define RA_BRACES(N) constexpr Container(braces<T, N> x) requires (R==ANY): Container(braces_shape<T, N>(x), ra::none) { view() = x; }
    RA_FE(RA_BRACES, 1, 2, 3, 4)
#undef RA_BRACES
// FIXME replace shape + row-major ravel by explicit it-is-ravel mark, also iter<n> initializers.
    constexpr Container(auto const & s, none_t)
    {
        static_assert(!std::is_convertible_v<value_t<decltype(s)>, Dim>);
        RA_CK(0==ra::rank(s) || 1==ra::rank(s), "Rank mismatch for init shape ", ra::rank(s), ".");
        static_assert(ANY==R || ANY==size_s(s) || R==size_s(s) || UNB==size_s(s), "Bad shape for rank.");
        store = storage_traits<Store>::create(filldimv(ra::iter(s), dimv));
        cp = storage_traits<Store>::data(store);
    }
    constexpr Container(auto const & s, auto const & x): Container(s, none) { view() = x; }
    constexpr Container(auto const & s, std::initializer_list<T> x): Container(s, none) { to_ravel(x, *this); }
// sharg overloads handle {...} arguments.
    using sharg = decltype(shape(std::declval<View>()));
    constexpr Container(sharg const & s, none_t): Container(ra::iter(s), ra::none) {}
    constexpr Container(sharg const & s, auto const & x): Container(ra::iter(s), x) {}
    constexpr Container(sharg const & s, std::initializer_list<T> x): Container(ra::iter(s), x) {}
// FIXME maybe remove, cf to_ravel().
    constexpr Container(sharg const & s, auto * p): Container(ra::iter(s), none) { std::ranges::copy_n(p, size(), begin()); }

// resize first axis or full shape. Only for some kinds of store.
    constexpr void resize(dim_t const s)
    {
        if constexpr (ANY==R) { RA_CK(0<rank()); } else { static_assert(0<=R); }
        dimv[0].len = s; store.resize(size()); cp = store.data();
    }
    constexpr void resize(dim_t const s, T const & t)
    {
        static_assert(ANY==R || 0<R); RA_CK(0<rank());
        dimv[0].len = s; store.resize(size(), t); cp = store.data();
    }
    constexpr void resize(auto const & s) requires (1==rank_s(s))
    {
        store.resize(filldimv(ra::iter(s), dimv)); cp = store.data();
    }
// auto && + RA_FW wouldn't work for push_back(brace-enclosed-list). p1219??
    constexpr void push_back(T && t)
    {
        if constexpr (ANY==R) { RA_CK(1==rank()); } else { static_assert(1==R); }
        store.push_back(std::move(t)); ++dimv[0].len; cp = store.data();
    }
    constexpr void push_back(T const & t)
    {
        if constexpr (ANY==R) { RA_CK(1==rank()); } else { static_assert(1==R); }
        store.push_back(t); ++dimv[0].len; cp = store.data();
    }
    constexpr void emplace_back(auto && ... a)
    {
        if constexpr (ANY==R) { RA_CK(1==rank()); } else { static_assert(1==R); }
        store.emplace_back(RA_FW(a) ...); ++dimv[0].len; cp = store.data();
    }
    constexpr void pop_back()
    {
        if constexpr (ANY==R) { RA_CK(1==rank()); } else { static_assert(1==R); }
        RA_CK(0<dimv[0].len, "Empty array pop_back().");
        --dimv[0].len; store.pop_back();
    }
    constexpr decltype(auto) back(this auto && self) { return RA_FW(self).view().back(); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) at(this auto && self, auto const & i) { return RA_FW(self).view().at(i); }
    constexpr auto begin(this auto && self) { assert(c_order(self.view().dimv)); return self.view().data(); }
    constexpr auto end(this auto && self) { return self.view().data()+self.size(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { if constexpr (1==R && 0==c) return ptr(self.data(), self.size()); else return RA_FW(self).view().template iter<c>(); }
    constexpr auto iter(this auto && self, rank_t c) { return RA_FW(self).view().iter(c); }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
};

// rely on std::swap; else ambiguous
template <class Store, rank_t RA, rank_t RB> requires (RA!=RB)
void
swap(Container<Store, RA> & a, Container<Store, RB> & b)
{
    if constexpr ((ANY==RA)==(ANY==RB)) {
        static_assert(RA==RB);
        std::swap(a.dimv, b.dimv);
    } else {
        RA_CK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " ", rank(b), ".");
        auto [xa, xb] = [&](){ if constexpr (ANY==RA) return std::tie(a, b); else return std::tie(b, a); }();
        decltype(xb.dimv) c;
        ra::iter(c) = xa.dimv;
        ra::iter(xa.dimv) = xb.dimv;
        std::swap(xb.dimv, c);
    }
    std::swap(a.store, b.store);
    std::swap(a.cp, b.cp);
}

template <class T, rank_t R=ANY> using Big = Container<vector_default_init<T>, R>;
template <class T, rank_t R=ANY> using Unique = Container<std::unique_ptr<T []>, R>;
template <class T, rank_t R=ANY> using Shared = Container<std::shared_ptr<T>, R>;

// In Guile wrappers to either borrow from Guile storage or convert into new array (eg 'f32 to 'f64).
// TODO Can use unique_ptr's deleter for this?
// TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args

template <rank_t R, class T>
Shared<T, R>
shared_borrowing(ViewBig<T *, R> & raw)
{
    Shared<T, R> a;
    a.dimv = raw.dimv;
    a.cp = raw.cp;
    a.store = std::shared_ptr<T>(raw.data(), [](T *){});
    return a;
}

#define RA_TENSORINDEX(w) constexpr auto RA_JOIN(_, w) = iota<w>();
RA_FE(RA_TENSORINDEX, 0, 1, 2, 3, 4);
#undef RA_TENSORINDEX


// --------------------
// Container if rank>0, else value type, even unregistered. FIXME concrete-or-view.
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
with_shape(std::initializer_list<S> && s, X && x) { return with_shape<E>(ra::iter(s), RA_FW(x)); }


// --------------------
// View ops. FIXME Slice && takes Container & / View & / View &&, but Container && is a risk.
// --------------------

template <class K=ic_t<0>>
constexpr auto
reverse(Slice auto && a, K k = K {})
{
    if constexpr (is_ptr<decltype(a)>) {
        static_assert(UNB!=a.len_s(0), "Bad arguments to reverse.");
        return ptr(Seq { a.data().i+(a.dimv[0].len-1)*a.dimv[0].step }, a.dimv[0].len, csub(ic<0>, a.dimv[0].step));
    } else if constexpr (is_ctype<typename std::decay_t<decltype(a)>::Dimv>) {
        constexpr auto dimv = [&]{
            RA_CK(inside(k, a.rank()), "Bad axis ", k, " for rank ", a.rank(), ".");
            auto dimv = a.dimv;
            dimv[k].step *= -1;
            return dimv;
        }();
        return ViewSmall<decltype(a.data()), ic_t<dimv>>(a.data() + (0==dimv[k].len ? 0 : dimv[k].step*(1-dimv[k].len)));
    } else {
        RA_CK(inside(k, a.rank()), "Bad axis ", k, " for rank ", a.rank(), ".");
        ViewBig<decltype(a.data()), rank_s(a)> r = a;
        auto & dim = r.dimv[k];
        dim.step *= -1;
        r.cp += 0==dim.len ? 0 : dim.step*(1-dim.len);
        return r;
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

// static transposed axes list, output rank is static.
template <int ... I>
constexpr auto
transpose(Slice auto && a, ilist_t<I ...>)
{
    constexpr std::array<dim_t, sizeof...(I)> s = { I ... };
    constexpr rank_t rank = 0==ra::size(s) ? 0 : 1+std::ranges::max(s);
    if constexpr (is_ctype<typename std::decay_t<decltype(a)>::Dimv>) {
        constexpr auto r = [&]{ std::array<Dim, rank> r; transpose_dims(s, a.dimv, r); return r; }();
        return ViewSmall<decltype(a.data()), ic_t<r>>(a.data());
    } else {
        ViewBig<decltype(a.data()), rank> r(a.data());
        transpose_dims(s, a.dimv, r.dimv);
        return r;
    }
}

// dynamic transposed axes list, output rank is dynamic.
constexpr auto
transpose(Slice auto && a, auto && s)
{
    rank_t rank = 0==ra::size(s) ? 0 : 1+std::ranges::max(s); // no amax() yet
    ViewBig<decltype(a.data()), ANY> r { decltype(r.dimv)(rank), a.data() };
    transpose_dims(s, a.dimv, r.dimv);
    return r;
}

template <class T, int N> constexpr auto transpose(Slice auto && a, T (&&s)[N]) { return transpose(RA_FW(a), ra::iter(s)); }
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
ravel_free(Slice auto && a)
{
    RA_CK(c_order(a.dimv, false), "View cannot be raveled.");
    int r = a.rank()-1;
    for (; r>=0 && a.len(r)==1; --r) {}
    return ViewBig<decltype(a.data()), 1>({{ra::size(a), r<0 ? 1 : a.step(r)}}, a.cp);
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
    ViewBig<decltype(a.data()), ra::size_s(sb)> b(map([](auto i){ return Dim { UNB, 0 }; }, ra::iota(ra::size(sb))), a.data());
    rank_t i = 0;
    for (; i<a.rank() && i<b.rank(); ++i) {
        if (sa[a.rank()-i-1]!=sb[b.rank()-i-1]) {
            RA_CK(c_order(a.dimv, false) && la>=lb, "Reshape with copy not implemented.");
// FIXME ViewBig(SS const & s, T * p). Cf [ra37].
            filldimv(ra::iter(sb), b.dimv);
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

template <class T, int N> constexpr auto reshape(Slice auto && a, T (&&s)[N]) { return reshape(RA_FW(a), ra::iter(s)); }

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
        return ViewSmall<sup_t *, ic_t<bdimv>>(reinterpret_cast<sup_t *>(a.data()));
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
