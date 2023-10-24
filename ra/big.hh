// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with dynamic lengths/strides, cf small.hh.

// (c) Daniel Llorens - 2013-2023
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "small.hh"
#include <memory>

namespace ra {

// Default storage for Big - see https://stackoverflow.com/a/21028912
// Allocator adaptor that interposes construct() calls to convert value initialization into default initialization.
template <typename T, typename A=std::allocator<T>>
struct default_init_allocator: public A
{
    using a_t = std::allocator_traits<A>;
    using A::A;

    template <typename U>
    struct rebind
    {
        using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
    };

    template <typename U>
    void construct(U * ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
    {
        ::new(static_cast<void *>(ptr)) U;
    }
    template <typename U, typename... Args>
    void construct(U * ptr, Args &&... args)
    {
        a_t::construct(static_cast<A &>(*this), ptr, std::forward<Args>(args)...);
    }
};


// --------------------
// Big iterator
// --------------------

template <class T, rank_t RANK=ANY> struct View;

// TODO Refactor with CellSmall. Take iterator like Ptr does and View should, not raw pointers
// TODO Clear up Dimv's type. Should I use span/Ptr? Avoid copying c in flat/at.
template <class T, class Dimv, class Spec=ic_t<0>>
struct CellBig
{
    constexpr static rank_t spec = maybe_any<Spec>;
    constexpr static rank_t fullr = size_s<Dimv>();
    constexpr static rank_t cellr = is_constant<Spec> ? rank_cell(fullr, spec) : ANY;
    constexpr static rank_t framer = is_constant<Spec> ? rank_frame(fullr, spec) : ANY;
    static_assert(cellr>=0 || cellr==ANY, "Bad cell rank.");
    static_assert(framer>=0 || framer==ANY, "Bad frame rank.");

    using ctype = View<T, cellr>;
    using value_type = std::conditional_t<0==cellr, T, ctype>;

    ctype c;
    Dimv dimv;
    [[no_unique_address]] Spec const dspec = {};

    constexpr static rank_t rank_s() { return framer; }
    constexpr rank_t rank() const requires (ANY==framer) { return rank_frame(std::ssize(dimv), dspec); }
    constexpr static rank_t rank() requires (ANY!=framer) { return framer; }
#pragma GCC diagnostic push // gcc 12.2 and 13.2 with RA_DO_CHECK=0 and -fno-sanitize=all
#pragma GCC diagnostic warning "-Warray-bounds"
    constexpr dim_t len(int k) const { return dimv[k].len; } // len(0<=k<rank) or step(0<=k)
#pragma GCC diagnostic pop
    constexpr static dim_t len_s(int k) { return ANY; }
    constexpr dim_t step(int k) const { return k<rank() ? dimv[k].step : 0; }
    constexpr bool keep_step(dim_t st, int z, int j) const { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { c.cp += step(k)*d; }

    constexpr CellBig(T * cp, Dimv const & dimv_, Spec dspec_ = Spec {})
        : dimv(dimv_), dspec(dspec_)
    {
// see STLIterator for the case of dimv_[0]=0, etc. [ra12].
        c.cp = cp;
        rank_t dcellr = rank_cell(std::ssize(dimv), dspec);
        rank_t dframer = this->rank();
        RA_CHECK(0<=dframer && 0<=dcellr, "Bad cell rank ", dcellr, " for array rank ", ssize(dimv), ").");
        resize(c.dimv, dcellr);
        for (int k=0; k<dcellr; ++k) {
            c.dimv[k] = dimv[dframer+k];
        }
    }
    constexpr CellBig(CellBig const & ci) = default;
    RA_DEF_ASSIGNOPS_DEFAULT_SET

    constexpr auto
    flat() const
    {
        if constexpr (0==cellr) {
            return c.cp;
        } else {
            return CellFlat<ctype> { c };
        }
    }
    constexpr decltype(auto)
    at(auto const & i) const
    {
        auto d = longer(*this, i);
        if constexpr (0==cellr) {
            return c.cp[d];
        } else {
            ctype cc(c); cc.cp += d;
            return cc;
        }
    }
};


// --------------------
// nested braces for Container initializers
// --------------------

template <class T, rank_t rank>
struct braces_def { using type = no_arg; };

template <class T, rank_t rank>
using braces = braces_def<T, rank>::type;

template <class T, rank_t rank> requires (rank==1)
struct braces_def<T, rank> { using type = std::initializer_list<T>; };

template <class T, rank_t rank> requires (rank>1)
struct braces_def<T, rank> { using type = std::initializer_list<braces<T, rank-1>>; };

template <int i, class T, rank_t rank>
constexpr dim_t braces_len(braces<T, rank> const & l)
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
    return std::apply([&l](auto ... i) { return S { braces_len<decltype(i)::value, T, rank>(l) ... }; },
                      mp::iota<rank> {});
}


// --------------------
// View
// --------------------

// FIXME size is immutable so that it can be kept together with rank.
// TODO Parameterize on Child having .data() so that there's only one pointer.
// TODO Parameterize on iterator type not on value type.
// TODO Constructor, if only for RA_CHECK (nonnegative lens, steps inside, etc.)
template <class T, rank_t RANK>
struct View
{
    using Dimv = std::conditional_t<RANK==ANY, std::vector<Dim>, Small<Dim, RANK==ANY ? 0 : RANK>>;

    Dimv dimv;
    T * cp;

    template <class S>
    constexpr dim_t
    filldim(S && s)
    {
        for_each([](Dim & dim, dim_t s) { RA_CHECK(s>=0, "Bad len ", s, "."); dim.len = s; },
                 dimv, s);
        dim_t next = 1;
        for (int i=dimv.size(); --i>=0;) {
            dimv[i].step = next;
            next *= dimv[i].len;
        }
        return next;
    }

    constexpr static rank_t rank_s() { return RANK; };
    constexpr static rank_t rank() requires (RANK!=ANY) { return RANK; }
    constexpr rank_t rank() const requires (RANK==ANY) { return rank_t(dimv.size()); }
    constexpr static dim_t len_s(int j) { return ANY; }
    constexpr dim_t len(int k) const { return dimv[k].len; }
    constexpr dim_t step(int k) const { return dimv[k].step; }
    constexpr auto data() const { return cp; }
    constexpr dim_t size() const { return prod(map(&Dim::len, dimv)); }

// FIXME Used by Big::init(). View can be a deduced type (e.g. from value_t<X>)
    constexpr View(): cp() {}
    constexpr View(Dimv const & dimv_, T * cp_): dimv(dimv_), cp(cp_) {} // [ra36]
    template <class SS>
    constexpr View(SS && s, T * cp_): cp(cp_)
    {
        ra::resize(dimv, start(s).len(0)); // [ra37]
        if constexpr (std::is_convertible_v<value_t<SS>, Dim>) {
            start(dimv) = s;
        } else {
            filldim(s);
        }
    }
    constexpr View(std::initializer_list<dim_t> s, T * cp_): View(start(s), cp_) {}

// [ra38] [ra34] and RA_DEF_ASSIGNOPS_SELF
    View(View && x) = default;
    View(View const & x) = default;
    View & operator=(View && x) { ra::start(*this) = x; return *this; }
    View & operator=(View const & x) { ra::start(*this) = x; return *this; }
#define DEF_ASSIGNOPS(OP)                                               \
    template <class X> View & operator OP (X && x) { ra::start(*this) OP x; return *this; }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS

    constexpr View &
    operator=(braces<T, RANK> x) requires (RANK!=ANY)
    {
        ra::iter<-1>(*this) = x;
        return *this;
    }
#define RA_BRACES_ANY(N)                            \
    constexpr View &                                \
    operator=(braces<T, N> x) requires (RANK==ANY)  \
    {                                               \
        ra::iter<-1>(*this) = x;                    \
        return *this;                               \
    }
    FOR_EACH(RA_BRACES_ANY, 2, 3, 4);
#undef RA_BRACES_ANY

// braces row-major ravel for rank!=1. See Container::fill1
    using ravel_arg = std::conditional_t<RANK==1, no_arg, std::initializer_list<T>>;
    View & operator=(ravel_arg const x)
    {
        auto xsize = ssize(x);
        RA_CHECK(size()==xsize, "Mismatched sizes ", View::size(), " ", xsize, ".");
        std::copy_n(x.begin(), xsize, begin());
        return *this;
    }

    constexpr bool const empty() const { return 0==size(); } // TODO Optimize

    template <rank_t c=0> constexpr auto iter() const && { return CellBig<T, Dimv, ic_t<c>>(cp, std::move(dimv)); }
    template <rank_t c=0> constexpr auto iter() const & { return CellBig<T, Dimv const &, ic_t<c>>(cp, dimv); }
    constexpr auto iter(rank_t c) const && { return CellBig<T, Dimv, dim_t>(cp, std::move(dimv), c); }
    constexpr auto iter(rank_t c) const & { return CellBig<T, Dimv const &, dim_t>(cp, dimv, c); }
// FIXME [ra17] should return a static object. Dimv should never be used, check also need for Dim's initializers.
    constexpr decltype(auto) static end() { return STLIterator(CellBig<T, Dimv const &>(nullptr, Dimv {})); }
    constexpr auto begin() const { return STLIterator(CellBig<T, Dimv const &>(cp, dimv)); }

    constexpr dim_t
    select(Dim * dim, int k, dim_t i) const
    {
        RA_CHECK(inside(i, dimv[k].len),
                 "Out of range for len[", k, "]=", dimv[k].len, ": ", i, ".");
        return dimv[k].step*i;
    }

    template <class I> requires (is_iota<I>)
    constexpr dim_t
    select(Dim * dim, int k, I i) const
    {
        RA_CHECK(inside(i, dimv[k].len),
                 "Out of range for len[", k, "]=", dimv[k].len, ": iota [", i.n, " ", i.i, " ", i.s, "].");
        *dim = { .len = i.n, .step = dimv[k].step * i.s };
        return dimv[k].step*i.i;
    }

    template <class I0, class ... I>
    constexpr dim_t
    select_loop(Dim * dim, int k, I0 && i0, I && ... i) const
    {
        return select(dim, k, with_len(len(k), std::forward<I0>(i0)))
            + select_loop(dim + beatable<I0>.dst, k + beatable<I0>.src, std::forward<I>(i) ...);
    }

    template <int n, class ... I>
    constexpr dim_t
    select_loop(Dim * dim, int k, dots_t<n> i0, I && ... i) const
    {
        int nn = (BAD==n) ? (rank() - k - (0 + ... + beatable<I>.src)) : n;
        for (Dim * end = dim+nn; dim!=end; ++dim, ++k) {
            *dim = dimv[k];
        }
        return select_loop(dim, k, std::forward<I>(i) ...);
    }

    template <int n, class ... I>
    constexpr dim_t
    select_loop(Dim * dim, int k, insert_t<n> i0, I && ... i) const
    {
        for (Dim * end = dim+n; dim!=end; ++dim) {
            *dim = { .len = BAD, .step = 0 };
        }
        return select_loop(dim, k, std::forward<I>(i) ...);
    }

    constexpr dim_t
    select_loop(Dim * dim, int k) const
    {
        return 0;
    }

// Specialize for rank() integer-args -> scalar, same in ra::SmallBase in small.hh.
    template <class ... I>
    constexpr decltype(auto)
    operator()(I && ... i) const
    {
        constexpr int stretch = (0 + ... + (beatable<I>.dst==BAD));
        static_assert(stretch<=1, "Cannot repeat stretch index.");
        if constexpr ((0 + ... + is_scalar_index<I>)==RANK) {
            return data()[select_loop(nullptr, 0, i ...)];
        } else if constexpr ((beatable<I>.rt && ...)) {
            constexpr rank_t extended = (0 + ... + beatable<I>.add);
            View<T, rank_sum(RANK, extended)> sub;
            rank_t subrank = rank()+extended;
            if constexpr (RANK==ANY) {
                RA_CHECK(subrank>=0, "Bad rank.");
                sub.dimv.resize(subrank);
            }
            sub.cp = data() + select_loop(sub.dimv.data(), 0, i ...);
// fill the rest of dim, skipping over beatable subscripts.
            for (int k = (0==stretch ? (0 + ... + beatable<I>.dst) : subrank); k<subrank; ++k) {
                sub.dimv[k] = dimv[k-extended];
            }
// if RANK==ANY then rank may be 0
            return sub;
// TODO partial beating
        } else {
            return unbeat<std::tuple<I ...>>::op(*this, std::forward<I>(i) ...);
        }
    }

    template <class ... I>
    constexpr decltype(auto)
    operator[](I && ... i) const
    {
        return (*this)(std::forward<I>(i) ...);
    }

    template <class I>
    constexpr decltype(auto)
    at(I && i) const
    {
// FIXME there's no way to say 'frame rank 0' so -size wouldn't work.
       constexpr rank_t crank = rank_diff(RANK, ra::size_s<I>());
       if constexpr (ANY==crank) {
            return iter(rank()-ra::size(i)).at(std::forward<I>(i));
        } else {
            return iter<crank>().at(std::forward<I>(i));
        }
    }

// conversion to scalar.
    constexpr
    operator T & () const
    {
        if constexpr (ANY==RANK) {
            RA_CHECK(0==rank(), "Error converting rank ", rank(), " to scalar.");
        } else {
            static_assert(0==RANK, "Bad rank for conversion to scalar.");
        }
        return data()[0];
    }

// necessary here per [ra15] (?)
    constexpr operator T & () { return std::as_const(*this); }

// conversions from var rank to fixed rank
    template <rank_t R> requires (R==ANY && R!=RANK)
    constexpr View(View<T, R> const & x): dimv(x.dimv), cp(x.cp) {}
    template <rank_t R> requires (R==ANY && R!=RANK && std::is_const_v<T>)
    constexpr View(View<std::remove_const_t<T>, R> const & x): dimv(x.dimv), cp(x.cp) {}

// conversion from fixed rank to var rank
    template <rank_t R> requires (R!=ANY && RANK==ANY)
    constexpr View(View<T, R> const & x): dimv(x.dimv.begin(), x.dimv.end()), cp(x.cp) {}
    template <rank_t R> requires (R!=ANY && RANK==ANY && std::is_const_v<T>)
    constexpr View(View<std::remove_const_t<T>, R> const & x): dimv(x.dimv.begin(), x.dimv.end()), cp(x.cp) {}

// conversion to const. We rely on it for Container::view().
// FIXME probably illegal, and doesn't work for SmallBase. Need another way.
    constexpr
    operator View<T const, RANK> const & () const requires (!std::is_const_v<T>)
    {
        return *reinterpret_cast<View<T const, RANK> const *>(this);
    }
};


// --------------------
// Container types
// --------------------

// std::vector-like
template <class V>
struct storage_traits
{
    using T = V::value_type;
    static_assert(!std::is_same_v<std::remove_const_t<T>, bool>, "No pointers to bool in std::vector<bool>.");
    constexpr static auto create(dim_t n) { RA_CHECK(n>=0); return V(n); }
    constexpr static T const * data(V const & v) { return v.data(); }
    constexpr static T * data(V & v) { return v.data(); }
};

template <class P>
struct storage_traits<std::unique_ptr<P>>
{
    using V = std::unique_ptr<P>;
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CHECK(n>=0); return V(new T[n]); }
    constexpr static T const * data(V const & v) { return v.get(); }
    constexpr static T * data(V & v) { return v.get(); }
};

template <class P>
struct storage_traits<std::shared_ptr<P>>
{
    using V = std::shared_ptr<P>;
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    constexpr static auto create(dim_t n) { RA_CHECK(n>=0); return V(new T[n], std::default_delete<T[]>()); }
    constexpr static T const * data(V const & v) { return v.get(); }
    constexpr static T * data(V & v) { return v.get(); }
};

template <class T, rank_t RANK>
constexpr bool
is_c_order(View<T, RANK> const & a)
{
    dim_t s = 1;
    for (int i=a.rank()-1; i>=0; --i) {
        if (s!=a.step(i)) {
            return false;
        }
        s *= a.len(i);
        if (s==0) {
            return true;
        }
    }
    return true;
}

// FIXME avoid duplicating View::p.
// FIXME avoid overhead with rank 1.
template <class Store, rank_t RANK>
struct Container: public View<typename storage_traits<Store>::T, RANK>
{
    Store store;
    using T = typename storage_traits<Store>::T;
    using View = ra::View<T, RANK>;
    using ViewConst = ra::View<T const, RANK>;
    using View::rank_s;
    using shape_arg = decltype(ra::shape(std::declval<View>().iter()));

    constexpr View & view() { return *this; }
    constexpr ViewConst const & view() const { return static_cast<View const &>(*this); }

// Needed to set View::cp. FIXME Remove duplication as in SmallBase/SmallArray, then remove the constructors and the assignment operators.
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
    Container(Container & w): store(w.store)
    {
        View::dimv = w.dimv;
        View::cp = storage_traits<Store>::data(store);
    }

// override View::operator= to allow initialization-of-reference. Unfortunately operator>>(std::istream &, Container &) requires it. The presence of these operator= means that A(shape 2 3) = type-of-A [1 2 3] initializes so it doesn't behave as A(shape 2 3) = not-type-of-A [1 2 3] which will use View::operator= and frame match. See test/ownership.cc [ra20].
// TODO do this through .set() op.
// TODO don't require copiable T from constructors, see fill1 below. That requires initialization and not update semantics for operator=.
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
    Container & operator=(Container & w)
    {
        store = w.store;
        View::dimv = w.dimv;
        View::cp = storage_traits<Store>::data(store);
        return *this;
    }

// provided so that {} calls shape_arg constructor below.
    Container()
    {
        if constexpr (ANY==RANK) {
// rank 1 to avoid store init
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Warray-bounds"
            View::dimv = { Dim {0, 1} };
#pragma GCC diagnostic pop
        } else if constexpr (0==RANK) {
// cannot have zero size
            store = storage_traits<Store>::create(1);
            View::cp = storage_traits<Store>::data(store);
        } else {
            for (Dim & dimi: View::dimv) { dimi = {0, 1}; } // 1 so we can push_back()
        }
    }

    template <class S>
    requires (1==ra::rank_s<S>() || ANY==ra::rank_s<S>())
    void
    init(S && s)
    {
        static_assert(!std::is_convertible_v<value_t<S>, Dim>);
        RA_CHECK(1==ra::rank(s), "Rank mismatch for init shape.");
// [ra37] Dimv might be STL type. Otherwise I'd just View::dimv.set(map(...)).
        if constexpr (ANY==RANK) {
            ra::resize(View::dimv, ra::size(s));
        } else if constexpr (ANY==ra::size_s<S>()) {
            RA_CHECK(RANK==ra::size(s), "Bad shape [", ra::noshape, s, "] for rank ", RANK, ".");
        } else {
            static_assert(RANK==ra::size_s<S>() || BAD==ra::size_s<S>(), "Invalid shape for rank.");
        }
        store = storage_traits<Store>::create(View::filldim(s));
        View::cp = storage_traits<Store>::data(store);
    }

    void init(ra::dim_t s) { init(std::array {s}); } // scalar allowed as shape if rank is 1.

// shape_arg overloads handle {...} arguments. Size check is at conversion (if shape_arg is Small) or init().

    Container(shape_arg const & s, none_t) { init(s); }

    template <class XX>
    Container(shape_arg const & s, XX && x): Container(s, none) { view() = x; }

    template <class XX>
    Container(XX && x): Container(ra::shape(x), none) { view() = x; }

    Container(shape_arg const & s, braces<T, RANK> x) requires (RANK==1)
        : Container(s, none) { view() = x; }

    Container(braces<T, RANK> x) requires (RANK!=ANY)
        : Container(braces_shape<T, RANK>(x), none) { view() = x; }
#define RA_BRACES_ANY(N)                                           \
    Container(braces<T, N> x) requires (RANK==ANY)                 \
        : Container(braces_shape<T, N>(x), none) { view() = x; }
    FOR_EACH(RA_BRACES_ANY, 1, 2, 3, 4)
#undef RA_BRACES_ANY

// FIXME requires T to be copyable, which conflicts with the semantics of view_.operator=. store(x) avoids it for Big, but doesn't work for Unique. Should construct in place like std::vector does.
// FIXME expr traversal is not possible for STLIterator. Fix by having plyers that don't require adv.
    template <class Xbegin>
    constexpr void
    fill1(Xbegin xbegin, dim_t xsize)
    {
        RA_CHECK(View::size()==xsize, "Mismatched sizes ", View::size(), " ", xsize, ".");
        std::copy_n(xbegin, xsize, begin());
    }

// shape + row-major ravel.
// FIXME explicit it-is-ravel mark. Also iter<n> initializers.
// FIXME regular (no need for fill1) for ANY if rank is 1.
    Container(shape_arg const & s, typename View::ravel_arg x)
        : Container(s, none) { fill1(x.begin(), x.size()); }

// FIXME remove
    template <class TT>
    Container(shape_arg const & s, TT * p)
        : Container(s, none) { fill1(p, View::size()); } // FIXME fake check

// FIXME remove
    template <class P>
    Container(shape_arg const & s, P pbegin, dim_t psize)
        : Container(s, none) { fill1(pbegin, psize); }

// for SS that doesn't convert implicitly to shape_arg
    template <class SS>
    Container(SS && s, none_t) { init(std::forward<SS>(s)); }

    template <class SS, class XX>
    Container(SS && s, XX && x)
        : Container(std::forward<SS>(s), none) { view() = x; }

    template <class SS>
    Container(SS && s, std::initializer_list<T> x)
        : Container(std::forward<SS>(s), none) { fill1(x.begin(), x.size()); }

    using View::operator=;

// resize first axis. Only for some kinds of store.
    void resize(dim_t const s)
    {
        static_assert(RANK==ANY || RANK>0); RA_CHECK(this->rank()>0);
        View::dimv[0].len = s;
        store.resize(View::size());
        View::cp = store.data();
    }
    void resize(dim_t const s, T const & t)
    {
        static_assert(RANK==ANY || RANK>0); RA_CHECK(this->rank()>0);
        View::dimv[0].len = s;
        store.resize(View::size(), t);
        View::cp = store.data();
    }
// resize full shape. Only for some kinds of store.
    template <class S>
    requires (ra::rank_s<S>() > 0)
    void resize(S const & s)
    {
        ra::resize(View::dimv, start(s).len(0)); // [ra37] FIXME is View constructor
        store.resize(View::filldim(s));
        View::cp = store.data();
    }
// lets us move. A template + std::forward wouldn't work for push_back(brace-enclosed-list).
    void push_back(T && t)
    {
        static_assert(RANK==1 || RANK==ANY); RA_CHECK(this->rank()==1);
        store.push_back(std::move(t));
        ++View::dimv[0].len;
        View::cp = store.data();
    }
    void push_back(T const & t)
    {
        static_assert(RANK==1 || RANK==ANY); RA_CHECK(this->rank()==1);
        store.push_back(t);
        ++View::dimv[0].len;
        View::cp = store.data();
    }
    template <class ... A>
    void emplace_back(A && ... a)
    {
        static_assert(RANK==1 || RANK==ANY); RA_CHECK(this->rank()==1);
        store.emplace_back(std::forward<A>(a) ...);
        ++View::dimv[0].len;
        View::cp = store.data();
    }
    void pop_back()
    {
        static_assert(RANK==1 || RANK==ANY); RA_CHECK(this->rank()==1);
        RA_CHECK(View::dimv[0].len>0);
        store.pop_back();
        --View::dimv[0].len;
    }
    constexpr bool empty() const { return 0==this->size(); }
    constexpr T const & back() const { RA_CHECK(this->rank()==1 && this->size()>0); return store[this->size()-1]; }
    constexpr T & back() { RA_CHECK(this->rank()==1 && this->size()>0); return store[this->size()-1]; }

// FIXME P0847R7 explicit object parameter
    constexpr auto data() { return view().data(); }
    constexpr auto data() const { return view().data(); }
    template <class ... A> constexpr decltype(auto) operator()(A && ... a) { return view()(std::forward<A>(a) ...); }
    template <class ... A> constexpr decltype(auto) operator()(A && ... a) const { return view()(std::forward<A>(a) ...); }
    template <class ... A> constexpr decltype(auto) operator[](A && ... a) { return view()(std::forward<A>(a) ...); }
    template <class ... A> constexpr decltype(auto) operator[](A && ... a) const { return view()(std::forward<A>(a) ...); }
    template <class I> constexpr decltype(auto) at(I && i) { return view().at(std::forward<I>(i)); }
    template <class I> constexpr decltype(auto) at(I && i) const { return view().at(std::forward<I>(i)); }
// container is always compact/row-major, so STL-like iterators can be raw pointers.
    constexpr auto begin() { assert(is_c_order(*this)); return view().data(); }
    constexpr auto begin() const { assert(is_c_order(*this)); return view().data(); }
    constexpr auto end() { return view().data()+this->size(); }
    constexpr auto end() const { return view().data()+this->size(); }
// FIXME size is redundant e.g. for Store = std::vector.
    template <rank_t c=0> constexpr auto iter() { if constexpr (1==RANK && 0==c) { return ptr(begin(), View::size()); } else { return view().template iter<c>(); } }
    template <rank_t c=0> constexpr auto iter() const { if constexpr (1==RANK && 0==c) { return ptr(begin(), View::size()); } else { return view().template iter<c>(); } }
    constexpr operator T & () { return view(); }
    constexpr operator T const & () const { return view(); }
};

template <class Store, rank_t RANKA, rank_t RANKB>
requires (RANKA!=RANKB) // rely on std::swap; else ambiguous
void
swap(Container<Store, RANKA> & a, Container<Store, RANKB> & b)
{
    if constexpr (ANY==RANKA) {
        RA_CHECK(a.rank()==b.rank());
        decltype(b.dimv) c = a.dimv;
        start(a.dimv) = b.dimv;
        std::swap(b.dimv, c);
    } else if constexpr (ANY==RANKB) {
        RA_CHECK(a.rank()==b.rank());
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

// Beyond this, we probably should have fixed-size (~std::dynarray), resizeable (~std::vector).
template <class T, rank_t RANK=ANY> using Big = Container<std::vector<T, default_init_allocator<T>>, RANK>;
// template <class T, rank_t RANK=ANY> using Big = Container<vector<T>, RANK>;
template <class T, rank_t RANK=ANY> using Unique = Container<std::unique_ptr<T []>, RANK>;
template <class T, rank_t RANK=ANY> using Shared = Container<std::shared_ptr<T>, RANK>;

// -------------
// Used in the Guile wrappers to allow an array parameter to either borrow from Guile storage or convert into a new array (e.g. passing 'f32 into 'f64).
// TODO Can use unique_ptr's deleter for this?
// TODO Shared/Unique should maybe have constructors with unique_ptr/shared_ptr args
// -------------

template <rank_t RANK, class T>
Shared<T, RANK> shared_borrowing(View<T, RANK> & raw)
{
    Shared<T, RANK> a;
    a.dimv = raw.dimv;
    a.cp = raw.cp;
    a.store = std::shared_ptr<T>(raw.data(), [](T *) {});
    return a;
}


// --------------------
// Obtain concrete type from array expression.
// --------------------

template <class E>
struct concrete_type_def
{
    using type = void;
};

template <class E>
requires (size_s<E>()==ANY)
struct concrete_type_def<E>
{
    using type = Big<value_t<E>, rank_s<E>()>;
};

template <class E>
requires (size_s<E>()!=ANY)
struct concrete_type_def<E>
{
    template <class I> struct T;
    template <int ... I> struct T<mp::int_list<I ...>>
    {
        using type = Small<value_t<E>, E::len_s(I) ...>;
    };
    using type = typename T<mp::iota<rank_s<E>()>>::type;
};

// Scalars are their own concrete_type. Treat unregistered types as scalars. FIXME (in bootstrap.hh).
template <class E>
using concrete_type = std::decay_t<
    std::conditional_t<
        (0==rank_s<E>() && !(requires { std::decay_t<E>::rank_s(); })) || is_scalar<E>,
        std::decay_t<E>,
        typename concrete_type_def<std::decay_t<decltype(start(std::declval<E>()))>>::type>
    >;

template <class E>
constexpr auto
concrete(E && e)
{
// FIXME gcc 11.3 on GH workflows (?)
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"
    return concrete_type<E>(std::forward<E>(e));
#pragma GCC diagnostic pop
}

template <class E>
constexpr auto
with_same_shape(E && e)
{
    if constexpr (size_s<concrete_type<E>>()!=ANY) {
        return concrete_type<E>();
    } else {
        return concrete_type<E>(ra::shape(e), ra::none);
    }
}

template <class E, class X>
constexpr auto
with_same_shape(E && e, X && x)
{
// FIXME gcc 11.3 on GH workflows (?)
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"
    if constexpr (size_s<concrete_type<E>>()!=ANY) {
        return concrete_type<E>(std::forward<X>(x));
    } else {
        return concrete_type<E>(ra::shape(e), std::forward<X>(x));
    }
#pragma GCC diagnostic pop
}

template <class E, class S, class X>
constexpr auto
with_shape(S && s, X && x)
{
    if constexpr (size_s<concrete_type<E>>()!=ANY) {
        return concrete_type<E>(std::forward<X>(x));
    } else {
        return concrete_type<E>(std::forward<S>(s), std::forward<X>(x));
    }
}

template <class E, class S, class X>
constexpr auto
with_shape(std::initializer_list<S> && s, X && x)
{
    if constexpr (size_s<concrete_type<E>>()!=ANY) {
        return concrete_type<E>(std::forward<X>(x));
    } else {
        return concrete_type<E>(s, std::forward<X>(x));
    }
}

} // namespace ra

#include <complex> // for View ops

namespace ra {


// --------------------
// Operations specific to View.
// --------------------

template <class T, rank_t RANK> inline
View<T, RANK> reverse(View<T, RANK> const & view, int k)
{
    View<T, RANK> r = view;
    auto & dim = r.dimv[k];
    if (dim.len!=0) {
        r.cp += dim.step*(dim.len-1);
        dim.step *= -1;
    }
    return r;
}

// dynamic transposed axes list.
template <class T, rank_t RANK, class S> inline
View<T, ANY> transpose_(S && s, View<T, RANK> const & view)
{
    RA_CHECK(view.rank()==ra::size(s));
    auto rp = std::max_element(s.begin(), s.end());
    rank_t dstrank = (rp==s.end() ? 0 : *rp+1);

    View<T, ANY> r { decltype(r.dimv)(dstrank, Dim { BAD, 0 }), view.data() };
    for (int k=0; int sk: s) {
        Dim & dest = r.dimv[sk];
        dest.step += view.dimv[k].step;
        dest.len = dest.len>=0 ? std::min(dest.len, view.dimv[k].len) : view.dimv[k].len;
        ++k;
    }
    return r;
}

template <class T, rank_t RANK, class S> inline
View<T, ANY> transpose(S && s, View<T, RANK> const & view)
{
    return transpose_(std::forward<S>(s), view);
}

// Note that we need the compile time values and not the sizes to deduce the rank of the output, so it would be useless to provide a builtin array shim as we do with reshape().
template <class T, rank_t RANK> inline
View<T, ANY> transpose(std::initializer_list<ra::rank_t> s, View<T, RANK> const & view)
{
    return transpose_(s, view);
}

// static transposed axes list.
template <int ... Iarg, class T, rank_t RANK> inline
auto transpose(View<T, RANK> const & view)
{
    static_assert(RANK==ANY || RANK==sizeof...(Iarg), "Bad output rank.");
    RA_CHECK(view.rank()==sizeof...(Iarg), "Bad output rank: ", view.rank(), "should be ", (sizeof...(Iarg)), ".");

    using dummy_s = mp::makelist<sizeof...(Iarg), ic_t<0>>;
    using ti = axes_list_indices<mp::int_list<Iarg ...>, dummy_s, dummy_s>;
    constexpr rank_t DSTRANK = mp::len<typename ti::dst>;

    View<T, DSTRANK> r { decltype(r.dimv)(Dim { BAD, 0 }), view.data() };
    std::array<int, sizeof...(Iarg)> s {{ Iarg ... }};
    for (int k=0; int sk: s) {
        Dim & dest = r.dimv[sk];
        dest.step += view.dimv[k].step;
        dest.len = dest.len>=0 ? std::min(dest.len, view.dimv[k].len) : view.dimv[k].len;
        ++k;
    }
    return r;
}

template <class T, rank_t RANK> inline
auto diag(View<T, RANK> const & view)
{
    return transpose<0, 0>(view);
}

template <class T, rank_t RANK> inline
bool is_ravel_free(View<T, RANK> const & a)
{
    int r = a.rank()-1;
    for (; r>=0 && a.len(r)==1; --r) {}
    if (r<0) { return true; }
    ra::dim_t s = a.step(r)*a.len(r);
    while (--r>=0) {
        if (1!=a.len(r)) {
            if (a.step(r)!=s) {
                return false;
            }
            s *= a.len(r);
        }
    }
    return true;
}

template <class T, rank_t RANK> inline
View<T, 1> ravel_free(View<T, RANK> const & a)
{
    RA_CHECK(is_ravel_free(a));
    int r = a.rank()-1;
    for (; r>=0 && a.len(r)==1; --r) {}
    ra::dim_t s = r<0 ? 1 : a.step(r);
    return ra::View<T, 1>({{size(a), s}}, a.cp);
}

template <class T, rank_t RANK, class S> inline
auto reshape_(View<T, RANK> const & a, S && sb_)
{
    auto sb = concrete(std::forward<S>(sb_));
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
    View<T, ra::size_s(sb)> b(map([](auto i) { return Dim { BAD, 0 }; }, ra::iota(ra::size(sb))), a.data());
    rank_t i = 0;
    for (; i<a.rank() && i<b.rank(); ++i) {
        if (sa[a.rank()-i-1]!=sb[b.rank()-i-1]) {
            assert(is_ravel_free(a) && "reshape w/copy not implemented");
            if (la>=lb) {
// FIXME View(SS const & s, T * p). Cf [ra37].
                b.filldim(sb);
                for (int j=0; j!=b.rank(); ++j) {
                    b.dimv[j].step *= a.step(a.rank()-1);
                }
                return b;
            } else {
                assert(0 && "reshape case not implemented");
            }
        } else {
// select
            b.dimv[b.rank()-i-1] = a.dimv[a.rank()-i-1];
        }
    }
    if (i==a.rank()) {
// tile & return
        for (rank_t j=i; j<b.rank(); ++j) {
            b.dimv[b.rank()-j-1] = { sb[b.rank()-j-1], 0 };
        }
    }
    return b;
}

template <class T, rank_t RANK, class S> inline
auto reshape(View<T, RANK> const & a, S && sb_)
{
    return reshape_(a, std::forward<S>(sb_));
}

// We need dimtype bc {1, ...} deduces to int and that fails to match ra::dim_t.
// We could use initializer_list to handle the general case, but that would produce a var rank result because its size cannot be deduced at compile time :-/. Unfortunately an initializer_list specialization would override this one, so we cannot provide it as a fallback.
template <class T, rank_t RANK, class dimtype, int N> inline
auto reshape(View<T, RANK> const & a, dimtype const (&sb_)[N])
{
    return reshape_(a, sb_);
}

// lo = lower bounds, hi = upper bounds.
// The stencil indices are in [0 lo+1+hi] = [-lo +hi].
template <class LO, class HI, class T, rank_t N> inline
View<T, rank_sum(N, N)>
stencil(View<T, N> const & a, LO && lo, HI && hi)
{
    View<T, rank_sum(N, N)> s;
    s.cp = a.data();
    ra::resize(s.dimv, 2*a.rank());
    RA_CHECK(every(lo>=0));
    RA_CHECK(every(hi>=0));
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

// Make last sizes of View<> be compile-time constants.
template <class super_t, rank_t SUPERR, class T, rank_t RANK> inline
auto explode_(View<T, RANK> const & a)
{
// TODO Reduce to single check, either the first or the second.
    static_assert(RANK>=SUPERR || RANK==ANY, "rank of a is too low");
    RA_CHECK(a.rank()>=SUPERR, "Rank of a ", a.rank(), " should be at least ", SUPERR, ".");
    View<super_t, rank_sum(RANK, -SUPERR)> b;
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

template <class super_t, class T, rank_t RANK> inline
auto explode(View<T, RANK> const & a)
{
    return explode_<super_t, (std::is_same_v<super_t, std::complex<T>> ? 1 : rank_s<super_t>())>(a);
}

// FIXME Maybe namespace level generics in atom.hh
template <class T> inline int gstep(int i) { if constexpr (is_scalar<T>) return 1; else return T::step(i); }
template <class T> inline int glen(int i) { if constexpr (is_scalar<T>) return 1; else return T::len(i); }

// TODO This routine is not totally safe; the ranks below SUBR must be compact, which is not checked.
template <class sub_t, class super_t, rank_t RANK> inline
auto collapse(View<super_t, RANK> const & a)
{
    using super_v = value_t<super_t>;
    using sub_v = value_t<sub_t>;
    constexpr int subtype = sizeof(super_v)/sizeof(sub_t);
    constexpr int SUBR = rank_s<super_t>() - rank_s<sub_t>();

    View<sub_t, rank_sum(RANK, SUBR+int(subtype>1))> b;
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
        RA_CHECK(((gstep<super_t>(i)/s)*s==gstep<super_t>(i)), "Bad steps."); // TODO is actually static
        b.dimv[a.rank()+i] = { .len = glen<super_t>(i), .step = gstep<super_t>(i) / s * t };
    }
    if (subtype>1) {
        b.dimv[a.rank()+SUBR] = { .len = t, .step = 1 };
    }
    b.cp = reinterpret_cast<sub_t *>(a.data());
    return b;
}

// For functions that require compact arrays (TODO they really shouldn't).
template <class A> inline
bool const crm(A const & a)
{
    return ra::size(a)==0 || is_c_order(a);
}

} // namespace ra
