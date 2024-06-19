// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with dynamic lengths/strides, cf small.hh.

// (c) Daniel Llorens - 2013-2024
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "small.hh"
#include <memory>
#include <complex> // for view ops

namespace ra {


// --------------------
// nested braces for Container initializers, cf small_args in small.hh. FIXME Let any expr = braces.
// --------------------

template <class T, rank_t rank>
struct braces_def { using type = noarg; };

template <class T, rank_t rank>
using braces = braces_def<T, rank>::type;

template <class T, rank_t rank> requires (rank==1)
struct braces_def<T, rank> { using type = std::initializer_list<T>; };

template <class T, rank_t rank> requires (rank>1)
struct braces_def<T, rank> { using type = std::initializer_list<braces<T, rank-1>>; };

template <int i, class T, rank_t rank>
constexpr dim_t
braces_len(braces<T, rank> const & l)
{
    if constexpr (i>=rank) {
        return 0;
    } else if constexpr (i==0) {
        return l.size();
    } else {
        return braces_len<i-1, T, rank-1>(*(l.begin()));
    }
}

template <class T, rank_t rank, class S = std::array<dim_t, rank>>
constexpr S
braces_shape(braces<T, rank> const & l)
{
    return std::apply([&l](auto ... i) { return S { braces_len<i.value, T, rank>(l) ... }; }, mp::iota<rank> {});
}


// --------------------
// ViewBig
// --------------------

// TODO Parameterize on Child having .data() so that there's only one pointer.
// TODO Parameterize on iterator type not on value type.
// TODO Constructor checks (nonnegative lens, steps inside, etc.).
template <class T, rank_t RANK>
struct ViewBig
{
    using Dimv = std::conditional_t<ANY==RANK, vector_default_init<Dim>, Small<Dim, ANY==RANK ? 0 : RANK>>;
    Dimv dimv;
    T * cp;

    consteval static rank_t rank() requires (RANK!=ANY) { return RANK; }
    constexpr rank_t rank() const requires (RANK==ANY) { return rank_t(dimv.size()); }
    constexpr static dim_t len_s(int k) { return ANY; }
    constexpr dim_t len(int k) const { return dimv[k].len; }
    constexpr dim_t step(int k) const { return dimv[k].step; }
    constexpr auto data() const { return cp; }
    constexpr dim_t size() const { return prod(map(&Dim::len, dimv)); }
    constexpr bool empty() const { return any(0==map(&Dim::len, dimv)); }

    constexpr ViewBig(): cp() {} // FIXME used by Container constructors
    constexpr ViewBig(Dimv const & dimv_, T * cp_): dimv(dimv_), cp(cp_) {} // [ra36]
    constexpr ViewBig(auto && s, T * cp_): cp(cp_)
    {
        ra::resize(dimv, ra::size(s)); // [ra37]
        if constexpr (std::is_convertible_v<value_t<decltype(s)>, Dim>) {
            start(dimv) = s;
        } else {
            filldim(dimv, s);
        }
    }
    constexpr ViewBig(std::initializer_list<dim_t> s, T * cp_): ViewBig(start(s), cp_) {}
// cf RA_ASSIGNOPS_SELF [ra38] [ra34]
    ViewBig const & operator=(ViewBig && x) const { start(*this) = x; return *this; }
    ViewBig const & operator=(ViewBig const & x) const { start(*this) = x; return *this; }
    constexpr ViewBig(ViewBig &&) = default;
    constexpr ViewBig(ViewBig const &) = default;
#define ASSIGNOPS(OP)                                               \
    ViewBig const & operator OP (auto && x) const { start(*this) OP x; return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS
// braces row-major ravel for rank!=1. See Container::fill1
    using ravel_arg = std::conditional_t<RANK==1, noarg, std::initializer_list<T>>;
    ViewBig const & operator=(ravel_arg const x) const
    {
        auto xsize = ssize(x);
        RA_CHECK(size()==xsize, "Mismatched sizes ", ViewBig::size(), " ", xsize, ".");
        std::ranges::copy_n(x.begin(), xsize, begin());
        return *this;
    }
    constexpr ViewBig const & operator=(braces<T, RANK> x) const requires (RANK!=ANY) { ra::iter<-1>(*this) = x; return *this; }
#define RA_BRACES_ANY(N)                                                \
    constexpr ViewBig const & operator=(braces<T, N> x) const requires (RANK==ANY) { ra::iter<-1>(*this) = x; return *this; }
    FOR_EACH(RA_BRACES_ANY, 2, 3, 4);
#undef RA_BRACES_ANY

    constexpr dim_t
    select(Dim * dim, int k, dim_t i) const
    {
        RA_CHECK(inside(i, len(k)), "Bad index ", i, " for len[", k, "]=", len(k), ".");
        return step(k)*i;
    }
    constexpr dim_t
    select(Dim * dim, int k, is_iota auto i) const
    {
        RA_CHECK(inside(i, len(k)), "Bad index iota [", i.n, " ", i.i, " ", i.s, "] for len[", k, "]=", len(k), ".");
        *dim = { .len = i.n, .step = step(k) * i.s };
        return 0==i.n ? 0 : step(k)*i.i;
    }
    template <class I0, class ... I>
    constexpr dim_t
    select_loop(Dim * dim, int k, I0 && i0, I && ... i) const
    {
        return select(dim, k, wlen(len(k), RA_FWD(i0)))
            + select_loop(dim + beatable<I0>.dst, k + beatable<I0>.src, RA_FWD(i) ...);
    }
    template <int n, class ... I>
    constexpr dim_t
    select_loop(Dim * dim, int k, dots_t<n> i0, I && ... i) const
    {
        int nn = (BAD==n) ? (rank() - k - (0 + ... + beatable<I>.src)) : n;
        for (Dim * end = dim+nn; dim!=end; ++dim, ++k) {
            *dim = dimv[k];
        }
        return select_loop(dim, k, RA_FWD(i) ...);
    }
    template <int n, class ... I>
    constexpr dim_t
    select_loop(Dim * dim, int k, insert_t<n> i0, I && ... i) const
    {
        for (Dim * end = dim+n; dim!=end; ++dim) {
            *dim = { .len = BAD, .step = 0 };
        }
        return select_loop(dim, k, RA_FWD(i) ...);
    }
    constexpr static dim_t
    select_loop(Dim * dim, int k) { return 0; }

    template <class ... I>
    constexpr decltype(auto)
    operator()(this auto && self, I && ... i)
    {
        constexpr int stretch = (0 + ... + (beatable<I>.dst==BAD));
        static_assert(stretch<=1, "Cannot repeat stretch index.");
        if constexpr ((0 + ... + is_scalar_index<I>)==RANK) {
            return self.cp[self.select_loop(nullptr, 0, i ...)];
        } else if constexpr ((beatable<I>.rt && ...)) {
            constexpr rank_t extended = (0 + ... + beatable<I>.add);
            ViewBig<T, rank_sum(RANK, extended)> sub;
            rank_t subrank = self.rank()+extended;
            if constexpr (ANY==RANK) {
                sub.dimv.resize(subrank);
            }
            sub.cp = self.cp + self.select_loop(sub.dimv.data(), 0, i ...);
// fill rest of dim, skipping over beatable subscripts.
            for (int k = (0==stretch ? (0 + ... + beatable<I>.dst) : subrank); k<subrank; ++k) {
                sub.dimv[k] = self.dimv[k-extended];
            }
            return sub;
// TODO partial beating
        } else {
// cf ViewSmall::operator()
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
       constexpr rank_t crank = rank_diff(RANK, ra::size_s(i));
       if constexpr (ANY==crank) {
            return iter(rank()-ra::size(i)).at(RA_FWD(i));
        } else {
            return iter<crank>().at(RA_FWD(i));
        }
    }

    template <rank_t c=0> constexpr auto iter() const && { return Cell<T, Dimv, ic_t<c>>(cp, std::move(dimv)); }
    template <rank_t c=0> constexpr auto iter() const & { return Cell<T, Dimv const &, ic_t<c>>(cp, dimv); }
    constexpr auto iter(rank_t c) const && { return Cell<T, Dimv, dim_t>(cp, std::move(dimv), c); }
    constexpr auto iter(rank_t c) const & { return Cell<T, Dimv const &, dim_t>(cp, dimv, c); }
    constexpr auto begin() const { return STLIterator(iter<0>()); }
    constexpr decltype(auto) static end() { return std::default_sentinel; }
    constexpr T & back() const { dim_t s = size(); RA_CHECK(s>0, "Bad back()."); return cp[s-1]; }

    constexpr
    operator T & () const
    {
        if constexpr (0!=RANK) {
            RA_CHECK(1==size(), "Bad scalar conversion from shape [", ra::noshape, ra::shape(*this), "].");
        }
        return cp[0];
    }
// FIXME override SmallArray(X && x) if T is SmallArray [ra15]
    constexpr operator T & () { return std::as_const(*this); }
// conversions from var rank to fixed rank
    template <rank_t R> requires (R==ANY && R!=RANK)
    constexpr ViewBig(ViewBig<T, R> const & x): dimv(x.dimv), cp(x.cp) {}
    template <rank_t R> requires (R==ANY && R!=RANK && std::is_const_v<T>)
    constexpr ViewBig(ViewBig<std::remove_const_t<T>, R> const & x): dimv(x.dimv), cp(x.cp) {}
// conversion from fixed rank to var rank
    template <rank_t R> requires (R!=ANY && RANK==ANY)
    constexpr ViewBig(ViewBig<T, R> const & x): dimv(x.dimv.begin(), x.dimv.end()), cp(x.cp) {}
    template <rank_t R> requires (R!=ANY && RANK==ANY && std::is_const_v<T>)
    constexpr ViewBig(ViewBig<std::remove_const_t<T>, R> const & x): dimv(x.dimv.begin(), x.dimv.end()), cp(x.cp) {}
// conversion to const. We rely on it for Container::view(). FIXME iffy? not constexpr, and doesn't work for Small.
    constexpr operator ViewBig<T const, RANK> const & () const requires (!std::is_const_v<T>)
    {
        return *reinterpret_cast<ViewBig<T const, RANK> const *>(this);
    }
};


// --------------------
// Container types
// --------------------

template <class V>
struct storage_traits
{
    using T = V::value_type;
    static_assert(!std::is_same_v<std::remove_const_t<T>, bool>, "No pointers to bool in std::vector<bool>.");
    constexpr static auto create(dim_t n) { RA_CHECK(0<=n, "Bad size ", n, "."); return V(n); }
    template <class VV> constexpr static auto data(VV & v) { return v.data(); }
};

template <class P>
struct storage_traits<std::unique_ptr<P>>
{
    using V = std::unique_ptr<P>;
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CHECK(0<=n, "Bad size ", n, "."); return V(new T[n]); }
    template <class VV> constexpr static auto data(VV & v) { return v.get(); }
};

template <class P>
struct storage_traits<std::shared_ptr<P>>
{
    using V = std::shared_ptr<P>;
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CHECK(0<=n, "Bad size ", n, "."); return V(new T[n], std::default_delete<T[]>()); }
    template <class VV> constexpr static auto data(VV & v) { return v.get(); }
};

// FIXME avoid duplicating ViewBig::p. Avoid overhead with rank 1.
template <class Store, rank_t RANK>
struct Container: public ViewBig<typename storage_traits<Store>::T, RANK>
{
    Store store;
    using T = typename storage_traits<Store>::T;
    using View = ra::ViewBig<T, RANK>;
    using ViewConst = ra::ViewBig<T const, RANK>;
    using View::size, View::rank;
    using shape_arg = decltype(shape(std::declval<View>().iter()));

    constexpr ViewConst const & view() const { return static_cast<View const &>(*this); }
    constexpr View & view() { return *this; }

// Needed to set View::cp. FIXME Remove duplication as in SmallBase/SmallArray.
    Container(Container && w): store(std::move(w.store))
    {
        View::dimv = std::move(w.dimv);
        View::cp = storage_traits<Store>::data(store);
    }
    Container(Container const & w): store(w.store)
    {
        View::dimv = w.dimv;
        View::cp = storage_traits<Store>::data(store);
    }
    Container(Container & w): Container(std::as_const(w)) {}

// A(shape 2 3) = A-type [1 2 3] initializes, so it doesn't behave as A(shape 2 3) = not-A-type [1 2 3] which uses View::operator=. This is used by operator>>(std::istream &, Container &). See test/ownership.cc [ra20].
// TODO don't require copyable T in constructors, see fill1. That requires operator= to initialize, not update.
    Container & operator=(Container && w)
    {
        store = std::move(w.store);
        View::dimv = std::move(w.dimv);
        View::cp = storage_traits<Store>::data(store);
        return *this;
    }
    Container & operator=(Container const & w)
    {
        store = w.store;
        View::dimv = w.dimv;
        View::cp = storage_traits<Store>::data(store);
        return *this;
    }
    Container & operator=(Container & w) { return *this = std::as_const(w); }

    constexpr decltype(auto) back(this auto && self) { return RA_FWD(self).view().back(); }
    constexpr auto data(this auto && self) { return self.view().data(); }
    constexpr decltype(auto) operator()(this auto && self, auto && ... a) { return RA_FWD(self).view()(RA_FWD(a) ...); }
    constexpr decltype(auto) operator[](this auto && self, auto && ... a) { return RA_FWD(self).view()(RA_FWD(a) ...); }
    constexpr decltype(auto) at(this auto && self, auto && i) { return RA_FWD(self).view().at(RA_FWD(i)); }
// always compact/row-major, so STL-like iterators can be raw pointers.
    constexpr auto begin(this auto && self) { assert(is_c_order(self.view())); return self.view().data(); }
    constexpr auto end(this auto && self) { return self.view().data()+self.size(); }
    template <rank_t c=0> constexpr auto iter(this auto && self) { if constexpr (1==RANK && 0==c) { return ptr(self.data(), self.size()); } else { return RA_FWD(self).view().template iter<c>(); } }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }

// non-copy assignment operators follow View, but cannot be just using'd because of constness.
#define ASSIGNOPS(OP)                                               \
    Container & operator OP (auto && x) { view() OP x; return *this; }
    FOR_EACH(ASSIGNOPS, =, *=, +=, -=, /=)
#undef ASSIGNOPS
    using ravel_arg = std::conditional_t<RANK==1, noarg, std::initializer_list<T>>;
    Container & operator=(ravel_arg const x) { view() = x; return *this; }
    constexpr Container & operator=(braces<T, RANK> x) requires (RANK!=ANY) { view() = x; return *this; }
#define RA_BRACES_ANY(N)                                                \
    constexpr Container & operator=(braces<T, N> x) requires (RANK==ANY) { view() = x; return *this; }
    FOR_EACH(RA_BRACES_ANY, 2, 3, 4);
#undef RA_BRACES_ANY

    void
    init(auto && s) requires (1==rank_s(s) || ANY==rank_s(s))
    {
        static_assert(!std::is_convertible_v<value_t<decltype(s)>, Dim>);
        RA_CHECK(1==ra::rank(s), "Rank mismatch for init shape.");
        static_assert(ANY==RANK || ANY==size_s(s) || RANK==size_s(s) || BAD==size_s(s), "Bad shape for rank.");
        ra::resize(View::dimv, ra::size(s)); // [ra37]
        store = storage_traits<Store>::create(filldim(View::dimv, s));
        View::cp = storage_traits<Store>::data(store);
    }
    void init(dim_t s) { init(std::array {s}); } // scalar allowed as shape if rank is 1.

// provided so that {} calls shape_arg constructor below.
    Container() requires (ANY==RANK): View({ Dim {0, 1} }, nullptr) {}
    Container() requires (ANY!=RANK && 0!=RANK): View(typename View::Dimv(Dim {0, 1}), nullptr) {}
    Container() requires (0==RANK): Container({}, none) {}

// shape_arg overloads handle {...} arguments. Size check is at conversion (if shape_arg is Small) or init().
    Container(shape_arg const & s, none_t) { init(s); }
    Container(shape_arg const & s, auto && x): Container(s, none) { iter() = x; }
    Container(shape_arg const & s, braces<T, RANK> x) requires (RANK==1) : Container(s, none) { view() = x; }
    Container(auto && x): Container(ra::shape(x), none) { iter() = x; }

    Container(braces<T, RANK> x) requires (RANK!=ANY)
        : Container(braces_shape<T, RANK>(x), none) { view() = x; }
#define RA_BRACES_ANY(N)                                           \
    Container(braces<T, N> x) requires (RANK==ANY)                 \
        : Container(braces_shape<T, N>(x), none) { view() = x; }
    FOR_EACH(RA_BRACES_ANY, 1, 2, 3, 4)
#undef RA_BRACES_ANY

// FIXME requires T to be copiable, which conflicts with the semantics of view_.operator=. store(x) avoids it for Big, but doesn't work for Unique. Should construct in place like std::vector does.
    constexpr void
    fill1(auto && xbegin, dim_t xsize)
    {
        RA_CHECK(size()==xsize, "Mismatched sizes ", size(), " ", xsize, ".");
        std::ranges::copy_n(RA_FWD(xbegin), xsize, begin());
    }

// shape + row-major ravel.
// FIXME explicit it-is-ravel mark. Also iter<n> initializers.
// FIXME regular (no need for fill1) for ANY if rank is 1.
    Container(shape_arg const & s, typename View::ravel_arg x)
        : Container(s, none) { fill1(x.begin(), x.size()); }

// FIXME remove
    Container(shape_arg const & s, auto * p)
        : Container(s, none) { fill1(p, size()); } // FIXME fake check
// FIXME remove
    Container(shape_arg const & s, auto && pbegin, dim_t psize)
        : Container(s, none) { fill1(RA_FWD(pbegin), psize); }

// for shape arguments that doesn't convert implicitly to shape_arg
    Container(auto && s, none_t)
        { init(RA_FWD(s)); }
    Container(auto && s, auto && x)
        : Container(RA_FWD(s), none) { iter() = x; }
    Container(auto && s, std::initializer_list<T> x)
        : Container(RA_FWD(s), none) { fill1(x.begin(), x.size()); }

// resize first axis or full shape. Only for some kinds of store.
    void resize(dim_t const s)
    {
        static_assert(ANY==RANK || 0<RANK); RA_CHECK(0<rank());
        View::dimv[0].len = s;
        store.resize(size());
        View::cp = store.data();
    }
    void resize(dim_t const s, T const & t)
    {
        static_assert(ANY==RANK || 0<RANK); RA_CHECK(0<rank());
        View::dimv[0].len = s;
        store.resize(size(), t);
        View::cp = store.data();
    }
    void resize(auto const & s) requires (rank_s(s) > 0)
    {
        ra::resize(View::dimv, start(s).len(0)); // [ra37] FIXME is View constructor
        store.resize(filldim(View::dimv, s));
        View::cp = store.data();
    }
// template + RA_FWD wouldn't work for push_back(brace-enclosed-list).
    void push_back(T && t)
    {
        static_assert(ANY==RANK || 1==RANK); RA_CHECK(1==rank());
        store.push_back(std::move(t));
        ++View::dimv[0].len;
        View::cp = store.data();
    }
    void push_back(T const & t)
    {
        static_assert(ANY==RANK || 1==RANK); RA_CHECK(1==rank());
        store.push_back(t);
        ++View::dimv[0].len;
        View::cp = store.data();
    }
    void emplace_back(auto && ... a)
    {
        static_assert(ANY==RANK || 1==RANK); RA_CHECK(1==rank());
        store.emplace_back(RA_FWD(a) ...);
        ++View::dimv[0].len;
        View::cp = store.data();
    }
    void pop_back()
    {
        static_assert(ANY==RANK || 1==RANK); RA_CHECK(1==rank());
        RA_CHECK(0<View::dimv[0].len, "Empty array trying to pop_back().");
        --View::dimv[0].len;
        store.pop_back();
    }
};

// rely on std::swap; else ambiguous
template <class Store, rank_t RANKA, rank_t RANKB> requires (RANKA!=RANKB)
void
swap(Container<Store, RANKA> & a, Container<Store, RANKB> & b)
{
    if constexpr (ANY==RANKA) {
        RA_CHECK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " and ", rank(b), ".");
        decltype(b.dimv) c = a.dimv;
        start(a.dimv) = b.dimv;
        std::swap(b.dimv, c);
    } else if constexpr (ANY==RANKB) {
        RA_CHECK(rank(a)==rank(b), "Mismatched ranks ", rank(a), " and ", rank(b), ".");
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

// -------------
// Used in Guile wrappers to let parameter either borrow from Guile storage or convert into new array (eg 'f32 into 'f64).
// TODO Can use unique_ptr's deleter for this?
// TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args
// -------------

template <rank_t RANK, class T>
Shared<T, RANK>
shared_borrowing(ViewBig<T, RANK> & raw)
{
    Shared<T, RANK> a;
    a.dimv = raw.dimv;
    a.cp = raw.cp;
    a.store = std::shared_ptr<T>(raw.data(), [](T *) {});
    return a;
}


// --------------------
// concrete (container) type from expression.
// --------------------

template <class E>
struct concrete_type_def
{
    using type = void;
};
template <class E> requires (size_s<E>()==ANY)
struct concrete_type_def<E>
{
    using type = Big<ncvalue_t<E>, rank_s<E>()>;
};
template <class E> requires (size_s<E>()!=ANY)
struct concrete_type_def<E>
{
    using type = decltype(std::apply([](auto ... i) { return Small<ncvalue_t<E>, E::len_s(i) ...> {}; },
                                     mp::iota<rank_s<E>()> {}));
};
// Scalars are their own concrete_type. Treat unregistered types as scalars.
template <class E>
using concrete_type = std::decay_t<
    std::conditional_t<(0==rank_s<E>() && !is_ra<E>) || is_scalar<E>,
                       std::decay_t<E>,
                       typename concrete_type_def<std::decay_t<decltype(start(std::declval<E>()))>>::type>>;

template <class E>
constexpr auto
concrete(E && e)
{
    return concrete_type<E>(RA_FWD(e));
}

template <class E>
constexpr auto
with_same_shape(E && e)
{
    if constexpr (ANY!=size_s(e)) {
        return concrete_type<E>();
    } else {
        return concrete_type<E>(ra::shape(e), none);
    }
}

template <class E, class X>
constexpr auto
with_same_shape(E && e, X && x)
{
    if constexpr (ANY!=size_s(e)) {
        return concrete_type<E>(RA_FWD(x));
    } else {
        return concrete_type<E>(ra::shape(e), RA_FWD(x));
    }
}

template <class E, class S, class X>
constexpr auto
with_shape(S && s, X && x)
{
    if constexpr (ANY!=size_s<E>()) {
        return concrete_type<E>(RA_FWD(x));
    } else {
        return concrete_type<E>(RA_FWD(s), RA_FWD(x));
    }
}

template <class E, class S, class X>
constexpr auto
with_shape(std::initializer_list<S> && s, X && x)
{
    if constexpr (ANY!=size_s<E>()) {
        return concrete_type<E>(RA_FWD(x));
    } else {
        return concrete_type<E>(s, RA_FWD(x));
    }
}


// --------------------
// ViewBig ops
// --------------------

template <class T, rank_t RANK>
inline ViewBig<T, RANK>
reverse(ViewBig<T, RANK> const & view, int k=0)
{
    RA_CHECK(inside(k, view.rank()), "Bad axis ", k, " for rank ", view.rank(), ".");
    ViewBig<T, RANK> r = view;
    if (auto & dim=r.dimv[k]; dim.len!=0) {
        r.cp += dim.step*(dim.len-1);
        dim.step *= -1;
    }
    return r;
}

// static transposed axes list, output rank is static.
template <int ... Iarg, class T, rank_t RANK>
inline auto
transpose(ViewBig<T, RANK> const & view)
{
    static_assert(RANK==ANY || RANK==sizeof...(Iarg), "Bad output rank.");
    RA_CHECK(view.rank()==sizeof...(Iarg), "Bad output rank ", view.rank(), " should be ", (sizeof...(Iarg)), ".");
    constexpr static std::array<dim_t, sizeof...(Iarg)> s = { Iarg ... };
    constexpr rank_t dstrank = (0==ra::size(s)) ? 0 : 1 + *std::ranges::max_element(s);
    ViewBig<T, dstrank> r;
    r.cp = view.data();
    transpose_filldim(s, view.dimv, r.dimv);
    return r;
}

// dynamic transposed axes list, output rank is dynamic. FIXME only some S are valid here.
template <class T, rank_t RANK, class S>
inline ViewBig<T, ANY>
transpose_(S && s, ViewBig<T, RANK> const & view)
{
    RA_CHECK(view.rank()==ra::size(s), "Bad size for transposed axes list.");
    rank_t dstrank = (0==ra::size(s)) ? 0 : 1 + *std::ranges::max_element(s);
    ViewBig<T, ANY> r { decltype(r.dimv)(dstrank), view.data() };
    transpose_filldim(s, view.dimv, r.dimv);
    return r;
}

template <class T, rank_t RANK, class S>
inline ViewBig<T, ANY>
transpose(S && s, ViewBig<T, RANK> const & view)
{
    return transpose_(RA_FWD(s), view);
}

// Need compile time values and not sizes to deduce the output rank, so initializer_list suffices.
template <class T, rank_t RANK>
inline ViewBig<T, ANY>
transpose(std::initializer_list<ra::rank_t> s, ViewBig<T, RANK> const & view)
{
    return transpose_(s, view);
}

template <class T, rank_t RANK>
inline auto
diag(ViewBig<T, RANK> const & view)
{
    return transpose<0, 0>(view);
}

template <class T, rank_t RANK>
inline ViewBig<T, 1>
ravel_free(ViewBig<T, RANK> const & a)
{
    RA_CHECK(is_c_order(a, false));
    int r = a.rank()-1;
    for (; r>=0 && a.len(r)==1; --r) {}
    ra::dim_t s = r<0 ? 1 : a.step(r);
    return ra::ViewBig<T, 1>({{ra::size(a), s}}, a.cp);
}

template <class T, rank_t RANK, class S>
inline auto
reshape_(ViewBig<T, RANK> const & a, S && sb_)
{
    auto sb = concrete(RA_FWD(sb_));
// FIXME when we need to copy, accept/return Shared
    dim_t la = ra::size(a);
    dim_t lb = 1;
    for (int i=0; i<ra::size(sb); ++i) {
        if (sb[i]==-1) {
            dim_t quot = lb;
            for (int j=i+1; j<ra::size(sb); ++j) {
                quot *= sb[j];
                RA_CHECK(quot>0, "Cannot deduce dimensions.");
            }
            auto pv = la/quot;
            RA_CHECK((la%quot==0 && pv>=0), "Bad placeholder.");
            sb[i] = pv;
            lb = la;
            break;
        } else {
            lb *= sb[i];
        }
    }
    auto sa = shape(a);
// FIXME should be able to reshape Scalar etc.
    ViewBig<T, ra::size_s(sb)> b(map([](auto i) { return Dim { BAD, 0 }; }, ra::iota(ra::size(sb))), a.data());
    rank_t i = 0;
    for (; i<a.rank() && i<b.rank(); ++i) {
        if (sa[a.rank()-i-1]!=sb[b.rank()-i-1]) {
            RA_CHECK(is_c_order(a, false) && la>=lb, "Reshape with copy not implemented.");
// FIXME ViewBig(SS const & s, T * p). Cf [ra37].
            filldim(b.dimv, sb);
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

template <class T, rank_t RANK, class S>
inline auto
reshape(ViewBig<T, RANK> const & a, S && sb_)
{
    return reshape_(a, RA_FWD(sb_));
}

// We need dimtype bc {1, ...} deduces to int and that fails to match ra::dim_t. initializer_list could handle the general case, but the result would have var rank and override this one (?).
template <class T, rank_t RANK, class dimtype, int N>
inline auto
reshape(ViewBig<T, RANK> const & a, dimtype const (&sb_)[N])
{
    return reshape_(a, sb_);
}

// lo: lower bounds, hi: upper bounds. The stencil indices are in [0 lo+1+hi] = [-lo +hi].
template <class LO, class HI, class T, rank_t N>
inline ViewBig<T, rank_sum(N, N)>
stencil(ViewBig<T, N> const & a, LO && lo, HI && hi)
{
    ViewBig<T, rank_sum(N, N)> s;
    s.cp = a.data();
    ra::resize(s.dimv, 2*a.rank());
    RA_CHECK(every(lo>=0) && every(hi>=0), "Bad stencil bounds lo ", noshape, lo, " hi ", noshape, hi, ".");
    for_each([](auto & dims, auto && dima, auto && lo, auto && hi)
             {
                 RA_CHECK(dima.len>=lo+hi, "Stencil is too large for array.");
                 dims = {dima.len-lo-hi, dima.step};
             },
             ptr(s.dimv.data()), a.dimv, lo, hi);
    for_each([](auto & dims, auto && dima, auto && lo, auto && hi)
             { dims = {lo+hi+1, dima.step}; },
             ptr(s.dimv.data()+a.rank()), a.dimv, lo, hi);
    return s;
}

// Make last sizes of ViewBig<> be compile-time constants.
template <class super_t, rank_t SUPERR, class T, rank_t RANK>
inline auto
explode_(ViewBig<T, RANK> const & a)
{
// TODO Reduce to single check, either the first or the second.
    static_assert(RANK>=SUPERR || RANK==ANY, "rank of a is too low");
    RA_CHECK(a.rank()>=SUPERR, "Rank of a ", a.rank(), " should be at least ", SUPERR, ".");
    ViewBig<super_t, rank_sum(RANK, -SUPERR)> b;
    ra::resize(b.dimv, a.rank()-SUPERR);
    dim_t r = 1;
    for (int i=0; i<SUPERR; ++i) {
        r *= a.len(i+b.rank());
    }
    RA_CHECK(r*sizeof(T)==sizeof(super_t), "Length of axes ", r*sizeof(T), " doesn't match type ", sizeof(super_t), ".");
    for (int i=0; i<b.rank(); ++i) {
        RA_CHECK(a.step(i) % r==0, "Step of axes ", a.step(i), " doesn't match type ", r, " on axis ", i, ".");
        b.dimv[i] = { .len = a.len(i), .step = a.step(i) / r };
    }
    RA_CHECK((b.rank()==0 || a.step(b.rank()-1)==r), "Super type is not compact in array.");
    b.cp = reinterpret_cast<super_t *>(a.data());
    return b;
}

template <class super_t, class T, rank_t RANK>
inline auto
explode(ViewBig<T, RANK> const & a)
{
    return explode_<super_t, (std::is_same_v<super_t, std::complex<T>> ? 1 : rank_s<super_t>())>(a);
}

// TODO Check that ranks below SUBR are compact.
template <class sub_t, class super_t, rank_t RANK>
inline auto
collapse(ViewBig<super_t, RANK> const & a)
{
    using super_v = value_t<super_t>;
    using sub_v = value_t<sub_t>;
    constexpr int subtype = sizeof(super_v)/sizeof(sub_t);
    constexpr int SUBR = rank_s<super_t>() - rank_s<sub_t>();
    static auto gstep = [](int i) { if constexpr (is_scalar<super_t>) return 1; else return super_t::step(i); };
    static auto glen = [](int i) { if constexpr (is_scalar<super_t>) return 1; else return super_t::len(i); };

    ViewBig<sub_t, rank_sum(RANK, SUBR+int(subtype>1))> b;
    resize(b.dimv, a.rank()+SUBR+int(subtype>1));

    constexpr dim_t r = sizeof(super_t)/sizeof(sub_t);
    static_assert(sizeof(super_t)==r*sizeof(sub_t), "Cannot make axis of super_t from sub_t.");
    for (int i=0; i<a.rank(); ++i) {
        b.dimv[i] = { .len = a.len(i), .step = a.step(i) * r };
    }
    constexpr int t = sizeof(super_v)/sizeof(sub_v);
    constexpr int s = sizeof(sub_t)/sizeof(sub_v);
    static_assert(t*sizeof(sub_v)>=1, "Bad subtype.");
    for (int i=0; i<SUBR; ++i) {
        RA_CHECK(((gstep(i)/s)*s==gstep(i)), "Bad steps."); // TODO actually static
        b.dimv[a.rank()+i] = { .len = glen(i), .step = gstep(i) / s * t };
    }
    if (subtype>1) {
        b.dimv[a.rank()+SUBR] = { .len = t, .step = 1 };
    }
    b.cp = reinterpret_cast<sub_t *>(a.data());
    return b;
}

} // namespace ra
