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


// ---------------------
// Small view and container.
// ---------------------

// cf braces_def for big views.

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
// row-major ravel braces
    constexpr ViewSmall const &
    operator=(T (&&x)[have_braces ? size() : 0]) const
        requires (have_braces && rank()>1 && size()>1)
    {
        std::ranges::copy(std::ranges::subrange(x), begin()); return *this;
    }
    using sub = typename nested_arg<T, Dimv>::sub;
// nested braces
    constexpr ViewSmall const &
    operator=(sub (&&x)[have_braces ? (rank()>0 ? len(0) : 0) : 0]) const
        requires (have_braces && 0<rank() && 0<len(0) && (1!=rank() || 1!=len(0)))
    {
        ra::iter<-1>(*this) = x;
        if !consteval { asm volatile("" ::: "memory"); } // patch for [ra01]
        return *this;
    }
// T not is_scalar [ra44]
    constexpr ViewSmall const & operator=(T const & t) const { ra::iter(*this) = ra::scalar(t); return *this; }
// cf RA_ASSIGNOPS_ITER [ra38][ra34]
    ViewSmall const & operator=(ViewSmall const & x) const { ra::iter(*this) = x; return *this; }
#define RA_ASSIGNOPS(OP)                                                   \
    constexpr ViewSmall const & operator OP(auto const & x) const { ra::iter(*this) OP x; return *this; } \
    constexpr ViewSmall const & operator OP(Iterator auto && x) const { ra::iter(*this) OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <dim_t s, dim_t o=0> constexpr auto as() const { return from(*this, ra::iota(ic<s>, o)); }
    template <rank_t c=0> constexpr auto iter() const { return Cell<P, ic_t<dimv>, ic_t<c>>(cp); }
    constexpr auto iter(rank_t c) const { return Cell<P, decltype(dimv) const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { if constexpr (c_order(dimv)) return cp; else return STLIterator(iter()); }
    constexpr auto end() const requires (c_order(dimv)) { return cp+size(); }
    constexpr static auto end() requires (!c_order(dimv)) { return std::default_sentinel; }
    constexpr decltype(auto) back() const { static_assert(size()>0, "Bad back()."); return cp[size()-1]; }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return *indexer(*this, cp, ra::iter(i)); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
// conversion to const
    constexpr operator ViewSmall<reconst<P>, Dimv> () const requires (!std::is_void_v<reconst<P>>)
    {
        return ViewSmall<reconst<P>, Dimv>(cp);
    }
};

#define RA_TENSORINDEX(w) constexpr auto RA_JOIN(_, w) = iota<w>();
RA_FE(RA_TENSORINDEX, 0, 1, 2, 3, 4);
#undef RA_TENSORINDEX

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
#if RA_OPT_SMALL==1
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
        requires ((0<rank() && 0<len(0) && (1!=rank() || 1!=len(0))))
    {
        view() = { x ... };
    }
    constexpr SmallArray(auto const & x) { view() = x; }
    constexpr SmallArray(Iterator auto && x) { view() = RA_FW(x); }
#define RA_ASSIGNOPS(OP)                                                \
    constexpr SmallArray & operator OP(auto const & x) { view() OP x; return *this; } \
    constexpr SmallArray & operator OP(Iterator auto && x) { view() OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <int s, int o=0> constexpr decltype(auto) as(this auto && self) { return RA_FW(self).view().template as<s, o>(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { return RA_FW(self).view().template iter<c>(); }
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


// --------------------
// Big view and containers.
// --------------------

// cf small_args. FIXME Let any expr = braces.

template <class T, rank_t r> struct braces_ { using type = noarg; };
template <class T, rank_t r> using braces = braces_<T, r>::type;
template <class T, rank_t r> requires (r==1) struct braces_<T, r> { using type = std::initializer_list<T>; };
template <class T, rank_t r> requires (r>1) struct braces_<T, r> { using type = std::initializer_list<braces<T, r-1>>; };

template <int i, class T, rank_t r>
constexpr dim_t
braces_len(braces<T, r> const & l)
{
    if constexpr (i>=r) {
        return 0;
    } else if constexpr (i==0) {
        return l.size();
    } else {
        return braces_len<i-1, T, r-1>(*(l.begin()));
    }
}

template <class T, rank_t r>
constexpr auto
braces_shape(braces<T, r> const & l)
{
    return [&l]<int ... i>(ilist_t<i ...>){ return std::array { braces_len<i, T, r>(l) ... }; }(mp::iota<r>{});
}

// FIXME avoid duplicating cp, dimv from Container without being parent. Parameterize on Dimv, like Cell.
// FIXME constructor checks (lens>=0, steps inside, etc.).
template <class P, rank_t RANK>
struct ViewBig
{
    using Dimv = std::conditional_t<ANY==RANK, vector_default_init<Dim>, Small<Dim, ANY==RANK ? 0 : RANK>>;
    [[no_unique_address]] Dimv dimv;
    P cp;

    consteval static rank_t rank() requires (RANK!=ANY) { return RANK; }
    constexpr rank_t rank() const requires (RANK==ANY) { return rank_t(dimv.size()); }
    constexpr static dim_t len_s(int k) { return ANY; }
    constexpr dim_t len(int k) const { return dimv[k].len; }
    constexpr dim_t step(int k) const { return dimv[k].step; }
    constexpr auto data() const { return cp; }
    constexpr dim_t size() const { return ra::size(*this); }
    constexpr bool empty() const { return any(0==map(&Dim::len, dimv)); }

    constexpr ViewBig() {} // used by Container constructors
    constexpr explicit ViewBig(P cp_): cp(cp_) {} // used by slicers, esp. when P has_len
    constexpr ViewBig(ViewBig const &) = default;
    constexpr ViewBig(Dimv const & dimv_, P cp_): dimv(dimv_), cp(cp_) {} // [ra36]
    constexpr ViewBig(auto const & s, P cp_): cp(cp_)
    {
        if constexpr (std::is_convertible_v<value_t<decltype(s)>, Dim>) {
            ra::resize(dimv, ra::size(s)); // [ra37]
            ra::iter(dimv) = s;
        } else {
            filldimv(ra::iter(s), dimv);
        }
    }
    constexpr ViewBig(std::initializer_list<dim_t> s, P cp_): ViewBig(ra::iter(s), cp_) {}
    using T = std::remove_reference_t<decltype(*std::declval<P>())>;
// row-major ravel braces
    constexpr ViewBig const &
    operator=(std::initializer_list<T> const x) const requires (1!=RANK)
    {
        auto xsize = ssize(x);
        RA_CK(size()==xsize, "Mismatched sizes ", size(), " ", xsize, ".");
        std::ranges::copy_n(x.begin(), xsize, begin());
        return *this;
    }
// nested braces
    constexpr ViewBig const &
    operator=(braces<T, RANK> x) const requires (RANK!=ANY) { ra::iter<-1>(*this) = x; return *this; }
#define RA_BRACES(N)                                                    \
    constexpr ViewBig const &                                           \
    operator=(braces<T, N> x) const requires (RANK==ANY) { ra::iter<-1>(*this) = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES
// T not is_scalar [ra44]
    constexpr ViewBig const & operator=(T const & t) const { ra::iter(*this) = ra::scalar(t); return *this; }
// cf RA_ASSIGNOPS_ITER [ra38][ra34]
    ViewBig const & operator=(ViewBig const & x) const { ra::iter(*this) = x; return *this; }
#define RA_ASSIGNOPS(OP)                                                \
    constexpr ViewBig const & operator OP (auto const & x) const { ra::iter(*this) OP x; return *this; } \
    constexpr ViewBig const & operator OP (Iterator auto && x) const { ra::iter(*this) OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    template <rank_t c=0> constexpr auto iter() const && { return Cell<P, Dimv, ic_t<c>>(cp, std::move(dimv)); }
    template <rank_t c=0> constexpr auto iter() const & { return Cell<P, Dimv const &, ic_t<c>>(cp, dimv); }
    constexpr auto iter(rank_t c) const && { return Cell<P, Dimv, dim_t>(cp, std::move(dimv), c); }
    constexpr auto iter(rank_t c) const & { return Cell<P, Dimv const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { return STLIterator(iter<0>()); }
    constexpr decltype(auto) static end() { return std::default_sentinel; }
    constexpr decltype(auto) back() const { dim_t s=size(); RA_CK(s>0, "Bad back()."); return cp[s-1]; }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return *indexer(*this, cp, ra::iter(i)); }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
// conversion to const, used by Container::view(). FIXME cf Small
    constexpr operator ViewBig<reconst<P>, RANK> const & () const requires (!std::is_void_v<reconst<P>>)
    {
        return *reinterpret_cast<ViewBig<reconst<P>, RANK> const *>(this);
    }
// conversions from var rank to fixed rank
    template <rank_t R> requires (R==ANY && R!=RANK)
    constexpr ViewBig(ViewBig<P, R> const & x): dimv(x.dimv), cp(x.cp) {}
    template <rank_t R> requires (R==ANY && R!=RANK && !std::is_void_v<unconst<P>>)
    constexpr ViewBig(ViewBig<unconst<P>, R> const & x): dimv(x.dimv), cp(x.cp) {}
// conversions from fixed rank to var rank
    template <rank_t R> requires (R!=ANY && RANK==ANY)
    constexpr ViewBig(ViewBig<P, R> const & x): dimv(x.dimv.begin(), x.dimv.end()), cp(x.cp) {}
    template <rank_t R> requires (R!=ANY && RANK==ANY && !std::is_void_v<unconst<P>>)
    constexpr ViewBig(ViewBig<unconst<P>, R> const & x): dimv(x.dimv.begin(), x.dimv.end()), cp(x.cp) {}
};

template <class V>
struct storage_traits
{
    using T = V::value_type;
    static_assert(!std::is_same_v<std::remove_const_t<T>, bool>, "No pointers to bool in std::vector<bool>.");
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
template <class Store, rank_t RANK>
struct Container: public ViewBig<typename storage_traits<Store>::T *, RANK>
{
    Store store;
    constexpr auto data(this auto && self) { return self.view().data(); }
    using T = typename storage_traits<Store>::T;
    using View = ViewBig<T *, RANK>;
    using ViewConst = ViewBig<T const *, RANK>;
    constexpr ViewConst const & view() const { return *this; }
    constexpr View & view() { return *this; }
    using View::size, View::rank, View::dimv, View::cp;

// A(shape 2 3) = A-type [1 2 3] initializes, so it doesn't behave as A(shape 2 3) = not-A-type [1 2 3] which uses View::operator=. This is used by operator>>(std::istream &, Container &). See test/ownership.cc [ra20].
// TODO don't require copyable T in constructors, see fill1. That requires operator= to initialize, not update.
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
    constexpr Container & operator OP (auto const & x) { view() OP x; return *this; } \
    constexpr Container & operator OP (Iterator auto && x) { view() OP RA_FW(x); return *this; }
    RA_FE(RA_ASSIGNOPS, =, *=, +=, -=, /=)
#undef RA_ASSIGNOPS
    constexpr Container & operator=(std::initializer_list<T> const x) requires (1!=RANK) { view() = x; return *this; }
    constexpr Container & operator=(braces<T, RANK> x) requires (ANY!=RANK) { view() = x; return *this; }
#define RA_BRACES(N)                                                    \
    constexpr Container & operator=(braces<T, N> x) requires (ANY==RANK) { view() = x; return *this; }
    RA_FE(RA_BRACES, 2, 3, 4);
#undef RA_BRACES

    constexpr void
    init(auto const & s) requires (1==rank_s(s) || ANY==rank_s(s))
    {
        static_assert(!std::is_convertible_v<value_t<decltype(s)>, Dim>);
        RA_CK(1==ra::rank(s), "Rank mismatch for init shape.");
        static_assert(ANY==RANK || ANY==size_s(s) || RANK==size_s(s) || UNB==size_s(s), "Bad shape for rank.");
        store = storage_traits<Store>::create(filldimv(ra::iter(s), dimv));
        cp = storage_traits<Store>::data(store);
    }
    constexpr void init(dim_t s) { init(std::array {s}); } // scalar allowed as shape if rank is 1.

// provided so that {} calls sharg constructor below.
    constexpr Container() requires (ANY==RANK): View({ Dim {0, 1} }, nullptr) {}
    constexpr Container() requires (ANY!=RANK && 0!=RANK): View(typename View::Dimv(Dim {0, 1}), nullptr) {}
    constexpr Container() requires (0==RANK): Container({}, none) {}

    using sharg = decltype(shape(std::declval<View>().iter()));
// sharg overloads handle {...} arguments. Size check is at conversion if sharg is Small.
    constexpr Container(sharg const & s, none_t) { init(s); }
    constexpr Container(sharg const & s, auto && x): Container(s, none) { view() = x; }
    constexpr Container(sharg const & s, braces<T, RANK> x) requires (1==RANK): Container(s, none) { view() = x; }

    constexpr Container(auto const & x): Container(ra::shape(x), none) { view() = x; }
    constexpr Container(braces<T, RANK> x) requires (RANK!=ANY): Container(braces_shape<T, RANK>(x), none) { view() = x; }
#define RA_BRACES(N)                                                    \
    constexpr Container(braces<T, N> x) requires (RANK==ANY): Container(braces_shape<T, N>(x), none) { view() = x; }
    RA_FE(RA_BRACES, 1, 2, 3, 4)
#undef RA_BRACES

// FIXME requires copyable Y, which conflicts with semantics of view_.operator=. store(x) avoids the conflict for Big, but not for Unique. Should construct in place like std::vector.
    constexpr void
    fill1(auto && xbegin, dim_t xsize)
    {
        RA_CK(size()==xsize, "Mismatched sizes ", size(), " ", xsize, ".");
        std::ranges::copy_n(RA_FW(xbegin), xsize, begin());
    }

// shape + row-major ravel.
// FIXME explicit it-is-ravel mark. Also iter<n> initializers.
// FIXME regular (no need for fill1) for ANY if rank is 1.
    constexpr Container(sharg const & s, std::initializer_list<T> x) requires (1!=RANK): Container(s, none) { fill1(x.begin(), x.size()); }
// FIXME remove these two
    constexpr Container(sharg const & s, auto * p): Container(s, none) { fill1(p, size()); } // FIXME fake check
    constexpr Container(sharg const & s, auto && pbegin, dim_t psize): Container(s, none) { fill1(RA_FW(pbegin), psize); }
// for shape arguments that doesn't convert implicitly to sharg
    constexpr Container(auto const & s, none_t) { init(s); }
    constexpr Container(auto const & s, auto const & x): Container(s, none) { view() = x; }
    constexpr Container(auto const & s, std::initializer_list<T> x): Container(s, none) { fill1(x.begin(), x.size()); }

// resize first axis or full shape. Only for some kinds of store.
    constexpr void resize(dim_t const s)
    {
        if constexpr (ANY==RANK) { RA_CK(0<rank()); } else { static_assert(0<=RANK); }
        dimv[0].len = s;
        store.resize(size()); cp = store.data();
    }
    constexpr void resize(dim_t const s, T const & t)
    {
        static_assert(ANY==RANK || 0<RANK); RA_CK(0<rank());
        dimv[0].len = s;
        store.resize(size(), t); cp = store.data();
    }
    constexpr void resize(auto const & s) requires (1==rank_s(s))
    {
        store.resize(filldimv(ra::iter(s), dimv)); cp = store.data();
    }
// template + RA_FW wouldn't work for push_back(brace-enclosed-list).
    constexpr void push_back(T && t)
    {
        if constexpr (ANY==RANK) { RA_CK(1==rank()); } else { static_assert(1==RANK); }
        store.push_back(std::move(t));
        ++dimv[0].len; cp = store.data();
    }
    constexpr void push_back(T const & t)
    {
        if constexpr (ANY==RANK) { RA_CK(1==rank()); } else { static_assert(1==RANK); }
        store.push_back(t);
        ++dimv[0].len; cp = store.data();
    }
    constexpr void emplace_back(auto && ... a)
    {
        if constexpr (ANY==RANK) { RA_CK(1==rank()); } else { static_assert(1==RANK); }
        store.emplace_back(RA_FW(a) ...);
        ++dimv[0].len; cp = store.data();
    }
    constexpr void pop_back()
    {
        if constexpr (ANY==RANK) { RA_CK(1==rank()); } else { static_assert(1==RANK); }
        RA_CK(0<dimv[0].len, "Empty array pop_back().");
        --dimv[0].len;
        store.pop_back();
    }
    constexpr decltype(auto) back(this auto && self) { return RA_FW(self).view().back(); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return RA_FW(self).view()(RA_FW(i) ...); }
    constexpr decltype(auto) at(this auto && self, auto const & i) { return RA_FW(self).view().at(i); }
    constexpr auto begin(this auto && self) { assert(c_order(self.view().dimv)); return self.view().data(); }
    constexpr auto end(this auto && self) { return self.view().data()+self.size(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { if constexpr (1==RANK && 0==c) { return ptr(self.data(), self.size()); } else { return RA_FW(self).view().template iter<c>(); } }
    constexpr auto iter(this auto && self, rank_t c) { return RA_FW(self).view().iter(c); }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
};

// rely on std::swap; else ambiguous
template <class Store, rank_t RANKA, rank_t RANKB> requires (RANKA!=RANKB)
void
swap(Container<Store, RANKA> & a, Container<Store, RANKB> & b)
{
    if constexpr (ANY==RANKA) {
        RA_CK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " and ", rank(b), ".");
        decltype(b.dimv) c = a.dimv;
        ra::iter(a.dimv) = b.dimv;
        std::swap(b.dimv, c);
    } else if constexpr (ANY==RANKB) {
        RA_CK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " and ", rank(b), ".");
        decltype(a.dimv) c = b.dimv;
        ra::iter(b.dimv) = a.dimv;
        std::swap(a.dimv, c);
    } else {
        static_assert(RANKA==RANKB);
        std::swap(a.dimv, b.dimv);
    }
    std::swap(a.store, b.store);
    std::swap(a.cp, b.cp);
}

template <class T, rank_t RANK=ANY> using Big = Container<vector_default_init<T>, RANK>;
template <class T, rank_t RANK=ANY> using Unique = Container<std::unique_ptr<T []>, RANK>;
template <class T, rank_t RANK=ANY> using Shared = Container<std::shared_ptr<T>, RANK>;

// In Guile wrappers to either borrow from Guile storage or convert into new array (eg 'f32 to 'f64).
// TODO Can use unique_ptr's deleter for this?
// TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args

template <rank_t RANK, class T>
Shared<T, RANK>
shared_borrowing(ViewBig<T *, RANK> & raw)
{
    Shared<T, RANK> a;
    a.dimv = raw.dimv;
    a.cp = raw.cp;
    a.store = std::shared_ptr<T>(raw.data(), [](T *){});
    return a;
}


// --------------------
// Container if rank>0, else value type, even unregistered. FIXME concrete-or-view.
// --------------------

template <class E> struct concrete_type_;

template <class E> requires (0==rank_s<E>())
struct concrete_type_<E> { using type = ncvalue_t<E>; };

template <class E> requires (ANY==size_s<E>())
struct concrete_type_<E> { using type = Big<ncvalue_t<E>, rank_s<E>()>; };

template <class E> requires (0!=rank_s<E>() && ANY!=size_s<E>())
struct concrete_type_<E> { using type = SmallArray<ncvalue_t<E>, ic_t<default_dims(shape_s<E>)>>; };

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
// View ops.
// --------------------

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

template <class P, rank_t RANK>
constexpr auto
reverse(ViewBig<P, RANK> const & view, int k=0)
{
    RA_CK(inside(k, view.rank()), "Bad axis ", k, " for rank ", view.rank(), ".");
    ViewBig<P, RANK> r = view;
    auto & dim = r.dimv[k];
    dim.step *= -1;
    if (dim.len!=0) { r.cp += dim.step*(1-dim.len); }
    return r;
}

// FIXME Merge transpose & Reframe (beat reframe(view) into transpose(view)).
constexpr void
transpose_dims(auto const & s, auto const & src, auto & dst)
{
    std::ranges::fill(dst, Dim {UNB, 0});
    for (int k=0; int sk: s) {
        dst[sk].step += src[k].step;
        dst[sk].len = dst[sk].len>=0 ? std::min(dst[sk].len, src[k].len) : src[k].len;
        ++k;
    }
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

// static transposed axes list, output rank is static.
template <int ... I, class P, rank_t RANK>
constexpr auto
transpose(ViewBig<P, RANK> const & view, ilist_t<I ...>)
{
    if constexpr (ANY==RANK) {
        RA_CK(view.rank()==sizeof...(I), "Bad rank ", view.rank(), " for ", sizeof...(I), " axes.");
    } else {
        static_assert(ANY==RANK || RANK==sizeof...(I), "Bad rank."); // c++26
    }
    constexpr std::array<dim_t, sizeof...(I)> s = { I ... };
    constexpr rank_t dstrank = 0==ra::size(s) ? 0 : 1 + std::ranges::max(s);
    ViewBig<P, dstrank> r;
    r.cp = view.data();
    transpose_dims(s, view.dimv, r.dimv);
    return r;
}

// dynamic transposed axes list, output rank is dynamic. FIXME only some S are valid here.
template <class P, rank_t RANK, class S>
constexpr ViewBig<P, ANY>
transpose(ViewBig<P, RANK> const & view, S && s)
{
    RA_CK(view.rank()==ra::size(s), "Bad rank ",  view.rank(), " for ", ra::size(s), " axes.");
    rank_t dstrank = 0==ra::size(s) ? 0 : 1 + std::ranges::max(s); // FIXME amax(), but that's in ra.hh
    ViewBig<P, ANY> r { decltype(r.dimv)(dstrank), view.data() };
    transpose_dims(s, view.dimv, r.dimv);
    return r;
}

template <class P, rank_t RANK, class dimtype, int N>
constexpr ViewBig<P, ANY>
transpose(ViewBig<P, RANK> const & view, dimtype const (&s)[N])
{
    return transpose(view, ra::iter(s));
}

constexpr decltype(auto)
transpose(auto && a) { return transpose(RA_FW(a), ilist<1, 0>); }

constexpr decltype(auto)
diag(auto && a) { return transpose(RA_FW(a), ilist<0, 0>); };

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

template <class sup_t, class T, rank_t RANK>
constexpr auto
explode(ViewBig<T *, RANK> const & a)
{
    constexpr static rank_t ru = sizeof(value_t<sup_t>)==sizeof(value_t<decltype(a)>) ? 0 : 1;
    ViewBig<sup_t *, rank_sum(RANK, -rank_s<sup_t>()-ru)> b;
    ra::resize(b.dimv, a.rank()-rank_s<sup_t>()-ru);
    explode_dims<sup_t, T>(a.dimv, b.dimv);
    b.cp = reinterpret_cast<sup_t *>(a.data());
    return b;
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

template <class P, rank_t RANK>
constexpr ViewBig<P, 1>
ravel_free(ViewBig<P, RANK> const & a)
{
    RA_CK(c_order(a.dimv, false));
    int r = a.rank()-1;
    for (; r>=0 && a.len(r)==1; --r) {}
    ra::dim_t s = r<0 ? 1 : a.step(r);
    return ViewBig<P, 1>({{ra::size(a), s}}, a.cp);
}

template <class P, rank_t RANK, class S>
inline auto
reshape(ViewBig<P, RANK> const & a, S && sb_)
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
    ViewBig<P, ra::size_s(sb)> b(map([](auto i){ return Dim { UNB, 0 }; }, ra::iota(ra::size(sb))), a.data());
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
        } else {
// select
            b.dimv[b.rank()-i-1] = a.dimv[a.rank()-i-1];
        }
    }
    if (i==a.rank()) {
// tile
        for (rank_t j=i; j<b.rank(); ++j) {
            b.dimv[b.rank()-j-1] = { sb[b.rank()-j-1], 0 };
        }
    }
    return b;
}

// We need dimtype bc {1, ...} deduces to int and that fails to match ra::dim_t. initializer_list could handle the general case, but the result would have var rank and override this one (?).
template <class P, rank_t RANK, class dimtype, int N>
constexpr auto
reshape(ViewBig<P, RANK> const & a, dimtype const (&s)[N])
{
    return reshape(a, ra::iter(s));
}

// lo: lower bounds, hi: upper bounds. The stencil indices are in [0 lo+1+hi] = [-lo +hi].
template <class LO, class HI, class P, rank_t N>
inline ViewBig<P, rank_sum(N, N)>
stencil(ViewBig<P, N> const & a, LO && lo, HI && hi)
{
    ViewBig<P, rank_sum(N, N)> s;
    s.cp = a.data();
    ra::resize(s.dimv, 2*a.rank());
    RA_CK(every(lo>=0) && every(hi>=0), "Bad stencil bounds lo ", fmt(nstyle, lo), " hi ", fmt(nstyle, hi), ".");
    for_each([](auto & dims, auto && dima, auto && lo, auto && hi){
                 RA_CK(dima.len>=lo+hi, "Stencil is too large for array.");
                 dims = { dima.len-lo-hi, dima.step };
             },
             ptr(s.dimv.data()), a.dimv, lo, hi);
    for_each([](auto & dims, auto && dima, auto && lo, auto && hi) {
                 dims = { lo+hi+1, dima.step };
             },
             ptr(s.dimv.data()+a.rank()), a.dimv, lo, hi);
    return s;
}

// TODO Check that ranks below SUBR are compact. Version for Small.
template <class sub_t, class sup_t, rank_t RANK>
constexpr auto
collapse(ViewBig<sup_t *, RANK> const & a)
{
    using super_v = value_t<sup_t>;
    using sub_v = value_t<sub_t>;
    constexpr int subtype = sizeof(super_v)/sizeof(sub_t);
    constexpr int SUBR = rank_s<sup_t>() - rank_s<sub_t>();
    static auto gstep = [](int i){ if constexpr (is_scalar<sup_t>) return 1; else return sup_t::step(i); };
    static auto glen = [](int i){ if constexpr (is_scalar<sup_t>) return 1; else return sup_t::len(i); };
    ViewBig<sub_t *, rank_sum(RANK, SUBR+int(subtype>1))> b;
    resize(b.dimv, a.rank()+SUBR+int(subtype>1));
    constexpr dim_t r = sizeof(sup_t)/sizeof(sub_t);
    static_assert(sizeof(sup_t)==r*sizeof(sub_t), "Cannot make axis of sup_t from sub_t.");
    for (int i=0; i<a.rank(); ++i) {
        b.dimv[i] = Dim { a.len(i), a.step(i)*r };
    }
    constexpr dim_t t = sizeof(super_v)/sizeof(sub_v);
    constexpr dim_t s = sizeof(sub_t)/sizeof(sub_v);
    static_assert(t*sizeof(sub_v)>=1, "Bad subtype.");
    for (int i=0; i<SUBR; ++i) {
        dim_t step = gstep(i);
        RA_CK(0==step % s, "Bad steps.");
        b.dimv[a.rank()+i] = Dim { glen(i), step/s*t };
    }
    if (subtype>1) {
        b.dimv[a.rank()+SUBR] = Dim { t, 1 };
    }
    b.cp = reinterpret_cast<sub_t *>(a.data());
    return b;
}

} // namespace ra
