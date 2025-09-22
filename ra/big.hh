// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Views and containers with dynamic lengths and steps, cf small.hh.

// (c) Daniel Llorens - 2013-2025
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "small.hh"
#include <memory>

namespace ra {


// --------------------
// Big view and containers
// --------------------

// cf small_args in small.hh. FIXME Let any expr = braces.

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

// FIXME avoid duplicating cp, dimv from Container without being parent. Parameterize on Dimv, like Cell.
// FIXME constructor checks (lens>=0, steps inside, etc.).
template <class P, rank_t RANK>
struct ViewBig
{
    using T = std::remove_reference_t<decltype(*std::declval<P>())>;
    using Dimv = std::conditional_t<ANY==RANK, vector_default_init<Dim>, Small<Dim, ANY==RANK ? 0 : RANK>>;
    Dimv dimv;
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
        ra::resize(dimv, ra::size(s)); // [ra37]
        if constexpr (std::is_convertible_v<value_t<decltype(s)>, Dim>) {
            start(dimv) = s;
        } else {
            filldimv(s, dimv);
        }
    }
    constexpr ViewBig(std::initializer_list<dim_t> s, P cp_): ViewBig(start(s), cp_) {}
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
    operator=(braces<T, RANK> x) const requires (RANK!=ANY)
    {
        ra::iter<-1>(*this) = x; return *this;
    }
#define RA_BRACES_ANY(N)                                                \
    constexpr ViewBig const &                                           \
    operator=(braces<T, N> x) const requires (RANK==ANY)                \
    {                                                                   \
        ra::iter<-1>(*this) = x; return *this;                          \
    }
    FOR_EACH(RA_BRACES_ANY, 2, 3, 4);
#undef RA_BRACES_ANY
// T not is_scalar [ra44]
    constexpr ViewBig const & operator=(T const & t) const { start(*this) = ra::scalar(t); return *this; }
// cf RA_ASSIGNOPS_ITER [ra38] [ra34]
    ViewBig const & operator=(ViewBig const & x) const { start(*this) = x; return *this; }
#define ASSIGNOPS(OP)                                                   \
    constexpr ViewBig const & operator OP (Iterator auto && x) const { start(*this) OP RA_FW(x); return *this; } \
    constexpr ViewBig const & operator OP (auto const & x) const { start(*this) OP x; return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS
    template <rank_t c=0> constexpr auto iter() const && { return Cell<P, Dimv, ic_t<c>>(cp, std::move(dimv)); }
    template <rank_t c=0> constexpr auto iter() const & { return Cell<P, Dimv const &, ic_t<c>>(cp, dimv); }
    constexpr auto iter(rank_t c) const && { return Cell<P, Dimv, dim_t>(cp, std::move(dimv), c); }
    constexpr auto iter(rank_t c) const & { return Cell<P, Dimv const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { return STLIterator(iter<0>()); }
    constexpr decltype(auto) static end() { return std::default_sentinel; }
    constexpr decltype(auto) back() const { dim_t s=size(); RA_CK(s>0, "Bad back()."); return cp[s-1]; }
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
// conversion to const, used by Container::view(). FIXME cf Small
    constexpr operator ViewBig<reconst<P>, RANK> const & () const requires (!std::is_void_v<reconst<P>>)
    {
        return *reinterpret_cast<ViewBig<reconst<P>, RANK> const *>(this);
    }
    constexpr operator decltype(*cp) () const { return to_scalar(*this); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... i) { return from(RA_FW(self), RA_FW(i) ...); }
    constexpr decltype(auto) at(auto const & i) const { return at_view(*this, i); }
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
// repeated bc of constness re. ViewBig
#define ASSIGNOPS(OP)                                                   \
    constexpr Container & operator OP (Iterator auto && x) { view() OP RA_FW(x); return *this; } \
    constexpr Container & operator OP (auto const & x) { view() OP x; return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS
    constexpr Container & operator=(std::initializer_list<T> const x) requires (1!=RANK) { view() = x; return *this; }
    constexpr Container & operator=(braces<T, RANK> x) requires (ANY!=RANK) { view() = x; return *this; }
#define RA_BRACES_ANY(N)                                                \
    constexpr Container & operator=(braces<T, N> x) requires (ANY==RANK) { view() = x; return *this; }
    FOR_EACH(RA_BRACES_ANY, 2, 3, 4);
#undef RA_BRACES_ANY

    constexpr void
    init(auto const & s) requires (1==rank_s(s) || ANY==rank_s(s))
    {
        static_assert(!std::is_convertible_v<value_t<decltype(s)>, Dim>);
        RA_CK(1==ra::rank(s), "Rank mismatch for init shape.");
        static_assert(ANY==RANK || ANY==size_s(s) || RANK==size_s(s) || UNB==size_s(s), "Bad shape for rank.");
        ra::resize(dimv, ra::size(s)); // [ra37]
        store = storage_traits<Store>::create(filldimv(s, dimv));
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
#define RA_BRACES_ANY(N)                                                \
    constexpr Container(braces<T, N> x) requires (RANK==ANY): Container(braces_shape<T, N>(x), none) { view() = x; }
    FOR_EACH(RA_BRACES_ANY, 1, 2, 3, 4)
#undef RA_BRACES_ANY

// FIXME requires T to be copiable, which conflicts with the semantics of view_.operator=. store(x) avoids it for Big, but doesn't work for Unique. Should construct in place like std::vector does.
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
        ra::resize(dimv, 0==rank_s(s) ? 1 : ra::size(s));
        store.resize(filldimv(s, dimv)); cp = store.data();
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
    constexpr auto begin(this auto && self) { assert(is_c_order(self.view())); return self.view().data(); }
    constexpr auto end(this auto && self) { return self.view().data()+self.size(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { if constexpr (1==RANK && 0==c) { return ptr(self.data(), self.size()); } else { return RA_FW(self).view().template iter<c>(); } }
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
        start(a.dimv) = b.dimv;
        std::swap(b.dimv, c);
    } else if constexpr (ANY==RANKB) {
        RA_CK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " and ", rank(b), ".");
        decltype(a.dimv) c = b.dimv;
        start(b.dimv) = a.dimv;
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
// container if rank>0, else value type, even unregistered. FIXME concrete-or-view.
// --------------------

template <class E> struct concrete_type_;

template <class E> requires (0==rank_s<E>())
struct concrete_type_<E> { using type = ncvalue_t<E>; };

template <class E> requires (ANY==size_s<E>())
struct concrete_type_<E> { using type = Big<ncvalue_t<E>, rank_s<E>()>; };

template <class E> requires (0!=rank_s<E>() && ANY!=size_s<E>())
struct concrete_type_<E> { using type = SmallArray<ncvalue_t<E>, ic_t<default_dims(shape_s<E>)>>; };

template <class E> using concrete_type = std::conditional_t<(0==rank_s<E>() && !is_ra<E>), std::decay_t<E>,
                                                            typename concrete_type_<E>::type>;

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
with_shape(std::initializer_list<S> && s, X && x) { return with_shape<E>(start(s), RA_FW(x)); }


// --------------------
// Big view ops
// --------------------

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

template <class P, rank_t RANK>
constexpr ViewBig<P, 1>
ravel_free(ViewBig<P, RANK> const & a)
{
    RA_CK(is_c_order(a, false));
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
            RA_CK(is_c_order(a, false) && la>=lb, "Reshape with copy not implemented.");
// FIXME ViewBig(SS const & s, T * p). Cf [ra37].
            filldimv(sb, b.dimv);
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
    return reshape(a, start(s));
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
    return transpose(view, start(s));
}

constexpr decltype(auto)
transpose(auto && a) { return transpose(RA_FW(a), ilist<1, 0>); }

constexpr decltype(auto)
diag(auto && a) { return transpose(RA_FW(a), ilist<0, 0>); };

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
