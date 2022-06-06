// -*- mode: c++; coding: utf-8 -*-
// ra-ra - Arrays with dynamic size, cf small.hh.

// (c) Daniel Llorens - 2013-2022
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#pragma once
#include "small.hh"
#include <memory>
#include <iosfwd>

namespace ra {

// Dope vector element
struct Dim { dim_t len, step; };

// For debugging
inline std::ostream & operator<<(std::ostream & o, Dim const & dim)
{ o << "[Dim " << dim.len << " " << dim.step << "]"; return o; }


// --------------------
// Develop indices for Big
// --------------------

namespace indexer1 {

    template <class Dimv, class P>
    constexpr dim_t shorter(Dimv const & dimv, P && p)
    {
        RA_CHECK(ssize(dimv)>=start(p).len(0), "Too many indices");
// use dim.data() to skip the size check.
        dim_t c = 0;
        for_each([&c](auto && d, auto && p) { RA_CHECK(inside(p, d.len)); c += d.step*p; },
                 ptr(dimv.data()), p);
        return c;
    }

// for rank matching on rank<driving rank, no slicing. TODO Static check?
    template <class Dimv, class P>
    constexpr dim_t longer(rank_t framer, Dimv const & dimv, P const & p)
    {
        RA_CHECK(framer<=ssize(p), "Too few indices");
        dim_t c = 0;
        for (rank_t k=0; k<framer; ++k) {
            RA_CHECK(inside(p[k], dimv[k].len) || (dimv[k].len==DIM_BAD && dimv[k].step==0));
            c += dimv[k].step * p[k];
        }
        return c;
    }

} // namespace indexer1


// --------------------
// Big iterator
// --------------------
// TODO Refactor with cell_iterator_small

// V is View. FIXME Parameterize? apparently only for order-of-decl.
template <class V, rank_t cellr_=0>
struct cell_iterator_big
{
    constexpr static rank_t cellr_spec = cellr_;
    static_assert(cellr_spec!=RANK_ANY && cellr_spec!=RANK_BAD, "bad cell rank");
    constexpr static rank_t fullr = ra::rank_s<V>();
    constexpr static rank_t cellr = dependent_cell_rank(fullr, cellr_spec);
    constexpr static rank_t framer = dependent_frame_rank(fullr, cellr_spec);
    static_assert(cellr>=0 || cellr==RANK_ANY, "bad cell rank");
    static_assert(framer>=0 || framer==RANK_ANY, "bad frame rank");
    static_assert(fullr==cellr || gt_rank(fullr, cellr), "bad cell rank");

    using Dimv_ = typename std::decay_t<V>::Dimv;
    using Dimv = std::conditional_t<std::is_lvalue_reference_v<V>, Dimv_ const &, Dimv_>;
    Dimv dimv;

    constexpr static rank_t rank_s() { return framer; }
    constexpr rank_t rank() const { return dependent_frame_rank(rank_t(dimv.size()), cellr_spec); }

    using shape_type = std::conditional_t<rank_s()==DIM_ANY, std::vector<dim_t>,
                                          Small<dim_t, rank_s()==DIM_ANY ? 0 : rank_s()>>; // still needs protection :-/
    using atom_type = std::remove_reference_t<decltype(*(std::declval<V>().data()))>;
    using cell_type = View<atom_type, cellr>;
    using value_type = std::conditional_t<0==cellr, atom_type, cell_type>;

    cell_type c;

    constexpr cell_iterator_big(cell_iterator_big const & ci): dimv(ci.dimv), c { ci.c.dimv, ci.c.p } {}
// s_ is array's full shape; split it into dimv/i (frame) and c (cell).
    constexpr cell_iterator_big(Dimv const & dimv_, atom_type * p_): dimv(dimv_)
    {
        rank_t rank = this->rank();
// see stl_iterator for the case of dimv_[0]=0, etc. [ra12].
        c.p = p_;
        rank_t cellr = dependent_cell_rank(rank_t(dimv.size()), cellr_spec);
        resize(c.dimv, cellr);
        for (int k=0; k<cellr; ++k) {
            c.dimv[k] = dimv_[k+rank];
        }
    }

    constexpr static dim_t len_s(int i) { /* RA_CHECK(inside(k, rank())); */ return DIM_ANY; }
    constexpr dim_t len(int k) const { RA_CHECK(inside(k, rank())); return dimv[k].len; }
    constexpr dim_t step(int k) const { return k<rank() ? dimv[k].step : 0; }
// FIXME handle z or j over rank()? check cell_iterator_small versions.
    constexpr bool keep_step(dim_t st, int z, int j) const { return st*step(z)==step(j); }
    constexpr void adv(rank_t k, dim_t d) { c.p += step(k)*d; }

    constexpr
    auto flat() const
    {
        if constexpr (0==cellr) {
            return c.p;
        } else {
            return CellFlat<cell_type> { c };
        }
    }
// Return type to allow either View & or View const & verb. Can't set self bc original p isn't kept. TODO Think this over.
#define RA_CONST_OR_NOT(CONST)                                          \
    template <class I> constexpr                                        \
    decltype(auto) at(I const & i_) CONST                               \
    {                                                                   \
        if constexpr (0==cellr) {                                       \
            return c.p[indexer1::longer(rank(), dimv, i_)];             \
        } else {                                                        \
            return cell_type { c.dimv, c.p + indexer1::longer(rank(), dimv, i_) }; \
        }                                                               \
    }
    FOR_EACH(RA_CONST_OR_NOT, /*const*/, const)
#undef RA_CONST_OR_NOT
    RA_DEF_ASSIGNOPS_DEFAULT_SET
};


// --------------------
// nested braces for Container initializers
// --------------------

// Avoid clash of T with scalar constructors (for rank 0).
template <class T, rank_t rank>
struct nested_braces { using list = no_arg; };

template <class T, rank_t rank>
requires (rank==1)
struct nested_braces<T, rank>
{
    using list = std::initializer_list<T>;
    template <std::size_t N> constexpr static
    void shape(list l, std::array<dim_t, N> & s)
    {
        static_assert(N>0);
        s[N-1] = l.size();
    }
};

template <class T, rank_t rank>
requires (rank>1)
struct nested_braces<T, rank>
{
    using sub = nested_braces<T, rank-1>;
    using list = std::initializer_list<typename sub::list>;
    template <std::size_t N> constexpr static
    void shape(list l, std::array<dim_t, N> & s)
    {
        s[N-rank] = l.size();
        if (l.size()>0) {
            sub::template shape(*(l.begin()), s);
        } else {
            std::fill(s.begin()+N-rank+1, s.end(), 0);
        }
    }
};


// --------------------
// Indexing views
// --------------------

// Always C order. If you need another, transpose this.
// Works on dim vector with sizes already assigned, so that I can work from an expr. Not pretty though.
template <class D>
dim_t filldim(int n, D dend)
{
    dim_t next = 1;
    for (; n>0; --n) {
        --dend;
        RA_CHECK((*dend).len>=0, "bad dim", (*dend).len);
        (*dend).step = next;
        next *= (*dend).len;
    }
    return next;
}

template <class D>
dim_t proddim(D d, D dend)
{
    dim_t t = 1;
    for (; d!=dend; ++d) {
        t *= (*d).len;
    }
    return t;
}

// raw <- shared; raw <- unique; shared <-- unique.
// layout is
// [data] (fixed shape)
// [size] p -> [data...] (fixed rank)
// [rank] [size...] p -> [data...] (var rank)
// TODO size is immutable so that it can be kept together with rank.

inline constexpr dim_t
select(Dim * dim, Dim const * dim_src, dim_t i)
{
    RA_CHECK(inside(i, dim_src->len), " i ", i, " len ", dim_src->len);
    return dim_src->step*i;
}
template <class II>
inline dim_t
select(Dim * dim, Dim const * dim_src, ra::Iota<II> i)
{
    RA_CHECK((inside(i.i_, dim_src->len) && inside(i.i_+(i.len_-1)*i.step_, dim_src->len))
             || (i.len_==0 && i.i_<=dim_src->len));
    *dim = { .len = i.len_, .step = dim_src->step * i.step_ };
    return dim_src->step*i.i_;
}
template <class I0, class ... I>
inline dim_t
select_loop(Dim * dim, Dim const * dim_src, I0 && i0, I && ... i)
{
    return select(dim, dim_src, std::forward<I0>(i0))
        + select_loop(dim+is_beatable<I0>::skip, dim_src+is_beatable<I0>::skip_src, std::forward<I>(i) ...);
}
template <int n, class ... I>
inline dim_t
select_loop(Dim * dim, Dim const * dim_src, dots_t<n> dots, I && ... i)
{
    for (Dim * end = dim+n; dim!=end; ++dim, ++dim_src) {
        *dim = *dim_src;
    }
    return select_loop(dim, dim_src, std::forward<I>(i) ...);
}
template <int n, class ... I>
inline dim_t
select_loop(Dim * dim, Dim const * dim_src, insert_t<n> insert, I && ... i)
{
    for (Dim * end = dim+n; dim!=end; ++dim) {
        *dim = { .len = DIM_BAD, .step = 0 };
    }
    return select_loop(dim, dim_src, std::forward<I>(i) ...);
}
inline constexpr
dim_t select_loop(Dim * dim, Dim const * dim_src)
{
    return 0;
}

// allow conversion only from T to T const with null type.
template <class T> using const_atom = std::conditional_t<std::is_const_v<T>, no_arg, T const>;


// --------------------
// View
// --------------------

// TODO Parameterize on Child having .data() so that there's only one pointer.
// TODO A constructor, if only for RA_CHECK (nonnegative lens, steps inside, etc.)
template <class T, rank_t RANK>
struct View
{
    using Dimv = std::conditional_t<RANK==RANK_ANY, std::vector<Dim>, Small<Dim, RANK==RANK_ANY ? 0 : RANK>>;

    Dimv dimv;
    T * p;

    constexpr static rank_t rank_s() { return RANK; };
    constexpr static rank_t rank() requires (RANK!=RANK_ANY) { return RANK; }
    constexpr rank_t rank() const requires (RANK==RANK_ANY) { return rank_t(dimv.size()); }
    constexpr static dim_t len_s(int j) { return DIM_ANY; }
    constexpr dim_t len(int j) const
    {
        if constexpr (RANK==RANK_ANY) {
            RA_CHECK(j<rank(), " j : ", j, " rank ", rank());
        }
        return dimv[j].len;
    }
    constexpr dim_t step(int j) const
    {
        if constexpr (RANK==RANK_ANY) {
            RA_CHECK(j<rank(), " j : ", j, " rank ", rank());
        }
        return dimv[j].step;
    }
    constexpr auto data() { return p; }
    constexpr auto data() const { return p; } // [ra47]
    constexpr dim_t size() const { return proddim(dimv.begin(), dimv.end()); }

// FIXME Remove, too dangerous. View can be a deduced type (e.g. from value_t<X>)
    constexpr View(): p(nullptr) {}
// Constructors using pointers need extra care
    constexpr View(Dimv const & dimv_, T * p_): dimv(dimv_), p(p_) {} // [ra36]
    template <class SS>
    View(SS && s, T * p_): p(p_)
    {
        if constexpr (std::is_convertible_v<value_t<SS>, Dim>) {
            ra::resize(View::dimv, start(s).len(0)); // [ra37]
            start(View::dimv) = s;
        } else {
            ra::resize(View::dimv, start(s).len(0)); // [ra37]
            for_each([](Dim & dim, auto && s) { dim.len = s; }, View::dimv, s);
            filldim(View::dimv.size(), View::dimv.end());
        }
    }
    View(std::initializer_list<dim_t> s, T * p_): View(start(s), p_) {}

// [ra38] [ra34] and RA_DEF_ASSIGNOPS_SELF
    View(View && x) = default;
    View(View const & x) = default;
    View & operator=(View && x) { ra::start(*this) = x; return *this; }
    View & operator=(View const & x) { ra::start(*this) = x; return *this; }
// see same thing for SmallBase.
#define DEF_ASSIGNOPS(OP)                                               \
    template <class X> View & operator OP (X && x) { ra::start(*this) OP x; return *this; }
    FOR_EACH(DEF_ASSIGNOPS, =, *=, +=, -=, /=)
#undef DEF_ASSIGNOPS

// array type is not deduced by (X &&)
    constexpr View &
    operator=(typename nested_braces<T, RANK>::list x)
    {
        ra::iter<-1>(*this) = x;
        return *this;
    }
// braces row-major ravel for rank!=1
    using ravel_arg = std::conditional_t<RANK==1, no_arg, std::initializer_list<T>>;
    View & operator=(ravel_arg const x)
    {
        RA_CHECK(p && this->size()==ssize(x), "bad assignment");
        std::copy(x.begin(), x.end(), this->begin());
        return *this;
    }
    bool const empty() const { return 0==size(); } // TODO Optimize

    template <rank_t c=0> constexpr auto iter() && { return ra::cell_iterator_big<View<T, RANK>, c>(std::move(dimv), p); }
    template <rank_t c=0> constexpr auto iter() & { return ra::cell_iterator_big<View<T, RANK> &, c>(dimv, p); }
    template <rank_t c=0> constexpr auto iter() const & { return ra::cell_iterator_big<View<T const, RANK> &, c>(dimv, p); }
    constexpr auto begin() const { return stl_iterator(iter()); }
    constexpr auto begin() { return stl_iterator(iter()); }
// here dim doesn't matter, but we have to give it if it's a ref
    constexpr auto end() const { return stl_iterator(decltype(iter())(dimv, nullptr)); }
    constexpr auto end() { return stl_iterator(decltype(iter())(dimv, nullptr)); }

// Specialize for rank() integer-args -> scalar, same in ra::SmallBase in small.hh.
#define RA_CONST_OR_NOT(CONST)                                          \
    template <class ... I>                                              \
    requires ((0 + ... + std::is_integral_v<std::decay_t<I>>)<RANK      \
              && (0 + ... + is_beatable<I>::value)==sizeof...(I))       \
    auto operator()(I && ... i) CONST                                   \
        requires (RANK!=RANK_ANY)                                       \
    {                                                                   \
        constexpr rank_t extended = (0 + ... + (is_beatable<I>::skip-is_beatable<I>::skip_src)); \
        constexpr rank_t subrank = rank_sum(RANK, extended);            \
        static_assert(subrank>=0, "bad subrank");                       \
        View<T CONST, subrank> sub;                                     \
        sub.p = data() + select_loop(sub.dimv.data(), this->dimv.data(), i ...); \
        /* fill the rest of dim, skipping over beatable subscripts */   \
        for (int i=(0 + ... + is_beatable<I>::skip); i<subrank; ++i) {  \
            sub.dimv[i] = this->dimv[i-extended];                       \
        }                                                               \
        return sub;                                                     \
    }                                                                   \
    /* BUG doesn't handle generic zero-rank indices */                  \
    template <class ... I>                                              \
    requires ((0 + ... + std::is_integral_v<I>)==RANK)                  \
    decltype(auto) operator()(I const & ... i) CONST                    \
        requires (RANK!=RANK_ANY)                                       \
    {                                                                   \
        return data()[select_loop(nullptr, this->dimv.data(), i ...)];  \
    }                                                                   \
    /* Contrary to RANK!=RANK_ANY, the scalar case cannot be separated at compile time. So operator() will return a rank 0 view in that case (and rely on conversion if, say, this ends up assigned to a scalar) */ \
    template <class ... I>                                              \
    requires (is_beatable<I>::value && ...)                             \
    auto operator()(I && ... i) CONST                                   \
        requires (RANK==RANK_ANY)                                       \
    {                                                                   \
        constexpr rank_t extended = (0 + ... + (is_beatable<I>::skip-is_beatable<I>::skip_src)); \
        assert(this->rank()+extended>=0);                               \
        View<T CONST, RANK_ANY> sub;                                    \
        sub.dimv.resize(this->rank()+extended);                         \
        sub.p = data() + select_loop(sub.dimv.data(), this->dimv.data(), i ...); \
        for (int i=(0 + ... + is_beatable<I>::skip); i<sub.rank(); ++i) { \
            sub.dimv[i] = this->dimv[i-extended];                       \
        }                                                               \
        return sub;                                                     \
    }                                                                   \
    template <class  I>                                                 \
    auto at(I && i) CONST                                               \
    {                                                                   \
        if constexpr (RANK_ANY!=RANK)                                   \
        {                                                               \
            constexpr rank_t subrank = rank_diff(RANK, ra::size_s<I>()); \
            using Sub = View<T CONST, subrank>;                         \
            if constexpr (RANK_ANY==subrank) {                          \
                return Sub { typename Sub::Dimv(dimv.begin()+ra::size(i), dimv.end()),  /* Dimv is std::vector */ \
                        data() + indexer1::shorter(dimv, i) };          \
            } else {                                                    \
                return Sub { typename Sub::Dimv(ptr(dimv.begin()+ra::size(i))),  /* Dimv is ra::Small */ \
                        data() + indexer1::shorter(dimv, i) };          \
            }                                                           \
        } else {                                                        \
            return View<T CONST, RANK_ANY> { Dimv(dimv.begin()+i.size(), dimv.end()), /* Dimv is std::vector */ \
                    data() + indexer1::shorter(dimv, i) };              \
        }                                                               \
    }                                                                   \
    /* TODO > 1 selector... This still covers (unbeatable, integer) for example, which could be reduced. */ \
    template <class ... I>                                              \
    requires (!(is_beatable<I>::value && ...))                          \
    auto operator()(I && ... i) CONST                                   \
    {                                                                   \
        return from(*this, std::forward<I>(i) ...);                     \
    }                                                                   \
    constexpr decltype(auto) operator[](dim_t const i) CONST            \
    {                                                                   \
        return (*this)(i);                                              \
    }                                                                   \
    /* conversion to scalar */                                          \
    operator T CONST & () CONST                                         \
    {                                                                   \
        if constexpr (RANK_ANY==RANK) {                                 \
            RA_CHECK(rank()==0, "converting rank ", rank(), " to scalar"); \
        } else {                                                        \
            static_assert(0==RANK, "bad rank");                         \
        }                                                               \
        return data()[0];                                               \
    }
    FOR_EACH(RA_CONST_OR_NOT, /*const*/, const)
#undef RA_CONST_OR_NOT

// conversion from var rank to fixed rank
    template <rank_t R>
    requires (R==RANK_ANY && R!=RANK)
    View(View<T const, R> const & x) requires (RANK!=RANK_ANY): dimv(x.dimv), p(x.p) {}
// conversion from var rank & non const to fixed rank and const
    template <rank_t R>
    requires (R==RANK_ANY && R!=RANK)
    View(View<std::remove_const_t<T>, R> const & x) requires (RANK!=RANK_ANY): dimv(x.dimv), p(x.p) {}
// conversion from fixed rank to var rank
    template <rank_t R>
    requires (R!=RANK_ANY)
    View(View<T const, R> const & x) requires (RANK==RANK_ANY): dimv(x.dimv.begin(), x.dimv.end()), p(x.p) {}
// conversion from fixed rank & non const to var rank and const
    template <rank_t R>
    requires (R!=RANK_ANY)
    View(View<std::remove_const_t<T>, R> const & x) requires (RANK==RANK_ANY): dimv(x.dimv.begin(), x.dimv.end()), p(x.p) {}
// conversion to const from non const
    operator View<const_atom<T>, RANK> const & () const
    {
        return *reinterpret_cast<View<const_atom<T>, RANK> const *>(this);
    }
};


// --------------------
// Container types
// --------------------

template <class V> struct storage_traits
{
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    static V create(dim_t n) { RA_CHECK(n>=0); return V(new T[n]); }
    static T const * data(V const & v) { return v.get(); }
    static T * data(V & v) { return v.get(); }
};

template <class P> struct storage_traits<std::shared_ptr<P>>
{
    using V = std::shared_ptr<P>;
    using T = std::decay_t<decltype(*std::declval<V>().get())>;
    static V create(dim_t n) { RA_CHECK(n>=0); return V(new T[n], std::default_delete<T[]>()); }
    static T const * data(V const & v) { return v.get(); }
    static T * data(V & v) { return v.get(); }
};

template <class T_, class A> struct storage_traits<std::vector<T_, A>>
{
    using T = T_;
    static_assert(!std::is_same_v<std::remove_const_t<T>, bool>, "No pointers to bool in std::vector<bool>");
    static std::vector<T, A> create(dim_t n) { return std::vector<T, A>(n); }
    static T const * data(std::vector<T, A> const & v) { return v.data(); }
    static T * data(std::vector<T, A> & v) { return v.data(); }
};

template <class T, rank_t RANK>
inline constexpr bool
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

// TODO be convertible to View only, so that View::p is not duplicated
template <class Store, rank_t RANK>
struct Container: public View<typename storage_traits<Store>::T, RANK>
{
    Store store;
    using T = typename storage_traits<Store>::T;
    using View = ra::View<T, RANK>;
    using View::rank_s;

    using shape_arg = typename decltype(std::declval<View>().iter())::shape_type;

    View & view() { return *this; }
    View const & view() const { return *this; }

    template <class ... A> decltype(auto) operator()(A && ... a) { return View::operator()(std::forward<A>(a) ...); }
    template <class ... A> decltype(auto) operator()(A && ... a) const { return View::operator()(std::forward<A>(a) ...); }

// Needed to set View::p. FIXME Remove duplication as in SmallBase/SmallArray, then remove the constructors and the assignment operators.
    Container(Container && w): store(std::move(w.store))
    {
        View::dimv = std::move(w.dimv);
        View::p = storage_traits<Store>::data(store);
    }
    Container(Container const & w): store(w.store)
    {
        View::dimv = w.dimv;
        View::p = storage_traits<Store>::data(store);
    }
    Container(Container & w): store(w.store)
    {
        View::dimv = w.dimv;
        View::p = storage_traits<Store>::data(store);
    }

// Override View::operator= to allow initialization-of-reference. Unfortunately operator>>(std::istream &, Container &) requires it. The presence of these operator= means that A(shape 2 3) = type-of-A [1 2 3] initializes so it doesn't behave as A(shape 2 3) = not-type-of-A [1 2 3] which will use View::operator= and frame match. See test/ownership.cc [ra20].
// TODO do this through .set() op.
// TODO don't require copiable T from constructors, see fill1 below. That requires initialization and not update semantics for operator=.
    Container & operator=(Container && w)
    {
        store = std::move(w.store);
        View::dimv = std::move(w.dimv);
        View::p = storage_traits<Store>::data(store);
        return *this;
    }
    Container & operator=(Container const & w)
    {
        store = w.store;
        View::dimv = w.dimv;
        View::p = storage_traits<Store>::data(store);
        return *this;
    }
    Container & operator=(Container & w)
    {
        store = w.store;
        View::dimv = w.dimv;
        View::p = storage_traits<Store>::data(store);
        return *this;
    }

// Provided so that {} calls shape_arg constructor below.
    Container()
    {
        if constexpr (RANK_ANY==RANK) {
            View::dimv = { Dim {0, 1} }; // rank 1 to avoid store init
        } else {
            for (Dim & dimi: View::dimv) { dimi = {0, 1}; } // 1 so we can push_back()
        }
    }

    template <class S> void init(S && s)
    {
        static_assert(!std::is_convertible_v<value_t<S>, Dim>);
// no rank extension here, because it's error prone and not very useful.
        static_assert(1==ra::rank_s<S>(), "rank mismatch for init shape");
// [ra37] Need two parts because Dimv might be STL type. Otherwise I'd just View::dimv.set(map(...)).
        if constexpr (RANK_ANY==RANK) {
            ra::resize(View::dimv, ra::size(s));
        }
        for_each([](Dim & dim, auto const & s) { dim.len = s; }, View::dimv, s);
        dim_t t = filldim(View::dimv.size(), View::dimv.end());
        store = storage_traits<Store>::create(t);
        View::p = storage_traits<Store>::data(store);
    }

// FIXME use of fill1 requires T to be copiable, this is unfortunate as it conflicts with the semantics of view_.operator=.
// store(x) avoids it for Big, but doesn't work for Unique. Should construct in place like std::vector does.
    template <class Pbegin> void fill1(dim_t xsize, Pbegin xbegin)
    {
        RA_CHECK(this->size()==xsize, "mismatched sizes");
        std::copy_n(xbegin, xsize, this->begin()); // TODO Use xpr traversal.
    }

// shape_arg overloads handle {...} arguments. Size check is courtesy of conversion (if shape_arg is Small) or init().

// explicit shape.
    Container(shape_arg const & s, none_t) { init(s); }
    template <class XX> Container(shape_arg const & s, XX && x): Container(s, none) { view() = x; }

// shape from data.
    template <class XX> Container(XX && x): Container(ra::shape(x), none) { view() = x; }
    Container(typename nested_braces<T, RANK>::list x)
    {
        static_assert(RANK!=RANK_ANY);
        std::array<dim_t, RANK> s;
        nested_braces<T, RANK>::template shape(x, s);
        init(s);
        view() = x;
    }

// braces row-major ravel for rank!=1
    Container(typename View::ravel_arg x)
        : Container({dim_t(x.size())}, none) { fill1(x.size(), x.begin()); }

// shape + row-major ravel. // TODO Maybe remove these? See also small.hh.
    Container(shape_arg const & s, std::initializer_list<T> x)
        : Container(s, none) { fill1(x.size(), x.begin()); }
    template <class TT>
    Container(shape_arg const & s, TT * p)
        : Container(s, none) { fill1(this->size(), p); }
    template <class P>
    Container(shape_arg const & s, P pbegin, P pend)
        : Container(s, none) { fill1(this->size(), pbegin); }

// these are needed when shape_arg is std::vector, since that doesn't handle conversions like Small does.
    template <class SS> Container(SS && s, none_t) { init(s); }
    template <class SS, class XX> Container(SS && s, XX && x): Container(s, none) { view() = x; }
    template <class SS> Container(SS const & s, std::initializer_list<T> x)
        : Container(s, none) { fill1(x.size(), x.begin()); }

    using View::operator=;

// resize first axis. Only for some kinds of store.
    void resize(dim_t const s)
    {
        static_assert(RANK==RANK_ANY || RANK>0); RA_CHECK(this->rank()>0);
        store.resize(proddim(View::dimv.begin()+1, View::dimv.end())*s);
        View::dimv[0].len = s;
        View::p = store.data();
    }
    void resize(dim_t const s, T const & t)
    {
        static_assert(RANK==RANK_ANY || RANK>0); RA_CHECK(this->rank()>0);
        store.resize(proddim(View::dimv.begin()+1, View::dimv.end())*s, t);
        View::dimv[0].len = s;
        View::p = store.data();
    }
// resize full shape. Only for some kinds of store.
    template <class S>
    requires (ra::rank_s<S>() > 0)
    void resize(S const & s)
    {
        ra::resize(View::dimv, start(s).len(0)); // [ra37] FIXME is View constructor
        for_each([](Dim & dim, auto && s) { dim.len = s; }, View::dimv, s);
        filldim(View::dimv.size(), View::dimv.end());
        store.resize(proddim(View::dimv.begin(), View::dimv.end()));
        View::p = store.data();
    }
// lets us move. A template + std::forward wouldn't work for push_back(brace-enclosed-list).
    void push_back(T && t)
    {
        static_assert(RANK==1 || RANK==RANK_ANY); RA_CHECK(this->rank()==1);
        store.push_back(std::move(t));
        ++View::dimv[0].len;
        View::p = store.data();
    }
    void push_back(T const & t)
    {
        static_assert(RANK==1 || RANK==RANK_ANY); RA_CHECK(this->rank()==1);
        store.push_back(t);
        ++View::dimv[0].len;
        View::p = store.data();
    }
    template <class ... A>
    void emplace_back(A && ... a)
    {
        static_assert(RANK==1 || RANK==RANK_ANY); RA_CHECK(this->rank()==1);
        store.emplace_back(std::forward<A>(a) ...);
        ++View::dimv[0].len;
        View::p = store.data();
    }
    void pop_back()
    {
        static_assert(RANK==1 || RANK==RANK_ANY); RA_CHECK(this->rank()==1);
        RA_CHECK(View::dimv[0].len>0);
        store.pop_back();
        --View::dimv[0].len;
    }
    bool empty() const
    {
        return this->size()==0;
    }
    T const & back() const { RA_CHECK(this->rank()==1 && this->size()>0); return store[this->size()-1]; }
    T & back() { RA_CHECK(this->rank()==1 && this->size()>0); return store[this->size()-1]; }

// Container is always compact/row-major. Then the 0-rank STL-like iterators can be raw pointers. TODO But .iter() should also be able to benefit from this constraint, and the check should be faster for some cases (like RANK==1) or ellidable.

    auto begin() { assert(is_c_order(*this)); return this->data(); }
    auto begin() const { assert(is_c_order(*this)); return this->data(); }
    auto end() { return this->data()+this->size(); }
    auto end() const { return this->data()+this->size(); }
};

template <class Store, rank_t RANKA, rank_t RANKB>
requires (RANKA!=RANKB) // rely on std::swap; else ambiguous
void
swap(Container<Store, RANKA> & a, Container<Store, RANKB> & b)
{
    if constexpr (RANK_ANY==RANKA) {
        RA_CHECK(a.rank()==b.rank());
        decltype(b.dimv) c = a.dimv;
        start(a.dimv) = b.dimv;
        std::swap(b.dimv, c);
    } else if constexpr (RANK_ANY==RANKB) {
        RA_CHECK(a.rank()==b.rank());
        decltype(a.dimv) c = b.dimv;
        start(b.dimv) = a.dimv;
        std::swap(a.dimv, c);
    } else {
        static_assert(RANKA==RANKB);
        std::swap(a.dimv, b.dimv);
    }
    std::swap(a.store, b.store);
    std::swap(a.p, b.p);
}

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

// Beyond this, we probably should have fixed-size (~std::dynarray), resizeable (~std::vector).
template <class T, rank_t RANK=RANK_ANY> using Big = Container<std::vector<T, default_init_allocator<T>>, RANK>;
template <class T, rank_t RANK=RANK_ANY> using Unique = Container<std::unique_ptr<T []>, RANK>;
template <class T, rank_t RANK=RANK_ANY> using Shared = Container<std::shared_ptr<T>, RANK>;

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
    a.p = raw.p;
    a.store = std::shared_ptr<T>(raw.data(), [](T *) {});
    return a;
}

} // namespace ra
